#pragma once
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <string>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>
#include <stdexcept>
#include <vector>
#define PENDING_LENGTH 2048    //表示命令区的最大长度
#define SINGLE_COMMAND_LENGTH 1024  //表示单条命令的最大长度 这里表示正文加\r\n也就是去除这些符号之后的正文长度

//用于保存客户端的连接信息,ip+port
struct client_address
{
    std::string _ip;
    int _port;
    //打印客户端的连接信息
    void client_address_print();
};
class tcp_server
{
public:
    //缺省构造函数
    tcp_server(std::string ip="0.0.0.0",int port=8080);
    //删除拷贝构造函数,不允许值拷贝,防止资源泄露或者重复释放资源
    tcp_server(const tcp_server& my_server)=delete;
    ~tcp_server();//析构函数
    void start_server();//开启服务的接口
private:
//在暂存命令string中找到此时此刻完整的命令,_str是暂存命令的暂存区,然后commands这个就是我们解析之后的命令
void  get_all_commands(const int& client_fd,std::string& _pending,std::vector<std::string> & commands);
//进行单客户端通信
void  communication(const int& client_fd);
//处理单条命令的函数
void process_command(const int& client_fd,const std::string& command);
//知识仓库进行一个初始化,里面假如加了十条常用的回复语
void init_Knowledge_Base(); 
private:
    //对于传入的ip我们用字符串表示，可以用点分十进制来表示
    std::string _ip_adress;
    int _port;//这个表示监听端口的端口号
    int _listen_file_descriptor;//监听标识符
    std::map<int,client_address> _client_map;//存储文件标识符与客户端的信息的映射
    bool _status;//表示服务器在创建的过程中的状态
    std::map<std::string,std::string> _knowledge_base; //知识库,用于回复客户端的内容
};