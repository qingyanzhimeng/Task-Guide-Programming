#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if(fd == -1)
    {
        std::cerr
            << "socket error: "
            << std::strerror(errno)
            << '\n';

        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);

    if(inet_pton(
        AF_INET,
        "127.0.0.1",
        &addr.sin_addr) != 1)
    {
        std::cerr << "inet_pton failed\n";
        close(fd);
        return 1;
    }

    if(connect(
        fd,
        reinterpret_cast<sockaddr*>(&addr),
        sizeof(addr)) == -1)
    {
        std::cerr
            << "connect error: "
            << std::strerror(errno)
            << '\n';

        close(fd);
        return 1;
    }

    std::cout << "connected\n";

    const char hello[] = "hello\n";

    ssize_t n = send(
        fd,
        hello,
        sizeof(hello) - 1,
        MSG_NOSIGNAL
    );

    if(n == -1)
    {
        std::cerr
            << "initial send error: "
            << std::strerror(errno)
            << '\n';

        close(fd);
        return 1;
    }

    char buffer[128];

    n = recv(
        fd,
        buffer,
        sizeof(buffer),
        0
    );

    if(n > 0)
    {
        std::cout << "reply: ";
        std::cout.write(buffer, n);
    }

    std::cout
        << "\n现在去服务端按 Ctrl+C 结束 server，"
        << "然后回来按 Enter...\n";

    std::cin.get();

    for(int i = 1; i <= 5; ++i)
    {
        errno = 0;

        ssize_t sent = send(
            fd,
            "test\n",
            5,
            MSG_NOSIGNAL
        );

        int saved_errno = errno;

        std::cout
            << "send #" << i
            << " return=" << sent
            << " errno=" << saved_errno
            << " "
            << std::strerror(saved_errno)
            << std::endl;

        sleep(1);
    }

    close(fd);

    return 0;
}
