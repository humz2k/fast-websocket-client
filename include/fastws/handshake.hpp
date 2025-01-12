#ifndef _FASTWS_KEYGEN_HPP_
#define _FASTWS_KEYGEN_HPP_

#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>

#include <random>
#include <string>
#include <vector>

namespace fastws {

// Minimal function to produce a 16-byte random key and Base64-encode it.
inline std::string generate_sec_websocket_key() {
    // Generate 16 random bytes
    std::vector<unsigned char> randomBytes(16);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 255);
    for (auto& b : randomBytes) {
        b = static_cast<unsigned char>(dist(gen));
    }

    // Base64-encode the 16 bytes using OpenSSL's BIO
    BIO* b64 = BIO_new(BIO_f_base64());
    // Don't insert newlines
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    BIO* mem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, mem);

    BIO_write(b64, randomBytes.data(), static_cast<int>(randomBytes.size()));
    BIO_flush(b64);

    BUF_MEM* memPtr;
    BIO_get_mem_ptr(b64, &memPtr);

    // Create a std::string from the memory buffer
    std::string base64Key(memPtr->data, memPtr->length);

    BIO_free_all(b64);

    return base64Key;
}

inline std::string build_websocket_handshake_request(const std::string& host,
                                                     const std::string& path,
                                                     const std::string& key) {
    std::string request;
    request += "GET " + path + " HTTP/1.1\r\n";
    request += "Host: " + host + "\r\n";
    request += "Upgrade: websocket\r\n";
    request += "Connection: Upgrade\r\n";
    request += "Sec-WebSocket-Key: " + key + "\r\n";
    request += "Sec-WebSocket-Version: 13\r\n";
    request += "\r\n";
    return request;
}

} // namespace fastws

#endif // _FASTWS_KEYGEN_HPP_