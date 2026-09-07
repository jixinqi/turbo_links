#include <iostream>
#include <chrono>
#include <csignal>
#include <exception>
#include <iostream>
#include <memory>
#include <random>

#include <boost/asio.hpp>

#include "scan/scanner_sender_udp.h"

int main()
{
    try
    {
        // 演示配置：在这里修改本地绑定地址/端口及对端公网 IP/监听端口。
        const auto local_address = boost::asio::ip::make_address("0.0.0.0");
        const std::uint16_t local_port = 1111;

        turbo_links::scan::scanner_packet_udp_ping_data_t ping;
        // 演示用 flow_token，两端配置须相同。
        ping.flow_token = {
            't', 'u', 'r', 'b', 'o', '_', 'l', 'i',
            'n', 'k', 's', '_', 'd', 'e', 'm', 'o'
        };
        std::random_device random;
        std::uniform_int_distribution<unsigned int> byte(0, 255);
        for (auto& value : ping.nonce)
            value = static_cast<std::uint8_t>(byte(random));

        auto params =
            std::make_shared<turbo_links::scan::scanner_sender_udp_params_t>(
                "192.168.23.21", 2222, 1000, 2000, 3, 1000, ping);

        // io_context 和 socket 由 main 持有，活到发送器完成全部异步回调之后。
        boost::asio::io_context io_context;
        boost::asio::ip::udp::socket socket(io_context, boost::asio::ip::udp::endpoint(local_address, local_port));
        boost::asio::signal_set stop_signals(io_context, SIGINT, SIGTERM);

        auto sender = std::make_shared<turbo_links::scan::scanner_sender_udp_t>(
            socket);
        stop_signals.async_wait(
            [weak_sender = std::weak_ptr<turbo_links::scan::scanner_sender_udp_t>(sender)]
            (boost::system::error_code ec, int /*signal_number*/)
            {
                if (!ec)
                {
                    if (auto active_sender = weak_sender.lock())
                        active_sender->exit();
                }
            });

        sender->do_send(params);

        std::cout << "UDP PING sender: " << socket.local_endpoint()
                  << " -> " << params->target_ip << ':' << params->target_port
                  << " (preferred port)\nPress Ctrl+C to stop.\n" << std::flush;

        io_context.run();
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Failed to start UDP sender: " << error.what() << '\n';
        return 1;
    }
}
