#pragma once
#include "Cubed/gameplay/server_world.hpp"
#include "Cubed/gameplay/session.hpp"

#include <asio.hpp>
#include <thread>
namespace Cubed {

class NetworkServer {
public:
    NetworkServer(int port = 25530);
    ~NetworkServer();
    void stop();

    // Run in another thread after initialization is complete
    void start_server(int port = 25530);

    int port() const;
    ServerWorld& server_world();

private:
    asio::io_context m_io;
    std::thread m_net_thread;
    int m_port = 25530;
    std::atomic<bool> m_stopped{false};
    std::atomic<bool> m_started{false};
    ServerWorld m_world;
    std::mutex m_session_mutex;
    std::unordered_map<std::string, std::shared_ptr<Session>> m_session;
    asio::awaitable<void> listen();
    void net_run();
};
} // namespace Cubed