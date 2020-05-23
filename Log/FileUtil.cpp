#include "FileUtil.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
using namespace std;

AppendFile::AppendFile(std::string filename) : fp_(fopen(filename.c_str(), "ae"))
{
    //使用自己提供的缓冲区
    setbuffer(fp_, buffer_, sizeof buffer_);
}
AppendFile::~AppendFile() { fclose(fp_); }

void AppendFile::append(const char *logmsg, size_t len)
{
    size_t n = this->write(logmsg, len);
    size_t remain = len - n;
    while (remain > 0)
    {
        size_t writen = write(logmsg + n, len);
        if (writen == 0)
        {
            int err = ferror(fp_);
            if (err)
                fprintf(stderr, "AppendFile::append() failed !\n");
            break;
        }
        n += writen;
        remain = len - n;
    }
}

size_t AppendFile::write(const char *logline, size_t len)
{
    //fwrite_unlocked和fwrite是一样的 , 但前者并非线程安全
    return fwrite_unlocked(logline, 1, len, fp_);
}

void AppendFile::flush(){
    //将缓冲区的内容刷到设备中
    fflush(fp_);
}