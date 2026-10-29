#include "tcp_client.h"
#include <cstdlib>

int main(int argc ,char* argv[])
{
    //这个客户端从echo 接受ip地址与端口号
    std::string _ip;
    std::string _port;
    if(argc<3)
    {
        return 1;
    }
    else
    {
        //假如这个时候用户输入的值是正确的
        _ip=argv[1];
        _port=argv[2];
        //对字符串转成整型
        int port=atoi(_port.c_str());
        try
        {
            tcp_client client_one(_ip,port);
            client_one.communication();
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
    }
    return 0;
}