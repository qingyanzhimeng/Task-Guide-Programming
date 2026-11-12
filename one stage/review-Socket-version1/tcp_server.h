#pragma once
#include<iostream>
#include<string>
#include<sys/types.h>
#include<sys/socket.h>
#include<thread>
#include<map>
#include<vector>
#include<errno.h>
#include<cstdio>
#include<cstring>
#include<netinet/in.h>
#include<stdexcept>
#include<arpa/inet.h>
#include<atomic>
#include <system_error>
#include<unistd.h>
#include<sys/time.h>
#include<mutex>
#define ACCEPT_TIMEOUT 5
#define MAX_COMMAND 1024 //表示单条命令的最长的长度 应用层的协议先用简单的'\n'来进行一个区分
class client
{
    public:
    client(std::string client_ip,uint16_t client_port);//缺省构造函数
    void client_info_print();//来打印客户端的信息
    private:
    std::string _client_ip;
    uint16_t _client_port;
};

//定义一个tcp_server类,用来接受来自客户端的连接,这个服务器设计的是一线程一客户端,是中间review阶段的代码
class tcp_server
{
public:
//缺省构造函数来构建服务器
tcp_server(std::string server_ip="0.0.0.0",uint16_t server_port=8080);
tcp_server(tcp_server& tcp_server)=delete;
tcp_server& operator=(tcp_server& tcp_server)=delete;
void start();//用一个线程来执行服务器的accept过程
void server_end();//将服务的监听accept过程进行停止
~tcp_server();//析构函数回收各种资源
private:
void server_start();//服务执行的函数,利用一个线程来调用这个过程
void communication(int client_fd);//一客户端对应一个线程,用一个线程来对接实际连接的客户端
void process_echo(int client_fd,std::vector<std::string>& commands);//对客户端发送的信息进行回复
bool set_fdrev_timeout(const int fd,timeval timeout);//给文件描述的接收设置超时机制,如果timeout不为0,超时了就设置全局错误码
void get_all_commands(std::string & pending,std::vector<std::string>& commands);//从pending中获取完整的命令
void shutdown_clientfd_rdwr();//将所有的被阻塞在recv的客户端进行一个接收端关闭,这样就可以返回了
void join_client_thread();//将所有的客户端线程进行一个join,等待客户端线程返回
//这个服务器
private:
//首先是监听套接字
int _listen_fd;
std::string _server_ip;//表示服务器绑定的ip地址
uint16_t _server_port;//表示服务器绑定的端口号
std::vector<std::thread> _cl_thread_vector;//维护一线程一客户端的容器
std::thread _server_thread;//服务端的线程,用来执行server_start 的操作
std::map<int,client> _cl_info_map;//用于存储客户端的信息
bool _server_init_status;//表示服务器初始化的状态
std::atomic<bool> _server_accept_status;//表示可以进行循环接收的状态,为true表示可以循环的接收accept,若为false表示此时不可以进行循环accept了
std::mutex _map_mutex;//用于互斥访问_cl_info_map的锁
};
