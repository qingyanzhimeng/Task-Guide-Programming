#pragma once 
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define  MASSAGE_LENGTH 1024
//创建一个客户端类与tcp_server进行一个通信
class tcp_client
{
public:
    //默认的缺省构造函数,这个用于传连接tcp_server服务器地址的一个默认值
    tcp_client(std::string ip="121.4.52.63",int port=8080);
    //用于交流通信的接口
    void communication();
private:
    void massageprocess(std::string& massage);
public:
    //析构函数,用于释放资源
    ~tcp_client();
private:
//客户端没有监听端口，只有一个通信的端口,所以创建的socket就是用来通信的
int  _comm_fd;
bool _status;//用于表示是否连接成功
sockaddr_in _tcp_server_address;
};
