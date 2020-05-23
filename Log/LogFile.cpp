#include "LogFile.h"

#include <stdio.h>

#include "FileUtil.h"

using namespace std;

LogFile::LogFile(const std::string &str, int n = 1024)
    : basename_(str), everyNAppendFlush_(n), count_(0), mutex_(new MutexLock) {
  file_.reset(new AppendFile(basename_));
}
LogFile::~LogFile(){};

void LogFile::append(const char *logline, int len) {
  MutexLockGuard lock(*mutex_);
  append_unlocked(logline, len);
}

void LogFile::append_unlocked(const char *logline,int len){
    //append中循环write直至写完
    file_->append(logline,len);
    //增加append次数
    ++count_;
    if(count_>= everyNAppendFlush_)
    {
        count_ = 0;
        file_->flush();
    }
}

void LogFile::flush() {
    MutexLockGuard lock(*mutex_);
    file_->flush();
}