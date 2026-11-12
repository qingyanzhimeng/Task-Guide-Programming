#include "tcp_server.h"
#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    int port = 8080;

    if (argc >= 2)
    {
        port = std::atoi(argv[1]);
    }

    std::cout
        << "[TEST] creating tcp_server on port "
        << port
        << '\n';

    {
        tcp_server server("0.0.0.0", port);

        server.start();

        std::cout
            << "[TEST] server started. Connect clients now.\n"
            << "[TEST] Press Enter to leave scope "
            << "WITHOUT calling stop_server().\n";

        std::string line;
        std::getline(std::cin, line);

        std::cout
            << "[TEST] leaving scope now...\n";

        // 注意：
        // 这里故意不写 server.stop_server();
    }

    // 如果能够执行到这里，
    // 说明 tcp_server 的析构函数已经完整返回
    std::cout
        << "[TEST] tcp_server destructor finished; "
        << "scope exited cleanly.\n";

    return 0;
}
