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
    for (auto& [key, s] : m_session) {
        if (s) {
            s->close();
        }
    }

    m_io.stop();
    m_session.clear();
    if (m_net_thread.joinable()) {
        m_net_thread.join();
    }
    Logger::info("Server Stopped!");
}

asio::awaitable<void> NetworkServer::listen() {
    tcp::acceptor acceptor(m_io, tcp::endpoint(tcp::v4(), m_port));
    while (true) {
        tcp::socket socket =
            co_await acceptor.async_accept(asio::use_awaitable);
        if (m_stopped) {
            break;
        }
        std::shared_ptr<Session> s =
            std::make_shared<Session>(std::move(socket), m_world);
        s->start();
        m_session.emplace(s->uuid(), s);
    }
    co_return;
}

void NetworkServer::net_run() {
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

} // namespace Cubed