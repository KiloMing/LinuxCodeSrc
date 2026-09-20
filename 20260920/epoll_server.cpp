#include <iostream>
#include <cstring>
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
#include <sys/epoll.h>

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
    int opt = 1;
    if(setsockopt(socket_server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        close(socket_server);
        return 1;
    }

    //配置addr
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family =  AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; 
    server_addr.sin_port = htons(8080);
    if(bind(socket_server, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0)
    {
        cerr << "bind" << endl;
        return 0;
    }
    
    if(listen(socket_server, 5) < 0)
    {
        perror("listen");
        close(socket_server);
        return 1;
    }

    cout << "Server listening on port 8080......." << endl;

    //创建epoll实例
    int epfd = epoll_create1(1);
    if(epfd < 0)
    {
        perror("epoll_create1");
        close(socket_server);
        return 1;
    }
    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = socket_server;

    if(epoll_ctl(epfd, EPOLL_CTL_ADD, socket_server, &ev) < 0)      //epoll_ctl 的返回值是什么？
    {
        perror("epoll_ctl ADD server");
        close(epfd);
        close(socket_server);
        return 1;
    }
    //为什么要创建的MAX_EVENT呢？ select， poll，epoll都有
    const int MAX_EVENTS = 1024;
    epoll_event events[MAX_EVENTS];
    //进入事件循环
    while(true)
    {
       int ready = epoll_wait(epfd, events, MAX_EVENTS, -1);
       if(ready < 0)
       {
            if(errno == EINTR)
            {
                continue;
            }
            perror("epoll_wait");
            break;
       }

       //只遍历已经就绪的事件
       for(int i = 0; i < ready; i++)
       {
            int fd = events[i].data.fd; 
            if(fd == socket_server)
            {   
                //accept函数的参数是什么？ 
                int client_sock = accept(socket_server, nullptr, nullptr);
                if(client_sock < 0)
                {
                    perror("accept");
                    continue;
                }
                cout << "New client fd = " << client_sock << endl;
                //这个结构体是什么？
                epoll_event client_ev{};
                client_ev.events = EPOLLIN;
                client_ev.data.fd = client_sock;

                if(epoll_ctl(epfd, EPOLL_CTL_ADD, client_sock, &client_ev) < 0)
                {
                    perror("epill_ctl ADD client");
                    close(client_sock);
                }
                else
                {
                    char buffer[1024];
                    ssize_t n = read(fd, buffer, sizeof(buffer));
                    if(n == 0)
                    {
                        cout << "Client closed fd = " << fd << endl;
                        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                        close(fd);
                    } 
                    else if(n < 0)
                    {
                        perror("read");
                        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                        close(fd);
                    }
                    else
                    {
                        cout << "recv from fd " << fd << " : ";
                        cout.write(buffer, n);  // ???
                        cout << endl;
                        write(fd, buffer, n);
                    }
                }
            }
       }
    }
    close(epfd);
    close(socket_server);
    return 0;
} 