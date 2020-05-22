#pragma once
#include "Mutex.h"
#include "noncopyable.h"
#include <cstdint>
#include <errno.h>
#include <pthread.h>
#include <time.h>

//条件变量包裹类
class Condition : noncopyable
{
public:
    explicit Condition(MutexLock &_mutex) : mutexlock(_mutex)
    {
        pthread_cond_init(&cond, NULL);
    }
    ~Condition() { pthread_cond_destroy(&cond); }
    //这里注意条件变量的虚假唤醒:https://www.zhihu.com/question/271521213
    void wait() { pthread_cond_wait(&cond, mutexlock.get()); }
    void notify() { pthread_cond_signal(&cond); }
    void notifyAll() { pthread_cond_broadcast(&cond); }
    bool waitForSeconds(int seconds)
    {
        struct timespec abstime;
        clock_gettime(CLOCK_REALTIME, &abstime);
        abstime.tv_sec += static_cast<time_t>(seconds);
        //若超时还没有等到条件变量 , 返回true,并重新加锁
        return ETIMEDOUT == pthread_cond_timedwait(&cond, mutexlock.get(), &abstime);
    }

private:
    MutexLock &mutexlock;
    pthread_cond_t cond;
}