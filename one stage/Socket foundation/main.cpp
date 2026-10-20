#include "Server.hpp"

int main(int argc,char* argv[])
{
  
    
    //只传入一个参数,这个表示对应绑定的ip
    if(argc>1)
    {
        std::string port=argv[1];
        int _port=std::stoi(port);
        //创建服务器的对象,然后进行一个监听
        tcp_server myserver("0.0.0.0",_port);
        myserver.start_server();
    }   
    return 0;
}