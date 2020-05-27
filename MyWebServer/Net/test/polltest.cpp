#include <unistd.h>
#include <sys/timerfd.h>
#include <poll.h>
#include "../EventLoop.h"
#include "../Channel.h"
EventLoop *g_loop;

void timeout()
{
    printf("Timeout!\n");
    g_loop->quit();
}

int main(){
    //setenv
    ::setenv("USE_POLL","pollpoller",1);
    EventLoop loop;
    g_loop = &loop;
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK | TFD_CLOEXEC);
    Channel channel(&loop,timerfd);
    channel.setReadHandler(timeout);
    channel.setEvents(POLLIN | POLLPRI);
    loop.updateChannel(&channel);

    struct itimerspec howlong;
    bzero(&howlong,sizeof howlong);
    howlong.it_value.tv_sec = 5;
    ::timerfd_settime(timerfd,0,&howlong,NULL);

    loop.loop();
    ::close(timerfd);
}