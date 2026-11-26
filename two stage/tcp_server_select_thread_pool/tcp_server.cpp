#include "tcp_server.h"

client_state::client_state(std::string client_ip,std::uint16_t client_port)
:_client_ip(client_ip)
,_client_port(client_port)
,_pending()
,_send_buffer()
,_send_offset(0)
{
}
void client_state::client_info_print()//用户信息打印
{
    std::cout<<this->_client_ip<<":"<<this->_client_port;
}

tcp_server::tcp_server(std::string ip,std::uint16_t port)
:_ip(ip)
,_port(port)
,_listen_fd(-1)
,_client_map()
,_client_task_queue()
,_stop(true)
,_init_status(false)
,_max_fd()
{
    //初始化线程池
    try
    {
        this->_thread_pool=std::make_unique<thread_pool>(THREAD_NUM,TASK_NUM);
    }
    catch(...)
    {
        std::cerr <<"thread_pool create failure"<< '\n';
        throw;
    }
    std::cout<<"the thread_pool create success"<<'\n';
    //线程池创建成功,创建listen套接字
    this->_listen_fd=socket(AF_INET,SOCK_STREAM,0);
    if(this->_listen_fd>=0)
    {
        std::cout<<"socket create success"<<'\n';
        //然后开始绑定
        sockaddr_in server_addr;
        server_addr.sin_family=AF_INET;
        //端口号进行转换
        try
        {
            server_addr.sin_port=htons(this->_port);
        }
        catch(...)
        {
            std::cerr<<"htons have error"<<'\n';
            close(this->_listen_fd);
            throw;
        }
        //ip地址进行一个转换
        int ret=inet_pton(AF_INET,this->_ip.c_str(),&server_addr.sin_addr);
        if(ret==1)
        {
            //转换正确
            int ret2=bind(this->_listen_fd,reinterpret_cast<sockaddr*>(&server_addr),static_cast<socklen_t>(sizeof(server_addr)));
            if(ret2==0)
            {
                //绑定成功
                std::cout<<"bind success"<<'\n';
                this->_init_status=true;//可以开始start函数了
                //初始化位图
                FD_ZERO(&this->master_read_set);
                FD_ZERO(&this->ready_read_set);
                FD_ZERO(&this->master_write_set);
                FD_ZERO(&this->ready_write_set);
                this->update_read_write_fd(this->_listen_fd,ADD); 
                //设置非阻塞io
                if(!this->set_noblock(this->_listen_fd))
                {
                    //设置失败
                    close(this->_listen_fd);
                    throw std::runtime_error("set_noblock set failure");
                }
            }
            else
            {
                int bind_errno=errno;
                close(this->_listen_fd);
                throw std::runtime_error(std::string("bind error")+strerror(bind_errno));
            }
        }
        else if(ret==0)
        {
            //格式有问题
            std::cerr<<"inet_pton format error"<<'\n';
            close(this->_listen_fd);
            throw std::runtime_error("inet_pton format error");
        }
        else
        {
            int inet_pton_errno=errno;
            close(this->_listen_fd);
            throw std::runtime_error(std::string("af does not contain a valid address family")+strerror(inet_pton_errno));
        }
    }
    else 
    {
        int socket_errno=errno;
        throw std::runtime_error(std::string("socket create failure")+std::strerror(socket_errno));
    }
}
tcp_server::~tcp_server()
{
    //调用this->stop()
    this->server_end();
    //回收服务线程
    if(this->_server_thread.joinable())
    {
        this->_server_thread.join();
    }
    //调用thread_stop的函数,表示线程池可以
    this->_thread_pool->stop_thread_pool();
    //线程池也回收完毕,开始清理,map
    {
        std::lock_guard<std::mutex> lg(this->_mutex_map);
        auto it=this->_client_map.begin();
        while(it!=this->_client_map.end())
        {
            close(it->first);
            ++it;
        }
    }
    //然后关闭监听套接字
    if(this->_listen_fd>=0)
        close(this->_listen_fd);
}
void tcp_server::start()
{
    //防止重复执行这个start函数
    bool start_status=false;
    {
        std::lock_guard<std::mutex> lg(this->_mutex_stop);
        if(this->_init_status)
        {
            //初始化成功,可以调用start
            start_status=true;
            this->_init_status=false;
        }
    }
    if(start_status)
    {
        //这里可以启动服务线程了
        try
        {
            std::cout<<"the tcp_server start execute"<<'\n';
            this->_stop.store(false);
            this->_server_thread=std::thread(&tcp_server::server_start,this);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            this->_stop.store(true);
        }
    }
    else
    {
        std::cerr<<"The start function is being called repeatedly"<<'\n';
        return ;
    }
}
void tcp_server::server_end()//结束服务函数
{
    //防止重复停止
    bool stop_status=this->_stop.exchange(true);
    if(stop_status)
    {
        std::cerr<<"the stop function is beening called repeatedly"<<'\n';
        return ;
    }
}
void tcp_server::server_start()//服务开始函数
{
    //开始提供服务
    int ret=listen(this->_listen_fd,10);
    if(ret==0)
    {
        std::cout<<"listen success"<<'\n';
        while(!this->_stop.load())
        {
            timeval time_out;
            time_out.tv_sec=SEC;
            time_out.tv_usec=USEC;
            this->ready_read_set = this->master_read_set;
            this->ready_write_set = this->master_write_set;
            int _nfds=this->get_max_fd()+1;
            int num=select(_nfds,&this->ready_read_set,&this->ready_write_set,nullptr,&time_out);
            if(num>0)
            {
                //进行一个处理
                try
                {
                    this->process_select(_nfds-1);
                }
                catch(const std::exception& e)
                {
                    std::cerr << e.what() << '\n';
                }
            }
        }
    }
    else 
    {
        int listen_errno=errno;
        throw std::runtime_error(std::string("the tcp_server listen error")+std::strerror(listen_errno));
    }
}

void tcp_server::process_select(int max_fd)
{
    for(int fd=0;fd<=max_fd;++fd)
    {
        if(FD_ISSET(fd,&this->ready_read_set))
        {
            //处理accept
            if(this->_listen_fd==fd)
            {
                try
                {  
                    this->process_accept();
                }
                catch(const std::exception& e)
                {
                    std::cerr << e.what() << '\n';
                    continue;
                }
            }
            else
            {
                try
                {
                    //否则是其他的通信函数
                    this->commands_parsing(fd);
                }
                catch(...)
                {
                    this->update_client_map(fd,client_state("",0),DEL);
                    this->update_read_write_fd(fd,DEL);
                    close(fd);
                    continue;
                }  
            }
        }
        if(FD_ISSET(fd,&this->ready_write_set))
        {
            try
            {
                //处理写
                this->send_result(fd);
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
            } 
        }
    }   
}
void tcp_server::process_accept()//处理this->listen_fd中的recv
{
    sockaddr_in client_addr;
    client_addr.sin_family=AF_INET;
    socklen_t len=static_cast<socklen_t>(sizeof(client_addr));
    int client_fd=accept(this->_listen_fd,reinterpret_cast<sockaddr*>(&client_addr),&len);
    if(client_fd>=0)
    {
        uint16_t client_port;
        try
        {
            //端口号
            client_port=ntohs(client_addr.sin_port);
        }
        catch(...)
        {
            close(client_fd);
            throw;
        }
        char buff[16];
        const char * client_ip=inet_ntop(AF_INET,&client_addr.sin_addr,buff,static_cast<socklen_t>(sizeof(buff)));
        if(client_ip!=nullptr)
        {
            //转换成功
            client_state new_client(client_ip,client_port);
            if(!this->set_noblock(client_fd))
            {
                close(client_fd);
                throw std::runtime_error("set_noblock set failure");
            }
            this->update_client_map(client_fd,std::move(new_client),ADD);
            //更新master_set
            this->update_read_write_fd(client_fd,ADD);
        }
        else
        {
            //为空
            int inet_ntop_errno=errno;
            close(client_fd);
            throw std::runtime_error(std::string("inet_ntop have error")+std::strerror(inet_ntop_errno));
        }
    }
    else
    {
        int accept_errno=errno;
        throw std::runtime_error(std::string("accept have error")+std::strerror(accept_errno));
    }
}
void tcp_server::commands_parsing(int client_fd)//命令解析函数以及加入任务队列
{
    //解析命令,这个同样是io线程进行的任务,然后将解析的命令放到std::queue<std::string> _client_task_queue;队列中,由线程池完成任务,然后将结果放到
    //std::queue<std::string> _completion_task_queue;中就行了
    char buffer[1024];
    size_t size=sizeof(buffer);
    std::map<int,client_state>::iterator it;
    {
        std::lock_guard<std::mutex> lg(this->_mutex_map);
        it=this->_client_map.find(client_fd);
    }
    while(true)
    {
        ssize_t len=recv(client_fd,buffer,size,0);
        if(len>0)
        {
            //拷贝
            it->second._pending.append(buffer,len);
            try
            {
                this->get_all_commands(it);
            }
            catch(...)
            {
               std::cerr<<"get_all_commands have error"<<'\n';
               throw;
            }  
        }
        else if(len==0)
        {
            //对方优雅的关闭连接
            std::cout<<"client:";
            it->second.client_info_print();
            std::cout<<" close the link"<<'\n';
            this->update_client_map(client_fd,client_state("",0),DEL);
            this->update_read_write_fd(client_fd,DEL);
            close(client_fd);
            break;
        }
        else
        {
            //有对应的错误码,判断错误码
            int recv_errno=errno;
            if(recv_errno==EINTR)
                continue;
            if(recv_errno==EAGAIN||recv_errno==EWOULDBLOCK)
                break;
            throw std::runtime_error(std::string("recv have error")+std::strerror(recv_errno));
        }
    }
}
void tcp_server::get_all_commands(std::map<int,client_state>::iterator& it)
{
    //对收到的命令进行一个解析,然后将命令与文件描述符进行一个打包,存储到task_queue中
    for(size_t pos=it->second._pending.find('\n');pos!=std::string::npos;pos=it->second._pending.find('\n'))
    {
        std::string command=it->second._pending.substr(0,pos);
        if(command.size()==0)
        {
            it->second._pending.erase(0,pos+1);
            continue;
        }
        if(command.size()==1&&command.back()=='\r')
        {
            it->second._pending.erase(0,pos+1);
            continue;
        }
        if(command.back()=='\r')
        {
            command.pop_back();
        }
        //判断长度是否符合要求
        if(command.size()<=SIGNAL_COMMAND)
        {
            //对命令进行一个包装,然后放到待处理队列中,然后让线程池里面的线程处理命令,并且得到最终的结果,插入到completion_queue中
            {
                std::lock_guard<std::mutex> lg(this->_mutex_task_queue);
                this->_client_task_queue.push(std::to_string(it->first)+"#"+command);
                it->second._pending.erase(0,pos+1);
                if(!this->_thread_pool->add_task(std::bind(&tcp_server::task_process,this)))
                {
                    std::cout<<"Add the task again"<<'\n';
                    while(!this->_thread_pool->add_task(std::bind(&tcp_server::task_process,this)))
                    {
                        std::cout<<"Add the task again"<<'\n';
                    }
                }
            }
        }
        else
        {
            throw std::runtime_error(std::string("Command exceeds maximum length limit"));
        }
    }
    size_t length=it->second._pending.size();
    if(!it->second._pending.empty()&&it->second._pending.back()=='\r')
        --length;
    if(length>SIGNAL_COMMAND)
        throw std::runtime_error(std::string("Command exceeds maximum length limit"));
}
void tcp_server::task_process()//线程池调用的函数,进行一个任务处理
{
    std::string command;
    //首先取一个任务
    {
        std::lock_guard<std::mutex> lg(this->_mutex_task_queue);
        if(!this->_client_task_queue.empty())
        {
            command=this->_client_task_queue.front();
            this->_client_task_queue.pop();
        }   
        else
            return;
    }
    //释放锁,开始对任务进行一个解析,解析完成之后加入到completion_queue中
    size_t l_pos=command.find('#');
    std::string client_fd=command.substr(0,l_pos);
    std::string real_command=command.substr(l_pos+1,command.size());
    std::string result=this->arith_process(real_command);
    //得到处理结果之后,加入到completion_queue队列中
    {
        std::lock_guard<std::mutex> lg(this->_mutex_completion_queue);
        this->_completion_queue.push(client_fd+'#'+real_command+"="+result);
        this->_completion_con.notify_one();
    } 
}
void tcp_server::process_completion_queue()
{
    //取队列中的数据,然后发送
    while(true)
    {
        std::string result;
        {
            std::unique_lock<std::mutex> ul(this->_mutex_completion_queue);
            this->_completion_con.wait(ul,[&](){return !this->_completion_queue.empty();});
            result=this->_completion_queue.front();
            this->_completion_queue.pop();
        }
        //然后开始发送
        //解析fd
        size_t pos=result.find('#');
        int fd=-1;
        try
        {
            fd=std::stoi(result.substr(0,pos));
        }
        catch(...)
        {
            std::cerr<<"std::stoi thransfer error"<<'\n';
            throw;
        }
        try
        {
            //先发送这个客户端的发送缓冲区是否有数据,如果有数据,先发送这个里面的数据,如果没有就执行完成队列里面数据的发送
            std::map<int,client_state>::iterator it;
            {
                std::lock_guard<std::mutex> lg(this->_mutex_map);
                it=this->_client_map.find(fd);
                if(it==this->_client_map.end())
                    throw std::runtime_error("the client_map no exist the client ");
            }
            //判断发送缓冲区里面是否有数据
            if(it->second._send_buffer.size()>0)
            {
                ssize_t len=0;
                //有数据优先发送上一轮没有发送完成的数据
                while(it->second._send_offset<it->second._send_buffer.size())
                {
                    len=send(fd,it->second._send_buffer.c_str()+it->second._send_offset,it->second._send_buffer.size(),MSG_NOSIGNAL);
                    if(len>0)
                    {
                        it->second._send_offset+=len;
                        {
                            std::lock_guard<std::mutex> lg(this->_mutex_map);
                            auto it=this->_client_map.find(fd);
                            if(it!=this->_client_map.end())
                            {   
                                std::cout<<"the tcp_server send "<<len<<" Bytes to ";
                                it->second.client_info_print();
                                std::cout<<'\n';
                            }
                        }
                    }
                    else if(len==0)
                    {
                        throw std::runtime_error("send 0 Bytes");
                    }
                    else
                    {
                        int send_errno=errno;
                        if(send_errno==EINTR)
                            continue;
                        if(send_errno==EAGAIN||send_errno==EWOULDBLOCK)
                            break;
                        throw std::runtime_error(std::string("send have error")+std::strerror(send_errno));
                    }
                }
                //看是否发送完毕
                if(it->second._send_offset==it->second._send_buffer.size())
                {
                    //发送完毕,移除master_write_set
                    if(FD_ISSET(fd,&this->master_write_set))
                    {
                        FD_CLR(fd,&this->master_write_set);
                    }
                }
                else
                {
                    FD_SET(fd,&this->master_write_set);
                }
            }
            //然后发送队列中的数据
            std::string send_comment=result.substr(pos+1,result.size());
            ssize_t len=0;
            size_t sent=0;
            while(sent<send_comment.size())
            {
                len=send(fd,send_comment.c_str()+sent,send_comment.size()-sent,MSG_NOSIGNAL);
                if(len>0)
                {
                    sent+=len;
                    {
                        std::lock_guard<std::mutex> lg(this->_mutex_map);
                        auto it=this->_client_map.find(fd);
                        if(it!=this->_client_map.end())
                        {   
                            std::cout<<"the tcp_server send "<<len<<" Bytes to ";
                            it->second.client_info_print();
                            std::cout<<'\n';
                        }
                    }
                }
                else if(len==0)
                {
                    throw std::runtime_error("send 0 Bytes");
                }
                else
                {
                    int send_errno=errno;
                    if(send_errno==EINTR)
                        continue;
                    if(send_errno==EAGAIN||send_errno==EWOULDBLOCK)
                        break;
                    throw std::runtime_error(std::string("send have error")+std::strerror(send_errno));
                }
            }
            //看是否发送完毕
            if(sent==send_comment.size())
            {
                //发送完毕,移除master_write_set
            }
            else
            {
                //没有发送完毕,加入client的发送缓冲区
                it->second._send_buffer.append(send_comment.c_str()+sent,send_comment.size()-sent);
                FD_SET(fd,&this->master_write_set);
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            this->update_client_map(fd,client_state("",0),DEL);
            this->update_read_write_fd(fd,DEL);
            close(fd);
        }
        result.clear();
    }
}
void tcp_server::send_result(int client_fd)//将结果进行发送
{
    std::map<int,client_state>::iterator it;
    {
        std::lock_guard<std::mutex> lg(this->_mutex_map);
        it=this->_client_map.find(client_fd);
        if(it==this->_client_map.end())
            throw std::runtime_error("The client fd cannot be found in the client map");
    }
    //发送数据
    ssize_t len=0;
    while(it->second._send_offset<it->second._send_buffer.size())
    {
        len=send(client_fd,it->second._send_buffer.c_str()+it->second._send_offset,it->second._send_buffer.size()-it->second._send_offset,MSG_NOSIGNAL);
        if(len>0)
        {
            it->second._send_offset+=static_cast<size_t>(len);
            std::cout<<"TCP server has sent "<<len<<" bytes to the client:";
            it->second.client_info_print();
            std::cout<<std::endl;
        }
        else if(len==0)
        {
            throw std::runtime_error(std::string("send 0 Bytes"));
        }
        else 
        {
            int send_errno=errno;
            if(send_errno==EINTR)
                continue;
            if(send_errno==EAGAIN||send_errno==EWOULDBLOCK)
                break;
            throw std::runtime_error(std::string("send have error")+std::strerror(send_errno));
        }
    }
    //判断此次发送是否发送完成,如果没有完成继续设置ready_write_set
    if(it->second._send_offset==it->second._send_buffer.size())
    {
        //发送完毕,如果在master_write_set中就进行一个移除操作
        {
            std::lock_guard<std::mutex> lg(this->_mutex_map);
            if(FD_ISSET(it->first,&this->master_write_set))
            {
                FD_CLR(it->first,&this->master_write_set);
            }
            //同时清理用户发送缓冲区
            it->second._send_buffer.clear();
            it->second._send_offset=0;
        }
    }
    else
    {
        {
            std::lock_guard<std::mutex> lg(this->_mutex_map);
            if(!FD_ISSET(it->first,&this->master_write_set))
            {
                FD_SET(it->first,&this->master_write_set);
            }
        }
    }
}
std::string tcp_server::arith_process(std::string & real_command)//这个表示对算术进行处理
{
    //对命令进行一个解析,看是否满足算术表达式的协议类型,如果不符合返回一个提醒,本服务器目前没有引入复杂的算术表达式的处理
    //后续可以加上词法分析,语法分析,以及语义分析功能,这样就可以接收一个比较复杂的加减乘除的运算
    std::vector<size_t> pos_vector;
    pos_vector.push_back(real_command.find('+'));
    pos_vector.push_back(real_command.find('-'));
    pos_vector.push_back(real_command.find('*'));
    pos_vector.push_back(real_command.find('/'));
    std::string result;
    for(size_t i=0;i<pos_vector.size();++i)
    {
        if(pos_vector[i]!=std::string::npos)
        {
            //首先获取两个数字
            std::string s_num1=real_command.substr(0,pos_vector[i]);
            std::string s_num2=real_command.substr(pos_vector[i]+1,real_command.size());
            //进行一个简单的判断,判断两个是否全是数字
            double num1;
            double num2;
            try
            {
               num1=std::stod(s_num1);
               num2=std::stod(s_num2);
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
                //如果抛异常,就将结果改成,传入的数字非法
                result="The number passed in is illegal";
                return result;
            }
            //成功获得两个数字
            double sum;
            switch(i)
            {
                case 0:
                {
                    //加法
                    sum=num1+num2;
                    break;
                }
                 case 1:
                {
                    //减法
                    sum=num1-num2;
                    break;
                }
                 case 2:
                {
                    //乘法
                    sum=num1*num2;
                    break;
                }
                 case 3:
                {
                    //除法
                    if(std::fabs(num2)<EPS)
                    {
                        //这个时候表示为零,除零肯定是不对的
                        result="The divisor cannot be zero";
                        return result;
                    }
                    sum=num1/num2;
                    break;
                }
                default:
                {
                    break;
                }
            }
            result=std::to_string(sum);
            return result;
        }
    }
    result="Parsing failed";
    return result;
}
bool tcp_server::set_noblock(int client_fd)//给这个套接字设置非阻塞
{
    //设置非阻塞IO
    int flags=fcntl(client_fd,F_GETFL,0);
    if(flags==-1)
    {
        std::perror("F_GETFL error");
        return false ;
    }
    int ret=fcntl(client_fd,F_SETFL,flags|O_NONBLOCK);
    if(ret==-1)
    {
        std::perror("F_SETFL error");
        return false ;
    }
    std::cout<<"set the client_fd:"<<client_fd<<" nonblock success"<<'\n';
    return true;
}
void tcp_server::update_read_write_fd(int client_fd,int flag)//更新最大fd的队列,两种模式一种是插入,一种是删除
{
    {
        std::lock_guard<std::mutex> lg(this->_mutex_max_fd);
        switch(flag)
        {
            case ADD:
            {
                //add
                FD_SET(client_fd,&this->master_read_set);
                if(this->_max_fd.empty()||this->_max_fd.top()<client_fd)
                {
                    this->_max_fd.push(client_fd);
                    std::cout<<"push the _max_fd queue success"<<'\n';
                }
                break;
            }
            case DEL:
            {
                if(FD_ISSET(client_fd,&this->master_read_set))
                {
                    FD_CLR(client_fd,&this->master_read_set);
                    std::cout<<"delete the client_fd:"<<client_fd<<" from the master_read_set success"<<'\n';
                }
                if(FD_ISSET(client_fd,&this->master_write_set))
                {
                    FD_CLR(client_fd,&this->master_write_set);
                }
                if(!this->_max_fd.empty()&&this->_max_fd.top()==client_fd)
                {
                    this->_max_fd.pop();
                    std::cout<<"update the _max_fd success"<<'\n';
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }
}
int tcp_server::get_max_fd()//获得当前最大的fd
{
    {
        std::lock_guard<std::mutex> lg(this->_mutex_max_fd);
        if(!this->_max_fd.empty())
            return this->_max_fd.top();
        return -1;
    }
    return -1;
}
void tcp_server::update_client_map(int client_fd,client_state && client,int flag)//更新client_map
{
    {
        std::lock_guard<std::mutex> lg(this->_mutex_map);
        switch(flag)
        {
            case ADD:
            {
                std::cout<<"have a new client add the client_map"<<'\n';
                //添加到map中
                this->_client_map.emplace(client_fd,client);
                break;
            }
            case DEL:
            {
                std::cout<<"the client :";
                client.client_info_print();
                std::cout<<" have been deleted"<<'\n';
                this->_client_map.erase(client_fd);
                break;
            }
            default:
            {
                break;
            }
        }
    }
}