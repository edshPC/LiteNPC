#include "Global.h"

#include "ll/api/schedule/Scheduler.h"
#include "mod/Util.h"

#include <ll/api/utils/RandomUtils.h>
#include <mc/deps/core/utility/BinaryStream.h>
#include <mc/deps/json/Reader.h>
#include <mc/deps/json/ValueIterator.h>

#include "lodepng.h"

using namespace ll::schedule;

namespace LiteNPC::Util {
    ServerTimeScheduler taskScheduler;
    unordered_map<int, std::shared_ptr<Task<ll::chrono::ServerClock>>> tasks;
    int lastTaskId = 0;

    struct SkinData {
        string name;
        filesystem::path path;
        string geometry_name;
        Json::Value geometry;
    };
    unordered_map<string, SkinData*> skinDatas;

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

    void makeUnique(SerializedSkin& skin) {
        skin.mId = "Custom" + mce::UUID::random().asString();
        skin.mFullId = skin.mId + mce::UUID::random().asString();
        skin.mCapeId = mce::UUID::random().asString();
        std::stringstream stream;
        stream << std::hex << ll::random_utils::rand<uint64>();
        skin.mPlayFabId = stream.str();
        skin.setIsTrustedSkin(true);
    }

    bool readImage(const filesystem::path &path, mce::Image &image) {
        image.imageFormat = mce::ImageFormat::RGBA8Unorm;
        image.mUsage = mce::ImageUsage::sRGB;
        vector<uchar> data;
        auto err = lodepng::decode(data, image.mWidth, image.mHeight, path.string());
        if (err) return false;
        image.mImageBytes = {data};
        return true;
    }

    bool tryLoadSkin(const std::string& name) {
        auto& skins = NPC::getLoadedSkins();
        if (skins.contains(name)) return true;
        if (auto data = DB.get(name)) {
            SerializedSkin skin;
            BinaryStream bs(data.value(), true);
            skin.read(bs);
            skins[name] = skin;
            return true;
        }
        mce::Image image;
        if (readImage(NATIVE_MOD.getModDir() / std::format("skins/{}.png", name), image)) {
            SerializedSkin skin = image.mWidth == 128 ? skins.at("default128") : skins.at("default64");
            skin.mSkinImage = image;
            makeUnique(skin);
            skins[name] = skin;
            return true;
        }
        if (skinDatas.contains(name)) {
            SkinData* skinData = skinDatas.at(name);
            if (!readImage(skinData->path, image)) return false;
            SerializedSkin skin = skins.at("default64");
            skin.mSkinImage = image;
            Json::Value geometry;
            skin.mGeometryData = skinData->geometry;
            skin.mDefaultGeometryName = skinData->geometry_name;
            json j;
            j["geometry"]["default"] = skinData->geometry_name;
            skin.mResourcePatch = j.dump();
            makeUnique(skin);
            skins[name] = skin;
            return true;
        }
        return false;
    }

    void loadSkinPacks() {
        auto path = NATIVE_MOD.getModDir() / "skins/packs";
        filesystem::create_directories(path);
        Json::Reader reader;
        for (auto& pack : filesystem::directory_iterator(path)) {
            if (!pack.is_directory()) continue;
            LOGGER.info("Loading {}", pack.path().string());
            ifstream skins_stream(pack.path() / "skins.json"), geometry_stream(pack.path() / "geometry.json");
            Json::Value skins, geometries;
            reader.parse(skins_stream, skins, false);
            reader.parse(geometry_stream, geometries, false);
            Json::Value format_version = geometries.get("format_version", {});
            for (const Json::Value& skin : skins.get("skins", {})) {
                SkinData* skinData = new SkinData();
                string name = skin.get("localization_name", {}).asString("");
                skinData->path = pack.path() / skin.get("texture", {}).asString("");
                skinData->geometry_name = skin.get("geometry", {}).asString("");
                skinData->geometry[skinData->geometry_name] = geometries.get(skinData->geometry_name, {});
                skinData->geometry["format_version"] = format_version;
                skinDatas[name] = skinData;
            }
        }
        LOGGER.info("{} pack skins loaded", skinDatas.size());
    }

} // namespace LiteNPC::Util

// namespace OreShop
