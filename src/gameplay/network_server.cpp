#include "Cubed/gameplay/network_server.hpp"

#include "Cubed/tools/log.hpp"
using asio::ip::tcp;
namespace Cubed {

NetworkServer::NetworkServer(int port) : m_port(port) {}

NetworkServer::~NetworkServer() { stop(); }

void NetworkServer::stop() {
    if (m_stopped.exchange(true)) {
        return;
    }

    m_io.stop();

    std::vector<std::shared_ptr<Session>> sessions;

    {
        std::lock_guard lock(m_session_mutex);

        for (auto& [id, s] : m_session) {
            sessions.push_back(s);
        }

        m_session.clear();
    }

    for (auto& s : sessions) {
        s->close();
    }

    if (m_net_thread.joinable()) {
        Logger::info("Server join thread={}, current={}", m_net_thread.get_id(),
                     std::this_thread::get_id());
        m_net_thread.join();
    }
    Logger::info("Server Net Thread Stopped!");
}

asio::awaitable<void> NetworkServer::listen() {

    try {
        tcp::acceptor acceptor(m_io, tcp::endpoint(tcp::v4(), m_port));
        while (!m_stopped) {
            tcp::socket socket =
                co_await acceptor.async_accept(asio::use_awaitable);

            std::shared_ptr<Session> s =
                std::make_shared<Session>(std::move(socket), m_world, m_io);
            {
                std::lock_guard lock(m_session_mutex);
                m_session.emplace(s->uuid(), s);
            }
            s->start();
        }
    } catch (const std::exception& e) {
        if (!m_stopped) {
            Logger::error("accept error {}", e.what());
        }
    } catch (...) {
        if (!m_stopped) {
            Logger::error("Network Server: Unknown Error");
        }
    }

    co_return;
}

void NetworkServer::net_run() {
    if (m_net_thread.joinable()) {
        return;
    }
    m_net_thread = std::thread([this]() {
        asio::co_spawn(m_io, listen(), asio::detached);
        m_io.run();
    });
    Logger::info("Server Started!");
}

void NetworkServer::start_server() {
    m_world.init_world();
    net_run();
}

int NetworkServer::port() const { return m_port; }
ServerWorld& NetworkServer::server_world() { return m_world; }
} // namespace Cubed