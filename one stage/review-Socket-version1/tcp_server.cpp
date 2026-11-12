#include "tcp_server.h"

client::client(std::string client_ip,uint16_t client_port)//缺省构造函数
:_client_ip(client_ip)
,_client_port(client_port)
{

}
void client::client_info_print()//来打印客户端的信息
{
    std::cout<<"The client socket is :"<<this->_client_ip<<":"<<this->_client_port;
}

//缺省构造函数来构建服务器
tcp_server::tcp_server(std::string server_ip,uint16_t server_port)
:_listen_fd(-1)
,_server_ip(server_ip)
,_server_port(server_port)
,_server_init_status(false)
,_server_accept_status(false)
{
    //这里不处理ip地址以及端口号的正确或者错误,这种判断的逻辑需要交给调用方来进行一个判断
    int ret=socket(AF_INET,SOCK_STREAM,0);
    if(ret>=0)
    {
        //返回对应的文件描述符fd
        this->_listen_fd=ret;
        //创建ipv4的地址结构体
        sockaddr_in server_addr;
        server_addr.sin_family=AF_INET;
        server_addr.sin_port=htons(this->_server_port);//进行网络主机的端口大小端字节序的转换
        int ret2=inet_pton(AF_INET,this->_server_ip.c_str(),&server_addr.sin_addr);
        if(ret2==1)
        {
            //地址转换成功
            //进行ip:port的绑定
            int ret3=bind(this->_listen_fd,reinterpret_cast<sockaddr*>(&server_addr),sizeof(server_addr));
            if(ret3==0)
            {
                this->_server_init_status=true;
                //绑定成功
                std::cout<<"tcp_server bind successful"<<std::endl;
            }
            else
            {
                //绑定错误
                int bind_errno=errno;
                close(this->_listen_fd);
                this->_listen_fd=-1;
                throw std::runtime_error(std::string("tcp_server bind error")+std::strerror(bind_errno));
            }
            
        }
        else if(ret2==0)
        {
            //证明这个时候传入的地址格式是不对的,所以转化失败,这个时候不设置全局的错误码
             close(this->_listen_fd);
             this->_listen_fd=-1;
             throw std::runtime_error("server_addr's ip exists format error");
        }
        else
        {
            //传入的协议的类型是不对的
            int af_errno=errno;
            close(this->_listen_fd);
            this->_listen_fd=-1;
            throw std::runtime_error(std::string("does not contain a valid address family")+std::strerror(af_errno));
        }
    }
    else
    {
        //返回-1,然后错误码也被设置
        int sock_errno=errno;
        throw std::runtime_error(std::string("socket create error :")+std::strerror(sock_errno));
    }
}
tcp_server::~tcp_server()
{
    //对资源进行一个释放,防止出现没有释放的资源
    //首先再调用一次shutdown_client的操作,防止服务端没有执行
    this->shutdown_clientfd_rdwr();
    //然后等待服务线程的结束
    if(this->_server_thread.joinable())
    {
        this->_server_thread.join();
    }
    //服务端线程结束之后就可以对监听文件描述符进行一个回收了
    if(this->_listen_fd>=0)
    {
        close(this->_listen_fd);
    }
    {
        //对client_map中没有回收的fd进行一个关闭
        std::lock_guard<std::mutex> _lk(this->_map_mutex);
        for(auto& client:this->_cl_info_map)
        {
            close(client.first);
        }
    }
}
void tcp_server::start()//用一个线程来执行服务器的accept过程
{
    //如果服务线程已经被创建了那么就不能多次调用start函数来创建服务线程,会引发terminate杀死整个进程
    if(this->_server_thread.joinable())
    {
        throw std::logic_error("tcp_server has already been started");
    }
    else
    {
        //开始执行服务器的监听以及accept的操作,首先看服务器的初始化的状态
        if(this->_server_init_status)
        {
            this->_server_accept_status.store(true);//原子操作
            try
            {
                //开始创建服务端线程,来调用服务器运行的函数
                this->_server_thread=std::thread(&tcp_server::server_start,this);
                std::cout<<"the server_thread is running"<<std::endl;
            }
            catch(const std::exception& e)
            {
                this->_server_accept_status.store(false);
                throw;
            }
        }
        else
        {
            throw std::runtime_error("tcp_server init not successful");
        }
    }
}
void tcp_server::server_end()//将服务的监听accept过程进行停止
{
    //结束服务继续accept新用户的操作
    this->shutdown_clientfd_rdwr();
    std::cout<<"main 端输入 quit 了"<<std::endl;
}
void tcp_server::server_start()//服务执行的函数,利用一个线程来调用这个过程
{
    //服务端可以进行一个监听了
    int ret=listen(this->_listen_fd,10);//给监听listen设置10,表示在队列中一次可以装10个没有被处理的客户端
    if(ret==0)
    {
        //设置超时机制
        timeval accept_timeout{};
        accept_timeout.tv_sec=ACCEPT_TIMEOUT;
        accept_timeout.tv_usec=0;
        if(!this->set_fdrev_timeout(this->_listen_fd,accept_timeout))
        {
            //这里我们设置了超时的设置,但是如果没有设置成功的话,那么下面的while中的this->_server_accpet_status就不会因为accept的超时
            //而反复的检查this->_server_accept_status.load()的值,服务端就会被阻塞在accept中
            this->_server_accept_status.store(false);
            return ;
        }
        //走到这里就表示可以进行accept了
        while(this->_server_accept_status.load())
        {
            sockaddr_in client_addr;
            unsigned int len=static_cast<unsigned int>(sizeof(client_addr));
            memset(&client_addr,0,sizeof(client_addr));
            int client_fd=accept(this->_listen_fd,reinterpret_cast<sockaddr*>(&client_addr),&len);
            if(client_fd>=0)
            {
                //成功返回对接客户端的文件描述符
                //因为这里的client_fd会继承accept设置的超时设置,在recv时也会进行设置对应的超时,这里需要还原
                timeval recv_timeout{};
                recv_timeout.tv_sec=0;
                recv_timeout.tv_usec=0;
                if(!this->set_fdrev_timeout(client_fd,recv_timeout))
                {
                    close(client_fd);
                    continue;
                }
                //然后将客户端的信息进行一个存储,为了后续多线程同时访问map操作资源竞争,所以要加锁,利用RAII思想
                //首先读取客户端的ip:port信息
                char buf[16];
                memset(buf,0,sizeof(buf));
                const char* client_ip=inet_ntop(AF_INET,&client_addr.sin_addr,buf,sizeof(buf));
                if(client_ip)
                {
                    bool worker_created=false;//这个字段表示是否创建worker子线程
                    //先判断这个期间main是否已经输入了quit
                    {
                        //加锁,防止quit的时候用户调用server_stop来对_cli_info_map中的客户端进行唤醒,也就是少了当前
                        //对客户端的唤醒,不然这个客户端可能会被永久阻塞在recv
                        std::lock_guard<std::mutex> _lk(this->_map_mutex);
                        //检查此时_server_accept_status的状态已经被修改了
                        if(!this->_server_accept_status.load())
                        {
                            close(client_fd);
                            break;
                        }
                        else
                        {
                            //非空,证明已经接收到了一个点分十进制的ip地址
                            uint16_t client_port=ntohs(client_addr.sin_port);
                            //创建client来接收这些信息
                            client new_client(client_ip,client_port);
                            //打印一个连接信息
                            std::cout<<"there is a new client linking"<<std::endl;
                            new_client.client_info_print();
                            std::cout<<std::endl;
                            //插入client_map
                            this->_cl_info_map.insert(std::make_pair(client_fd,new_client));
                            //然后开始创建子线程调用communication进行一个通信
                            //加入vector<std::thread>容器里面,直接在容器里面进行构建子线程
                            try
                            {
                                this->_cl_thread_vector.emplace_back(&tcp_server::communication,this,client_fd);
                                std::cout<<"fd:"<<client_fd<<"<-> create a new client thread"<<std::endl;
                                worker_created=true;
                            }
                            catch(const std::exception& e)
                            {
                                std::cerr << e.what() << '\n';
                                //客户端的线程创建失败
                                close(client_fd);
                                this->_cl_info_map.erase(client_fd);
                            }
                        }
                    }
                    if(!worker_created)
                    {
                        //表示创建失败
                        break;
                    }
                }
                else
                {
                    std::perror("client ip thransfer error");
                    //一般的情况都是在communication里面关闭client_fd,这里直接关闭,然后退出循环
                    close(client_fd);
                    break;
                }
            }
            else
            {
                //这里表示返回失败,如果后续给他加上一个超时的作用就可以返回一些超时的信息,比如
                if(errno==EINTR)
                    continue;
                else if(errno==EAGAIN||errno==EWOULDBLOCK)
                {
                    std::cout<<"time out"<<std::endl;
                     continue;
                }
                //否则是其他的错误码
                std::perror("accept have a other error");
                break;//退出循环
            }
        }
    }
    else
    {
        int listen_errno=errno;
        std::cerr<<"listen have a error"<<std::strerror(listen_errno)<<std::endl;
        this->_server_accept_status.store(false);
        return;
    }
    std::cout<<"shut client 操作"<<std::endl;
    this->shutdown_clientfd_rdwr();
    //如果服务线程退出了accept了这个时候要等待在服务进程中创建的子进程的退出
    this->join_client_thread();
}
void tcp_server::communication(int client_fd)//一客户端对应一个线程,用一个线程来对接实际连接的客户端
{
    //开始与client进行一个通信
    //创建一个缓冲区来接收信息
    char buffer[1024];
    size_t length=sizeof(buffer);
    memset(buffer,0,length);
    std::string pending;//命令的暂存区,从缓冲区里面拿命令拿到这里
    std::vector<std::string> commands;//存放解析的完整命令
    ssize_t len=0;
    while(true)
    {
        len = recv(client_fd,buffer,length,0);
        if(len>0)
        {
            //追加内容
            pending.append(buffer,static_cast<size_t>(len));
            //然后获取pending中的所有的命令,存放到commands里面
            try
            {
                this->get_all_commands(pending,commands);
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
                break;
            }
            //判断commands里面是否有命令
            if(commands.size()>0)
            {
                //然后开始对命令进行一个响应
                try
                {
                    this->process_echo(client_fd,commands);
                    //对命令进行一个清理
                    commands.clear();
                }
                catch(const std::exception& e)
                {
                    std::cerr << e.what() << '\n';
                    break;
                } 
            }
            else
            {
                //用户没有输入命令,继续接收信息
                continue;
            }
        }
        else if(len==0)
        {
            //对方优雅的关闭的连接,四次挥手
            break;
        }
        else
        {
            //对方强制关闭了连接
            if(errno==ECONNRESET)
            {
                std::perror("Client recv encountered forced connection closure");
                break;
            }
            if(errno==EINTR)
                continue;
            break;
        }
    }
    {
        std::lock_guard<std::mutex> _lk(this->_map_mutex);
        this->_cl_info_map.erase(client_fd);
    }
    //关闭客户端的文件描述符
    close(client_fd);
}
void tcp_server::process_echo(int client_fd,std::vector<std::string>& commands)//对客户端发送的信息进行回复
{
    ssize_t len=0;
    size_t sent=0;
    //开始对命令进行一个回复
    for(auto & e:commands)
    {
        len=0;
        sent=0;
        e+='\n';
        while(sent<e.size())
        {
            len=send(client_fd,e.c_str()+sent,e.size()-sent,MSG_NOSIGNAL);
            if(len>0)
            {
                sent+=len;
                {
                    std::lock_guard<std::mutex> _lk(this->_map_mutex);
                    auto client=this->_cl_info_map.find(client_fd);
                    std::cout<<"The server has sent "<<len<<" bytes to the client ip:";
                    client->second.client_info_print();
                    std::cout<<std::endl;
                }
                continue;
            }
            else if(len==0)
            {
               throw std::runtime_error("The server has sent 0 bytes");
            }
            else
            {
                if(errno==EINTR)
                    continue;
                int send_errno=errno;
                throw std::runtime_error(std::string("send have error")+std::strerror(send_errno)); 
            }
        }
    }
}
void tcp_server::get_all_commands(std::string & pending,std::vector<std::string>& commands)//从pending中获取完整的命令
{
    //首先解析所有的命令
    for(size_t pos=pending.find('\n');pos!=std::string::npos;pos=pending.find('\n'))
    {
        std::string command=pending.substr(0,pos);
        //先判断是否为空
        if(command.size()==0||(command.size()==1&&command.back()=='\r'))
        {
            //找到了"\n"或者"\r\n",这种直接从pending中删掉然后继续接收
            pending.erase(0,pos+1);
            continue;
        }
        else
        {
            //找到了一条命令,但是要判断这条命令是否满足长度的要求
            if(command.back()=='\r')
            {
                command.pop_back();//删除最后一个元素
            }
            //判断长度是否符合要求
            if(command.size()>MAX_COMMAND)
            {
                throw std::logic_error("command.size more than the MAX_COMMAND");
            }
            //否则加入commands中去
            commands.push_back(std::move(command));
            //然后删除pending中的command命令
            pending.erase(0,pos+1);
        }
    }
    //然后就是剩余的命令了,这些是没有找到'\n',但是也需要在这一步判断剩余的字符的长度有没有超过限制
    size_t length=pending.size();
    if(!pending.empty()&&pending.back()=='\r')
    {
        --length;
    }
    if(length>MAX_COMMAND)
    {
        throw std::logic_error("command.size more than the MAX_COMMAND");
    }
}
bool tcp_server::set_fdrev_timeout(const int fd,timeval timeout)//给文件描述的接收设置超时机制,如果timeout不为0,超时了就设置全局错误码
{
    int ret=setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
    if(ret==0)
    {
        //设置成功返回true
        return true;
    }
    else
    {
        std::perror("fdrev_timeout set error");
        //设置失败返回false
        return false;
    }
}
void tcp_server::shutdown_clientfd_rdwr()//将所有的被阻塞在recv的客户端进行一个接收端关闭,这样就可以返回了
{
    //首先可以先检查这个函数是否已经被调用过了,如果没有被调用就执行shutdown系统调用
    bool status=this->_server_accept_status.exchange(false);
    if(!status)
    {
        //表示已经被调用过了
        return;
    }
    else
    {
        //第一次调用shutdown_clientfd_rdwr
        std::lock_guard<std::mutex> _lk(this->_map_mutex);
        for(auto& client:this->_cl_info_map)
        {
            shutdown(client.first,SHUT_RDWR);
        }
    }
}
void tcp_server::join_client_thread()//将所有的客户端线程进行一个join,等待客户端线程返回
{
    for(auto& client:this->_cl_thread_vector)
    {
        if(client.joinable())
        {
            client.join();
        }
    }
}
