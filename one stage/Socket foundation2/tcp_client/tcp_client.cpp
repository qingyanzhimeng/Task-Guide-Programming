#include "tcp_client.h"
  //默认的缺省构造函数,这个用于传连接tcp_server服务器地址的一个默认值
tcp_client::tcp_client(std::string ip,int port)
:_comm_fd(-1)
,_status(true)
{
    //首先创建socket
    _comm_fd=socket(AF_INET,SOCK_STREAM,0);//选择ipv4协议,对应的字节流,然后默认是阻塞式通信
    if(_comm_fd>0)
    {
        std::cout<<"_comm_fd create success !!"<<"socket create success"<<std::endl;
        //然后开始连接服务器
        //确定协议类型
        memset(&_tcp_server_address,0,sizeof(_tcp_server_address));
        _tcp_server_address.sin_family=AF_INET;
        _tcp_server_address.sin_port=htons(port);
        //点分十进制的ip进行一个转换
        int ret=inet_pton(AF_INET,ip.c_str(),&_tcp_server_address.sin_addr);
        if(ret >0)
        {
            //得到了服务端的地址
            int ret2=connect(_comm_fd,reinterpret_cast<const sockaddr*>(&_tcp_server_address),sizeof(_tcp_server_address));
            if(ret2==0)
            {
                std::cout<<"client link to tcp_server success"<<std::endl;
            }
            else
            {
                //连接失败
                std::perror("connect error");
                _status=false;
            }
        }
        else if(ret==0)
        {
            std::cout<<"ip format error"<<std::endl;
            _status=false;
        }
        else
        {
            std::perror("ip conversion error");
            _status=false;
        }
    }
    else
    {
        throw std::runtime_error(std::string("client communication fd create error")+strerror(errno));
    } 
}
//析构函数,用于释放资源
tcp_client::~tcp_client()
{
    if(_comm_fd>0)
    {
        close(_comm_fd);
    }
}
//用于交流通信的接口
void tcp_client::communication()
{
    //先判断连接状态
    if(!_status)
    {
        return;   
    }
    else
    {
        //开始通信!!!
        //定义应用层的接收buffer
        char buffer[1024];
        memset(buffer,0,sizeof(buffer));
        size_t buffer_size=sizeof(buffer);
        std::string massage;
        std::string dialogue;
        bool comm_status=true;//这个状态看在接收的过程中是否有很长的消息
        std::cout<<"开始与服务器进行一个对话吧"<<std::endl;
        while(true)
        {
            std::cout<<"client>>:";
            std::getline(std::cin,dialogue);
            //拼接'\n'
            dialogue+='\n';
            //加入_pending;
            ssize_t len=0;
            size_t sent=0;
            //发送给服务端
            try
            {
                while(sent<dialogue.size())
                {
                    len=send(_comm_fd,dialogue.c_str()+sent,dialogue.size()-sent,0);
                    if(len>0)
                    {
                        sent+=len;
                        continue;
                    }
                    else if(len==0)
                    {
                        //判断errno是否为EINTR
                        if(errno==EINTR)
                            continue;
                        throw std::runtime_error(std::string("send error")+strerror(errno));
                    }
                    else
                    {
                        throw std::runtime_error(std::string("send error")+strerror(errno));
                    }
                }
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
                comm_status=false;
                break;
            }
            if(!comm_status)
                break;
            //清空上一条问答
            dialogue.clear();
            //接收服务器反馈过来的信息
            std::cout<<"tcp_server>>:";
            try
            {
                 while(true)
                {
                    int len2=recv(_comm_fd,buffer,buffer_size,0);
                    if(len2>0)
                    {
                        massage.append(buffer,len2);
                        //消息处理
                        try
                        {
                            massageprocess(massage);
                            if(massage.size()==0)
                                break;
                        }
                        catch(const std::exception& e)
                        {
                            std::cerr << e.what() << '\n';
                            comm_status=false;
                            break;
                        }
                    }
                    else if(len2==0)
                    {
                        if(errno==EINTR)
                            continue;
                        throw std::runtime_error(std::string("recv have a error")+strerror(errno));
                    }
                    else
                    {
                        throw std::runtime_error(std::string("recv have a error")+strerror(errno));
                    }
                }
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
                comm_status=false;
            }
            if(!comm_status)
                break;
        }
    }
}
void tcp_client::massageprocess(std::string& massage)
{
    //消息打印,打印消息
    std::string _mid;
    for(size_t pos=massage.find('\n');pos!=std::string::npos;pos=massage.find('\n'))
    {
        //截取消息
        _mid=massage.substr(0,pos);
        if(_mid.size()==0)
        {
            //删掉回复的'\n'
            massage.erase(0,pos+1);
            continue;
        }
        if(_mid.back()=='\r'&&_mid.size()==1)
        {
            massage.erase(0,pos+1);
            continue;
        }    
        if(_mid.back()=='\r')
        {
            _mid.pop_back();
        }
        if(_mid.size()<=MASSAGE_LENGTH)
        {      
            std::cout<<_mid<<std::endl;
            massage.erase(0,pos+1);
        }
        else
        {
            throw std::runtime_error(std::string("massage length more than MASSAGE_LENGTH"));
        }
    }
    //剩余的消息
    size_t length=massage.size();
    if(massage.size()>0&&massage.back()=='\r')
    {
        --length;
    }
    if(length>MASSAGE_LENGTH)
    {
        throw std::runtime_error(std::string("massage length more than MASSAGE_LENGTH"));
    }
}
