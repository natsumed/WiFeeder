#include "mqtt_client.h"

#include <iostream>

namespace wifeeder {

#ifdef HAVE_MOSQUITTO
#include <mosquittopp.h>

struct MqttClient::MosquittoImpl : public mosqpp::mosquittopp {
    MqttClient* owner = nullptr;

    explicit MosquittoImpl(MqttClient* parent, const std::string& id)
        : mosqpp::mosquittopp(id.c_str()), owner(parent)
    {
    }

    void on_connect(int rc) override
    {
        if (owner != nullptr) {
            owner->connected_ = (rc == 0);
        }
    }

    void on_message(const struct mosquitto_message* message) override
    {
        if (owner == nullptr || message == nullptr || message->topic == nullptr || message->payload == nullptr) {
            return;
        }
        std::lock_guard<std::mutex> lock(owner->topics_mtx_);
        owner->topics_[message->topic].emplace(
            std::string(static_cast<const char*>(message->payload), message->payloadlen));
    }
};
#endif

MqttClient::MqttClient(std::string id, std::string broker, int port)
    : client_id_(std::move(id)), broker_(std::move(broker)), port_(port)
{
#ifdef HAVE_MOSQUITTO
    mosqpp::lib_init();
    impl_ = new MosquittoImpl(this, client_id_);
#endif
}

MqttClient::~MqttClient()
{
#ifdef HAVE_MOSQUITTO
    disconnect_client();
    delete impl_;
    impl_ = nullptr;
    mosqpp::lib_cleanup();
#endif
}

int32_t MqttClient::connect_client()
{
#ifdef HAVE_MOSQUITTO
    if (impl_ == nullptr) {
        return ENULLPTR;
    }
    const int rc = impl_->connect(broker_.c_str(), port_, 60);
    if (rc == MOSQ_ERR_SUCCESS) {
        impl_->loop_start();
        connected_ = true;
        return ENOERR;
    }
    std::cerr << "[mqtt] connect failed rc=" << rc << std::endl;
    return ENETWORK;
#else
    std::cout << "[mqtt] stub connect to " << broker_ << ":" << port_ << std::endl;
    connected_ = true;
    return ENOERR;
#endif
}

void MqttClient::disconnect_client()
{
#ifdef HAVE_MOSQUITTO
    if (impl_ != nullptr && connected_) {
        impl_->loop_stop(true);
        impl_->disconnect();
    }
#endif
    connected_ = false;
}

int32_t MqttClient::subscribe(const std::string& topic)
{
    if (!connected_) {
        return EBADRES;
    }
#ifdef HAVE_MOSQUITTO
    if (impl_ == nullptr || impl_->subscribe(nullptr, topic.c_str(), 0) != MOSQ_ERR_SUCCESS) {
        return ENETWORK;
    }
#else
    std::cout << "[mqtt] stub subscribe " << topic << std::endl;
#endif
    return ENOERR;
}

void MqttClient::publish(const std::string& topic, const std::string& data)
{
#ifdef HAVE_MOSQUITTO
    if (impl_ != nullptr && connected_) {
        impl_->publish(nullptr, topic.c_str(), static_cast<int>(data.size()), data.c_str(), 0, false);
    }
#else
    std::cout << "[mqtt] stub publish " << topic << " => " << data << std::endl;
#endif
}

std::string MqttClient::get_msg(const std::string& topic)
{
    std::lock_guard<std::mutex> lock(topics_mtx_);
    auto it = topics_.find(topic);
    if (it == topics_.end() || it->second.empty()) {
        return {};
    }
    std::string msg = it->second.front();
    it->second.pop();
    return msg;
}

} /* namespace wifeeder */
