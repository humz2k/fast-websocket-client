#pragma once

#ifndef _FASTWS_FASTWS_HPP_
#define _FASTWS_FASTWS_HPP_

#include "handshake.hpp"
#include "plf_nanotimer.h"
#include "socket_wrapper.hpp"
#include "wsframe/wsframe.hpp"

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace fastws {

enum ConnectionStatus {
    HEALTHY = 0,
    CLOSED_BY_SERVER,
    CLOSED_BY_CLIENT,
    PING_TIMED_OUT,
    FAILED,
    UNKNOWN
};

template <template <bool> class SocketType, class FrameHandler> class WSClient {
  private:
    FrameHandler& m_handler;
    std::string m_host;
    std::string m_path;
    long m_port;
    std::string m_extra_headers;
    SocketType<false> m_socket;
    wsframe::FrameParser m_parser;
    wsframe::FrameFactory m_factory;
    ConnectionStatus m_status = ConnectionStatus::UNKNOWN;
    bool m_connection_open = false;

    bool connect(int timeout = 10 /*seconds*/) {
        m_socket = SocketType<false>(m_host, m_port);
        auto host = m_host;
        if (m_port != 443) {
            host += ":" + std::to_string(m_port);
        }
        auto request = fastws::build_websocket_handshake_request(
            host, m_path, fastws::generate_sec_websocket_key(),
            m_extra_headers);
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
            m_status = ConnectionStatus::HEALTHY;
            m_handler.on_open(*this);
        } else {
            m_status = ConnectionStatus::FAILED;
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

    plf::nanotimer m_ping_timer;
    bool m_waiting_for_ping = false;
    const double m_ping_every;   // ms
    const double m_ping_timeout; // ms
    double m_last_rtt = 0;

    void handle_pong(std::string_view payload = {}) {
        m_last_rtt = m_ping_timer.get_elapsed_ms();
        m_ping_timer.start();
        m_waiting_for_ping = false;
    }

    void update_ping() {
        if (m_waiting_for_ping) {
            if (m_ping_timer.get_elapsed_ms() > m_ping_timeout) {
                m_connection_open = false;
                m_status = ConnectionStatus::PING_TIMED_OUT;
                std::cout << "ping timed out" << std::endl;
            }
        } else {
            if (m_ping_timer.get_elapsed_ms() > m_ping_every) {
                m_waiting_for_ping = true;
                m_ping_timer.start();
                send_ping();
            }
        }
    }

  public:
    WSClient(FrameHandler& handler, const std::string& host,
             const std::string& path, const long port = 443,
             const std::string& extra_headers = "",
             int connection_timeout = 10 /*seconds*/,
             int ping_frequency = 60 /*seconds*/,
             int ping_timeout = 10 /*seconds*/)
        : m_handler(handler), m_host(host), m_path(path), m_port(port),
          m_extra_headers(extra_headers),
          m_ping_every(((double)ping_frequency) * 1000.0),
          m_ping_timeout(((double)ping_timeout) * 1000.0) {
        if (!connect(connection_timeout)) {
            throw std::runtime_error("Failed to connect to ws server");
        }
        m_ping_timer.start();
    }

    ConnectionStatus status() const { return m_status; }

    bool close(int timeout = 10 /*seconds*/) {
        if (!m_connection_open)
            return true;
        poll();
        m_parser.clear();
        send_close();
        m_status = ConnectionStatus::CLOSED_BY_CLIENT;
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

    ~WSClient() { close(); }

    void send_text(std::string_view payload) {
        send(m_factory.text(true, true, payload));
    }

    void send_binary(std::string_view payload) {
        send(m_factory.binary(true, true, payload));
    }

    ConnectionStatus poll() {
        for (auto parsed_frame =
                 m_parser.update(m_socket.read_into(m_parser.frame_buffer()));
             parsed_frame.has_value(); parsed_frame = m_parser.update(false)) {
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
            case wsframe::Frame::Opcode::PONG:
                handle_pong(frame.payload);
                break;
            case wsframe::Frame::Opcode::CLOSE:
                m_connection_open = false;
                m_status = ConnectionStatus::CLOSED_BY_SERVER;
                send_close();
                m_handler.on_close(*this, true);
                return m_status;
            default:
                m_handler.on_continuation(*this, std::move(frame));
                break;
            }
        }
        update_ping();
        return m_status;
    }

    double last_rtt() const { return m_last_rtt; }
};

template <class FrameHandler>
using TLSClient = WSClient<SSLSocketWrapper, FrameHandler>;

template <class FrameHandler>
using NoTLSClient = WSClient<SocketWrapper, FrameHandler>;

} // namespace fastws

#endif // _FASTWS_FASTWS_HPP_