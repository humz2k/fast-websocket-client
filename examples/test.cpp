#include <chrono>
#include <fastws/fastws.hpp>
#include <iostream>
#include <signal.h>
#include <string>
#include <thread>

struct FrameHandler {
    void on_open(fastws::Client<FrameHandler>& client) {
        std::cout << "connection opened" << std::endl;
        client.send_text("Hello, World!");
    }
    void on_close(fastws::Client<FrameHandler>& client, bool success) {
        std::cout << "connection closed, success = " << success << std::endl;
    }
    void on_text(fastws::Client<FrameHandler>& client, wsframe::Frame frame) {
        std::cout << "text: " << frame << std::endl;
        if (frame.payload == "Hello, World!") {
            client.close();
        }
    }
    void on_binary(fastws::Client<FrameHandler>& client, wsframe::Frame frame) {
        std::cout << "binary: " << frame << std::endl;
    }
    void on_continuation(fastws::Client<FrameHandler>& client,
                         wsframe::Frame frame) {
        std::cout << "contiunation: " << frame << std::endl;
    }
};

bool should_run = true;
void quit_handler(int s) { should_run = false; }

int main() {
    signal(SIGINT, quit_handler);
    FrameHandler handler;
    fastws::Client client(handler, "echo.websocket.org", "/");
    while (should_run) {
        if (!client.poll())
            break;
    }
    return 0;
}