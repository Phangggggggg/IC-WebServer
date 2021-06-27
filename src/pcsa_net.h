#ifndef __PCSA_NET_
#define __PCSA_NET_
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#define RIO_BUFSIZE 8192
typedef struct
{
    int rio_fd;                /* descriptor for this internal buf */
    int rio_cnt;               /* unread bytes in internal buf */
    char *rio_bufptr;          /* next unread byte in internal buf */
    char rio_buf[RIO_BUFSIZE]; /* internal buffer */
} rio_t;
void rio_readinitb(rio_t *rp, int fd);
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n);
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);
int open_listenfd(char *port);
int open_clientfd(char *hostname, char *port);
ssize_t read_line(int connFd, char *usrbuf, size_t maxlen);
void write_all(int connFd, char *buf, size_t len);

#endif
