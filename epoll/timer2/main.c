#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h> 
#include <fcntl.h>
#include <sys/epoll.h>
#include <string.h>

/***************
高并发场景下的定时器
*****************/

//定时器回调函数
typedef void *(*TimerCallback)(int fd, void *);

typedef enum enFD_TYPE
{
    FD_TIMER,
    FD_SOCKET,
    FD_FILE,
}ENFD_TYPE;

typedef struct STEpollParam_s
{
    int fd;                  //文件描述符
    ENFD_TYPE enType;        //文件描述符类型
    TimerCallback cb;        //定时器回调函数
    void * pvParam;          //回调函数参数
} STEpollParam;

//创建定时器对象
int createTimer(unsigned int uiSec, unsigned int uiNsec);

//设置文件描述符非阻塞
int setNoBlock(int fd);

//创建epoll
int createEpoll();

//添加文件描述符到epoll
int addFdToEpoll(int epfd, STEpollParam *pstParam);

//消息处理
int recvMsg(void *pvParam);

//等待消息
void epollWait(int epfd);


#define EPOOL_SIZE 32000
#define EPOOL_EVENT 32


/********************************************************
   Func Name: createTimer
Date Created: 2018-7-30
 Description: 创建定时器对象
       Input: uiTvSec：设置间隔多少秒
             uiTvUsec：设置间隔多少微秒
      Output: 
      Return: 文件句柄
     Caution: 
*********************************************************/
int createTimer(unsigned int uiSec, unsigned int uiNsec)
{
    int iRet = 0;
    int tfd = 0;
    struct itimerspec timeValue;

    //初始化定时器
    /*
     When the file descriptor is no longer required it should be closed.  
     When all file descriptors associated with the same timer object have been closed, 
     the timer is disarmed and its resources are freed by the kernel.

     意思是该文件句柄还是需要调用close函数关闭的
    */
    tfd = timerfd_create(CLOCK_REALTIME,0);
    if (tfd < 0)
    {
        return -1;
    }

    //设置开启定时器
    /*
    Setting either field of new_value.it_value to a nonzero value arms the timer. 
    Setting both fields of new_value.it_value to zero disarms the timer.
    意思是如果不设置it_interval的值非零，那么即关闭定时器
    */
    timeValue.it_value.tv_sec = 1;
    timeValue.it_value.tv_nsec = 0;

    //设置定时器周期
    timeValue.it_interval.tv_sec = (time_t)uiSec;
    timeValue.it_interval.tv_nsec = (long)uiNsec;

    iRet = timerfd_settime(tfd, 0, &timeValue, NULL);
    if (iRet < 0)
    {
        return -1;
    }

    return tfd;
}

/********************************************************
   Func Name: setNoBlock
Date Created: 2018-7-27
 Description: 设置文件描述符非阻塞
       Input: fd：文件描述符
      Output:         
      Return: error code
     Caution: 
*********************************************************/
int setNoBlock(int fd)
{
    int iRet = -1;

    int iOption = -1;

    iOption = fcntl(fd, F_GETFD);
    if(iOption < 0)
    {
        iRet = -1;
        return iRet;
    }

    iOption = iOption | O_NONBLOCK;

    iOption = fcntl(fd,F_SETFD,iOption);
    if(iOption < 0)
    {
        iRet = -1;
        return iRet;
    }

    return 0;
}

/********************************************************
   Func Name: createEpoll
Date Created: 2018-7-30
 Description: 创建epoll
       Input: 
      Output:         
      Return: epoll句柄
     Caution: 
*********************************************************/
int createEpoll()
{
    int epfd = 0;

    /*
    When no longer required,
    the file descriptor returned by epoll_create() should be closed byusing close(2).  
    When all file descriptors referring to an epoll instance have been closed, 
    the kernel destroys the instance and releases the associated resources for reuse.

    意思是用完需要调用close函数关闭epoll句柄
    */
    epfd = epoll_create(EPOOL_SIZE);
    if (epfd < 0)
    {
        return -1;
    }

    return epfd;
}

/********************************************************
   Func Name: addFdToEpoll
Date Created: 2018-7-30
 Description: 添加文件描述符到epoll
       Input: 
      Output:         
      Return: error code
     Caution: 
*********************************************************/
int addFdToEpoll(int epfd, STEpollParam *pstParam)
{
    int iRet = -1;
    struct epoll_event ev;
    ev.data.ptr = pstParam;
    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
    iRet = epoll_ctl(epfd, EPOLL_CTL_ADD, pstParam->fd, &ev);
    if (iRet < 0)
    {
        return -1;
    }
    return 0;
}

/********************************************************
   Func Name: recvMsg
Date Created: 2018-7-30
 Description: 消息处理
       Input: pvParam：参数指针
      Output:         
      Return: 
     Caution: 
*********************************************************/
int recvMsg(void *pvParam)
{
    int iRet = -1;
    STEpollParam * pstParam = NULL;

    if (NULL == pvParam)
    {
        iRet = -1;
        return iRet;
    }
    pstParam = (STEpollParam *)pvParam;
    switch(pstParam->enType)
    {
    case FD_TIMER:
        pstParam->cb(pstParam->fd, pstParam->pvParam);
        break;
    default:
        break;
    }
    return 0;
}

/********************************************************
   Func Name: epollWait
Date Created: 2018-7-30
 Description: 等待消息
       Input: epfd：epoll句柄
      Output:         
      Return: 
     Caution: 
*********************************************************/
void epollWait(int epfd)
{
    int i = 0;
    int nfds = 0;
    struct epoll_event *events = NULL;

    //分配epoll事件内存
    events = (struct epoll_event *)malloc(sizeof(struct epoll_event)*EPOOL_EVENT);
    if (NULL == events)
    {
        return ;
    }
    memset(events, 0, sizeof(struct epoll_event)*EPOOL_EVENT);

    for (;;)
    {
        nfds = epoll_wait(epfd, events, EPOOL_EVENT, -1);
        if (nfds < 0)
        {
            break;
        }
        for (i = 0; i < nfds; i++ )
        {
            //监听读事件
            if (events[i].events & EPOLLIN)
            {
                recvMsg(events[i].data.ptr);
            }
        }
    }

    //关闭epoll
    close(epfd);

    return ;
}

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
    STEpollParam *pstParam = malloc(sizeof(STEpollParam));
    int tfd = 0;
    int num = 10;

    //初始化epoll
    int epfd = createEpoll();
    if (epfd < 0)
    {
        printf("createEpoll() failed .");
        return -1;
    }
    //初始化定时器
    tfd = createTimer(INTERVAL,0);
    if (tfd < 0)
    {
        printf("createTimer() failed .");
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

    free(pstParam);

    return 0;
}

int main()
{
    test();
    getchar();
    return 0;
}









