#include "protocol.h"
#include "crc8.h"
#include <string.h>

static size_t pack_body(uint8_t dest, uint8_t src, uint8_t msg_type, uint8_t seq,
                        const void *body, size_t body_len, uint8_t *out, size_t cap)
{
    proto_packet_t pkt;

    if (body_len > PROTO_MAX_PAYLOAD || (PROTO_HEADER_SIZE + body_len + 1U) > cap) {
        return 0;
    }

    pkt.dest = dest;
    pkt.src = src;
    pkt.msg_type = msg_type;
    pkt.seq = seq;
    pkt.payload_len = (uint8_t)body_len;
    if (body_len > 0U && body != NULL) {
        memcpy(pkt.payload, body, body_len);
    }
    return protocol_pack(&pkt, out, cap);
}

static size_t protocol_expected_len(uint8_t msg_type)
{
    switch (msg_type) {
    case PROTO_MSG_STATUS:    return PROTO_HEADER_SIZE + sizeof(proto_status_t) + 1U;
    case PROTO_MSG_RFID_TAG:  return PROTO_HEADER_SIZE + sizeof(proto_rfid_tag_t) + 1U;
    case PROTO_MSG_FEED_CMD:  return PROTO_HEADER_SIZE + sizeof(proto_feed_cmd_t) + 1U;
    case PROTO_MSG_FEED_DONE: return PROTO_HEADER_SIZE + sizeof(proto_feed_done_t) + 1U;
    case PROTO_MSG_WEIGHT:    return PROTO_HEADER_SIZE + sizeof(proto_weight_t) + 1U;
    case PROTO_MSG_CONFIG:    return PROTO_HEADER_SIZE + sizeof(proto_config_t) + 1U;
    case PROTO_MSG_ACK:       return PROTO_HEADER_SIZE + sizeof(proto_ack_t) + 1U;
    case PROTO_MSG_ERROR:     return PROTO_HEADER_SIZE + sizeof(proto_error_t) + 1U;
    default:
        return 0U;
    }
}

size_t protocol_pack(const proto_packet_t *pkt, uint8_t *out, size_t out_cap)
{
    size_t total;

    if (pkt == NULL || out == NULL) {
        return 0;
    }
    if (pkt->payload_len > PROTO_MAX_PAYLOAD) {
        return 0;
    }

    total = PROTO_HEADER_SIZE + pkt->payload_len + 1U;
    if (total > out_cap || total > PROTO_MAX_PACKET) {
        return 0;
    }

    out[0] = pkt->dest;
    out[1] = pkt->src;
    out[2] = pkt->msg_type;
    out[3] = pkt->seq;
    if (pkt->payload_len > 0U) {
        memcpy(&out[4], pkt->payload, pkt->payload_len);
    }
    out[total - 1U] = crc8_calc(out, total - 1U);
    return total;
}

bool protocol_unpack(const uint8_t *in, size_t in_len, proto_packet_t *pkt)
{
    size_t len = in_len;

    if (in == NULL || pkt == NULL || in_len < (PROTO_HEADER_SIZE + 1U) || in_len > PROTO_MAX_PACKET) {
        return false;
    }

    if (len == PROTO_MAX_PACKET) {
        size_t expected = protocol_expected_len(in[2]);
        if (expected >= (PROTO_HEADER_SIZE + 1U) && expected <= len) {
            len = expected;
        }
    }

    if (!protocol_crc_ok(in, len)) {
        return false;
    }

    pkt->dest = in[0];
    pkt->src = in[1];
    pkt->msg_type = in[2];
    pkt->seq = in[3];
    pkt->payload_len = (uint8_t)(len - PROTO_HEADER_SIZE - 1U);
    if (pkt->payload_len > 0U) {
        memcpy(pkt->payload, &in[4], pkt->payload_len);
    }
    return true;
}

bool protocol_crc_ok(const uint8_t *buf, size_t len)
{
    uint8_t expected;

    if (buf == NULL || len < (PROTO_HEADER_SIZE + 1U)) {
        return false;
    }
    expected = crc8_calc(buf, len - 1U);
    return (expected == buf[len - 1U]);
}

size_t protocol_build_status(uint8_t dest, uint8_t src, uint8_t seq,
                             const proto_status_t *body, uint8_t *out, size_t cap)
{
    return pack_body(dest, src, PROTO_MSG_STATUS, seq, body, sizeof(proto_status_t), out, cap);
}

size_t protocol_build_rfid_tag(uint8_t dest, uint8_t src, uint8_t seq,
                               const proto_rfid_tag_t *body, uint8_t *out, size_t cap)
{
    return pack_body(dest, src, PROTO_MSG_RFID_TAG, seq, body, sizeof(proto_rfid_tag_t), out, cap);
}

size_t protocol_build_feed_done(uint8_t dest, uint8_t src, uint8_t seq,
                                const proto_feed_done_t *body, uint8_t *out, size_t cap)
{
    return pack_body(dest, src, PROTO_MSG_FEED_DONE, seq, body, sizeof(proto_feed_done_t), out, cap);
}

size_t protocol_build_weight(uint8_t dest, uint8_t src, uint8_t seq,
                             const proto_weight_t *body, uint8_t *out, size_t cap)
{
    return pack_body(dest, src, PROTO_MSG_WEIGHT, seq, body, sizeof(proto_weight_t), out, cap);
}

bool protocol_parse_feed_cmd(const proto_packet_t *pkt, proto_feed_cmd_t *out)
{
    if (pkt == NULL || out == NULL) {
        return false;
    }
    if (pkt->msg_type != PROTO_MSG_FEED_CMD) {
        return false;
    }
    if (pkt->payload_len != sizeof(proto_feed_cmd_t)) {
        return false;
    }
    memcpy(out, pkt->payload, sizeof(proto_feed_cmd_t));
    return true;
}
