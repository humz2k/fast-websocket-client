#ifndef _FASTWS_FASTWS_HPP_
#define _FASTWS_FASTWS_HPP_

#include "handshake.hpp"
#include "socket_wrapper.hpp"
#include <wsframe/wsframe.hpp>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace fastws {

template <class FrameHandler> class Client {
  private:
    FrameHandler& m_handler;
    std::string m_host;
    std::string m_path;
    long m_port;
    SocketWrapper<false> m_socket;
    wsframe::FrameParser m_parser;
    wsframe::FrameFactory m_factory;
    bool m_connection_open = false;

    bool connect(int timeout = 10 /*seconds*/) {
        m_socket = SocketWrapper<false>(m_host, m_port);
        auto request = fastws::build_websocket_handshake_request(
            m_host, m_path, fastws::generate_sec_websocket_key());
        m_socket.send(request);
        std::string response = "";
        for (int i = 0; i < timeout * 10; i++) {
            response += m_socket.read(4096);
            if (response.find("\r\n\r\n") != std::string::npos) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        m_connection_open = response.find("HTTP/1.1 101 Switching Protocols") !=
                            std::string::npos;
        if (m_connection_open) {
            m_handler.on_open(*this);
        }
        return m_connection_open;
    }

    void send(const std::string_view& frame) { m_socket.send(frame); }

    void send_pong(std::string_view payload) {
        send(m_factory.pong(true, payload));
    }

    void send_close(std::string_view payload = {}) {
        send(m_factory.close(true, payload));
    }

    void send_ping(std::string_view payload = {}) {
        send(m_factory.ping(true, payload));
    }

  public:
    Client(FrameHandler& handler, const std::string& host,
           const std::string& path, const long port = 443)
        : m_handler(handler), m_host(host), m_path(path), m_port(port) {
        if (!connect()) {
            throw std::runtime_error("Failed to connect to ws server");
        }
    }

    bool close(int timeout = 10 /*seconds*/) {
        if (!m_connection_open)
            return true;
        poll();
        m_parser.clear();
        send_close();
        m_connection_open = false;
        bool success = false;
        for (int i = 0; i < timeout * 10; i++) {
            auto frame = m_parser.update(m_socket.read(1024));
            if (frame) {
                if (frame->opcode == wsframe::Frame::Opcode::CLOSE) {
                    success = true;
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        m_handler.on_close(*this, success);
        return success;
    }

    ~Client() { close(); }

    void send_text(std::string_view payload) {
        send(m_factory.text(true, true, payload));
    }

    void send_binary(std::string_view payload) {
        send(m_factory.binary(true, true, payload));
    }

    bool poll() {
        for (auto parsed_frame = m_parser.update(m_socket.read(1024));
             parsed_frame.has_value(); parsed_frame = m_parser.update({})) {
            auto frame = parsed_frame.value();
            switch (frame.opcode) {
            case wsframe::Frame::Opcode::TEXT:
                m_handler.on_text(*this, std::move(frame));
                break;
            case wsframe::Frame::Opcode::BINARY:
                m_handler.on_binary(*this, std::move(frame));
                break;
            case wsframe::Frame::Opcode::PING:
                send_pong(frame.payload);
                break;
            case wsframe::Frame::Opcode::CLOSE:
                m_connection_open = false;
                send_close();
                m_handler.on_close(*this, true);
                break;
            default:
                m_handler.on_continuation(*this, std::move(frame));
                break;
            }
        }
        return m_connection_open;
    }
};

} // namespace fastws

#endif // _FASTWS_FASTWS_HPP_