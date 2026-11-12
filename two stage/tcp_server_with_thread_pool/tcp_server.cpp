#include "tcp_server.h"
client_addr::client_addr(std::string ip,uint16_t port)
:_client_ip(ip)
,_client_port(port)
{
}
//打印信息函数
void client_addr::print_client()
{
    std::cout<<this->_client_ip<<":"<<this->_client_port;
}

tcp_server::tcp_server(std::string ip,uint16_t port)//缺省构造函数,包含了默认构造与有参构造
:_ip(ip)
,_port(port)
,_listen_fd(-1)
,_thread_pool(THREAD_NUM)
,_client_map()
,_stop(true)
,_init_status(false)
,_mutex()
{
    //根据ip以及端口号来初始化对应的服务端
    this->_listen_fd=socket(AF_INET,SOCK_STREAM,0);
    if(this->_listen_fd>=0)
    {
        std::cout<<"socket created"<<std::endl;
        //然后开始绑定ip以及端口号
        sockaddr_in server_addr;
        server_addr.sin_family=AF_INET;
        //第一步将ip地址的点分十进制转成整型数字
        try
        {
           int ret=inet_pton(AF_INET,this->_ip.c_str(),&server_addr.sin_addr);
           if(ret==-1)
           {
                //这个时候表示转换失败了,然后进行一个抛异常处理
                int inte_errno=errno;
                throw std::runtime_error(std::string("ip address thransfer failure")+std::strerror(inte_errno));
           }
        }
        catch(...)
        {
            //先关闭文件描述符
            if(this->_listen_fd>=0)
            {
                close(this->_listen_fd);
            }
            throw ;
        }
        //转换成功,将端口号的主机序转成网络序
        server_addr.sin_port=htons(this->_port);
        int ret2=bind(this->_listen_fd,reinterpret_cast<const sockaddr*>(&server_addr),sizeof(server_addr));
        if(ret2==0)
        {
            //绑定成功
            std::cout<<"bind success"<<std::endl;
            //设置初始化的标志
            this->_init_status=true;//这样就可以开始服务了
        }   
        else
        {
            //绑定失败
            int bind_errno=errno;
            if(this->_listen_fd>=0)
            {
                close(this->_listen_fd);
            }
            throw std::runtime_error(std::string("bind error")+std::strerror(bind_errno));
        }
    }
    else
    {
        //走到这里证明创建失败
        int sock_errno=errno;
        throw std::runtime_error(std::string("socket create failure")+std::strerror(sock_errno));
    }
}
tcp_server::~tcp_server()//析构函数
{
    {
        std::lock_guard<std::mutex> lg(this->_mutex);
        this->_stop.store(true);
    }
    std::cout<<"tcp_server join start"<<std::endl;
    //然后这个时候确保所有已经获得提交资格的任务进行一个提交
    if(this->_server_thread.joinable())
    {
        this->_server_thread.join();
    }
    std::cout<<"tcp_server join end"<<std::endl;
    std::cout<<"client_fd shutdown start"<<std::endl;
    //首先关闭所有的client_fd,因为读或者写而阻塞的文件描述符,这样对应的线程池中的线程就不会被阻塞
    {
        std::lock_guard<std::mutex> lg(this->_mutex);
        for(auto & e: this->_client_map)
        {
            shutdown(e.first,SHUT_RDWR);
        }
    }
    std::cout<<"client_fd shutdown end"<<std::endl;
    //线程池停止接收新任务,等待所有的线程池里的线程全部完成工作
    this->_thread_pool.stop_thread_pool();
    std::cout<<"client_map delete start"<<std::endl;
    //删除map中所有的客户端的文件描述符
    {
        std::lock_guard<std::mutex> lg(this->_mutex);
        auto it=this->_client_map.begin();
        while(it!=this->_client_map.end())
        {
            close(it->first);
            it=this->_client_map.erase(it);
        }
    }
    std::cout<<"client_map delete end"<<std::endl;
    std::cout<<"listen_fd close start"<<std::endl;
    //关闭listen_fd
    if(this->_listen_fd>=0)
        close(this->_listen_fd);
    std::cout<<"listen_fd close end"<<std::endl;
}
void tcp_server::start()//开始服务,执行服务线程
{
    //防止重复调用start成员函数
    bool start_status=false;
    {
        std::lock_guard<std::mutex> lg(this->_mutex);
        if(this->_init_status)
        {
            start_status=true;
            this->_init_status=false;
        }  
    }
    if(start_status)
    {
        //将状态设置为false
        this->_stop.store(false);
        //创建一个服务线程
        try
        {
           this->_server_thread=std::thread(&tcp_server::start_server,this);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            if(this->_server_thread.joinable())
            {
                this->_server_thread.join();
            }
            return ;
        }  
    }
    else
    {
        std::cerr<<"The service thread has been created"<<std::endl;
    }
}
void tcp_server::start_server()//开始提供服务
{ 
    int ret=listen(this->_listen_fd,10);
    if(ret==0)
    {
        //监听成功
        std::cout<<"listen success"<<std::endl;
        //给监听的套接字设置超时时间
        timeval _time;
        _time.tv_sec=TV_SEC;
        _time.tv_usec=TV_USEC;
        if(!this->set_fdrecv_timeout(this->_listen_fd,_time))
        {
            return;
        }
        while(!this->_stop.load())
        {
            sockaddr_in cli_addr;
            unsigned int len=static_cast<unsigned int>(sizeof(cli_addr));
            //开始接收新的连接
            int client_fd=accept(this->_listen_fd,reinterpret_cast<sockaddr*>(&cli_addr),&len);
            //获得对应的端口号
            if(client_fd>=0)
            {
                //表示接收成功,设置超时限制
                timeval _time2;
                _time2.tv_sec=0;
                _time2.tv_usec=0;
                if(!this->set_fdrecv_timeout(client_fd,_time2))
                {
                    close(client_fd);
                    break;
                }
                //否则获得ip以及port的值
                char buff[16];
                uint32_t len=sizeof(buff);
                memset(buff,0,len);
                const char * client_ip=inet_ntop(AF_INET,&cli_addr.sin_addr,buff,len);
                if(client_ip==nullptr)
                {
                    //关闭这个文件描述符,然后返回
                    close(client_fd);
                    break;
                }
                //端口号进行一个转换
                uint16_t client_port=ntohs(cli_addr.sin_port);
                client_addr _client(client_ip,client_port);
                //将通信任务放到线程池里面
                auto task=std::bind(&tcp_server::communication,this,client_fd);
                bool _stop_status;
                bool set_status;
                //先放存入我们的数据
                {
                    std::lock_guard<std::mutex> lg(this->_mutex);
                    //在map中加入这个客户端
                    this->_client_map.insert(std::make_pair(client_fd,_client));
                    _stop_status=this->_stop.load();
                    if(!_stop_status)
                    {
                        //表示服务器此时没有停止
                        set_status=true;
                    }
                    else
                    {
                        set_status=false;
                    }
                }
                //然后将任务放到我们的线程池中
                if(set_status)
                {
                    bool ret=this->_thread_pool.set_task(task);
                    if(!_stop_status&&ret)
                    {
                        //这个时候表示成功
                        {
                            std::cout<<"new client: ";
                            _client.print_client();
                            std::cout<<std::endl;
                        }
                        continue;
                    }
                    else if(!_stop_status&&!ret)
                    {
                        {
                            std::lock_guard<std::mutex> lg(this->_mutex);
                            //在map中加入这个客户端
                            this->_client_map.erase(client_fd); 
                        }
                        //这次虽然连接上了,但是已经超出了服务器的承受的范围,线程池里面的线程已经忙不过来了,给客户端发个通知
                        std::string reply("We are currently unable to process your connection request. Please try again later");
                        this->server_thread_notification(std::move(reply),client_fd,std::move(_client));
                        close(client_fd);
                        continue;
                    }
                }
                else 
                {
                    {
                        std::lock_guard<std::mutex> lg(this->_mutex);
                        //在map中加入这个客户端
                        this->_client_map.erase(client_fd); 
                    }
                    //这次表示服务器已经进行停止工作了,不再接受新的连接了
                    std::string reply("The server is currently shut down and unable to connect. Please try again later");
                    this->server_thread_notification(std::move(reply),client_fd,std::move(_client));
                    close(client_fd);
                    break;
                }
            }
            else
            {
                if(errno == EAGAIN||errno==EWOULDBLOCK)
                {
                     std::cout<<"Currently, there are no users available"<<std::endl;
                     continue;
                }   
                std::perror("accept failure");
                break;
            }
        }
    }
    else
    {
        std::perror("listen failure");
    }
}
void tcp_server::server_thread_notification(std::string&& reply,int client_fd,client_addr && client)//这个函数表示主线程告知客户端,我虽然已经给你连上了但是我线程池的任务队列已经满了,不能再处理你的请求了
{
    reply+='\n';
    ssize_t len=0;
    size_t sent=0;
    size_t size=reply.size();
    while(sent<size)
    {
        len=send(client_fd,reply.c_str()+sent,size-sent,MSG_NOSIGNAL);
        if(len>0)
        {
            sent+=len;
            std::cout<<"server -> ";
            client.print_client();
            std::cout<<len<<"Bytes"<<std::endl;
        }
        else if(len==0)
        {
            std::cerr<<"send 0 Byte"<<std::endl;
            return ;
        }
        else
        {
            if(errno==EINTR)
                continue;
            std::perror("There are other errors");
            return ;
        }
    } 
}
bool tcp_server::set_fdrecv_timeout(const int& _fd,const timeval time)//给文件描述符的接收缓冲区来设置对应的超时策略
{
    int ret=setsockopt(_fd,SOL_SOCKET,SO_RCVTIMEO,&time,sizeof(time));
    if(ret==0)
    {
        return true;
    }
    else
    {
        std::cerr<<"timeout set failure"<<std::endl;
        return false;
    }
}
void tcp_server::stop_server()//关闭提供的服务
{
    bool status;
    {
        std::lock_guard<std::mutex> lg(this->_mutex);
        status=this->_stop.exchange(true);
    }
    if(status==true)
    {
        std::cerr<<"The stop operation has been repeatedly executed"<<std::endl;
        return ;
    }
}
void tcp_server::communication(int client_fd)//与client_fd进行一个通信
{
    //与客户端进行一个通信
    char buffer[1024];//缓冲区大小
    size_t size=sizeof(buffer);
    client_addr client("",0);
    memset(buffer,0,size);
    {
        std::lock_guard<std::mutex> lg(this->_mutex);
        client=this->_client_map.find(client_fd)->second;
    }
    std::string pending;//这个表示接收命令的字符串
    std::vector<std::string> commands;//表示存储解析出来的命令的容器
    while(true)
    {
        int ret=recv(client_fd,buffer,size,0);
        if(ret>0)
        {
            pending.append(buffer,ret);
            //下面就可以调用get_all_commands(),然后调用handle_commands
            try
            {
               this->get_all_commands(pending,commands);
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
                break;
            }
            //然后判断是否有命令
            if(commands.size()>0)
            {
                //处理命令
                this->handle_commands(client_fd,commands,client);
                //对已经回复的命令进行一个清理
                commands.clear();
            }
            else
            {
                //没有解析出来命令
                continue;
            }
        }
        else if(ret==0)
        {
            //表示对方优雅的关闭
            client.print_client();
            std::cout<<"close this link"<<std::endl;
            break;
        }
        else
        {
            if(errno==EINTR)
                continue;
            if(errno==ECONNRESET)
                break;
            break;
        }
    }
    //通信结束,要关闭这个文件描述符
    {
        std::lock_guard<std::mutex> lg(this->_mutex);
        //移除map中的client的信息
        this->_client_map.erase(client_fd);
    }
    //然后关闭文件描述符
    close(client_fd);
}
void tcp_server::get_all_commands(std::string& pending,std::vector<std::string> & commands)//将pending中的字符串进行一个解析,将命令解析到commands中
{
    //进行一个命令的解析
    for(size_t pos=pending.find('\n');pos!=std::string::npos;pos=pending.find('\n'))
    {
        std::string command=pending.substr(0,pos);
        //首先判断command是否为空串,对标的是"\n"
        if(command.size()==0)
        {
            pending.erase(0,pos+1);
            continue;
        }
        //这种表示"\r\n"这种命令,这种也是无效的
        if(command.size()==1&&command.back()=='\r')
        {
            pending.erase(0,pos+1);
            continue;
        }
        else
        {
            //判断长度是否超出命令的限制
            if(command.back()=='\r')
            {
                //去除\r,然后判断长度
                command.pop_back();
            }
            //判断是否大于最大命令长度的限制
            if(command.size()>SIGNAL_COMMAND_MAX_LENGTH)
            {
                throw std::runtime_error("signal command length more than the max_length");
            }
            //否则可以插入命令
            commands.push_back(command);
            pending.erase(0,pos+1);
        }
    }
    //判断剩余的命令
    size_t len=pending.size();
    if(!pending.empty()&&pending.back()=='\r')
    {
        --len;
    }
    if(len>SIGNAL_COMMAND_MAX_LENGTH)
    {
        throw std::runtime_error("signal command length more than the max_length");
    }
}
bool tcp_server::handle_commands(int client_fd,std::vector<std::string> & commands,client_addr client)//对commands中的命令进行一个处理
{
    //对客户端进行一个回复
    for(auto &command:commands)
    {
        ssize_t len =0;
        size_t sent=0;
        //给每条命令加一个'\n'
        command+='\n';
        while(sent<command.size())
        {
            len=send(client_fd,command.c_str()+sent,command.size()-sent,MSG_NOSIGNAL);
            if(len>0)
            {
                sent+=len;
                std::cout<<"server -> ";
                client.print_client();
                std::cout<<len<<"Bytes"<<std::endl;
            }
            else if(len==0)
            {
                std::cerr<<"send 0 Byte"<<std::endl;
                return false;
            }
            else
            {
                if(errno==EINTR)
                    continue;
                std::perror("There are other errors");
                return false;
            }
        }
    }
    return true;
}