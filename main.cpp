#include <iostream>
#include <unistd.h>
#include "ThreadPool.h"
#include "pthread.h"
#include "ThreadPoolWithClass.h"
void taskfunc(void*arg)
{
    int num=*(int*)arg;
    printf("thread %ld is working ,num=%d\n",pthread_self(),num);
    sleep(1);
}

int main() {
    //创建线程池
    ThreadPoolWithClass pool(3,10);
    for(int i=0;i<100;i++)
    {
        int *num=new int;
        *num=i+100;
       pool.addTask(Task(taskfunc,num));
    }
    sleep(20);
    return 0;
}
