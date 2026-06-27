#include "Cubed/gameplay/session.hpp"

#include "Cubed/gameplay/server_world.hpp"
#include "Cubed/tools/log.hpp"
#include "Cubed/tools/uuid.hpp"
using asio::ip::tcp;
using namespace google::protobuf;
namespace Cubed {
Session::Session(tcp::socket socket, ServerWorld& server_world,
                 asio::io_context& io)
    : m_socket(std::move(socket)), m_strand(asio::make_strand(io)),
      m_uuid(generate_uuid()), m_server_world(server_world) {}

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
            std::array<uint8_t, HEADER_LEN> header_buffer;
            co_await asio::async_read(m_socket, asio::buffer(header_buffer),
                                      asio::use_awaitable);

            auto header = decode_packet_header(header_buffer);
            uint32_t total_len = HEADER_LEN + header.compressed_size;

            if (total_len < HEADER_LEN || total_len > MAX_PACKET_SIZE) {

                throw std::runtime_error("invalid packet length");
            }
            std::vector<uint8_t> body_data(header.compressed_size);
            if (header.compressed_size > 0) {
                co_await asio::async_read(m_socket, asio::buffer(body_data),
                                          asio::use_awaitable);
            }
            auto cmd_id = header.cmd;
            Arena arena;
            if (cmd_id == std::to_underlying(PacketEnum::LOGIN_REQ)) {
                auto* req = Arena::Create<LoginReq>(&arena);
                Logger::info("Session: Receive Login req");
                if (decode_packet(*req, body_data, header)) {
                    m_server_world.handle_player_login(req->name(),
                                                       shared_from_this());
                }
            }
            if (cmd_id == std::to_underlying(PacketEnum::PLAYER_POS)) {
                auto* pos = Arena::Create<PlayerPos>(&arena);
                if (decode_packet(*pos, body_data, header)) {
                    m_server_world.sync_player_pos(pos->uuid(), pos->pos().x(),
                                                   pos->pos().y(),
                                                   pos->pos().z());
                }
            }
            if (cmd_id == std::to_underlying(PacketEnum::CHUNK_DATA_REQ)) {
                auto* req = Arena::Create<ChunkDataReq>(&arena);
                // Logger::info("Session: Receive Chunk Data req");
                if (decode_packet(*req, body_data, header)) {
                    m_server_world.handle_chunk_req(
                        req->task_id(), req->uuid(),
                        ChunkPos(req->pos().x(), req->pos().z()));
                }
            }
            if (cmd_id == std::to_underlying(PacketEnum::BLOCK_CHANGE_REQ)) {
                auto* req = Arena::Create<BlockChangeReq>(&arena);
                Logger::info("Session: Receive Block Change req");
                if (decode_packet(*req, body_data, header)) {
                    m_server_world.handle_block_change(*req);
                }
            }
            if (cmd_id == std::to_underlying(PacketEnum::LOGOUT_REQ)) {
                auto* req = Arena::Create<LogoutReq>(&arena);
                if (decode_packet(*req, body_data, header)) {
                    m_server_world.handle_player_exit(req->uuid());
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
    } catch (const std::exception& e) {
        Logger::error("Session Error {}", e.what());
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