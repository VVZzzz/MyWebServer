#pragma once
#include "FileUtil.h"
#include "Thread/Mutex.h"
#include "Thread/noncopyable.h"
#include <memory>
#include <string>

//包裹AppendFile(FileUtil)一层,为的是每append n 次,就flush一次
class LogFile:public noncopyable{
    public:
    LogFile(const std::string &str,int n = 1024);
    ~LogFile();

    void append(const char *logline , int len);
    void flush();
    //bool rollFile();  //TODO

    private:
    void append_unlocked(const char *logline,int len);
    const int everyNAppendFlush_;  //每append n次,就flush一次(无论是否写满缓冲区)
    const std::string basename_;
    int count_;

    std::unique_ptr<MutexLock> mutex_;
    std::unique_ptr<AppendFile>file_;

};
