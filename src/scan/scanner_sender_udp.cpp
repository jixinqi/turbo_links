#include "scan/scanner_sender_udp.h"

#include <chrono>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>

#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/address.hpp>

namespace turbo_links::scan
{

scanner_sender_udp_params_t::scanner_sender_udp_params_t(
    std::string        _target_ip,
    std::uint32_t      _target_port,
    std::uint16_t      _scan_start_port,
    std::uint16_t      _scan_end_port,
    std::uint32_t      _loop_count,
    std::uint32_t      _pps,
    scanner_packet_udp_ping_data_t _ping_packet_data
)
: target_ip(_target_ip)
, target_port(_target_port)
, scan_start_port(_scan_start_port)
, scan_end_port(_scan_end_port)
, loop_count(_loop_count)
, pps(_pps)
, ping_packet_data(_ping_packet_data)
{
}

scanner_sender_udp_t::scanner_sender_udp_t(boost::asio::ip::udp::socket& _socket)
: socket_ { _socket }
, exit_signal_ { false }
, running_flag_ { false }
{
}

void scanner_sender_udp_t::do_send(std::shared_ptr<scanner_sender_udp_params_t> _params)
{
    if (
        !_params ||
        _params->loop_count == 0 ||
        _params->scan_start_port == 0 ||
        _params->scan_start_port > _params->scan_end_port
    )
    {
        throw std::invalid_argument("invalid UDP scan params.");
    }

    std::shared_ptr<scanner_sender_udp_state_t> state = std::make_shared<scanner_sender_udp_state_t>();
    state->target_endpoint = boost::asio::ip::udp::endpoint(
        boost::asio::ip::make_address(_params->target_ip),
        _params->scan_start_port
    );
    state->current_port_ = _params->scan_start_port;
    state->current_loop_ = 0;
    state->send_buffer = scanner_packet_udp_ping_t::serialize(_params->ping_packet_data);

    // 只有 running_flag_ 当前为 false 时，才能把它改成 true 并继续启动；
    // 否则抛出异常。检查和修改合在一个原子操作里完成。
    bool expected = false;
    if (!running_flag_.compare_exchange_strong(expected, true, std::memory_order_acq_rel, std::memory_order_acquire))
    {
        throw std::logic_error("UDP scanner sender is already running.");
    }

    params_ = std::move(_params);
    state_ = std::move(state);
    do_send_impl();
}

void scanner_sender_udp_t::exit()
{
    exit_signal_.store(true, std::memory_order_release);
}

void scanner_sender_udp_t::do_send_impl()
{
    try
    {
        state_->target_endpoint.port(static_cast<std::uint16_t>(state_->current_port_));

        socket_.async_send_to(
            boost::asio::buffer(state_->send_buffer),
            state_->target_endpoint,
            [this]
            (boost::system::error_code ec, std::size_t /*bytes_sent*/)
            {
                if (ec || this->exit_signal_.load(std::memory_order_acquire))
                {
                    this->running_flag_.store(false, std::memory_order_release);
                    return;
                }

                if (
                    this->state_->current_port_ == this->params_->scan_end_port &&
                    this->state_->current_loop_ + 1 == this->params_->loop_count
                )
                {
                    this->running_flag_.store(false, std::memory_order_release);
                    return;
                }
                else if(this->state_->current_port_ < this->params_->scan_end_port)
                {
                    this->state_->current_port_++;
                }
                else
                {
                    this->state_->current_loop_++;
                    this->state_->current_port_ = this->params_->scan_start_port;
                }

                this->do_send_impl();
            }
        );
    }
    catch (...)
    {
        running_flag_.store(false, std::memory_order_release);
        throw;
    }
}

}
