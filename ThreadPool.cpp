#include "ThreadPool.h"
#include "iostream"
#include<pthread.h>
#include <cstring>
#include <unistd.h>

using namespace std;
const int NUMBER=2;



//定义线程结构体
typedef struct TaskQueue
{
    void (*function)(void *arg);
    void *arg;
}Task;

//线程池结构体
struct ThreadPool
{
    TaskQueue*taskQ;//任务队列
    int queueCapacity;//任务容量

    int queueSize;//当前任务个数
    int queueFront;//队头->取数据
    int queueRear;//对位->放数据

    pthread_t managerID;//管理者线程ID
    pthread_t *threadIDs;//工作的线程ID
    int minNum;//最小线程数量
    int maxNum;//最大线程数量
    int busyNum;//忙的线程数量
    int liveNum;//存活的线程数量
    int exitNum;//要销毁的线程数量

    pthread_mutex_t  mutexPool;//
    pthread_mutex_t mutexBusy;

    pthread_cond_t notFull;//任务队列是否满了
    pthread_cond_t notEmpty;//任务队列是否为空

    int shutdown;//是都要销毁线程池
};



ThreadPool *threadPoolCreate(int min,int max,int queueSize)
{
    auto*pool=new ThreadPool;


    pool->threadIDs=new pthread_t[max];
    memset(pool->threadIDs,0,sizeof(pthread_t)*max);
    pool->minNum=min;
    pool->maxNum=max;
    pool->busyNum=0;
    pool->liveNum=min;
    pool->exitNum=0;

    if(pthread_mutex_init(&pool->mutexPool,NULL)!=0||
    pthread_mutex_init(&pool->mutexBusy,NULL)!=0||
    pthread_cond_init(&pool->notEmpty,NULL)!=0||
    pthread_cond_init(&pool->notFull,NULL)!=0)
    {
        cout<<"mutex or condition init failed"<<endl;
        return 0;
    }
    //任务队列
    pool->taskQ=new TaskQueue[queueSize];
    pool->queueCapacity=queueSize;
    pool->queueSize=0;
    pool->queueFront=0;
    pool->queueRear=0;

    pool->shutdown=0;

    //创建线程
    pthread_create(&pool->managerID,NULL,manager,pool);
    for(int i=0;i<min;i++)
    {
        pthread_create(&pool->threadIDs[i],NULL,worker,pool);
    }
    return pool;
}

void *worker(void*arg)
{
    auto*pool=(ThreadPool*)arg;

    while(1)
    {
        //读取任务队列，访问线程池，所以要加锁
        pthread_mutex_lock(&pool->mutexPool);
        //判断任务队列是否为空
        //当前没任务，就要阻塞
        while(pool->queueSize==0&&!pool->shutdown)
        {
            pthread_cond_wait(&pool->notEmpty,&pool->mutexPool);
            if(pool->exitNum>0)
            {
                pool->exitNum--;
                if(pool->liveNum>pool->minNum)
                    pool->liveNum--;
                pthread_mutex_unlock(&pool->mutexPool);
                ThreadExit(pool);
            }

        }
        //开始消费
        //如果线程池已经关闭
        if(pool->shutdown)
        {
            pthread_mutex_unlock(&pool->mutexPool);
            ThreadExit(pool);
        }
        //从任务队列取出任务
        TaskQueue task;
        task.function=pool->taskQ[pool->queueFront].function;
        task.arg=pool->taskQ[pool->queueFront].arg;

        //移动环形队列
        pool->queueFront=(pool->queueFront+1)%pool->queueCapacity;
        //当前任务数量-1
        pool->queueSize--;
        //唤醒生产者
        pthread_cond_signal(&pool->notFull);
        pthread_mutex_unlock(&pool->mutexPool);

        //执行取出的任务
        pthread_mutex_lock(&pool->mutexBusy);
        //忙的线程加1
        printf("thread %ld start working..\n",pthread_self());
        pool->busyNum++;
        pthread_mutex_unlock(&pool->mutexBusy);
        task.function(task.arg);
        free(task.arg);
        task.arg=nullptr;
        pthread_mutex_lock(&pool->mutexBusy);
        printf("thread %ld end working..\n",pthread_self());
        pool->busyNum--;
        pthread_mutex_unlock(&pool->mutexBusy);


    }

}

void *manager(void*arg)
{
    auto*pool=(ThreadPool*)arg;
    //以一定频率检测是否需要对工作者线程调节
    while(!pool->shutdown)
    {
        sleep(3);
        //取出线程池中任务的数量和当前线程的数量
        pthread_mutex_lock(&pool->mutexPool);
        int queueSize=pool->queueSize;
        int liveNum=pool->liveNum;
        pthread_mutex_unlock(&pool->mutexPool);
        //取出忙线程的数量
        pthread_mutex_lock(&pool->mutexBusy);
        int busyNum=pool->busyNum;
        pthread_mutex_unlock(&pool->mutexBusy);


        //添加线程
        //任务个数>存活的个数&& 存活线程数<最大线程数

        if(queueSize>liveNum&&liveNum<pool->maxNum)
        {
            pthread_mutex_lock(&pool->mutexPool);
            int counter=0;
            for(int i=0;i<pool->maxNum&&counter<NUMBER&&pool->liveNum<pool->maxNum;i++)
            {
                //找到池子里哪个位置没用
                if(pool->threadIDs[i]==0)
                {
                    pthread_create(&pool->threadIDs[i],NULL,worker,pool);
                    counter++;
                    pool->liveNum++;
                }
            }
            pthread_mutex_unlock(&pool->mutexPool);
        }

       //销毁线程
       //忙的线程*2<存活的线程数&&存活的线程>最小线程数
       if(busyNum*2<liveNum&&liveNum>pool->minNum)
       {
           pthread_mutex_lock(&pool->mutexPool);
           pool->exitNum=NUMBER;
           pthread_mutex_unlock(&pool->mutexPool);
           //让工作的线程自杀,即唤醒没有事的worker让他向下执行自杀,因为他其实不携带任务
           for(int i=0;i<NUMBER;++i)
           {
               pthread_cond_signal(&pool->notEmpty);
           }

       }

    }


}

void ThreadExit(ThreadPool*pool)
{
    pthread_t tid= pthread_self();
    for(int i=0;i<pool->maxNum;++i)
    {
        if(pool->threadIDs[i]==tid)
        {
            pool->threadIDs[i]=0;
            printf("threadExit() called,%llu exiting\n",tid);
        }
    }

    pthread_exit(nullptr);

}


void threadPoolAdd(ThreadPool*pool,void(*func)(void*),void*arg)
{
    pthread_mutex_lock(&pool->mutexPool);
    while(pool->queueSize==pool->queueCapacity&&!pool->shutdown)
    {
        //阻塞生产者线程
        pthread_cond_wait(&pool->notFull,&pool->mutexPool);
    }
    if(pool->shutdown)
    {
        pthread_mutex_unlock(&pool->mutexPool);
        return;
    }
    //添加任务
    pool->taskQ[pool->queueRear].function=func;
    pool->taskQ[pool->queueRear].arg=arg;
    pool->queueRear=(pool->queueRear+1)%pool->queueCapacity;
    pool->queueSize++;

    //唤醒阻塞的工作
    pthread_cond_signal(&pool->notEmpty);

    pthread_mutex_unlock(&pool->mutexPool);

}

int threadBusyNum(ThreadPool*pool)
{
    pthread_mutex_lock(&pool->mutexBusy);
    int busyNum=pool->busyNum;
    pthread_mutex_unlock(&pool->mutexBusy);

    return busyNum;
}

int threadPoolAliveNum(ThreadPool*pool)
{
    pthread_mutex_lock(&pool->mutexPool);
    int liveNum=pool->liveNum;
    pthread_mutex_unlock(&pool->mutexPool);
    return liveNum;
}

int threadPoolDestory(ThreadPool*pool)
{
    if(pool==NULL)
        return -1;
    //关闭先处处
    pool->shutdown=1;
    pthread_join(pool->managerID,NULL);
    for(int i=0;i<pool->liveNum;++i)
        pthread_cond_signal(&pool->notEmpty);
    //释放堆
    delete[] pool->taskQ;
    if(pool->threadIDs)
        delete[] pool->threadIDs;

    pthread_mutex_destroy(&pool->mutexPool);
    pthread_mutex_destroy(&pool->mutexBusy);
    pthread_cond_destroy(&pool->notEmpty);
    pthread_cond_destroy(&pool->notFull);
    delete pool;
    return 0;
}
