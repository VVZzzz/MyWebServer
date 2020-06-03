#include "Util.h"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

const int MAX_BUFF = 4096;

//为避免转换次数过多,使用这个全局变量
//其中epoll采用ET触发,而不用LT触发
// On Linux, the constants of poll(2) and epoll(4)
// are expected to be the same.
static_assert(EPOLLIN == POLLIN,        "epoll uses same flag values as poll");
static_assert(EPOLLPRI == POLLPRI,      "epoll uses same flag values as poll");
static_assert(EPOLLOUT == POLLOUT,      "epoll uses same flag values as poll");
static_assert(EPOLLRDHUP == POLLRDHUP,  "epoll uses same flag values as poll");
static_assert(EPOLLERR == POLLERR,      "epoll uses same flag values as poll");
static_assert(EPOLLHUP == POLLHUP,      "epoll uses same flag values as poll");
const int READEVENT = (::getenv("USE_POLL") ? (POLLIN | POLLPRI)
                                            : (EPOLLIN | EPOLLPRI | EPOLLET));
const int WRITEEVENT = (::getenv("USE_POLL") ? (POLLOUT)
                                            : (EPOLLOUT | EPOLLET));
                                    
//epoll监听事件总数,等待事件
const int EVENTSUM = 4096;
const int EPOLLWAIT_TIME = 10000;  //ms

ssize_t readn(int fd, void *buff, size_t n) {
  size_t nleft = n;
  ssize_t nread = 0;
  ssize_t readSum = 0;
  char *ptr = (char *)buff;
  //由于字节流socket可能会发生缓冲区不够情况, 实际收到或发送的数据量小于计划的
  //故需要循环读
  while (nleft > 0) {
    //出各种错误
    if ((nread = read(fd, ptr, nleft)) < 0) {
      if (errno == EINTR)  //被打断,继续读
        nread = 0;
      else if (errno ==
               EAGAIN)  //类似于Win上的EWOULDBLOCK,即non-blocksocket上暂时无数据
        return readSum;
      else
        return -1;
    } else if (nread == 0)
      // EOF
      break;
    readSum += nread;
    nleft -= nread;
    ptr += nread;
  }
  return readSum;
}
ssize_t readn(int fd, std::string &inBuffer, bool &zero) {
  ssize_t nread = 0;
  ssize_t readSum = 0;
  while (true) {
    char buff[MAX_BUFF];
    if ((nread = read(fd, buff, MAX_BUFF)) < 0) {
      if (errno == EINTR)
        continue;
      else if (errno == EAGAIN) {
        return readSum;
      } else {
        perror("read error");
        return -1;
      }
    } else if (nread == 0) {
      // printf("redsum = %d\n", readSum);
      // EOF
      zero = true;
      break;
    }
    // printf("before inBuffer.size() = %d\n", inBuffer.size());
    // printf("nread = %d\n", nread);
    readSum += nread;
    // buff += nread;
    inBuffer += std::string(buff, buff + nread);
    // printf("after inBuffer.size() = %d\n", inBuffer.size());
  }
  return readSum;
}
ssize_t readn(int fd, std::string &inBuffer) {
  ssize_t nread = 0;
  ssize_t readSum = 0;
  while (true) {
    char buff[MAX_BUFF];
    if ((nread = read(fd, buff, MAX_BUFF)) < 0) {
      if (errno == EINTR)
        continue;
      else if (errno == EAGAIN) {
        return readSum;
      } else {
        perror("read error");
        return -1;
      }
    } else if (nread == 0) {
      break;
    }
    readSum += nread;
    inBuffer += std::string(buff, buff + nread);
  }
  return readSum;
}
ssize_t writen(int fd, void *buff, size_t n) {
  size_t nleft = n;
  ssize_t nwritten = 0;
  ssize_t writeSum = 0;
  char *ptr = (char *)buff;
  while (nleft > 0) {
    if ((nwritten = write(fd, ptr, nleft)) <= 0) {
      if (nwritten < 0) {
        if (errno == EINTR) {
          nwritten = 0;
          continue;
        } else if (errno == EAGAIN) {
          return writeSum;
        } else
          return -1;
      }
    }
    writeSum += nwritten;
    nleft -= nwritten;
    ptr += nwritten;
  }
  return writeSum;
}
ssize_t writen(int fd, std::string &sbuff) {
  size_t nleft = sbuff.size();
  ssize_t nwritten = 0;
  ssize_t writeSum = 0;
  const char *ptr = sbuff.c_str();
  while (nleft > 0) {
    if ((nwritten = write(fd, ptr, nleft)) <= 0) {
      if (nwritten < 0) {
        if (errno == EINTR) {
          nwritten = 0;
          continue;
        } else if (errno == EAGAIN)
          break;
        else
          return -1;
      }
    }
    writeSum += nwritten;
    nleft -= nwritten;
    ptr += nwritten;
  }
  //更新sbuff
  if (writeSum == static_cast<int>(sbuff.size()))
    sbuff.clear();
  else
    sbuff = sbuff.substr(writeSum);
  return writeSum;
}

//处理SIGPIPE信号,信号处理程序是SIG_IGN(忽略)
void handle_for_sigpipe() {
  struct sigaction sa;
  memset(&sa, '\0', sizeof(sa));
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0;
  if (sigaction(SIGPIPE, &sa, NULL)) return;
}
int setSocketNonBlocking(int fd) {
  int flag = fcntl(fd, F_GETFL, 0);
  if (flag == -1) return -1;

  flag |= O_NONBLOCK;
  if (fcntl(fd, F_SETFL, flag) == -1) return -1;
  return 0;
}
void setSocketNodelay(int fd) {
  int enable = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&enable, sizeof(enable));
}
void setSocketNoLinger(int fd) {
  struct linger linger_;
  linger_.l_onoff = 1;
  linger_.l_linger = 30;
  setsockopt(fd, SOL_SOCKET, SO_LINGER, (const char *)&linger_,
             sizeof(linger_));
}
void shutDownWR(int fd) {
  shutdown(fd, SHUT_WR);
  // printf("shutdown\n");
}
int socket_bind_listen(int port) {
  // 检查port值，取正确区间范围
  if (port < 0 || port > 65535) return -1;

  // 创建socket(IPv4 + TCP)，返回监听描述符
  int listen_fd = 0;
  if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
  {
    printf("create socket error: %d\n",errno);
    return -1;
  }

  // 消除bind时"Address already in use"错误
  //即端口复用
  int optval = 1;
  if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval,
                 sizeof(optval)) == -1) {
    close(listen_fd);
    return -1;
  }

  // 设置服务器IP和Port，和监听描述副绑定
  struct sockaddr_in server_addr;
  bzero((char *)&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons((unsigned short)port);
  if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    close(listen_fd);
    return -1;
  }

  // 开始监听，最大等待队列长为LISTENQ
  if (listen(listen_fd, 2048) == -1) {
    close(listen_fd);
    return -1;
  }

  // 无效监听描述符
  if (listen_fd == -1) {
    close(listen_fd);
    return -1;
  }
  return listen_fd;
}