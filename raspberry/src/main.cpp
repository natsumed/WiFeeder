#include "config.h"
#include "database.h"
#include "diet_engine.h"
#include "mqtt_client.h"
#include "nrf_link.h"
#include "protocol.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>

using namespace wifeeder;

static std::atomic<bool> g_stop{false};
static NrfLink g_link;
static Database* g_db = nullptr;
static MqttClient* g_mqtt = nullptr;

static std::mutex g_wa_mtx;
static std::map<uint8_t, std::time_t> g_wa_last_rx;
static std::map<uint8_t, int32_t> g_wa_conn_status;

static std::mutex g_daily_mtx;

static void handle_signal(int)
{
    g_stop = true;
}

static std::string data_path()
{
    const char* env = std::getenv("WIFEEDER_DATA");
    return env != nullptr ? std::string(env) : DEFAULT_DATA_PATH;
}

static void send_ack(uint8_t dest, uint8_t echo_type)
{
    ProtoAck ack{};
    ack.echo_msg_type = echo_type;
    ack.status = PROTO_ACK_OK;
    g_link.sendto(PROTO_MSG_ACK, dest, &ack, sizeof(ack));
}

static bool packet_from_message(const NrfMessage& msg, ProtoPacket& pkt)
{
    std::vector<uint8_t> raw(PROTO_HEADER_SIZE + msg.payload.size() + 1U);
    raw[0] = msg.dest_id;
    raw[1] = msg.src_id;
    raw[2] = msg.msg_type;
    raw[3] = msg.seq;
    if (!msg.payload.empty()) {
        std::memcpy(raw.data() + 4, msg.payload.data(), msg.payload.size());
    }
    raw.back() = crc8_calc(raw.data(), raw.size() - 1U);
    return Protocol::unpack(raw.data(), raw.size(), pkt);
}

static int32_t handle_rfid_tag(uint8_t src_id, const ProtoRfidTag& tag)
{
    if (!rfid_is_valid(tag.tag_id)) {
        std::cerr << "[feed] invalid RFID " << tag.tag_id << std::endl;
        return ENOERR;
    }

    std::vector<uint32_t> p_qty;
    ProtoFeedCmd cmd{};
    cmd.animal_id = tag.tag_id;

    try {
        JsonValue json_anim = g_db->get_animal_json(tag.tag_id);
        DietEngine diet(json_anim["diet"]);
        diet.get_portion(p_qty);

        if (!p_qty.empty() && p_qty[0] >= DEF_CALIBER_REF_REVS) {
            const uint32_t density = diet.target_json()["target"][static_cast<size_t>(0)]
                .get("density", JsonValue(static_cast<unsigned int>(480))).asUInt();
            cmd.motor1_revs = static_cast<uint16_t>(grams_to_revs(p_qty[0], density));
        }
        if (p_qty.size() > 1 && p_qty[1] >= DEF_CALIBER_REF_REVS) {
            const uint32_t density = diet.target_json()["target"][static_cast<size_t>(1)]
                .get("density", JsonValue(static_cast<unsigned int>(480))).asUInt();
            cmd.motor2_revs = static_cast<uint16_t>(grams_to_revs(p_qty[1], density));
        }
    } catch (const std::exception& ex) {
        std::cerr << "[feed] RFID lookup failed for " << tag.tag_id << ": " << ex.what() << std::endl;
        JsonValue uplink;
        uplink["unknown_rfid"] = static_cast<unsigned int>(tag.tag_id);
        g_mqtt->publish(MQTT_TOPIC_DIET_UASYNC, uplink.dump());
    }

    std::cout << "[feed] FEED_CMD animal=" << cmd.animal_id
              << " m1=" << cmd.motor1_revs << " m2=" << cmd.motor2_revs << std::endl;
    return g_link.sendto(PROTO_MSG_FEED_CMD, src_id, &cmd, sizeof(cmd));
}

static void handle_feed_done(uint8_t src_id, const ProtoFeedDone& done)
{
    (void)src_id;
    std::vector<uint32_t> p_eff;
    try {
        JsonValue json_anim = g_db->get_animal_json(done.animal_id);
        DietEngine diet(json_anim["diet"]);
        const uint32_t density0 = diet.target_json()["target"][static_cast<size_t>(0)]
            .get("density", JsonValue(static_cast<unsigned int>(480))).asUInt();
        p_eff.push_back(revs_to_grams(done.motor1_actual, density0));
        if (diet.target_json()["target"].size() > 1) {
            const uint32_t density1 = diet.target_json()["target"][static_cast<size_t>(1)]
                .get("density", JsonValue(static_cast<unsigned int>(480))).asUInt();
            p_eff.push_back(revs_to_grams(done.motor2_actual, density1));
        }

        std::lock_guard<std::mutex> guard(g_daily_mtx);
        diet.log_portion(p_eff);
        json_anim["diet"] = diet.to_json();
        g_db->update_animal(json_anim);

        JsonValue uplink;
        JsonValue animal_entry;
        animal_entry["rfid"] = static_cast<unsigned int>(done.animal_id);
        animal_entry["diet"] = diet.to_json();
        uplink["animal"].append(animal_entry);
        uplink["actuator_id"] = static_cast<unsigned int>(src_id);
        g_mqtt->publish(MQTT_TOPIC_DIET_UDATA, uplink.dump());
        std::cout << "[feed] FEED_DONE logged for animal " << done.animal_id << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << "[feed] FEED_DONE processing failed: " << ex.what() << std::endl;
    }
}

static void feed_thread()
{
    while (!g_stop) {
        NrfMessage msg;
        ProtoPacket pkt{};

        if (g_link.recv(PROTO_MSG_RFID_TAG, msg, FEED_THREAD_POLL_MS)) {
            if (packet_from_message(msg, pkt)) {
                ProtoRfidTag body{};
                if (Protocol::parse_rfid_tag(pkt, body)) {
                    handle_rfid_tag(msg.src_id, body);
                }
            }
            continue;
        }
        if (g_link.recv(PROTO_MSG_FEED_DONE, msg, 0)) {
            if (packet_from_message(msg, pkt)) {
                ProtoFeedDone body{};
                if (Protocol::parse_feed_done(pkt, body)) {
                    handle_feed_done(msg.src_id, body);
                }
            }
            continue;
        }
        if (g_link.recv(PROTO_MSG_WEIGHT, msg, 0)) {
            if (packet_from_message(msg, pkt)) {
                ProtoWeight body{};
                if (Protocol::parse_weight(pkt, body)) {
                    JsonValue uplink;
                    uplink["animal_id"] = static_cast<unsigned int>(body.animal_id);
                    uplink["weight_g"] = static_cast<unsigned int>(body.weight_g);
                    g_mqtt->publish(MQTT_TOPIC_DIET_UDATA, uplink.dump());
                }
            }
        }
    }
}

static void status_thread()
{
    while (!g_stop) {
        NrfMessage msg;
        if (g_link.recv(PROTO_MSG_STATUS, msg, STATUS_POLL_MS)) {
            std::lock_guard<std::mutex> lock(g_wa_mtx);
            g_wa_last_rx[msg.src_id] = std::time(nullptr);
            if (g_wa_conn_status[msg.src_id] != 0) {
                g_wa_conn_status[msg.src_id] = 0;
                JsonValue uplink;
                uplink["actuator_status"][std::to_string(msg.src_id)] = "online";
                g_mqtt->publish(MQTT_TOPIC_DIET_UASYNC, uplink.dump());
                std::cout << "[status] actuator " << static_cast<int>(msg.src_id) << " online" << std::endl;
            }
            send_ack(msg.src_id, PROTO_MSG_STATUS);
        }

        const std::time_t now = std::time(nullptr);
        std::lock_guard<std::mutex> lock(g_wa_mtx);
        for (auto& wa : g_wa_last_rx) {
            if (now > wa.second + WA_CONN_TIMEOUT_SEC && g_wa_conn_status[wa.first] == 0) {
                g_wa_conn_status[wa.first] = -1;
                JsonValue uplink;
                uplink["actuator_status"][std::to_string(wa.first)] = "offline";
                g_mqtt->publish(MQTT_TOPIC_DIET_UASYNC, uplink.dump());
                std::cout << "[status] actuator " << static_cast<int>(wa.first) << " offline" << std::endl;
            }
        }
    }
}

static bool is_daily_reset_window()
{
    std::time_t now = std::time(nullptr);
    std::tm local_tm{};
    localtime_r(&now, &local_tm);
    return local_tm.tm_hour == DAILY_RESET_HOUR && local_tm.tm_min >= DAILY_RESET_MIN;
}

static void daily_reset_thread()
{
    bool reset_done_today = false;
    while (!g_stop) {
        if (is_daily_reset_window()) {
            if (!reset_done_today) {
                std::cout << "[daily] running daily reset" << std::endl;
                JsonValue refresh;
                refresh["dailyrefresh"] = JsonValue();
                const auto diets = g_db->get_dietlist();
                for (uint32_t diet_id : diets) {
                    const auto animals = g_db->get_animallist(diet_id);
                    for (uint32_t animal_id : animals) {
                        try {
                            JsonValue json_anim = g_db->get_animal_json(animal_id);
                            DietEngine diet(json_anim["diet"]);
                            std::lock_guard<std::mutex> guard(g_daily_mtx);
                            diet.daily_reset();
                            json_anim["diet"] = diet.to_json();
                            g_db->update_animal(json_anim);
                            refresh["dailyrefresh"].append(static_cast<unsigned int>(animal_id));
                        } catch (const std::exception& ex) {
                            std::cerr << "[daily] reset failed for " << animal_id << ": " << ex.what() << std::endl;
                        }
                    }
                }
                g_mqtt->publish(MQTT_TOPIC_DIET_UASYNC, refresh.dump());
                reset_done_today = true;
            }
        } else {
            reset_done_today = false;
        }
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    const std::string db_path = data_path();
    Database db(db_path);
    if (!db.ensure_seed()) {
        std::cerr << "Warning: could not seed database at " << db_path << std::endl;
    }
    g_db = &db;

    MqttClient mqtt(STATION_SN, MQTT_BROKER_DEFAULT, MQTT_BROKER_PORT_DEFAULT);
    g_mqtt = &mqtt;
    mqtt.connect_client();
    mqtt.subscribe(MQTT_TOPIC_DIET_DDATA);
    mqtt.subscribe(MQTT_TOPIC_DIET_DASYNC);

    if (g_link.init() != ENOERR) {
        std::cerr << "NRF init failed; continuing in stub mode" << std::endl;
    }
    g_link.start();

    std::thread feed_worker(feed_thread);
    std::thread status_worker(status_thread);
    std::thread daily_worker(daily_reset_thread);

    std::cout << "WiFeeder v2 host started (SN=" << STATION_SN << ", db=" << db_path << ")" << std::endl;

    while (!g_stop) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    g_link.stop();
    if (feed_worker.joinable()) feed_worker.join();
    if (status_worker.joinable()) status_worker.join();
    if (daily_worker.joinable()) daily_worker.join();
    mqtt.disconnect_client();

    std::cout << "WiFeeder v2 host stopped" << std::endl;
    return 0;
}
