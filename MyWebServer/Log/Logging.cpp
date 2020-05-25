#include "Logging.h"

#include <sys/time.h>
#include <time.h>

#include <iostream>

#include "../Thread/CurrentThread.h"
#include "../Thread/Thread.h"
#include "AsyncLogging.h"

// pthread_once_t必须是非局部static变量,且初始化
//用它来保证只执行一次once_init(众多线程中只执行一次)
static pthread_once_t once_control_ = PTHREAD_ONCE_INIT;
static AsyncLogging *AsyncLogger_;

void once_init() {
  AsyncLogger_ = new AsyncLogging(Logger::getLogFileName());
  //后端线程开始(后端线程只有一个)
  AsyncLogger_->start();
}

void output(const char *msg, int len) {
  pthread_once(&once_control_, once_init);
  //前端的线程(多个,都调用append)
  AsyncLogger_->append(msg, len);
}

std::string Logger::logFileName_ = "./WebServer.log";

Logger::Impl::Impl(const char *fileName, int line)
    : stream_(), line_(line), basename_(fileName) {
  formatTime();
}

void Logger::Impl::formatTime() {
  struct timeval tv;
  time_t time;
  char str_t[26] = {0};
  gettimeofday(&tv, NULL);
  time = tv.tv_sec;
  struct tm *p_time = localtime(&time);
  strftime(str_t, 26, "%Y-%m-%d %H:%M:%S\n", p_time);
  stream_ << str_t;
}

Logger::Logger(const char *fileName, int line) : impl_(fileName, line) {}

Logger::~Logger() {
  impl_.stream_ << " -- " << impl_.basename_ << ":" << impl_.line_ << '\n';
  const LogStream::Buffer &buf(stream().buffer());
  output(buf.data(), buf.length());
}