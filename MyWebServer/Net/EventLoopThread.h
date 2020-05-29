#pragma once
#include "../Thread/Condition.h"
#include "../Thread/Mutex.h"
#include "../Thread/noncopyable.h"
#include "EventLoop.h"
/**
 * EventLoopThread类 : 是将eventloop和thread连接起来
 * 就是说现在我处于主线程中,但是这个主线程不是IO线程,那么我想遵循
 * one loop perthread , 那么我应该创建一个IO线程并运行EventLoop
 * EventLoopThread就是完成这个事情的,我在本线程中定义
 * EventLoopThread并startloop,即开启一个IO线程并开始eventloop
 * 同时获得eventloop指针
 */ 

class EventLoopThread : noncopyable {
 public:
  EventLoopThread();
  ~EventLoopThread();
  //EventLoop指针生存周期和开启的IO线程一样,这点注意
  EventLoop *startloop();

 private:
  void threadFunc();
  EventLoop *loop_;
  bool exiting_;
  Thread thread_;
  MutexLock mutex_;
  Condition cond_;
};