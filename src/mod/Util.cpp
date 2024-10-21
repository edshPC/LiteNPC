#include "Global.h"

#include "ll/api/schedule/Scheduler.h"
#include "mod/Util.h"

using namespace ll::schedule;

namespace LiteNPC::Util {
    ServerTimeScheduler taskScheduler;
    unordered_map<int, std::shared_ptr<Task<ll::chrono::ServerClock>>> tasks;
    int lastTaskId = 0;

    int setTimeout(function<void()> f, int ms) {
        tasks[++lastTaskId] = taskScheduler.add<DelayTask>(chrono::milliseconds(ms), f);
        return lastTaskId;
    }

    int setInterval(function<void()> f, int ms, int count) {
        tasks[++lastTaskId] = taskScheduler.add<RepeatTask>(chrono::milliseconds(ms), f, count);
        return lastTaskId;
    }

    void clearTask(int id) {
        if (tasks.contains(id)) {
            tasks.at(id)->cancel();
            tasks.erase(id);
        }
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
        std::unordered_set<Player*> player_list;
        LEVEL->forEachPlayer([&](Player& pl) {
            player_list.insert(&pl);
            return true;
        });
        return player_list;
    }
} // namespace LiteNPC::Util

// namespace OreShop
