#ifndef WIFEEDER_DATABASE_H
#define WIFEEDER_DATABASE_H

#include "diet_engine.h"
#include "error.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace wifeeder {

class Database {
public:
    explicit Database(std::string data_file);
    ~Database() = default;

    int32_t add_animal(const JsonValue& animal);
    int32_t update_animal(const JsonValue& animal);
    int32_t update_animal(const AnimalRecord& animal);
    int32_t rm_animal(uint32_t animal_id);

    JsonValue get_animal_json(uint32_t animal_id);
    int32_t get_animal(uint32_t animal_id, AnimalRecord& out);

    int32_t add_diet(const JsonValue& diet);
    int32_t update_diet(const JsonValue& diet);
    int32_t rm_diet(uint32_t diet_id);
    JsonValue get_diet_json(uint32_t diet_id);

    std::vector<uint32_t> get_dietlist();
    std::vector<uint32_t> get_animallist(uint32_t diet_id);

    bool ensure_seed();

private:
    std::string data_path_;
    std::mutex mtx_;

    JsonValue load_root_locked();
    int32_t save_root_locked(const JsonValue& root);
    JsonValue merge_animal_with_diet(const JsonValue& animal, const JsonValue& root);
};

} /* namespace wifeeder */

#endif /* WIFEEDER_DATABASE_H */
