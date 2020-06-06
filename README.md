# MyWebServer
C++11轻量级高性能网络框架
## Introduction
采用c++11编写的Web服务器,解析get,head请求,可处理静态资源的请求. 实现异步日志系统记录服务器状态,支持http长连接及管线化请求.

## Environment
- OS: CentOS 7
- Compiler: g++4.8
## Build
```
$cmake .
$make
```
## Usage
```
./WebServer [-t thread_numbers] [-p port] [-l log_file_path(should begin with '/')]
```

## Technological Points
- 采用C++11标准,使用智能指针RAII机制自动管理对象声明,右值引用提高效率
- 整体架构: 多线程reactor模型+线程池+定时器+双缓冲区异步日志系统,IO复用采用epoll-et方式.
- 多线程: 使用reactor模型(one thread one loop),每个线程最多一个loop
- IO复用: 非阻塞IO,采用EPOLL边沿触发
- 线程池: 在程序开始时创建线程池,避免创建销毁线程带来的开销.
- 任务分发: accept线程(主线程),只接收新客户,并以Round-Robin分发给线程池(IO线程/计算线程),各个IO/计算线程不会发生竞争,锁的争用只发生在accept线程和特定线程(发生在```queueInLoop()```中).
- 定时器: 基于小根堆的定时器,可以关闭长时间未活动的连接请求.
- accept线程与线程池中的线程通过eventfd进行异步唤醒,已实现线程之间的通信.
- 简单的异步日志系统: 单独开启一个logging线程,将缓冲区收集到的内容写入到日志文件中,采用双缓冲区技术.

## Model
多线程reactor模型+线程池+定时器+双缓冲区异步日志系统,IO复用采用epoll-et方式.
![model]("./pic/model.png")
## Performance Test
采用WebBench进行长连接与短链接的压力测试,详细报告点[这里]()
