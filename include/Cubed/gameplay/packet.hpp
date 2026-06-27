#pragma once
#include "Cubed/tools/compression.hpp"
#include "packet.pb.h" // IWYU pragma: keep

#include <concepts>
#include <cstdint>
#include <cstring>
#include <memory>
#include <netinet/in.h>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
namespace Cubed {
constexpr size_t HEADER_LEN =
    sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t);
constexpr size_t PACKET_COMPRESSION_THRESHOLD = 100;
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
    throw std::runtime_error(std::format("Unknown CompressType {}", id));
}

struct PacketHeader {
    uint16_t cmd{};
    CompressType compress_type{}; // 0=none 1=zlib
    uint32_t uncompressed_size{};
    uint32_t compressed_size{};
};

enum class PacketEnum : uint16_t {
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
    static_assert(always_false<T>::value, "Unknown Type");
    return 0;
}

template <> constexpr uint16_t get_packet_id<LoginReq>() {
    return std::to_underlying(PacketEnum::LOGIN_REQ);
}
template <> constexpr uint16_t get_packet_id<LoginRsp>() {
    return std::to_underlying(PacketEnum::LOGIN_RSP);
}
template <> constexpr uint16_t get_packet_id<LogoutReq>() {
    return std::to_underlying(PacketEnum::LOGOUT_REQ);
}
template <> constexpr uint16_t get_packet_id<LogoutRsp>() {
    return std::to_underlying(PacketEnum::LOGOUT_RSP);
}
template <> constexpr uint16_t get_packet_id<PlayerInfo>() {
    return std::to_underlying(PacketEnum::PLAYER_INFO);
}
template <> constexpr uint16_t get_packet_id<PlayerPos>() {
    return std::to_underlying(PacketEnum::PLAYER_POS);
}
template <> constexpr uint16_t get_packet_id<PlayerInfoRsp>() {
    return std::to_underlying(PacketEnum::PLAYER_INFO_RSP);
}
template <> constexpr uint16_t get_packet_id<ChunkDataReq>() {
    return std::to_underlying(PacketEnum::CHUNK_DATA_REQ);
}
template <> constexpr uint16_t get_packet_id<ChunkDataRsp>() {
    return std::to_underlying(PacketEnum::CHUNK_DATA_RSP);
}
template <> constexpr uint16_t get_packet_id<BlockChangeReq>() {
    return std::to_underlying(PacketEnum::BLOCK_CHANGE_REQ);
}
template <> constexpr uint16_t get_packet_id<BlockChangeRsp>() {
    return std::to_underlying(PacketEnum::BLOCK_CHANGE_RSP);
}
template <> constexpr uint16_t get_packet_id<UpdateTime>() {
    return std::to_underlying(PacketEnum::UPDATE_TIME);
}
template <> constexpr uint16_t get_packet_id<Ping>() {
    return std::to_underlying(PacketEnum::PING);
}
template <> constexpr uint16_t get_packet_id<Pong>() {
    return std::to_underlying(PacketEnum::PONG);
}

template <typename T>
    requires std::derived_from<T, google::protobuf::Message>
Packet make_packet(const T& msg) {
    PacketHeader header{};
    header.cmd = get_packet_id<T>();
    uint32_t raw_size = static_cast<uint32_t>(msg.ByteSizeLong());
    std::vector<uint8_t> raw(raw_size);

    if (!msg.SerializeToArray(raw.data(), raw_size)) {
        return {};
    }
    std::vector<uint8_t> payload;
    if (raw_size >= PACKET_COMPRESSION_THRESHOLD) {
        std::vector<uint8_t> compressed = compress_data(raw);
        if (compressed.size() < raw.size()) {
            payload = std::move(compressed);
            header.compress_type = CompressType::ZSTD;
        } else {
            payload = std::move(raw);
            header.compress_type = CompressType::NONE;
        }
    } else {
        payload = std::move(raw);
        header.compress_type = CompressType::NONE;
    }
    header.uncompressed_size = raw_size;
    header.compressed_size = static_cast<uint32_t>(payload.size());

    auto packet =
        std::make_shared<std::vector<uint8_t>>(HEADER_LEN + payload.size());

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
    std::memcpy(packet->data() + HEADER_LEN, payload.data(), payload.size());

    return packet;
}

inline PacketHeader decode_packet_header(std::span<const uint8_t> header) {
    if (header.size() < HEADER_LEN)
        throw std::runtime_error("Invalid header");
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
template <typename T>
    requires std::derived_from<T, google::protobuf::Message>
bool decode_packet(T& message, std::span<const uint8_t> data,
                   const PacketHeader& header) {
    if (data.size() != header.compressed_size) {
        return false;
    }

    if (header.compress_type == CompressType::NONE &&
        header.uncompressed_size != header.compressed_size) {
        return false;
    }

    switch (header.compress_type) {
    case CompressType::NONE: {
        return message.ParseFromArray(
            data.data(), static_cast<int>(header.uncompressed_size));
    }
    case CompressType::ZSTD: {
        auto raw = decompress_data(data, header.uncompressed_size);
        return message.ParseFromArray(raw.data(), static_cast<int>(raw.size()));
    }
    default:
        return false;
    }
}

} // namespace Cubed
