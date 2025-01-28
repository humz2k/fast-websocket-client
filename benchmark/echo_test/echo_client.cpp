// raw_tcp_client.cpp
#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <host> <port>\n";
        return 1;
    }
    std::string host = argv[1];
    int port = std::atoi(argv[2]);

    struct addrinfo hints, *res;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res) !=
        0) {
        std::cerr << "Error: getaddrinfo() failed.\n";
        return 1;
    }

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        std::cerr << "Error: socket() failed.\n";
        freeaddrinfo(res);
        return 1;
    }

    if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        std::cerr << "Error: connect() failed.\n";
        close(sockfd);
        freeaddrinfo(res);
        return 1;
    }

    freeaddrinfo(res);

    std::string message = "ping";
    double min_rtt = INT32_MAX;
    double max_rtt = 0;
    double mean_rtt = 0;
    int samples = 0;

    for (int i = 0; i < 10000; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        if (send(sockfd, message.c_str(), message.size(), 0) < 0) {
            std::cerr << "Error: send() failed.\n";
            close(sockfd);
            return 1;
        }

        char buffer[4096];
        ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer), 0);
        if (bytes_received < 0) {
            std::cerr << "Error: recv() failed.\n";
            close(sockfd);
            return 1;
        }
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::micro> rtt = end - start;
        samples++;
        mean_rtt += rtt.count();
        min_rtt = std::min(min_rtt, rtt.count());
        max_rtt = std::max(max_rtt, rtt.count());
    }
    std::cout << "min rtt (us): " << min_rtt << std::endl;
    std::cout << "max rtt (us): " << max_rtt << std::endl;
    std::cout << "mean rtt (us): " << mean_rtt / ((double)samples) << std::endl;

    close(sockfd);
    return 0;
}
