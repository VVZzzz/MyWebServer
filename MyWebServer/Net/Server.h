#pragma once
/**
 * http服务器类
 * 注意:这个server主线程只用来接受新连接
 */
#include <memory>

#include "Channel.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"

class Server {
 public:
  Server(EventLoop *loop, int threadNum_, int port);
  //用RAII机制方便析构
  ~Server() {}
  EventLoop *getLoop() const { return loop_; }
  //注意,此处start()并不是让Server主线程的loop开始loop
  //而是在main中外部,loop.loop()
  void start();
  void handleNewConn();
  void handleThisConn() { loop_->updateToPoller(acceptChannel_); }

 private:
  EventLoop *loop_;  // Server主线程的loop
  int threadNum_;    //线程池开启线程数量
  bool started_;
  int port_;
  int listenFd_;
  //线程池
  std::unique_ptr<EventLoopThreadPool> eventLoopThreadPool_;
  // accept的channel
  std::shared_ptr<Channel> acceptChannel_;
  //限制最大并发数(同时处理多少个客户)
  static const int MAXFDS = 100000;
};