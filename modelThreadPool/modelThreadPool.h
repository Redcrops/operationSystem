#ifndef __MODEL_THREAD_POOL_H_
#define __MODEL_THREAD_POOL_H_
#include <pthread.h>
typedef struct task_t
{
    void *(*worker)(void *);
    void *arg;
} task_t;

typedef struct ThreadPool
{
    /*任务队列*/
    task_t *task;

    /*工作线程*/
    pthread_t *threadId;
    int minCapacity;
    int maxCapacity;

    pthread_cond_t notEmpty;
    pthread_cond_t notFull;
    pthread_mutex_t mutexPool;
} ThreadPool;
#endif /*__MODEL_THREAD_POOL_H_*/