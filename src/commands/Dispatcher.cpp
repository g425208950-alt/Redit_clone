#include "Dispatcher.h"
#include <cctype>

std::string Dispatcher::toUpper(const std::string& s) {
    std::string r = s;
    for (auto& c : r) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return r;
}

Dispatcher::Dispatcher(Store& store) : store_(store) {}

RespValue Dispatcher::execute(const std::vector<std::string>& args) {
    if (args.empty()) return RespValue::error("ERR empty command");
    std::string cmd = toUpper(args[0]);

    if (cmd == "PING") {
        if (args.size() == 1) return RespValue::simpleString("PONG");
        if (args.size() == 2) return RespValue::bulkString(args[1]);
        return RespValue::error("ERR wrong number of arguments for 'ping' command");
    }
    if (cmd == "GET") {
        if (args.size() != 2) return RespValue::error("ERR wrong number of arguments for 'get' command");
        auto v = store_.get(args[1]);
        if (!v) return RespValue::nullBulk();
        return RespValue::bulkString(*v);
    }
    if (cmd == "SET") {
        if (args.size() != 3) return RespValue::error("ERR wrong number of arguments for 'set' command");
        store_.set(args[1], args[2]);
        return RespValue::simpleString("OK");
    }
    if (cmd == "DEL") {
        if (args.size() < 2) return RespValue::error("ERR wrong number of arguments for 'del' command");
        long long count = 0;
        for (size_t i = 1; i < args.size(); i++) if (store_.del(args[i])) count++;
        return RespValue::integer(count);
    }
    if (cmd == "EXISTS") {
        if (args.size() < 2) return RespValue::error("ERR wrong number of arguments for 'exists' command");
        long long count = 0;
        for (size_t i = 1; i < args.size(); i++) if (store_.exists(args[i])) count++;
        return RespValue::integer(count);
    }
    if (cmd == "COMMAND") {
        return RespValue::array({});
    }
    if (cmd == "HELLO") {
        return RespValue::error("ERR unknown command 'HELLO'");
    }
    if (cmd == "FLUSHALL" || cmd == "FLUSHDB") {
        return RespValue::simpleString("OK");
    }
    if (cmd == "QUIT") {
        return RespValue::error("QUIT");
    }
    return RespValue::error("ERR unknown command '" + args[0] + "'");
}