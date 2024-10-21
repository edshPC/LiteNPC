#pragma once

#include "Global.h"

namespace LiteNPC::Util {

    std::vector<std::string> split(std::string s, const std::string& delimiter = " ");
    std::unordered_set<Player*> getAllPlayers();

    int setTimeout(std::function<void()>, int ms = 0);
    int setInterval(std::function<void()>, int ms = 0, int count = 0);
    void clearTask(int id);

} // namespace OreShop::Util