#include <iostream>
#include <cstring>
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

    int opt = 1;
    if(setsockopt(socket_server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        close(socket_server);
        return 1;
    }

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

    if(listen(socket_server, 5) < 0)
    {
        perror("listen");
        close(socket_server);
        return 1;
    }

    cout << "Server listening on port 8080......." << endl;

    pollfd fds[MAX_FDS];

    for(int i = 0; i < MAX_FDS; ++i)
    {
        fds[i].fd = -1;
        fds[i].events = 0;
        fds[i].revents = 0;
    }

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

            if(!(fds[i].revents & POLLIN))
            {
                continue;
            }

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
                {
                    if(fds[j].fd == -1)
                    {
                        fds[j].fd = client_sock;
                        fds[j].events = POLLIN;
                        inserted = true;
                        break;
                    }
                }

                if(!inserted)
                {
                    cerr << "Too many clients" << endl;
                    close(client_sock);
                }
            }
            else
            {
                char buffer[1024];

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
