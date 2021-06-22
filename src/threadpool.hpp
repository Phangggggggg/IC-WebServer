#ifndef THREADPOOL_H
#define THREADPOOL_H
#include "simple_work_queue.hpp"
#include <pthread.h>
#endif
#define ACTIVE 0
#define INACTIVE -1

struct threadpool
{
    pthread_t *thread_array;
    int numThread;
    int status;
    work_queue work_q;
};
struct threadpool initThreadPool(int numThreads);
