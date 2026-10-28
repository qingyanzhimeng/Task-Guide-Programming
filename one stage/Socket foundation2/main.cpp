
#include "tcp_server.h"
#include <cctype>

int main(int argc,char* argv[])
{
    //这里先不判断argv[1]这个值的有效性,因为这些东西可以通过分层来进行一个限制
    if(argc>1)
    {
        //获得第一个第二个字符串的内容
        std::string argv_two=argv[1];
        //然后判断这个内容是否为整数数字,以及合法的端口范围,所以进行一个循环的判断然后再进行一个处理
        try
        {
            for(auto e:argv_two)
            {
                if(isdigit(e)==0)
                {
                    //证明输入的有不是数字的部分,这个时候就需要抛逻辑异常
                    throw std::logic_error(std::string("输入的端口号非数字 :")+strerror(errno));
                }
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            return 1;
        }
        //走到这里证明用户传入的就是整型数字
        //将字符串转成数字,然后判断传入的端口号是否合法
        try
        {
            int port=atoi(argv[1]);
            if(port>=1024&&port<=49151)
            {
                try
                {
                     //创建tcp_server服务器,然后程序继续进行
                    tcp_server my_tcp("0.0.0.0",port);
                    //开启服务
                    my_tcp.start_server();
                }
                catch(const std::exception& e)
                {
                    //服务创建失败,这里直接说明原因
                    std::cerr<<e.what()<<std::endl;
                    std::cout<<"暂时没有办法创建服务器程序!!!"<<std::endl;
                    return 3;
                }
            }
            else
            {
                throw std::invalid_argument(std::string("invalid port:")+strerror(errno));
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            return 2;
        }    
    }
    return 0;
}