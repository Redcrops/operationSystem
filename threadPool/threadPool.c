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
void *threadHanlder(void *arg)
{
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
                ret = pthread_create(&pool->threadIds[idx], NULL, threadHanlder, NULL);
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
    return UNKNOWN_ERROR;
}

/*线程池销毁*/
int threadPoolDestory(threadpool_t *pool)
{
}