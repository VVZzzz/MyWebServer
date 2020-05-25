#include "AsyncLogging.h"

#include "LogFile.h"

AsyncLogging::AsyncLogging(const std::string basename, int flush)
    : flushInterval_(flush),
      running_(false),
      basename_(basename),
      //此处用bind,将threadFunc和this作为threadFunc的参数
      thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
      mutex_(),
      cond_(mutex_),
      currentBuffer_(new Buffer),
      nextBuffer_(new Buffer),
      buffers_(),
      latch_(1) {
  //清空缓冲区
  currentBuffer_->bzero();
  nextBuffer_->bzero();
  buffers_.reserve(16);
}

//各个前端线程调用这个append,将log内容写到
//currentbuffer中
void AsyncLogging::append(const char *logline, int len) {
  MutexLockGuard lock(mutex_);
  //若currentBuffer空间足够,就append
  //不够,就换nextBuffer来
  if (currentBuffer_->avail() > len) {
    currentBuffer_->append(logline, len);
  } else {
    //currentBuffer不够了 , 换nextBuffer_
    //并唤醒后端线程
    buffers_.push_back(currentBuffer_);
    //由于currentBuffer_是shared_ptr
    //此处reset是将current置为nullptr
    currentBuffer_.reset();
    if (nextBuffer_)
      currentBuffer_ = std::move(nextBuffer_);
    else
      //nextBuffer也不够了 , 只能在前端线程这里先开辟一个new Buffer
      currentBuffer_.reset(new Buffer);

    currentBuffer_->append(logline, len);
    //唤醒后端线程,将buffers_里的内容写到文件上.
    cond_.notify();
  }
}

//后端线程
//这里可以认为buffers_是前端的一个vector,收集log
//buffersToWrite是后端的,真正用来写入logfile的
void AsyncLogging::threadFunc() {
  //倒计时减1
  latch_.countDown();
  LogFile output(basename_);
  //再用2块作为备用
  BufferPtr newBuffer1(new Buffer);
  BufferPtr newBuffer2(new Buffer);
  newBuffer1->bzero();
  newBuffer2->bzero();

  BufferVector buffersToWrite;
  buffersToWrite.reserve(16);

  while (running_) {
    assert(newBuffer1 && newBuffer1->length() == 0);
    assert(newBuffer2 && newBuffer2->length() == 0);
    assert(buffersToWrite.empty());

    {
      MutexLockGuard lock(mutex_);
      // unusual usage!
      if (buffers_.empty()) cond_.waitForSeconds(flushInterval_);
      buffers_.push_back(currentBuffer_);
      currentBuffer_.reset();

      //将已有内容的buffer添加到vector中
      //再次指向空闲的newBuffer1
      currentBuffer_ = std::move(newBuffer1);
      //此处注意,STL中vector的swap成员函数
      //时间为O(1)无需对每个元素进行移动,拷贝等
      //buffersToWrite是要写入LogFile的缓冲
      buffersToWrite.swap(buffers_);

      if (!nextBuffer_) nextBuffer_ = std::move(newBuffer2);
    }

    assert(!buffersToWrite.empty());
    //丢弃多余的
    if (buffersToWrite.size() > 25) {
      // char buf[256];
      // snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger
      // buffers\n",
      //          Timestamp::now().toFormattedString().c_str(),
      //          buffersToWrite.size()-2);
      // fputs(buf, stderr);
      // output.append(buf, static_cast<int>(strlen(buf)));
      buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
    }

    for (size_t i = 0; i < buffersToWrite.size(); ++i) {
      // FIXME: use unbuffered stdio FILE ? or use ::writev ?
      output.append(buffersToWrite[i]->data(), buffersToWrite[i]->length());
    }
    if (buffersToWrite.size() > 2) {
      // drop non-bzero-ed buffers, avoid trashing
      buffersToWrite.resize(2);
    }

    // append完内容后的处理
    //将这两块备用的重新填充
    if (!newBuffer1) {
      assert(!buffersToWrite.empty());
      newBuffer1 = buffersToWrite.back();
      buffersToWrite.pop_back();
      //注意这里是->reset(),调用的是Buffer类的reset()
      //不收回内存
      newBuffer1->reset();
    }
    if (!newBuffer2) {
      assert(!buffersToWrite.empty());
      newBuffer2 = buffersToWrite.back();
      buffersToWrite.pop_back();
      newBuffer2->reset();
    }

    buffersToWrite.clear();
    output.flush();
  }
  output.flush();
}
