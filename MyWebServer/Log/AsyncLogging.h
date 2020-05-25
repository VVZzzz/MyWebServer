#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../Thread/CountDownLatch.h"
#include "../Thread/Mutex.h"
#include "../Thread/Thread.h"
#include "../Thread/noncopyable.h"
#include "LogStream.h"

class AsyncLogging : noncopyable {
 public:
  AsyncLogging(const std::string basename, int flush = 2);
  ~AsyncLogging() {
    if (running_) stop();
  }

  void append(const char *logline, int len);

  void start() {
    running_ = true;
    thread_.start();
    latch_.wait();  //只有thread_真正start , wait才返回
  }
  void stop() {
    running_ = false;
    cond_.notify();  // thread结束前, 再次通知后端线程
    thread_.join();
  }

 private:
  // 4000*1000Byte的缓冲区
  typedef FixedBuffer<kLargeBuffer> Buffer;
  typedef std::vector<std::shared_ptr<Buffer>> BufferVector;
  typedef std::shared_ptr<Buffer> BufferPtr;

  const int flushInterval_;
  bool running_;
  std::string basename_;
  BufferPtr currentBuffer_;  //当前缓冲
  BufferPtr nextBuffer_;     //预备缓冲
  BufferVector buffers_;     //待写入文件的已填满的缓冲

  void threadFunc();
  Thread thread_;
  MutexLock mutex_;
  Condition cond_;        //用来唤醒后端线程
  CountDownLatch latch_;  //用来保证后端线程真正启动起来
};