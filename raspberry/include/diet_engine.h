#ifndef WIFEEDER_DIET_ENGINE_H
#define WIFEEDER_DIET_ENGINE_H

#include "config.h"
#include "error.h"

#include <ctime>
#include <string>
#include <vector>

#ifdef HAVE_JSONCPP
#include <json/json.h>
using JsonValue = Json::Value;
#else
#include "min_json.h"
using JsonValue = wifeeder::JsonValue;
#endif

namespace wifeeder {

enum class DietStatus : uint32_t {
    Ok = 0,
    Warning = 1,
    Alert = 2,
};

struct DietTarget {
    uint32_t feed_id = 0;
    std::string start = "00:01:00";
    std::string end = "23:59:00";
    std::time_t inter = 3600;
    uint32_t d_max = 0;
    uint32_t d_qty = 0;
    uint32_t p_max = 0;
    uint32_t density = 480;
};

struct DietCons {
    uint32_t feed_id = 0;
    std::time_t ts = 0;
    uint32_t d_eff = 0;
    uint32_t p_qty = 0;
    uint32_t p_eff = 0;
    uint32_t missed = 0;
};

struct DietData {
    uint32_t diet_id = 0;
    std::vector<DietTarget> target;
    std::vector<DietCons> cons;
};

struct AnimalRecord {
    uint32_t rfid = 0;
    std::time_t feedtime_lasteff = 0;
    double weight = 0.0;
    DietData diet;
};

std::time_t daytime_to_ts(const std::string& daytime);
std::string ts_to_daytime(std::time_t ts);

uint32_t grams_to_revs(uint32_t grams, uint32_t density);
uint32_t revs_to_grams(uint32_t revs, uint32_t density);

class DietEngine {
public:
    DietEngine();
    explicit DietEngine(uint32_t diet_id);
    explicit DietEngine(const DietData& data);
    explicit DietEngine(const JsonValue& data);

    JsonValue to_json() const;
    DietData to_struct() const;
    uint32_t diet_id() const { return diet_id_; }

    int32_t refresh_portion();
    int32_t get_portion(std::vector<uint32_t>& p_qty);
    int32_t log_portion(const std::vector<uint32_t>& qty);
    int32_t daily_reset();
    DietStatus diet_status() const;

    JsonValue cons_json() const;
    JsonValue target_json() const;
    int32_t cons_set(const JsonValue& data);
    int32_t target_set(const JsonValue& target);

    static bool load_from_json(const JsonValue& data, DietEngine& out);

private:
    uint32_t diet_id_ = 0;
    std::vector<DietTarget> target_;
    std::vector<DietCons> cons_;

    bool is_valid_cons(const JsonValue& cons) const;
    bool is_valid_target(const JsonValue& target) const;
    bool is_valid(const JsonValue& data) const;
};

} /* namespace wifeeder */

#endif /* WIFEEDER_DIET_ENGINE_H */
