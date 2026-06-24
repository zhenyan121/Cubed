#include "Cubed/gameplay/session.hpp"

#include "Cubed/gameplay/server_world.hpp"
#include "Cubed/tools/log.hpp"
#include "Cubed/tools/uuid.hpp"
using asio::ip::tcp;

namespace Cubed {
Session::Session(tcp::socket socket, ServerWorld& server_world)
    : m_socket(std::move(socket)), m_uuid(generate_uuid()),
      m_server_world(server_world) {}

Session::~Session() {}

void Session::start() {
    auto self = shared_from_this();
    asio::co_spawn(
        m_strand,
        [self]() -> asio::awaitable<void> { co_await self->read_loop(); },
        asio::detached);
}

void Session::send(std::shared_ptr<std::vector<uint8_t>> packet) {
    asio::post(m_strand, [self = shared_from_this(),
                          packet = std::move(packet)]() mutable {
        bool idle = self->m_write_queue.empty();
        self->m_write_queue.emplace_back(std::move(packet));
        if (idle) {
            self->do_write();
        }
    });
}

const std::string& Session::uuid() const { return m_uuid; }

asio::awaitable<void> Session::read_loop() {
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
            if (cmd_id == to_num(PacketEnum::LOGIN_REQ)) {
                LoginReq req;
                if (req.ParseFromArray(body_data.data(), body_data.size())) {
                    m_server_world.handle_player_login(req.name(),
                                                       shared_from_this());
                }
            }
            if (cmd_id == to_num(PacketEnum::PLAYER_POS)) {
                PlayerPos pos;
                if (pos.ParseFromArray(body_data.data(), body_data.size())) {
                    m_server_world.sync_player_pos(pos.uuid(), pos.pos().x(),
                                                   pos.pos().y(),
                                                   pos.pos().z());
                }
            }
            if (cmd_id == to_num(PacketEnum::CHUNK_DATA_REQ)) {
                ChunkDataReq req;
                if (req.ParseFromArray(body_data.data(), body_data.size())) {
                    m_server_world.handle_chunk_req(
                        req.uuid(), ChunkPos(req.pos().x(), req.pos().z()));
                }
            }
            if (cmd_id == to_num(PacketEnum::BLOCK_CHANGE_REQ)) {
                BlockChangeReq req;
                if (req.ParseFromArray(body_data.data(), body_data.size())) {
                    m_server_world.handle_block_change(req);
                }
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
    } catch (...) {
        Logger::error("Unknow Error");
        close();
    }
    co_return;
}

void Session::do_write() {

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

void Session::close() {
    if (m_closed.exchange(true)) {
        return;
    }

    std::error_code ec;

    m_socket.shutdown(tcp::socket::shutdown_both, ec);

    m_socket.close(ec);
}

} // namespace Cubed