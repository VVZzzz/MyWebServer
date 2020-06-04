#include "HttpData.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <iostream>

#include "Channel.h"
#include "EventLoop.h"
#include "Timer.h"
#include "Util.h"

using namespace std;

// MimeType static成员变量初始化
pthread_once_t MimeType::once_control = PTHREAD_ONCE_INIT;
std::unordered_map<std::string, std::string> MimeType::mime;

// const __uint32_t DEFAULT_EVENT = EPOLLIN | EPOLLET | EPOLLONESHOT;
const int DEFAULT_EXPIRED_TIME = 2000;              // ms
const int DEFAULT_KEEP_ALIVE_TIME = 5 * 60 * 1000;  // ms
char favicon[555] = {
    '\x89', 'P',    'N',    'G',    '\xD',  '\xA',  '\x1A', '\xA',  '\x0',
    '\x0',  '\x0',  '\xD',  'I',    'H',    'D',    'R',    '\x0',  '\x0',
    '\x0',  '\x10', '\x0',  '\x0',  '\x0',  '\x10', '\x8',  '\x6',  '\x0',
    '\x0',  '\x0',  '\x1F', '\xF3', '\xFF', 'a',    '\x0',  '\x0',  '\x0',
    '\x19', 't',    'E',    'X',    't',    'S',    'o',    'f',    't',
    'w',    'a',    'r',    'e',    '\x0',  'A',    'd',    'o',    'b',
    'e',    '\x20', 'I',    'm',    'a',    'g',    'e',    'R',    'e',
    'a',    'd',    'y',    'q',    '\xC9', 'e',    '\x3C', '\x0',  '\x0',
    '\x1',  '\xCD', 'I',    'D',    'A',    'T',    'x',    '\xDA', '\x94',
    '\x93', '9',    'H',    '\x3',  'A',    '\x14', '\x86', '\xFF', '\x5D',
    'b',    '\xA7', '\x4',  'R',    '\xC4', 'm',    '\x22', '\x1E', '\xA0',
    'F',    '\x24', '\x8',  '\x16', '\x16', 'v',    '\xA',  '6',    '\xBA',
    'J',    '\x9A', '\x80', '\x8',  'A',    '\xB4', 'q',    '\x85', 'X',
    '\x89', 'G',    '\xB0', 'I',    '\xA9', 'Q',    '\x24', '\xCD', '\xA6',
    '\x8',  '\xA4', 'H',    'c',    '\x91', 'B',    '\xB',  '\xAF', 'V',
    '\xC1', 'F',    '\xB4', '\x15', '\xCF', '\x22', 'X',    '\x98', '\xB',
    'T',    'H',    '\x8A', 'd',    '\x93', '\x8D', '\xFB', 'F',    'g',
    '\xC9', '\x1A', '\x14', '\x7D', '\xF0', 'f',    'v',    'f',    '\xDF',
    '\x7C', '\xEF', '\xE7', 'g',    'F',    '\xA8', '\xD5', 'j',    'H',
    '\x24', '\x12', '\x2A', '\x0',  '\x5',  '\xBF', 'G',    '\xD4', '\xEF',
    '\xF7', '\x2F', '6',    '\xEC', '\x12', '\x20', '\x1E', '\x8F', '\xD7',
    '\xAA', '\xD5', '\xEA', '\xAF', 'I',    '5',    'F',    '\xAA', 'T',
    '\x5F', '\x9F', '\x22', 'A',    '\x2A', '\x95', '\xA',  '\x83', '\xE5',
    'r',    '9',    'd',    '\xB3', 'Y',    '\x96', '\x99', 'L',    '\x6',
    '\xE9', 't',    '\x9A', '\x25', '\x85', '\x2C', '\xCB', 'T',    '\xA7',
    '\xC4', 'b',    '1',    '\xB5', '\x5E', '\x0',  '\x3',  'h',    '\x9A',
    '\xC6', '\x16', '\x82', '\x20', 'X',    'R',    '\x14', 'E',    '6',
    'S',    '\x94', '\xCB', 'e',    'x',    '\xBD', '\x5E', '\xAA', 'U',
    'T',    '\x23', 'L',    '\xC0', '\xE0', '\xE2', '\xC1', '\x8F', '\x0',
    '\x9E', '\xBC', '\x9',  'A',    '\x7C', '\x3E', '\x1F', '\x83', 'D',
    '\x22', '\x11', '\xD5', 'T',    '\x40', '\x3F', '8',    '\x80', 'w',
    '\xE5', '3',    '\x7',  '\xB8', '\x5C', '\x2E', 'H',    '\x92', '\x4',
    '\x87', '\xC3', '\x81', '\x40', '\x20', '\x40', 'g',    '\x98', '\xE9',
    '6',    '\x1A', '\xA6', 'g',    '\x15', '\x4',  '\xE3', '\xD7', '\xC8',
    '\xBD', '\x15', '\xE1', 'i',    '\xB7', 'C',    '\xAB', '\xEA', 'x',
    '\x2F', 'j',    'X',    '\x92', '\xBB', '\x18', '\x20', '\x9F', '\xCF',
    '3',    '\xC3', '\xB8', '\xE9', 'N',    '\xA7', '\xD3', 'l',    'J',
    '\x0',  'i',    '6',    '\x7C', '\x8E', '\xE1', '\xFE', 'V',    '\x84',
    '\xE7', '\x3C', '\x9F', 'r',    '\x2B', '\x3A', 'B',    '\x7B', '7',
    'f',    'w',    '\xAE', '\x8E', '\xE',  '\xF3', '\xBD', 'R',    '\xA9',
    'd',    '\x2',  'B',    '\xAF', '\x85', '2',    'f',    'F',    '\xBA',
    '\xC',  '\xD9', '\x9F', '\x1D', '\x9A', 'l',    '\x22', '\xE6', '\xC7',
    '\x3A', '\x2C', '\x80', '\xEF', '\xC1', '\x15', '\x90', '\x7',  '\x93',
    '\xA2', '\x28', '\xA0', 'S',    'j',    '\xB1', '\xB8', '\xDF', '\x29',
    '5',    'C',    '\xE',  '\x3F', 'X',    '\xFC', '\x98', '\xDA', 'y',
    'j',    'P',    '\x40', '\x0',  '\x87', '\xAE', '\x1B', '\x17', 'B',
    '\xB4', '\x3A', '\x3F', '\xBE', 'y',    '\xC7', '\xA',  '\x26', '\xB6',
    '\xEE', '\xD9', '\x9A', '\x60', '\x14', '\x93', '\xDB', '\x8F', '\xD',
    '\xA',  '\x2E', '\xE9', '\x23', '\x95', '\x29', 'X',    '\x0',  '\x27',
    '\xEB', 'n',    'V',    'p',    '\xBC', '\xD6', '\xCB', '\xD6', 'G',
    '\xAB', '\x3D', 'l',    '\x7D', '\xB8', '\xD2', '\xDD', '\xA0', '\x60',
    '\x83', '\xBA', '\xEF', '\x5F', '\xA4', '\xEA', '\xCC', '\x2',  'N',
    '\xAE', '\x5E', 'p',    '\x1A', '\xEC', '\xB3', '\x40', '9',    '\xAC',
    '\xFE', '\xF2', '\x91', '\x89', 'g',    '\x91', '\x85', '\x21', '\xA8',
    '\x87', '\xB7', 'X',    '\x7E', '\x7E', '\x85', '\xBB', '\xCD', 'N',
    'N',    'b',    't',    '\x40', '\xFA', '\x93', '\x89', '\xEC', '\x1E',
    '\xEC', '\x86', '\x2',  'H',    '\x26', '\x93', '\xD0', 'u',    '\x1D',
    '\x7F', '\x9',  '2',    '\x95', '\xBF', '\x1F', '\xDB', '\xD7', 'c',
    '\x8A', '\x1A', '\xF7', '\x5C', '\xC1', '\xFF', '\x22', 'J',    '\xC3',
    '\x87', '\x0',  '\x3',  '\x0',  'K',    '\xBB', '\xF8', '\xD6', '\x2A',
    'v',    '\x98', 'I',    '\x0',  '\x0',  '\x0',  '\x0',  'I',    'E',
    'N',    'D',    '\xAE', 'B',    '\x60', '\x82',
};

void MimeType::init() {
  mime[".html"] = "text/html";
  mime[".avi"] = "video/x-msvideo";
  mime[".bmp"] = "image/bmp";
  mime[".c"] = "text/plain";
  mime[".doc"] = "application/msword";
  mime[".gif"] = "image/gif";
  mime[".gz"] = "application/x-gzip";
  mime[".htm"] = "text/html";
  mime[".ico"] = "image/x-icon";
  mime[".jpg"] = "image/jpeg";
  mime[".png"] = "image/png";
  mime[".txt"] = "text/plain";
  mime[".mp3"] = "audio/mp3";
  mime["default"] = "text/html";
}
std::string MimeType::getMime(const std::string &suffix) {
  //只有一个线程可以执行init
  pthread_once(&once_control, MimeType::init);
  if (mime.find(suffix) == mime.end())
    return mime["default"];
  else
    return mime[suffix];
}

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
  //解绑定时器节点(真正的删除在timeManager中的handleExpired中)
  if (timer_.lock()) {
    std::shared_ptr<TimerNode<HttpData>> my_timer(timer_.lock());
    my_timer->clearReq();
    timer_.reset();
  }
}

//解绑定时器节点(真正的删除在timeManager中的handleExpired中)
void HttpData::seperateTimer() {
  if (timer_.lock()) {
    std::shared_ptr<TimerNode<HttpData>> my_timer(timer_.lock());
    my_timer->clearReq();
    timer_.reset();
  }
}

/*******************Http连接的事件处理函数*****************/
// Http连接的核心:处理新到来的http连接
void HttpData::handleRead() {
  __uint32_t &events_ = channel_->getEvents();
  //以下用do{...break...} while(false)结构
  //这样做的好处是,break可以跳过后面不想执行的语句
  do {
    bool zero = false;
    int read_num = readn(fd_, inBuffer_, zero);
    //LOG << "Request: " << inBuffer_;
    if (connectionState_ == H_DISCONNECTING) {
      inBuffer_.clear();
      break;
    }
    // cout << inBuffer_ << endl;
    if (read_num < 0) {
      //perror("Read request error: ");
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
        //perror("parse uri error:");
        //LOG << "FD = " << fd_ << "," << inBuffer_ << "******";
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
        //perror("parse header error:");
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
    //此时inBuffer_中已经没有请求行+首部了

    //这里注意:如果是GET方法的话,其请求报文中的body没有,即直接请求行+首部+\r\n
    // POST方法请求报文为:请求行+首部+主体
    //所以POST需要先RECV_BODY再ANALYSIS

    // PARSE_HEADER->RECV_BODY(used by POST)/ANALYSIS
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
      //下面这个if是说body部分没有一次性读完,需要再次注册可读事件读入
      if (static_cast<int>(inBuffer_.size()) < content_length) break;
      state_ = STATE_ANALYSIS;
    }
    // ANALYSIS->FINISH
    //经过analysisRequest之后,outBuffer_中有数据
    //且inBuffer_保存的body会被清空(处理分析)
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
  //cout << "state_=" << state_ << endl;
  //至此已经分析了请求报文,并得到要相应的要响应
  //的报文内容,下面要发送响应报文了
  if (!error_) {
    if (outBuffer_.size() > 0) {
      handleWrite();
    }
    // error_ may change(in handleWrite)
    if (!error_ && state_ == STATE_FINISH) {
      //注意这个reset,一旦这个http完成了接收并给予回应
      // reset中就解绑这个定时器
      this->reset();
      //一般情况下,state_为FINISH时,inbuffer为空
      //不为空,那就再次handleRead
      if (inBuffer_.size() > 0) {
        if (connectionState_ != H_DISCONNECTING) handleRead();
      }

      // if ((keepAlive_ || inBuffer_.size() > 0) && connectionState_ ==
      // H_CONNECTED)
      // {
      //     this->reset();
      //     events_ |= EPOLLIN;
      // }
    } else if (!error_ && connectionState_ != H_DISCONNECTED) {
      //对于不处于STATE_FINISH来讲()(一般是对于POST,无法一次性读完body)
      //再次注册读事件
      events_ |= EPOLLIN;
    }
  }
}

void HttpData::handleWrite() {
  if (!error_ && connectionState_ != H_DISCONNECTED) {
    __uint32_t &events_ = channel_->getEvents();
    if (writen(fd_, outBuffer_) < 0) {
      //perror("writen");
      events_ = 0;
      error_ = true;
    }
    //尽管在writen中是循环写,但由于可能缓冲区不足等,一次就是发不完
    //这样的话再次注册写事件
    if (outBuffer_.size() > 0) events_ |= EPOLLOUT;
  }
}

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

// handleConn是在handleRead和handleWrite完毕之后
//再执行的
void HttpData::handleConn() {
  //解绑定时器节点(真正的删除在timeManager中的handleExpired中)
  seperateTimer();
  __uint32_t &events_ = channel_->getEvents();
  if (!error_ && connectionState_ == H_CONNECTED) {
    if (events_ != 0) {
      int timeout = DEFAULT_EXPIRED_TIME;
      if (keepAlive_) timeout = DEFAULT_KEEP_ALIVE_TIME;
      //同时设置读事件和写事件时(见http的handleRead和handleWrite)
      //先处理写事件
      if ((events_ & EPOLLIN) && (events_ & EPOLLOUT)) {
        events_ = __uint32_t(0);
        events_ |= EPOLLOUT;
      }
      // events_ |= (EPOLLET | EPOLLONESHOT);
      events_ |= EPOLLET;
      loop_->updateToPoller(channel_, timeout);
    }
    // events_ == 0(即当前http请求和回应已完成) 但设为keepAlive的情况(长连接)
    //此时如果是长连接,那就定时器为5min
    else if (keepAlive_) {
      events_ |= (EPOLLIN | EPOLLET);
      // events_ |= (EPOLLIN | EPOLLET | EPOLLONESHOT);
      int timeout = DEFAULT_KEEP_ALIVE_TIME;
      loop_->updateToPoller(channel_, timeout);
    }
    // events_ == 0(即当前http请求和回应已完成) 但设为短连接
    //此时如果是短连接: 回应方式一: 那就定时器为2min30s(超时断开)
    //回应方式二: 直接shutdown并close.(注意先shutdown,再close详见那篇博客)
    else {
      //cout << "close normally" << endl;
      loop_->shutdown(channel_);
      loop_->runInLoop(bind(&HttpData::handleClose, shared_from_this()));
      events_ |= (EPOLLIN | EPOLLET);
      // events_ |= (EPOLLIN | EPOLLET | EPOLLONESHOT);
      int timeout = (DEFAULT_KEEP_ALIVE_TIME >> 1);
      loop_->updateToPoller(channel_, timeout);
    }
  }
  //此处注意:connectionState为DISCONNECTING是我们read结果只
  //收到0个字节,这说明对方关闭了连接,但究竟是shutdown(WR)还是close?
  //所以server这里,如果还有要发送的数据(events_&EPOLLOUT),那么就
  //继续发送(此时如果client是close的话会直接收到RST报文,连接也对应关闭)
  //(如果client是shutdown的话,server会继续把数据发完再断开)
  else if (!error_ && connectionState_ == H_DISCONNECTING &&
           (events_ & EPOLLOUT)) {
    //再次发送数据并重新设置EPOLLET
    //这里没有重新绑定定时器
    events_ = (EPOLLOUT | EPOLLET);
  }
  //这里是对方关闭了连接,且server这里没有要发送的数据,直接关闭连接即可
  //或者是发生错误,不得不关闭连接
  //关闭连接(close(connfd))发生在HttpData的析构函数中,也就是在handleClose->removeChannel
  else {
    //cout << "close with errors" << endl;
    loop_->runInLoop(bind(&HttpData::handleClose, shared_from_this()));
  }
}

//关闭连接,删除channel
void HttpData::handleClose() {
  connectionState_ = H_DISCONNECTED;
  std::shared_ptr<HttpData> guard(shared_from_this());
  //在removedFromPoller中,发生close(fd)关闭连接
  loop_->removedFromPoller(channel_);
}

//在server里handNewConn中使用(将这个http事件插入到线程loop中)
void HttpData::newEvent() {
  channel_->setEvents(READEVENT | EPOLLONESHOT);
  loop_->addToPoller(channel_, DEFAULT_EXPIRED_TIME);
}

/**********************************以下是解析Http行,首部,body等工具函数********************/
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
  //cout << "fileName_: " << fileName_ << endl;

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
AnalysisState HttpData::analysisRequest() {
  if (method_ == METHOD_POST) {
    // TODO:POST方法
    outBuffer_ =
        "HTTP/1.1 200 OK\r\nContent-type: text/plain\r\n\r\nPOST method not "
        "support yet!";
    return ANALYSIS_SUCCESS;
  } else if (method_ == METHOD_GET || method_ == METHOD_HEAD) {
    std::string header;
    header += "HTTP/1.1 200 OK\r\n";
    // keep-Alive选项
    if (headers_.find("Connection") != headers_.end() &&
        (headers_["Connection"] == "Keep-Alive" ||
         headers_["Connection"] == "keep-alive")) {
      keepAlive_ = true;
      header +=
          std::string("Connection: Keep-Alive\r\n") +
          "Keep-Alive: timeout=" + std::to_string(DEFAULT_KEEP_ALIVE_TIME) +
          "\r\n";
    }
    int dot_pos = fileName_.find('.');
    //获取GET的文件类型
    std::string filetype;
    if (dot_pos < 0)
      filetype = MimeType::getMime("default");
    else
      filetype = MimeType::getMime(fileName_.substr(dot_pos));

    // echo test(测试整个网络框架用)
    //客户端发来hello
    if (fileName_ == "hello") {
      outBuffer_ =
          "HTTP/1.1 200 OK\r\nContent-type: text/plain\r\n\r\nHello World";
      return ANALYSIS_SUCCESS;
    }
    //请求favicon.ico(页面图标)
    if (fileName_ == "favicon.ico") {
      header += "Content-Type: image/png\r\n";
      header += "Content-Length: " + std::to_string(sizeof favicon) + "\r\n";
      header += "Server: WRH's Web Server\r\n";

      header += "\r\n";
      outBuffer_ += header;
      outBuffer_ += std::string(favicon, favicon + sizeof favicon);
      return ANALYSIS_SUCCESS;
    }

    // stat获取服务器上文件的信息
    //详见<<APUE>>4.2
    struct stat sbuf;
    if (stat(fileName_.c_str(), &sbuf) < 0) {
      header.clear();
      handleError(fd_, 404, "Not Found!");
      return ANALYSIS_ERROR;
    }
    header += "Content-Type: " + filetype + "\r\n";
    header += "Content-Length: " + std::to_string(sbuf.st_size) + "\r\n";
    header += "Server: WRH's Web Server\r\n";
    // 头部结束
    header += "\r\n";
    outBuffer_ += header;

    if (method_ == METHOD_HEAD) return ANALYSIS_SUCCESS;

    int src_fd = open(fileName_.c_str(), O_RDONLY, 0);
    if (src_fd < 0) {
      outBuffer_.clear();
      handleError(fd_, 404, "Not Found!");
      return ANALYSIS_ERROR;
    }
    //使用mmap内核空间和用户空间共享
    // mmap详解:https://blog.csdn.net/Holy_666/article/details/86532671
    void *mmapRet = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
    close(src_fd);
    if (mmapRet == (void *)-1) {
      munmap(mmapRet, sbuf.st_size);
      outBuffer_.clear();
      handleError(fd_, 404, "Not Found!");
      return ANALYSIS_ERROR;
    }
    char *src_addr = static_cast<char *>(mmapRet);
    //将服务器上面的资源写到outBuffer_中
    outBuffer_ += std::string(src_addr, src_addr + sbuf.st_size);
    ;
    munmap(mmapRet, sbuf.st_size);
    return ANALYSIS_SUCCESS;
  }

  return ANALYSIS_ERROR;
}
