#pragma once
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include <deque>
#include <memory>
#include <queue>

#include "../Thread/Mutex.h"
#include "../Thread/noncopyable.h"

//定时器节点,其中数据类为T
template <typename T>
class TimerNode {
 public:
  TimerNode(std::shared_ptr<T> requestData, int timeout);
  TimerNode(TimerNode<T> &tn);
  ~TimerNode();
  void update(int timeout);
  bool isValid();
  void clearReq();
  void setDeleted() { deleted_ = true; }
  bool isDeleted() const { return deleted_; }
  size_t getExpTime() const { return expiredTime_; }

 private:
  bool deleted_;
  size_t expiredTime_;
  std::shared_ptr<T> SPData_;
};
template <typename T>
TimerNode<T>::TimerNode(std::shared_ptr<T> requestData, int timeout)
    : deleted_(false), SPData_(requestData) {
  struct timeval now;
  gettimeofday(&now, NULL);
  //以毫秒计算
  expiredTime_ =
      (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

template <typename T>
TimerNode<T>::TimerNode(TimerNode<T> &tn)
    : SPData_(tn.SPData_), expiredTime_(0) {}

template <typename T>
TimerNode<T>::~TimerNode() {
  // if(SPData_) SPData->handleClose();
}

template <typename T>
void TimerNode<T>::update(int timeout) {
  struct timeval now;
  gettimeofday(&now, NULL);
  expiredTime_ =
      (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

//查看该定时器是否已过定时时间
template <typename T>
bool TimerNode<T>::isValid() {
  struct timeval now;
  gettimeofday(&now, NULL);
  size_t temp = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000));
  if (temp < expiredTime_)
    return true;
  else {
    this->setDeleted();
    return false;
  }
}
template <typename T>
void TimerNode<T>::clearReq() {
  SPData_.reset();
  this->setDeleted();
}

struct TimerCmp {
  bool operator()(std::shared_ptr<TimerNode<T>> &a,
                  std::shared_ptr<TimerNode<T>> &b) const {
    return a->getExpTime() > b->getExpTime();
  }
};

//定时器类
//底层数据结构使用STL中的priority_queue(使用deque实现)
//小根堆,堆顶是最早的定时器
template <typename T>
class TimerManager {
 public:
  TimerManager() {}
  ~TimerManager() {}
  //对外的两个接口
  void addTimer(std::shared_ptr<T> SPData, int timeout);
  //处理已超时及设置deleted_标志的定时器(懒惰删除)
  void handleExpiredEvent();

 private:
  typedef std::shared_ptr<TimerNode<T>> SPTimerNode;
  std::priority_queue<SPTimerNode, std::deque<SPTimerNode>, TimerCmp>
      timerNodeQueue_;
};

template <typename T>
void TimerManager<T>::addTimer(std::shared_ptr<T> SPData, int timeout) {
  SPTimerNode node(new TimerNode<T>(SPData, timeout));
  timerNodeQueue_.push(new_node);
  // SPData->linkTimer(new_node);
}

/**
 * 懒惰删除
 * 处理逻辑为:
 * 优先队列根据其实现结构可能无法实现随机访问,且若可以随机访问,删除某个节点
 * 也会破坏堆的结构.故此处对于已经设置为deleted_的节点何时真正删除?
 * 1. 延迟到超时时候删除.
 * 2. 或者它之前的所有节点都被删除时,它才真正被删除.
 * 故一个点被设为deleted_,它最迟会在TIMER_TIME_OUT时间后删除
 * 这样做的好处是:
 * 1.不用遍历队列
 * 2.给超时时间一个容忍的时间，就是设定的超时时间是删除的下限(并不是一到超时时间就立即删除)，
 * 如果监听的请求在超时后的下一次请求中又一次出现了，
 * 就不用再重新申请RequestData节点了，这样可以继续重复利用前面的RequestData，减少了一次delete和一次new的时间。
 *
 */
template <typename T>
void TimerManager<T>::handleExpiredEvent() {
  while (!timerNodeQueue_.empty()) {
    SPTimerNode ptimer_now = timerNodeQueue_.top();
    if (ptimer_now->isDeleted())
      timerNodeQueue.pop();
    else if (ptimer_now->isValid() == false)
      timerNodeQueue.pop();
    else
      break;
  }
}
