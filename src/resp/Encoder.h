#pragma once
#include <string>
#include "RespValue.h"

class Encoder {
public:
    static std::string encode(const RespValue& v);
};