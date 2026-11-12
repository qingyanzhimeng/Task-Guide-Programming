#include "thread_pool.h"

thread_pool::thread_pool(size_t thread_num)
:_thread_vector()
,_stop(false)
{
    //根据调用方,传入的thread_num来创建对应的线程
    if(thread_num==0)
    {
        throw std::invalid_argument("Invalid parameter");
    }
    else
    {
        this->_thread_vector.reserve(thread_num);
        //传入的参数有效,然后根据参数的大小创建对应的线程
        try
        {
            for(size_t i=0;i<thread_num;++i)
            {
                this->_thread_vector.emplace_back(&thread_pool::worker,this);
            }
        }
        catch(...)
        {
            std::cerr<<"An exception occurred while creating the thread"<<std::endl;
            //对已经创建好的线程进行一个释放
            //首先唤醒所有的线程
            {
                std::lock_guard<std::mutex> _lg(this->_mutex);
                this->_stop=true;
            }
            //唤醒所有线程
            this->_con_variable.notify_all();
            //回收线程资源
            for(auto& e:this->_thread_vector)
            {
                if(e.joinable())
                {
                    e.join();
                }
            }
            throw;
        } 
    }
}
thread_pool::~thread_pool()//析构函数
{
    //对于析构函数也是如何,按照顺序来释放资源
    this->stop_thread_pool();
}
bool thread_pool::set_task(task _task)//往任务队列里面放置任务
{
    //放置任务,检查与放置任务一气呵成
    {
        std::lock_guard<std::mutex> _lg(this->_mutex);
        //判断当前系统状态是否可以放置任务
        if(this->_stop||this->_task_queue.size()==TASK_QUEUE_LENGTH)
        {
            //已经停止操作,禁止放置新的任务
            return false;
        }
        else
        {
            //放置新的任务
            this->_task_queue.push(std::move(_task));
        }
    }
    //放置任务成功,通知一个线程进行取任务
    this->_con_variable.notify_one();
    return true;
}
void thread_pool::worker()//线程去任务队列取任务
{
    //每个线程都在不断地循环的读取任务队列中的信息来取任务
    while(true)
    {
        task _task;
        {
            //检查是否可以取任务
            std::unique_lock<std::mutex> _ul(this->_mutex);
            this->_con_variable.wait(_ul,[this](){return this->_stop||!this->_task_queue.empty();});
            //不用等待就表示要取任务,判断任务是否为空
            if(!this->_task_queue.empty())
            {
                //表示任务队列不为空,取任务
                _task=std::move(this->_task_queue.front());
                //任务出队列
                this->_task_queue.pop();
            }
            else
            {
                //表示任务为空,这个表示,我线程本身不再等待,但是没有任何任务
                return ;
            }
        }
        //执行任务
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
void thread_pool::stop_thread_pool()//结束线程池
{
    {
        std::lock_guard<std::mutex> lg(this->_mutex);
        if(this->_stop==true)
        {
            return;
        }
    }
    {
        std::lock_guard<std::mutex> lg(this->_mutex);
        this->_stop=true;
        this->_con_variable.notify_all();
    }
    std::cout<<"thread_pool stop start"<<std::endl;
    for(auto&e:this->_thread_vector)
    {
        if(e.joinable())
        {
            e.join();
        }
    }
    std::cout<<"thread_pool stop end"<<std::endl;
}