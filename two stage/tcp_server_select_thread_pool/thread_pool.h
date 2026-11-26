#pragma once 
#include <iostream>
#include <thread>
#include <vector>
#include <functional>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
//线程池,用来并发的执行任务的线程池

using task=std::function<void()>;//任务
using task_queue=std::queue<task>;//任务队列

class thread_pool
{
public:
thread_pool(size_t thread_num=4,size_t task_num=5);//默认是四个线程,然后任务队列的大小默认是5个
thread_pool(thread_pool&& _thread_pool)=delete;
thread_pool(const thread_pool& _thread_pool)=delete;
thread_pool& operator=(const thread_pool& _thread_pool)=delete;
thread_pool& operator=(thread_pool && _thread_pool)=delete;
~thread_pool();//析构函数
bool add_task(task _task);//添加任务
void stop_thread_pool();//停止线程池工作
private:
void working();//线程开始工作的入口函数
private:
std::vector<std::thread> _worker;//线程容器
task_queue _task_queue;//任务队列
std::mutex _mutex;//互斥锁
std::condition_variable _con_var;//条件变量
bool _stop;//停止标识
size_t _task_num;//任务队列的最大容量
};
