#pragma once
#include "packet.pb.h" // IWYU pragma: keep

#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <netinet/in.h>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace Cubed {
constexpr int HEADER_LEN = 12;
using Packet = std::shared_ptr<std::vector<uint8_t>>;
enum class CompressType : uint16_t {
    NONE = 0,
    ZSTD = 1,
};

inline CompressType get_compress_type(uint16_t id) {
    using enum CompressType;
    constexpr auto& to = std::to_underlying<CompressType>;
    switch (id) {
    case to(NONE):
        return NONE;
    case to(ZSTD):
        return ZSTD;
    }
    throw std::runtime_error(std::format("Unkown CompressType {}", id));
}

struct PacketHeader {
    uint16_t cmd;
    CompressType compress_type; // 0=none 1=zlib
    uint32_t uncompressed_size;
    uint32_t compressed_size;
};

enum class PacketEnum {
    LOGIN_REQ = 1001,
    LOGIN_RSP = 1002,
    LOGOUT_REQ = 1003,
    LOGOUT_RSP = 1004,
    PLAYER_INFO = 2001,
    PLAYER_POS = 2002,
    PLAYER_INFO_RSP = 2003,
    CHUNK_DATA_REQ = 3001,
    CHUNK_DATA_RSP = 3002,
    BLOCK_CHANGE_REQ = 3003,
    BLOCK_CHANGE_RSP = 3004,
    UPDATE_TIME = 3005,
    PING = 9001,
    PONG = 9002

};

template <typename> struct always_false : std::false_type {}; // NOLINT
template <typename T> constexpr uint16_t get_packet_id() {
    using enum PacketEnum;
    using std::is_same_v;

    using U = std::decay_t<T>;
    constexpr auto& to_num = std::to_underlying<PacketEnum>;
    if constexpr (is_same_v<U, LoginReq>) {
        return to_num(LOGIN_REQ);
    } else if constexpr (is_same_v<U, LoginRsp>) {
        return to_num(LOGIN_RSP);
    } else if constexpr (is_same_v<U, LogoutReq>) {
        return to_num(LOGOUT_REQ);
    } else if constexpr (is_same_v<U, LogoutRsp>) {
        return to_num(LOGOUT_RSP);
    } else if constexpr (is_same_v<U, PlayerInfo>) {
        return to_num(PLAYER_INFO);
    } else if constexpr (is_same_v<U, PlayerPos>) {
        return to_num(PLAYER_POS);
    } else if constexpr (is_same_v<U, PlayerInfoRsp>) {
        return to_num(PLAYER_INFO_RSP);
    } else if constexpr (is_same_v<U, ChunkDataReq>) {
        return to_num(CHUNK_DATA_REQ);
    } else if constexpr (is_same_v<U, ChunkDataRsp>) {
        return to_num(CHUNK_DATA_RSP);
    } else if constexpr (is_same_v<U, BlockChangeReq>) {
        return to_num(BLOCK_CHANGE_REQ);
    } else if constexpr (is_same_v<U, BlockChangeRsp>) {
        return to_num(BLOCK_CHANGE_RSP);
    } else if constexpr (is_same_v<U, UpdateTime>) {
        return to_num(UPDATE_TIME);
    } else if constexpr (is_same_v<U, Ping>) {
        return to_num(PING);
    } else if constexpr (is_same_v<U, Pong>) {
        return to_num(PONG);
    } else {
        static_assert(always_false<U>::value, "Unkonw Type");
    }
}

template <typename T> Packet make_packet(const T& msg) {
    PacketHeader header{};
    header.cmd = get_packet_id<T>();
    uint32_t size = static_cast<uint32_t>(msg.ByteSizeLong());
    header.uncompressed_size = size;
    header.compressed_size = size;
    header.compress_type = CompressType::NONE;

    auto packet = std::make_shared<std::vector<uint8_t>>(
        HEADER_LEN + header.compressed_size);

    uint16_t cmd_net = htons(header.cmd);
    uint16_t compress_type_net =
        htons(std::to_underlying(header.compress_type));
    uint32_t uncompressed_size_net = htonl(header.uncompressed_size);
    uint32_t compressed_size_net = htonl(header.compressed_size);

    std::memcpy(packet->data(), &cmd_net, sizeof(cmd_net));

    std::memcpy(packet->data() + 2, &compress_type_net,
                sizeof(compress_type_net));
    std::memcpy(packet->data() + 4, &uncompressed_size_net,
                sizeof(uncompressed_size_net));
    std::memcpy(packet->data() + 8, &compressed_size_net,
                sizeof(compressed_size_net));
    if (!msg.SerializeToArray(packet->data() + HEADER_LEN,
                              static_cast<int>(size))) {
        return {};
    }

    return packet;
}

inline PacketHeader
decode_packet_header(const std::array<char, HEADER_LEN>& header) {
    uint16_t cmd_net;
    uint16_t compress_type_net;
    uint32_t uncompressed_size_net;
    uint32_t compressed_size_net;
    std::memcpy(&cmd_net, header.data(), sizeof(cmd_net));
    std::memcpy(&compress_type_net, header.data() + 2,
                sizeof(compress_type_net));
    std::memcpy(&uncompressed_size_net, header.data() + 4,
                sizeof(uncompressed_size_net));
    std::memcpy(&compressed_size_net, header.data() + 8,
                sizeof(compressed_size_net));

    return {ntohs(cmd_net), get_compress_type(ntohs(compress_type_net)),
            ntohl(uncompressed_size_net), ntohl(compressed_size_net)};
}

} // namespace Cubed
