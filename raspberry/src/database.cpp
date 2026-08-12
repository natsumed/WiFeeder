#include "database.h"

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace wifeeder {

#ifdef HAVE_JSONCPP
static JsonValue parse_file(const std::string& path, bool& ok)
{
    Json::CharReaderBuilder builder;
    Json::Value root;
    std::ifstream ifile(path);
    std::string errs;
    ok = Json::parseFromStream(builder, ifile, &root, &errs);
    return root;
}
#else
static JsonValue parse_file(const std::string& path, bool& ok)
{
    std::ifstream ifile(path);
    std::string text((std::istreambuf_iterator<char>(ifile)), std::istreambuf_iterator<char>());
    return JsonValue::parse(text, ok);
}
#endif

Database::Database(std::string data_file) : data_path_(std::move(data_file)) {}

JsonValue Database::load_root_locked()
{
    bool ok = false;
    JsonValue root = parse_file(data_path_, ok);
    if (!ok) {
        throw std::runtime_error("failed to parse database");
    }
    return root;
}

int32_t Database::save_root_locked(const JsonValue& root)
{
    std::ofstream ofile(data_path_, std::ios::out | std::ios::trunc);
    if (!ofile) {
        return EACCES;
    }
#ifdef HAVE_JSONCPP
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "\t";
    ofile << Json::writeString(builder, root);
#else
    ofile << root.dump();
#endif
    return ENOERR;
}

bool Database::ensure_seed()
{
    std::ifstream probe(data_path_);
    if (probe.good()) {
        return true;
    }
    static const char* seed_paths[] = {
        DEFAULT_SEED_DATA_PATH,
        "./config/wifeeder-data.json",
        "../config/wifeeder-data.json",
        "../../raspberry/config/wifeeder-data.json",
    };
    for (const char* seed_path : seed_paths) {
        std::ifstream seed(seed_path);
        if (!seed.good()) {
            continue;
        }
        std::ofstream out(data_path_, std::ios::out | std::ios::trunc);
        out << seed.rdbuf();
        return out.good();
    }
    return false;
}

JsonValue Database::merge_animal_with_diet(const JsonValue& animal, const JsonValue& root)
{
    JsonValue merged = animal;
    if (!merged.isMember("diet")) {
        return merged;
    }
    const uint32_t diet_id = merged["diet"].get("diet_id", JsonValue(static_cast<unsigned int>(0))).asUInt();
    const std::string diet_key = std::to_string(diet_id);
    if (root["diet"].isMember(diet_key)) {
        JsonValue template_diet = root["diet"][diet_key];
        if (!merged["diet"].isMember("target")) {
            merged["diet"]["target"] = template_diet["target"];
        }
        if (!merged["diet"].isMember("cons")) {
            merged["diet"]["cons"] = template_diet.get("cons", JsonValue());
        }
    }
    return merged;
}

int32_t Database::add_animal(const JsonValue& animal)
{
    std::lock_guard<std::mutex> guard(mtx_);
    try {
        JsonValue root = load_root_locked();
        const std::string animal_id = std::to_string(animal.get("rfid", JsonValue(static_cast<unsigned int>(0))).asUInt());
        JsonValue animal_root = root.get("animal", JsonValue());
        if (animal_root.isMember(animal_id)) {
            return EDUP;
        }
        animal_root[animal_id] = animal;
        root["animal"] = animal_root;
        return save_root_locked(root);
    } catch (...) {
        return EUNKNOWN;
    }
}

int32_t Database::update_animal(const JsonValue& animal)
{
    std::lock_guard<std::mutex> guard(mtx_);
    try {
        JsonValue root = load_root_locked();
        const std::string animal_id = std::to_string(animal.get("rfid", JsonValue(static_cast<unsigned int>(0))).asUInt());
        JsonValue animal_root = root.get("animal", JsonValue());
        if (!animal_root.isMember(animal_id)) {
            return ENOTFOUND;
        }
        animal_root[animal_id] = animal;
        root["animal"] = animal_root;
        return save_root_locked(root);
    } catch (...) {
        return EUNKNOWN;
    }
}

int32_t Database::update_animal(const AnimalRecord& animal)
{
    JsonValue json;
    json["rfid"] = static_cast<unsigned int>(animal.rfid);
    json["feedtime_lasteff"] = static_cast<uint64_t>(animal.feedtime_lasteff);
    json["weight"] = animal.weight;
    json["diet"] = DietEngine(animal.diet).to_json();
    return update_animal(json);
}

int32_t Database::rm_animal(uint32_t animal_id)
{
    std::lock_guard<std::mutex> guard(mtx_);
    try {
        JsonValue root = load_root_locked();
        JsonValue animal_root = root.get("animal", JsonValue());
        const std::string key = std::to_string(animal_id);
        if (!animal_root.isMember(key)) {
            return ENOTFOUND;
        }
        JsonValue rebuilt;
        for (const auto& member : animal_root.getMemberNames()) {
            if (member != key) {
                rebuilt[member] = animal_root[member];
            }
        }
        root["animal"] = rebuilt;
        return save_root_locked(root);
    } catch (...) {
        return EUNKNOWN;
    }
}

JsonValue Database::get_animal_json(uint32_t animal_id)
{
    std::lock_guard<std::mutex> guard(mtx_);
    JsonValue root = load_root_locked();
    const std::string key = std::to_string(animal_id);
    JsonValue animal_root = root.get("animal", JsonValue());
    if (!animal_root.isMember(key)) {
        throw std::runtime_error("animal not found");
    }
    return merge_animal_with_diet(animal_root[key], root);
}

int32_t Database::get_animal(uint32_t animal_id, AnimalRecord& out)
{
    try {
        JsonValue json = get_animal_json(animal_id);
        out.rfid = json.get("rfid", JsonValue(static_cast<unsigned int>(0))).asUInt();
        out.feedtime_lasteff = static_cast<std::time_t>(
            json.get("feedtime_lasteff", JsonValue(static_cast<uint64_t>(0))).asUInt64());
        out.weight = json.get("weight", JsonValue(0.0)).asDouble();
        DietEngine diet(json["diet"]);
        out.diet = diet.to_struct();
        return ENOERR;
    } catch (...) {
        return ENOTFOUND;
    }
}

int32_t Database::add_diet(const JsonValue& diet)
{
    std::lock_guard<std::mutex> guard(mtx_);
    try {
        JsonValue root = load_root_locked();
        const std::string diet_key = std::to_string(diet.get("diet_id", JsonValue(static_cast<unsigned int>(0))).asUInt());
        JsonValue diet_root = root.get("diet", JsonValue());
        if (diet_root.isMember(diet_key)) {
            return EDUP;
        }
        diet_root[diet_key] = diet;
        root["diet"] = diet_root;
        return save_root_locked(root);
    } catch (...) {
        return EUNKNOWN;
    }
}

int32_t Database::update_diet(const JsonValue& diet)
{
    std::lock_guard<std::mutex> guard(mtx_);
    try {
        JsonValue root = load_root_locked();
        const std::string diet_key = std::to_string(diet.get("diet_id", JsonValue(static_cast<unsigned int>(0))).asUInt());
        JsonValue diet_root = root.get("diet", JsonValue());
        if (!diet_root.isMember(diet_key)) {
            return ENOTFOUND;
        }
        diet_root[diet_key] = diet;
        root["diet"] = diet_root;
        return save_root_locked(root);
    } catch (...) {
        return EUNKNOWN;
    }
}

int32_t Database::rm_diet(uint32_t diet_id)
{
    std::lock_guard<std::mutex> guard(mtx_);
    try {
        JsonValue root = load_root_locked();
        JsonValue diet_root = root.get("diet", JsonValue());
        const std::string key = std::to_string(diet_id);
        if (!diet_root.isMember(key)) {
            return ENOTFOUND;
        }
        JsonValue rebuilt;
        for (const auto& member : diet_root.getMemberNames()) {
            if (member != key) {
                rebuilt[member] = diet_root[member];
            }
        }
        root["diet"] = rebuilt;
        return save_root_locked(root);
    } catch (...) {
        return EUNKNOWN;
    }
}

JsonValue Database::get_diet_json(uint32_t diet_id)
{
    std::lock_guard<std::mutex> guard(mtx_);
    JsonValue root = load_root_locked();
    const std::string key = std::to_string(diet_id);
    JsonValue diet_root = root.get("diet", JsonValue());
    if (!diet_root.isMember(key)) {
        JsonValue empty;
        empty["diet_id"] = static_cast<unsigned int>(diet_id);
        return empty;
    }
    return diet_root[key];
}

std::vector<uint32_t> Database::get_dietlist()
{
    std::lock_guard<std::mutex> guard(mtx_);
    std::vector<uint32_t> ids;
    JsonValue root = load_root_locked();
    JsonValue diet_root = root.get("diet", JsonValue());
    for (const auto& key : diet_root.getMemberNames()) {
        ids.push_back(static_cast<uint32_t>(std::stoul(key)));
    }
    return ids;
}

std::vector<uint32_t> Database::get_animallist(uint32_t diet_id)
{
    std::lock_guard<std::mutex> guard(mtx_);
    std::vector<uint32_t> ids;
    JsonValue root = load_root_locked();
    JsonValue animal_root = root.get("animal", JsonValue());
    for (const auto& key : animal_root.getMemberNames()) {
        JsonValue animal = animal_root[key];
        if (animal["diet"].get("diet_id", JsonValue(static_cast<unsigned int>(0))).asUInt() == diet_id) {
            ids.push_back(animal.get("rfid", JsonValue(static_cast<unsigned int>(0))).asUInt());
        }
    }
    return ids;
}

} /* namespace wifeeder */
