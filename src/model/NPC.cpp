#include "NPC.h"

#include "mc/network/packet/AddPlayerPacket.h"
#include "mc/network/packet/PlayerSkinPacket.h"
#include "mc/network/packet/RemoveActorPacket.h"
#include "mc/network/packet/SetActorDataPacket.h"
#include "mc/network/packet/MovePlayerPacket.h"
#include "mc/network/packet/EmotePacket.h"
#include "mc/deps/core/utility/BinaryStream.h"

#include "ll/api/utils/RandomUtils.h"

namespace LiteNPC {
    unordered_map<unsigned long long, NPC *> loadedNPC;
    unordered_map<string, SerializedSkin> loadedSkins;

    void NPC::remove() {
        loadedNPC.erase(runtimeId);
        RemoveActorPacket pkt{actorId};
        pkt.sendToClients();
    }

    void NPC::spawn(Player *pl) {
        if (pl->getDimensionId().id != dim) return;

        AddPlayerPacket pkt;
        pkt.mName = name;
        pkt.mUuid = uuid;
        pkt.mEntityId = actorId;
        pkt.mRuntimeId = runtimeId;
        pkt.mPos = pos;
        pkt.mRot = rot;
        pkt.mYHeadRot = rot.y;
        pkt.mUnpack.emplace_back(DataItem::create(ActorDataIDs::NametagAlwaysShow, (schar)1));
        pl->sendNetworkPacket(pkt);

        Util::setTimeout([this, pl]() { this->updateSkin(pl); });
    }

    void NPC::updateSkin(Player *pl) {
        if (!loadedSkins.contains(skinName)) return;
        auto &skin = loadedSkins.at(skinName);
        PlayerSkinPacket pkt;
        pkt.mUUID = uuid;
        pkt.mSkin = skin;
        pkt.mLocalizedNewSkinName = skinName;
        pl->sendNetworkPacket(pkt);
    }

    void NPC::emote(string emoteName) {
        if (!emotionsConfig.emotions.contains(emoteName)) return;
        EmotePacket pkt;
        pkt.mRuntimeId = runtimeId;
        pkt.mPieceId = emotionsConfig.emotions.at(emoteName);
        pkt.mFlags = 0x2;
        pkt.sendToClients();
    }

    void NPC::moveTo(Vec3 dest, float speed) {
        dest += Vec3(.5f, .0f, .5f);
        Vec3 offset = dest - pos;
        int steps = std::ceil(offset.length() / (.21585f * speed));
        if (steps == 0) return;
        Vec3 step = offset / steps;
        Vec2 look = Vec3::rotationFromDirection(offset);
        LOGGER.info("off {} step {} steps {}", offset.toString(), step.toString(), steps);
        Util::setInterval([this, step, look] {
            pos += step;
            MovePlayerPacket pkt;
            pkt.mPlayerID = runtimeId;
            pkt.mPos = pos + Vec3(.0f, 1.62f,  .0f);
            pkt.mRot = look;
            pkt.mYHeadRot = look.y;
            pkt.mOnGround = true;
            pkt.sendToClients();
        }, 0, steps);
    }

    void NPC::onUse(Player *pl) {
        callback(pl);
    }

    void NPC::setCallback(function<void(Player *pl)> callback) {
        this->callback = callback;
    }

    NPC *NPC::create(string name, Vec3 pos, int dim, Vec2 rot, string skin, function<void(Player *pl)> callback) {
        NPC *npc = new NPC(name, pos, dim, rot, skin, callback);
        loadedNPC[npc->runtimeId] = npc;
        for (auto pl : Util::getAllPlayers()) npc->spawn(pl);
        return npc;
    }

    void NPC::spawnAll(Player *pl) {
        for (auto entry: loadedNPC) {
            entry.second->spawn(pl);
        }
    }

    void NPC::setName(string name) {
        this->name = name;

        SetActorDataPacket pkt;
        pkt.mId = runtimeId;
        pkt.mPackedItems.emplace_back(DataItem::create(ActorDataIDs::Name, name)); // text
        pkt.sendToClients();
    }

    void NPC::setSkin(string skin) {
        this->skinName = skin;
        for (auto pl : Util::getAllPlayers()) updateSkin(pl);
    }

    NPC *NPC::getByRId(unsigned long long rId) {
        if (loadedNPC.contains(rId)) return loadedNPC.at(rId);
        return nullptr;
    }

    void NPC::saveSkin(string name, SerializedSkin &skin) {
        loadedSkins[name] = skin;
        auto &newSkin = loadedSkins.at(name);

        newSkin.mId = "Custom" + mce::UUID().asString();
        newSkin.mFullId = newSkin.mId + mce::UUID().asString();
        newSkin.mCapeId = mce::UUID().asString();
        std::stringstream stream;
        stream << std::hex << ll::random_utils::rand<uint64>();
        newSkin.mPlayFabId = stream.str();
        newSkin.setIsTrustedSkin(true);

        BinaryStream bs;
        newSkin.write(bs);
        DB.set(name, bs.getAndReleaseData());
    }

    void NPC::saveEmotion(string name, string emotionUuid) {
        emotionsConfig.emotions[name] = emotionUuid;
        ll::config::saveConfig(emotionsConfig, NATIVE_MOD.getConfigDir() / "emotions.json");
    }

    void NPC::init() {
        DB.iter([](string_view k, string_view v) -> bool {
            string name(k), data(v);
            loadedSkins[name] = SerializedSkin::createTrustedDefaultSerializedSkin();
            SerializedSkin &skin = loadedSkins.at(name);
            BinaryStream bs(data, true);
            skin.read(bs);
            return true;
        });
    }
}
