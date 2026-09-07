#include "udp_ping_packet.h"

// 本文件已定型，无需更改

namespace turbo_links::scan
{

std::array<std::uint8_t, ping_packet_t::wire_size> ping_packet_t::serialize(const ping_packet_data_t& packet)
{
    std::array<std::uint8_t, wire_size> result{};

    std::copy(magic_num.begin(), magic_num.end(), result.begin() + magic_offset);
    std::copy(packet.flow_token.begin(), packet.flow_token.end(), result.begin() + flow_token_offset);
    std::copy(packet.nonce.begin(), packet.nonce.end(), result.begin() + nonce_offset);

    return result;
}

std::optional<ping_packet_data_t> ping_packet_t::parse(const std::uint8_t* data, std::size_t size)
{
    if (
        data == nullptr ||
        size != wire_size ||
        !std::equal(magic_num.begin(), magic_num.end(), data + magic_offset)
    )
    {
        return std::nullopt;
    }

    ping_packet_data_t result{};

    std::copy_n(data + flow_token_offset, flow_token_size, result.flow_token.begin());
    std::copy_n(data + nonce_offset, nonce_size, result.nonce.begin());

    return result;
}

}