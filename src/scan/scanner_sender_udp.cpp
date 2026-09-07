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

scanner_sender_udp_params::scanner_sender_udp_params(
    std::string        _target_ip,
    std::uint32_t      _target_port,
    std::uint32_t      _scan_start_port,
    std::uint32_t      _scan_end_port,
    std::uint32_t      _loop_count,
    std::uint32_t      _pps,
    ping_packet_data_t _ping_packet_data
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

scanner_sender_udp::scanner_sender_udp(boost::asio::ip::udp::socket& _socket)
: socket_ { _socket }
, exit_signal_ { false }
{
}

void scanner_sender_udp::do_send(std::shared_ptr<scanner_sender_udp_params> _params)
{
    params_ = _params;

    if (
        params_->loop_count == 0                           ||
        params_->scan_start_port == 0                      ||
        params_->scan_end_port == UINT16_MAX               ||
        params_->scan_start_port > params_->scan_end_port
    )
    {
        throw std::invalid_argument("invalid UDP scan params.");
    }

    state_ = std::make_shared<scanner_sender_udp_state>();
    state_->target_endpoint = boost::asio::ip::udp::endpoint(
        boost::asio::ip::make_address(params_->target_ip),
        static_cast<std::uint16_t>(params_->scan_start_port)
    );
    state_->current_port_ = params_->scan_start_port;
    state_->current_loop_ = 0;
    state_->send_buffer = ping_packet_t::serialize(params_->ping_packet_data);

    do_send_impl();
}

void scanner_sender_udp::exit()
{
    exit_signal_.store(true, std::memory_order_release);
}

void scanner_sender_udp::do_send_impl()
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
                return;
            }

            if (
                this->state_->current_port_ == this->params_->scan_end_port &&
                this->state_->current_loop_ == this->params_->loop_count
            )
            {
                return;
            }
            else if(this->state_->current_port_ < this->params_->scan_end_port)
            {
                this->state_->current_port_++;
            }
            else if(this->state_->current_loop_ < this->params_->loop_count)
            {
                this->state_->current_loop_++;
                this->state_->current_port_ = this->params_->scan_start_port;
            }

            this->do_send_impl();
        }
    );
}

}
