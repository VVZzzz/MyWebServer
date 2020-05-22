//保存当前线程的标识信息
#pragma once
namespace CurrentThread {
extern __thread int t_cachedTid;
extern __thread char t_tidString[32];
extern __thread int t_tidStringLength;
extern __thread const char* t_threadName;
void cacheTid();
bool isMainThread();

inline int tid() {
  //只有在线程初始化时调用系统gettid函数
  if (t_cachedTid == 0) cacheTid();
  return t_cachedTid;
}
inline const char* tidString()  // log用
{
  return t_tidString;
}

inline int tidStringLength()  // log用
{
  return t_tidStringLength;
}

inline const char* name() { return t_threadName; }
}  // namespace CurrentThread