#ifndef __THREAD_POOL_H_
#define __THREAD_POOL_H_
#include <pthread.h>

typedef struct task_t
{
    void *(*worker_hander)(void *arg);
    void *arg;
} task_t;

typedef struct threadpool_t
{
    /*任务队列*/
    task_t *taskQueue;
    /*任务队列的容量*/
    int queueCapacity;
    /*任务队列的任务数*/
    int queueSize;
    /*任务队列的队头*/
    int queueFront;
    /*任务队列的队尾*/
    int queueRear;

    /*线程池中的线程*/
    pthread_t *threadIds;
    /*最小线程数*/
    int minThread;
    /*最大线程数*/
    int maxThread;

    /*真正干活的线程数*/
    int busyThreadNums;
    /*存活的线程数*/
    int liveThreadNums;

    /*锁----维护线程池*/
    pthread_mutex_t mutexPool;
    /*锁----维护干活的线程*/
    pthread_mutex_t mutexBusy;

    /*条件变量：任务队列有任务*/
    pthread_cond_t notEmpty;
    /*条件变量：任务队列有空位，可以继续存放*/
    pthread_cond_t notFull;
} threadpool_t;

/*线程池初始化*/
int threadPoolInit(threadpool_t *pool, int minThread, int maxThread, int queueCapacity);

/*线程池添加任务*/
int threadPoolAddTask(threadpool_t *pool, void *(*worker_hander)(void *), void *arg);

/*线程池销毁*/
int threadPoolDestory(threadpool_t *pool);
#endif /*__THREAD_POOL_H_*/