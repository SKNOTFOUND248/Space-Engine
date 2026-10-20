#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <sstream>
#include <stdexcept>
#include <iomanip>
#include <cctype>
#include <charconv>

namespace Orbital {

class JsonValue {
public:
    enum class Type { Null, Boolean, Number, String, Array, Object };

    using Array = std::vector<JsonValue>;
    using Object = std::unordered_map<std::string, JsonValue>;

private:
    Type m_type = Type::Null;
    std::variant<std::monostate, bool, double, std::string, Array, Object> m_data;

public:
    JsonValue() : m_type(Type::Null), m_data(std::monostate{}) {}
    JsonValue(std::nullptr_t) : m_type(Type::Null), m_data(std::monostate{}) {}
    JsonValue(bool b) : m_type(Type::Boolean), m_data(b) {}
    JsonValue(double d) : m_type(Type::Number), m_data(d) {}
    JsonValue(int i) : m_type(Type::Number), m_data(static_cast<double>(i)) {}
    JsonValue(int64_t i) : m_type(Type::Number), m_data(static_cast<double>(i)) {}
    JsonValue(uint64_t u) : m_type(Type::Number), m_data(static_cast<double>(u)) {}
    JsonValue(const char* s) : m_type(Type::String), m_data(std::string(s)) {}
    JsonValue(std::string s) : m_type(Type::String), m_data(std::move(s)) {}
    JsonValue(Array a) : m_type(Type::Array), m_data(std::move(a)) {}
    JsonValue(Object o) : m_type(Type::Object), m_data(std::move(o)) {}

    Type type() const { return m_type; }
    bool isNull() const { return m_type == Type::Null; }
    bool isBool() const { return m_type == Type::Boolean; }
    bool isNumber() const { return m_type == Type::Number; }
    bool isString() const { return m_type == Type::String; }
    bool isArray() const { return m_type == Type::Array; }
    bool isObject() const { return m_type == Type::Object; }

    bool asBool(bool def = false) const {
        if (isBool()) return std::get<bool>(m_data);
        return def;
    }

    double asDouble(double def = 0.0) const {
        if (isNumber()) return std::get<double>(m_data);
        return def;
    }

    int64_t asInt64(int64_t def = 0) const {
        if (isNumber()) return static_cast<int64_t>(std::get<double>(m_data));
        return def;
    }

    int asInt(int def = 0) const {
        if (isNumber()) return static_cast<int>(std::get<double>(m_data));
        return def;
    }

    const std::string& asString(const std::string& def = "") const {
        static const std::string s_empty;
        if (isString()) return std::get<std::string>(m_data);
        return def.empty() ? s_empty : def;
    }

    const Array& asArray() const {
        static const Array s_empty;
        if (isArray()) return std::get<Array>(m_data);
        return s_empty;
    }

    Array& asArray() {
        if (!isArray()) {
            m_type = Type::Array;
            m_data = Array{};
        }
        return std::get<Array>(m_data);
    }

    const Object& asObject() const {
        static const Object s_empty;
        if (isObject()) return std::get<Object>(m_data);
        return s_empty;
    }

    Object& asObject() {
        if (!isObject()) {
            m_type = Type::Object;
            m_data = Object{};
        }
        return std::get<Object>(m_data);
    }

    bool contains(const std::string& key) const {
        if (!isObject()) return false;
        const auto& obj = std::get<Object>(m_data);
        return obj.find(key) != obj.end();
    }

    const JsonValue& operator[](const std::string& key) const {
        static const JsonValue s_null;
        if (!isObject()) return s_null;
        const auto& obj = std::get<Object>(m_data);
        auto it = obj.find(key);
        if (it != obj.end()) return it->second;
        return s_null;
    }

    JsonValue& operator[](const std::string& key) {
        return asObject()[key];
    }

    const JsonValue& operator[](size_t index) const {
        static const JsonValue s_null;
        if (!isArray()) return s_null;
        const auto& arr = std::get<Array>(m_data);
        if (index < arr.size()) return arr[index];
        return s_null;
    }

    JsonValue& operator[](size_t index) {
        auto& arr = asArray();
        if (index >= arr.size()) arr.resize(index + 1);
        return arr[index];
    }

    size_t size() const {
        if (isArray()) return std::get<Array>(m_data).size();
        if (isObject()) return std::get<Object>(m_data).size();
        return 0;
    }

    // Parsing
    static JsonValue parse(const std::string& jsonStr, std::string* errorMsg = nullptr) {
        size_t pos = 0;
        skipWhitespace(jsonStr, pos);
        if (pos >= jsonStr.size()) {
            if (errorMsg) *errorMsg = "Empty JSON input";
            return JsonValue();
        }

        try {
            JsonValue val = parseValue(jsonStr, pos);
            skipWhitespace(jsonStr, pos);
            return val;
        } catch (const std::exception& e) {
            if (errorMsg) *errorMsg = e.what();
            return JsonValue();
        }
    }

    // Serialization
    std::string serialize(int indent = 2, int currentIndent = 0) const {
        std::string indStr(currentIndent, ' ');
        std::string nextIndStr(currentIndent + indent, ' ');

        switch (m_type) {
            case Type::Null:    return "null";
            case Type::Boolean: return asBool() ? "true" : "false";
            case Type::Number: {
                std::ostringstream ss;
                ss << std::setprecision(14) << asDouble();
                return ss.str();
            }
            case Type::String:  return escapeString(asString());
            case Type::Array: {
                const auto& arr = asArray();
                if (arr.empty()) return "[]";
                std::ostringstream ss;
                ss << "[\n";
                for (size_t i = 0; i < arr.size(); ++i) {
                    ss << nextIndStr << arr[i].serialize(indent, currentIndent + indent);
                    if (i + 1 < arr.size()) ss << ",";
                    ss << "\n";
                }
                ss << indStr << "]";
                return ss.str();
            }
            case Type::Object: {
                const auto& obj = asObject();
                if (obj.empty()) return "{}";
                std::ostringstream ss;
                ss << "{\n";
                size_t count = 0;
                for (const auto& [k, v] : obj) {
                    ss << nextIndStr << escapeString(k) << ": " << v.serialize(indent, currentIndent + indent);
                    if (++count < obj.size()) ss << ",";
                    ss << "\n";
                }
                ss << indStr << "}";
                return ss.str();
            }
        }
        return "null";
    }

private:
    static void skipWhitespace(const std::string& str, size_t& pos) {
        while (pos < str.size()) {
            char c = str[pos];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                pos++;
            } else if (c == '/' && pos + 1 < str.size() && str[pos + 1] == '/') {
                // Line comment
                pos += 2;
                while (pos < str.size() && str[pos] != '\n') pos++;
            } else {
                break;
            }
        }
    }

    static std::string escapeString(const std::string& s) {
        std::ostringstream ss;
        ss << '"';
        for (char c : s) {
            switch (c) {
                case '"':  ss << "\\\""; break;
                case '\\': ss << "\\\\"; break;
                case '\b': ss << "\\b";  break;
                case '\f': ss << "\\f";  break;
                case '\n': ss << "\\n";  break;
                case '\r': ss << "\\r";  break;
                case '\t': ss << "\\t";  break;
                default:   ss << c;       break;
            }
        }
        ss << '"';
        return ss.str();
    }

    static JsonValue parseValue(const std::string& str, size_t& pos) {
        skipWhitespace(str, pos);
        if (pos >= str.size()) throw std::runtime_error("Unexpected end of input");

        char c = str[pos];
        if (c == '{') return parseObject(str, pos);
        if (c == '[') return parseArray(str, pos);
        if (c == '"') return parseString(str, pos);
        if (c == 't' || c == 'f') return parseBool(str, pos);
        if (c == 'n') return parseNull(str, pos);
        if (c == '-' || (c >= '0' && c <= '9')) return parseNumber(str, pos);

        throw std::runtime_error(std::string("Unexpected character: ") + c);
    }

    static JsonValue parseObject(const std::string& str, size_t& pos) {
        pos++; // skip '{'
        Object obj;

        skipWhitespace(str, pos);
        if (pos < str.size() && str[pos] == '}') {
            pos++;
            return JsonValue(obj);
        }

        while (pos < str.size()) {
            skipWhitespace(str, pos);
            if (pos >= str.size() || str[pos] != '"') {
                throw std::runtime_error("Expected string key in object");
            }
            std::string key = parseString(str, pos).asString();

            skipWhitespace(str, pos);
            if (pos >= str.size() || str[pos] != ':') {
                throw std::runtime_error("Expected ':' after key in object");
            }
            pos++; // skip ':'

            JsonValue val = parseValue(str, pos);
            obj[key] = std::move(val);

            skipWhitespace(str, pos);
            if (pos < str.size() && str[pos] == ',') {
                pos++; // skip ','
            } else if (pos < str.size() && str[pos] == '}') {
                pos++; // end of object
                return JsonValue(obj);
            } else {
                throw std::runtime_error("Expected ',' or '}' in object");
            }
        }
        throw std::runtime_error("Unclosed object");
    }

    static JsonValue parseArray(const std::string& str, size_t& pos) {
        pos++; // skip '['
        Array arr;

        skipWhitespace(str, pos);
        if (pos < str.size() && str[pos] == ']') {
            pos++;
            return JsonValue(arr);
        }

        while (pos < str.size()) {
            JsonValue val = parseValue(str, pos);
            arr.push_back(std::move(val));

            skipWhitespace(str, pos);
            if (pos < str.size() && str[pos] == ',') {
                pos++;
            } else if (pos < str.size() && str[pos] == ']') {
                pos++;
                return JsonValue(arr);
            } else {
                throw std::runtime_error("Expected ',' or ']' in array");
            }
        }
        throw std::runtime_error("Unclosed array");
    }

    static JsonValue parseString(const std::string& str, size_t& pos) {
        pos++; // skip opening '"'
        std::string s;
        while (pos < str.size()) {
            char c = str[pos++];
            if (c == '"') return JsonValue(s);
            if (c == '\\') {
                if (pos >= str.size()) throw std::runtime_error("Incomplete escape sequence");
                char esc = str[pos++];
                switch (esc) {
                    case '"':  s += '"';  break;
                    case '\\': s += '\\'; break;
                    case '/':  s += '/';  break;
                    case 'b':  s += '\b'; break;
                    case 'f':  s += '\f'; break;
                    case 'n':  s += '\n'; break;
                    case 'r':  s += '\r'; break;
                    case 't':  s += '\t'; break;
                    default:   s += esc;  break;
                }
            } else {
                s += c;
            }
        }
        throw std::runtime_error("Unclosed string");
    }

    static JsonValue parseNumber(const std::string& str, size_t& pos) {
        size_t start = pos;
        if (str[pos] == '-') pos++;
        while (pos < str.size() && (std::isdigit(static_cast<unsigned char>(str[pos])) || 
               str[pos] == '.' || str[pos] == 'e' || str[pos] == 'E' || str[pos] == '+' || str[pos] == '-')) {
            // Check if '-' or '+' is only after e/E
            if ((str[pos] == '-' || str[pos] == '+') && (str[pos-1] != 'e' && str[pos-1] != 'E')) break;
            pos++;
        }
        std::string numStr = str.substr(start, pos - start);
        double val = 0.0;
        try {
            val = std::stod(numStr);
        } catch (...) {
            throw std::runtime_error("Invalid number: " + numStr);
        }
        return JsonValue(val);
    }

    static JsonValue parseBool(const std::string& str, size_t& pos) {
        if (str.compare(pos, 4, "true") == 0) {
            pos += 4;
            return JsonValue(true);
        }
        if (str.compare(pos, 5, "false") == 0) {
            pos += 5;
            return JsonValue(false);
        }
        throw std::runtime_error("Invalid boolean literal");
    }

    static JsonValue parseNull(const std::string& str, size_t& pos) {
        if (str.compare(pos, 4, "null") == 0) {
            pos += 4;
            return JsonValue(nullptr);
        }
        throw std::runtime_error("Invalid null literal");
    }
};

} // namespace Orbital
