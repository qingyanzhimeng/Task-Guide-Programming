#pragma once
#include <iostream>
#include <thread>
#include <vector>
#include <functional>
#include <atomic>
#include <queue>
#include <condition_variable>
#include <mutex>
#include <cstddef>
#include <utility>
using task=std::function<void()>;//将返回值为void以及空参数值用function进行一个包装,得到一个类型
using task_queue=std::queue<task>;//这个队列表示一个任务队列
class ThreadPoll
{
public:
ThreadPoll(size_t thread_num);//构造函数,可以通过传入的个数来创建线程的个数
ThreadPoll(const ThreadPoll& threadpoll)=delete;//删除拷贝构造与拷贝赋值
ThreadPoll& operator=(const ThreadPoll& threadpoll)=delete;
~ThreadPoll();//析构函数用来将构建的线程进行一个回收
bool add_task(task _task);//往任务队列里面放置任务
private:
void worker();//线程可以从任务队列里面取出任务进行一个执行
private:
//先有一个容器来装线程对象
std::vector<std::thread> _thread_pool;//表示线程池
std::atomic<bool> _stop;//表示是否还继续放置任务
task_queue _task_queue;//表示任务队列,外界可以放置任务
std::condition_variable _task_condi_var;//表示任务队列的条件变量
std::mutex _mutex;//这个互斥锁用来互斥的访问任务队列
};