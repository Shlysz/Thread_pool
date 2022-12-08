#include <iostream>
#include <unistd.h>
#include "ThreadPool.h"
#include "pthread.h"

void taskfunc(void*arg)
{
    int num=*(int*)arg;
    printf("thread %ld is working ,num=%d\n",pthread_self(),num);
    sleep(1);
}

int main() {
    //创建线程池
    ThreadPool*pool= threadPoolCreate(3,10,100);
    for(int i=0;i<100;i++)
    {
        int *num=new int;
        *num=i+100;
        threadPoolAdd(pool,taskfunc,num);
    }
    sleep(30);
    threadPoolDestory(pool);
    return 0;
}
