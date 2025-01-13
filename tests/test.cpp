#include <chrono>
#include <fastws/fastws.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <cstdint>

struct FrameHandler1_1 {
    using Client = fastws::WSClient<fastws::SocketWrapper, FrameHandler1_1>;
    std::string initial_message;
    FrameHandler1_1(std::string initial_message_) : initial_message(initial_message_){}
    void on_open(Client& client) { client.send_text(initial_message); }
    void on_close(Client& client, bool success) {}
    void on_text(Client& client, wsframe::Frame frame) {}
    void on_binary(Client& client, wsframe::Frame frame) {}
    void on_continuation(Client& client, wsframe::Frame frame) {}
};

void run_test_1_1(uint64_t message_len, std::string path, std::string ip = "127.0.0.1", long port = 9001){
    std::string initial_message = "";
    for (uint64_t i = 0; i < message_len; i++){
        initial_message += "*";
    }
    FrameHandler1_1 handler(initial_message);
    FrameHandler1_1::Client client(handler, ip,
                                path, port);
    while (true)
        if (client.poll() != fastws::ConnectionStatus::HEALTHY)break;
}

void test_1_1_1(std::string path = "/runCase?case=1&agent=myagent", std::string ip = "127.0.0.1", long port = 9001){
    run_test_1_1(0,path,ip,port);
}

void test_1_1_2(std::string path = "/runCase?case=2&agent=myagent", std::string ip = "127.0.0.1", long port = 9001){
    run_test_1_1(125,path,ip,port);
}

void test_1_1_3(std::string path = "/runCase?case=3&agent=myagent", std::string ip = "127.0.0.1", long port = 9001){
    run_test_1_1(126,path,ip,port);
}

void test_1_1_4(std::string path = "/runCase?case=4&agent=myagent", std::string ip = "127.0.0.1", long port = 9001){
    run_test_1_1(127,path,ip,port);
}

void test_1_1_5(std::string path = "/runCase?case=5&agent=myagent", std::string ip = "127.0.0.1", long port = 9001){
    run_test_1_1(128,path,ip,port);
}

void test_1_1_6(std::string path = "/runCase?case=6&agent=myagent", std::string ip = "127.0.0.1", long port = 9001){
    run_test_1_1(65535ULL,path,ip,port);
}

void test_1_1_7(std::string path = "/runCase?case=7&agent=myagent", std::string ip = "127.0.0.1", long port = 9001){
    run_test_1_1(65536ULL,path,ip,port);
}

struct FrameHandler1_2 {
    using Client = fastws::WSClient<fastws::SocketWrapper, FrameHandler1_2>;
    std::string initial_message;
    FrameHandler1_2(std::string initial_message_) : initial_message(initial_message_){}
    void on_open(Client& client) { client.send_binary(initial_message); }
    void on_close(Client& client, bool success) {}
    void on_text(Client& client, wsframe::Frame frame) {}
    void on_binary(Client& client, wsframe::Frame frame) {}
    void on_continuation(Client& client, wsframe::Frame frame) {}
};

void run_test_1_2(uint64_t message_len, std::string path, std::string ip = "127.0.0.1", long port = 9001){
    std::string initial_message = "";
    for (uint64_t i = 0; i < message_len; i++){
        initial_message += "\xfe";
    }
    FrameHandler1_2 handler(initial_message);
    FrameHandler1_2::Client client(handler, ip,
                                path, port);
    while (true)
        if (client.poll() != fastws::ConnectionStatus::HEALTHY)break;
}

void test_1_2_1(std::string path = "/runCase?case=9&agent=myagent", std::string ip = "127.0.0.1", long port = 9001){
    run_test_1_2(0,path,ip,port);
}

void test_1_2_2(std::string path = "/runCase?case=10&agent=myagent", std::string ip = "127.0.0.1", long port = 9001){
    run_test_1_2(125,path,ip,port);
}

void test_1_2_3(std::string path = "/runCase?case=11&agent=myagent", std::string ip = "127.0.0.1", long port = 9001){
    run_test_1_2(126,path,ip,port);
}

void test_1_2_4(std::string path = "/runCase?case=12&agent=myagent", std::string ip = "127.0.0.1", long port = 9001){
    run_test_1_2(127,path,ip,port);
}

void test_1_2_5(std::string path = "/runCase?case=13&agent=myagent", std::string ip = "127.0.0.1", long port = 9001){
    run_test_1_2(128,path,ip,port);
}

void test_1_2_6(std::string path = "/runCase?case=14&agent=myagent", std::string ip = "127.0.0.1", long port = 9001){
    run_test_1_2(65535ULL,path,ip,port);
}

void test_1_2_7(std::string path = "/runCase?case=15&agent=myagent", std::string ip = "127.0.0.1", long port = 9001){
    run_test_1_2(65536ULL,path,ip,port);
}

int main() {
    test_1_1_1();
    test_1_1_2();
    test_1_1_3();
    test_1_1_4();
    test_1_1_5();
    test_1_1_6();
    test_1_1_7();
    test_1_2_1();
    test_1_2_2();
    test_1_2_3();
    test_1_2_4();
    test_1_2_5();
    test_1_2_6();
    test_1_2_7();
    return 0;
}