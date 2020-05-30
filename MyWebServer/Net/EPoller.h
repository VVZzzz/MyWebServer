#pragma once
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <deque>
#include <memory>
#include <queue>
#include <vector>

#include "../Log/Logging.h"
#include "Channel.h"
#include "Timer.h"
#include "Util.h"

//为了做到框架与业务数据分离,采用模板
// T为使用框架的数据类型
template <typename T>
class Epoller {
 public:
  // typedef std::shared_ptr<Channel> SP_Channel;
  Epoller();
  ~Epoller();
  //添加,修改,删除channel
  void epoll_add(SP_Channel request, int timeout);
  void epoll_mod(SP_Channel request, int timeout);
  void epoll_del(SP_Channel request);
  // poll返回已就绪的channle
  std::vector<SP_Channel> poll();
  // used by poll
  void getEventsRequest(std::vector<SP_Channel>& chnvec);
  //为某个channel添加定时器
  void add_timer(SP_Channel request_data, int timeout);
  int getEpollFd() { return epollFd_; }
  //超时的channel事件处理
  void handleExpired();

 private:
  static const int MAXFDS = 100000;
  int epollFd_;
  std::vector<epoll_event> events_;
  //利用数组保存fd<--->data和fd<--->Channel对应关系
  //定义最多的fd数量为4096
  // muduo中是利用unodered_map<fd,channel*>来保存
  std::shared_ptr<Channel> fd2channel_[MAXFDS];
  std::shared_ptr<T> fd2data_[MAXFDS];
  TimerManager<T> timermanager_;
};

template <typename T>
//这里使用epoll_create1,不用epoll_create
//有细微区别见:https://cppfans.org/1418.html
Epoller<T>::Epoller()
    : epollFd_(epoll_create1(EPOLL_CLOEXEC)), events_(EVENTSUM) {
  assert(epollFd_ > 0);
}
template <typename T>
Epoller<T>::~Epoller() {}

//注册新fd
template <typename T>
void Epoller<T>::epoll_add(SP_Channel request, int timeout) {
  int fd = request->getFd();
  if (timeout > 0) {
    add_timer(request, timeout);
    // getHolder返回的是shared_ptr<void>
    //此处转换一下智能指针类型
    std::shared_ptr<T> tptr = std::static_pointer_cast<T>(request->getHolder());
    fd2data_[fd] = tptr;
    tptr.reset();
  }
  struct epoll_event event;
  event.data.fd = fd;
  event.events = request->getEvents();

  request->EqualAndUpdateLastEvents();

  fd2channel_[fd] = request;
  // epoll_add
  if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) < 0) {
    perror("epoll_add error");
    fd2channel_[fd].reset();
  }
}

//修改这个channel中的events
template <typename T>
void Epoller<T>::epoll_mod(SP_Channel request, int timeout) {
  if (timeout > 0) add_timer(request, timeout);
  int fd = request->getFd();
  //如果这个channel中的events没有变化,就不更新了
  if (!request->EqualAndUpdateLastEvents()) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->getEvents();
    if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) < 0) {
      perror("epoll_mod error");
      fd2channel_[fd].reset();
    }
  }
}

template <typename T>
void Epoller<T>::epoll_del(SP_Channel request) {
  int fd = request->getFd();
  struct epoll_event event;
  event.data.fd = fd;
  event.events = request->getLastEvents();
  // event.events = 0;
  // request->EqualAndUpdateLastEvents()
  if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &event) < 0) {
    perror("epoll_del error");
  }
  fd2channel_[fd].reset();
  fd2data_[fd].reset();
}

// epoll等待,返回活跃channel集合
template <typename T>
std::vector<SP_Channel> Epoller<T>::poll() {
  //注意此处,使用while(true),epoll等待超过10s就继续进行
  while (true) {
    // 10000ms
    int event_count =
        epoll_wait(epollFd_, &*events_.begin(), events_.size(), EPOLLWAIT_TIME);
    if (event_count < 0) perror("epoll wait error");
    //先定义且预留空间(为了效率)
    std::vector<SP_Channel> req_data(event_count);
    getEventsRequest(req_data);
    if (!req_data.empty()) return req_data;
  }
}

template <typename T>
void Epoller<T>::getEventsRequest(std::vector<SP_Channel>& chnvec) {
  int sz = chnvec.size();
  for (size_t i = 0; i < sz; i++) {
    //获取已就绪的事件
    int fd = events_[i].data.fd;
    SP_Channel cur_channel = fd2channel_[fd];
    if (cur_channel) {
      //设置revents
      cur_channel->setRevents(events_[i].events);
      cur_channel->setEvents(0);
      // 加入线程池之前将Timer和request分离
      // cur_req->seperateTimer();
      chnvec[i] = cur_channel;

    } else {
      LOG << "SP cur_req is invalid!";
    }
  }
}

template <typename T>
void Epoller<T>::add_timer(SP_Channel request_data, int timeout) {
  std::shared_ptr<T> t = std::static_pointer_cast<T>(request_data->getHolder());
  if (t)
    timermanager_.addTimer(t, timeout);
  else
    LOG << "timer add fail";
}

//删除超时的那些定时器节点
template <typename T>
void Epoller<T>::handleExpired() {
  timermanager_.handleExpiredEvent();
}