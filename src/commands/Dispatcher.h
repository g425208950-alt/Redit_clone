#pragma once
#include <string>
#include <vector>
#include "../resp/RespValue.h"
#include "../store/Store.h"

class Dispatcher {
public:
    explicit Dispatcher(Store& store);
    RespValue execute(const std::vector<std::string>& args);
private:
    Store& store_;
    static std::string toUpper(const std::string& s);
};