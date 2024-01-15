#ifndef __THREAD_POOL_H_
#define __THREAD_POOL_H_
#include <pthread.h>
typedef struct threadpool_t
{
    /*线程池中的线程*/
    pthread_t *threadId;
    /*最小线程数*/
    int minThread;
    /*最大线程数*/
    int maxThread;

} threadpool_t;
#endif /*__THREAD_POOL_H_*/