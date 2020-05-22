#pragma once
#include "Condition.h"
#include "Mutex.h"
#include "noncopyable.h"

//倒计时
//参考muduo2.2节讲述的CountDownLatch
//这里主要是为了当主线程开启子线程时,子线程一定已接收线程数据并开始真正执行函数时,
//主线程才从外层start返回
class CountDownLatch : noncopyable
{
public:
    explicit CountDownLatch(int count);  //倒数几次
    void wait();  //等待计数变为0
    void countDown();  //计数减一
    int getCount() const;

private:
    //注意此处mutex_为mutable变量,原因是
    //对于getCount这个const函数,虽然是const但我们需要
    //对mutex进行加锁才能读count,mutable使得在const函数中
    //也能修改这个变量
    mutable MutexLock mutex_;
    Condition condition_;
    int count_;
};