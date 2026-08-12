#ifndef WIFEEDER_NRF_LINK_H
#define WIFEEDER_NRF_LINK_H

#include "protocol.h"
#include "nrf24l01.h"
#include "config.h"
#include "error.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

namespace wifeeder {

struct NrfMessage {
    uint8_t msg_type = 0;
    uint8_t src_id = 0;
    uint8_t dest_id = 0;
    uint8_t seq = 0;
    std::vector<uint8_t> payload;
};

class NrfLink {
public:
    NrfLink();
    ~NrfLink();

    int32_t init(const std::string& spi_device = NRF_SPI_DEVICE, int ce_gpio = NRF_CE_GPIO);
    int32_t start();
    int32_t stop();

    int32_t sendto(uint8_t msg_type, uint8_t dest_id, const void* payload, size_t payload_len);
    bool recv(uint8_t msg_type, NrfMessage& out, int timeout_ms = 1000);
    bool recv_any(NrfMessage& out, int timeout_ms = 1000);

    void inject_rx_for_test(const uint8_t* raw, size_t len);

private:
    Nrf24L01 nrf_;
    std::thread rx_thread_;
    std::atomic<bool> running_{false};
    uint8_t tx_seq_{0};

    std::mutex rx_mtx_;
    std::condition_variable rx_cv_;
    std::deque<NrfMessage> rx_queue_;

    std::mutex tx_mtx_;

    void rx_task();
    void enqueue(const ProtoPacket& pkt);
    int32_t transmit_raw(const std::vector<uint8_t>& frame);
};

} /* namespace wifeeder */

#endif /* WIFEEDER_NRF_LINK_H */
