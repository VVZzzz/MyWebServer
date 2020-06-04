#include <unistd.h>

#include <iostream>
#include <string>

#include "Log/Logging.h"
#include "Net/EventLoop.h"
#include "Net/Server.h"

int main(int argc, char **argv) {
  //默认开启线程数和端口
  int threadNum = 4;
  int port = 80;
  std::string logPath = "./server.log";
  //解析命令行
  //-p port -t threadNum -l logpath

  int opt;
  const char *str = "t:l:p:";
  while ((opt = getopt(argc, argv, str)) != -1) {
    switch (opt) {
      case 't': {
        threadNum = atoi(optarg);
        break;
      }
      case 'l': {
        logPath = optarg;
        if (logPath.size() < 2 || optarg[0] != '/') {
          printf("logPath should start with \"/\"\n");
          abort();
        }
        break;
      }
      case 'p': {
        port = atoi(optarg);
        break;
      }
      default:
        break;
    }
  }

  Logger::setLogFileName(logPath);
  // STL库在多线程上应用
#ifndef _PTHREADS
  LOG << "_PTHREADS is not defined !";
#endif
  //开启服务器
  EventLoop mainloop;
  Server httpServer(&mainloop, threadNum, port);
  httpServer.start();
  mainloop.loop();
  return 0;
}