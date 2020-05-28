#include "EventLoop.h"

#include <poll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include "../Log/Logging.h"
#include "Channel.h"
#include "Poller.h"
#include "Util.h"

using namespace std;

//__thread变量,指示该EventLoop在哪个线程
__thread EventLoop *t_loopInThisThread = 0;

// wakeup唤醒线程用,传统唤醒一个线程(阻塞在poll中)方式是
//使用pipe,然后监听这个pipefd,调用线程通过pipe给本线程发1个字节的数据来唤醒
//但这里只为了唤醒,使用eventfd更好!详见
// https://zhuanlan.zhihu.com/p/40572954
int createEventfd() {
  int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0) {
    LOG << "Failed in eventfd";
    abort();
  }
}

const int kPollTimeMs = 10000;

EventLoop::EventLoop()
    : looping_(false),
      threadId_(CurrentThread::tid()),
      quit_(false),
      poller_(Poller::newDefaultPoller(this)),
      callingPendingFunctors_(false),
      eventHandling_(false),
      wakeupFd_(createEventfd()) {
  if (t_loopInThisThread)
    LOG << "Another EventLoop " << t_loopInThisThread
        << " exists in this thread " << threadId_;
  else
    t_loopInThisThread = this;
}

EventLoop::~EventLoop() {
  assert(!looping_);
  close(wakeupFd_);
  t_loopInThisThread = NULL;
}

// one loop perthread
void EventLoop::loop() {
  assert(!looping_);
  assertInLoopThread();
  looping_ = true;
  quit_ = false;
  // LOG << "EventLoop " << this << " start looping";
  while (!quit_) {
    activeChannels_.clear();
    // IO线程阻塞在此处
    //其中wakeup就是在此处进行唤醒
    poller_->poll(kPollTimeMs, &activeChannels_);
    //处理poll拿到的事件
    eventHandling_ = true;
    for (auto &itr : activeChannels_) itr->handleEvents();
    eventHandling_ = false;
    //处理用户定义的回调事件(即pendingFunctors中的函数)
    doPendingFunctors();
  }
  // LOG << "EventLoop " << this << " stop looping";
  looping_ = false;
}

void EventLoop::quit() {
  quit_ = true;
  //如果不是在当前线程调用,就唤醒(不让它阻塞在poll)
  //可以及时终止循环
  if (!isInLoopThread()) {
    wakeup();
  }
}

void EventLoop::updateChannel(Channel *channel) {
  poller_->updateChannel(channel);
}

//这个函数主要用于在IO线程中执行用户回调cb()
//如果不是在本线程调用的runInLoop,那么就将此cb()
//加入到队列中(queueInLoop),然后唤醒IO线程进行回调
//这个功能使得能在线程之间调配任务
void EventLoop::runInLoop(Functor &&cb) {
  if (isInLoopThread())
    cb();
  else
    queueInLoop(std::move(cb));
}

//将cb加入到队列中,必要时唤醒IO线程
//必要时刻为:1. 调用queueInLoop线程不是当前IO线程
// 2. 是IO线程,但是现在IO线程是正在调用doingpendingFunctor(而非是poll得到的事件)
//对于2是由于这些pendingFunc有可能继续调用queueInLoop(cb),如果不唤醒那么cb就不能及时
//处理
//查看doingpendingfunctor在loop的位置就明白了
void EventLoop::queueInLoop(Functor &&cb) {
  //临界区
  {
    MutexLockGuard lock(mutex_);
    pendingFunctors_.emplace_back(std::move(cb));
  }
  if (!isInLoopThread() || callingPendingFunctors_) wakeup();
}

void EventLoop::doPendingFunctors() {
  std::vector<Functor> functors;
  callingPendingFunctors_ = true;
  {
    MutexLockGuard lock(mutex_);
    //使用局部变量和pendingFunctors_交换
    //一方面使得临界区代码变短,另一方面防止死锁,因为functor中又可能仍旧调用doPendingFunctors
    functors.swap(pendingFunctors_);
  }
  for (size_t i = 0; i < functors.size(); i++) functors[i]();
  callingPendingFunctors_ = false;
}

//向wakeupFd_发送8个字节(uint64)
void EventLoop::wakeup() {
  uint64_t one = 1;
  ssize_t n = writen(wakeupFd_, (char *)(&one), sizeof one);
  if (n != sizeof one) {
    LOG << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
  }
}

//从wakeupFd_中读数据
void EventLoop::handleRead() {
  uint64_t one = 1;
  ssize_t n = readn(wakeupFd_, &one, sizeof one);
  if (n != sizeof one) {
    LOG << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
  }
}