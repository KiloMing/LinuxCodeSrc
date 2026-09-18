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

using namespace::std;

//client_server :
//
struct file_Info
{
    char file_name[256];
    uint64_t file_size;
};

bool send_all(int sock, const void* data, size_t len)
{
    const char* ptr = static_cast<const char*>(data);
    ssize_t total_sent = 0;
    while(total_sent < len) 
    {
        size_t n = send(sock, ptr + total_sent, len - total_sent, 0);
        if(n <= 0)
        {
            return false;
        }
        total_sent += n;
    }   
    return true;
}

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
};

bool socket_client::send_all(int sock, const void* data, size_t len)
{
    const char* ptr = static_cast<const char*>(data);
    ssize_t total_sent = 0;
    while(total_sent < len) 
    {
        size_t n = send(sock, ptr + total_sent, len - total_sent, 0);
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
    if(stat(filename, &st))        // ???
    {
        perror("stat");
        return false;
    }
    strncpy(
        file.file_name,
        filename,
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
    int port = std::stoi(argv[2]);
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

int main(int argc, char *argv[])
{
    socket_client client(argv);
    char buffer[1024];
    file_Info file1;
    client.file_init(argv, file1);
    client.send_message(file1);
    char recv_buffer[1024];
    client.receive_message(buffer, sizeof(buffer));
    if(strcmp(buffer, "OK") == 0){
        cout << "Client: OK" << endl;
        client.send_file(file1.file_name);
    }
    return 0;
}