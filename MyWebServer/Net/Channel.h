#pragma once
#include <poll.h>
#include <sys/epoll.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

//#include "Timer.h"
#include "../Thread/noncopyable.h"
#include "Util.h"

//每个Channel对象只属于一个Eventloop,故只属于一个IO线程
//只负责一个fd的IO事件分发,但并不拥有该fd
//析构时并不关闭fd
class EventLoop;
class Channel : noncopyable {
 public:
  typedef std::function<void()> CallBack;
  Channel(EventLoop *loop);
  Channel(EventLoop *loop, int fd);
  ~Channel();

  // fd处理函数
  int getFd();
  void setFd(int fd);

  //设置回调函数
  void setReadHandler(CallBack &&readHandler) { readHandler_ = readHandler; }
  void setWriteHandler(CallBack &&writeHandler) {
    writeHandler_ = writeHandler;
  }
  void setErrorHandler(CallBack &&errorHandler) {
    errorHandler_ = errorHandler;
  }
  void setConnHandler(CallBack &&connHandler) { connHandler_ = connHandler; }

  // events相关函数
  void setRevents(__uint32_t ev) { revents_ = ev; }
  void setEvents(__uint32_t ev) { events_ = ev; }
  __uint32_t &getEvents() { return events_; }
  bool EqualAndUpdateLastEvents() {
    bool ret = (lastEvents_ == events_);
    lastEvents_ = events_;
    return ret;
  }
  __uint32_t getLastEvents() { return lastEvents_; }

  //包裹一层event
  /*
  void enableReading() { events_ |= READEVENT; update(); }
  void disableReading() { events_ &= ~READEVENT; update(); }
  void enableWriting() { events_ |= WRITEEVENT; update(); }
  void disableWriting() { events_ &= ~WRITEEVENT; update(); }
  void disableAll() { events_ = 0; update(); }
  bool isWriting() const { return events_ & WRITEEVENT; }
  bool isReading() const { return events_ & READEVENT; }
  */
  


  // 处理events
  void handleEvents() {
    events_ = 0;
    //写端挂起且无数据接收到
    if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
      events_ = 0;
      return;
    }
    //发生错误
    if (revents_ & POLLERR) {
      if (errorHandler_) errorHandler_();
      events_ = 0;
      return;
    }
    //有数据来或者是对方发来关闭连接请求(EPOLLRDHUP事件)
    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
      handleRead();
    }
    if (revents_ & EPOLLOUT) {
      handleWrite();
    }
    handleConn();
  }

  void handleRead();
  void handleWrite();
  void handleError(int fd, int err_num, std::string short_msg);
  void handleConn();

  // for poller
  //int index() { return index_; }
  //void set_index(int idx) { index_ = idx; }

  //inner usage
  //void remove();

 private:
 //inter usage 将channelupdate到loop里
 //void update();
  EventLoop *loop_;
  int fd_;
  //int index_;  // channel在ChannelList的下标

  __uint32_t events_;   //关心的事件
  __uint32_t revents_;  //目前活动的事件
  __uint32_t lastEvents_;

  CallBack readHandler_;
  CallBack writeHandler_;
  CallBack errorHandler_;
  CallBack connHandler_;
};

typedef std::shared_ptr<Channel> SP_Channel;