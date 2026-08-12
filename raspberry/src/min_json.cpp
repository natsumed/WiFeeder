#include "min_json.h"

#include <cctype>
#include <sstream>
#include <stdexcept>

namespace wifeeder {

static void skip_ws(const std::string& s, size_t& i)
{
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) {
        ++i;
    }
}

static JsonValue parse_value(const std::string& s, size_t& i);

static std::string parse_string(const std::string& s, size_t& i)
{
    if (s[i] != '"') {
        throw std::runtime_error("expected string");
    }
    ++i;
    std::string out;
    while (i < s.size() && s[i] != '"') {
        if (s[i] == '\\' && i + 1 < s.size()) {
            ++i;
        }
        out.push_back(s[i++]);
    }
    if (i >= s.size() || s[i] != '"') {
        throw std::runtime_error("unterminated string");
    }
    ++i;
    return out;
}

static JsonValue parse_number(const std::string& s, size_t& i)
{
    const size_t start = i;
    if (s[i] == '-') {
        ++i;
    }
    while (i < s.size() && (std::isdigit(static_cast<unsigned char>(s[i])) || s[i] == '.')) {
        ++i;
    }
    const std::string num = s.substr(start, i - start);
    if (num.find('.') != std::string::npos) {
        return JsonValue(std::stod(num));
    }
    if (num[0] == '-') {
        return JsonValue(static_cast<int>(std::stoll(num)));
    }
    return JsonValue(static_cast<unsigned int>(std::stoul(num)));
}

static JsonValue parse_array(const std::string& s, size_t& i)
{
    JsonValue result;
    ++i;
    skip_ws(s, i);
    if (i < s.size() && s[i] == ']') {
        ++i;
        JsonValue empty_arr;
        empty_arr.append(JsonValue());
        empty_arr = JsonValue();
        result = JsonValue();
        return result;
    }
    while (i < s.size()) {
        skip_ws(s, i);
        if (s[i] == ']') {
            ++i;
            break;
        }
        result.append(parse_value(s, i));
        skip_ws(s, i);
        if (i < s.size() && s[i] == ',') {
            ++i;
            continue;
        }
        if (i < s.size() && s[i] == ']') {
            ++i;
            break;
        }
        break;
    }
    return result;
}

static JsonValue parse_object(const std::string& s, size_t& i)
{
    JsonValue obj;
    ++i;
    skip_ws(s, i);
    while (i < s.size() && s[i] != '}') {
        skip_ws(s, i);
        const std::string key = parse_string(s, i);
        skip_ws(s, i);
        if (s[i] != ':') {
            throw std::runtime_error("expected colon");
        }
        ++i;
        skip_ws(s, i);
        obj[key] = parse_value(s, i);
        skip_ws(s, i);
        if (s[i] == ',') {
            ++i;
            continue;
        }
        if (s[i] == '}') {
            break;
        }
    }
    if (i >= s.size() || s[i] != '}') {
        throw std::runtime_error("unterminated object");
    }
    ++i;
    return obj;
}

static JsonValue parse_value(const std::string& s, size_t& i)
{
    skip_ws(s, i);
    if (i >= s.size()) {
        return JsonValue();
    }
    const char c = s[i];
    if (c == '"') {
        return JsonValue(parse_string(s, i));
    }
    if (c == '{') {
        return parse_object(s, i);
    }
    if (c == '[') {
        return parse_array(s, i);
    }
    if (c == 't' && s.compare(i, 4, "true") == 0) {
        i += 4;
        return JsonValue(true);
    }
    if (c == 'f' && s.compare(i, 5, "false") == 0) {
        i += 5;
        return JsonValue(false);
    }
    if (c == 'n' && s.compare(i, 4, "null") == 0) {
        i += 4;
        return JsonValue();
    }
    return parse_number(s, i);
}

JsonValue::JsonValue(bool v) : type_(JsonType::Bool), bool_val_(v) {}
JsonValue::JsonValue(int v) : type_(JsonType::Int), int_val_(v) {}
JsonValue::JsonValue(unsigned int v) : type_(JsonType::UInt), int_val_(static_cast<int64_t>(v)) {}
JsonValue::JsonValue(int64_t v) : type_(JsonType::Int), int_val_(v) {}
JsonValue::JsonValue(uint64_t v) : type_(JsonType::UInt), int_val_(static_cast<int64_t>(v)) {}
JsonValue::JsonValue(double v) : type_(JsonType::Double), double_val_(v) {}
JsonValue::JsonValue(const char* v) : type_(JsonType::String), str_val_(v ? v : "") {}
JsonValue::JsonValue(const std::string& v) : type_(JsonType::String), str_val_(v) {}

bool JsonValue::isMember(const std::string& key) const
{
    return type_ == JsonType::Object && object_val_.count(key) > 0;
}

bool JsonValue::asBool() const { return bool_val_; }
int JsonValue::asInt() const { return static_cast<int>(int_val_); }
unsigned int JsonValue::asUInt() const { return static_cast<unsigned int>(int_val_); }
uint64_t JsonValue::asUInt64() const { return static_cast<uint64_t>(int_val_); }
double JsonValue::asDouble() const
{
    if (type_ == JsonType::Double) return double_val_;
    if (type_ == JsonType::Int || type_ == JsonType::UInt) return static_cast<double>(int_val_);
    return 0.0;
}
std::string JsonValue::asString() const { return str_val_; }

JsonValue JsonValue::get(const std::string& key, const JsonValue& fallback) const
{
    if (type_ != JsonType::Object) return fallback;
    auto it = object_val_.find(key);
    return it == object_val_.end() ? fallback : it->second;
}

JsonValue& JsonValue::operator[](const std::string& key)
{
    if (type_ != JsonType::Object) {
        type_ = JsonType::Object;
        object_val_.clear();
    }
    return object_val_[key];
}

JsonValue& JsonValue::operator[](size_t index)
{
    if (type_ != JsonType::Array) {
        type_ = JsonType::Array;
        array_val_.clear();
    }
    if (index >= array_val_.size()) {
        array_val_.resize(index + 1);
    }
    return array_val_[index];
}

const JsonValue& JsonValue::operator[](const std::string& key) const
{
    static JsonValue null_val;
    if (type_ != JsonType::Object) return null_val;
    auto it = object_val_.find(key);
    return it == object_val_.end() ? null_val : it->second;
}

const JsonValue& JsonValue::operator[](size_t index) const
{
    static JsonValue null_val;
    if (type_ != JsonType::Array || index >= array_val_.size()) return null_val;
    return array_val_[index];
}

void JsonValue::append(const JsonValue& v)
{
    if (type_ != JsonType::Array) {
        type_ = JsonType::Array;
        array_val_.clear();
    }
    array_val_.push_back(v);
}

size_t JsonValue::size() const
{
    if (type_ == JsonType::Array) return array_val_.size();
    if (type_ == JsonType::Object) return object_val_.size();
    return 0;
}

JsonValue JsonValue::parse(const std::string& text, bool& ok)
{
    ok = false;
    try {
        size_t i = 0;
        JsonValue v = parse_value(text, i);
        skip_ws(text, i);
        if (i != text.size()) {
            return JsonValue();
        }
        ok = true;
        return v;
    } catch (...) {
        return JsonValue();
    }
}

static std::string escape(const std::string& s)
{
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else out += c;
    }
    out += '"';
    return out;
}

std::string JsonValue::dump() const
{
    switch (type_) {
    case JsonType::Null: return "null";
    case JsonType::Bool: return bool_val_ ? "true" : "false";
    case JsonType::Int: return std::to_string(int_val_);
    case JsonType::UInt: return std::to_string(static_cast<uint64_t>(int_val_));
    case JsonType::Double: {
        std::ostringstream oss;
        oss << double_val_;
        return oss.str();
    }
    case JsonType::String: return escape(str_val_);
    case JsonType::Array: {
        std::string out = "[";
        for (size_t i = 0; i < array_val_.size(); ++i) {
            if (i > 0) out += ",";
            out += array_val_[i].dump();
        }
        out += "]";
        return out;
    }
    case JsonType::Object: {
        std::string out = "{";
        bool first = true;
        for (const auto& kv : object_val_) {
            if (!first) out += ",";
            first = false;
            out += escape(kv.first) + ":" + kv.second.dump();
        }
        out += "}";
        return out;
    }
    }
    return "null";
}

std::string JsonValue::toStyledString() const
{
    return dump();
}

std::vector<std::string> JsonValue::getMemberNames() const
{
    std::vector<std::string> names;
    if (type_ != JsonType::Object) {
        return names;
    }
    for (const auto& kv : object_val_) {
        names.push_back(kv.first);
    }
    return names;
}

JsonValue::const_iterator JsonValue::begin() const
{
    return object_val_.begin();
}

JsonValue::const_iterator JsonValue::end() const
{
    return object_val_.end();
}

} /* namespace wifeeder */
