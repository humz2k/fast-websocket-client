#include <fastws/fastws.hpp>

#include <chrono>
#include <iostream>
#include <string>
#include <string_view>

struct FrameHandler {
    using Client = fastws::Client<FrameHandler>;
    int count = 0;
    int max_count = 10000;
    std::chrono::high_resolution_clock::time_point start;
    std::chrono::high_resolution_clock::time_point end;

    void on_open(Client& client) {
        start = std::chrono::high_resolution_clock::now();
        client.send_text("ping");
    }

    void on_text(Client& client, wsframe::Frame frame) {
        if (frame.payload == "ping"){
            count++;
            if (count >= max_count) {
                end = std::chrono::high_resolution_clock::now();
                double total_time =
                    std::chrono::duration_cast<std::chrono::nanoseconds>(end -
                                                                        start)
                        .count();
                std::cout << "total time (us): " << total_time * 0.001 << std::endl;
                std::cout << "average_rtt (us): "
                        << (total_time / (double)max_count) * 0.001 << std::endl;
                client.close();
                return;
            }
            client.send_text("ping");
        }
    }

    void on_close(Client& client, bool success) {}
    void on_binary(Client& client, wsframe::Frame frame) {}
    void on_continuation(Client& client, wsframe::Frame frame) {}
};

int main() {
    FrameHandler handler;
    FrameHandler::Client client(handler, "127.0.0.1", "/", 9001);
    while (true)
        if (!client.poll())
            break;
    return 0;
}