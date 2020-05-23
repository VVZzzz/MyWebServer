#include "Logging.h"

#include <sys/time.h>
#include <time.h>

#include <iostream>

#include "Thread/CurrentThread.h"
#include "Thread/Thread.h"

//pthread_once_t必须是非局部static变量,且初始化
//用它来保证只有当前线程执行once_init
static pthread_once_t once_control_ = PTHREAD_ONCE_INIT;
//static AsyncLogging *AsyncLogger_;

void once_init(){

}

void output(const char *msg,int len){
    pthread_once(&once_control_,once_init);
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

Logger::~Logger()
{
    impl_.stream_<<" -- "<<impl_.basename_<<":"<<impl_.line_<<'\n';
    const LogStream::Buffer &buf(stream().buffer());
    //output(buf.data(), buf.length());
}