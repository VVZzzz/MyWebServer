#include "EventLoop.h"

#include <poll.h>

#include "../Log/Logging.h"
#include "Channel.h"
#include "Poller.h"

using namespace std;

//__thread变量,指示该EventLoop在哪个线程
__thread EventLoop *t_loopInThisThread = 0;

const int kPollTimeMs = 10000;

EventLoop::EventLoop()
    : looping_(false),
      threadId_(CurrentThread::tid()),
      quit_(false),
      poller_(Poller::newDefaultPoller(this)) {
  if (t_loopInThisThread)
    LOG << "Another EventLoop " << t_loopInThisThread
        << " exists in this thread " << threadId_;
  else
    t_loopInThisThread = this;
}

EventLoop::~EventLoop() {
  assert(!looping_);
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
    poller_->poll(kPollTimeMs, &activeChannels_);
    for (auto &itr : activeChannels_) itr->handleEvents();
  }
  // LOG << "EventLoop " << this << " stop looping";
  looping_ = false;
}

void EventLoop::quit(){
  quit_ = true;
  //if(!isInLoopThread()){
    //wakeup();
  //}
}

void EventLoop::updateChannel(Channel *channel)
{
  poller_->updateChannel(channel);
}