#ifndef WIFEEDER_MQTT_CLIENT_H
#define WIFEEDER_MQTT_CLIENT_H

#include "error.h"

#include <map>
#include <mutex>
#include <queue>
#include <string>

namespace wifeeder {

struct MosquittoImpl;

class MqttClient {
public:
    MqttClient(std::string id, std::string broker, int port);
    ~MqttClient();

    int32_t connect_client();
    void disconnect_client();

    int32_t subscribe(const std::string& topic);
    void publish(const std::string& topic, const std::string& data);
    std::string get_msg(const std::string& topic);

    bool is_connected() const { return connected_; }

private:
    friend struct MosquittoImpl;

    bool connected_ = false;
    std::string client_id_;
    std::string broker_;
    int port_ = 1883;

    std::mutex topics_mtx_;
    std::map<std::string, std::queue<std::string>> topics_;

#ifdef HAVE_MOSQUITTO
    struct MosquittoImpl;
    MosquittoImpl* impl_ = nullptr;
#endif
};

} /* namespace wifeeder */

#endif /* WIFEEDER_MQTT_CLIENT_H */
