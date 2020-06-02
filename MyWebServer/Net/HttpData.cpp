#include "HttpData.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <iostream>

#include "Channel.h"
#include "EventLoop.h"
#include "Timer.h"
#include "Util.h"

HttpData::HttpData(EventLoop *loop, int connfd)
    : loop_(loop),
      channel_(new Channel(loop, connfd)),
      fd_(connfd),
      error_(false),
      connectionState_(H_CONNECTED),
      method_(METHOD_GET),
      HTTPVersion_(HTTP_11),
      nowReadPos_(0),
      state_(STATE_PARSE_URI),
      hState_(H_START),
      keepAlive_(false) {
  // loop_->queueInLoop(bind(&HttpData::setHandlers, this));
  //设置channel处理事件函数
  channel_->setReadHandler(bind(&HttpData::handleRead, this));
  channel_->setWriteHandler(bind(&HttpData::handleWrite, this));
  channel_->setConnHandler(bind(&HttpData::handleConn, this));
}

//重置http连接状态
void HttpData::reset() {
  // inBuffer_.clear();
  fileName_.clear();
  path_.clear();
  nowReadPos_ = 0;
  state_ = STATE_PARSE_URI;
  hState_ = H_START;
  headers_.clear();
  // keepAlive_ = false;
  //解绑定时器节点,并删除这个节点
  if (timer_.lock()) {
    std::shared_ptr<TimerNode<HttpData>> my_timer(timer_.lock());
    my_timer->clearReq();
    timer_.reset();
  }
}

//解绑定时器节点,并删除这个节点
void HttpData::seperateTimer() {
  if (timer_.lock()) {
    std::shared_ptr<TimerNode<HttpData>> my_timer(timer_.lock());
    my_timer->clearReq();
    timer_.reset();
  }
}

// Http连接的核心:处理新到来的http连接
void HttpData::handleRead() {
  __uint32_t &events = channel_->getEvents();
  //以下用do{...break...} while(false)结构
  //这样做的好处是,break可以跳过后面不想执行的语句
  do {
    bool zero = false;
    int read_num = readn(fd_, inBuffer_, zero);
    LOG << "Request: " << inBuffer_;
    if (connectionState_ == H_DISCONNECTING) {
      inBuffer_.clear();
      break;
    }
    // cout << inBuffer_ << endl;
    if (read_num < 0) {
      perror("1");
      error_ = true;
      handleError(fd_, 400, "Bad Request");
      break;
    }
    //这是read到0个字节且zero为0的情况(即下面):
    // 有请求出现但是读不到数据，可能是Request
    // Aborted，或者来自网络的数据没有达到等原因
    // 最可能是对端已经关闭了，统一按照对端已经关闭处理
    else if (zero) {
      // error_ = true;
      connectionState_ = H_DISCONNECTING;
      if (read_num == 0) {
        // error_ = true;
        break;
      }
      // cout << "readnum == 0" << endl;
    }
  } while (false);
}