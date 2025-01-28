#include <fastws/fastws.hpp>

#include "benchmark.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <string_view>

struct FrameHandler {
    using Client = fastws::NoTLSClient<FrameHandler>;
    void on_open(Client& client) {
        client.send_text("fastws");
    }
    void on_text(Client& client, wsframe::Frame frame) {
        client.send_text(frame.payload);
    }
    void on_close(Client& client, bool success) {}
    void on_binary(Client& client, wsframe::Frame frame) {}
    void on_continuation(Client& client, wsframe::Frame frame) {}
};

int main() {
    set_max_priority();
    FrameHandler handler;
    FrameHandler::Client client(handler, "127.0.0.1", "/", 8765);
    while (true)
        if (client.poll() != fastws::ConnectionStatus::HEALTHY)
            break;
    return 0;
}