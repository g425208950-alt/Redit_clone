#pragma once
#include <optional>
#include <string>
#include "RespValue.h"

class Decoder {
public:
    std::optional<RespValue> tryParse(const std::string& buf, size_t& consumed);
private:
    std::optional<RespValue> parseAt(const std::string& buf, size_t start, size_t& consumed);
};