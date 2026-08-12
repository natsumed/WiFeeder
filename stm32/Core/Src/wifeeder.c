/*
 * wifeeder.c - Simplified feeding loop ported from v1
 */
#include "wifeeder.h"
#include "board.h"
#include "protocol.h"
#include "nrf24l01.h"
#include "rfid.h"
#include "motor.h"
#include "encoder.h"
#include "flash_int.h"
#include <string.h>

#define WIFEEDER_STATUS_INTERVAL_MS     30000U
#define WIFEEDER_STATUS_RETRY_MS        1000U
#define WIFEEDER_RFID_DEBOUNCE_MS       30000U
#define WIFEEDER_FEED_CMD_TIMEOUT_MS    300000U
#define WIFEEDER_FEED_STALL_MS          1500U
#define WIFEEDER_DEFAULT_DEVICE_ID      2U

typedef enum {
    WIFEEDER_STATE_IDLE = 0,
    WIFEEDER_STATE_WAIT_FEED_CMD,
    WIFEEDER_STATE_FEEDING
} wifeeder_state_t;

static uint8_t gs_device_id = WIFEEDER_DEFAULT_DEVICE_ID;
static uint8_t gs_seq;
static uint32_t gs_status_due_ms;
static uint32_t gs_last_tag_id;
static uint32_t gs_last_tag_ms;
static uint32_t gs_active_animal_id;
static uint16_t gs_target_m1_revs;
static uint16_t gs_target_m2_revs;
static int32_t gs_feed_target_pulses;
static uint32_t gs_feed_stall_check_ms;
static int32_t gs_feed_last_pulses;
static bool gs_m1_done;
static bool gs_m2_done;
static wifeeder_state_t gs_state;
static bool gs_link_up;
static uint32_t gs_feed_cmd_deadline_ms;

__attribute__((weak)) bool wifeeder_radio_send(const uint8_t *buf, uint8_t len)
{
    return nrf24_send(buf, len);
}

__attribute__((weak)) bool wifeeder_radio_recv(uint8_t *buf, uint8_t len)
{
    return nrf24_recv(buf, len);
}

__attribute__((weak)) bool wifeeder_radio_available(void)
{
    return nrf24_is_data_available();
}

#if !WIFEEDER_MVP
static void pack_u24(uint8_t out[3], uint32_t value)
{
    out[0] = (uint8_t)(value & 0xFFU);
    out[1] = (uint8_t)((value >> 8) & 0xFFU);
    out[2] = (uint8_t)((value >> 16) & 0xFFU);
}
#endif

static bool send_packet(const uint8_t *buf, size_t len)
{
    return wifeeder_radio_send(buf, (uint8_t)len);
}

static bool send_status(void)
{
    proto_status_t body;
    uint8_t tx[PROTO_MAX_PACKET];
    size_t len;

    memset(&body, 0, sizeof(body));
    body.heartbeat = board_millis();
    body.sensor_status = 0;
    body.battery = 0xFFU;

    len = protocol_build_status(PROTO_ID_CONTROLLER, gs_device_id, gs_seq++, &body, tx, sizeof(tx));
    if (len == 0U) {
        return false;
    }
    return send_packet(tx, len);
}

static bool wait_for_ack(uint8_t echo_type, uint32_t timeout_ms)
{
    uint32_t end = board_millis() + timeout_ms;
    uint8_t rx[PROTO_MAX_PACKET];
    proto_packet_t pkt;
    proto_ack_t ack;

    while (board_millis() < end) {
        if (wifeeder_radio_available() && wifeeder_radio_recv(rx, sizeof(rx))) {
            if (protocol_unpack(rx, sizeof(rx), &pkt) &&
                pkt.msg_type == PROTO_MSG_ACK &&
                pkt.payload_len == sizeof(proto_ack_t)) {
                memcpy(&ack, pkt.payload, sizeof(ack));
                if (ack.echo_msg_type == echo_type && ack.status == PROTO_ACK_OK) {
                    return true;
                }
            }
        }
    }
    return false;
}

#if !WIFEEDER_MVP
static bool send_rfid_tag(uint32_t tag_id)
{
    proto_rfid_tag_t body;
    uint8_t tx[PROTO_MAX_PACKET];
    size_t len;

    body.tag_id = tag_id;
    pack_u24(body.timestamp, board_millis());

    len = protocol_build_rfid_tag(PROTO_ID_CONTROLLER, gs_device_id, gs_seq++, &body, tx, sizeof(tx));
    if (len == 0U) {
        return false;
    }
    return send_packet(tx, len);
}
#endif

static bool send_feed_done(uint32_t animal_id, uint16_t m1_actual, uint16_t m2_actual)
{
    proto_feed_done_t body;
    uint8_t tx[PROTO_MAX_PACKET];
    size_t len;

    body.animal_id = animal_id;
    body.motor1_actual = m1_actual;
    body.motor2_actual = m2_actual;

    len = protocol_build_feed_done(PROTO_ID_CONTROLLER, gs_device_id, gs_seq++, &body, tx, sizeof(tx));
    if (len == 0U) {
        return false;
    }
    return send_packet(tx, len);
}

static bool poll_feed_cmd(proto_feed_cmd_t *cmd_out)
{
    uint8_t rx[PROTO_MAX_PACKET];
    proto_packet_t pkt;

    if (!wifeeder_radio_available() || !wifeeder_radio_recv(rx, sizeof(rx))) {
        return false;
    }
    if (!protocol_unpack(rx, sizeof(rx), &pkt)) {
        return false;
    }
    if (pkt.dest != gs_device_id && pkt.dest != 0xFFU) {
        return false;
    }
    return protocol_parse_feed_cmd(&pkt, cmd_out);
}

static uint16_t pulses_to_revs(int32_t pulses)
{
    int32_t abs_pulses = pulses;
    if (abs_pulses < 0) {
        abs_pulses = -abs_pulses;
    }
    return (uint16_t)(abs_pulses / (int32_t)BOARD_ENCODER_COUNTS_PER_REV);
}

static void start_feeding(uint16_t m1_revs, uint16_t m2_revs)
{
    gs_target_m1_revs = m1_revs;
    gs_target_m2_revs = m2_revs;
    gs_feed_target_pulses = (int32_t)m1_revs * (int32_t)BOARD_ENCODER_COUNTS_PER_REV;
    gs_feed_stall_check_ms = board_millis() + WIFEEDER_FEED_STALL_MS;
    gs_feed_last_pulses = 0;
    gs_m1_done = (m1_revs == 0U);
    gs_m2_done = (m2_revs == 0U);

    encoder_reset(MOTOR_1);
    if (m1_revs > 0U) {
        motor_run(MOTOR_1, m1_revs);
    }
    if (m2_revs > 0U) {
        /* Motor2 deferred (PCA9685 MVP uses PB6/PB7 for I2C). */
        gs_m2_done = true;
        gs_target_m2_revs = 0U;
    }
    gs_state = WIFEEDER_STATE_FEEDING;
}

static void service_status(void)
{
    if (board_millis() < gs_status_due_ms) {
        return;
    }

    if (send_status() && wait_for_ack(PROTO_MSG_STATUS, 300U)) {
        gs_link_up = true;
        gs_status_due_ms = board_millis() + WIFEEDER_STATUS_INTERVAL_MS;
    } else {
        gs_link_up = false;
        gs_status_due_ms = board_millis() + WIFEEDER_STATUS_RETRY_MS;
    }
}

static void service_rfid(void)
{
#if WIFEEDER_MVP
    /* RFID not on MVP Fritzing pack — enable with -DWIFEEDER_MVP=0 */
    return;
#else
    uint32_t tag_id;

    if (!gs_link_up || gs_state != WIFEEDER_STATE_IDLE) {
        return;
    }

    if (!rfid_poll(&tag_id)) {
        return;
    }

    if (tag_id == gs_last_tag_id &&
        (board_millis() - gs_last_tag_ms) < WIFEEDER_RFID_DEBOUNCE_MS) {
        return;
    }

    gs_last_tag_id = tag_id;
    gs_last_tag_ms = board_millis();
    gs_active_animal_id = tag_id;

    if (send_rfid_tag(tag_id)) {
        gs_feed_cmd_deadline_ms = board_millis() + WIFEEDER_FEED_CMD_TIMEOUT_MS;
        gs_state = WIFEEDER_STATE_WAIT_FEED_CMD;
    }
#endif
}

static void service_wait_feed_cmd(void)
{
    proto_feed_cmd_t cmd;

    if (gs_state != WIFEEDER_STATE_WAIT_FEED_CMD) {
        return;
    }

    if (board_millis() > gs_feed_cmd_deadline_ms) {
        gs_state = WIFEEDER_STATE_IDLE;
        return;
    }

    if (!poll_feed_cmd(&cmd)) {
        return;
    }
    if (cmd.animal_id != gs_active_animal_id) {
        return;
    }

    start_feeding(cmd.motor1_revs, cmd.motor2_revs);
}

static void service_feeding(void)
{
    int32_t pulses;
    uint16_t m1_actual;
    uint16_t m2_actual;

    if (gs_state != WIFEEDER_STATE_FEEDING) {
        return;
    }

    if (!gs_m1_done) {
        pulses = encoder_get_count(MOTOR_1);
        if (pulses >= gs_feed_target_pulses || pulses <= -gs_feed_target_pulses) {
            motor_stop(MOTOR_1);
            gs_m1_done = true;
        } else if (board_millis() >= gs_feed_stall_check_ms) {
            if (pulses == gs_feed_last_pulses) {
                motor_stop(MOTOR_1);
                gs_m1_done = true;
            }
            gs_feed_last_pulses = pulses;
            gs_feed_stall_check_ms = board_millis() + WIFEEDER_FEED_STALL_MS;
        }
    }

    if (!gs_m2_done && gs_m1_done) {
        motor_stop(MOTOR_2);
        gs_m2_done = true;
    }

    if (!gs_m1_done || !gs_m2_done) {
        return;
    }

    m1_actual = (gs_target_m1_revs > 0U) ? pulses_to_revs(encoder_get_count(MOTOR_1)) : 0U;
    m2_actual = gs_target_m2_revs;
    (void)send_feed_done(gs_active_animal_id, m1_actual, m2_actual);
    gs_state = WIFEEDER_STATE_IDLE;
}

void wifeeder_init(void)
{
    uint32_t stored_id = WIFEEDER_DEFAULT_DEVICE_ID;
    uint32_t machine_type = 1U;

    board_systick_init();
    board_gpio_enable_clocks();

    if (flash_int_read_id(&stored_id)) {
        if (stored_id > 0U && stored_id < 255U) {
            gs_device_id = (uint8_t)stored_id;
        }
    }
    (void)flash_int_read_type(&machine_type);

#if !WIFEEDER_MVP
    rfid_init();
#endif
    encoder_init();
    motor_init();
    nrf24_init();
    if (nrf24_is_connected()) {
        nrf24_config_esb();
    }

    gs_seq = 0;
    gs_status_due_ms = board_millis() + 1000U;
    gs_last_tag_id = 0;
    gs_last_tag_ms = 0;
    gs_state = WIFEEDER_STATE_IDLE;
    gs_link_up = false;
}

void wifeeder_poll(void)
{
    service_status();
    service_rfid();
    service_wait_feed_cmd();
    service_feeding();
}
