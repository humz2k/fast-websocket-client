#pragma once

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

inline std::string build_websocket_handshake_request(
    const std::string& host, const std::string& path, const std::string& key,
    const std::string& extra_headers = "") {
    std::string request;
    request += "GET " + path + " HTTP/1.1\r\n";
    request += "Host: " + host + "\r\n";
    request += "Upgrade: websocket\r\n";
    request += "Connection: Upgrade\r\n";
    request += "Sec-WebSocket-Key: " + key + "\r\n";
    request += "Sec-WebSocket-Version: 13\r\n";
    request += extra_headers;
    request += "\r\n";
    return request;
}

} // namespace fastws

// Copyright (c) 2022, Matthew Bentley (mattreecebentley@gmail.com) www.plflib.org

// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgement in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#ifndef PLF_NANOTIMER_H
#define PLF_NANOTIMER_H

// Compiler-specific defines:

#define PLF_NOEXCEPT throw() // default before potential redefine

#if defined(_MSC_VER) && !defined(__clang__) && !defined(__GNUC__)
	#if _MSC_VER >= 1900
		#undef PLF_NOEXCEPT
		#define PLF_NOEXCEPT noexcept
	#endif
#elif defined(__cplusplus) && __cplusplus >= 201103L // C++11 support, at least
	#if defined(__GNUC__) && defined(__GNUC_MINOR__) && !defined(__clang__) // If compiler is GCC/G++
		#if (__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4
			#undef PLF_NOEXCEPT
			#define PLF_NOEXCEPT noexcept
		#endif
	#elif defined(__clang__)
		#if __has_feature(cxx_noexcept)
			#undef PLF_NOEXCEPT
			#define PLF_NOEXCEPT noexcept
		#endif
	#else // Assume type traits and initializer support for other compilers and standard libraries
		#undef PLF_NOEXCEPT
		#define PLF_NOEXCEPT noexcept
	#endif
#endif

// ~Nanosecond-precision cross-platform (linux/bsd/mac/windows, C++03/C++11) simple timer class:

// Mac OSX implementation:
#if defined(__MACH__)
	#include <mach/clock.h>
	#include <mach/mach.h>

	namespace plf
	{

	class nanotimer
	{
	private:
		clock_serv_t system_clock;
		mach_timespec_t time1, time2;
	public:
		nanotimer() PLF_NOEXCEPT
		{
			host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &system_clock);
		}

		~nanotimer() PLF_NOEXCEPT
		{
			mach_port_deallocate(mach_task_self(), system_clock);
		}

		void start() PLF_NOEXCEPT
		{
			clock_get_time(system_clock, &time1);
		}

		double get_elapsed_ms() PLF_NOEXCEPT
		{
			return static_cast<double>(get_elapsed_ns()) / 1000000.0;
		}

		double get_elapsed_us() PLF_NOEXCEPT
		{
			return static_cast<double>(get_elapsed_ns()) / 1000.0;
		}

		double get_elapsed_ns() PLF_NOEXCEPT
		{
			clock_get_time(system_clock, &time2);
			return ((1000000000.0 * static_cast<double>(time2.tv_sec - time1.tv_sec)) + static_cast<double>(time2.tv_nsec - time1.tv_nsec));
		}
	};

// Linux/BSD implementation:
#elif (defined(linux) || defined(__linux__) || defined(__linux)) || (defined(__DragonFly__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__))
	#include <time.h>
	#include <sys/time.h>

	namespace plf
	{

	class nanotimer
	{
	private:
		struct timespec time1, time2;
	public:
		nanotimer() PLF_NOEXCEPT {}

		void start() PLF_NOEXCEPT
		{
			clock_gettime(CLOCK_MONOTONIC, &time1);
		}

		double get_elapsed_ms() PLF_NOEXCEPT
		{
			return get_elapsed_ns() / 1000000.0;
		}

		double get_elapsed_us() PLF_NOEXCEPT
		{
			return get_elapsed_ns() / 1000.0;
		}

		double get_elapsed_ns() PLF_NOEXCEPT
		{
			clock_gettime(CLOCK_MONOTONIC, &time2);
			return ((1000000000.0 * static_cast<double>(time2.tv_sec - time1.tv_sec)) + static_cast<double>(time2.tv_nsec - time1.tv_nsec));
		}
	};

// Windows implementation:
#elif defined(_WIN32)
	#if defined(_MSC_VER) && !defined(__clang__) && !defined(__GNUC__) && !defined(NOMINMAX)
		#define NOMINMAX // Otherwise MS compilers act like idiots when using std::numeric_limits<>::max() and including windows.h
	#endif

	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
		#include <windows.h>
		#undef WIN32_LEAN_AND_MEAN
	#else
		#include <windows.h>
	#endif

	namespace plf
	{

	class nanotimer
	{
	private:
		LARGE_INTEGER ticks1, ticks2;
		double frequency;
	public:
		nanotimer() PLF_NOEXCEPT
		{
			LARGE_INTEGER freq;
			QueryPerformanceFrequency(&freq);
			frequency = static_cast<double>(freq.QuadPart);
		}

		void start() PLF_NOEXCEPT
		{
			QueryPerformanceCounter(&ticks1);
		}

		double get_elapsed_ms() PLF_NOEXCEPT
		{
			QueryPerformanceCounter(&ticks2);
			return (static_cast<double>(ticks2.QuadPart - ticks1.QuadPart) * 1000.0) / frequency;
		}

		double get_elapsed_us() PLF_NOEXCEPT
		{
			return get_elapsed_ms() * 1000.0;
		}

		double get_elapsed_ns() PLF_NOEXCEPT
		{
			return get_elapsed_ms() * 1000000.0;
		}
	};
#endif
// Else: failure warning - your OS is not supported

#if defined(__MACH__) || (defined(linux) || defined(__linux__) || defined(__linux)) || (defined(__DragonFly__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)) || defined(_WIN32)
inline void nanosecond_delay(const double delay_ns) PLF_NOEXCEPT
{
	nanotimer timer;
	timer.start();

	while(timer.get_elapsed_ns() < delay_ns)
	{};
}

inline void microsecond_delay(const double delay_us) PLF_NOEXCEPT
{
	nanosecond_delay(delay_us * 1000.0);
}

inline void millisecond_delay(const double delay_ms) PLF_NOEXCEPT
{
	nanosecond_delay(delay_ms * 1000000.0);
}

} // namespace
#endif

#undef PLF_NOEXCEPT

#endif // PLF_NANOTIMER_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace wsframe {

// slow! use for seeds
inline uint64_t device_random() {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<std::uint64_t> dist;
    return dist(rng);
}

class XorShift128Plus {
  public:
    // Seeds (64-bit each). Make sure they're not both zero.
    // You can seed them however you like.
    std::array<std::uint64_t, 2> s;

    XorShift128Plus(std::uint64_t seed1, std::uint64_t seed2) {
        if (seed1 == 0 && seed2 == 0)
            seed2 = 1;
        s[0] = seed1;
        s[1] = seed2;
    }

    // Generate next 64-bit random value
    std::uint64_t next64() {
        std::uint64_t x = s[0];
        std::uint64_t const y = s[1];
        s[0] = y;
        x ^= x << 23;
        s[1] = x ^ y ^ (x >> 17) ^ (y >> 26);
        return s[1] + y;
    }

    void fill_bytes(std::uint8_t* buf, std::size_t n) {
        while (n >= 8) {
            std::uint64_t rnd = next64();
            // Copy 8 bytes of rnd into buf
            std::memcpy(buf, &rnd, 8);
            buf += 8;
            n -= 8;
        }
        // Handle leftover bytes
        if (n > 0) {
            uint64_t rnd = next64();
            std::memcpy(buf, &rnd, n);
        }
    }

    template <typename T, size_t n> void fill_bytes(std::array<T, n>& buf) {
        fill_bytes(static_cast<uint8_t*>(buf.data()), n * sizeof(T));
    }
};

class FrameBuffer {
  private:
    std::vector<std::uint8_t> m_buf;
    std::size_t m_ptr = 0;

  public:
    class View {
      private:
        const std::uint8_t* m_buf;
        const std::size_t m_sz;

      public:
        View(const std::uint8_t* buf, std::size_t sz) : m_buf(buf), m_sz(sz) {}
        std::size_t size() const { return m_sz; }
        const std::uint8_t* buf() const { return m_buf; }
        const std::uint8_t* begin() const { return m_buf; }
        const std::uint8_t* end() const { return m_buf + m_sz; }
    };

    FrameBuffer(std::size_t initial_capacity = 4096)
        : m_buf(initial_capacity) {}

    std::size_t capacity() const { return m_buf.size(); }

    void reset() { m_ptr = 0; }

    void ensure_fit(std::size_t sz) {
        if (capacity() < sz) {
            m_buf.resize(sz);
        }
    }

    void ensure_extra_space(std::size_t extra) { ensure_fit(m_ptr + extra); }

    // no bounds checking
    void push_back(uint8_t byte) {
        m_buf[m_ptr] = byte;
        m_ptr++;
    }

    // no bounds checking
    std::uint8_t* get_space(std::size_t sz) {
        std::uint8_t* out = &m_buf[m_ptr];
        m_ptr += sz;
        return out;
    }

    void claim_space(std::size_t sz) { m_ptr += sz; }

    void push_back(const View& view) {
        ensure_extra_space(view.size());
        std::memcpy(get_space(view.size()), view.buf(),
                    view.size() * sizeof(std::uint8_t));
    }

    void push_back(std::string_view view) {
        ensure_fit(m_ptr + view.size());
        std::memcpy(get_space(view.size()), view.data(),
                    view.size() * sizeof(char));
    }

    std::uint8_t* head() { return m_buf.data(); }
    const std::uint8_t* head() const { return m_buf.data(); }

    std::uint8_t* tail() { return &m_buf[m_ptr]; }
    const std::uint8_t* tail() const { return &m_buf[m_ptr]; }

    std::size_t size() const { return m_ptr; }

    template <typename T> T view() const;
};

template <>
inline FrameBuffer::View FrameBuffer::view<FrameBuffer::View>() const {
    return View(m_buf.data(), m_ptr);
}

template <>
inline std::string_view FrameBuffer::view<std::string_view>() const {
    return std::string_view((const char*)m_buf.data(), m_ptr);
}

struct Frame {
    enum class Opcode : uint8_t {
        CONTINUATION = 0x0,
        TEXT = 0x1,
        BINARY = 0x2,
        CLOSE = 0x8,
        PING = 0x9,
        PONG = 0xA,
        UNKNOWN
    };

    static inline const char* opcode_to_string(Opcode code) {
        switch (code) {
        case Opcode::CONTINUATION:
            return "Opcode::CONTINUATION";
        case Opcode::TEXT:
            return "Opcode::TEXT";
        case Opcode::BINARY:
            return "Opcode::BINARY";
        case Opcode::CLOSE:
            return "Opcode::CLOSE";
        case Opcode::PING:
            return "Opcode::PING";
        case Opcode::PONG:
            return "Opcode::PONG";
        default:
            return "Opcode::UNKNOWN";
        }
    }

    bool fin;
    bool mask;
    Opcode opcode;
    std::array<std::uint8_t, 4> masking_key;
    std::string_view payload;

    friend std::ostream& operator<<(std::ostream& stream, const Frame& frame) {
        stream << "[fin=" << frame.fin << "]["
               << Frame::opcode_to_string(frame.opcode)
               << "][mask=" << frame.mask << "]";
        if (frame.mask) {
            stream << "[key=" << std::hex << frame.masking_key[0] << " "
                   << frame.masking_key[1] << " " << frame.masking_key[2] << " "
                   << frame.masking_key[3] << std::dec << "]";
        }
        if (frame.payload.size() > 0) {
            stream << "[payload=\"" << frame.payload << "\"]";
        }
        return stream;
    }

  protected:
    void construct(FrameBuffer& buf) const {
        buf.reset();
        buf.ensure_fit(payload.length() + 14 + 100);

        // fin bit + 3 rsv bits + opcode
        buf.push_back(
            ((fin ? 0x80 : 0x00) | (static_cast<std::uint8_t>(opcode) & 0x0F)));

        // mask bit + payload length
        //   if payload.len < 126, len fits in 7 bits
        //   if 126 <= payload.len < 65526, write 126, then 16-bit length
        //   if payload.len >= 65536, write 127, then 64-bit length
        const std::uint8_t mask_bit = mask ? 0x80 : 0x00;

        std::uint64_t payload_length = payload.length();
        auto* payload_data = payload.data();

        if (payload_length < 126U) {
            buf.push_back(mask_bit | static_cast<std::uint8_t>(payload_length));
        } else if (payload_length <= 0xFFFFU) {
            buf.push_back(mask_bit | 126U);
            // write length in network order (big-endian)
            buf.push_back(
                static_cast<std::uint8_t>((payload_length >> 8) & 0xFFU));
            buf.push_back(static_cast<std::uint8_t>(payload_length & 0xFFU));
        } else {
            buf.push_back(mask_bit | 127U);
            // write length in big-endian
            std::uint8_t* ptr = buf.get_space(8);
            for (int i = 7; i >= 0; i--) {
                *ptr = static_cast<std::uint8_t>((payload_length >> (8 * i)) &
                                                 0xFFU);
                ptr++;
            }
        }

        // if mask, write the mask bytes and then xor payload bytes with that
        if (mask) {

            std::memcpy(buf.get_space(4), masking_key.data(),
                        masking_key.size() * sizeof(std::uint8_t));
            std::uint8_t* ptr = buf.get_space(payload_length);
            for (std::size_t i = 0; i < payload_length; i++) {
                ptr[i] = payload_data[i] ^ masking_key[i % 4];
            }
        } else {
            // otherwise, just write payload
            std::memcpy(buf.get_space(payload_length), payload_data,
                        payload_length * sizeof(std::uint8_t));
        }
    }
    friend class FrameFactory;
};

class FrameFactory {
  private:
    template <int entries> class RandomCache {
      private:
        XorShift128Plus m_random;
        std::array<std::uint8_t, entries * 4> m_cache;
        std::size_t m_cache_ptr = 0;

      public:
        RandomCache() : m_random(device_random(), device_random()) {
            fill_cache();
        }

        void fill_cache() {
            m_random.fill_bytes(m_cache);
            m_cache_ptr = 0;
        }

        void get(std::array<uint8_t, 4>& ptr) {
            if (m_cache_ptr >= entries * 4) {
                fill_cache();
            }
            std::copy(&m_cache[m_cache_ptr], &m_cache[m_cache_ptr + 4],
                      &ptr[0]);
            m_cache_ptr += 4;
        }
    };

    FrameBuffer m_buf;
    RandomCache<8> m_random;

  public:
    FrameFactory(std::size_t initial_capacity = 4096)
        : m_buf(initial_capacity) {}

    void fill_random_cache() { m_random.fill_cache(); }

    std::string_view construct(bool fin, Frame::Opcode opcode, bool mask,
                               std::string_view payload) {
        Frame frame;
        frame.fin = fin;
        frame.mask = mask;
        frame.opcode = opcode;
        if (mask) {
            m_random.get(frame.masking_key);
        }
        frame.payload = payload;
        frame.construct(m_buf);
        return m_buf.view<std::string_view>();
    }

    std::string_view text(bool fin, bool mask, std::string_view payload) {
        return construct(fin, Frame::Opcode::TEXT, mask, payload);
    }

    std::string_view binary(bool fin, bool mask, std::string_view payload) {
        return construct(fin, Frame::Opcode::BINARY, mask, payload);
    }

    std::string_view ping(bool mask, std::string_view payload) {
        if (payload.size() > 125) {
            throw std::runtime_error(
                "Payload should be <= 125 for ping frames");
        }
        return construct(true, Frame::Opcode::PING, mask, payload);
    }

    std::string_view pong(bool mask, std::string_view payload) {
        if (payload.size() > 125) {
            throw std::runtime_error(
                "Payload should be <= 125 for pong frames");
        }
        return construct(true, Frame::Opcode::PONG, mask, payload);
    }

    std::string_view close(bool mask, std::string_view payload) {
        if (payload.size() > 125) {
            throw std::runtime_error(
                "Payload should be <= 125 for close frames");
        }
        return construct(true, Frame::Opcode::CLOSE, mask, payload);
    }
};

class FrameParser {
  private:
    enum class ParseStage {
        FIN_BIT,
        OPCODE,
        MASK_BIT,
        PAYLOAD_LEN,
        EXTENDED_PAYLOAD_LEN_16,
        EXTENDED_PAYLOAD_LEN_64,
        MASKING_KEY,
        PAYLOAD_DATA,
        DONE
    };
    ParseStage m_parse_stage = ParseStage::FIN_BIT;
    Frame m_frame;
    FrameBuffer m_frame_buffer;
    std::uint64_t m_payload_len = 0;
    std::size_t m_ptr = 0;

    std::size_t remaining() const { return m_frame_buffer.size() - m_ptr; }

    std::uint8_t read() const { return *(m_frame_buffer.head() + m_ptr); }

    std::uint8_t consume() {
        auto out = read();
        m_ptr++;
        return out;
    }

    void check_fin_bit() {
        if ((m_parse_stage != ParseStage::FIN_BIT) || (remaining() == 0))
            return;
        m_frame.fin = read() & 0x80;
        m_parse_stage = ParseStage::OPCODE;
    }

    void check_opcode() {
        if ((m_parse_stage != ParseStage::OPCODE) || (remaining() == 0))
            return;
        m_frame.opcode = static_cast<Frame::Opcode>(consume() & 0x0F);
        m_parse_stage = ParseStage::MASK_BIT;
    }

    void check_mask_bit() {
        if ((m_parse_stage != ParseStage::MASK_BIT) || (remaining() == 0))
            return;
        m_frame.mask = read() & 0x80;
        m_parse_stage = ParseStage::PAYLOAD_LEN;
    }

    void check_payload_len() {
        if ((m_parse_stage != ParseStage::PAYLOAD_LEN) || (remaining() == 0))
            return;
        std::size_t len = consume() & 0x7F;
        if (len == 126) {
            m_parse_stage = ParseStage::EXTENDED_PAYLOAD_LEN_16;
            return;
        }
        if (len == 127) {
            m_parse_stage = ParseStage::EXTENDED_PAYLOAD_LEN_64;
            return;
        }
        m_payload_len = len;
        m_parse_stage =
            m_frame.mask ? ParseStage::MASKING_KEY : ParseStage::PAYLOAD_DATA;
    }

    void check_extended_payload_len_16() {
        if ((m_parse_stage != ParseStage::EXTENDED_PAYLOAD_LEN_16) ||
            (remaining() < 2))
            return;
        uint64_t left = consume();
        uint64_t right = consume();
        m_payload_len = (left << 8) | right;
        m_parse_stage =
            m_frame.mask ? ParseStage::MASKING_KEY : ParseStage::PAYLOAD_DATA;
    }

    void check_extended_payload_len_64() {
        if ((m_parse_stage != ParseStage::EXTENDED_PAYLOAD_LEN_64) ||
            (remaining() < 8))
            return;
        m_payload_len = 0;
        for (int i = 7; i >= 0; i--) {
            m_payload_len |= consume() << 8 * i;
        }
        m_parse_stage =
            m_frame.mask ? ParseStage::MASKING_KEY : ParseStage::PAYLOAD_DATA;
    }

    void check_masking_key() {
        if ((m_parse_stage != ParseStage::MASKING_KEY) || (remaining() < 4))
            return;
        for (int i = 0; i < 4; i++) {
            m_frame.masking_key[i] = consume();
        }
        m_parse_stage = ParseStage::PAYLOAD_DATA;
    }

    void check_payload_data() {
        if ((m_parse_stage != ParseStage::PAYLOAD_DATA) ||
            (remaining() < m_payload_len))
            return;
        if (m_payload_len == 0) {
            m_parse_stage = ParseStage::DONE;
            return;
        }
        const char* buf = (const char*)(m_frame_buffer.head() + m_ptr);
        m_frame.payload = std::string_view(buf, m_payload_len);
        m_ptr += m_payload_len;
        m_parse_stage = ParseStage::DONE;
    }

    bool done() const { return m_parse_stage == ParseStage::DONE; }

    std::optional<Frame> parse() {
        check_fin_bit();
        check_opcode();
        check_mask_bit();
        check_payload_len();
        check_extended_payload_len_16();
        check_extended_payload_len_64();
        check_masking_key();
        check_payload_data();
        if (!done())
            return {};
        return m_frame;
    }

    void reset() {
        if (remaining() > 0) {
            auto re_space = remaining();
            std::memmove(m_frame_buffer.head(), m_frame_buffer.head() + m_ptr,
                         re_space);
            m_frame_buffer.reset();
            m_frame_buffer.claim_space(re_space);
            m_ptr = 0;
        } else {
            m_frame_buffer.reset();
            m_ptr = 0;
        }

        m_frame = {};
        m_payload_len = 0;
        m_parse_stage = ParseStage::FIN_BIT;
    }

  public:
    FrameParser() {}

    void clear() {
        m_frame_buffer.reset();
        m_ptr = 0;
        m_frame = {};
        m_payload_len = 0;
        m_parse_stage = ParseStage::FIN_BIT;
    }

    std::optional<Frame> update(const FrameBuffer::View& view) {
        if (done())
            reset();
        if (view.size() != 0)
            m_frame_buffer.push_back(view);
        else if (m_parse_stage != ParseStage::FIN_BIT)
            return {};
        if (remaining() == 0)
            return {};
        return parse();
    }

    std::optional<Frame> update(std::string_view view) {
        if (done())
            reset();
        if (view.size() != 0)
            m_frame_buffer.push_back(view);
        else if (m_parse_stage != ParseStage::FIN_BIT)
            return {};
        if (remaining() == 0)
            return {};
        return parse();
    }

    std::optional<Frame> update(bool new_data) {
        if (done())
            reset();
        if ((!new_data) && (m_parse_stage != ParseStage::FIN_BIT))
            return {};
        if (remaining() == 0)
            return {};
        return parse();
    }

    wsframe::FrameBuffer& frame_buffer() { return m_frame_buffer; }
};

} // namespace wsframe

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
    int send(std::string_view req) {
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
        m_out.clear();
        size_t read = 0;
        const size_t original_size = m_out.size();
        m_out.resize(original_size + read_size);
        char* buf = &(m_out.data()[original_size]);
        SSL_read_ex(m_ssl, buf, read_size, &read);
        m_out.resize(original_size + read);
        return m_out;
    }

    bool read_into(wsframe::FrameBuffer& frame_buffer,
                   const std::size_t chunk_size_hint = 1024) {
        std::size_t read = 0;
        bool new_data = false;
        frame_buffer.ensure_extra_space(chunk_size_hint);
        auto* buf = frame_buffer.tail();
        SSL_read_ex(m_ssl, buf, chunk_size_hint, &read);
        if (read > 0)
            new_data = true;
        frame_buffer.claim_space(read);
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

    // send all data
    int send(std::string_view req) {
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
        // expand buffer
        std::size_t old_size = m_out.size();
        m_out.resize(old_size + chunk_size);
        char* buf = &m_out[old_size];

        // read from socket
        ssize_t ret = ::recv(m_sockfd, buf, chunk_size, 0);
        if (ret < 0) {
            // handle EAGAIN or EWOULDBLOCK if non-blocking
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                m_out.resize(old_size);
            } else {
                throw SocketWrapperException("recv() failed: " +
                                             std::to_string(errno));
            }
        } else if (ret == 0) {
            m_out.resize(old_size);
        } else {
            m_out.resize(old_size + ret);
        }
        return m_out;
    }

    bool read_into(wsframe::FrameBuffer& frame_buffer,
                   const std::size_t chunk_size_hint = 1024) {
        bool new_data = false;
        frame_buffer.ensure_extra_space(chunk_size_hint);
        auto* buf = frame_buffer.tail();

        ssize_t ret = ::recv(m_sockfd, buf, chunk_size_hint, 0);
        if (ret < 0) {
            if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
                throw SocketWrapperException("recv() failed: " +
                                             std::to_string(errno));
            }
        } else if (ret > 0) {
            new_data = true;
            frame_buffer.claim_space(ret);
        }
        return new_data;
    }
};

} // namespace fastws

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
        m_connection_open = response.find("HTTP/1.1 101") !=
                            std::string::npos;
        if (m_connection_open) {
            m_status = ConnectionStatus::HEALTHY;
            m_handler.on_open(*this);
        } else {
            m_status = ConnectionStatus::FAILED;
        }
        return m_connection_open;
    }

    void send(std::string_view frame) { m_socket.send(frame); }

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
        m_waiting_for_ping = true;
        m_ping_timer.start();
        send_ping();
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

    ConnectionStatus poll(const int max_reads = 4) {
        int count_reads = 0;
        for (auto parsed_frame = m_parser.update(
                 m_socket.read_into(m_parser.frame_buffer(), 1024));
             parsed_frame.has_value();
             parsed_frame = m_parser.update(
                 m_socket.read_into(m_parser.frame_buffer(), 1024))) {
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
            count_reads++;
            if (count_reads >= max_reads)break;
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

