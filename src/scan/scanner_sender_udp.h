#ifndef TURBO_LINKS_SCAN_SCANNER_SENDER_UDP_H
#define TURBO_LINKS_SCAN_SCANNER_SENDER_UDP_H

// 本文件已定型，无需更改

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

#include <boost/asio/ip/udp.hpp>
#include <boost/asio/steady_timer.hpp>

#include "scan/scanner_packet_udp_ping.h"

namespace turbo_links::scan
{


class scanner_sender_udp_params_t
{
public:
    scanner_sender_udp_params_t(
        std::string        _target_ip,
        std::uint32_t      _target_port,
        std::uint16_t      _scan_start_port,
        std::uint16_t      _scan_end_port,
        std::uint32_t      _loop_count,
        std::uint32_t      _pps,
        scanner_packet_udp_ping_data_t _ping_packet_data
    );
    const std::string        target_ip;
    const std::uint32_t      target_port;
    const std::uint16_t      scan_start_port;
    const std::uint16_t      scan_end_port;
    const std::uint32_t      loop_count;
    const std::uint32_t      pps;
    const scanner_packet_udp_ping_data_t ping_packet_data;
};

struct scanner_sender_udp_state_t
{
public:
    boost::asio::ip::udp::endpoint target_endpoint;
    std::uint32_t current_port_{0};
    std::uint32_t current_loop_{0};
    std::array<std::uint8_t, scanner_packet_udp_ping_t::wire_size> send_buffer;
};

// scanner_udp 的独立发送任务，只发送固定长度的 PING。
class scanner_sender_udp_t
{
public:
    // socket 必须已绑定且未 connect；扫描总超时由上层 scanner_udp 管理。
    scanner_sender_udp_t(boost::asio::ip::udp::socket& _socket);
    ~scanner_sender_udp_t() = default;

    // 接收 scanner 的发送请求并开始扫描。
    // 一次请求严格发送 target_port, 1, 2, ... 65535，然后退出。
    // 即配置端口在遍历中仍会再发送一次，不探测 0 端口。
    // 空参数或扫描范围无效时抛出 std::invalid_argument；运行中重复启动抛出 std::logic_error。
    void do_send(std::shared_ptr<scanner_sender_udp_params_t> _params);

    // 请求停止发送；任务结束时先将 stop_flag_ 恢复为 false，再将 running_flag_ 置为 false。
    void exit();

protected:
    void do_send_impl();

    boost::asio::ip::udp::socket& socket_;
    std::shared_ptr<scanner_sender_udp_params_t> params_;
    std::shared_ptr<scanner_sender_udp_state_t> state_;

    std::atomic<bool> stop_flag_;
    std::atomic<bool> running_flag_;
};

}

#endif // TURBO_LINKS_SCAN_SCANNER_SENDER_UDP_H
