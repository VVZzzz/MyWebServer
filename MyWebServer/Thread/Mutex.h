#pragma once
#include <pthread.h>

#include <cassert>

#include "CurrentThread.h"
#include "noncopyable.h"
class MutexLock : noncopyable {
 public:
  MutexLock() : hold_thread_(0) { pthread_mutex_init(&mutex_, NULL); }
  ~MutexLock() {
    assert(hold_thread_ == 0);  //解锁时恢复为0
    int ret = pthread_mutex_destroy(&mutex_);
    assert(ret == 0);
  }
  void lock() {
    int ret = pthread_mutex_lock(&mutex_);
    assert(ret == 0);
    assignHold();  //标志哪个线程获得了锁
  }
  void unlock() {
    unassignHold();  //先解绑tid
    int ret = pthread_mutex_unlock(&mutex_);
    assert(ret == 0);
  }
  //用来assert(在被锁时用)
  bool isLockedByThread() const { return hold_thread_ == CurrentThread::tid(); }
  pthread_mutex_t* get() { return &mutex_; }

 private:
  // friend class Condition;
  pthread_mutex_t mutex_;
  pid_t hold_thread_;  //持有该锁的thread

  void unassignHold() { hold_thread_ = 0; }
  void assignHold() {
    hold_thread_ = 0;
    hold_thread_ = CurrentThread::tid();
  }
};
class MutexLockGuard : noncopyable {
 public:
  //创建时加锁
  explicit MutexLockGuard(MutexLock& mutex) : mutex_(mutex) { mutex_.lock(); }
  //析构时解锁
  ~MutexLockGuard() { mutex_.unlock(); }

 private:
  MutexLock& mutex_;  //在Guard看来MutexLock就是mutex;
};
