#include "thread_pool.h"

ThreadPoll::ThreadPoll(size_t thread_num)//构造函数,可以通过传入的个数来创建线程的个数
:_thread_pool()
,_stop(false)
{
    if(thread_num==0)
    {
        throw std::invalid_argument("invalid thread_num");
    }
    //根据传入的size大小来预留空间的大小
    this->_thread_pool.reserve(thread_num);//这个表示可以装几个线程对象
    
     try
    {
        for(size_t i=0;i<thread_num;++i)
        {
            //创建线程
           this->_thread_pool.emplace_back(&ThreadPoll::worker,this);
        } 
    }
     catch(...)
     {
        {
            std::lock_guard<std::mutex> lg(this->_mutex);
            this->_stop.store(true);
        }
        //通知所有的线程
        this->_task_condi_var.notify_all();
        for(auto&e:this->_thread_pool)
        {
            if(e.joinable())
            {
                e.join();
            }
        }
        //将异常继续往上面抛
        throw;    
    }
}
ThreadPoll::~ThreadPoll()//析构函数用来将构建的线程进行一个回收
{
    //这里首先先对所有的线程进行一个唤醒操作
    {
        std::lock_guard<std::mutex> lg(this->_mutex);
        //设置停止表示
        this->_stop.store(true);
    }
    this->_task_condi_var.notify_all();
    for(auto& e:this->_thread_pool)
    {
        if(e.joinable())
        {
            e.join();
        }
    }

}
bool ThreadPoll::add_task(task _task)//往任务队列里面放置任务
{
    //往任务队列里面放任务
    {
        std::lock_guard<std::mutex> lg(this->_mutex);
        //判断当前线程池的状态是否还可以放新的任务
        if(this->_stop.load())
        {
            //不用放置新的任务
            return false;
        }
        else
        {
            //放置新的任务
            this->_task_queue.push(std::move(_task));
        }
    }
    //解锁,通知某个正在等待任务的线程
    this->_task_condi_var.notify_one();
    return true;
}
void ThreadPoll::worker()//线程可以从任务队列里面取出任务进行一个执行
{
    //每个线程都需要循环的取任务
    while(true)
    {
        task _task;
        {
            //加锁
            std::unique_lock<std::mutex> ul(this->_mutex);
            //线程都会从任务队列里面取任务
            this->_task_condi_var.wait(ul,[this](){return this->_stop.load()||!this->_task_queue.empty();});
            //从这里出去不一定是一定是有任务,还有可能是线程池已经结束
            if(this->_stop.load()&&this->_task_queue.empty())
            {
                //直接结束线程
                return ;
            }
            //否则从任务队列里面取任务,执行任务
            _task=std::move(this->_task_queue.front());
            this->_task_queue.pop();
        }
        //释放锁,执行任务
        try
        {
             _task();
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        catch(...)
        {
            std::cerr << "unknown task error" << '\n';
        }
       
    }
}