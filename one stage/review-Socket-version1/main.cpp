#include "tcp_server.h"

//判断str中的字符是否全是数字字符
bool is_num(std::string & str)
{
    if(str.size()==0)
        return false;
    for(auto &e:str)
    {
        if(e>=48&&e<=57)
        {
            continue;
        }
        return false;
    }
    return true;
}
void server_quit(tcp_server& my_server)
{
    std::string command;
    while(std::getline(std::cin,command))
    {
        //判断command的字符串
        if(command.compare("quit")==0)
        {
            //调用my_server的停止服务函数
            my_server.server_end();
            break;
        }
        //清理字符串
        command.clear();
    }
}
int main(int argc,char* argv[])
{
    //服务端程序绑定端口号进行一个接收
    //首先判断argc的大小
    if(argc==2)
    {
        //这里表示输入了用户指定的端口号,所以可以进行一个处理
        std::string port=argv[1];
        //判断端口号是否符合规范
        if(is_num(port))
        {
            //是纯数字,判断是否合法1024 ~ 49151
            //转成整型
            int _port=std::stoi(port);
            if(_port>=1024&&_port<=49151)
            {
                try
                {
                    //然后创建服务器
                    tcp_server my_server("0.0.0.0",static_cast<uint16_t>(_port));
                    my_server.start();
                    server_quit(my_server);
                }
                catch(const std::exception& e)
                {
                    std::cerr << e.what() << '\n';
                    return 3;
                } 
            }
            else
            {
                throw std::logic_error("The port number entered by the user exceeds the legal range");
                return 2;
            }
        }
        else
        {
            std::logic_error("The port number entered by the user is not a pure number");
            return 1;
        } 
    }
    else
    {
        try
        {
            //这里使用默认的端口号来进行一个监听
            tcp_server my_server;
            my_server.start();
            server_quit(my_server);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            return 3;
        }
        
    }
    return 0;
}