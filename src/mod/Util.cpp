#include "Global.h"

#include "ll/api/schedule/Scheduler.h"
#include "mod/Util.h"

using namespace ll::schedule;

namespace LiteNPC::Util {
    ServerTimeScheduler taskScheduler;

    void setTimeout(function<void()> f, int ms) {
        taskScheduler.add<DelayTask>(chrono::milliseconds(ms), f);
    }

    void setInterval(function<void()> f, int ms, int count) {
        taskScheduler.add<RepeatTask>(chrono::milliseconds(ms), f, count);
    }

    std::vector<std::string> split(std::string s, const std::string& delimiter) {
        std::vector<std::string> tokens;
        size_t pos = 0;
        while ((pos = s.find(delimiter)) != std::string::npos) {
            if (std::string token = s.substr(0, pos); !token.empty()) tokens.push_back(token);
            s.erase(0, pos + delimiter.length());
        }
        if (!s.empty()) tokens.push_back(s);
        return tokens;
    }

    std::unordered_set<Player*> getAllPlayers() {
        try {
            std::unordered_set<Player*> player_list;
            LEVEL->forEachPlayer([&](Player& pl) {
                player_list.insert(&pl);
                return true;
            });
            return player_list;
        } catch (...) {}
        return {};
    }
} // namespace LiteNPC::Util

// namespace OreShop
