#pragma once 
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <utility>
#include <functional>
#include <condition_variable>
#include <stdexcept>
#include <iostream>
#define TASK_QUEUE_LENGTH 2  //这个表示任务队列最多可以同时放置八个任务

using task=std::function<void()>;//表示任务
using task_queue=std::queue<task>;//表示任务队列

class thread_pool
{
public:
//构造函数
thread_pool(size_t thread_num=4);
thread_pool(const thread_pool& _thread_pool)=delete;
thread_pool& operator=(const thread_pool& _thread_pool)=delete;
~thread_pool();//析构函数
bool set_task(task _task);//往任务队列里面放置任务
void stop_thread_pool();//结束线程池
private:
void worker();//线程去任务队列取任务
private:
//存放线程的容器
std::vector<std::thread> _thread_vector;
task_queue _task_queue;//任务队列
bool _stop;//表示线程池是否要进行结束
std::mutex _mutex;//互斥锁,用来互斥访问任务队列资源
std::condition_variable _con_variable;//条件变量,用来进行一个同步操作
};