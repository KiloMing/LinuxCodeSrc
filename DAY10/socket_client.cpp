#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
using namespace std;

class socket_client
{
private:
    /* data */
    int sock;
    struct sockaddr_in server_addr;
public:
    socket_client(/* args */);
    ~socket_client();
    int send_message(const char* msg);
    void receive_message(char* buffer, size_t size);
};

socket_client::socket_client(/* args */)
{
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Socket creation failed" << endl;
        exit(1);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080); // server port
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        cerr << "Invalid address/ Address not supported" << endl;
        exit(1);
    }
    if (connect(sock, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        cerr << "Connection failed" << endl;
        exit(1);
    }
}

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



int main()
{
    socket_client client;
    char buffer[1024];
    for(int i = 0; i < 5; i++)
    {
        if (client.send_message("hello server") == 0) {
            client.receive_message(buffer, sizeof(buffer));
            sleep(1);
        }
            
    }
    return 0;
}