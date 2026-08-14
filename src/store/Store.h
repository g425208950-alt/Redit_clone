#pragma once
#include <string>
#include <optional>
#include <unordered_map>

class Store {
public:
    void set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key) const;
    bool del(const std::string& key);
    bool exists(const std::string& key) const;
private:
    std::unordered_map<std::string, std::string> map_;
};