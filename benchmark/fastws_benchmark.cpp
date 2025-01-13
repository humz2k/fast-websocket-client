#include <fastws/fastws.hpp>

#include "benchmark.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <string_view>

struct FrameHandler {
    using Client = fastws::NoTLSClient<FrameHandler>;
    int count = 0;
    int max_count = NSENDS;
    std::chrono::high_resolution_clock::time_point start;
    std::chrono::high_resolution_clock::time_point end;

    void on_open(Client& client) {
        start = std::chrono::high_resolution_clock::now();
        client.send_text(MESSAGE);
    }

    void on_text(Client& client, wsframe::Frame frame) {
        if (frame.payload == MESSAGE) {
            count++;
            if (count >= max_count) {
                end = std::chrono::high_resolution_clock::now();
                double total_time =
                    std::chrono::duration_cast<std::chrono::nanoseconds>(end -
                                                                         start)
                        .count();
                std::cout << "total time (us): " << total_time * 0.001
                          << std::endl;
                std::cout << "average_rtt (us): "
                          << (total_time / (double)max_count) * 0.001
                          << std::endl;
                client.close();
                return;
            }
            client.send_text(MESSAGE);
        }
    }

    void on_close(Client& client, bool success) {}
    void on_binary(Client& client, wsframe::Frame frame) {}
    void on_continuation(Client& client, wsframe::Frame frame) {}
};

int main() {
    set_max_priority();
    auto start = std::chrono::high_resolution_clock::now();
    while (true) {
        std::string command;
        std::cin >> command;
        if (command == "go")
            break;
    }
    FrameHandler handler;
    FrameHandler::Client client(handler, "127.0.0.1", "/", 9001);
    while (true)
        if (client.poll() != fastws::ConnectionStatus::HEALTHY)
            break;
    auto end = std::chrono::high_resolution_clock::now();
    double total_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
            .count();
    std::cout << "TOTAL_TIME=" << total_time << "ms" << std::endl;
    return 0;
}