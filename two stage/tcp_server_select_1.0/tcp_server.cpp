#include "tcp_server.h"
client_state::client_state(std::string ip,uint16_t port)
:_ip(ip)
,_port(port)
,_pending()
,_send_buffer()
,_send_offset(0)
{}
void client_state::print_client_info()//打印客户端的连接信息
{
    std::cout<<this->_ip<<":"<<this->_port;
}

tcp_server::tcp_server(std::string ip,uint16_t port)
:_ip(ip)
,_port(port)
,_listen_fd(-1)
,_stop(true)
,_client_map()
,_init_status(false)
,_max_fd()
{
    //首先创建一个监听的socket
    this->_listen_fd=socket(AF_INET,SOCK_STREAM,0);
    if(this->_listen_fd>=0)
    {
        std::cout<<"listen socket create success"<<std::endl;
        //进行一个地址的转换
        sockaddr_in server_addr;
        server_addr.sin_family=AF_INET;
        server_addr.sin_port=htons(this->_port);//大小端进行一个转换
        int ret=inet_pton(AF_INET,this->_ip.c_str(),&server_addr.sin_addr);
        if(ret>0)
        {
            //地址转换成功,进行一个bind绑定
            int ret2=bind(this->_listen_fd,reinterpret_cast<sockaddr*>(&server_addr),sizeof(server_addr));
            if(ret2==0)
            {
                //绑定成功
                this->_init_status=true;//可以调用开始了
                std::cout<<"bind success"<<std::endl;
            }
            else
            {
                //绑定失败
                int bind_errno=errno;
                close(this->_listen_fd);
                throw std::runtime_error(std::string("bind failure")+std::strerror(bind_errno));
            }    
        }
        else if(ret==0)
        {
            //地址格式不对,转化失败
            close(this->_listen_fd);
            std::cerr<<"server_add address format have error"<<std::endl;
        }
        else
        {
            int add_thransfer_errno=errno;
            //表示转换过程中有错误,关闭listen_fd
            close(this->_listen_fd);
            throw std::runtime_error(std::string("server_addr ip address thransfer failure")+std::strerror(add_thransfer_errno));
        }
    }
    else
    {
        //创建失败,错误码被设置
        int listen_errno=errno;
        throw std::runtime_error(std::string("listen socket create failure")+std::strerror(listen_errno));
    }
}
tcp_server::~tcp_server()//析构函数
{
    this->server_end();
    //回收服务线程
    if(this->_server_thread.joinable())
    {
        this->_server_thread.join();
    }
    //回收client_fd与listen_fd
    {
        std::lock_guard<std::mutex> lg(this->_mutex);
        auto it=this->_client_map.begin();
        while(it!=this->_client_map.end())
        {
            close(it->first);
            it++;
        }
    }
    if(this->_listen_fd>=0)
        close(this->_listen_fd);
}
void tcp_server::start()//服务入口函数
{
    bool start_status=false;
    {
        //进行一个加锁的判断,判断是否可以开始服务了
        std::lock_guard<std::mutex> lg(this->_mutex);
        if(this->_init_status)
        {
            start_status=true;
        }
        this->_init_status=false;
    }
    if(!start_status)
    {
        //表示这个函数已经被调用过了
        std::cerr<<"The function has already been executed"<<std::endl;
        return ;
    }
    else 
    {
        try
        {
            this->_stop.store(false);
            this->_server_thread=std::thread(&tcp_server::server_start,this);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
    }
}
void tcp_server::server_start()//服务开始函数
{
    //进行一个监听
    int ret=listen(this->_listen_fd,10);
    if(ret==0)
    {
        //监听成功
        std::cout<<"listen success"<<std::endl;
        //创建两个fd_set 
        fd_set master_read_set;//这个记录目前所有的fd,监视可以读的fd
        fd_set ready_read_set;//这个表示只读位图结构
        fd_set master_write_set;//这个记录目前所有的fd,监视可以写的fd
        fd_set ready_write_set;//这个表示只读位图结构
        //给监听套接字设置非阻塞
        if(!this->set_nonblock(this->_listen_fd))
            return;
        //初始化
        FD_ZERO(&master_read_set);
        FD_ZERO(&master_write_set);
        //将监听套接字加入到master_read_set中
        if(this->_listen_fd<FD_SETSIZE)
            FD_SET(this->_listen_fd,&master_read_set);
        //当前队列为空,直接入队,不用做额外的判断
        std::cout<<"listen_fd=="<<this->_listen_fd<<"=>入队列"<<std::endl;
        this->_max_fd.push(this->_listen_fd);
        timeval select_time;
        while(!this->_stop)
        {
            //重置位图
            FD_ZERO(&ready_read_set);
            FD_ZERO(&ready_write_set);
            //用select来实现一个io多路复用
            ready_read_set=master_read_set;
            ready_write_set=master_write_set;
            select_time.tv_sec=TV_SEC;
            select_time.tv_usec=TV_USEC;
            //填入最大的fd+1
            int num=select(this->_max_fd.top()+1,&ready_read_set,&ready_write_set,nullptr,&select_time);//阻塞式是监视,当有一个fd是readable 的时候进行一个返回
            if(num>0)
            {
                std::cout<<"select 返回"<<std::endl;
                try
                {
                    this->fd_check_loop(this->_max_fd.top(),master_read_set,ready_read_set,master_write_set,ready_write_set);
                }
                catch(const std::exception& e)
                {
                    std::cerr << e.what() << '\n';
                }
            }
            else if(num==0)
            {
                std::cout<<"select 超时"<<std::endl;
            }
            else
            {
                int select_errno=errno;
                if(select_errno==EINTR)
                    continue;
                throw std::runtime_error(std::string("select errno")+std::strerror(select_errno));
            }
        }
    }
    else
    {
        int listen_errno=errno;
        throw std::runtime_error(std::string("listen failure")+std::strerror(listen_errno));
    }
}
void tcp_server::fd_check_loop(int  max_fd,fd_set& master_read_set,fd_set& ready_read_set,fd_set& master_write_set,fd_set& ready_write_set)//对位图中的文件描述符进行一个遍历检查
{
    //循环的检查是哪个fd被触发了
    for(int i=0;i<=max_fd;++i)
    {
        if(FD_ISSET(i,&master_read_set)&&FD_ISSET(i,&ready_read_set))
        {
            if(this->_listen_fd==i)
            {
                std::cout<<"accept触发"<<std::endl;
                //这个表示此时有连接在全连接队列里
                std::string client_ip;//表示客户端的ip地址
                uint16_t client_port;//表示客户端的port
                sockaddr_in client_add;
                client_add.sin_family=AF_INET;
                socklen_t len=static_cast<socklen_t>(sizeof(client_add));
                int client_fd=accept(this->_listen_fd,reinterpret_cast<sockaddr*>(&client_add),&len);
                if(client_fd>=0)
                {
                    //设置非阻塞
                    if(!this->set_nonblock(client_fd))
                    {
                        //这里表示没有设置非阻塞成功
                        this->update_max_fd(client_fd,master_read_set,master_write_set);
                    }
                    else
                    {
                        //表示接收到了一个新的客户端,进行一个地址转化
                        char buf[16];
                        memset(buf,0,sizeof(buf));
                        const char* thrans_ret=inet_ntop(AF_INET,&client_add.sin_addr,buf,static_cast<socklen_t>(sizeof(buf)));
                        if(thrans_ret==nullptr)
                        {
                            //这个表示地址转换失败
                            int inet_ntop_errno=errno;
                            this->update_max_fd(client_fd,master_read_set,master_write_set);
                            throw std::runtime_error(std::string("inet_ntop failure")+std::strerror(inet_ntop_errno));
                        }
                        else
                        {
                            //转换成功
                            client_ip=thrans_ret;
                            //端口号转换
                            client_port=ntohs(client_add.sin_port);
                            //先判断当前的client_fd是否在合理的范围内
                            if(client_fd<FD_SETSIZE)
                            {
                                FD_SET(client_fd,&master_read_set);
                                //创建client_state
                                client_state client(client_ip,client_port);
                                this->_client_map.insert(std::make_pair(client_fd,client));
                                std::cout<<"new link socket:";
                                client.print_client_info();
                                std::cout<<std::endl;
                                //与当前栈顶元素进行一个判断看谁大谁小
                                if(!this->_max_fd.empty()&&this->_max_fd.top()<client_fd)
                                {
                                    this->_max_fd.push(client_fd);
                                    std::cout<<"fd=="<<client_fd<<"=>入队列"<<std::endl;
                                }
                                std::cout<<"now max_fd:"<<this->_max_fd.top()<<std::endl;
                            }
                           else
                           {
                                std::cerr<<"the client_fd :"<<client_fd<<" more than the FD_SETSIZE"<<std::endl;
                                //关闭这个新的client_fd文件描述符
                                this->update_max_fd(client_fd,master_read_set,master_write_set);
                           }
                        }
                    } 
                }
                else
                {
                    int accept_errno=errno;
                    throw std::runtime_error(std::string("accept error")+std::strerror(accept_errno));
                }
            }
            else 
            {
                //这里表示通信客户端fd
                this->communication(i,master_read_set,master_write_set);
            }
        }
        if(FD_ISSET(i,&master_write_set)&&FD_ISSET(i,&ready_write_set))
        {
            std::vector<std::string> commands;
            //写操作
            this->handle_commands(i,commands,master_write_set);
        }
    }
}
bool tcp_server::set_nonblock(int client_fd)//这个函数表示给client_fd设置非阻塞处理
{
    int flags=fcntl(client_fd,F_GETFL,0);
    if(flags==-1)
    {
        std::perror("fcntl failure");
        return false;
    }
    int ret=fcntl(client_fd,F_SETFL,flags|O_NONBLOCK);//设置非阻塞
    return ret==0?true:false;
}
void tcp_server::server_end()//服务结束函数
{
    bool server_end_status=false;
    {
        std::lock_guard<std::mutex> lg(this->_mutex);
        bool stop_status=this->_stop.exchange(true); 
        if(!stop_status)
        {
            //这个表示第一个执行end
            server_end_status=true;
        }
    }
    if(!server_end_status)
    {
        std::cerr<<"server_end repeatedly executed"<<std::endl;
    }   
}
void tcp_server::communication(int client_fd,fd_set& master_read_set,fd_set& master_write_set)//与客户端进行一个沟通的函数
{
    std::cout<<"communicating with fd: "<<client_fd<<std::endl;
    //与client_fd进行一个通信
    char buf[1024];
    size_t size=sizeof(buf);
    memset(buf,0,size);
    //表示待处理的客户端
    auto it=this->_client_map.find(client_fd);
    std::vector<std::string> commands;//表示解析出来的命令
    while(true)
    {
        ssize_t len=recv(client_fd,buf,size,0);
        if(len>0)
        {
            //往pending里面追加数据
            it->second._pending.append(buf,static_cast<size_t>(len));
            //然后对pending中的数据进行一个命令解析
            try
            {
                //单条命令可能过长
                this->get_all_commands(it->second._pending,commands);
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
                this->update_max_fd(client_fd,master_read_set,master_write_set);
                break;
            }
            //解析完成之后,我们判断是否有解析的命令
            if(commands.size()>0)
            {
                //调优处理命令的函数
                try
                {
                   this->handle_commands(client_fd,commands,master_write_set);
                   //对命令进行一个清理
                   commands.clear();
                }
                catch(const std::exception& e)
                {
                    std::cerr << e.what() << '\n';
                    //将_max_fd进行一个更新,跟栈顶元素进行一个判断是否为最大的fd
                    this->update_max_fd(client_fd,master_read_set,master_write_set);
                    break;
                }
            }
        }
        else if(len==0)
        {
            //对方优雅的关闭了连接
            //将_max_fd进行一个更新
           this->update_max_fd(client_fd,master_read_set,master_write_set);
            break;
        }
        else 
        {
            //这里表示有错误
            //首先要么是其他信号中断
            if(errno==EINTR)
                continue;
            if(errno==EWOULDBLOCK||errno==EAGAIN)
                break;
            std::perror("recv failure");
            this->update_max_fd(client_fd,master_read_set,master_write_set);
            break;
        }
    }
}
void tcp_server::update_max_fd(int client_fd,fd_set& master_read_set,fd_set& master_write_set)//对主动断开连接的以及被动断开连接的fd进行一个判断
{
    if(!this->_max_fd.empty()&&this->_max_fd.top()==client_fd)
    {
        //栈顶元素出栈
        this->_max_fd.pop();
    }
    if(client_fd<FD_SETSIZE)
    {
        if(FD_ISSET(client_fd,&master_read_set))
            FD_CLR(client_fd,&master_read_set);
        if(FD_ISSET(client_fd,&master_write_set))
            FD_CLR(client_fd,&master_write_set);
    }
    //将map中的client_fd同样进行一次更新
    {
        std::lock_guard<std::mutex> lg(this->_mutex);
        auto it=this->_client_map.find(client_fd);
        if(it!=this->_client_map.end())
            this->_client_map.erase(client_fd);
    }
    close(client_fd);
    std::cout<<"close fd: "<<client_fd<<std::endl;
}
void tcp_server::get_all_commands(std::string& pending,std::vector<std::string> & commands)//对接收的数据进行命令解析函数
{
    for(size_t pos=pending.find('\n');pos!=std::string::npos;pos=pending.find('\n'))
    {
        std::string command=pending.substr(0,pos);
        //对获得命令进行一个判断
        if(command.size()==0||(command.size()==1&&command.back()=='\r'))
        {
            pending.erase(0,pos+1);
            continue;
        }
        if(command.back()=='\r')
        {
            command.pop_back();
        }
        if(command.size()>SIGNAL_COMMAND_LENGTH)
        {
            throw std::runtime_error("signal command length more than the max length");
        }
        else
        {
            commands.push_back(command);
            pending.erase(0,pos+1);
        }
    }
    //对剩余的命令同样做一个判断
    size_t len=pending.size();
    if(!pending.empty()&&pending.back()=='\r')
        --len;
    if(len>SIGNAL_COMMAND_LENGTH)
        throw std::runtime_error("signal command length more than the max length");
}
void tcp_server::handle_commands(int client_fd,std::vector<std::string> & commands,fd_set& master_write_set)//对命令进行一个回复的函数
{
    std::map<int,client_state>::iterator it;
    {
        std::lock_guard<std::mutex> lg(this->_mutex);
        it=this->_client_map.find(client_fd);
    }
    if(commands.size()>0)
    {
        for(auto & e: commands)
        {
            e+='\n';
            //将发送的数据存入send_buffer
            it->second._send_buffer+=e;
        }
    }
    //已经发送的数据
    // it->second._send_offset=0;
    ssize_t len=0;//表示每次循环发送的数据
    //发送每一条的命令
    while(it->second._send_offset<it->second._send_buffer.size())
    {
        len=send(client_fd,it->second._send_buffer.c_str()+it->second._send_offset,it->second._send_buffer.size()-it->second._send_offset,MSG_NOSIGNAL);
        if(len>0)
        {
            it->second._send_offset+=len;
            std::cout<<"tcp_server send "<<len<<" Bytes to :";
            it->second.print_client_info();
            std::cout<<std::endl;
        }
        else if(len==0)
        {
            throw std::runtime_error("send 0 Byte");
        }
        else
        {
            if(errno==EINTR)
                continue;
            if(errno==EWOULDBLOCK||errno==EAGAIN)
            {
                //表示发送缓冲区已经满了,这个时候可以处理发送缓冲区过大的问题,首先偏移量表示已经发送的字节数
                if(it->second._send_offset<it->second._send_buffer.size())
                {
                    //加入writeable的监视位图中
                    if(client_fd<FD_SETSIZE)
                        FD_SET(client_fd,&master_write_set);
                    return ;
                }
                break;
            }
            throw std::runtime_error(std::string("send failure")+std::strerror(errno));
        }
    }
    //如果此时保留的数据全都被发送了,那么这个时候就不需要监视这个fd了,直接移除就行了
    if(it->second._send_buffer.size()==it->second._send_offset)
    {
        //将这个判断移动到这里,表示一个正常的send中没有EAGAIN,那么也就这个fd就不会存在于master_write_set中,所以要想
        //正确的进行一个清理,只需要判断偏移量是否等于send_buffer.size()就行了
        if(FD_ISSET(client_fd,&master_write_set))
            FD_CLR(client_fd,&master_write_set);
        //同时清理client_state中的send_buffer以及send_offset
        it->second._send_buffer.clear();
        it->second._send_offset=0;
    }
}