#include <fastws/fastws.hpp>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

int main(){
    std::string host = "echo.websocket.org";
    std::string path = "/";
    std::string key = fastws::generate_sec_websocket_key();
    auto request = fastws::build_websocket_handshake_request(host,path,key);
    std::cout << request << std::endl;

    fastws::SocketWrapper<false> socket("echo.websocket.org");
    socket.send(request);
    for (int i = 0; i < 10; i++){
        std::cout << "reading..." << std::endl;
        std::cout << socket.read(4096);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}