#pragma once
#include "packet.pb.h" // IWYU pragma: keep

#include <netinet/in.h>
#include <type_traits>
#include <utility>
namespace Cubed {
constexpr int HEADER_LEN = 8;
using Packet = std::shared_ptr<std::vector<uint8_t>>;

enum class PacketEnum {
    LOGIN_REQ = 1001,
    LOGIN_RSP = 1002,
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
    } else if (is_same_v<U, Pong>) {
        return to_num(PONG);
    } else {
        static_assert(always_false<U>::value, "Unkonw Type");
    }
}

template <typename T> Packet make_packet(const T& msg) {
    uint16_t cmd = get_packet_id<T>();

    uint32_t body_len = static_cast<uint32_t>(msg.ByteSizeLong());

    uint32_t total_len = HEADER_LEN + body_len;

    auto packet = std::make_shared<std::vector<uint8_t>>(total_len);

    uint32_t total_len_net = htonl(total_len);
    uint16_t cmd_net = htons(cmd);

    std::memcpy(packet->data(), &total_len_net, sizeof(total_len_net));

    std::memcpy(packet->data() + 4, &cmd_net, sizeof(cmd_net));

    if (!msg.SerializeToArray(packet->data() + HEADER_LEN,
                              static_cast<int>(body_len))) {
        return {};
    }

    return packet;
}

} // namespace Cubed
