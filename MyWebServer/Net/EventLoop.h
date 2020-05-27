#pragma once
#include <sys/types.h>

#include <functional>
#include <iostream>
#include <memory>
#include <vector>
#include <stdlib.h>
#include <atomic>

#include "../Log/Logging.h"
#include "../Thread/CurrentThread.h"
#include "../Thread/Thread.h"
#include "../Thread/noncopyable.h"
class Poller;
class Channel;

class EventLoop : noncopyable {
 public:
  EventLoop();
  ~EventLoop();
  void loop();
  void assertInLoopThread() {
    if (!isInLoopThread()) abortNotInLoopThread();
  }
  bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
  //跳出该loop
  void quit();

  //internal usage
  void wakeup();
  void updateChannel(Channel *channel);
  void removeChannel(Channel *channel);
  bool hasChannel(Channel *channel);

 private:
 typedef std::vector<Channel *> ChannelList;
  void abortNotInLoopThread() { exit(-1);}
  bool looping_;
  const pid_t threadId_;
  std::atomic<bool> quit_;
  std::unique_ptr<Poller> poller_;

  ChannelList activeChannels_;
};