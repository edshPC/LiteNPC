#pragma once

#include "Global.h"

#include "mc/deps/core/mce/Image.h"

namespace LiteNPC::Util {

    std::vector<std::string> split(std::string s, const std::string& delimiter = " ");
    std::unordered_set<Player*> getAllPlayers();
    std::string contatenateDialogue(const std::deque<std::string>& dialogue);

    int setTimeout(std::function<void()>, int ms = 0);
    int setInterval(std::function<void()>, int ms = 0, int count = 0);
    void clearTask(int id);

    void makeUnique(SerializedSkin&);
    bool readImage(const std::filesystem::path &path, mce::Image &image);
    bool tryLoadSkin(const std::string& name);
    void loadSkinPacks();

    void sendPlaySound(Vec3 pos, const std::string& name, float volume = 1, float pitch = 1);

} // namespace OreShop::Util