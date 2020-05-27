#pragma once
#include <vector>

#include "Channel.h"
#include "Poller.h"

/**
 * Poll的IO复用机制类,继承自Poller抽象基类
 */
//前向声明,减少编译工作量
struct pollfd;

class PollPoller : public Poller {
  public:
  PollPoller(EventLoop *loop);
  ~PollPoller() override;
  int poll(int timeoutMs, ChannelList* activeChannels) override;
  void updateChannel(Channel* channel) override;
  void removeChannel(Channel* channel) override;

 private:
  void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
  typedef std::vector<struct pollfd> PollFdList;
  PollFdList pollfds_;
};