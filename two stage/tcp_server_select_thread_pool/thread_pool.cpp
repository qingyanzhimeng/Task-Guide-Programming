#include "thread_pool.h"

thread_pool::thread_pool(size_t thread_num,size_t task_num)//默认是四个线程
:_worker(thread_num)
,_task_queue()
,_stop(false)
,_task_num(task_num)
{
    try
    {
        //创建线程
        for(size_t i=0;i<thread_num;++i)
        {
            this->_worker.emplace_back(&thread_pool::working,this);
        }
    }
    catch(...)
    {
        //创建线程池出现了异常,关闭等待线程
        this->stop_thread_pool();
        throw;
    } 
}
thread_pool::~thread_pool()//析构函数
{
    this->stop_thread_pool();
}
bool thread_pool::add_task(task _task)//添加任务
{
    //向线程池添加任务
    {
        std::lock_guard<std::mutex> lg(this->_mutex);
        //线程池没有停止工作,但是任务栈已经满了,禁止添加新的任务
        if(this->_stop)
        {
            std::cerr<<"The thread pool has stopped working"<<"\n";
            return false;
        }
        if(!this->_stop&&this->_task_queue.size()==this->_task_num)
        {
            std::cerr<<"The task queue is full"<<"\n";
            return false;
        }
        //否则添加任务
        this->_task_queue.push(_task);
    }
    //通知一个线程
    this->_con_var.notify_one();
    return true;
}
void thread_pool::stop_thread_pool()//停止线程池工作
{
    bool stop_status=false;
    //防止多次执行停止
    {
        std::lock_guard<std::mutex> lg(this->_mutex);
        if(!this->_stop)
        {
            //第一次停止线程池
            stop_status=true;
            this->_stop=true;//更新为停止
        }
    }
    if(stop_status)
    {
        //通知所有的线程
        this->_con_var.notify_all();
        for(auto & e:this->_worker)
        {
            if(e.joinable())
            {
                e.join();
            }
        }
    }
    else
    {
        std::cerr<<"Stop the operation from repeating"<<"\n";
    }
}
void thread_pool::working()//线程开始工作的入口函数
{
    //拿任务,就算当_stop为true的时候,还是需要将队列中的任务做完才能返回
    while(true)
    {
        task _task;
        {
            std::unique_lock<std::mutex> ul(this->_mutex);
            //用条件变量的wait谓词来进行一个判断,判断此时此刻是否可以拿去任务执行任务
            this->_con_var.wait(ul,[&](){return this->_stop||!this->_task_queue.empty();});
            //满足谓词条件返回,有四种返回
            //第一种,线程池没有停止,但是队列中有任务 false|| true
            //第二种 false || false //阻塞等待
            //第三种 true || true //线程池停止有任务
            //第四种 true || false 线程池停止没有任务
            if(!this->_task_queue.empty())
            {
                //取任务
                _task=std::move(this->_task_queue.front());
                this->_task_queue.pop();
            }
            else
            {
                //线程池停止且没有任务了
                break;
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
    }
}