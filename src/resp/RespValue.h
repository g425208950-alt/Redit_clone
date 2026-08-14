#pragma once
#include <string>
#include <vector>
#include <cstdint>

enum class RespType {
    SimpleString,
    Error,
    Integer,
    BulkString,
    Array
};

struct RespValue {
    RespType type;
    long long intValue = 0;
    std::string str;
    std::vector<RespValue> items;
    bool isNull = false;

    static RespValue simpleString(const std::string& s) {
        RespValue v; v.type = RespType::SimpleString; v.str = s; return v;
    }
    static RespValue error(const std::string& s) {
        RespValue v; v.type = RespType::Error; v.str = s; return v;
    }
    static RespValue integer(long long n) {
        RespValue v; v.type = RespType::Integer; v.intValue = n; return v;
    }
    static RespValue bulkString(const std::string& s) {
        RespValue v; v.type = RespType::BulkString; v.str = s; return v;
    }
    static RespValue nullBulk() {
        RespValue v; v.type = RespType::BulkString; v.isNull = true; return v;
    }
    static RespValue array(std::vector<RespValue> a) {
        RespValue v; v.type = RespType::Array; v.items = std::move(a); return v;
    }
    static RespValue nullArray() {
        RespValue v; v.type = RespType::Array; v.isNull = true; return v;
    }
};