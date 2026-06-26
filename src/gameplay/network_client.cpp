#include "Cubed/gameplay/network_client.hpp"

#include "Cubed/gameplay/client_world.hpp"
#include "Cubed/tools/log.hpp"
namespace Cubed {
NetworkClient::NetworkClient(ClientWorld& world)
    : m_socket(m_io), m_strand(asio::make_strand(m_io)), m_world(world) {}

NetworkClient::~NetworkClient() { close(); }

void NetworkClient::start(std::string ip, int port) {
    if (m_net_thread.joinable()) {
        return;
    }
    m_net_thread = std::thread([self = shared_from_this(), ip, port]() {
        asio::co_spawn(self->m_strand, self->connect(ip, port), asio::detached);
        self->m_io.run();
    });
    Logger::info("NetworkClient Started");
}

bool NetworkClient::is_connected() const { return m_connected.load(); }
bool NetworkClient::is_connect_error() const { return m_connect_error.load(); }
asio::awaitable<void> NetworkClient::connect(std::string ip, int port) {
    Logger::info("Connect Begin");
    try {
        auto ex = co_await asio::this_coro::executor;
        tcp::resolver resolver(ex);
        auto eps = co_await resolver.async_resolve(ip, std::to_string(port),
                                                   asio::use_awaitable);
        Logger::info("Resolve Success");
        co_await async_connect(m_socket, eps, asio::use_awaitable);
        Logger::info("Connect Success, Server ip {} port {}", ip, port);
        asio::co_spawn(m_strand, read_loop(), asio::detached);
        Logger::info("NetworkClient Read Loop Started");
        m_connected = true;
        co_return;

    } catch (const std::exception& e) {
        Logger::error("Client Error {}", e.what());
        m_connect_error = true;
    }
}

asio::awaitable<void> NetworkClient::read_loop() {
    try {
        while (true) {
            std::array<char, HEADER_LEN> header;
            co_await asio::async_read(m_socket, asio::buffer(header),
                                      asio::use_awaitable);
            uint32_t total_len_net;
            std::memcpy(&total_len_net, header.data(), sizeof(total_len_net));
            uint32_t total_len = ntohl(total_len_net);

            uint16_t cmd_id_net;
            std::memcpy(&cmd_id_net, header.data() + 4, sizeof(cmd_id_net));
            uint16_t cmd_id = ntohs(cmd_id_net);
            if (total_len < HEADER_LEN || total_len > MAX_PACKET_SIZE) {

                throw std::runtime_error("invalid packet length");
            }
            uint32_t body_len = total_len - HEADER_LEN;
            std::vector<uint8_t> body_data(body_len);
            if (body_len > 0) {
                co_await asio::async_read(m_socket, asio::buffer(body_data),
                                          asio::use_awaitable);
            }

            constexpr auto& to_num = std::to_underlying<PacketEnum>;
            switch (cmd_id) {
            case to_num(PacketEnum::LOGIN_RSP): {
                LoginRsp rsp;
                Logger::info("Client: Receive Login rsp");
                if (rsp.ParseFromArray(body_data.data(), body_data.size())) {
                    if (rsp.success()) {
                        m_world.start_client_thread(rsp.uuid());
                    } else {
                        Logger::error("Connected Server Fail");
                    }
                }
            } break;
            case to_num(PacketEnum::CHUNK_DATA_RSP): {
                ChunkDataRsp rsp;
                // Logger::info("Client: Receive Chunk Data rsp, size {}mb",
                //              body_data.size() / 1024.0f / 1024);
                if (rsp.ParseFromArray(body_data.data(), body_data.size())) {
                    m_world.receive_chunk(rsp);
                }
            } break;
            case to_num(PacketEnum::BLOCK_CHANGE_RSP): {
                BlockChangeRsp rsp;
                Logger::info("Client: Receive Block Change rsp");
                if (rsp.ParseFromArray(body_data.data(), body_data.size())) {
                    m_world.receive_block_change(rsp);
                }
            } break;
            case to_num(PacketEnum::UPDATE_TIME): {
                UpdateTime rsp;
                if (rsp.ParseFromArray(body_data.data(), body_data.size())) {
                    m_world.receive_time(rsp);
                }
            }
            case to_num(PacketEnum::PLAYER_INFO_RSP): {
                PlayerInfoRsp rsp;
                if (rsp.ParseFromArray(body_data.data(), body_data.size())) {
                    m_world.receive_other_player(rsp);
                }
            } break;
            }
        }
    } catch (const asio::system_error& e) {
        auto ec = e.code();

        if (ec == asio::error::eof || ec == asio::error::operation_aborted) {

            Logger::info("Client disconnected");
        } else {
            Logger::warn("Asio Error {}", e.what());
        }

        close();
    } catch (const std::exception& e) {
        Logger::error("Session Error {}", e.what());
        close();
    } catch (...) {
        Logger::error("Unknow Error");
        close();
    }
    co_return;
}

void NetworkClient::send(std::shared_ptr<std::vector<uint8_t>> packet) {
    if (m_closed.load()) {
        return;
    }
    asio::post(m_strand, [self = shared_from_this(),
                          packet = std::move(packet)]() mutable {
        bool idle = self->m_write_queue.empty();
        self->m_write_queue.emplace_back(std::move(packet));
        if (idle) {
            self->do_write();
        }
    });
}

void NetworkClient::do_write() {
    if (m_closed.load()) {
        return;
    }

    auto self = shared_from_this();
    asio::async_write(
        m_socket, asio::buffer(*(m_write_queue.front())),
        asio::bind_executor(m_strand, [self](std::error_code ec, size_t) {
            if (ec) {
                Logger::warn("Write Ec {}", ec.message());
                self->close();
                return;
            }
            self->m_write_queue.pop_front();
            if (!self->m_write_queue.empty()) {
                self->do_write();
            }
        }));
}

void NetworkClient::close() {
    if (m_closed.exchange(true)) {
        return;
    }

    std::error_code ec;

    m_socket.shutdown(tcp::socket::shutdown_both, ec);

    m_socket.close(ec);
    Logger::info("NetworkClient Closed");
    m_connected = false;
    m_io.stop();
}

void NetworkClient::stop() {
    close();

    if (m_net_thread.joinable()) {
        m_net_thread.join();
    }
}

} // namespace Cubed