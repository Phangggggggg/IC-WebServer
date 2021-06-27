#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "io.h"
void rio_readinitb(rio_t *rp, int fd)
{
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}

static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
    int cnt;

    while (rp->rio_cnt <= 0)
    { /* refill if buf is empty */
        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf,
                           sizeof(rp->rio_buf));
        if (rp->rio_cnt < 0)
        {
            if (errno != EINTR) /* interrupted by sig handler return */
                return -1;
        }
        else if (rp->rio_cnt == 0) /* EOF */
            return 0;
        else
            rp->rio_bufptr = rp->rio_buf; /* reset buffer ptr */
    }

    /* Copy min(n, rp->rio_cnt) bytes from internal buf to user buf */
    cnt = n;
    if (rp->rio_cnt < n)
        cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    int n, rc;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++)
    {
        if ((rc = rio_read(rp, &c, 1)) == 1)
        {
            *bufp++ = c;
            if (c == '\n')
                break;
        }
        else if (rc == 0)
        {
            if (n == 1)
                return 0; /* EOF, no data read */
            else
                break; /* EOF, some data was read */
        }
        else
            return -1; /* error */
    }
    *bufp = 0;
    return n;
}
// ssize_t io_readline(int connFd, char *usrbuf, size_t maxlen)
// {
//     ioBuffer *io = (ioBuffer *)malloc(sizeof(ioBuffer));
//     io->ioFd = connFd;
//     ssize_t n = ioReadfromBuf(io, usrbuf, maxlen);
//     for (int i = 0; i < maxlen; i++)
//     {
//         if ()
//         {
//             /* code */
//         }

//     }
//     int n;
//     char c, *bufp = usrbuf;

//     for (n = 1; n < maxlen; n++)
//     {
//         int numRead;
//         if ((numRead =) == 1)
//         {
//             *bufp++ = c;
//             if (c == '\n')
//             {
//                 n++;
//                 break;
//             }
//         }
//         else if (numRead == 0)
//         {
//             break;
//         } /* EOF */
//         else
//             return -1; /* Error */
//     }
//     *bufp = '\0';
//     return n - 1;
// }

// ssize_t ioReadfromBuf(ioBuffer *io, char *usrbuf, size_t maxlen)
// {
//     int bytesRead = 0;
//     int bufSize = maxlen;
//     char *buf = io->ioBuf;
//     while (bufSize != 0)
//     {
//         if (bytesRead = read(io->ioFd, io->ioBuf, bufSize) < 0)
//         {
//             return -1;
//         }
//         else if (bytesRead == 0)
//         {
//             break;
//         }
//         bufSize -= bytesRead; // what left to read
//         buf += bytesRead;
//     }
//     return (maxlen - bufSize);
// }
