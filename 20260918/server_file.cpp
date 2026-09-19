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
using namespace std; 

struct file_Info
{
    char file_name[256];
    uint64_t file_size;
};

class socket_server
{
private:
    int server_sock;
    struct sockaddr_in server_addr;
    int recv_flag = 0;
public:
    socket_server();
    ~socket_server();
    void start();
    bool recv_all(int client_sock, void* data, size_t len);
    bool send_all(int sock, const void* data, size_t len);
    bool recv_file(int client_sock, const file_Info &file);
};

bool socket_server::recv_file(int client_sock, const file_Info &file)
{
    //creat new file
    string recv_name = "recv_" + string(file.file_name);
    int file_fd = open(recv_name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(file_fd < 0)
    {
        perror("open");
        return false;
    }
    char buffer[4096];
    uint64_t total_received = 0;

    // 2. 一直接收，直到收到 file_size 个字节
    while(total_received < file.file_size)
    {
        uint64_t remaining =
            file.file_size - total_received;

        size_t need =
            remaining < sizeof(buffer)
            ? static_cast<size_t>(remaining)
            : sizeof(buffer);

        ssize_t n = recv(
            client_sock,
            buffer,
            need,
            0
        );

        if(n > 0)
        {
            // 写入磁盘文件
            ssize_t written = write(
                file_fd,
                buffer,
                n
            );

            if(written < 0)
            {
                perror("write");

                close(file_fd);
                return false;
            }

            total_received += n;

            cout << "received: "
                 << total_received
                 << " / "
                 << file.file_size
                 << " bytes"
                 << endl;
        }
        else if(n == 0)
        {
            cout << "Client disconnected" << endl;

            close(file_fd);
            return false;
        }
        else
        {
            perror("recv");

            close(file_fd);
            return false;
        }
    }

    close(file_fd);

    cout << "File received successfully!" << endl;

    return true;
};

bool socket_server::send_all(int sock, const void* data, size_t len)
{
    const char* ptr = static_cast<const char*>(data);
    ssize_t total_sent = 0;
    while(total_sent < len) 
    {
        ssize_t n = send(sock, ptr + total_sent, len - total_sent, 0);
        if(n <= 0)
        {
            return false;
        }
        total_sent += n;
    }   
    return true;
}


bool socket_server::recv_all(int client_sock, void* data, size_t len)
{
    char *ptr = static_cast<char*>(data);
    ssize_t total_received = 0;
    while(total_received < len)
    {
        ssize_t n = recv(client_sock, ptr + total_received, len - total_received, 0);
        if(n < 0)
        {
            perror("recv");
            return false;
        }
        if(n == 0)
        {
            cout << "Client disconnected" << endl;
            return false;
        }
        total_received += n;
    }
    return true;
}
 
socket_server::socket_server()
{
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        cerr << "Socket creation failed" << endl;
        exit(1);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);
    if (bind(server_sock, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        cerr << "Bind failed" << endl;
        exit(1);
    }
}

void socket_server::start()
{
    if (listen(server_sock, 5) < 0) {
        cerr << "Listen failed" << endl;
        return;
    }
    cout << "Server is listening on port 8080" << endl;
    while(true)
    {
        int client_sock = accept(server_sock, nullptr, nullptr);
        if (client_sock < 0) {
            perror("Accept failed");
            return;
        }
        pid_t pid = fork();
        if(pid > 0)
        {   
            //父进程
            close(client_sock);
            //回去等待下一个客户端
            continue;
        }
        else if(pid == 0)
        {
            //子进程
            cout << "Client connected" << endl;
            close(server_sock);
            file_Info file{};

            if(!recv_all(client_sock, &file, sizeof(file)))
            {
                cout << "Receive file info failed" << endl;
                close(client_sock);
                _exit(1);
            }
            cout << "file name: " << file.file_name << endl;
            cout << "file size: " << file.file_size << endl;
            recv_flag = 1;   // receive successfully
            const char ok[] = "OK";
            if(!send_all(client_sock, ok, 2))
            {
                cout << "Send ACK failed" << endl;
                close(client_sock);
                _exit(1);
            }
            recv_file(client_sock, file);
            }
        }
}

socket_server::~socket_server()
{
    close(server_sock);
}

int main()
{
    socket_server server;
    server.start();
    return 0;
}