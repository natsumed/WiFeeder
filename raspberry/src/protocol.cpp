#include "protocol.h"

#include <cstring>

namespace wifeeder {

uint8_t crc8_calc(const uint8_t* data, size_t len)
{
    uint8_t crc = 0;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x80U) {
                crc = static_cast<uint8_t>((crc << 1) ^ 0x07U);
            } else {
                crc = static_cast<uint8_t>(crc << 1);
            }
        }
    }
    return crc;
}

static size_t expected_len(uint8_t msg_type)
{
    switch (msg_type) {
    case PROTO_MSG_STATUS: return PROTO_HEADER_SIZE + sizeof(ProtoStatus) + 1U;
    case PROTO_MSG_RFID_TAG: return PROTO_HEADER_SIZE + sizeof(ProtoRfidTag) + 1U;
    case PROTO_MSG_FEED_CMD: return PROTO_HEADER_SIZE + sizeof(ProtoFeedCmd) + 1U;
    case PROTO_MSG_FEED_DONE: return PROTO_HEADER_SIZE + sizeof(ProtoFeedDone) + 1U;
    case PROTO_MSG_WEIGHT: return PROTO_HEADER_SIZE + sizeof(ProtoWeight) + 1U;
    case PROTO_MSG_CONFIG: return PROTO_HEADER_SIZE + sizeof(ProtoConfig) + 1U;
    case PROTO_MSG_ACK: return PROTO_HEADER_SIZE + sizeof(ProtoAck) + 1U;
    case PROTO_MSG_ERROR: return PROTO_HEADER_SIZE + sizeof(ProtoError) + 1U;
    default: return 0U;
    }
}

static size_t pack_body(uint8_t dest, uint8_t src, uint8_t msg_type, uint8_t seq,
                        const void* body, size_t body_len, std::vector<uint8_t>& out)
{
    ProtoPacket pkt{};
    if (body_len > PROTO_MAX_PAYLOAD) {
        return 0;
    }
    pkt.dest = dest;
    pkt.src = src;
    pkt.msg_type = msg_type;
    pkt.seq = seq;
    pkt.payload_len = static_cast<uint8_t>(body_len);
    if (body_len > 0 && body != nullptr) {
        std::memcpy(pkt.payload, body, body_len);
    }
    out.resize(PROTO_MAX_PACKET);
    return Protocol::pack(pkt, out.data(), out.size());
}

size_t Protocol::pack(const ProtoPacket& pkt, uint8_t* out, size_t out_cap)
{
    if (out == nullptr || pkt.payload_len > PROTO_MAX_PAYLOAD) {
        return 0;
    }
    const size_t total = PROTO_HEADER_SIZE + pkt.payload_len + 1U;
    if (total > out_cap || total > PROTO_MAX_PACKET) {
        return 0;
    }
    out[0] = pkt.dest;
    out[1] = pkt.src;
    out[2] = pkt.msg_type;
    out[3] = pkt.seq;
    if (pkt.payload_len > 0) {
        std::memcpy(&out[4], pkt.payload, pkt.payload_len);
    }
    out[total - 1U] = crc8_calc(out, total - 1U);
    return total;
}

bool Protocol::unpack(const uint8_t* in, size_t in_len, ProtoPacket& pkt)
{
    if (in == nullptr || in_len < PROTO_HEADER_SIZE + 1U || in_len > PROTO_MAX_PACKET) {
        return false;
    }
    size_t len = in_len;
    if (len == PROTO_MAX_PACKET) {
        const size_t expected = expected_len(in[2]);
        if (expected >= PROTO_HEADER_SIZE + 1U && expected <= len) {
            len = expected;
        }
    }
    if (!crc_ok(in, len)) {
        return false;
    }
    pkt.dest = in[0];
    pkt.src = in[1];
    pkt.msg_type = in[2];
    pkt.seq = in[3];
    pkt.payload_len = static_cast<uint8_t>(len - PROTO_HEADER_SIZE - 1U);
    if (pkt.payload_len > 0) {
        std::memcpy(pkt.payload, &in[4], pkt.payload_len);
    }
    return true;
}

bool Protocol::crc_ok(const uint8_t* buf, size_t len)
{
    if (buf == nullptr || len < PROTO_HEADER_SIZE + 1U) {
        return false;
    }
    return crc8_calc(buf, len - 1U) == buf[len - 1U];
}

size_t Protocol::build_status(uint8_t dest, uint8_t src, uint8_t seq,
                              const ProtoStatus& body, std::vector<uint8_t>& out)
{
    return pack_body(dest, src, PROTO_MSG_STATUS, seq, &body, sizeof(body), out);
}

size_t Protocol::build_rfid_tag(uint8_t dest, uint8_t src, uint8_t seq,
                                const ProtoRfidTag& body, std::vector<uint8_t>& out)
{
    return pack_body(dest, src, PROTO_MSG_RFID_TAG, seq, &body, sizeof(body), out);
}

size_t Protocol::build_feed_cmd(uint8_t dest, uint8_t src, uint8_t seq,
                                const ProtoFeedCmd& body, std::vector<uint8_t>& out)
{
    return pack_body(dest, src, PROTO_MSG_FEED_CMD, seq, &body, sizeof(body), out);
}

size_t Protocol::build_feed_done(uint8_t dest, uint8_t src, uint8_t seq,
                                 const ProtoFeedDone& body, std::vector<uint8_t>& out)
{
    return pack_body(dest, src, PROTO_MSG_FEED_DONE, seq, &body, sizeof(body), out);
}

size_t Protocol::build_weight(uint8_t dest, uint8_t src, uint8_t seq,
                              const ProtoWeight& body, std::vector<uint8_t>& out)
{
    return pack_body(dest, src, PROTO_MSG_WEIGHT, seq, &body, sizeof(body), out);
}

size_t Protocol::build_ack(uint8_t dest, uint8_t src, uint8_t seq,
                           const ProtoAck& body, std::vector<uint8_t>& out)
{
    return pack_body(dest, src, PROTO_MSG_ACK, seq, &body, sizeof(body), out);
}

static bool parse_payload(const ProtoPacket& pkt, uint8_t expected_type, size_t expected_size, void* out)
{
    if (pkt.msg_type != expected_type || pkt.payload_len != expected_size || out == nullptr) {
        return false;
    }
    std::memcpy(out, pkt.payload, expected_size);
    return true;
}

bool Protocol::parse_status(const ProtoPacket& pkt, ProtoStatus& out)
{
    return parse_payload(pkt, PROTO_MSG_STATUS, sizeof(ProtoStatus), &out);
}

bool Protocol::parse_rfid_tag(const ProtoPacket& pkt, ProtoRfidTag& out)
{
    return parse_payload(pkt, PROTO_MSG_RFID_TAG, sizeof(ProtoRfidTag), &out);
}

bool Protocol::parse_feed_cmd(const ProtoPacket& pkt, ProtoFeedCmd& out)
{
    return parse_payload(pkt, PROTO_MSG_FEED_CMD, sizeof(ProtoFeedCmd), &out);
}

bool Protocol::parse_feed_done(const ProtoPacket& pkt, ProtoFeedDone& out)
{
    return parse_payload(pkt, PROTO_MSG_FEED_DONE, sizeof(ProtoFeedDone), &out);
}

bool Protocol::parse_weight(const ProtoPacket& pkt, ProtoWeight& out)
{
    return parse_payload(pkt, PROTO_MSG_WEIGHT, sizeof(ProtoWeight), &out);
}

bool Protocol::parse_ack(const ProtoPacket& pkt, ProtoAck& out)
{
    return parse_payload(pkt, PROTO_MSG_ACK, sizeof(ProtoAck), &out);
}

bool Protocol::parse_error(const ProtoPacket& pkt, ProtoError& out)
{
    return parse_payload(pkt, PROTO_MSG_ERROR, sizeof(ProtoError), &out);
}

} /* namespace wifeeder */
