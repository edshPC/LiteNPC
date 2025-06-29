#include "Global.h"

#include "mod/Util.h"

#include "ll/api/coro/CoroTask.h"
#include "ll/api/thread/ServerThreadExecutor.h"
#include <ll/api/utils/RandomUtils.h>
#include <mc/deps/core/utility/BinaryStream.h>
#include <mc/deps/core/image/ImageFormat.h>
#include <mc/deps/core/image/ImageUsage.h>
#include <mc/deps/json/Reader.h>
#include <mc/deps/json/ValueIterator.h>
#include <mc/network/packet/PlaySoundPacket.h>
#include <mc/deps/ecs/gamerefs_entity/EntityContext.h>
#include <mc/deps/ecs/gamerefs_entity/GameRefsEntity.h>

#include "lodepng.h"

using namespace ll::coro;

namespace LiteNPC::Util {
    std::atomic_int timeTaskId = 0;
    std::mutex locker;
    unordered_set<int> tasks;

    struct SkinData {
        string name;
        filesystem::path path;
        string geometry_name;
        Json::Value geometry;
    };
    unordered_map<string, SkinData*> skinDatas;

    int setTimeout(const function<void()>& f, int ms) {
        int tid = ++timeTaskId;
        keepThis([f, ms, tid]() -> CoroTask<> {
            co_await std::chrono::milliseconds(ms);
            std::lock_guard lock(locker);
            if (!tasks.contains(tid)) co_return;
            f();
            clearTask(tid);
        }).launch(ll::thread::ServerThreadExecutor::getDefault());
        std::lock_guard lock(locker);
        tasks.insert(tid);
        return tid;
    }

    int setInterval(const function<void()>& f, int ms, int count) {
        int tid = ++timeTaskId;
        keepThis([f, ms, count, tid]() -> CoroTask<> {
            int left = count;
            while (true) {
                co_await std::chrono::milliseconds(ms);
                std::lock_guard lock(locker);
                if (!tasks.contains(tid)) break;
                f();
                if (count && --left == 0) break;
            }
            clearTask(tid);
        }).launch(ll::thread::ServerThreadExecutor::getDefault());
        std::lock_guard lock(locker);
        tasks.insert(tid);
        return tid;
    }

    void clearTask(int tid) {
        std::lock_guard lock(locker);
        tasks.erase(tid);
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

    std::string contatenateDialogue(const std::deque<std::string>& dialogue) {
        stringstream ss;
        for (const auto& text : dialogue) ss << text << "\n§r\n";
        return ss.str();
    }

    Vec2 rotationFromDirection(Vec3 dir) {
        dir = dir.normalize();
        Vec2 res{-std::asin(dir.y), std::atan2(dir.z, dir.x)};
        res *= 180.0f / std::numbers::pi_v<float>;
        res.z -= 90;
        return res;
    }

    void makeUnique(SerializedSkin& skin) {
        skin.mId = "Custom" + mce::UUID::random().asString();
        skin.mFullId = *skin.mId + mce::UUID::random().asString();
        skin.mCapeId = mce::UUID::random().asString();
        std::stringstream stream;
        stream << std::hex << ll::random_utils::rand<uint64>();
        skin.mPlayFabId = stream.str();
        skin.mIsTrustedSkin = TrustedSkinFlag::True;
    }

    bool readImage(const filesystem::path &path, mce::Image &image) {
        image.imageFormat = mce::ImageFormat::RGBA8Unorm;
        image.mUsage = mce::ImageUsage::SRGB;
        vector<uchar> data;
        auto err = lodepng::decode(data, image.mWidth, image.mHeight, path.string());
        if (err) return false;
        image.resizeImageBytesToFitImageDescription();
        std::copy(data.begin(), data.end(), **image.mImageBytes->mBlob);
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
        mce::Image image(0, 0, mce::ImageFormat::Unknown, mce::ImageUsage::Unknown);
        if (readImage(NATIVE_MOD.getModDir() / std::format("skins/{}.png", name), image)) {
            SerializedSkin skin = image.mWidth == 128 ? skins.at("default128") : skins.at("default64");
            skin.mSkinImage = std::move(image);;
            makeUnique(skin);
            skins[name] = skin;
            return true;
        }
        if (skinDatas.contains(name)) {
            SkinData* skinData = skinDatas.at(name);
            if (!readImage(skinData->path, image)) return false;
            SerializedSkin skin = skins.at("default64");
            skin.mSkinImage = std::move(image);
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
            skins_stream.close(); geometry_stream.close();
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

    void sendPlaySound(Vec3 pos, const string& name, float volume, float pitch) {
        PlaySoundPacket pkt(name, pos, volume, pitch);
        pkt.sendToClients();
    }

    std::unordered_set<Player*> getPlayersNear(Vec3 pos, float distance) {
        std::unordered_set<Player*> player_list;
        LEVEL->forEachPlayer([&](Player& pl) {
            if (pl.getFeetPos().distanceTo(pos) < distance) player_list.insert(&pl);
            return true;
        });
        return player_list;
    }

    Actor* getRandomActor() {
        for (auto& i : LEVEL->getEntities()) {
            if (i.has_value() && i.tryUnwrap().has_value()) {
                return &i.tryUnwrap().get();
            }
        }
        return nullptr;
    }

} // namespace LiteNPC::Util

// namespace OreShop
