#include "Thread/Thread.h"
#include "CurrentThread.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
using namespace std;

namespace CurrentThread
{
    //使用__thread变量
    //__thread是GCC内置的线程局部存储设施。_thread变量每一个线程有一份独立实体，各个线程的值互不干扰。可以用来修饰那些带有全局性且值可能变，但是又不值得用全局变量保护的变量。
    // https://www.jianshu.com/p/997b533842c8
    __thread int t_cachedTid = 0; //缓存当前线程id
    __thread char t_tidString[32];
    __thread int t_tidStringLength = 6;
    __thread const char *t_threadName = "default";
} // namespace CurrentThread

//syscall系统调用,https://www.cnblogs.com/hazir/p/three_methods_of_syscall.html
pid_t gettid() { return static_cast<pid_t>(::syscall(SYS_gettid)); }

void CurrentThread::cacheTid()
{
    if (t_cachedTid == 0)
    {
        t_cachedTid = gettid();
        t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
    }
}
//在线程中保留name tid等数据,作为线程传递的参数类型
struct ThreadData
{
    //线程函数
    typedef Thread::ThreadFunc ThreadFunc;
    ThreadFunc func_;
    string name_;
    pid_t *tid_;
    CountDownLatch *latch_;

    ThreadData(const ThreadFunc &func, const string &name, pid_t *tid, CountDownLatch *latch) : 
    func_(func), name_(name), tid_(tid), latch_(latch) {}

    void runInThread()
    {
        *tid_ = CurrentThread::tid();
        tid_ = NULL;
        //减1
        latch_->countDown();
        latch_ = NULL;

        CurrentThread::t_threadName = name_.empty() ? "Thread" : name_.c_str();
        //使用prctl接口设置进程名字
        prctl(PR_SET_NAME, CurrentThread::t_threadName);

        //执行线程函数
        func_();
        //执行完毕
        CurrentThread::t_threadName = "finished";
    }
};

//类外的线程函数
void *startThread(void *obj)
{
    ThreadData *data = static_cast<ThreadData *>(obj);
    data->runInThread();
    delete data;
    return NULL;
}

Thread::Thread(const ThreadFunc &func, const string &n)
    : started_(false),
      joined_(false),
      pthreadId_(0),
      tid_(0),
      func_(func),
      name_(n),
      latch_(1)  //只有1个子线程
{
    setDefalutName();
}

Thread::~Thread()
{
    if (started_ && !joined_)
        //这里用detach,将该线程设为"分离"状态
        //如果线程是joinable状态，当线程函数自己返回退出时或
        //pthread_exit时都不会释放线程所占用堆栈和线程描述符。
        //只有当你调用了pthread_join之后这些资源才会被释放。
        //若是unjoinable状态的线程，这些资源在线程函数退出时或pthread_exit时自动被操作系统释放。
        pthread_detach(pthreadId_);
}

void Thread::setDefalutName()
{
    if (name_.empty())
    {
        char buf[32];
        snprintf(buf, sizeof buf, "Thread");
        name_ = buf;
    }
}

void Thread::start()
{
    assert(!started_);
    started_ = true;
    ThreadData *data = new ThreadData(func_, name_, &tid_,&latch_);
    //开启一个线程
    if (pthread_create(&pthreadId_, NULL, &startThread, data))
    {
        //开启失败
        started_ = false;
        delete data;
    }
    else
    {
        //这个wait是等待子线程真正开始执行startThread,主线程才返回
        latch_.wait();
        assert(tid_ > 0);
    }
}

int Thread::join(){
    joined_ = true;
    //无需返回状态,设为NULL
    return pthread_join(pthreadId_,NULL);
}
