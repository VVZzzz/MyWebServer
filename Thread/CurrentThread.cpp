#include "Thread/CurrentThread.h"

namespace CurrentThread {
//使用__thread变量
//__thread是GCC内置的线程局部存储设施。_thread变量每一个线程有一份独立实体，各个线程的值互不干扰。可以用来修饰那些带有全局性且值可能变，但是又不值得用全局变量保护的变量。
//https://www.jianshu.com/p/997b533842c8
__thread int t_cachedTid = 0;  //缓存当前线程id
__thread char t_tidString[32];
__thread int t_tidStringLength = 6;
__thread const char* t_threadName = "unknown";

}  // namespace CurrentThread