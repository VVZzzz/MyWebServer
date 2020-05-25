//不可拷贝类,使用delete关键词即可
#pragma once
class noncopyable {
 public:
  noncopyable(const noncopyable&) = delete;
  void operator=(const noncopyable&) = delete;

 protected:
  noncopyable(/* args */) = default;
  ~noncopyable() = default;
};
