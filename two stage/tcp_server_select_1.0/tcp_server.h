//这个只做一个简单的单线程的利用select来实现io多路复用的tcp_server服务器
#pragma once
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <map>
#include <thread>
#include <atomic>
#include <string>
#include <cstring>
#include <vector>
#include <cstdint>
#include <mutex>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <stack>
#include <fcntl.h>
#define SIGNAL_COMMAND_LENGTH 1024 //表示单条命令的最大长度
#define TV_SEC 2 
#define TV_USEC 0
class client_state
{
public:
client_state(std::string ip="",uint16_t port=0);
void print_client_info();//打印客户端的连接信息
private:
std::string _ip;
uint16_t _port;
public:
std::string _pending;//保存还没有解析的命令
std::string _send_buffer;//保存要发送的数据
size_t _send_offset;//表示此前已经发送到哪里了,哪里还没有发送,表示偏移量
};

class tcp_server
{
public:
//缺省构造函数
tcp_server(std::string ip="0.0.0.0",uint16_t port=8080);
tcp_server(const tcp_server & _tcp_server)=delete;
tcp_server& operator=(const tcp_server& _tcp_server)=delete;//删除拷贝构造以及赋值构造函数
~tcp_server();//析构函数
void start();//服务入口函数
void server_start();//服务开始函数
void server_end();//服务结束函数
void communication(int client_fd,fd_set& master_read_set,fd_set& master_write_set);//与客户端进行一个沟通的函数
void get_all_commands(std::string& pending,std::vector<std::string> & commands);//对接收的数据进行命令解析函数
void handle_commands(int client_fd,std::vector<std::string> & commands,fd_set& master_write_set);//对命令进行一个回复的函数
void fd_check_loop(int  max_fd,fd_set& master_read_set,fd_set& ready_read_set,fd_set& master_write_set,fd_set& ready_write_set);//对位图中的文件描述符进行一个遍历检查
void update_max_fd(int client_fd,fd_set& master_read_set,fd_set& master_write_set);//对主动断开连接的以及被动断开连接的fd进行一个判断
bool set_nonblock(int client_fd);//这个函数表示给client_fd设置非阻塞处理
private:
std::string _ip;
uint16_t _port;
int _listen_fd;//表示监听文件描述符
std::atomic<bool> _stop;//表示服务器的停止状态
std::map<int,client_state> _client_map;//表示存储客户端信息的map
std::thread _server_thread;//表示执行服务端的线程
std::mutex _mutex;//用于保护临界资源
bool _init_status;//表示初始化是否成功
std::stack<int> _max_fd;//表示目前接收的最大的fd栈
};