#include "threadPool.h"
#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_MIN_THREADS 5
#define DEFAULT_MAX_THREADS 10
#define DEFAULT_QUEUE_CAPACITY 100
enum STATUS_CODE
{
    UNKNOWN_ERROR,
    INVALID_ACCESS,
    MALLOC_ERROR,
    NULL_PTR,
    ON_SUCCESS,
};

/*本质是一个消费者*/
void *threadHanlder(void *arg)
{
    threadpool_t *pool = (threadpool_t *)arg;
    while (1)
    {
        pthread_mutex_lock(&pool->mutexPool);
        while (pool->queueSize == 0)
        {
            /*等待条件变量，生产者发送过来的*/
            pthread_cond_wait(&(pool->notEmpty), &(pool->mutexPool));
        }
        /*队列有任务*/
        task_t tmpTask = pool->taskQueue[pool->queueFront];
        pool->queueFront = (pool->queueFront + 1) % (pool->queueCapacity);
        /*任务数减一*/
        pool->queueSize--;
        /*解锁*/
        pthread_mutex_unlock(&pool->mutexPool);
        /*发信号给生产者，告诉他可以继续生产*/
        pthread_cond_signal(&pool->notFull);

        /*为了提升我们的性能，再创建一把忙碌锁*/
        pthread_mutex_lock(&pool->mutexBusy);
        /*忙碌线程加一*/
        pool->busyThreadNums++;
        pthread_mutex_unlock(&pool->mutexBusy);
        /*执行线程：钩子*/
        tmpTask.worker_hander(tmpTask.arg);

        /*忙碌线程减一*/
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyThreadNums--;
        pthread_mutex_unlock(&pool->mutexBusy);
    }
    pthread_exit(NULL);
}
/*线程池初始化*/
int threadPoolInit(threadpool_t *pool, int minThread, int maxThread, int queueCapacity)
{
    if (pool == NULL)
    {
        return NULL_PTR;
    }
    do
    {
        /*判断合法性*/
        if (minThread < 0 || maxThread < 0 || minThread >= maxThread)
        {
            minThread = DEFAULT_MIN_THREADS;
            maxThread = DEFAULT_MAX_THREADS;
        }
        /*更新线程池属性*/
        pool->minThread = minThread;
        pool->maxThread = maxThread;

        /*初始化时，忙碌的线程数为0*/
        int busyThreadNums = 0;

        if (pool->queueCapacity < 0)
        {
            pool->queueCapacity = DEFAULT_QUEUE_CAPACITY;
        }
        /*维护任务队列*/
        pool->queueCapacity = queueCapacity;
        pool->queueFront = 0;
        pool->queueRear = 0;
        pool->queueSize = 0;
        pool->taskQueue = (task_t *)malloc(sizeof(task_t) * pool->queueCapacity);
        if (pool->taskQueue == NULL)
        {
            perror("malloc error\n");
            break;
        }
        memset(pool->taskQueue, 0, sizeof(task_t) * pool->queueCapacity);
        /*****/
        pool->threadIds = (pthread_t *)malloc(sizeof(pthread_t) * maxThread);
        if (pool->threadIds == NULL)
        {
            perror("malloc error!\n");
            exit(-1);
        }
        /*清除脏数据*/
        memset(pool->threadIds, 0, sizeof(pthread_t) * maxThread);

        int ret = 0;
        /*创建线程*/
        for (int idx = 0; idx < pool->minThread; idx++)
        {
            if (pool->threadIds[idx] == 0)
            {
                ret = pthread_create(&pool->threadIds[idx], NULL, threadHanlder, pool);
                if (ret != 0)
                {
                    perror("pthread create error!\n");
                    break;
                }
            }
        }
        /*这个ret是创建线程函数的返回值*/
        if (ret != 0)
        {
            break;
        }

        pool->liveThreadNums = pool->minThread;
        /*初始化锁资源*/
        pthread_mutex_init(&(pool->mutexPool), NULL);
        pthread_mutex_init(&(pool->mutexBusy), NULL);
        /*初始化条件变量*/
        if (pthread_cond_init(&(pool->notEmpty), NULL) != 0 || pthread_cond_init(&(pool->notFull), NULL) != 0)
        {
            perror("thread cond error");
            break;
        }

        return ON_SUCCESS;
    } while (0);
    /*程序执行到这个地方，上面一定有bug*/
    /*销毁队列*/
    if (pool->taskQueue != NULL)
    {
        free(pool->taskQueue);
        pool->taskQueue = NULL;
    }

    /*回收线程资源*/
    for (int idx = 0; idx < pool->minThread; idx++)
    {
        if (pool->threadIds[idx] != 0)
        {
            pthread_join(&pool->threadIds[idx], NULL);
        }
    }

    /*回收ids*/
    if (pool->threadIds != NULL)
    {
        free(pool->threadIds);
        pool->threadIds = NULL;
    }

    /*释放锁资源*/
    pthread_mutex_destroy(&pool->mutexBusy);
    pthread_mutex_destroy(&pool->mutexPool);

    /*释放条件变量资源*/
    pthread_cond_destroy(&pool->notEmpty);
    pthread_cond_destroy(&pool->notFull);

    return UNKNOWN_ERROR;
}
/*线程池添加任务*/
int threadPoolAddTask(threadpool_t *pool, void *(*worker_hander)(void *), void *arg)
{
    if (pool == NULL)
    {
        return NULL_PTR;
    }
    pthread_mutex_lock(&pool->mutexPool);
    while (pool->queueSize == pool->queueCapacity)
    {
        pthread_cond_wait(&pool->notFull, &pool->mutexPool);
    }
    /*程序到这个地方一定有位置可以放任务*/
    /*将任务放到队尾*/
    pool->taskQueue[pool->queueRear].worker_hander = worker_hander;
    pool->taskQueue[pool->queueRear].arg = arg;
    /*队尾向后移动*/
    pool->queueRear = (pool->queueRear + 1) % pool->queueCapacity;
    /*任务数加一*/
    pool->queueSize++;
    /*解锁*/
    pthread_mutex_unlock(&pool->mutexPool);
    /*发信号*/
    pthread_cond_signal(&pool->notEmpty);

    return ON_SUCCESS;
}
/*线程池销毁*/
int threadPoolDestory(threadpool_t *pool)
{
}