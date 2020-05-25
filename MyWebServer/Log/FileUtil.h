#pragma once
#include <string>

#include "../Thread/noncopyable.h"

// Log系统基础类,将后端缓冲区内容写入磁盘
class AppendFile : noncopyable {
 public:
  explicit AppendFile(std::string filename);
  ~AppendFile();
  void append(const char *logmsg, const size_t len);
  void flush();

 private:
  size_t write(const char *logmsg, size_t len);
  FILE *fp_;
  // 64KB的缓冲区
  char buffer_[64 * 1024];
};