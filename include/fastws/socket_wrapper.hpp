#ifndef _FASTWS_SOCKET_WRAPPER_HPP_
#define _FASTWS_SOCKET_WRAPPER_HPP_

#include "wsframe/wsframe.hpp"

#include <boost/pool/pool_alloc.hpp>

#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace fastws {

// messages are returned as a fastrest::string which uses a pool allocator
using string = std::basic_string<char, std::char_traits<char>,
                                 boost::fast_pool_allocator<char>>;

class SSLSocketWrapperException : public std::runtime_error {
  public:
    explicit SSLSocketWrapperException(const std::string& msg)
        : std::runtime_error(msg) {}
};

template <bool verbose = false> class SSLSocketWrapper {
  private:
    // the url of the host we are making requests to
    std::string m_host;
    long m_port;

    // socket
    int m_sockfd = -1;

    // ssl socket, the thing we actually use
    int m_sslsock = -1;

    // ssl shit
    SSL_CTX* m_ctx = nullptr;
    SSL* m_ssl = nullptr;

    string m_out;

    // dumb way to print ssl errors
    std::string get_ssl_error() {
        std::string out = "";
        int err;
        while ((err = ERR_get_error())) {
            char* str = ERR_error_string(err, 0);
            if (str)
                out += std::string(str);
        }
        return out;
    }

    void connect() {
        // reserve 1000 bytes for the out thingy
        m_out.reserve(1000);

        // lots of boring socket config stuff, so much boilerplate
        struct addrinfo hints = {}, *addrs;

        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        if (int rc = getaddrinfo(m_host.c_str(), std::to_string(m_port).c_str(),
                                 &hints, &addrs);
            rc != 0) {
            throw SSLSocketWrapperException(std::string(gai_strerror(rc)));
        }

        for (addrinfo* addr = addrs; addr != NULL; addr = addr->ai_next) {
            m_sockfd =
                socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

            if (m_sockfd == -1)
                break;

            // set nonblocking socket flag
            int flag = 1;
            if (setsockopt(m_sockfd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag,
                           sizeof(int)) < 0) {
                std::cerr << "Error setting TCP_NODELAY" << std::endl;
            } else {
                if constexpr (verbose) {
                    std::cout << "Set TCP_NODELAY" << std::endl;
                }
            }

            // try and connect
            if (::connect(m_sockfd, addr->ai_addr, addr->ai_addrlen) == 0)
                break;

            close(m_sockfd);
            m_sockfd = -1;
        }

        if (m_sockfd == -1)
            throw SSLSocketWrapperException("Failed to connect to server.");

        // ssl boilerplate
        const SSL_METHOD* meth = TLS_client_method();
        m_ctx = SSL_CTX_new(meth);
        m_ssl = SSL_new(m_ctx);

        if (!m_ssl)
            throw SSLSocketWrapperException("Failed to create SSL.");

        SSL_set_tlsext_host_name(m_ssl, m_host.c_str());

        m_sslsock = SSL_get_fd(m_ssl);
        SSL_set_fd(m_ssl, m_sockfd);

        if (SSL_connect(m_ssl) <= 0) {
            throw SSLSocketWrapperException(get_ssl_error());
        }

        if constexpr (verbose) {
            std::cout << "SSL connection using " << SSL_get_cipher(m_ssl)
                      << std::endl;
        }

        freeaddrinfo(addrs);

        fcntl(m_sockfd, F_SETFL, O_NONBLOCK);
    }

    void disconnect() {
        if (!(m_sockfd < 0))
            close(m_sockfd);
        if (m_ctx)
            SSL_CTX_free(m_ctx);
        if (m_ssl) {
            SSL_shutdown(m_ssl);
            SSL_free(m_ssl);
        }
    }

  public:
    SSLSocketWrapper(const std::string host, const long port = 443)
        : m_host(host), m_port(port) {
        connect();
    }

    SSLSocketWrapper() {}

    SSLSocketWrapper(const SSLSocketWrapper&) = delete;
    SSLSocketWrapper& operator=(const SSLSocketWrapper&) = delete;

    SSLSocketWrapper(SSLSocketWrapper&& other)
        : m_host(std::move(other.m_host)), m_sockfd(other.m_sockfd),
          m_sslsock(other.m_sslsock), m_ctx(other.m_ctx), m_ssl(other.m_ssl),
          m_out(std::move(other.m_out)) {
        other.m_sockfd = -1;
        other.m_sslsock = -1;
        other.m_ctx = nullptr;
        other.m_ssl = nullptr;
    }

    SSLSocketWrapper& operator=(SSLSocketWrapper&& other) {
        disconnect();

        m_host = std::move(other.m_host);
        m_sockfd = other.m_sockfd;
        m_sslsock = other.m_sslsock;
        m_ctx = other.m_ctx;
        m_ssl = other.m_ssl;
        m_out = std::move(other.m_out);

        other.m_sockfd = -1;
        other.m_sslsock = -1;
        other.m_ctx = nullptr;
        other.m_ssl = nullptr;

        return *this;
    }

    // sends a request - forces the socket to fully send everything
    int send(const std::string_view& req) {
        const char* buf = req.data();
        int to_send = req.length();
        int sent = 0;
        while (to_send > 0) {
            const int len = SSL_write(m_ssl, buf + sent, to_send);
            if (len < 0) {
                int err = SSL_get_error(m_ssl, len);
                switch (err) {
                case SSL_ERROR_WANT_WRITE:
                    throw SSLSocketWrapperException("SSL_ERROR_WANT_WRITE");
                case SSL_ERROR_WANT_READ:
                    throw SSLSocketWrapperException("SSL_ERROR_WANT_READ");
                case SSL_ERROR_ZERO_RETURN:
                    throw SSLSocketWrapperException("SSL_ERROR_ZERO_RETURN");
                case SSL_ERROR_SYSCALL:
                    throw SSLSocketWrapperException("SSL_ERROR_SYSCALL");
                case SSL_ERROR_SSL:
                    throw SSLSocketWrapperException("SSL_ERROR_SSL");
                default:
                    throw SSLSocketWrapperException("UNKNOWN SSL ERROR");
                }
            }
            to_send -= len;
            sent += len;
        }
        return sent;
    }

    std::string_view read(const size_t read_size = 100) {
        size_t read = 0;
        m_out.clear();
        while (true) {
            const size_t original_size = m_out.size();
            m_out.resize(original_size + read_size);
            char* buf = &(m_out.data()[original_size]);
            int rc = SSL_read_ex(m_ssl, buf, read_size, &read);
            m_out.resize(original_size + read);
            if ((read < read_size) || (rc == 0)) {
                break;
            }
        }
        return m_out;
    }

    bool read_into(wsframe::FrameBuffer& frame_buffer,
                   const std::size_t chunk_size_hint = 1024) {
        std::size_t read = 0;
        bool new_data = false;
        while (true) {
            frame_buffer.ensure_extra_space(chunk_size_hint);
            auto* buf = frame_buffer.tail();
            int rc = SSL_read_ex(m_ssl, buf, chunk_size_hint, &read);
            if (read > 0)
                new_data = true;
            frame_buffer.claim_space(read);
            if ((read < chunk_size_hint) || (rc == 0))
                break;
        }
        return new_data;
    }

    ~SSLSocketWrapper() { disconnect(); }
};

class SocketWrapperException : public std::runtime_error {
  public:
    explicit SocketWrapperException(const std::string& msg)
        : std::runtime_error(msg) {}
};

template <bool verbose = false> class SocketWrapper {
  private:
    std::string m_host;
    long m_port;

    // raw TCP socket
    int m_sockfd = -1;

    // buffer for storing read results
    string m_out;

    void connect() {
        // optional pre-allocation for m_out
        m_out.reserve(1000);

        // set up hints for getaddrinfo
        struct addrinfo hints = {}, *addrs = nullptr;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        if (int rc = getaddrinfo(m_host.c_str(), std::to_string(m_port).c_str(),
                                 &hints, &addrs);
            rc != 0) {
            throw SocketWrapperException(std::string(gai_strerror(rc)));
        }

        bool connected = false;
        for (addrinfo* addr = addrs; addr != NULL; addr = addr->ai_next) {
            // create socket
            m_sockfd =
                ::socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
            if (m_sockfd == -1) {
                continue; // try next address
            }

            // set TCP_NODELAY
            int flag = 1;
            if (::setsockopt(m_sockfd, IPPROTO_TCP, TCP_NODELAY,
                             reinterpret_cast<char*>(&flag), sizeof(int)) < 0) {
                std::cerr << "Error setting TCP_NODELAY" << std::endl;
            } else {
                if constexpr (verbose) {
                    std::cout << "Set TCP_NODELAY" << std::endl;
                }
            }

            // attempt connect
            if (::connect(m_sockfd, addr->ai_addr, addr->ai_addrlen) == 0) {
                // success
                connected = true;
                break;
            }

            // if connect fails, close socket and try next
            ::close(m_sockfd);
            m_sockfd = -1;
        }

        freeaddrinfo(addrs);

        if (!connected || m_sockfd == -1) {
            throw SocketWrapperException("Failed to connect to server.");
        }

        // make the socket non-blocking
        fcntl(m_sockfd, F_SETFL, O_NONBLOCK);
    }

    void disconnect() {
        if (m_sockfd >= 0) {
            ::close(m_sockfd);
            m_sockfd = -1;
        }
    }

  public:
    SocketWrapper(const std::string& host, long port = 80)
        : m_host(host), m_port(port) {
        connect();
    }

    SocketWrapper() {}

    SocketWrapper(const SocketWrapper&) = delete;
    SocketWrapper& operator=(const SocketWrapper&) = delete;

    SocketWrapper(SocketWrapper&& other)
        : m_host(std::move(other.m_host)), m_port(other.m_port),
          m_sockfd(other.m_sockfd), m_out(std::move(other.m_out)) {
        other.m_sockfd = -1;
    }

    SocketWrapper& operator=(SocketWrapper&& other) {
        disconnect();

        m_host = std::move(other.m_host);
        m_port = other.m_port;
        m_sockfd = other.m_sockfd;
        m_out = std::move(other.m_out);

        other.m_sockfd = -1;
        return *this;
    }

    ~SocketWrapper() { disconnect(); }

    // send all data, blocking until complete (though socket is non-blocking).
    // in production, you might handle EAGAIN / partial writes more gracefully.
    int send(const std::string_view& req) {
        const char* buf = req.data();
        int to_send = static_cast<int>(req.size());
        int total_sent = 0;

        while (to_send > 0) {
            ssize_t ret = ::send(m_sockfd, buf + total_sent, to_send, 0);
            if (ret < 0) {
                // handle EAGAIN if non-blocking
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // either wait or throw
                    throw SocketWrapperException("Socket would block on send");
                }
                throw SocketWrapperException("send() failed");
            }
            to_send -= ret;
            total_sent += ret;
        }
        return total_sent;
    }

    // read all available data in loops.
    // returns everything read in m_out as a string_view.
    // if no data is available, returns empty.
    // if the socket is closed or error, might throw or return partial.
    std::string_view read(std::size_t chunk_size = 1024) {
        m_out.clear();
        while (true) {
            // expand buffer
            std::size_t old_size = m_out.size();
            m_out.resize(old_size + chunk_size);
            char* buf = &m_out[old_size];

            // read from socket
            ssize_t ret = ::recv(m_sockfd, buf, chunk_size, 0);
            if (ret < 0) {
                // handle EAGAIN or EWOULDBLOCK if non-blocking
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // no more data available now
                    m_out.resize(old_size);
                    break;
                } else {
                    throw SocketWrapperException("recv() failed: " +
                                                 std::to_string(errno));
                }
            } else if (ret == 0) {
                // connection closed by peer
                m_out.resize(old_size);
                break;
            } else {
                // got ret bytes
                m_out.resize(old_size + ret);
                // if ret < chunk_size, might be no more data right now
                if (static_cast<size_t>(ret) < chunk_size) {
                    // done reading for now
                    break;
                }
                // else loop to grab more
            }
        }
        return m_out;
    }

    bool read_into(wsframe::FrameBuffer& frame_buffer,
                   const std::size_t chunk_size_hint = 1024) {
        bool new_data = false;
        while (true) {
            frame_buffer.ensure_extra_space(chunk_size_hint);
            auto* buf = frame_buffer.tail();

            ssize_t ret = ::recv(m_sockfd, buf, chunk_size_hint, 0);
            if (ret < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                } else {
                    throw SocketWrapperException("recv() failed: " +
                                                 std::to_string(errno));
                }
            } else if (ret == 0) {
                break;
            } else {
                new_data = true;
                frame_buffer.claim_space(ret);
                if (static_cast<size_t>(ret) < chunk_size_hint)
                    break;
            }
        }
        return new_data;
    }
};

} // namespace fastws

#endif // _FASTWS_SOCKET_WRAPPER_HPP_