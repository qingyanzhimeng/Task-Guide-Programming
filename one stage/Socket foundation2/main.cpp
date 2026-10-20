#include "tcp_server.hpp"

int main(int argc,char* argv[])
{
    //这里先不判断argv[1]这个值的有效性,因为这些东西可以通过分层来进行一个限制
    if(argc>1)
    {
        int port=atoi(argv[1]);
        tcp_server my_tcp("0.0.0.0",port);
        my_tcp.start_server();
    }
    return 0;
}