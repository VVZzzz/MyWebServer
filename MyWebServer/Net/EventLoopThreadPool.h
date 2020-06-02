#pragma once
#include <memory>
#include <vector>

#include "../Log/Logging.h"
#include "../Thread/noncopyable.h"
#include "EventLoop.h"
#include "EventLoopThread.h"

class EventLoopThreadPool : noncopyable {
 public:
  EventLoopThreadPool(EventLoop* baseLoop, int numThreads);
  ~EventLoopThreadPool() { LOG << "~EventLoopThreadPool()"; }
  void start();
  EventLoop* getNextLoop();

 private:
  EventLoop *baseLoop_;  //当前线程的eventloop指针
  bool started_;
  int numThreads_;   //开启的线程数
  int next_;  //指示下一个round robin拿到的线程的位置
  //线程池: 用vector存储EventloopThread,对象自动析构:线程退出loop停止
  std::vector<std::shared_ptr<EventLoopThread>> threads_;
  std::vector<EventLoop*> loops_;
};