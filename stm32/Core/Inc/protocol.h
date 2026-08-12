/*
 * protocol.h - WiFeeder v2 NRF application packet layer
 *
 * Layout: DestID, SrcID, MsgType, Seq, payload..., CRC8 (last byte)
 */
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define PROTO_MAX_PACKET        32U
#define PROTO_HEADER_SIZE       4U
#define PROTO_MAX_PAYLOAD       (PROTO_MAX_PACKET - PROTO_HEADER_SIZE - 1U)

#define PROTO_ID_CONTROLLER     0x00U

#define PROTO_MSG_STATUS        0x01U
#define PROTO_MSG_RFID_TAG      0x02U
#define PROTO_MSG_FEED_CMD      0x03U
#define PROTO_MSG_FEED_DONE     0x04U
#define PROTO_MSG_WEIGHT        0x05U
#define PROTO_MSG_CONFIG        0x06U
#define PROTO_MSG_ACK           0x07U
#define PROTO_MSG_ERROR         0x08U

#define PROTO_ACK_OK            0x00U

typedef struct __attribute__((packed)) {
    uint32_t heartbeat;
    uint8_t  sensor_status;
    uint8_t  battery;
    uint8_t  reserved[3];
} proto_status_t;

typedef struct __attribute__((packed)) {
    uint32_t tag_id;
    uint8_t  timestamp[3];
} proto_rfid_tag_t;

typedef struct __attribute__((packed)) {
    uint32_t animal_id;
    uint16_t motor1_revs;
    uint16_t motor2_revs;
} proto_feed_cmd_t;

typedef struct __attribute__((packed)) {
    uint32_t animal_id;
    uint16_t motor1_actual;
    uint16_t motor2_actual;
} proto_feed_done_t;

typedef struct __attribute__((packed)) {
    uint32_t animal_id;
    uint32_t weight_g;
} proto_weight_t;

typedef struct __attribute__((packed)) {
    uint8_t  param_id;
    uint8_t  reserved[3];
    uint32_t value;
} proto_config_t;

typedef struct __attribute__((packed)) {
    uint8_t echo_msg_type;
    uint8_t status;
} proto_ack_t;

typedef struct __attribute__((packed)) {
    uint8_t  error_code;
    uint8_t  reserved[3];
    uint32_t details;
} proto_error_t;

typedef struct {
    uint8_t dest;
    uint8_t src;
    uint8_t msg_type;
    uint8_t seq;
    uint8_t payload[PROTO_MAX_PAYLOAD];
    uint8_t payload_len;
} proto_packet_t;

size_t protocol_pack(const proto_packet_t *pkt, uint8_t *out, size_t out_cap);
bool protocol_unpack(const uint8_t *in, size_t in_len, proto_packet_t *pkt);
bool protocol_crc_ok(const uint8_t *buf, size_t len);

size_t protocol_build_status(uint8_t dest, uint8_t src, uint8_t seq,
                             const proto_status_t *body, uint8_t *out, size_t cap);
size_t protocol_build_rfid_tag(uint8_t dest, uint8_t src, uint8_t seq,
                               const proto_rfid_tag_t *body, uint8_t *out, size_t cap);
size_t protocol_build_feed_done(uint8_t dest, uint8_t src, uint8_t seq,
                                const proto_feed_done_t *body, uint8_t *out, size_t cap);
size_t protocol_build_weight(uint8_t dest, uint8_t src, uint8_t seq,
                             const proto_weight_t *body, uint8_t *out, size_t cap);
bool protocol_parse_feed_cmd(const proto_packet_t *pkt, proto_feed_cmd_t *out);

#endif /* PROTOCOL_H */
