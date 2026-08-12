#ifndef WIFEEDER_CONFIG_H
#define WIFEEDER_CONFIG_H

#include <cstdint>
#include <string>

namespace wifeeder {

constexpr const char* STATION_SN = "WCN-A100-0001X";
constexpr uint32_t STATION_ID = 0x00000001U;

constexpr const char* DEFAULT_DATA_PATH = "./wifeeder-data.json";
constexpr const char* SYSTEM_DATA_PATH = "/etc/wifeeder-data.json";
constexpr const char* DEFAULT_CONFIG_PATH = "./config/config.json";
constexpr const char* DEFAULT_SEED_DATA_PATH = "./config/wifeeder-data.json";

constexpr const char* MQTT_BROKER_DEFAULT = "localhost";
constexpr int MQTT_BROKER_PORT_DEFAULT = 1883;

constexpr int WA_CONN_TIMEOUT_SEC = 5 * 60;
constexpr int STATUS_POLL_MS = 1000;
constexpr int FEED_THREAD_POLL_MS = 50;
constexpr int DAILY_RESET_HOUR = 23;
constexpr int DAILY_RESET_MIN = 50;

constexpr uint32_t DEF_CALIBER_REF_WEIGHT = 960U;
constexpr uint32_t DEF_CALIBER_REF_REVS = 16U;
constexpr uint32_t DEF_DIET_ID_NULL = 0U;
constexpr uint32_t DEF_DIET_ID_CALIBER = 1U;
constexpr uint32_t DEF_FODDER_TYPES = 2U;

constexpr uint32_t RFID_CODINTEK_MAJOR_0 = 1994U;
constexpr uint32_t RFID_CODINTEK_MAJOR_1 = 6232U;
constexpr uint32_t RFID_CODINTEK_MAJOR_2 = 6227U;
constexpr uint32_t RFID_CODINTEK_MAJOR_3 = 1228U;

constexpr uint8_t PROTO_ID_CONTROLLER = 0x00U;

constexpr const char* NRF_SPI_DEVICE = "/dev/spidev0.0";
constexpr int NRF_CE_GPIO = 25;
constexpr uint8_t NRF_RF_CHANNEL = 76U;
constexpr uint8_t NRF_PAYLOAD_SIZE = 32U;

constexpr const char* MQTT_TOPIC_DIET_UDATA = "WCN-A100-0001X/diet/ulink/data";
constexpr const char* MQTT_TOPIC_DIET_DDATA = "WCN-A100-0001X/diet/dlink/data";
constexpr const char* MQTT_TOPIC_DIET_UASYNC = "WCN-A100-0001X/diet/ulink/async";
constexpr const char* MQTT_TOPIC_DIET_DASYNC = "WCN-A100-0001X/diet/dlink/async";

inline bool rfid_is_valid(uint32_t rfid)
{
    const uint32_t major = rfid / 10000U;
    return major == RFID_CODINTEK_MAJOR_0 ||
           major == RFID_CODINTEK_MAJOR_1 ||
           major == RFID_CODINTEK_MAJOR_2 ||
           major == RFID_CODINTEK_MAJOR_3;
}

} /* namespace wifeeder */

#endif /* WIFEEDER_CONFIG_H */
