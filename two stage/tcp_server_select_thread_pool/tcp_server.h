#pragma once
#include "thread_pool.h"
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <mutex>
#include <map>
#include <queue>
#include <memory>
#include <cstdint>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/time.h>
#include <cmath>
#include <fcntl.h>
#include <atomic>
#include <stack>
#define THREAD_NUM 4 //线程池的线程个数
#define TASK_NUM 5   //任务队列的大小
#define ADD 0
#define DEL 1
#define SEC 1
#define USEC 0
#define SIGNAL_COMMAND 1024 //单条命令的最大长度
constexpr double EPS = 1e-9;
//该类与可以与文件描述符在map中建立一个映射关系
class client_state
{
public:
client_state(std::string client_ip,std::uint16_t client_port);
void client_info_print();//用户信息打印
private:
std::string _client_ip; //用户的ip以及端口号
std::uint16_t _client_port;
public:
std::string _pending;//待解析的命令
std::string _send_buffer;//等待发送的信息
size_t _send_offset;//等待发送信息的偏移量
};

//能够处理简单的加减乘除的tcp_server服务器
class tcp_server
{
public:
tcp_server(std::string ip="0.0.0.0",std::uint16_t port=8080);
tcp_server(const tcp_server& _tcp_server)=delete;
tcp_server& operator=(const tcp_server& _tcp_server)=delete;
~tcp_server();
void start();
void server_start();//服务开始函数
void server_end();//结束服务函数
void commands_parsing(int client_fd);//命令解析函数以及加入任务队列
void get_all_commands(std::map<int,client_state>::iterator& it);//获得所有的命令,插入到任务队列中
void task_process();//线程池调用的函数,进行一个任务处理
void process_select(int max_fd);//对从select返回的fd进行一个处理
void update_read_write_fd(int client_fd,int flag);//更新最大fd的队列
int get_max_fd();//获得当前最大的fd
void process_accept();//处理this->listen_fd中的recv
void update_client_map(int client_fd,client_state && client,int flag);//更新client_map
std::string update_task_queue(std::queue<std::string> & _task_queue,std::string && str,int flag);
std::string arith_process(std::string & real_command);//这个表示对算术进行处理
void send_result(int client_fd);//将结果进行发送
bool set_noblock(int client_fd);//给这个套接字设置非阻塞
void process_completion_queue();//处理completion_queue 中的发送任务
private:
std::unique_ptr<thread_pool> _thread_pool;//线程池
std::string _ip;
std::uint16_t _port;
int _listen_fd;//监听套接字
std::map<int,client_state> _client_map;//存放已经连接的客户端的信息
std::queue<std::string> _client_task_queue;//这个表示已经解析好的命令,等待回复的命令   线程池只需要做的事情只有一个就是计算
std::queue<std::string> _completion_queue;//这个表示Worker已经处理之后的任务
std::mutex _mutex_map;  //这里一共定义了三把锁,用来分别互斥访问三个互斥资源
std::mutex _mutex_task_queue;
std::mutex _mutex_completion_queue;
std::mutex _mutex_stop;//这个是对_stop进行一个阻塞处理的锁
std::mutex _mutex_max_fd;//对最大fd队列进行阻塞处理的锁
std::atomic<bool> _stop;//表示tcp_server是否停止的一个标识
bool _init_status;//表示tcp_server 是否初始化的一个标识
std::thread _server_thread;//服务线程
std::thread _io_thread;//专门负责io的线程,负责接收数据以及发送数据
std::stack<int> _max_fd;//记录当前最大的fd,以便于提供一个服务
fd_set master_read_set; //四个位图
fd_set ready_read_set;
fd_set master_write_set;
fd_set ready_write_set;
std::condition_variable _completion_con;//用于提醒执行io线程的
};