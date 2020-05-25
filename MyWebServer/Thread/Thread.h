#pragma once
#include <pthread.h>

#include <functional>

#include "CountDownLatch.h"
#include "noncopyable.h"
class Thread : noncopyable {
 public:
  typedef std::function<void()> ThreadFunc;
  explicit Thread(const ThreadFunc&, const std::string& name = std::string());
  ~Thread();
  void start();  //开启线程
  int join();    // pthread_join()

  bool started() const { return started_; }
  pthread_t pthreadId() const { return pthreadId_; }
  pid_t tid() const { return tid_; }
  const std::string& name() const { return name_; }

 private:
  pthread_t pthreadId_;
  pid_t tid_;
  ThreadFunc func_;   //线程函数
  std::string name_;  //线程名字
  CountDownLatch latch_;

  bool started_;
  bool joined_;
  void setDefalutName();
};