#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstdio>
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
            // 磁盘 write 也可能短写，按实际返回值继续写。
            ssize_t total_written = 0;
            while(total_written < n)
            {
                ssize_t written = write(file_fd, buffer + total_written, n - total_written);
                if(written < 0 && errno == EINTR)
                {
                    continue;
                }
                if(written <= 0)
                {
                    cerr << "Write file failed" << endl;
                    close(file_fd);
                    return false;
                }
                total_written += written;
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
        else if(errno == EINTR)
        {
            continue;
        }
        else
        {
            perror("recv");

            close(file_fd);
            return false;
        }
    }

    if(close(file_fd) < 0)
    {
        perror("close file");
        return false;
    }

    cout << "File received successfully!" << endl;

    return true;
};

bool socket_server::send_all(int sock, const void* data, size_t len)
{
    const char* ptr = static_cast<const char*>(data);
    size_t total_sent = 0;
    while(total_sent < len)
    {
        ssize_t n = send(sock, ptr + total_sent, len - total_sent, 0);
        if(n < 0 && errno == EINTR)
        {
            continue;
        }
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
    size_t total_received = 0;
    while(total_received < len)
    {
        ssize_t n = recv(client_sock, ptr + total_received, len - total_received, 0);
        if(n < 0 && errno == EINTR)
        {
            continue;
        }
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
    int opt = 1;
    if(setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        close(server_sock);
        exit(1);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);
    if (::bind(server_sock, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_sock);
        exit(1);
    }
}

void socket_server::start()
{
    if (listen(server_sock, 5) < 0) {
        perror("listen");
        exit(1);
    }
    cout << "Server is listening on port 8080" << endl;
    while(true)
    {
        int client_sock = accept(server_sock, nullptr, nullptr);
        if(client_sock < 0 && errno == EINTR)
        {
            continue;
        }
        if (client_sock < 0) {
            perror("Accept failed");
            return;
        }
        pid_t pid = fork();
        if(pid > 0)
        {
            //父进程保留监听 fd，关闭本次连接的引用。
            close(client_sock);
            //回去等待下一个客户端
            continue;
        }
        else if(pid == 0)
        {
            //子进程只服务本次连接，关闭监听 fd。
            cout << "Client connected" << endl;
            close(server_sock);
            file_Info file{};

            if(!recv_all(client_sock, &file, sizeof(file)))
            {
                cout << "Receive file info failed" << endl;
                close(client_sock);
                _exit(1);
            }
            // 必须先验证终止符，再把网络字段作为 C 字符串使用。
            const char* end = static_cast<const char*>(memchr(file.file_name, '\0', sizeof(file.file_name)));
            if(end == nullptr || end == file.file_name || end - file.file_name > 250 ||
               strchr(file.file_name, '/') != nullptr)
            {
                cerr << "Invalid file name" << endl;
                close(client_sock);
                _exit(1);
            }
            cout << "file name: " << file.file_name << endl;
            cout << "file size: " << file.file_size << endl;
            const char ok[] = "OK";
            if(!send_all(client_sock, ok, 2))
            {
                cout << "Send ACK failed" << endl;
                close(client_sock);
                _exit(1);
            }
            if(!recv_file(client_sock, file))
            {
                cerr << "Receive file failed" << endl;
                close(client_sock);
                _exit(1);
            }
            close(client_sock);
            _exit(0);  // 不再回到父进程的 accept 循环。
        }
        else
        {
            perror("fork");
            close(client_sock);
        }
    }
}

socket_server::~socket_server()
{
    close(server_sock);
}

int main()
{
    signal(SIGPIPE, SIG_IGN);
    // 本练习不收集子进程状态，显式忽略 SIGCHLD 避免僵尸进程。
    // 后续练习可改为 SIGCHLD + waitpid(-1, ..., WNOHANG)。
    signal(SIGCHLD, SIG_IGN);
    socket_server server;
    server.start();
    return 0;
}
