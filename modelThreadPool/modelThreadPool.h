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
    task_t *queueTask;
    int queueCapacity;
    int queueFront;
    int queueRear;
    int queueSize;

    /*工作线程*/
    pthread_t *threadId;
    int minCapacity;
    int maxCapacity;
    /*管理者线程*/
    pthread_t managerId;
    int liveThreadNums;
    int busyThreadNums;
    int exitThreadNums;

    pthread_cond_t notEmpty;
    pthread_cond_t notFull;
    pthread_mutex_t mutexPool;
    pthread_mutex_t mutexBusy;

    int shutDown;
} ThreadPool;

/*初始化线程池*/
int poolInit(ThreadPool **pPool, int minCapacity, int maxCapacity, int queueCapacity);

/*线程池添加任务*/
int poolAdd(ThreadPool *pool, void *worker(void *), void *arg);


/*销毁线程池*/
int poolDestroy(ThreadPool *pool);

#endif /*__MODEL_THREAD_POOL_H_*/