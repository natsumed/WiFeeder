#include "nrf_link.h"
#include "config.h"

#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

namespace wifeeder {

NrfLink::NrfLink() = default;

NrfLink::~NrfLink()
{
    stop();
}

int32_t NrfLink::init(const std::string& spi_device, int ce_gpio)
{
    return nrf_.init(spi_device, ce_gpio);
}

int32_t NrfLink::start()
{
    if (running_) {
        return ENOERR;
    }
    running_ = true;
    nrf_.set_mode_rx();
    rx_thread_ = std::thread(&NrfLink::rx_task, this);
    return ENOERR;
}

int32_t NrfLink::stop()
{
    if (!running_) {
        return ENOERR;
    }
    running_ = false;
    rx_cv_.notify_all();
    if (rx_thread_.joinable()) {
        rx_thread_.join();
    }
    nrf_.deinit();
    return ENOERR;
}

int32_t NrfLink::sendto(uint8_t msg_type, uint8_t dest_id, const void* payload, size_t payload_len)
{
    if (payload_len > PROTO_MAX_PAYLOAD) {
        return EARG;
    }

    ProtoPacket pkt{};
    pkt.dest = dest_id;
    pkt.src = PROTO_ID_CONTROLLER;
    pkt.msg_type = msg_type;
    pkt.seq = tx_seq_++;
    pkt.payload_len = static_cast<uint8_t>(payload_len);
    if (payload_len > 0 && payload != nullptr) {
        std::memcpy(pkt.payload, payload, payload_len);
    }

    std::vector<uint8_t> frame(PROTO_MAX_PACKET);
    const size_t packed = Protocol::pack(pkt, frame.data(), frame.size());
    if (packed == 0) {
        return EFORMAT;
    }
    frame.resize(packed);

    std::lock_guard<std::mutex> lock(tx_mtx_);
    return transmit_raw(frame);
}

bool NrfLink::recv(uint8_t msg_type, NrfMessage& out, int timeout_ms)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    std::unique_lock<std::mutex> lock(rx_mtx_);
    while (running_) {
        for (auto it = rx_queue_.begin(); it != rx_queue_.end(); ++it) {
            if (it->msg_type == msg_type) {
                out = *it;
                rx_queue_.erase(it);
                return true;
            }
        }
        if (rx_cv_.wait_until(lock, deadline) == std::cv_status::timeout) {
            return false;
        }
    }
    return false;
}

bool NrfLink::recv_any(NrfMessage& out, int timeout_ms)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    std::unique_lock<std::mutex> lock(rx_mtx_);
    while (running_) {
        if (!rx_queue_.empty()) {
            out = rx_queue_.front();
            rx_queue_.pop_front();
            return true;
        }
        if (rx_cv_.wait_until(lock, deadline) == std::cv_status::timeout) {
            return false;
        }
    }
    return false;
}

void NrfLink::inject_rx_for_test(const uint8_t* raw, size_t len)
{
    ProtoPacket pkt{};
    if (!Protocol::unpack(raw, len, pkt)) {
        return;
    }
    enqueue(pkt);
}

void NrfLink::rx_task()
{
    uint8_t buf[PROTO_MAX_PACKET];
    while (running_) {
        if (nrf_.data_ready()) {
            if (nrf_.read_payload(buf, sizeof(buf)) > 0) {
                ProtoPacket pkt{};
                if (Protocol::unpack(buf, sizeof(buf), pkt)) {
                    enqueue(pkt);
                    ProtoAck ack{};
                    ack.echo_msg_type = pkt.msg_type;
                    ack.status = PROTO_ACK_OK;
                    std::vector<uint8_t> frame;
                    Protocol::build_ack(pkt.src, PROTO_ID_CONTROLLER, tx_seq_++, ack, frame);
                    std::lock_guard<std::mutex> lock(tx_mtx_);
                    transmit_raw(frame);
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

void NrfLink::enqueue(const ProtoPacket& pkt)
{
    NrfMessage msg{};
    msg.msg_type = pkt.msg_type;
    msg.src_id = pkt.src;
    msg.dest_id = pkt.dest;
    msg.seq = pkt.seq;
    msg.payload.assign(pkt.payload, pkt.payload + pkt.payload_len);

    {
        std::lock_guard<std::mutex> lock(rx_mtx_);
        rx_queue_.push_back(std::move(msg));
    }
    rx_cv_.notify_all();

    std::cout << "[nrf_link] RX type=0x" << std::hex << static_cast<int>(pkt.msg_type)
              << " src=0x" << static_cast<int>(pkt.src) << std::dec << std::endl;
}

int32_t NrfLink::transmit_raw(const std::vector<uint8_t>& frame)
{
    if (frame.empty()) {
        return EARG;
    }
    nrf_.set_mode_tx();
    const int32_t rc = nrf_.write_payload(frame.data(), frame.size());
    nrf_.set_mode_rx();
    if (rc < 0) {
        return rc;
    }
    return ENOERR;
}

} /* namespace wifeeder */
