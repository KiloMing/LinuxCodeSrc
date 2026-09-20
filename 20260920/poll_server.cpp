#include <iostream>
#include <cstring>
<<<<<<< HEAD
#include <cerrno>
#include <poll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

int main()
{
    constexpr int MAX_FDS = 1024;

    int socket_server = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_server < 0)
    {
        perror("socket");
        return 1;
    }

=======
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <cstring>
#include <cstdint>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <cerrno>
#include <poll.h>

using namespace std;

int main(int argc, char *argv[])
{
    int socket_server = socket(AF_INET, SOCK_STREAM, 0); // 最后一个参数是什么？: 是协议类型，0 是默认协议
    if(socket_server < 0)
    {
        cerr << "socket" << endl;
        return 0;
    }
    
    //不用在在意
>>>>>>> 252e04f (learn the poll and poll)
    int opt = 1;
    if(setsockopt(socket_server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        close(socket_server);
        return 1;
    }

<<<<<<< HEAD
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    if(bind(socket_server,
            reinterpret_cast<sockaddr*>(&server_addr),
            sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(socket_server);
        return 1;
    }

=======
    //配置addr
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family =  AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // 2?? 监听本机所有可用的ipv4网络接口
    server_addr.sin_port = htons(8080);
    if(bind(socket_server, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0)
    {
        cerr << "bind" << endl;
        return 0;
    }
    
    //listen 3???   ： 这个5 就是限制accept的连接队列长度
>>>>>>> 252e04f (learn the poll and poll)
    if(listen(socket_server, 5) < 0)
    {
        perror("listen");
        close(socket_server);
        return 1;
    }

    cout << "Server listening on port 8080......." << endl;

<<<<<<< HEAD
    pollfd fds[MAX_FDS];

    for(int i = 0; i < MAX_FDS; ++i)
=======
    pollfd fds[1024];
    //不能使用memset
    for(int i = 0; i < 1024; i++)
>>>>>>> 252e04f (learn the poll and poll)
    {
        fds[i].fd = -1;
        fds[i].events = 0;
        fds[i].revents = 0;
    }

<<<<<<< HEAD
    fds[0].fd = socket_server;
    fds[0].events = POLLIN;

    while(true)
    {
        int ready = poll(fds, MAX_FDS, -1);

        if(ready < 0)
        {
            if(errno == EINTR)
            {
                continue;
            }

            perror("poll");
            break;
        }

        for(int i = 0; i < MAX_FDS; ++i)
        {
            if(fds[i].fd < 0)
            {
                continue;
            }

=======
    //第 0 个位置专门放保存监听的fd
    fds[0].fd = socket_server;
    fds[0].events = POLLIN; //读事件


    //进入事件循环
    while(true)
    {
       int ready = poll(fds, 1024, -1);
       if(ready < 0)
       {
        perror("poll");
        break;
       }

       for(int i = 0; i < 1024; i++)
       {
            if(fds[i].fd == -1)
            {
                continue;
            }
>>>>>>> 252e04f (learn the poll and poll)
            if(!(fds[i].revents & POLLIN))
            {
                continue;
            }
<<<<<<< HEAD

            if(fds[i].fd == socket_server)
            {
                int client_sock = accept(socket_server, nullptr, nullptr);
                if(client_sock < 0)
                {
                    perror("accept");
                    continue;
                }

                cout << "New client fd = " << client_sock << endl;

                bool inserted = false;

                for(int j = 1; j < MAX_FDS; ++j)
=======
            if(fds[i].fd == socket_server)  //为什么要判断？ 
            {
                int client_sock = accept(socket_server, nullptr, nullptr);
                cout << "New client fd = " << client_sock << endl;

                for(int j = 1; j < 1024; j++)
>>>>>>> 252e04f (learn the poll and poll)
                {
                    if(fds[j].fd == -1)
                    {
                        fds[j].fd = client_sock;
                        fds[j].events = POLLIN;
<<<<<<< HEAD
                        inserted = true;
                        break;
                    }
                }

                if(!inserted)
                {
                    cerr << "Too many clients" << endl;
                    close(client_sock);
                }
=======
                        break;
                    }
                }
>>>>>>> 252e04f (learn the poll and poll)
            }
            else
            {
                char buffer[1024];
<<<<<<< HEAD

                ssize_t n = read(fds[i].fd, buffer, sizeof(buffer));

                if(n == 0)
                {
                    cout << "Client closed fd = " << fds[i].fd << endl;
                    close(fds[i].fd);
                    fds[i].fd = -1;
                    fds[i].events = 0;
                    fds[i].revents = 0;
                }
                else if(n < 0)
                {
                    perror("read");
                    close(fds[i].fd);
                    fds[i].fd = -1;
                    fds[i].events = 0;
                    fds[i].revents = 0;
                }
                else
                {
                    write(fds[i].fd, buffer, static_cast<size_t>(n));
                }
            }
        }
    }

    close(socket_server);
    return 0;
}
=======
                ssize_t n = read(fds[i].fd, buffer, sizeof(buffer));
                if(n <= 0)
                {
                    cout << "Client closed fd" << endl;
                    close(fds[i].fd);
                    fds[i].fd = -1;
                }
                else
                {
                    write(fds[i].fd, buffer, n);
                }
            }
       }
    }
    close(socket_server);
    return 0;
}
>>>>>>> 252e04f (learn the poll and poll)
