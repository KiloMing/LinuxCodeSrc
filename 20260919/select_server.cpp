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
    server_addr.sin_addr.s_addr = INADDR_ANY; // 2?? 监听本机所有可用的ipv4网络接口
    server_addr.sin_port = htons(8080);
    if(bind(socket_server, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0)
    {
        cerr << "bind" << endl;
        return 0;
    }
    
    //listen 3???   ： 这个5 就是限制accept的连接队列长度
    if(listen(socket_server, 5) < 0)
    {
        perror("listen");
        close(socket_server);
        return 1;
    }

    cout << "Server listening on port 8080......." << endl;

    fd_set master_set;
    fd_set read_set;
    // 清空集合
    FD_ZERO(&master_set);
    // 将监听socket加入集合
    FD_SET(socket_server, &master_set);
    // 当前最大的 fd.    4 ????
    int maxfd = socket_server;

    //进入事件循环
    while(true)
    {
        read_set = master_set;
        // 5 select 这个函数简单点说是什么意思？
        int ready = select(maxfd + 1, &read_set, nullptr, nullptr, nullptr);
        if(ready < 0)
        {
            if(errno == EINTR)  // 6 ?? EINTR 是什么意思？
            {
                continue;
            }

            perror("select");
            break;
        }

        //扫描所有fd 
        for(int fd = 0; fd <= maxfd; ++fd)
        {
            if(!FD_ISSET(fd, &read_set))
            {
                continue;
            }

            if(fd == socket_server)
            {
                
                int client_sock = accept(socket_server, nullptr, nullptr);
                if(client_sock < 0)
                {
                    perror("accept");
                    continue;
                }
                cout << "New client" << client_sock << endl;

                FD_SET(client_sock, &master_set);
                if(client_sock > maxfd)         // 7 为什么会有这一段？
                {
                    maxfd = client_sock;
                }
            }
            else
            {
                char buffer[1024];
                int n = read(fd, buffer, sizeof(buffer));
                
                //客户端关闭
                if(n <= 0) 
                {
                    cout << "Client closed : " << fd << endl;
                    close(fd);
                    FD_CLR(fd, &master_set);
                }
                else    //收到数据
                {
                    cout << "recv : ";
                    cout.write(buffer, n);
                    cout << endl;

                    write(fd, buffer, n); // 8 ？？这个write的参数是什么
                }

            }
        }
    }

    close(socket_server);
    return 0;
}