#ifndef THREAD_POOL_THREADPOOLWITHCLASS_H
#define THREAD_POOL_THREADPOOLWITHCLASS_H
#include "pthread.h"
#include "TaskQueue.h"
class ThreadPoolWithClass {
private:
    TaskQueue*taskQ;//任务队列

    static const int NUMBER=2;
    pthread_t managerID;//管理者线程ID
    pthread_t *threadIDs;//工作的线程ID
    int minNum;//最小线程数量
    int maxNum;//最大线程数量
    int busyNum;//忙的线程数量
    int liveNum;//存活的线程数量
    int exitNum;//要销毁的线程数量

    pthread_mutex_t  mutexPool;

    pthread_cond_t notEmpty;//任务队列是否为空

    bool shutdown;//是都要销毁线程
public:
    ThreadPoolWithClass(int min,int max);

//销毁线程池
    ~ThreadPoolWithClass();

//给线程池添加任务
    void addTask(Task task);

//获取线程池中线程的个数
    int getBusyNum();

//获取活着的线程个数
    int getAliveNum();
private:
    static void *worker(void*arg);
//管理线程
    static void *manager(void*arg);
    void ThreadExit();
};


#endif //THREAD_POOL_THREADPOOLWITHCLASS_H
