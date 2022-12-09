#ifndef THREAD_POOL_TASKQUEUE_H
#define THREAD_POOL_TASKQUEUE_H

#include "queue"
#include "pthread.h"
using callback=void(*)(void*arg);
typedef struct Task
{
   callback function;
   void *arg;
   Task()
   {
       function= nullptr;
       arg= nullptr;
   }
   Task(callback f,void*arg)
   {
       this->arg=arg;
       function=f;
   }

}Task;
class TaskQueue {
private:
    pthread_mutex_t m_mutex;
    std::queue<Task> m_taskQ;
public:
    TaskQueue();
    ~TaskQueue();
    //添加任务
    void  addTask(Task task);
    void  addTask(callback f,void*arg);
    //取出任务
    Task takeTask();
    inline int taskNumber()
    {
        return m_taskQ.size();
    }
};


#endif //THREAD_POOL_TASKQUEUE_H
