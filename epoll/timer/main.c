#include <iostream>

using namespace std;

#include "mytimer.h"

//#include <sys/timerfd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "comontype.h"

#define INTERVAL 3

void * timerFunc(int fd, void *pvParam)
{
    long data = 0;
    int *p = NULL;

    p = (int *)pvParam;
    read(fd, &data, sizeof(long));
    printf("data = %lu and p = %d \n", data, *p);

    return NULL;
}

int test()
{
    STEpollParam *pstParam =new STEpollParam;
    int tfd = 0;
    int num = 10;

    //初始化epoll
    int epfd = createEpoll();
    if (epfd < 0)
    {
        cout << "createEpoll() failed ." << endl;
        return -1;
    }
    //初始化定时器
    tfd = createTimer(INTERVAL,0);
    if (tfd < 0)
    {
        cout << "createTimer() failed ." << endl;
        return -1;
    }


    pstParam->fd = tfd;
    pstParam->enType = FD_TIMER;
    pstParam->cb = timerFunc;
    pstParam->pvParam = &num;

    addFdToEpoll(epfd, pstParam);

    epollWait(epfd);

    //关闭定时器文件描述符
    close(tfd);

    DE_FREE(pstParam);

    return 0;
}

int main()
{
    test();
    getchar();
    return 0;
}

