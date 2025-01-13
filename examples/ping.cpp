#include <fastws/fastws.hpp>

#include <iostream>
#include <string>
#include <string_view>

struct FrameHandler {
    using Client = fastws::TLSClient<FrameHandler>;
    void on_open(Client& client) {
        std::cout << "Connection Opened!" << std::endl;
        client.send_text("Hello, World!");
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

int main() {
    FrameHandler handler;
    FrameHandler::Client client(handler, "echo.websocket.org", "/", 443);
    while (true)
        if (client.poll() != fastws::ConnectionStatus::HEALTHY)
            break;
    return 0;
}