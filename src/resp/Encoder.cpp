#include "Encoder.h"

std::string Encoder::encode(const RespValue& v) {
    switch (v.type) {
        case RespType::SimpleString:
            return "+" + v.str + "\r\n";
        case RespType::Error:
            return "-" + v.str + "\r\n";
        case RespType::Integer:
            return ":" + std::to_string(v.intValue) + "\r\n";
        case RespType::BulkString:
            if (v.isNull) return "$-1\r\n";
            return "$" + std::to_string(v.str.size()) + "\r\n" + v.str + "\r\n";
        case RespType::Array: {
            std::string out;
            if (v.isNull) return "*-1\r\n";
            out = "*" + std::to_string(v.items.size()) + "\r\n";
            for (const auto& e : v.items) out += encode(e);
            return out;
        }
    }
    return "";
}