#include<sys/types.h>
#include<sys/socket.h>
#include<iostream>
#include<string>
#include<cstring>
#include<arpa/inet.h>
#include <netinet/in.h>
#include<unistd.h>
#include<map>
struct my_sockaddr
{
    std::string sin_ip;
    std::string sin_port;
};

//创建我们自己定义的tcp_server类,用这个类创建的对象可以充当服务器对象,可以对特定的端口进行一个监听
//处理不同的客户端发起的tcp连接请求
class tcp_server
{
    public:
    //缺省参数的构造函数,用于表示默认构造以及有参构造函数
    tcp_server(std::string ip="0.0.0.0",int port=8080)
    :_sock_port(port)
    ,_sock_ip(ip)
    {
        //经过上面的初始化操作,我们可以进行一个组合_sock_ip:_sock_port,这样用于创建一个通信阶段,下面开始创建
        //通信结点,获得对应的文件描述符fd
        _sock_file_descriptor=socket(AF_INET,SOCK_STREAM,0); //参数说明AF_INET:表示ipv4协议族,SOCK_STREAM:表示tcp协议,0:表示默认的协议
        //判断是否创建成功,如果创建失败,返回-1,并且设置errno的值
        if(_sock_file_descriptor==-1)
        {
            std::cerr<<"socket not found"<<std::endl;
            exit(1);
        }
        else
        {
            //创建成功就进行一个绑定我们的监听端口以及ip地址
            //创建我们的地址
            std::cout<<"fd create success"<<std::endl;
            sockaddr_in addr;
            //进行一个初始化操作
            memset(&addr,0,sizeof(addr));
            addr.sin_family=AF_INET;
            addr.sin_port=htons(_sock_port);//将主机的小端字节序转换成网络上面的大端字节序
            addr.sin_addr.s_addr=inet_addr(_sock_ip.c_str());//将点分十进制的ip地址转换成网络字节序的整数
            //进行一个bind绑定
            int ret=bind(_sock_file_descriptor,reinterpret_cast<struct sockaddr*>(&addr),sizeof(addr));
            //判断是否绑定成功
            if(ret==-1)
            {
                std::cerr<<"bind error"<<std::endl;
                exit(2);
            }
            else
            {
                std::cout<<"bind success"<<std::endl;//到这里算是创建了一个通信结点,后续我们可以通过创建成员函数来进行一个监听的行为
            }
        }
    }
    ~tcp_server()
    {
        //关闭通信结点,释放资源
        close(_sock_file_descriptor);
    }
    //进行连接监听的成员函数
    void  start_server()
    {
        //进行监听
        int ret=listen(_sock_file_descriptor,10);
        if(ret==-1)
        {
            std::cerr<<"listen error ,please check the socket file descriptor"<<std::endl;
            exit(3);
        }
        while(true)
        {
            //开始从监听的通信结点中获取客户端的连接请求,并且进行一个处理
            struct sockaddr_in client_addr;
            memset(&client_addr,0,sizeof(client_addr));
            socklen_t len=sizeof(client_addr);
            int client_fd=accept(_sock_file_descriptor,reinterpret_cast<struct sockaddr*>(&client_addr),&len);
            if(client_fd==-1)
            {
                std::cerr<<"client_fd create error"<<std::endl;
                exit(4);
            }
            else
            {
                std::cout<<"client_fd create success"<<std::endl;
                _client_socket_map.insert(std::make_pair(client_fd,my_sockaddr{inet_ntoa(*(struct in_addr*)&client_addr.sin_addr.s_addr),std::to_string(ntohs(client_addr.sin_port))}));
                std::cout<<_client_socket_map[client_fd].sin_ip<<":"<<_client_socket_map[client_fd].sin_port<<" connect success"<<std::endl;
            }
        }
    }
    private:
    int _sock_file_descriptor;//用来监听的通信结点的文件描述符,这里先使用一个,有的服务器可能会使用多个通信结点,但是我们这里先使用一个
    std::map<int, struct my_sockaddr> _client_socket_map;//我们创建一个map容器,用来存储不同的端口号以及对应的ip地址,这样可以方便我们后续的操作
    int _sock_port;//我们指定通信结点用于监听的端口号
    std::string _sock_ip;//通信节点的ip地址
};