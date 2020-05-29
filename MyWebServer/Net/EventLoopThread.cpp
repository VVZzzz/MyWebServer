#include "EventLoopThread.h"

#include <functional>

EventLoopThread::EventLoopThread()
    : loop_(NULL),
      exiting_(false),
      //使用bind初始化function对象
      thread_(std::bind(&EventLoopThread::threadFunc, this), "EventLoopThread"),
      mutex_(),
      cond_(mutex_) {}
EventLoopThread::~EventLoopThread() {
  exiting_ = true;
  if (loop_ != NULL) {
    loop_->quit();
    //停止线程
    thread_.join();
  }
}

EventLoop* EventLoopThread::startloop() {
  assert(!thread_.started());
  //子线程开启
  thread_.start();
  {
    MutexLockGuard lock(mutex_);
    // 一直等到threadFun在Thread里真正跑起来
    //因为只有真正进入了threadFunc才会将loop_设为非null
    while (loop_ == NULL) cond_.wait();
  }
  return loop_;
}

void EventLoopThread::threadFunc() {
  EventLoop loop;
  {
    MutexLockGuard lock(mutex_);
    loop_ = &loop;
    cond_.notify();
  }
  //开始loop
  loop.loop();
  //退出loop
  loop_ = NULL;
}