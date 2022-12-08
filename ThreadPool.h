#ifndef THREAD_POOL_THREADPOOL_H
#define THREAD_POOL_THREADPOOL_H

typedef struct ThreadPool ThreadPool;

//创建线程池并初始化
ThreadPool *threadPoolCreate(int min,int max,int queueSize);

//销毁线程池
int threadPoolDestory(ThreadPool*pool);

//给线程池添加任务
void threadPoolAdd(ThreadPool*pool,void(*func)(void*),void*arg);

//获取线程池中线程的个数
int threadBusyNum(ThreadPool*pool);

//获取活着的线程个数
int threadPoolAliveNum(ThreadPool*pool);



void*worker(void*arg);

//管理线程
void *manager(void*arg);
void ThreadExit(ThreadPool*pool);

#endif //THREAD_POOL_THREADPOOL_H
