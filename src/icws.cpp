#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <pthread.h>
#include <deque>
#include <iostream>
#include <poll.h>

#include "simple_work_queue.hpp"
#define ACTIVE 0
#define INACTIVE -1
extern "C"
{
#include "pcsa_net.h"
#include "parse.h"
}
using namespace std;
#define MAXBUF 1024

pthread_t *thread_array;
pthread_mutex_t jobs_mutex;
pthread_cond_t condition_variable;
deque<int> q;

static struct option long_options[] =
    {
        {"port", required_argument, NULL, 'p'},
        {"root", required_argument, NULL, 'r'},
        {"numThreads", required_argument, NULL, 'n'},
        {"timeout", required_argument, NULL, 't'},
        {NULL, 0, NULL, 0}};

typedef struct
{
    // struct threadpool *pool;
    char *port;
    char *rootFolder;
    int numThreads;
    int timeout;
    int connFd;
    char buf[MAXBUF];
    pthread_mutex_t add_mutex;
} Field;

typedef struct sockaddr SA;
void showdq(deque<int> g)
{
    deque<int>::iterator it;
    for (it = g.begin(); it != g.end(); ++it)
        cout << '\t' << *it;
    cout << '\n';
}
char *gettDateTime(void)
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    return asctime(tm);
}
void response_404(int connFd, char *buffer)
{
    char *msg = "404 Not Found\n";
    sprintf(buffer,
            "HTTP/1.1 404 Not Found\r\n"
            "Date: %s"
            "Server: icws\r\n"
            "Connection: close\r\n\r\n",
            gettDateTime());
    write_all(connFd, buffer, strlen(buffer));
    write_all(connFd, msg, strlen(msg));
}
void response_400(int connFd, char *buffer)
{
    char *msg = "400 Bad request\n";
    sprintf(buffer,
            "HTTP/1.1 400 Bad request\r\n"
            "Date: %s"
            "Server: icws\r\n"
            "Connection: close\r\n\r\n",
            gettDateTime());
    write_all(connFd, buffer, strlen(buffer));
    write_all(connFd, msg, strlen(msg));
}
void response_501(int connFd, char *buffer)
{
    char *msg = "501\n";
    sprintf(buffer,
            "HTTP/1.1 501 HTTP Method Not Unimplemented\r\n"
            "Date: %s\r\n"
            "Server: icws\r\n"
            "Connection: close\r\n\r\n",
            gettDateTime());
    write_all(connFd, buffer, strlen(buffer));
    write_all(connFd, msg, strlen(msg));
}

void response_505(int connFd, char *buffer)
{
    char *msg = "505\n";
    sprintf(buffer,
            "HTTP/1.1 505 HTTP Version Not Supported\r\n"
            "Date: %s\r\n"
            "Server: icws\r\n"
            "Connection: close\r\n\r\n",
            gettDateTime());
    write_all(connFd, buffer, strlen(buffer));
    write_all(connFd, msg, strlen(msg));
}
void write_logic(int connFd, int outputFd)
{
    ssize_t bytesRead;
    char buf[MAXBUF];

    while ((bytesRead = read(connFd, buf, MAXBUF)) > 0)
    {
        ssize_t numToWrite = bytesRead;
        char *writeBuf = buf;
        while (numToWrite > 0)
        {
            ssize_t numWritten = write(outputFd, writeBuf, numToWrite);
            if (numWritten < 0)
            {
                fprintf(stderr, "ERROR writing, meh\n");
                break;
            }
            numToWrite -= numWritten;
            writeBuf += numWritten;
        }
    }
    printf("DEBUG: Connection closed\n");
}
void respond_all_head(int connFd, char *uri, char *mime)
{
    char buf[MAXBUF];
    int uriFd = open(uri, O_RDONLY);
    char *msg = "404 Not Found\n";
    if (mime == NULL)
    {
        response_404(connFd, buf);
        return;
    }
    struct stat fstatbuf;
    fstat(uriFd, &fstatbuf);
    sprintf(buf,
            "HTTP/1.1 200 ok\r\n"
            "Date: %s"
            "Server: icws\r\n"
            "Connection: close\r\n"
            "Content-type: %s\r\n"
            "Content-length: %lu\r\n"
            "Last-modification: %s\r\n\r\n",
            gettDateTime(), mime, fstatbuf.st_size, ctime(&fstatbuf.st_mtime));
    write_all(connFd, buf, strlen(buf));
    return;
}

void respond_all(int connFd, char *uri, char *mime)
{
    char buf[MAXBUF];
    int uriFd = open(uri, O_RDONLY);
    fprintf(stdout, "uriFd is %d\n", uriFd);
    fprintf(stdout, "meme is %s\n", mime);
    char *msg = "404 Not Found\n";
    if (mime == NULL || uriFd < 0)
    {
        response_404(connFd, buf);
        return;
    }
    struct stat fstatbuf;
    fstat(uriFd, &fstatbuf);
    sprintf(buf,
            "HTTP/1.1 200 ok\r\n"
            "Date: %s"
            "Server: icws\r\n"
            "Connection: close\r\n"
            "Content-type: %s\r\n"
            "Content-length: %lu\r\n"
            "Last-modification: %s\r\n\r\n",
            gettDateTime(), mime, fstatbuf.st_size, ctime(&fstatbuf.st_mtime));
    write_all(connFd, buf, strlen(buf));
    write_logic(uriFd, connFd);
}

char *getExtension(char *filename)
{
    char *dot = strrchr(filename, '.');
    if (!dot || dot == filename)
        return "";
    return dot + 1;
}
int checkHTTPversion(Request *request)
{
    char *reqHTTPVersion = request->http_version;
    char *httpVersion = "HTTP/1.1";
    if (!strcmp(reqHTTPVersion, httpVersion))
    {
        return 1;
    }
    return 0;
}

void getMime(char *extension, char *result)
{
    if (!strcmp(extension, "html"))
    {
        strcpy(result, "text/html");
    }
    else if (!strcmp(extension, "js"))
    {
        strcpy(result, "text/javascript");
    }
    else if (!strcmp(extension, "png"))
    {
        strcpy(result, "image/png");
    }
    else if (!strcmp(extension, "jpeg"))
    {
        strcpy(result, "image/jpeg");
    }
    else if (!strcmp(extension, "jpg"))
    {
        strcpy(result, "image/jpeg");
    }
    else if (!strcmp(extension, "css"))
    {
        strcpy(result, "text/css");
    }
    else if (!strcmp(extension, "gif"))
    {
        strcpy(result, "image/gif");
    }
    else if (!strcmp(extension, "txt"))
    {
        strcpy(result, "text/plain");
    }
    else
    { // none of above mime
        strcpy(result, "none");
    }
}
void responseHEAD(int connFd, Request *request, Field *field, char *newPath)
{
    char *mime = (char *)malloc(sizeof(char) * 100);
    char *extension = getExtension(request->http_uri);
    getMime(extension, mime);
    if (!strcmp(mime, "none"))
    {
        respond_all_head(connFd, newPath, NULL);
        return;
    }
    respond_all_head(connFd, newPath, mime);
    return;
}

void responseGET(int connFd, Request *request, Field *field, char *newPath)
{
    char *mime = (char *)malloc(sizeof(char) * 100);
    char *extension = getExtension(request->http_uri);
    getMime(extension, mime);
    if (!strcmp(mime, "none"))
    {
        respond_all(connFd, newPath, NULL);
        return;
    }
    fprintf(stdout, "%s\n", mime);
    respond_all(connFd, newPath, mime);
    return;
}

void serve_http(int connFd, char *rootFolder, Field *field)
{
    int readRet;
    int sizeRet = 0;
    char buf[MAXBUF];
    char readBuf[MAXBUF];
    // rio_t *rc = (rio_t *)malloc(sizeof(rio_t));
    // rio_readinitb(rc, connFd);
    while ((readRet = read_line(connFd, readBuf, MAXBUF)) > 0)
    {
        sizeRet += readRet;
        strcat(buf, readBuf);
        if (!strcmp(readBuf, "\r\n"))
        {
            break;
        }
    }
    if (!readRet)
    {
        fprintf(stderr, "Failed to accept\n");
        return;
    }
    pthread_mutex_lock(&jobs_mutex);
    Request *request = parse(buf, sizeRet, connFd);
    pthread_mutex_unlock(&jobs_mutex);
    if (request == NULL) // handled malformede request
    {
        response_400(connFd, buf);
        return;
    }
    printf("Http Method %s\n", request->http_method);
    printf("Http Version %s\n", request->http_version);
    printf("Http Uri %s\n", request->http_uri);
    if (!checkHTTPversion(request))
    {
        response_505(connFd, buf);
        return;
    }
    char newPath[80];
    if (request->http_uri[0] == '/')
    {
        sprintf(newPath, "%s%s", field->rootFolder, request->http_uri);
        if (!strcasecmp(request->http_method, "GET"))
        {
            responseGET(connFd, request, field, newPath);
        }
        else if (!strcasecmp(request->http_method, "HEAD"))
        {

            responseHEAD(connFd, request, field, newPath);
        }
        else
        {
            response_501(connFd, buf);
        }
    }
    else
    {
        printf("LOG: Unknown request\n");
    }
    free(request);
    return;
}
void *do_work(void *field)
{

    for (;;)
    {
        Field *info = (Field *)field;
        // fprintf(stdout, "port is %s\n", info->port);

        // fprintf(stderr, "%d\n", info->pool->status);

        bool success = !q.empty();
        if (success)
        {
            pthread_mutex_lock(&jobs_mutex);
            int w = q.front();
            q.pop_front();
            pthread_mutex_unlock(&jobs_mutex);
            if (w == NULL)
            {
                fprintf(stderr, "no w\n");
            }
            else
            {
                struct pollfd pFD;
                pFD.fd = w;
                pFD.events = POLLIN;
                int result = poll(&pFD, 1, info->timeout * 1000);
                if (result == -1) // if not ready/ put back to the q
                {
                    pthread_mutex_lock(&jobs_mutex);
                    q.push_back(w);
                    pthread_mutex_unlock(&jobs_mutex);
                }
                else if (pFD.revents & POLLIN)
                {
                    serve_http(w, info->rootFolder, info);
                    close(w);
                }
                else
                {
                    close(w);
                }
            }
            // Terminate with a number < 0 , inactive file discriptor
            // NOTE: in fact printf is not thread safe
        }
        else // there is no work return
        {
            pthread_mutex_lock(&jobs_mutex);
            // fprintf(stderr, "lock\n");
            pthread_cond_wait(&condition_variable, &jobs_mutex);
            // fprintf(stderr, "signal\n");
            pthread_mutex_unlock(&jobs_mutex);
        }
    }
}
void initThreadPool(Field *field)
{

    int numThreads = field->numThreads;
    thread_array = (pthread_t *)malloc(sizeof(pthread_t) * numThreads);
    if ((pthread_mutex_init(&jobs_mutex, NULL)) < 0)
    {
        fprintf(stderr, "Error initialising mutex\n");
        exit(1);
    }
    if ((pthread_cond_init(&condition_variable, NULL)) < 0)
    {
        fprintf(stderr, "Error initialising condtion varaible %s\n");
        exit(1);
    }
    for (int i = 0; i < numThreads; i++)
    {
        if ((pthread_create(&thread_array[i], NULL, do_work, (void *)field)) < 0)
        {
            fprintf(stderr, "Error creating threads\n");
            exit(1);
        }
        // fprintf(stdout, "Created worker %d thread with tid %d.\n", pthread_self());
    }
    // field->pool->work_q->status = 88;
    // return field->pool;
    return;
}
int main(int argc, char **argv)
{
    int ch;
    Field *field = (Field *)malloc(sizeof(Field));
    while ((ch = getopt_long(argc, argv, "p:r:", long_options, NULL)) != -1)
    {
        switch (ch)
        {
        case 'p':
            field->port = optarg;
            printf("option --port with value '%s'\n", field->port);
            break;

        case 'r':
            field->rootFolder = optarg;
            printf("option --root with value '%s'\n", field->rootFolder);
            break;

        case 'n':
            field->numThreads = atoi(optarg);
            printf("option --numThreads with value '%d'\n", field->numThreads);
            break;

        case 't':
            field->timeout = atoi(optarg);
            printf("option --timeout with value '%d'\n", field->timeout);
            break;
        default:
            exit(1);
        }
    }
    initThreadPool(field);
    int listenFd = open_listenfd(field->port);
    for (;;)
    {
        char buf[8192];
        struct sockaddr_storage clientAddr;
        socklen_t clientLen = sizeof(struct sockaddr_storage);
        int connFd = accept(listenFd, (SA *)&clientAddr, &clientLen);
        if (connFd < 0)
        {
            fprintf(stderr, "Failed to accept\n");
            continue;
        }
        char hostBuf[MAXBUF], svcBuf[MAXBUF];

        if (getnameinfo((SA *)&clientAddr, clientLen,
                        hostBuf, MAXBUF, svcBuf, MAXBUF, 0) == 0)
            printf("Connection from %s:%s\n", hostBuf, svcBuf);
        else
            printf("Connection from ?UNKNOWN?\n");
        // fprintf(stdout, "status is %d\n", connFd);
        pthread_mutex_lock(&jobs_mutex);
        q.push_back(connFd);
        showdq(q);
        pthread_cond_signal(&condition_variable);
        pthread_mutex_unlock(&jobs_mutex);
    }
    free(field);
    pthread_mutex_destroy(&jobs_mutex);

    return 0;
}