#include "modelThreadPool.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#define MAX_THREAD_CAPACITY 50
#define DEFAULT_MIN_THREADS 3
#define DEFAULT_MAX_THREADS 10
#define DEFAULT_QUEUE_CAPACITY 8
#define TIME_INTERVAL 5
#define DEFAULT_CHANGE 3
enum STATUS_CODE
{
    ON_SUCCESS,
    NULL_PTR,
    CALLOC_ERROR,
    ACCESS_INVAILD,
    UNKNOWN_ERROR,
};

static void exitSelf(ThreadPool *pool)
{
    for (int idx = 0; idx < pool->maxCapacity; idx++)
    {
        if (pool->threadId[idx] == pthread_self())
        {
            pool->threadId[idx] = 0;
            break;
        }
    }
    pthread_exit(NULL);
}
/*消费者：工作函数*/
static void *worker_func(void *arg)
{
    ThreadPool *pool = (ThreadPool *)arg;

    while (1)
    {
        pthread_mutex_lock(&pool->mutexPool);
        /*任务队列为空，等待*/

        while (pool->queueSize == 0 && pool->shutDown == 0)
        {
            printf("i am waiting!\n");
            pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);

            if (pool->exitThreadNums > 0)
            {
                pool->exitThreadNums--;
                if (pool->liveThreadNums > pool->minCapacity)

                {

                    pool->liveThreadNums--;
                    pthread_mutex_unlock(&pool->mutexPool);
                    exitSelf(pool);
                    /*自行离开*/
                }
            }
        }

        if (pool->shutDown == 1)
        {
            pool->liveThreadNums--;
            pthread_mutex_unlock(&pool->mutexPool);
            exitSelf(pool);
        }
        /*到这里任务队列一定有任务*/
        /*任务队列不为空，从任务队列中取出任务执行*/
        printf("i get a task!\n");
        task_t performTask = pool->queueTask[pool->queueFront];
        performTask.worker = pool->queueTask[pool->queueFront].worker;
        performTask.arg = pool->queueTask[pool->queueFront].arg;
        /*队头指针后移*/
        pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
        /*任务数减一*/
        pool->queueSize--;

        pthread_mutex_unlock(&pool->mutexPool);
        /*告诉生产者可以生产了*/
        pthread_cond_signal(&pool->notFull);

        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyThreadNums++;
        pthread_mutex_unlock(&pool->mutexBusy);

        /*执行*/

        performTask.worker(performTask.arg);

        sleep(5);

        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyThreadNums--;
        pthread_mutex_unlock(&pool->mutexBusy);
    }
    return ON_SUCCESS;
}
/*管理者线程*/
static void *manager_func(void *arg)
{
    ThreadPool *pool = (ThreadPool *)arg;
    // sleep(199);
    /*根据任务数量对线程进行增加或者减少*/
    while (pool->shutDown == 0)
    {
        /*五秒钟维护一次线程池*/
        sleep(TIME_INTERVAL);
        /*任务数量多，线程少，扩容*/
        /*一次最多扩容三个线程*/
        int count = 0;
        int ret = 0;

        /*从线程池中取出任务数量和现存所有现成*/
        pthread_mutex_lock(&pool->mutexPool);
        int taskNums = pool->queueSize;
        int liveThreadNums = pool->liveThreadNums;
        pthread_mutex_unlock(&pool->mutexPool);

        if (taskNums > pool->liveThreadNums && liveThreadNums < pool->maxCapacity)
        {
            pthread_mutex_lock(&pool->mutexPool);
            for (int idx = 0; idx < pool->maxCapacity && count < DEFAULT_CHANGE; idx++)
            {
                if (pool->threadId[idx] == 0)
                {
                    ret = pthread_create(&pool->threadId[idx], NULL, worker_func, pool);
                    if (ret != 0)
                    {
                        perror("expand pthread_create error!\n");
                    }
                    count++;
                    printf("+++\n");
                    pool->liveThreadNums++;
                }
            }
        }
        pthread_mutex_unlock(&pool->mutexPool);
        /*任务数量少，线程多，缩容*/

        /*这里为了解决任务少，线程多问题，引入一个变量busyNums，记录正在忙碌的线程*/
        /*防止释放线程资源时，误杀了正处于工作的线程*/
        /*除此之外，还存在的一个问题就是如何释放处于等待任务状态的线程资源，而不释放*/
        /*忙碌线程*/
        pthread_mutex_lock(&pool->mutexBusy);
        int busyThreadNums = pool->busyThreadNums;
        pthread_mutex_unlock(&pool->mutexBusy);

        if (busyThreadNums << 1 < liveThreadNums && liveThreadNums > pool->minCapacity)
        {
            pthread_mutex_lock(&pool->mutexPool);
            pool->exitThreadNums = DEFAULT_CHANGE;
            for (int idx = 0; idx < DEFAULT_CHANGE && pool->liveThreadNums > pool->minCapacity; idx++)
            {
                pthread_cond_signal(&pool->notEmpty);
            }
            pthread_mutex_unlock(&pool->mutexPool);
        }
    }
}

/*初始化线程池*/
int poolInit(ThreadPool **pPool, int minCapacity, int maxCapacity, int queueCapacity)
{
    ThreadPool *pool = calloc(1, sizeof(ThreadPool));
    if (pool == NULL)
    {
        return CALLOC_ERROR;
    }
    if (minCapacity <= 0 || minCapacity >= maxCapacity)
    {
        pool->minCapacity = DEFAULT_MIN_THREADS;
    }
    if (maxCapacity <= 0 || minCapacity >= maxCapacity)
    {
        pool->maxCapacity = DEFAULT_MAX_THREADS;
    }
    pool->minCapacity = minCapacity;
    pool->maxCapacity = maxCapacity;
    pool->managerId = 0;
    do
    {
        /*创建任务队列*/
        if (pool->queueCapacity <= 0)
        {
            pool->queueCapacity = DEFAULT_QUEUE_CAPACITY;
        }
        else
        {
            pool->queueCapacity = queueCapacity;
        }

        pool->queueTask = calloc(pool->queueCapacity, sizeof(task_t));
        if (pool->queueTask == NULL)
        {
            return CALLOC_ERROR;
        }
        pool->queueFront = 0;
        pool->queueRear = 0;
        pool->queueSize = 0;
        /******以上是创建任务队列*****/

        /*创建工作线程*/
        pool->threadId = calloc(pool->maxCapacity, sizeof(pthread_t));
        if (pool->threadId == NULL)
        {
            return CALLOC_ERROR;
        }
        int ret = 0;
        for (int idx = 0; idx < pool->minCapacity; idx++)
        {

            ret = pthread_create(&pool->threadId[idx], NULL, worker_func, pool);
            if (ret != 0)
            {
                perror("worker pthread_create error!\n");
                break;
            }
        }
        /****以上为工作线程****/

        /*初始化管理者线程所需变量*/
        pool->busyThreadNums = 0;
        pool->exitThreadNums = 0;
        pool->liveThreadNums = minCapacity;
        /*创建管理者线程*/
        int retManager = 0;
        retManager = pthread_create(&pool->managerId, NULL, manager_func, pool);
        if (retManager != 0)
        {
            perror("manager pthread_create error!\n");
            break;
        }

        /*锁和条件变量初始化*/
        pthread_mutex_init(&pool->mutexPool, NULL);
        pthread_mutex_init(&pool->mutexBusy, NULL);
        pthread_cond_init(&pool->notEmpty, NULL);
        pthread_cond_init(&pool->notFull, NULL);

        *pPool = pool;
        return ON_SUCCESS;

    } while (0);

    /*回收锁和条件变量*/
    pthread_mutex_destroy(&pool->mutexPool);
    pthread_mutex_destroy(&pool->mutexBusy);
    pthread_cond_destroy(&pool->notEmpty);
    pthread_cond_destroy(&pool->notFull);

    /*回收管理者线程*/
    if (pool->managerId != 0)
    {
        pthread_join(pool->managerId, NULL);
    }
    /*回收线程*/
    for (int idx = 0; idx < minCapacity; idx++)
    {
        if (pool->threadId[idx] != 0)
        {
            pthread_join(pool->threadId[idx], NULL);
        }
    }
    /*回收线程数组*/
    if (pool->threadId != NULL)
    {
        free(pool->threadId);
        pool->threadId = NULL;
    }
    if (pool != NULL)
    {
        free(pool);
        pool = NULL;
    }
    return UNKNOWN_ERROR;
}

/*线程池添加任务，生产者*/
int poolAdd(ThreadPool *pool, void *worker(void *), void *arg)
{
    if (pool == NULL)
    {
        return NULL_PTR;
    }
    pthread_mutex_lock(&pool->mutexPool);
    /*任务队列满了就等待信号*/
    while (pool->queueSize == pool->queueCapacity)
    {
        pthread_cond_wait(&pool->notFull, &pool->mutexPool);
    }
    /*收到信号，添加任务*/
    pool->queueTask[pool->queueRear].worker = worker;
    pool->queueTask[pool->queueRear].arg = arg;
    pool->queueRear = (pool->queueRear + 1) % pool->queueCapacity;
    pool->queueSize++;

    printf("+++++++++++++++\n");
    pthread_mutex_unlock(&pool->mutexPool);
    /*发信号告诉消费者可以消费了*/
    pthread_cond_signal(&pool->notEmpty);
    return ON_SUCCESS;
}

/*销毁线程池*/
int poolDestroy(ThreadPool *pool)
{
    if (pool == NULL)
    {
        return -1;
    }

    // 关闭线程池
    pool->shutDown = 1;
    // 阻塞回收管理者线程
    pthread_join(pool->managerId, NULL);
    // 唤醒阻塞的消费者线程
    for (int idx = 0; idx < pool->liveThreadNums; idx++)
    {
        pthread_cond_signal(&pool->notEmpty);
    }
    // 释放堆内存
    if (pool->queueTask)
    {
        free(pool->queueTask);
        pool->queueTask = NULL;
    }
    if (pool->threadId)
    {
        free(pool->threadId);
        pool->threadId = NULL;
    }

    pthread_mutex_destroy(&pool->mutexPool);
    pthread_mutex_destroy(&pool->mutexBusy);
    pthread_cond_destroy(&pool->notEmpty);
    pthread_cond_destroy(&pool->notFull);

    if (pool != NULL)
    {
        free(pool);
        pool = NULL;
    }

    return ON_SUCCESS;
}