#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <pthread.h>
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
struct threadpool
{
    pthread_t *thread_array;
    int status;
    work_queue work_q;
};
static struct option long_options[] =
    {
        {"port", required_argument, NULL, 'p'},
        {"root", required_argument, NULL, 'r'},
        {"numThreads", required_argument, NULL, 'n'},
        {"timeout", required_argument, NULL, 't'},
        {NULL, 0, NULL, 0}};

typedef struct
{
    struct threadpool *pool;
    char *port;
    char *rootFolder;
    int numThreads;
    int timeout;
    int connFd;
    char buf[MAXBUF];
} Field;

typedef struct sockaddr SA;
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
            "Date: %s\r\n"
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
            "Date: %s\r\n"
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
            "Date: %s\r\n"
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
            "Date: %s\r\n"
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
        strcpy(result, "image/jpg");
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
    respond_all(connFd, newPath, mime);
    return;
}

void serve_http(int connFd, char *rootFolder, Field *field)
{
    int readRet;
    int sizeRet = 0;
    char buf[MAXBUF];
    char readBuf[MAXBUF];
    while ((readRet = read_line(connFd, readBuf, MAXBUF)) > 0)
    {
        sizeRet += readRet;
        strcat(field->buf, readBuf);
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
    for (int i = 0; i < sizeRet; i++)
    {
        fprintf(stderr, "%c", field->buf[i]);
    }
    Request *request = parse(field->buf, sizeRet, connFd);
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
        long w;
        if (info->pool->work_q.remove_job(&w))
        {
            if (w == NULL)
                break; // Terminate with a number < 0 , inactive file discriptor
            // NOTE: in fact printf is not thread safe
            serve_http(info->connFd, info->rootFolder, info);
        }
        else
        {
            pthread_mutex_lock(&info->pool->work_q.jobs_mutex);
            pthread_cond_wait(&info->pool->work_q.condition_variable, &info->pool->work_q.jobs_mutex);
            pthread_mutex_unlock(&info->pool->work_q.jobs_mutex);
        }
    }
}
void initThreadPool(Field *field)
{

    field->pool = (struct threadpool *)malloc(sizeof(struct threadpool));
    field->pool->status = INACTIVE;
    int numThreads = field->numThreads;
    field->pool->thread_array = (pthread_t *)malloc(sizeof(pthread_t) * numThreads);
    for (int i = 0; i < numThreads; i++)
    {
        pthread_create(&field->pool->thread_array[i], NULL, do_work, (void *)field);
    }
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
            break;
            continue;
        }
        field->connFd = connFd;
        char hostBuf[MAXBUF], svcBuf[MAXBUF];

        if (getnameinfo((SA *)&clientAddr, clientLen,
                        hostBuf, MAXBUF, svcBuf, MAXBUF, 0) == 0)
            printf("Connection from %s:%s\n", hostBuf, svcBuf);
        else
            printf("Connection from ?UNKNOWN?\n");
        initThreadPool(field);
        int *connect_fd = (int *)malloc(sizeof(int));
        *connect_fd = connFd;
        field->pool->work_q.add_job(connFd);
        close(field->connFd);
        free(field);
    }
    return 0;
}
