#pragma once

#include "Cubed/gameplay/packet.hpp"

#include <asio.hpp>
#include <string>
#include <thread>
namespace Cubed {
using asio::ip::tcp;
class NetworkClient : std::enable_shared_from_this<NetworkClient> {
public:
    NetworkClient();
    ~NetworkClient();
    void close();
    asio::awaitable<void> connect(std::string ip, int port);
    void send(Packet packet);
    void start(std::string ip, int port = 25530);

private:
    asio::io_context m_io;

    std::thread m_net_thread;
    static constexpr uint32_t MAX_PACKET_SIZE = 4 * 1024 * 1024;
    tcp::socket m_socket;
    std::vector<char> m_read_buffer;
    std::deque<Packet> m_write_queue;
    asio::strand<asio::io_context::executor_type> m_strand;
    asio::awaitable<void> read_loop();
    std::atomic<bool> m_closed{false};

    // ClientWorld is managed by App
    // ClientWorld& m_world;

    void do_write();
};
} // namespace Cubed