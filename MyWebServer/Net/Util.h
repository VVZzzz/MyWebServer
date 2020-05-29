/**
 * 包装一层readn writen bind 等API函数
 * 主要用于字节流socket上的一些read和write
 * 且定义了几个全局变量
 * 详见<UNP>3.9说明
 */ 
#pragma once
#include <cstdlib>
#include <string>

extern const int READEVENT;
extern const int WRITEEVENT;

ssize_t readn(int fd , void *buff,size_t n);
ssize_t readn(int fd, std::string &inBuffer, bool &zero);
ssize_t readn(int fd, std::string &inBuffer);
ssize_t writen(int fd, void *buff, size_t n);
ssize_t writen(int fd, std::string &sbuff);
void handle_for_sigpipe();
int setSocketNonBlocking(int fd);
void setSocketNodelay(int fd);
void setSocketNoLinger(int fd);
void shutDownWR(int fd);
int socket_bind_listen(int port);