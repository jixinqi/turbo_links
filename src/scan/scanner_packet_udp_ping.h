#ifndef _TURBO_LINKS_SCAN_UDP_PING_PACKET_H_
#define _TURBO_LINKS_SCAN_UDP_PING_PACKET_H_

// 本文件已定型，无需更改

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <algorithm>

namespace turbo_links::scan
{

inline constexpr std::size_t flow_token_size = 16;
inline constexpr std::size_t nonce_size = 4;

struct scanner_packet_udp_ping_data_t
{
    // 当前 flow 的 128-bit 标识符。
    std::array<std::uint8_t, flow_token_size> flow_token {};

    // 当前端口扫描使用的 32-bit 随机值。
    std::array<std::uint8_t, nonce_size>      nonce {};
};

/*
    * UDP PING packet 仅用于 NAT 打洞阶段的端口探测，因此刻意保持为一个
    * 固定长度、内容最少且格式长期稳定的数据报。
    * 此数据包格式不会区分 Version
    *
    * Wire format 固定为 24 bytes：
    *   magic       4 bytes：固定为 ASCII "UFFP"
    *   flow_token 16 bytes：标识本次打洞 flow
    *   nonce       4 bytes：标识当前端口扫描过程
    */
class scanner_packet_udp_ping_t
{
public:
    // PING magic，wire 上固定为 ASCII "UFFP"。
    // UDP flood flow PACKET
    static constexpr std::array<std::uint8_t, 4> magic_num = {'U', 'F', 'F', 'P'};

    // Wire format:
    //   offset 0   : magic_num    4 bytes
    //   offset 4   : flow_token  16 bytes
    //   offset 20  : nonce        4 bytes
    //   total      :             24 bytes
    static constexpr std::size_t magic_size        = magic_num.size();

    static constexpr std::size_t magic_offset      = 0;
    static constexpr std::size_t flow_token_offset = magic_offset + magic_size;
    static constexpr std::size_t nonce_offset      = flow_token_offset + flow_token_size;
    static constexpr std::size_t wire_size         = nonce_offset + nonce_size;

    static_assert(wire_size == 24);

    // 将 PING 数据序列化成固定 24-byte wire packet。
    static std::array<std::uint8_t, wire_size> serialize(const scanner_packet_udp_ping_data_t& packet);

    // 解析固定 24-byte PING packet；长度或 magic 不正确时返回 std::nullopt。
    static std::optional<scanner_packet_udp_ping_data_t> parse(const std::uint8_t* data, std::size_t size);
};

}

#endif // _TURBO_LINKS_SCAN_UDP_PING_PACKET_H_

