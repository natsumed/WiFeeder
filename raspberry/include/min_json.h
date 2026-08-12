#ifndef WIFEEDER_MIN_JSON_H
#define WIFEEDER_MIN_JSON_H

#include <map>
#include <string>
#include <vector>

namespace wifeeder {

enum class JsonType { Null, Bool, Int, UInt, Double, String, Array, Object };

class JsonValue {
public:
    JsonValue() = default;
    JsonValue(bool v);
    JsonValue(int v);
    JsonValue(unsigned int v);
    JsonValue(int64_t v);
    JsonValue(uint64_t v);
    JsonValue(double v);
    JsonValue(const char* v);
    JsonValue(const std::string& v);

    JsonType type() const { return type_; }

    bool isNull() const { return type_ == JsonType::Null; }
    bool isObject() const { return type_ == JsonType::Object; }
    bool isArray() const { return type_ == JsonType::Array; }
    bool isString() const { return type_ == JsonType::String; }
    bool isUInt() const { return type_ == JsonType::UInt; }
    bool isInt() const { return type_ == JsonType::Int; }
    bool isDouble() const { return type_ == JsonType::Double; }
    bool isMember(const std::string& key) const;

    bool asBool() const;
    int asInt() const;
    unsigned int asUInt() const;
    uint64_t asUInt64() const;
    double asDouble() const;
    std::string asString() const;

    JsonValue get(const std::string& key, const JsonValue& fallback) const;
    JsonValue& operator[](const std::string& key);
    JsonValue& operator[](size_t index);
    const JsonValue& operator[](const std::string& key) const;
    const JsonValue& operator[](size_t index) const;

    void append(const JsonValue& v);
    size_t size() const;

    static JsonValue parse(const std::string& text, bool& ok);
    std::string dump() const;
    std::string toStyledString() const;

    std::vector<std::string> getMemberNames() const;
    using const_iterator = std::map<std::string, JsonValue>::const_iterator;
    const_iterator begin() const;
    const_iterator end() const;

private:
    JsonType type_ = JsonType::Null;
    bool bool_val_ = false;
    int64_t int_val_ = 0;
    double double_val_ = 0.0;
    std::string str_val_;
    std::vector<JsonValue> array_val_;
    std::map<std::string, JsonValue> object_val_;
};

} /* namespace wifeeder */

#endif /* WIFEEDER_MIN_JSON_H */
