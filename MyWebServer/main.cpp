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
  std::string logpath = "./server.log";
  //解析命令行
  //-p port -t threadNum -l logpath
  int opt;
  const char *optstr = "ptl";
  while ((opt = getopt(argc, argv, optstr)) != -1) {
    switch (opt) {
      case 'p':
        port = atoi(optarg);
        break;
      case 't':
        threadNum = atoi(optarg);
        break;
      case 'l':
        logpath = optarg;
        if (logpath.size() < 2 || optarg[0] != '/') {
          std::cout << "logpath starts with \"/\"\n";
          exit(0);
        }
        break;
      default:
        std::cout << "usage: [-p] port [-t] thread [-l] logdir" << std::endl;
        break;
    }
  }
  Logger::setLogFileName(logpath);
  // STL库在多线程上应用
#ifndef _PTHREADS
  LOG << "_PTHREADS is not defined !";
#endif
  //开启服务器
  EventLoop mainloop;
  Server httpServer(&mainloop,threadNum,port);
  httpServer.start();
  mainloop.loop();
  return 0;

}