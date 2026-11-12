#pragma once
#include "thread_pool.h"
#include <string>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <sys/types.h>
#include <mutex>
#include <atomic>
#include <functional>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SIGNAL_COMMAND_MAX_LENGTH 1024 //表示单条命令的最大长度
#define THREAD_NUM 2  //定义一个宏,用来表示线程池中的线程数量
#define TV_SEC 5 //表示五秒一刷新
#define TV_USEC 0 //这个表示微秒

class client_addr
{
public:
client_addr(std::string ip,uint16_t port);
//打印信息函数
void print_client();
private:
std::string _client_ip;
uint16_t _client_port;
};

class tcp_server
{
public:
tcp_server(std::string ip="0.0.0.0",uint16_t port=8080);//缺省构造函数,包含了默认构造与有参构造
tcp_server(const tcp_server& _tcp_server)=delete;//删除拷贝构造函数
tcp_server& operator=(const tcp_server& _tcp_server)=delete;//删除拷贝赋值函数
~tcp_server();//析构函数
void start();//开始服务,执行服务线程
void start_server();//开始提供服务
void stop_server();//关闭提供的服务
void communication(int client_fd);//与client_fd进行一个通信
void get_all_commands(std::string& pending,std::vector<std::string> & commands);//将pending中的字符串进行一个解析,将命令解析到commands中
bool handle_commands(int client_fd,std::vector<std::string> & commands,client_addr client);//对commands中的命令进行一个处理
bool set_fdrecv_timeout(const int&_fd,const timeval time);//给文件描述符的接收缓冲区来设置对应的超时策略
void server_thread_notification(std::string&& reply,int client_fd,client_addr && client);//这个函数表示主线程告知客户端,我虽然已经给你连上了但是我线程池的任务队列已经满了,不能再处理你的请求了
private:
std::string _ip;//表示要监听的ip地址,0.0.0.0表示监听本地所有网卡的ip地址
uint16_t _port;//表示tcp绑定的端口号
int _listen_fd;//表示监听套接字绑定的文件描述符
thread_pool _thread_pool;//表示一个线程池对象
std::map<int,client_addr> _client_map;//用来存储客户端的相关的信息
std::atomic<bool> _stop;//表示客户端的停止标志
bool _init_status;//这个表示初始化的标志
std::mutex _mutex;//锁,主要用来保护map这种资源的
std::thread _server_thread;//表示服务线程
};