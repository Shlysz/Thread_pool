#include <cstring>
#include <unistd.h>
#include "ThreadPoolWithClass.h"
#include "iostream"

ThreadPoolWithClass::ThreadPoolWithClass(int min, int max) {
    taskQ=new TaskQueue;
    this->threadIDs=new pthread_t[max];
    memset(this->threadIDs,0,sizeof(pthread_t)*max);
    this->minNum=min;
    this->maxNum=max;
    this->busyNum=0;
    this->liveNum=min;
    this->exitNum=0;

    if(pthread_mutex_init(&this->mutexPool,NULL)!=0||
       pthread_cond_init(&this->notEmpty,NULL)!=0
      )
    {
        std::cout<<"mutex or condition init failed"<<std::endl;
    }


    this->shutdown= false;

    pthread_create(&this->managerID,NULL,manager,this);
    for(int i=0;i<min;i++)
    {
        pthread_create(&this->threadIDs[i],NULL,worker,this);
    }
}

ThreadPoolWithClass::~ThreadPoolWithClass() {
    shutdown= true;
    pthread_join(managerID,nullptr);
    for(int i=0;i<liveNum;++i)
        pthread_cond_signal(&notEmpty);
    //释放堆
    delete taskQ;
    delete[] threadIDs;

    pthread_mutex_destroy(&mutexPool);

    pthread_cond_destroy(&notEmpty);

}

void ThreadPoolWithClass::addTask(Task task) {
    pthread_mutex_lock(&mutexPool);

    if(shutdown)
    {
        pthread_mutex_unlock(&mutexPool);
        return;
    }
    //添加任务
    this->taskQ->addTask(task);

    //唤醒阻塞的工作
    pthread_cond_signal(&notEmpty);

    pthread_mutex_unlock(&mutexPool);
}

int ThreadPoolWithClass::getBusyNum() {

    pthread_mutex_lock(&mutexPool);
    int busynum= this->busyNum;
    pthread_mutex_unlock(&mutexPool);
    return busynum;
}

int ThreadPoolWithClass::getAliveNum() {

    pthread_mutex_lock(&mutexPool);
    int livenum=this->liveNum;
    pthread_mutex_unlock(&mutexPool);
    return livenum;
}

void *ThreadPoolWithClass::worker(void *arg) {
    ThreadPoolWithClass*pool=static_cast<ThreadPoolWithClass*>(arg);
    while(1)
    {
        //读取任务队列，访问线程池，所以要加锁
        pthread_mutex_lock(&pool->mutexPool);
        //判断任务队列是否为空
        //当前没任务，就要阻塞
        while(pool->taskQ->taskNumber()==0&&!pool->shutdown)
        {
            pthread_cond_wait(&pool->notEmpty,&pool->mutexPool);
            if(pool->exitNum>0)
            {
                pool->exitNum--;
                if(pool->liveNum>pool->minNum)
                    pool->liveNum--;
                pthread_mutex_unlock(&pool->mutexPool);
                pool->ThreadExit();
            }

        }
        //开始消费
        //如果线程池已经关闭
        if(pool->shutdown)
        {
            pthread_mutex_unlock(&pool->mutexPool);
            pool->ThreadExit();
        }
        //从任务队列取出任务
        Task task=pool->taskQ->takeTask();
        //忙的线程加1
        pool->busyNum++;
        //唤醒生产者
        pthread_mutex_unlock(&pool->mutexPool);

        //执行取出的任务
        printf("thread %ld start working..\n",pthread_self());


        task.function(task.arg);
        free(task.arg);
        task.arg=nullptr;
        pthread_mutex_lock(&pool->mutexPool);
        printf("thread %ld end working..\n",pthread_self());
        pool->busyNum--;
        pthread_mutex_unlock(&pool->mutexPool);

    }
}

void *ThreadPoolWithClass::manager(void *arg) {
    auto*pool=static_cast<ThreadPoolWithClass*>(arg);
    //以一定频率检测是否需要对工作者线程调节
    while(!pool->shutdown)
    {
        sleep(3);
        //取出线程池中任务的数量和当前线程的数量 //取出忙线程的数量
        pthread_mutex_lock(&pool->mutexPool);
        int queueSize=pool->taskQ->taskNumber();
        int liveNum=pool->liveNum;
        int busyNum=pool->busyNum;
        pthread_mutex_unlock(&pool->mutexPool);


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
void ThreadPoolWithClass::ThreadExit() {
    pthread_t tid= pthread_self();
    for(int i=0;i<maxNum;++i)
    {
        if(threadIDs[i]==tid)
        {
            threadIDs[i]=0;
            printf("threadExit() called,%llu exiting\n",tid);
            break;
        }
    }

    pthread_exit(nullptr);
}