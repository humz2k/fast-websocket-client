# fast-websocket-client

WIP C++17 low latency websocket client. The plan is for this to be fully compliant with RFC6455 eventually. Very very very basic benchmarks have the current version being ~20% faster than Websocket++ (with the specific intended usecase of a single-threaded non-blocking client).

The main goal here is to do minimal allocations and minimal copies.

## Usage
I will eventually write some better documentation, but here is a working example:
```c++
#include <fastws/fastws.hpp>

#include <iostream>
#include <string>
#include <string_view>

struct FrameHandler {
    using Client = fastws::SSLClient<FrameHandler>;
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
        if (frame.payload == "Hello, World!") {
            client.close();
        }
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
        if (!client.poll())
            break;
    return 0;
}
```