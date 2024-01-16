#include <stdio.h>
#include <unistd.h>
#include "modelThreadPool.h"
#define TASK_NUM 100
void *print(void *arg)
{
    int num = *(int *)arg;
    printf("num=%d\n", num);
}

int main()
{
    ThreadPool *pool = NULL;
    int num = 1;
    int minCapacity = 10;
    int maxCapacity = 20;
    int queueCapacity = 100;
    poolInit(&pool, minCapacity, maxCapacity, queueCapacity);
    sleep(5);
    for (int idx = 0; idx < TASK_NUM; idx++)
    {
        
        
        poolAdd(pool, print, &num);
        usleep(550);
        num++;
        pthread_mutex_lock(&pool->mutexBusy);
        printf("live:%d,busy:%d\n",pool->liveThreadNums,pool->busyThreadNums);
        printf("tast:%d\n", pool->queueSize);
        pthread_mutex_unlock(&pool->mutexBusy);
    }

    sleep(10);
    printf("tast:%d\n", pool->queueSize);
    sleep(10000);


    poolDestroy(pool);

    return 0;
}