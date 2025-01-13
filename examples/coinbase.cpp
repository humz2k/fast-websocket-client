#include <fastws/fastws.hpp>

#include <iostream>
#include <signal.h>
#include <string>
#include <string_view>

struct FrameHandler {
    using Client = fastws::TLSClient<FrameHandler>;
    void on_open(Client& client) {
        std::cout << "Connection Opened!" << std::endl;
        client.send_text("{\"type\":\"subscribe\",\"product_ids\":[\"BTC-USD\"]"
                         ",\"channels\":[\"ticker\",\"heartbeat\"]}");
    }
    void on_close(Client& client, bool success) {
        std::cout << "Connection Closed (success = " << success << ")"
                  << std::endl;
    }
    void on_text(Client& client, wsframe::Frame frame) {
        std::cout << " > text: " << frame << std::endl;
    }
    void on_binary(Client& client, wsframe::Frame frame) {
        std::cout << " > binary: " << frame << std::endl;
    }
    void on_continuation(Client& client, wsframe::Frame frame) {
        std::cout << " > continuation: " << frame << std::endl;
    }
};

bool should_run = true;
void quit_handler(int s) { should_run = false; }

int main() {
    signal(SIGINT, quit_handler);

    FrameHandler handler;
    FrameHandler::Client client(handler, "ws-feed.exchange.coinbase.com", "/",
                                443);
    while (should_run)
        if (client.poll() != fastws::ConnectionStatus::HEALTHY)
            break;
    return 0;
}