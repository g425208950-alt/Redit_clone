#include "Decoder.h"
#include <cctype>

static bool readLine(const std::string& buf, size_t start, size_t& lineEnd) {
    if (start >= buf.size()) return false;
    auto pos = buf.find("\r\n", start);
    if (pos == std::string::npos) return false;
    lineEnd = pos;
    return true;
}

std::optional<RespValue> Decoder::tryParse(const std::string& buf, size_t& consumed) {
    return parseAt(buf, 0, consumed);
}

std::optional<RespValue> Decoder::parseAt(const std::string& buf, size_t start, size_t& consumed) {
    size_t lineEnd;
    if (!readLine(buf, start, lineEnd)) {
        consumed = 0;
        return std::nullopt;
    }
    char t = buf[start];
    std::string payload = buf.substr(start + 1, lineEnd - (start + 1));
    size_t after = lineEnd + 2;

    switch (t) {
        case '+':
            consumed = after - start;
            return RespValue::simpleString(payload);
        case '-':
            consumed = after - start;
            return RespValue::error(payload);
        case ':': {
            try {
                long long n = std::stoll(payload);
                consumed = after - start;
                return RespValue::integer(n);
            } catch (...) {
                consumed = after - start;
                return RespValue::error("ERR value is not an integer or out of range");
            }
        }
        case '$': {
            long long len;
            try {
                len = std::stoll(payload);
            } catch (...) {
                consumed = after - start;
                return RespValue::error("ERR protocol error: invalid bulk length");
            }
            if (len < 0) {
                consumed = after - start;
                return RespValue::nullBulk();
            }
            size_t dataEnd = after + static_cast<size_t>(len);
            if (buf.size() < dataEnd + 2) {
                consumed = 0;
                return std::nullopt;
            }
            if (buf[dataEnd] != '\r' || buf[dataEnd + 1] != '\n') {
                consumed = dataEnd + 2 - start;
                return RespValue::error("ERR protocol error: bad bulk string terminator");
            }
            std::string data = buf.substr(after, static_cast<size_t>(len));
            consumed = dataEnd + 2 - start;
            return RespValue::bulkString(data);
        }
        case '*': {
            long long count;
            try {
                count = std::stoll(payload);
            } catch (...) {
                consumed = after - start;
                return RespValue::error("ERR protocol error: invalid multibulk length");
            }
            if (count < 0) {
                consumed = after - start;
                return RespValue::nullArray();
            }
            std::vector<RespValue> elems;
            elems.reserve(static_cast<size_t>(count));
            size_t cursor = after;
            for (long long i = 0; i < count; i++) {
                size_t subConsumed;
                auto sub = parseAt(buf, cursor, subConsumed);
                if (!sub) {
                    consumed = 0;
                    return std::nullopt;
                }
                elems.push_back(std::move(*sub));
                cursor += subConsumed;
            }
            consumed = cursor - start;
            return RespValue::array(std::move(elems));
        }
        default: {
            std::vector<RespValue> elems;
            size_t i = 0;
            std::string s = buf.substr(start, lineEnd - start);
            while (i < s.size()) {
                while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) i++;
                if (i >= s.size()) break;
                size_t j = i;
                while (j < s.size() && !std::isspace(static_cast<unsigned char>(s[j]))) j++;
                elems.push_back(RespValue::bulkString(s.substr(i, j - i)));
                i = j;
            }
            consumed = after - start;
            return RespValue::array(std::move(elems));
        }
    }
}