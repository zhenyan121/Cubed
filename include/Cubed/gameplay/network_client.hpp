#pragma once

#include "Cubed/gameplay/packet.hpp"

#include <asio.hpp>
#include <string>
#include <thread>
namespace Cubed {
using asio::ip::tcp;
class ClientWorld;
class NetworkClient : public std::enable_shared_from_this<NetworkClient> {
public:
    NetworkClient(ClientWorld& world);
    ~NetworkClient();
    void close();
    void stop();
    void send(Packet packet);
    void start(std::string ip, int port = 25530);
    bool is_connected() const;
    bool is_connect_error() const;

private:
    asio::io_context m_io;

    std::thread m_net_thread;
    static constexpr uint32_t MAX_PACKET_SIZE = 4 * 1024 * 1024;
    tcp::socket m_socket;
    std::vector<char> m_read_buffer;
    std::deque<Packet> m_write_queue;
    asio::strand<asio::io_context::executor_type> m_strand;
    asio::awaitable<void> connect(std::string ip, int port);
    asio::awaitable<void> read_loop();

    std::atomic<bool> m_closed{false};
    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_connect_error{false};
    // ClientWorld is managed by App
    ClientWorld& m_world;

    void do_write();
};
} // namespace Cubed