/*
 * Copyright (c) 2016, Peter Thorson. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the WebSocket++ Project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PETER THORSON BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

#include "benchmark.hpp"

#include <chrono>
#include <iostream>

typedef websocketpp::client<websocketpp::config::asio_client> client;

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

// pull out the type of messages sent by our config
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

struct Handler {
    int count = 0;
    int max_count = NSENDS;
    std::chrono::high_resolution_clock::time_point start;
    std::chrono::high_resolution_clock::time_point end;

    void on_message(client* c, websocketpp::connection_hdl hdl,
                    message_ptr msg) {
        if (msg->get_payload() == MESSAGE) {
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

                websocketpp::lib::error_code ec;
                c->close(hdl, websocketpp::close::status::going_away, "", ec);
                return;
            }
            websocketpp::lib::error_code ec;
            c->send(hdl, MESSAGE, websocketpp::frame::opcode::text, ec);
        }
    }

    void on_open(websocketpp::connection_hdl hdl, client* c) {
        websocketpp::lib::error_code ec;
        start = std::chrono::high_resolution_clock::now();
        c->send(hdl, MESSAGE, websocketpp::frame::opcode::text, ec);
    }
};

int main(int argc, char* argv[]) {
    set_max_priority();
    while (true) {
        std::string command;
        std::cin >> command;
        if (command == "go")
            break;
    }
    // Create a client endpoint
    client c;

    std::string uri = "ws://localhost:9001";

    if (argc == 2) {
        uri = argv[1];
    }

    try {
        c.set_access_channels(websocketpp::log::alevel::none);

        // Initialize ASIO
        c.init_asio();

        Handler handler;

        // Register our message handler
        c.set_message_handler(
            bind(&Handler::on_message, &handler, &c, ::_1, ::_2));
        c.set_open_handler(bind(&Handler::on_open, &handler, ::_1, &c));

        websocketpp::lib::error_code ec;
        client::connection_ptr con = c.get_connection(uri, ec);
        if (ec) {
            std::cout << "could not create connection because: " << ec.message()
                      << std::endl;
            return 0;
        }

        c.connect(con);
        c.run();
    } catch (websocketpp::exception const& e) {
        std::cout << e.what() << std::endl;
    }
}