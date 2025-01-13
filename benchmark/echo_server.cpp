#include <arpa/inet.h> // for inet_ntoa
#include <cstdlib>     // for std::atoi
#include <cstring>     // for std::memset
#include <iostream>
#include <netdb.h>      // for gethostbyname
#include <netinet/in.h> // for sockaddr_in
#include <string>
#include <sys/socket.h> // for socket functions
#include <unistd.h>     // for close()

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <port>\n";
        return 1;
    }
    int port = std::atoi(argv[1]);

    int server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Error: socket() failed.\n";
        return 1;
    }

    int optval = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // listen on any interface
    server_addr.sin_port = htons(port);

    if (::bind(server_fd, reinterpret_cast<sockaddr*>(&server_addr),
               sizeof(server_addr)) < 0) {
        std::cerr << "Error: bind() failed.\n";
        close(server_fd);
        return 1;
    }

    if (::listen(server_fd, 5) < 0) {
        std::cerr << "Error: listen() failed.\n";
        close(server_fd);
        return 1;
    }

    std::cout << "Echo server listening on port " << port << "...\n";

    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = ::accept(
            server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
        if (client_fd < 0) {
            std::cerr << "Error: accept() failed.\n";
            continue;
        }

        std::cout << "Client connected: " << inet_ntoa(client_addr.sin_addr)
                  << ":" << ntohs(client_addr.sin_port) << "\n";

        char buffer[4096];
        while (true) {
            ssize_t bytes_read = ::recv(client_fd, buffer, sizeof(buffer), 0);
            if (bytes_read <= 0) {
                break;
            }
            ssize_t bytes_sent = ::send(client_fd, buffer, bytes_read, 0);
            if (bytes_sent < 0) {
                std::cerr << "Error: send() failed.\n";
                break;
            }
        }

        std::cout << "Client disconnected.\n";
        ::close(client_fd);
    }

    close(server_fd);
    return 0;
}
