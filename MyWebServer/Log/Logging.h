#pragma once
#include <pthread.h>
#include <stdio.h>
#include <string>
#include <string.h>


#include "LogStream.h"

//该类是为了留出外部接口,即LOG
//且LOG会生成一个Logger对象,生成时在log信息头部添加
//当前时间信息,销毁时添加后续内容: "-- basename : line\n"
class Logger {
 public:
  Logger(const char *fileName, int line);
  ~Logger();
  LogStream &stream() { impl_.stream_; }

  //静态成员函数
  static void setLogFileName(std::string fileName) { logFileName_ = fileName; }
  static std::string getLogFileName() { return logFileName_; }

 private:
  class Impl {
   public:
    Impl(const char *fileName, int line);
    //将"当前时间"输入到stream_流(内在的buffer)中
    void formatTime();

    LogStream stream_;
    int line_;
    std::string basename_;
  };
  Impl impl_;
  //有可能定义多个Logger对象,公用一个logFileName
  static std::string logFileName_;
};

//外部接口
#define LOG Logger(__FILE__, __LINE__).stream()