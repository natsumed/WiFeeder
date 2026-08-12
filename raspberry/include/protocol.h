#ifndef WIFEEDER_PROTOCOL_H
#define WIFEEDER_PROTOCOL_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace wifeeder {

constexpr size_t PROTO_MAX_PACKET = 32U;
constexpr size_t PROTO_HEADER_SIZE = 4U;
constexpr size_t PROTO_MAX_PAYLOAD = PROTO_MAX_PACKET - PROTO_HEADER_SIZE - 1U;

constexpr uint8_t PROTO_MSG_STATUS = 0x01U;
constexpr uint8_t PROTO_MSG_RFID_TAG = 0x02U;
constexpr uint8_t PROTO_MSG_FEED_CMD = 0x03U;
constexpr uint8_t PROTO_MSG_FEED_DONE = 0x04U;
constexpr uint8_t PROTO_MSG_WEIGHT = 0x05U;
constexpr uint8_t PROTO_MSG_CONFIG = 0x06U;
constexpr uint8_t PROTO_MSG_ACK = 0x07U;
constexpr uint8_t PROTO_MSG_ERROR = 0x08U;

constexpr uint8_t PROTO_ACK_OK = 0x00U;

#pragma pack(push, 1)

struct ProtoStatus {
    uint32_t heartbeat;
    uint8_t sensor_status;
    uint8_t battery;
    uint8_t reserved[3];
};

struct ProtoRfidTag {
    uint32_t tag_id;
    uint8_t timestamp[3];
};

struct ProtoFeedCmd {
    uint32_t animal_id;
    uint16_t motor1_revs;
    uint16_t motor2_revs;
};

struct ProtoFeedDone {
    uint32_t animal_id;
    uint16_t motor1_actual;
    uint16_t motor2_actual;
};

struct ProtoWeight {
    uint32_t animal_id;
    uint32_t weight_g;
};

struct ProtoConfig {
    uint8_t param_id;
    uint8_t reserved[3];
    uint32_t value;
};

struct ProtoAck {
    uint8_t echo_msg_type;
    uint8_t status;
};

struct ProtoError {
    uint8_t error_code;
    uint8_t reserved[3];
    uint32_t details;
};

#pragma pack(pop)

struct ProtoPacket {
    uint8_t dest = 0;
    uint8_t src = 0;
    uint8_t msg_type = 0;
    uint8_t seq = 0;
    uint8_t payload[PROTO_MAX_PAYLOAD]{};
    uint8_t payload_len = 0;
};

uint8_t crc8_calc(const uint8_t* data, size_t len);

class Protocol {
public:
    static size_t pack(const ProtoPacket& pkt, uint8_t* out, size_t out_cap);
    static bool unpack(const uint8_t* in, size_t in_len, ProtoPacket& pkt);
    static bool crc_ok(const uint8_t* buf, size_t len);

    static size_t build_status(uint8_t dest, uint8_t src, uint8_t seq,
                               const ProtoStatus& body, std::vector<uint8_t>& out);
    static size_t build_rfid_tag(uint8_t dest, uint8_t src, uint8_t seq,
                                 const ProtoRfidTag& body, std::vector<uint8_t>& out);
    static size_t build_feed_cmd(uint8_t dest, uint8_t src, uint8_t seq,
                                 const ProtoFeedCmd& body, std::vector<uint8_t>& out);
    static size_t build_feed_done(uint8_t dest, uint8_t src, uint8_t seq,
                                  const ProtoFeedDone& body, std::vector<uint8_t>& out);
    static size_t build_weight(uint8_t dest, uint8_t src, uint8_t seq,
                               const ProtoWeight& body, std::vector<uint8_t>& out);
    static size_t build_ack(uint8_t dest, uint8_t src, uint8_t seq,
                            const ProtoAck& body, std::vector<uint8_t>& out);

    static bool parse_status(const ProtoPacket& pkt, ProtoStatus& out);
    static bool parse_rfid_tag(const ProtoPacket& pkt, ProtoRfidTag& out);
    static bool parse_feed_cmd(const ProtoPacket& pkt, ProtoFeedCmd& out);
    static bool parse_feed_done(const ProtoPacket& pkt, ProtoFeedDone& out);
    static bool parse_weight(const ProtoPacket& pkt, ProtoWeight& out);
    static bool parse_ack(const ProtoPacket& pkt, ProtoAck& out);
    static bool parse_error(const ProtoPacket& pkt, ProtoError& out);
};

} /* namespace wifeeder */

#endif /* WIFEEDER_PROTOCOL_H */
