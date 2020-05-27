#pragma once
/**
 * Poller作为基类 , 具体实现类有Poll , 和EPOLL两个IO复用机制
 * 该类不拥有Channel objects
 */
#include <map>
#include <vector>

#include "Channel.h"
#include "EventLoop.h"

class Poller : noncopyable {
 public:
  typedef std::vector<Channel*> ChannelList;
  Poller(EventLoop* loop);
  virtual ~Poller();

  /// Polls the I/O events.
  /// Must be called in the loop thread.
  virtual int poll(int timeoutMs, ChannelList* activeChannels) = 0;

  /// Changes the interested I/O events.
  /// Must be called in the loop thread.
  virtual void updateChannel(Channel* channel) = 0;

  /// Remove the channel, when it destructs.
  /// Must be called in the loop thread.
  virtual void removeChannel(Channel* channel) = 0;

  virtual bool hasChannel(Channel* channel) const;

  //静态函数,得到Poller(Poll或Epoll)实例
  static Poller *newDefaultPoller(EventLoop *loop);

 protected:
  // fd <---> Channel*
  typedef std::map<int, Channel*> ChannelMap;
  ChannelMap channels_;

 private:
  EventLoop* ownerLoop_;
};
