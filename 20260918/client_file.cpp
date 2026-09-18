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
#include <stdexcept>

using namespace::std;

//client_server :
//
struct file_Info
{
    char file_name[256];
    uint64_t file_size;
};

class socket_client
{
private:
    /* data */
    int sock;
    struct sockaddr_in server_addr;
public:
    socket_client(char *argv[]);
    ~socket_client();
    bool file_init(char *argv[], file_Info &file);
    int send_message(const char* msg);
    int send_message(file_Info &file);
    void receive_message(char* buffer, size_t size);
    bool recv_all(int client_sock, void* data, size_t len);
    bool send_all(int sock, const void* data, size_t len);
    bool send_file(const char* filename);
    bool wait_ack();
};

bool socket_client::send_all(int sock, const void* data, size_t len)
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


bool socket_client::recv_all(int client_sock, void* data, size_t len)
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
            std::cout << "Client disconnected" << endl;
            return false;
        }
        total_received += n;
    }
    return true;
}

bool socket_client::file_init(char *argv[], file_Info &file)
{
    memset(&file, 0, sizeof(file));
    struct stat st;
    char *filename = argv[3];
    if(stat(filename, &st) < 0)
    {
        perror("stat");
        return false;
    }
    if(!S_ISREG(st.st_mode) || st.st_size < 0)
    {
        cerr << "Not a regular file" << endl;
        return false;
    }
    // 只传文件名；本地打开时仍使用 argv[3] 的完整路径。
    const char* base_name = strrchr(filename, '/');
    base_name = base_name ? base_name + 1 : filename;
    if(strlen(base_name) == 0 || strlen(base_name) > 250)
    {
        cerr << "File name must contain 1-250 bytes (reserve recv_ prefix)" << endl;
        return false;
    }
    strncpy(
        file.file_name,
        base_name,
        sizeof(file.file_name) - 1
    );
    file.file_size = st.st_size;
    return true;
}


socket_client::socket_client(char *argv[])
{
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Socket creation failed" << endl;
        exit(1);
    }
    int port;
    try
    {
        size_t used = 0;
        port = std::stoi(argv[2], &used);
        if(used != strlen(argv[2]) || port < 1 || port > 65535)
        {
            throw out_of_range("port");
        }
    }
    catch(const exception&)
    {
        cerr << "Invalid port: expected an integer from 1 to 65535" << endl;
        close(sock);
        exit(1);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port); // server port
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        cerr << "Invalid address/ Address not supported" << endl;
        exit(1);
    }
    if (connect(sock, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0)
    {
        cerr << "Connection failed" << endl;
        exit(1);
    }
}
//sent the string
int socket_client::send_message(const char* msg)
{
    ssize_t n = write(sock, msg, strlen(msg));
    if (n < 0)
    {
        cerr << "Send failed" << endl;
        return -1;
    }
    else
    {
        cout << "Sent " << n << " bytes" << endl;
    }
    return 0;
}

//sent the file struct
int socket_client::send_message(file_Info &file)
{
    if(!send_all(sock, &file, sizeof(file)))
    {
        cout << "send file error !" << endl;
        return 0;
    }
    cout << "sent successfully !" << endl;
    return 1;
}


void socket_client::receive_message(char* buffer, size_t size)
{
    ssize_t n = read(sock, buffer, size - 1);
    if (n < 0)
    {
        cerr << "Receive failed" << endl;
    }
    else
    {
        buffer[n] = '\0'; // null-terminate the received data
        cout << "Received " << n << " bytes: " << buffer << endl;
    }
}
socket_client::~socket_client()
{
    close(sock);
}

bool socket_client::send_file(const char* filename)
{
    int file_fd = open(filename, O_RDONLY);
    if(file_fd < 0)
    {
        perror("open file");
        return false;
    }
    char file_buffer[4096];

    while(true)
    {
        ssize_t n = read(file_fd, file_buffer, sizeof(file_buffer));
        if(n > 0)
        {
            if(!send_all(sock,file_buffer,n))
            {
                cout << "Send file failed!" << endl;

                close(file_fd);
                return false;
            }
        }
        else if(n == 0)
        {
            //file ending
            break;
        }
        else if(errno == EINTR)
        {
            continue;
        }
        else
        {
            perror("read");
            close(file_fd);
            return false;
        }
    }
    close(file_fd);
    cout << "File sent successfully!" << endl;
    return true;
}

bool socket_client::wait_ack()
{
    char ack[3] = {0};
    if(!recv_all(sock, ack, 2))
    {
        cerr << "Receive ACK failed" << endl;
        return false;
    }
    if(strcmp(ack, "OK") != 0)
    {
        cerr << "Invalid ACK" << endl;
        return false;
    }
    cout << "Server ACK: OK" << endl;
    return true;
}

int main(int argc, char *argv[])
{
    if(argc != 4)
    {
        cerr << "Usage: " << argv[0] << " <server_ip> <port> <filename>" << endl;
        return 1;
    }
    // 对端断开时让 send 返回 EPIPE，交给返回值检查处理。
    signal(SIGPIPE, SIG_IGN);
    socket_client client(argv);
    file_Info file1{};
    if(!client.file_init(argv, file1) || !client.send_message(file1))
    {
        return 1;
    }
    if(!client.wait_ack() || !client.send_file(argv[3]))
    {
        return 1;
    }
    return 0;
}
