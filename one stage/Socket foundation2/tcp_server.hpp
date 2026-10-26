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
#define SINGLE_COMMAND_LENGTH 1024  //表示单条命令的最大长度
//定义一个结构体来来存储已经建立连接的客户端的ip与端口地址
struct client_address
{
    std::string _ip;
    int _port;
};
void client_address_print(client_address& c_addr)
{
    std::cout<<"have a new link"<<std::endl;
    std::cout<<"ip:"<<c_addr._ip<<" port:"<<c_addr._port<<std::endl;
}

//在暂存命令string中找到此时此刻完整的命令,_str是暂存命令的暂存区,然后commands这个就是我们解析之后的命令
void  get_all_commands(const int client_fd,std::string& _str,std::vector<std::string> & commands)
{
    //对_str暂存命令的区域进行一个扫描读取我们要想要的完整命令
    size_t pos=0;
    for(pos=_str.find('\n');pos!=std::string::npos;pos=_str.find('\n'))
    {
        //能够从_str中找到对应的'\n',然后得到的位置不等于非法,这个时候找到了
        //完整的命令,然后插入到我们对应的commands中去
        std::string _mid=_str.substr(0,pos);
        //首先判断每条命令是否按照我们规定的每条命令的长度不要超过SINGLE_COMMAND_LENGTH
        if(_mid.size()<SINGLE_COMMAND_LENGTH)
        {
            //然后判断_mid最后一个字符是否为'\r',如果是就移除
            if(_mid.size()!=0)
            {
                if(_mid.back()=='\r')
                {
                    _mid.pop_back();
                }
                //然后插入到commands中
                commands.push_back(_mid);
            }
            //然后剔除已经从_str中获取的命令
            _str.erase(0,pos+1);
        }
        else
        {
            throw std::runtime_error(std::string("single command length more than SINGLE_COMMAND_LENGTH")+std::strerror(errno));
        }  
    }
}
//处理单条命令的函数
void process_command(const int client_fd,std::map<std::string,std::string> & knowladge_base,const std::string& command)
{
    ssize_t len2 = 0;
    size_t sent = 0;
    // 查找知识库是否存在
    auto it = knowladge_base.find(command);
    if (it != knowladge_base.end())
    {
        std::string mid = it->second + '\n';
        // 存在
        while (sent < mid.size())
        {
            len2 = send(client_fd, mid.c_str() + sent, mid.size() - sent, 0);
            if (len2 <= 0)
            {
                std::perror("send error");
                break;
            }
            sent += len2;
        }
    }
    else
    {
        // 不存在,返回不存在的内容
        std::string default_command = "not exist\n";
        size_t size = default_command.size();
        while (sent < size)
        {
            len2 = send(client_fd, default_command.c_str() + sent, size - sent, 0);
            if (len2 <= 0)
            {
                std::perror("send error");
                break;
            }
            sent += len2;
        }
    }
}

//进行单客户端通信
void  communction(std::map<std::string,std::string> & knowladge_base,
    std::map<int,client_address>& client_map,int& client_fd)
{
    //与单个客户端进行一个对话
    char buffer[1024];
    memset(buffer,0,sizeof(buffer));
    //循环接收数据
    ssize_t len=0;
    std::string pending;//待定区域
    std::string command;
    std::vector<std::string> commands;//定义一个命令的容器集合,用于存放所有的命令,然后进行一个集中处理
    while(true)
    {
        len=recv(client_fd,buffer,sizeof(buffer),0);
        if(len>0)
        {
            //定义一个接受暂存区
            pending.append(buffer,len);
            //从pending获得我们想要的命令
            try
            {
                //如果这个函数出现一个异常,我们规定的这个异常是单条命令已经超过最大的长度
                get_all_commands(client_fd,pending,commands);
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
                break;
            }
            //循环处理命令
            int len=commands.size();
            for(int i=0;i<len;++i)
            {
                process_command(client_fd,knowladge_base,commands[i]);
            }
            //清理一下commands
            commands.clear();
        }
        else if(len==0)
        {
            //这里证明出现了错误,我们要根据错误类型进行一个处理
            std::cout<<"client shutdown"<<std::endl;
            break;
        }
        else
        {
            perror("communction have error");
            break;
        }
    }
    close(client_fd);
    //清理map的中的元素
    client_map.erase(client_fd);
}

class tcp_server
{
public:
    //缺省构造函数
    tcp_server(std::string ip="0.0.0.0",int port=8080)
    :_ip_adress(ip)
    ,_port(port)
    ,_status(false)
    {
        _listen_file_descriptor=socket(AF_INET,SOCK_STREAM,0);//默认为阻塞模式
        if(_listen_file_descriptor==-1)
        {
            //如果这里没有创建对应的socket,这个时候用异常进行处理,把异常抛出去,让外层进行一个处理
            throw std::runtime_error(std::string("socket create error")+strerror(errno));
        }
        else
        {
            //绑定对应的ip以及对应的端口号
            std::cout<<"socket create success"<<std::endl;
            sockaddr_in bind_addr;
            memset(&bind_addr,0,sizeof(bind_addr));
            bind_addr.sin_family=AF_INET;
            bind_addr.sin_port=htons(_port);//进行大小端端口转换
            int ret=inet_pton(AF_INET,_ip_adress.c_str(),&bind_addr.sin_addr);
            if(ret==1)
            {
                int ret=bind(_listen_file_descriptor,reinterpret_cast<sockaddr*>(&bind_addr),sizeof(bind_addr));
                if(ret==-1)
                {
                    std::perror("bind error :");
                    _status=false;
                }
                else
                {
                    std::cout<<"bind success"<<std::endl;
                    _status=true;
                    //调用知识库,进行一个初始化
                    init_Knowledge_Base();
                }
            }
            else if(ret==0)
            {
                std::cout<<"addr format error"<<std::endl;
            }
            else
            {
                std::perror("in bind,bind_addr have a question:");
            }     
        }      
    }
    tcp_server(tcp_server& my_server)=delete;
    ~tcp_server()
    {
        //关闭监听节点的文件描述符
        if(_listen_file_descriptor!=-1)
            close(_listen_file_descriptor);
        //首先这个关闭的意思是,如果服务器异常中断了,那么就需要关闭之前链接的tcp连接,在程序运行的时候,也可以进行关闭连接
        int num=_client_map.size();
        if(num>0)
        {
            auto it=_client_map.begin();
            while(num--)
            {
                close(it->first);
                it++;
            }
        }
    }
    void start_server()
    {
        if(_status==true)
        {
            //开始监听,然后进行接受连接
            int ret=listen(_listen_file_descriptor,10);
            if(ret==-1)
            {
                std::perror("listen error:");
                return;
            }
            else
            {
                std::cout << "listen success, waiting for client..." << std::endl;
                //开始接受连接
                while(true)
                {
                    sockaddr_in client_addr;
                    memset(&client_addr,0,sizeof(client_addr));
                    socklen_t len=sizeof(client_addr);//获取长度
                    int client_fd=accept(_listen_file_descriptor,reinterpret_cast<sockaddr*>(&client_addr),&len);
                    if(client_fd==-1)
                    {
                        std::perror("client_fd==-1:");
                    }
                    else
                    {
                        std::cout<<"one client haved a link"<<std::endl;
                        client_address c_addr;
                        //获得具体的信息
                        char mid[16];
                        const char* tag=inet_ntop(AF_INET,&client_addr.sin_addr,mid,sizeof(mid));
                        if(tag==nullptr)
                        {
                            std::cout<<"ip error"<<std::endl;
                            close(client_fd);
                            return ;
                        }
                        else
                        {
                            c_addr._ip=tag;
                            c_addr._port=ntohs(client_addr.sin_port);
                            _client_map.insert(std::make_pair(client_fd,c_addr));
                            //打印连接的信息
                            client_address_print(c_addr);
                            //进行一个通信
                            communction(_knowledge_base,_client_map,client_fd);
                        }
                    }
                }
            }
        }
        else
        {
            std::cout<<"Server can not init"<<std::endl;
            return ;
        } 
    }
private:
    //知识仓库进行一个初始化,里面假如加了十条常用的回复语
    void init_Knowledge_Base()
    {
        _knowledge_base.insert(std::make_pair("hello","hello小伙伴"));
        _knowledge_base.insert(std::make_pair("晚安","晚安! 祝你有一个好梦!!!"));
        _knowledge_base.insert(std::make_pair("早上好","早上好啊,小伙伴,让我们开始一天的工作吧!!"));
        _knowledge_base.insert(std::make_pair("china","中国"));
    }
private:
    //对于tcp_server,我们用以下成员变量来存储对应的信息
    //对于传入的ip我们用字符串表示，可以用点分十进制来表示
    std::string _ip_adress;
    int _port;//这个表示监听端口的端口号
    int _listen_file_descriptor;//监听标识符
    std::map<int,client_address> _client_map;
    bool _status;
    std::map<std::string,std::string> _knowledge_base;
};