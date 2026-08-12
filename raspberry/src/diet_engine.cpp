#include "diet_engine.h"

#include <algorithm>
#include <ctime>
#include <iostream>
#include <sstream>

namespace wifeeder {

#define FEEDING_ALGO_LINEAR_FORCED 1
#define FEEDING_ALGO FEEDING_ALGO_LINEAR_FORCED
#define EXTENSION_TIME_ENABLED 1
#define EXTENSION_TIME EXTENSION_TIME_ENABLED

std::time_t daytime_to_ts(const std::string& daytime)
{
    std::time_t now = std::time(nullptr);
    std::tm local_tm{};
#if defined(_WIN32)
    localtime_s(&local_tm, &now);
#else
    localtime_r(&now, &local_tm);
#endif
    int hour = 0, min = 0, sec = 0;
    if (std::sscanf(daytime.c_str(), "%d:%d:%d", &hour, &min, &sec) != 3) {
        return 0;
    }
    local_tm.tm_hour = hour;
    local_tm.tm_min = min;
    local_tm.tm_sec = sec;
    return std::mktime(&local_tm);
}

std::string ts_to_daytime(std::time_t ts)
{
    std::tm local_tm{};
#if defined(_WIN32)
    localtime_s(&local_tm, &ts);
#else
    localtime_r(&ts, &local_tm);
#endif
    char buf[16];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", &local_tm);
    return std::string(buf);
}

uint32_t grams_to_revs(uint32_t grams, uint32_t density)
{
    if (density == 0) {
        return 0;
    }
    const uint32_t grams_per_rev = density / DEF_CALIBER_REF_REVS;
    if (grams_per_rev == 0) {
        return 0;
    }
    return grams / grams_per_rev;
}

uint32_t revs_to_grams(uint32_t revs, uint32_t density)
{
    return revs * (density / DEF_CALIBER_REF_REVS);
}

DietEngine::DietEngine()
{
    target_.resize(2);
    cons_.resize(2);
}

DietEngine::DietEngine(uint32_t diet_id) : diet_id_(diet_id)
{
    target_.resize(2);
    cons_.resize(2);
}

DietEngine::DietEngine(const DietData& data)
    : diet_id_(data.diet_id), target_(data.target), cons_(data.cons)
{
}

DietEngine::DietEngine(const JsonValue& data)
{
    if (!load_from_json(data, *this)) {
        diet_id_ = 0;
        target_.resize(1);
        cons_.resize(1);
    }
}

bool DietEngine::load_from_json(const JsonValue& data, DietEngine& out)
{
    DietEngine tmp;
    if (!tmp.is_valid(data)) {
        return false;
    }
    tmp.diet_id_ = data.get("diet_id", JsonValue(static_cast<unsigned int>(0))).asUInt();
    tmp.target_set(data["target"]);
    tmp.cons_set(data["cons"]);
    while (tmp.cons_.size() < tmp.target_.size()) {
        tmp.cons_.push_back(DietCons{});
    }
    while (tmp.target_.size() < tmp.cons_.size()) {
        tmp.target_.push_back(DietTarget{});
    }
    out = std::move(tmp);
    return true;
}

JsonValue DietEngine::to_json() const
{
    JsonValue data;
    data["diet_id"] = static_cast<unsigned int>(diet_id_);
    data["target"] = target_json()["target"];
    data["cons"] = cons_json()["cons"];
    return data;
}

DietData DietEngine::to_struct() const
{
    DietData data{};
    data.diet_id = diet_id_;
    data.target = target_;
    data.cons = cons_;
    return data;
}

int32_t DietEngine::refresh_portion()
{
    const std::time_t now_ts = std::time(nullptr);
    std::vector<int32_t> elapsed_prts;
    std::vector<std::time_t> start_ts;
    std::vector<std::time_t> end_ts;
    std::vector<uint32_t> feed_dayleft;
    std::vector<uint32_t> feed_pmax;

    for (size_t idx = 0; idx < cons_.size() && idx < target_.size(); ++idx) {
        feed_dayleft.push_back((target_[idx].d_qty + cons_[idx].missed) -
            std::min(target_[idx].d_qty + cons_[idx].missed, cons_[idx].d_eff));
        feed_pmax.push_back(std::min(target_[idx].d_max - cons_[idx].d_eff, target_[idx].p_max));

        if (now_ts >= cons_[idx].ts) {
            elapsed_prts.push_back(static_cast<int32_t>(
                (now_ts - cons_[idx].ts) / std::max<std::time_t>(target_[idx].inter, 1)));
        } else {
            elapsed_prts.push_back(0);
        }
        start_ts.push_back(daytime_to_ts(target_[idx].start));
        end_ts.push_back(daytime_to_ts(target_[idx].end));
    }
    cons_.resize(feed_dayleft.size());

    for (size_t idx = 0; idx < start_ts.size(); ++idx) {
        if (now_ts > start_ts[idx] && now_ts < end_ts[idx]) {
            if (elapsed_prts[idx] > 0) {
                cons_[idx].p_eff = 0;
                const uint32_t left_prts = static_cast<uint32_t>(
                    (end_ts[idx] - now_ts) / std::max<std::time_t>(target_[idx].inter, 1));

#if (FEEDING_ALGO == FEEDING_ALGO_LINEAR_FORCED)
                if (left_prts > 0) {
                    const double daytime_fraction =
                        static_cast<double>(now_ts - start_ts[idx]) /
                        static_cast<double>(end_ts[idx] - start_ts[idx]);
                    const double dayweight_final =
                        static_cast<double>(target_[idx].d_qty + cons_[idx].missed);
                    const double nominal_prt = dayweight_final /
                        (static_cast<double>(end_ts[idx] - start_ts[idx]) /
                         static_cast<double>(target_[idx].inter));
                    const double dayweight_fraction =
                        dayweight_final * daytime_fraction +
                        nominal_prt * (1.0 - daytime_fraction);

                    if (dayweight_fraction < static_cast<double>(cons_[idx].d_eff)) {
                        cons_[idx].p_qty = 0;
                    } else {
                        cons_[idx].p_qty = static_cast<uint32_t>(std::min(
                            dayweight_fraction - static_cast<double>(cons_[idx].d_eff),
                            static_cast<double>(target_[idx].p_max)));
                    }
                } else {
                    cons_[idx].p_qty = std::min(feed_dayleft[idx], feed_pmax[idx]);
                }
#endif
            }
#if (EXTENSION_TIME == EXTENSION_TIME_ENABLED)
        } else if (now_ts > end_ts[idx] &&
                   now_ts < daytime_to_ts("23:59:59")) {
            if (elapsed_prts[idx] > 0) {
                cons_[idx].p_eff = 0;
                cons_[idx].p_qty = std::min(feed_dayleft[idx], feed_pmax[idx]);
            }
#endif
        } else {
            cons_[idx].p_qty = 0;
            cons_[idx].p_eff = 0;
        }
    }
    return ENOERR;
}

int32_t DietEngine::get_portion(std::vector<uint32_t>& p_qty)
{
    p_qty.clear();
    refresh_portion();

    for (size_t idx = 0; idx < target_.size(); ++idx) {
        uint32_t qty = 0;
        if (target_[idx].d_max < cons_[idx].d_eff ||
            target_[idx].p_max < cons_[idx].p_eff ||
            cons_[idx].p_qty < cons_[idx].p_eff) {
            qty = 0;
        } else {
            qty = std::min(target_[idx].d_max - cons_[idx].d_eff,
                std::min(cons_[idx].p_qty - cons_[idx].p_eff,
                    std::min(target_[idx].p_max - cons_[idx].p_eff, target_[idx].p_max)));
        }
        p_qty.push_back(qty);
    }
    return ENOERR;
}

int32_t DietEngine::log_portion(const std::vector<uint32_t>& qty)
{
    if (qty.size() != cons_.size()) {
        return EARG;
    }
    refresh_portion();
    const std::time_t now_ts = std::time(nullptr);
    for (size_t idx = 0; idx < cons_.size(); ++idx) {
        if (qty[idx] > 0) {
            if (now_ts >= cons_[idx].ts + target_[idx].inter) {
                cons_[idx].ts = now_ts;
            }
            cons_[idx].p_eff += qty[idx];
            cons_[idx].d_eff += qty[idx];
        }
    }
    return ENOERR;
}

int32_t DietEngine::daily_reset()
{
    for (size_t idx = 0; idx < cons_.size(); ++idx) {
#if (EXTENSION_TIME == EXTENSION_TIME_DISABLED)
        if (std::time(nullptr) >= daytime_to_ts(target_[idx].end))
#elif (EXTENSION_TIME == EXTENSION_TIME_ENABLED)
        if (std::time(nullptr) >= daytime_to_ts("23:50:00"))
#endif
        {
            if (cons_[idx].ts < daytime_to_ts("23:50:00")) {
                cons_[idx].ts = std::time(nullptr);
                cons_[idx].missed = 0;
                if (cons_[idx].d_eff < target_[idx].d_qty) {
                    cons_[idx].missed += std::min(target_[idx].d_qty - cons_[idx].d_eff,
                        target_[idx].d_max - target_[idx].d_qty);
                }
                cons_[idx].d_eff = 0;
                cons_[idx].p_eff = 0;
            }
        }
    }
    refresh_portion();
    return ENOERR;
}

DietStatus DietEngine::diet_status() const
{
    for (size_t idx = 0; idx < cons_.size() && idx < target_.size(); ++idx) {
        const std::time_t feedstart_ts = daytime_to_ts(target_[idx].start);
        const std::time_t feedend_ts = daytime_to_ts(target_[idx].end);
        const std::time_t now = std::time(nullptr);
        if (now > feedstart_ts && now < feedend_ts) {
            const double eaten_ratio =
                static_cast<double>(cons_[idx].d_eff) / static_cast<double>(target_[idx].d_qty);
            const double time_ratio =
                static_cast<double>(now - feedstart_ts) /
                static_cast<double>(feedend_ts - feedstart_ts);
            if (eaten_ratio < 0.4 && time_ratio > 0.6) {
                return DietStatus::Alert;
            }
            if (eaten_ratio < 0.2 && time_ratio > 0.5) {
                return DietStatus::Warning;
            }
        }
    }
    return DietStatus::Ok;
}

JsonValue DietEngine::cons_json() const
{
    JsonValue data;
    data["diet_id"] = static_cast<unsigned int>(diet_id_);
    JsonValue cons_arr;
    for (const auto& c : cons_) {
        JsonValue item;
        item["feed_id"] = static_cast<unsigned int>(c.feed_id);
        item["ts"] = static_cast<uint64_t>(c.ts);
        item["d_eff"] = static_cast<unsigned int>(c.d_eff);
        item["p_qty"] = static_cast<unsigned int>(c.p_qty);
        item["p_eff"] = static_cast<unsigned int>(c.p_eff);
        item["missed"] = static_cast<unsigned int>(c.missed);
        cons_arr.append(item);
    }
    data["cons"] = cons_arr;
    return data;
}

JsonValue DietEngine::target_json() const
{
    JsonValue data;
    data["diet_id"] = static_cast<unsigned int>(diet_id_);
    JsonValue target_arr;
    for (const auto& t : target_) {
        JsonValue item;
        item["feed_id"] = static_cast<unsigned int>(t.feed_id);
        item["start"] = t.start;
        item["end"] = t.end;
        item["inter"] = static_cast<uint64_t>(t.inter);
        item["d_max"] = static_cast<unsigned int>(t.d_max);
        item["d_qty"] = static_cast<unsigned int>(t.d_qty);
        item["p_max"] = static_cast<unsigned int>(t.p_max);
        item["density"] = static_cast<unsigned int>(t.density);
        target_arr.append(item);
    }
    data["target"] = target_arr;
    return data;
}

int32_t DietEngine::cons_set(const JsonValue& data)
{
    if (!is_valid_cons(data)) {
        return EARG;
    }
    cons_.clear();
    JsonValue arr = data;
    if (data.isObject()) {
        diet_id_ = data.get("diet_id", JsonValue(static_cast<unsigned int>(0))).asUInt();
        arr = data["cons"];
    }
    for (size_t idx = 0; idx < arr.size(); ++idx) {
        DietCons c{};
        c.feed_id = arr[idx].get("feed_id", JsonValue(static_cast<unsigned int>(0))).asUInt();
        c.ts = static_cast<std::time_t>(arr[idx].get("ts", JsonValue(static_cast<uint64_t>(1))).asUInt64());
        c.d_eff = arr[idx].get("d_eff", JsonValue(static_cast<unsigned int>(0))).asUInt();
        c.p_qty = arr[idx].get("p_qty", JsonValue(static_cast<unsigned int>(0))).asUInt();
        c.p_eff = arr[idx].get("p_eff", JsonValue(static_cast<unsigned int>(0))).asUInt();
        c.missed = arr[idx].get("missed", JsonValue(static_cast<unsigned int>(0))).asUInt();
        cons_.push_back(c);
    }
    return ENOERR;
}

int32_t DietEngine::target_set(const JsonValue& target)
{
    if (!is_valid_target(target)) {
        return EARG;
    }
    target_.clear();
    for (size_t idx = 0; idx < target.size(); ++idx) {
        DietTarget t{};
        t.feed_id = target[idx].get("feed_id", JsonValue(static_cast<unsigned int>(0))).asUInt();
        t.start = target[idx].get("start", JsonValue("00:01:00")).asString();
        t.end = target[idx].get("end", JsonValue("23:59:00")).asString();
        t.inter = static_cast<std::time_t>(target[idx].get("inter", JsonValue(static_cast<uint64_t>(1))).asUInt64());
        t.d_max = target[idx].get("d_max", JsonValue(static_cast<unsigned int>(0))).asUInt();
        t.d_qty = target[idx].get("d_qty", JsonValue(static_cast<unsigned int>(0))).asUInt();
        t.p_max = target[idx].get("p_max", JsonValue(static_cast<unsigned int>(0))).asUInt();
        t.density = target[idx].get("density", JsonValue(static_cast<unsigned int>(1))).asUInt();
        target_.push_back(t);
    }
    return ENOERR;
}

bool DietEngine::is_valid_cons(const JsonValue& cons) const
{
    if (cons.isObject()) {
        return cons.get("diet_id", JsonValue()).isUInt() &&
               cons["cons"].isArray() && cons["cons"].size() > 0;
    }
    return cons.isArray() && cons.size() > 0;
}

bool DietEngine::is_valid_target(const JsonValue& target) const
{
    if (target.isObject()) {
        return target.get("diet_id", JsonValue()).isUInt() &&
               target["target"].isArray() && target["target"].size() > 0;
    }
    return target.isArray() && target.size() > 0;
}

bool DietEngine::is_valid(const JsonValue& data) const
{
    if (!data.isObject()) {
        return false;
    }
    return is_valid_target(data["target"]) || is_valid_cons(data["cons"]);
}

} /* namespace wifeeder */
