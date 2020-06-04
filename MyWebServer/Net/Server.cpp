#include "Server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <functional>

#include "../Log/Logging.h"
#include "Util.h"
#include "HttpData.h"

Server::Server(EventLoop *loop, int threadNum, int port)
    : loop_(loop),
      threadNum_(threadNum),
      eventLoopThreadPool_(new EventLoopThreadPool(loop_, threadNum)),
      started_(false),
      acceptChannel_(new Channel(loop_)),
      port_(port),
      //注意,listensocket,bind,listen在初始化这里操作完成
      //这里面要注意listen(fd,SO_MAXCONN)中第二个参数的含义
      //<<UNP>>4.5
      listenFd_(socket_bind_listen(port_)) {
  acceptChannel_->setFd(listenFd_);
  //将默认的SIGPIPE信号处理改为忽略
  handle_for_sigpipe();
  // listen监听,置为非阻塞
  if (setSocketNonBlocking(listenFd_) < 0) {
    perror("set socket non block failed");
    abort();
  }
}

void Server::start() {
  //开启线程池(不包括acceptChannel这个loop)
  eventLoopThreadPool_->start();
  acceptChannel_->setEvents(READEVENT);
  //接收到新的可读事件进行处理,其实一直卡在
  acceptChannel_->setReadHandler(std::bind(&Server::handleNewConn, this));
  //对于当前这个accept channel,如果监听的events变化了,就update
  acceptChannel_->setConnHandler(std::bind(&Server::handleThisConn, this));
  loop_->addToPoller(acceptChannel_,0);
  started_ = true;
}

void Server::handleNewConn() {
  // accept的固定格式
  struct sockaddr_in client_addr;
  memset(&client_addr, 0, sizeof(struct sockaddr_in));
  socklen_t client_addr_len = sizeof(client_addr);
  int accept_fd = 0;
  //注意此处,accept的调用是非阻塞的,为何accept要设为非阻塞的
  //<<UNP>> 16.6
  //一般正常情况下由于accept是非阻塞,没有新连接时
  //会返回EWOULDBLOCK等(表示没有新客户到来),跳出该while
  //然后返回到epoll_wait那里阻塞等待
  while ((accept_fd = accept(listenFd_, (struct sockaddr *)&client_addr,
                             &client_addr_len)) > 0) {
    //Round robin选择下一个线程(这使得每个线程负载更加均衡)
    //注意:不是说一个线程只能处理一个客户,一个线程用poll可等待多个事件
    //IO复用
    EventLoop *loop = eventLoopThreadPool_->getNextLoop();
    LOG << "New connection from " << inet_ntoa(client_addr.sin_addr) << ":"
        << ntohs(client_addr.sin_port);
    /*
// TCP的保活机制默认是关闭的
int optval = 0;
socklen_t len_optval = 4;
getsockopt(accept_fd, SOL_SOCKET,  SO_KEEPALIVE, &optval, &len_optval);
cout << "optval ==" << optval << endl;
*/
    // 限制服务器的最大并发连接数
    if (accept_fd >= MAXFDS) {
      close(accept_fd);
      continue;
    }
    // 设为非阻塞模式
    if (setSocketNonBlocking(accept_fd) < 0) {
      LOG << "Set non block failed!";
      // perror("Set non block failed!");
      return;
    }

    //禁用Nagle算法:Nagle算法<<UNP>>7.9
    setSocketNodelay(accept_fd);
    //Linger控制close具体操作方式
    //setSocketNoLinger(accept_fd);

    std::shared_ptr<HttpData> req_info(new HttpData(loop,accept_fd));
    req_info->getChannel()->setHolder(req_info);
    //Server主线程将接收到的新客户newEvent插入到子线程的loop中,使得子线程开始poll这个newEvent
    //即打断这个子线程的loop,插入新事件到doPendingFunctors
    //此处不用runInLoop了,因为明显Server线程不是子IO线程
    loop->queueInLoop(std::bind(&HttpData::newEvent,req_info));
  }
  //如果运行到此处,说明没有新的连接或者出错,重新设置
  acceptChannel_->setEvents(EPOLLIN | EPOLLET);
}