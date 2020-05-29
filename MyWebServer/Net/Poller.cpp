#include "Poller.h"

#include "Channel.h"
#include "PollPoller.h"

Poller::Poller(EventLoop *loop) : ownerLoop_(loop) {}
Poller::~Poller() = default;
bool Poller::hasChannel(Channel *channel) const {
  auto itr = channels_.find(channel->getFd());
  return itr != channels_.end() && itr->second == channel;
}
Poller *Poller::newDefaultPoller(EventLoop *loop) {
  //if (::getenv("USE_POLL"))
    //return new PollPoller(loop);
  //else
  //  return new EPollPoller(loop);
}