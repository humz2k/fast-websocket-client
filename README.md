# fast-websocket-client
Lightweight header-only C++17 websocket client library. The plan is for this to be fully compliant with RFC6455 eventually.

## Dependencies
* C++17 or higher
* Boost (Boost.Pool)
* OpenSSL

## Building
> Make sure you init and update all the git submodules.

Use the included CMakeLists.txt, the single header version in `single_header/`, or just point your compiler to the fastws headers, `ext/plf_nanotimer` and `ext/websocket-frame-utility/include`, Boost headers, OpenSSL headers, and link with OpenSSL.

## Usage
### Client Types
There are two clients, `fastws::TLSClient` and `fastws::NoTLSClient` (which are specializations of `fastws::WSClient`), which are class templates that take a frame handler as the template argument (see below). Should be pretty obvious what the difference between these guys is.

### Frame Handler
To use fastws, you need to define a frame handler that responds to various WebSocket events, such as connection open, message received, connection close, etc. For example:
```c++
struct FrameHandler {
    void on_open(fastws::TLSClient<FrameHandler>& client) {}
    void on_close(fastws::TLSClient<FrameHandler>& client, bool success) {}
    void on_text(fastws::TLSClient<FrameHandler>& client, wsframe::Frame frame) {}
    void on_binary(fastws::TLSClient<FrameHandler>& client, wsframe::Frame frame) {}
    void on_continuation(fastws::TLSClient<FrameHandler>& client, wsframe::Frame frame) {}
};
```
All methods (`on_open`,`on_close`,`on_text`,...) need to be defined.

A `wsframe::Frame` looks like. The payload `string_view` is only valid for the duration of the FrameHandler method call, so if you want to keep it around you should copy it somewhere.
```c++
struct Frame{
    enum class Opcode : uint8_t {
        CONTINUATION = 0x0,
        TEXT = 0x1,
        BINARY = 0x2,
        CLOSE = 0x8,
        PING = 0x9,
        PONG = 0xA,
        UNKNOWN
    };

    bool fin;
    bool mask;
    Opcode opcode;
    std::array<std::uint8_t, 4> masking_key;
    std::string_view payload;
};
```

### Launching the client
We first create a FrameHandler instance, then pass a reference to this to the client (along with the url, path and port to connect to). Then just call `client.poll()` in a loop to handle incoming packets.
```c++
int main() {
    FrameHandler handler;
    fastws::TLSClient<FrameHandler> client(handler, "echo.websocket.org", "/", 443);
    while (true)
        if (client.poll() != fastws::ConnectionStatus::HEALTHY)
            break;
    return 0;
}
```
The constructor for `fastws::WSCLient` (and so `fastws::TLSClient` and `fastws::NoTLSCLient`) has the signature
```c++
WSClient(FrameHandler& handler, const std::string& host,
        const std::string& path, const long port = 443,
        const std::string& extra_headers = "",
        int connection_timeout = 10 /*seconds*/,
        int ping_frequency = 60 /*seconds*/,
        int ping_timeout = 10 /*seconds*/);
```
where `connection_timeout` is how long the client will wait to recieve the open connection handshake, `ping_frequency` is how often the client sends a ping to the server, and `ping_timeout` is how long the client will wait to receive a pong from the server before giving up.

### Client Methods
`fastws::WSClient` (which is passed to all of the handler functions) has the public methods
```c++
// get current status of the connection
ConnectionStatus fastws::WSClient::status() const;

// closes the connection
bool fastws::WSClient::close(int timeout = 10 /*seconds*/);

// sends text
void fastws::WSClient::send_text(std::string_view payload);

// sends binary
void fastws::WSClient::send_binary(std::string_view payload);

// handles incoming packets and returns current status of the connection
ConnectionStatus fastws::WSClient::poll();
```

### Minimal Example
This is a minimal example that connects to `echo.websocket.org`, sends a message, and then closes the connection once the echo is recieved.
```c++
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
        if (client.poll() != fastws::ConnectionStatus::HEALTHY)
            break;
    return 0;
}
```