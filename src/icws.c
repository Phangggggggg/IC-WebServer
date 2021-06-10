#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include "parse.h"
#include "pcsa_net.h"
#define MAXBUF 1024
static struct option long_options[] =
    {
        {"port", required_argument, NULL, 'p'},
        {"root", required_argument, NULL, 'r'},
        {NULL, 0, NULL, 0}};

typedef struct
{
    char *port;
    char *rootFolder;
    char buf[MAXBUF];
} Field;
// struct field

// {
//     char *port;
//     char *rootFolder;
//     char buf[MAXBUF];
// } field;

typedef struct sockaddr SA;
char *getExtension(const char *filename)
{
    char *dot = strrchr(filename, '.');
    if (!dot || dot == filename)
        return "";
    return dot + 1;
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
        result = NULL;
    }
}
void responseHEAD(int connFd, Request *request, Field *field, char *newPath)
{
    char *mime = (char *)malloc(sizeof(char) * 100);
    char *extension = getExtension(request->http_uri);
    // int isFileExist = access(newPath, F_OK);
    getMime(extension, mime);
    printf("mimie is %s\n", mime);
    respond_all_head(connFd, newPath, mime);
}

void responseGET(int connFd, Request *request, Field *field, char *newPath)
{
    char *mime = (char *)malloc(sizeof(char) * 100);
    char *extension = getExtension(request->http_uri);
    // int isFileExist = access(newPath, F_OK);
    getMime(extension, mime);
    printf("mimie is %s\n", mime);
    respond_all(connFd, newPath, mime);
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
    char *msg = "404 Not Found";
    if (uriFd < 0)
    {
        sprintf(buf,
                "HTTP/1.1 404 Not Found\r\n"
                "Server: Micro\r\n"
                "Connection: close\r\n\r\n");
        write_all(connFd, buf, strlen(buf));
        write_all(connFd, msg, strlen(msg));
        return;
    }
    // struct stat fstatbuf;
    // fstat(uriFd, &fstatbuf);
    // sprintf(buf,
    //         "HTTP/1.1 200 OK\r\n"
    //         "Server: Micro\r\n"
    //         "Connection: close\r\n"
    //         "Content-length: %lu\r\n"
    //         "Content-type: %s\r\n\r\n",
    //         fstatbuf.st_size, mime);
    // write_all(connFd, buf, strlen(buf));
    write_logic(uriFd, connFd);
}

void respond_all(int connFd, char *uri, char *mime)
{
    char buf[MAXBUF];
    int uriFd = open(uri, O_RDONLY);
    char *msg = "404 Not Found";
    if (uriFd < 0)
    {
        sprintf(buf,
                "HTTP/1.1 404 Not Found\r\n"
                "Server: Micro\r\n"
                "Connection: close\r\n\r\n");
        write_all(connFd, buf, strlen(buf));
        write_all(connFd, msg, strlen(msg));
        return;
    }
    struct stat fstatbuf;
    fstat(uriFd, &fstatbuf);
    sprintf(buf,
            "HTTP/1.1 200 OK\r\n"
            "Server: Micro\r\n"
            "Connection: close\r\n"
            "Content-length: %lu\r\n"
            "Content-type: %s\r\n\r\n",
            fstatbuf.st_size, mime);
    write_all(connFd, buf, strlen(buf));
    write_logic(uriFd, connFd);
}

void serve_http(int connFd, char *rootFolder, Field *field)
{
    int readRet;
    int sizeRet = 0;
    char readBuf[MAXBUF];
    while ((readRet = read_line(connFd, readBuf, MAXBUF)) > 0)
    {
        sizeRet += readRet;
        strcat(field->buf, readBuf);
        if (!strcmp(readBuf, "\r\n"))
        {
            printf("in");
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
        free(field);
        free(request);
        exit(1);
    }
    printf("Http Method %s\n", request->http_method);
    printf("Http Version %s\n", request->http_version);
    printf("Http Uri %s\n", request->http_uri);

    char buf[MAXBUF];

    sscanf(buf, "%s %s %s", request->http_method, request->http_uri, request->http_version);

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
    }
    else
    {
        // respond_all(connFd, newPath, NULL);
        printf("LOG: Unknown request\n");
    }
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
        char hostBuf[MAXBUF], svcBuf[MAXBUF];

        if (getnameinfo((SA *)&clientAddr, clientLen,
                        hostBuf, MAXBUF, svcBuf, MAXBUF, 0) == 0)
            printf("Connection from %s:%s\n", hostBuf, svcBuf);
        else
            printf("Connection from ?UNKNOWN?\n");

        serve_http(connFd, field->rootFolder, field);
        close(connFd);
    }
    return 0;
}