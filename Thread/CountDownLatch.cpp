#include "CountDownLatch.h"
CountDownLatch::CountDownLatch(int count)
    : mutex_(), condition_(mutex_), count_(count) {}

void CountDownLatch::wait(){
    MutexLockGuard(&mutex_);
    //注意这里的循环,防止虚假唤醒
    while (count_>0)
    {
        condition_.wait();
    }
}

void CountDownLatch::countDown(){
    MutexLockGuard(&mutex_);
    --count_;
    //通知其他所有子线程
    if(count_==0) condition_.notifyAll();
}

int CountDownLatch::getCount() const {
    MutexLockGuard(&mutex_);
    return count_;
}