#pragma once
/**
 * HTTP报文的解析模块,通过有限状态机实现
 * 参考<<Linux高性能服务器编程>> 8.6节
 */
#include <sys/epoll.h>
#include <unistd.h>

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>

//#include "Timer.h"
template <typename T>
class TimerNode;

class EventLoop;
class Channel;

//解析整体报文的状态:主状态机
//后面的为从状态机
enum ProcessState {
  STATE_PARSE_URI = 1,
  STATE_PARSE_HEADERS,
  STATE_RECV_BODY,
  STATE_ANALYSIS,
  STATE_FINISH
};

//解析URI的状态
enum URIState {
  PARSE_URI_AGAIN = 1,
  PARSE_URI_ERROR,
  PARSE_URI_SUCCESS,
};

//解析头部的状态
enum HeaderState {
  PARSE_HEADER_SUCCESS = 1,
  PARSE_HEADER_AGAIN,
  PARSE_HEADER_ERROR
};

//分析的状态
enum AnalysisState { ANALYSIS_SUCCESS = 1, ANALYSIS_ERROR };

enum ParseState {
  H_START = 0,
  H_KEY,
  H_COLON,
  H_SPACES_AFTER_COLON,
  H_VALUE,
  H_CR,
  H_LF,
  H_END_CR,
  H_END_LF
};

enum ConnectionState { H_CONNECTED = 0, H_DISCONNECTING, H_DISCONNECTED };

// POST GET HEAD方法
enum HttpMethod { METHOD_POST = 1, METHOD_GET, METHOD_HEAD };

// http版本
enum HttpVersion { HTTP_10 = 1, HTTP_11 };

//这个类用来处理文件名后缀<--->content-type
//类似单例模式,多个线程公用一个unordered_map实例
//只留getMime一个接口
class MimeType {
 private:
  static void init();
  static std::unordered_map<std::string, std::string> mime;
  MimeType();
  MimeType(const MimeType &m);

 public:
  static std::string getMime(const std::string &suffix);

 private:
  static pthread_once_t once_control;
};

//这个enable_shared_from_this<T>允许我们在类的定义中
//使用shared_ptr<T>/weak_ptr<T> , 即shared_from_this,weak_from_this
//返回这两个指针
//注意事项: https://blog.csdn.net/u012442719/article/details/55050871
// cpp文档
class HttpData : public std::enable_shared_from_this<HttpData> {
 public:
  HttpData(EventLoop *loop, int connfd);
  //析构函数:由于用shared_ptr管理,只需closefd即可
  ~HttpData() { close(fd_); }
  void reset();
  void seperateTimer();
  void linkTimer(std::shared_ptr<TimerNode<HttpData>> mtimer) {
    timer_ = mtimer;
  }
  void handleClose();
  void newEvent();

  std::shared_ptr<Channel> getChannel() { return channel_; }
  EventLoop *getLoop() { return loop_; }

 private:
  void handleRead();
  void handleWrite();
  void handleConn();
  void handleError(int fd, int err_num, std::string short_msg);
  URIState parseURI();
  HeaderState parseHeaders();
  AnalysisState analysisRequest();

  EventLoop *loop_;                   //处理该data的loop
  std::shared_ptr<Channel> channel_;  //含有的Channel
  int fd_;

  std::string inBuffer_;
  std::string outBuffer_;

  bool error_;
  ConnectionState connectionState_;
  HttpMethod method_;
  HttpVersion HTTPVersion_;
  std::string fileName_;
  std::string path_;
  int nowReadPos_;
  ProcessState state_;
  ParseState hState_;
  bool keepAlive_;

  //用map保存首部中<Accept: xxxxx>这种信息
  std::map<std::string, std::string> headers_;
  //这个timer_默认初始化为nullptr,由linkTimer进行赋值
  std::weak_ptr<TimerNode<HttpData>> timer_;
};