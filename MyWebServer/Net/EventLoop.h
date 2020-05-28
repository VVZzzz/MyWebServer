#pragma once
#include <stdlib.h>
#include <sys/types.h>

#include <atomic>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

#include "../Log/Logging.h"
#include "../Thread/CurrentThread.h"
#include "../Thread/Thread.h"
#include "../Thread/noncopyable.h"
class Poller;
class Channel;

class EventLoop : noncopyable {
 public:
  typedef std::function<void()> Functor;
  EventLoop();
  ~EventLoop();

  void assertInLoopThread() {
    if (!isInLoopThread()) abortNotInLoopThread();
  }
  bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

  void loop();
  void quit();  //跳出该loop
  void runInLoop(Functor &&cb);
  void queueInLoop(
      Functor &&
          cb);  //别的线程会调用runInLoop进而调用queueInLoop,进而操作pendingFunctors_,故需锁保护

  // internal usage
  void updateChannel(Channel *channel);
  void removeChannel(Channel *channel);
  bool hasChannel(Channel *channel);

 private:
  typedef std::vector<Channel *> ChannelList;
  void abortNotInLoopThread() { abort(); }
  // internal usage
  void wakeup();
  void doPendingFunctors();
  void handleRead();

  bool looping_;
  const pid_t threadId_;
  std::atomic<bool> quit_;
  std::unique_ptr<Poller> poller_;
  ChannelList activeChannels_;  //存放活动的Channel
  mutable MutexLock mutex_;     //锁pendingFunctors_
  std::vector<Functor>
      pendingFunctors_;  //存放待回调的用户回调函数(即该thread一直loop,那么此时我想让这个线程干点其他的事)
  bool callingPendingFunctors_;  //是否正在执行pendingFunctors
  bool eventHandling_;           //是否正在处理event

  int wakeupFd_;
};