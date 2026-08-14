#include "Store.h"

void Store::set(const std::string& key, const std::string& value) {
    map_[key] = value;
}

std::optional<std::string> Store::get(const std::string& key) const {
    auto it = map_.find(key);
    if (it == map_.end()) return std::nullopt;
    return it->second;
}

bool Store::del(const std::string& key) {
    return map_.erase(key) > 0;
}

bool Store::exists(const std::string& key) const {
    return map_.find(key) != map_.end();
}