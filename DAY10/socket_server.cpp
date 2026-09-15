#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
using namespace std;    

class socket_server
{
private:
    int server_sock;
    struct sockaddr_in server_addr;
public:
    socket_server();
    ~socket_server();
    void start();
};

void socket_server::start()
{
    if (listen(server_sock, 5) < 0) {
        cerr << "Listen failed" << endl;
        return;
    }

    cout << "Server is listening on port 8080" << endl;
    while(true)
    {
        pid_t pid = fork();
        if (pid < 0) {
            cerr << "Fork failed" << endl;
            return;
        }
        if (pid == 0) {
            close(server_sock); // Child doesn't need the listener
        } else {
            continue; // Parent goes back to accept new connections
        }

        int client_sock = accept(server_sock, nullptr, nullptr);
        if (client_sock < 0) {
            cerr << "Accept failed" << endl;
            return;
        }

    cout << "Client connected" << endl;
    char buffer[1024];

    while (true)
    {
        ssize_t n = read(
            client_sock,
            buffer,
            sizeof(buffer) - 1
        );

        if (n > 0)
        {
            cout << "Received: "
                << n
                << ": " << string(buffer, n)
                << endl;
            ssize_t sent = write(client_sock, buffer, n);
            if (sent < 0)
            {
                cerr << "Write failed" << endl;
                break;
            }
        }
        else if (n == 0)
        {
            cout << "Client disconnected" << endl;
            break;
        }
        else
        {
            cerr << "Recv failed" << endl;
            break;
        }
    }
    close(client_sock);
    }
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