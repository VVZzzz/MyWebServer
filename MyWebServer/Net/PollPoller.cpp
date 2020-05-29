#include "PollPoller.h"

#include <assert.h>
#include <errno.h>
#include <poll.h>

#include "../Log/Logging.h"
#include "Channel.h"

PollPoller::PollPoller(EventLoop* loop) : Poller(loop) {}

PollPoller::~PollPoller() = default;

int PollPoller::poll(int timeoutMs, ChannelList* activeChannels) {
  // XXX pollfds_ shouldn't change
  int numEvents = ::poll(&*pollfds_.begin(), pollfds_.size(), timeoutMs);
  int saveErrno = errno;
  if (numEvents > 0) {
    LOG << numEvents << " events happend";
    fillActiveChannels(numEvents,activeChannels);
  } else if (numEvents == 0) {
    LOG << "nothing happend";
  } else {
    if (saveErrno != EINTR) {
      errno = saveErrno;
      LOG << "poll error: PollPoller::Poll()";
    }
  }
  return 1;
}

//遍历pollfds_,将有事件的fd,其对应的channel添加到activeChannels中
// O(N),此处不能一遍遍历pollfds_一遍Channel::handleEvent
//后者会添加或删除Channel,且poll只负责IO复用,不管事件如何分发处理
void PollPoller::fillActiveChannels(int numEvents,
                                    ChannelList* activeChannels) const {
  for (PollFdList::const_iterator itr = pollfds_.begin();
       itr != pollfds_.end() && numEvents > 0; itr++) {
    //有事件
    if (itr->revents > 0) {
      --numEvents;
      ChannelMap::const_iterator ch = channels_.find(itr->fd);
      assert(ch != channels_.end());
      Channel* channel = ch->second;
      assert(channel->getFd() == itr->fd);
      //设置对应channel的事件
      channel->setRevents(itr->revents);
      // itr->revents = 0;
      activeChannels->push_back(channel);
    }
  }
}

/*
//维护和更新pollfds_数组,添加新的Channel复杂度为O(logN)
//更新已有的Channel为O(1),因为index()可获取下标
// removeChannel是O(logN)
void PollPoller::updateChannel(Channel* channel) {
  LOG << "fd = " << channel->getFd() << " events = " << channel->getEvents();
  //添加新的channel到ChannelMap,并添加fd到pollfds
  if (channel->index() < 0) {
    struct pollfd pfd;
    pfd.fd = channel->getFd();
    pfd.events = static_cast<short>(channel->getEvents());
    pfd.revents = 0;
    pollfds_.push_back(pfd);
    int idx = static_cast<int>(pollfds_.size()) - 1;
    channel->set_index(idx);
    channels_[pfd.fd] = channel;
  } else {
    //更新已有的channel,更新ChannelMap和pollfds
    assert(channels_.find(channel->getFd()) != channels_.end());
    assert(channels_[channel->getFd()] == channel);
    int idx = channel->index();
    struct pollfd& pfd = pollfds_[idx];
    pfd.fd = channel->getFd();
    pfd.events = static_cast<short>(channel->getEvents());
    pfd.revents = 0;
    //若感兴趣的events为0
    if (pfd.events == 0) {
      //忽略此fd
      pfd.fd = -channel->getFd() - 1;
    }
  }
}
*/

/*
void PollPoller::removeChannel(Channel* channel) {
  LOG << "remove channel fd = " << channel->getFd();
  assert(channels_.find(channel->getFd()) != channels_.end());
  assert(channels_[channel->getFd()] == channel);
  assert(channel->getEvents() == 0);
  int idx = channel->index();
  const struct pollfd& pfd = pollfds_[idx];
  (void)pfd;
  //在ChannelMap中删除这个channel(根据fd)
  size_t n = channels_.erase(channel->getFd());
  assert(n == 1);
  (void)n;
  //并从pollfds中删除这个fd
  //若这个fd正好是最后一个元素,直接pop_back
  if (static_cast<size_t>(idx) == pollfds_.size() - 1)
    pollfds_.pop_back();
  else {
    //不是,则跟最后一个元素换位,再pop_back
    int channelfdAtEnd = pollfds_.back().fd;
    std::iter_swap(pollfds_.begin() + idx, pollfds_.end() - 1);
    // fd<0,说明是它的events为0(看update函数,events为0的fd都设为-x-1)
    if (channelfdAtEnd < 0) channelfdAtEnd = -channelfdAtEnd - 1;
    //更新最后一个channel的idx
    channels_[channelfdAtEnd]->set_index(idx);
    pollfds_.pop_back();
  }
}
*/