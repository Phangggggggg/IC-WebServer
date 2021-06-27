#ifndef __SIMPLE_WORK_QUEUE_HPP_
#define __SIMPLE_WORK_QUEUE_HPP_

#include <deque>
#include <pthread.h>
#include <iostream>
using namespace std;

struct work_queue
{
    deque<int> jobs;
    pthread_mutex_t jobs_mutex;
    pthread_cond_t condition_variable;
    int status;

    /* add a new job to the work queue
     * and return the number of jobs in the queue */
    int add_job(int num, work_queue *q)
    {
        fprintf(stdout, "add_job is %d\n", q->status);
        fprintf(stdout, "empty is %d\n", q->jobs.empty());
        pthread_mutex_lock(&this->jobs_mutex);
        q->jobs.push_back(num);
        pthread_cond_signal(&this->condition_variable);
        size_t len = q->jobs.size();
        pthread_mutex_unlock(&this->jobs_mutex);
        return len;
    }

    /* return FALSE if no job is returned
     * otherwise return TRUE and set *job to the job */
    bool remove_job(int *job, work_queue *q)
    {
        pthread_mutex_lock(&q->jobs_mutex);
        bool success = !q->jobs.empty();
        if (success)
        {
            *job = q->jobs.front();
            q->jobs.pop_front();
        }
        pthread_mutex_unlock(&q->jobs_mutex);
        return success;
    }
};

#endif
