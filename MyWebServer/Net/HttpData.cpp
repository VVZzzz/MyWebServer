#include "HttpData.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <iostream>

#include "Channel.h"
#include "EventLoop.h"
#include "Timer.h"
#include "Util.h"

HttpData::HttpData(EventLoop *loop, int connfd)
    : loop_(loop),
      channel_(new Channel(loop, connfd)),
      fd_(connfd),
      error_(false),
      connectionState_(H_CONNECTED),
      method_(METHOD_GET),
      HTTPVersion_(HTTP_11),
      nowReadPos_(0),
      state_(STATE_PARSE_URI),
      hState_(H_START),
      keepAlive_(false) {
  // loop_->queueInLoop(bind(&HttpData::setHandlers, this));
  //设置channel处理事件函数
  channel_->setReadHandler(bind(&HttpData::handleRead, this));
  channel_->setWriteHandler(bind(&HttpData::handleWrite, this));
  channel_->setConnHandler(bind(&HttpData::handleConn, this));
}

//重置http连接状态
void HttpData::reset() {
  // inBuffer_.clear();
  fileName_.clear();
  path_.clear();
  nowReadPos_ = 0;
  state_ = STATE_PARSE_URI;
  hState_ = H_START;
  headers_.clear();
  // keepAlive_ = false;
  //解绑定时器节点,并删除这个节点
  if (timer_.lock()) {
    std::shared_ptr<TimerNode<HttpData>> my_timer(timer_.lock());
    my_timer->clearReq();
    timer_.reset();
  }
}

//解绑定时器节点,并删除这个节点
void HttpData::seperateTimer() {
  if (timer_.lock()) {
    std::shared_ptr<TimerNode<HttpData>> my_timer(timer_.lock());
    my_timer->clearReq();
    timer_.reset();
  }
}

// Http连接的核心:处理新到来的http连接
void HttpData::handleRead() {
  __uint32_t &events = channel_->getEvents();
  //以下用do{...break...} while(false)结构
  //这样做的好处是,break可以跳过后面不想执行的语句
  do {
    bool zero = false;
    int read_num = readn(fd_, inBuffer_, zero);
    LOG << "Request: " << inBuffer_;
    if (connectionState_ == H_DISCONNECTING) {
      inBuffer_.clear();
      break;
    }
    // cout << inBuffer_ << endl;
    if (read_num < 0) {
      perror("Read request error: ");
      error_ = true;
      handleError(fd_, 400, "Bad Request");
      break;
    }
    //这是read到0个字节且zero为0的情况(即下面):
    // 有请求出现但是读不到数据，可能是Request
    // Aborted，或者来自网络的数据没有达到等原因
    // 最可能是对端已经关闭了，统一按照对端已经关闭处理
    else if (zero) {
      // error_ = true;
      //对端关闭
      connectionState_ = H_DISCONNECTING;
      if (read_num == 0) {
        // error_ = true;
        break;
      }
      // cout << "readnum == 0" << endl;
    }
    //状态机:从PARSE_URI开始
    if (state_ == STATE_PARSE_URI) {
      URIState flag = this->parseURI();
      if (flag == PARSE_URI_AGAIN)
        break;
      else if (flag == PARSE_URI_ERROR) {
        perror("parse uri error:");
        LOG << "FD = " << fd_ << "," << inBuffer_ << "******";
        inBuffer_.clear();
        error_ = true;
        handleError(fd_, 400, "Bad Request");
        break;
      } else
        state_ = STATE_PARSE_HEADERS;
    }

    // PARSE_URI->PARSE_HEADER
    if (state_ == STATE_PARSE_HEADERS) {
      HeaderState flag = this->parseHeaders();
      if (flag == PARSE_HEADER_AGAIN)
        break;
      else if (flag == PARSE_HEADER_ERROR) {
        perror("parse header error:");
        error_ = true;
        handleError(fd_, 400, "Bad Request");
        break;
      }
      if (method_ == METHOD_POST) {
        // POST方法准备
        state_ = STATE_RECV_BODY;
      } else {
        state_ = STATE_ANALYSIS;
      }
    }

    // PARSE_HEADER->RECV_BODY/ANALYSIS
    // used by POST
    if (state_ == STATE_RECV_BODY) {
      int content_length = -1;
      if (headers_.find("Content-length") != headers_.end()) {
        content_length = stoi(headers_["Content-length"]);
      } else {
        // cout << "(state_ == STATE_RECV_BODY)" << endl;
        error_ = true;
        handleError(fd_, 400,
                    "Bad Request: Post Lack of argument (Content-length)");
        break;
      }
      //此时inbuffer内容只有首部
      if (static_cast<int>(inBuffer_.size()) < content_length) break;
      state_ = STATE_ANALYSIS;
    }
    if (state_ == STATE_ANALYSIS) {
      AnalysisState flag = this->analysisRequest();
      if (flag == ANALYSIS_SUCCESS) {
        state_ = STATE_FINISH;
        break;
      } else {
        // cout << "state_ == STATE_ANALYSIS" << endl;
        error_ = true;
        break;
      }
    }
  } while (false);
}

//一:解析请求行
URIState HttpData::parseURI() {
  std::string &str = inBuffer_;
  std::string cop = str;
  // step1:解析请求行-method
  // 读到完整的请求行再开始解析请求
  size_t pos = str.find('\r', nowReadPos_);
  // 去掉请求行所占的空间，节省空间
  std::string request_line = str.substr(0, pos);
  if (str.size() > pos + 1)
    str = str.substr(pos + 1);
  else
    str.clear();
  // Method
  int posGet = request_line.find("GET");
  int posPost = request_line.find("POST");
  int posHead = request_line.find("HEAD");
  if (posGet >= 0) {
    pos = posGet;
    method_ = METHOD_GET;
  } else if (posPost >= 0) {
    pos = posPost;
    method_ = METHOD_POST;
  } else if (posHead >= 0) {
    pos = posHead;
    method_ = METHOD_HEAD;
  } else {
    return PARSE_URI_ERROR;
  }

  // step2:解析filename
  pos = request_line.find("/", pos);
  if (pos < 0) {
    fileName_ = "index.html";
    HTTPVersion_ = HTTP_11;
    return PARSE_URI_SUCCESS;
  } else {
    size_t _pos = request_line.find(' ', pos);
    if (_pos < 0)
      return PARSE_URI_ERROR;
    else {
      if (_pos - pos > 1) {
        fileName_ = request_line.substr(pos + 1, _pos - pos - 1);
        size_t __pos = fileName_.find('?');
        if (__pos >= 0) {
          fileName_ = fileName_.substr(0, __pos);
        }
      } else
        fileName_ = "index.html";
    }
    pos = _pos;
  }
  // cout << "fileName_: " << fileName_ << endl;

  // step3: HTTP版本号
  pos = request_line.find("/", pos);
  if (pos < 0)
    return PARSE_URI_ERROR;
  else {
    if (request_line.size() - pos <= 3)
      return PARSE_URI_ERROR;
    else {
      std::string ver = request_line.substr(pos + 1, 3);
      if (ver == "1.0")
        HTTPVersion_ = HTTP_10;
      else if (ver == "1.1")
        HTTPVersion_ = HTTP_11;
      else
        return PARSE_URI_ERROR;
    }
  }
  return PARSE_URI_SUCCESS;
}

//二 解析首部
//此时inBuffer_中已经没有请求行了
/**
 * 比如:
 * Host: hackr.jp
 * Accept: text/html
 * \r\n
 *
 * Host为H_KEY,:为H_COLON,' '为H_SPACES_AFTER_COLON
 * hackr.jp为H_VALUE,后面的'\n'为H_CR,'\r'为H_LF
 * 最后的\r\n为H_END_CR和H_END_LF
 *
 */
HeaderState HttpData::parseHeaders() {
  std::string &str = inBuffer_;
  int key_start = -1, key_end = -1, value_start = -1, value_end = -1;
  int now_read_line_begin = 0;
  bool notFinish = true;
  size_t i = 0;
  //初始解析头部状态为HSTART
  for (; i < str.size() && notFinish; ++i) {
    switch (hState_) {
      case H_START: {
        if (str[i] == '\n' || str[i] == '\r') break;
        hState_ = H_KEY;
        key_start = i;
        now_read_line_begin = i;
        break;
      }
      case H_KEY: {
        if (str[i] == ':') {
          key_end = i;
          if (key_end - key_start <= 0) return PARSE_HEADER_ERROR;
          hState_ = H_COLON;
        } else if (str[i] == '\n' || str[i] == '\r')
          return PARSE_HEADER_ERROR;
        break;
      }
      case H_COLON: {
        if (str[i] == ' ') {
          hState_ = H_SPACES_AFTER_COLON;
        } else
          return PARSE_HEADER_ERROR;
        break;
      }
      case H_SPACES_AFTER_COLON: {
        hState_ = H_VALUE;
        value_start = i;
        break;
      }
      case H_VALUE: {
        if (str[i] == '\r') {
          hState_ = H_CR;
          value_end = i;
          if (value_end - value_start <= 0) return PARSE_HEADER_ERROR;
        } else if (i - value_start > 255)
          return PARSE_HEADER_ERROR;
        break;
      }
      case H_CR: {
        if (str[i] == '\n') {
          hState_ = H_LF;
          std::string key(str.begin() + key_start, str.begin() + key_end);
          std::string value(str.begin() + value_start, str.begin() + value_end);
          headers_[key] = value;
          now_read_line_begin = i;
        } else
          return PARSE_HEADER_ERROR;
        break;
      }
      case H_LF: {
        if (str[i] == '\r') {
          hState_ = H_END_CR;
        } else {
          key_start = i;
          hState_ = H_KEY;
        }
        break;
      }
      case H_END_CR: {
        if (str[i] == '\n') {
          hState_ = H_END_LF;
        } else
          return PARSE_HEADER_ERROR;
        break;
      }
      case H_END_LF: {
        notFinish = false;
        key_start = i;
        now_read_line_begin = i;
        break;
      }
    }
  }
  if (hState_ == H_END_LF) {
    str = str.substr(i);
    return PARSE_HEADER_SUCCESS;
  }
  str = str.substr(now_read_line_begin);
  return PARSE_HEADER_AGAIN;
}

//三 分析整个请求报文(ANALYSIS)
AnalysisState

//处理请求request出错,在界面显示
void HttpData::handleError(int fd, int err_num, std::string short_msg) {
  //前端页面显示(响应报文body)
  short_msg = " " + short_msg;
  char send_buff[4096];
  std::string body_buff, header_buff;
  body_buff += "<html><title>请求出错</title>";
  body_buff += "<body bgcolor=\"ffffff\">";
  body_buff += std::to_string(err_num) + short_msg;
  body_buff += "<hr><em> WRH's Web Server</em>\n</body></html>";

  //构造响应报文-响应行+首部
  header_buff += "HTTP/1.1 " + std::to_string(err_num) + short_msg + "\r\n";
  header_buff += "Content-Type: text/html\r\n";
  header_buff += "Connection: Close\r\n";
  header_buff += "Content-Length: " + std::to_string(body_buff.size()) + "\r\n";
  header_buff += "Server: WRH's Web Server\r\n";
  ;
  header_buff += "\r\n";
  // 错误处理不考虑writen不完的情况
  //将响应报文发给客户
  sprintf(send_buff, "%s", header_buff.c_str());
  writen(fd, send_buff, strlen(send_buff));
  sprintf(send_buff, "%s", body_buff.c_str());
  writen(fd, send_buff, strlen(send_buff));
}