#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "../EventLoop.h"

void threadFunc() {
  printf("threadFunc(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
  EventLoop loop;
  loop.loop();
}
int main() {
  printf("main(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
  //以下是正面测试
  /*
  EventLoop loop;
  Thread thread(threadFunc);
  thread.start();
  loop.loop();
  pthread_exit(NULL);
  */
 //以下是负面测试
 EventLoop loop1;
 EventLoop loop2;
 sleep(5);
 printf("has 2 loop!\n");
 return 0;
}