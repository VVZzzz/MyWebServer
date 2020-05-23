#pragma once
#include <string>
#include <string.h>

#include "Thread/noncopyable.h"

const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000 * 1000;

//非类型模板参数
template <int SIZE>
class FixedBuffer : noncopyable {
 public:
  // FixedBuffer() :
  FixedBuffer() : cur_(data_) {}
  ~FixedBuffer() {}

  // FixedBiffer的操作
  void append(const char* buf, size_t len) {
    if (avail() > static_cast<int>(len)) {
      memcpy(cur_, buf, len);
      cur_ += len;
    }
  }

  const char* data() const { return data_; }
  int length() const { return static_cast<int>(cur_ - data_); }
  char* current() { return cur_; }
  int avail() const { return static_cast<int>(end() - cur_); }

  void add(size_t len) { cur_ += len; }
  void reset() { cur_ = data_; }
  void bzero() { memset(data_, 0, sizeof data_); }

 private:
  const char* end() const { return data_ + sizeof data_; }
  char data_[SIZE];
  char* cur_;
};

//此类主要用来<<操作,底层也有一块缓冲区,主要是
//将多个<<的内容都放在这个缓冲区中
class LogStream : noncopyable {
 public:
  typedef FixedBuffer<kSmallBuffer> Buffer;

  //重载一系列<<操作符
  //bool变量输出为1 0
  LogStream& operator<<(bool v) {
    buffer_.append(v ? "1" : "0", 1);
    return *this;
  }

  LogStream& operator<<(short);
  LogStream& operator<<(unsigned short);
  LogStream& operator<<(int);
  LogStream& operator<<(unsigned int);
  LogStream& operator<<(long);
  LogStream& operator<<(unsigned long);
  LogStream& operator<<(long long);
  LogStream& operator<<(unsigned long long);

  LogStream& operator<<(const void*);

  LogStream& operator<<(float v) {
    *this << static_cast<double>(v);
    return *this;
  }
  LogStream& operator<<(double);
  LogStream& operator<<(long double);

  LogStream& operator<<(char v) {
    buffer_.append(&v, 1);
    return *this;
  }

  LogStream& operator<<(const char* str) {
    if (str)
      buffer_.append(str, strlen(str));
    else
      buffer_.append("(null)", 6);
    return *this;
  }

  LogStream& operator<<(const unsigned char* str) {
      //注意此处使用reinterpret_cast
    return operator<<(reinterpret_cast<const char*>(str));
  }

  LogStream& operator<<(const std::string& v) {
    buffer_.append(v.c_str(), v.size());
    return *this;
  }

  //针对内层的buffer的一些操作
  void append(const char* data, int len) { buffer_.append(data, len); }
  const Buffer& buffer() const { return buffer_; }
  void resetBuffer() { buffer_.reset(); }

 private:
  void staticCheck();

  //函数模板
  template <typename T>
  void formatInteger(T);

  // FixedBuffer
  Buffer buffer_;
  static const int kMaxNumericSize = 32;
};