#include "NPC.h"

#include <ll/api/chrono/GameChrono.h>

#include "mc/network/packet/AddPlayerPacket.h"
#include "mc/network/packet/PlayerSkinPacket.h"
#include "mc/network/packet/PlayerActionPacket.h"
#include "mc/network/packet/ActorEventPacket.h"
#include "mc/network/packet/RemoveActorPacket.h"
#include "mc/network/packet/SetActorDataPacket.h"
#include "mc/network/packet/MovePlayerPacket.h""
#include "mc/network/packet/AnimatePacket.h"
#include "mc/network/packet/EmotePacket.h"
#include "mc/network/packet/AddActorPacket.h"
#include "mc/network/packet/SetActorLinkPacket.h"
#include "mc/network/packet/MobEquipmentPacket.h"
#include "mc/network/packet/TextPacket.h"
#include "mc/deps/core/utility/BinaryStream.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/item/Item.h"

#include "ll/api/utils/RandomUtils.h"

namespace LiteNPC {
    unordered_map<uint64, NPC*> loadedNPC;
    unordered_map<string, SerializedSkin> loadedSkins;
    const Vec3 eyeHeight = {.0f, 1.62f, .0f};

    void NPC::remove(bool instant) {
        if (instant) freeTick = 0;
        newAction(make_unique<RemoveActorPacket>(actorId));
        int delay = freeTick + 1 - LEVEL->getCurrentTick().t;
        Util::setTimeout([this] {
            loadedNPC.erase(runtimeId);
            delete this;
        }, delay * 50);
    }

    void NPC::newAction(unique_ptr<Packet> pkt, uint64 delay, function<void()> cb) {
        uint64 tick = std::max(LEVEL->getCurrentTick().t + 1, freeTick);
        freeTick = tick + delay;
        actions[tick] = { move(pkt), cb };
    }

    void NPC::tick(uint64 tick) {
        if (auto it = actions.begin(); it != actions.end() && tick >= it->first) {
            if(it->second.pkt) it->second.pkt->sendToClients();
            if(it->second.cb) it->second.cb();
            actions.erase(it);
        }
    }

    void NPC::spawn(Player *pl) {
        if (pl->getDimensionId() != dim) return;

        AddPlayerPacket pkt;
        pkt.mName = name;
        pkt.mUuid = uuid;
        pkt.mEntityId = actorId;
        pkt.mRuntimeId = runtimeId;
        pkt.mPos = pos;
        pkt.mRot = rot;
        pkt.mYHeadRot = rot.y;
        pkt.mUnpack.emplace_back(DataItem::create(ActorDataIDs::NametagAlwaysShow, (schar) 1));
        pkt.mUnpack.emplace_back(DataItem::create(ActorDataIDs::Reserved038, size));
        BinaryStream bs;
        NetworkItemStackDescriptor(hand).write(bs);
        pkt.mCarriedItem.read(bs);
        pl->sendNetworkPacket(pkt);

        Util::setTimeout([this, pl]() { updateSkin(pl); });
        if (isSitting) {
            sit(false);
            sit();
        }
    }

    void NPC::updateSkin(Player *pl) {
        if (!Util::tryLoadSkin(skinName)) return;
        auto &skin = loadedSkins.at(skinName);
        PlayerSkinPacket pkt;
        pkt.mUUID = uuid;
        pkt.mSkin = skin;
        pkt.mLocalizedNewSkinName = skinName;
        if (pl) pl->sendNetworkPacket(pkt);
        else pkt.sendToClients();
    }

    void NPC::updatePosition() {
        auto pkt = make_unique<MovePlayerPacket>();
        pkt->mPlayerID = runtimeId;
        pkt->mPos = pos + eyeHeight;
        pkt->mRot = rot;
        pkt->mYHeadRot = rot.y;
        pkt->mOnGround = true;
        newAction(move(pkt));
    }

    void NPC::updateActorData() {
        auto pkt = make_unique<SetActorDataPacket>();
        pkt->mId = runtimeId;
        pkt->mPackedItems.emplace_back(DataItem::create(ActorDataIDs::Name, name)); // name
        pkt->mPackedItems.emplace_back(DataItem::create(ActorDataIDs::Reserved038, size)); // size
        pkt->mPackedItems.emplace_back(DataItem::create(ActorDataIDs::Reserved0, isEating ? 0x10ll : 0ll)); // flags
        newAction(move(pkt));
    }

    void NPC::emote(string emoteName) {
        if (!emotionsConfig.emotions.contains(emoteName)) return;
        auto pkt = make_unique<EmotePacket>();
        pkt->mRuntimeId = runtimeId;
        pkt->mPieceId = emotionsConfig.emotions.at(emoteName);
        pkt->mFlags = 0x2;
        newAction(move(pkt), 50);
    }

    void NPC::moveTo(Vec3 dest, float speed) {
        Vec3 offset = dest - pos;
        int steps = std::ceil(offset.length() / (.21585f * speed));
        if (steps == 0) return;
        Vec3 step = offset / steps;
        lookAt(dest + eyeHeight);
        // LOGGER.info("off {} step {} steps {}", offset.toString(), step.toString(), steps);
        for (int i = 0; i < steps; i++) {
            pos += step;
            updatePosition();
        }
        freeTick += 5;
    }

    void NPC::moveTo(BlockPos pos, float speed) { moveTo(pos.bottomCenter(), speed); }

    void NPC::moveToBlock(BlockPos pos, float speed) { moveTo(pos, speed); }

    void NPC::lookAt(Vec3 pos) {
        Vec2 dest = Vec3::rotationFromDirection(pos - this->pos - eyeHeight);
        lookRot(dest);
    }

    void NPC::lookRot(Vec2 dest) {
        Vec2 offset = dest - rot;
        while (offset.y > 180) offset.y -= 360;
        while (offset.y < -180) offset.y += 360;
        int steps = std::ceil(offset.length() / 20);
        if (steps == 0) return;
        Vec2 step = offset / steps;
        // LOGGER.info("off {} step {} steps {}", offset.toString(), step.toString(), steps);
        for (int i = 0; i < steps; i++) {
            rot += step;
            updatePosition();
        }
    }

    void NPC::lookRot(float rotX, float rotY) { lookRot(Vec2(rotX, rotY)); }

    void NPC::swing() {
        auto pkt = make_unique<AnimatePacket>();
        pkt->mRuntimeId = runtimeId;
        pkt->mAction = AnimatePacket::Action::Swing;
        newAction(move(pkt), 20);
    }

    void NPC::eat(int64 time) {
        isEating = true;
        updateActorData();
        isEating = false;
        freeTick += time;
        updateActorData();
    }

    void NPC::interactBlock(BlockPos bp) {
        lookAt(bp.center());
        newAction(nullptr, 1, [this, bp] {
            auto dim = LEVEL->getDimension(this->dim).get();
            if (!dim) return;
            auto& bs = dim->getBlockSourceFromMainChunkSource();
            if (auto bl = &const_cast<Block&>(bs.getBlock(bp))) {
                bl->onHitByActivatingAttack(bs, bp, nullptr);
            }
        });
        swing();
    }

    void NPC::say(const string &text) {
        auto pkt = make_unique<TextPacket>();
        pkt->mType = TextPacketType::Raw;
        pkt->mMessage = text;
        newAction(move(pkt));
    }

    void NPC::delay(uint64 ticks) { freeTick += ticks; }

    void NPC::sit(bool setSitting) {
        if (setSitting && !isSitting) {
            auto pkt = make_unique<AddPlayerPacket>();
            pkt->mUuid = mce::UUID::random();
            pkt->mEntityId = minecart.actorId;
            pkt->mRuntimeId = minecart.runtimeId;
            pos.y -= .5f;
            pkt->mPos = pos;
            pkt->mUnpack.emplace_back(DataItem::create(ActorDataIDs::Reserved038, .0001f));
            ActorLink link;
            link.mType = ActorLinkType::Passenger;
            link.mA = minecart.actorId;
            link.mB = actorId;
            pkt->mLinks.emplace_back(link);
            newAction(move(pkt));
            isSitting = true;
        } else if (!setSitting && isSitting) {
            pos.y += .5f;
            ActorLink link;
            link.mType = ActorLinkType::None;
            link.mA = minecart.actorId;
            link.mB = actorId;
            newAction(make_unique<SetActorLinkPacket>(link));
            newAction(make_unique<RemoveActorPacket>(minecart.actorId));
            updatePosition();
            isSitting = false;
        }
    }

    void NPC::onUse(Player *pl) {
        try {
            if (callback && freeTick < LEVEL->getCurrentTick()) callback(pl);
        }
        catch (...) { LOGGER.error("{}: Cant fire callback", name); }
    }

    void NPC::setCallback(function<void(Player *pl)> callback) { this->callback = callback; }

    NPC *NPC::create(string name, Vec3 pos, int dim, Vec2 rot, string skin, function<void(Player *pl)> callback) {
        NPC *npc = new NPC(name, pos + Vec3(0.5f, 0.0f, 0.5f), dim, rot, skin, callback);
        loadedNPC[npc->runtimeId] = npc;
        for (auto pl: Util::getAllPlayers()) npc->spawn(pl);
        return npc;
    }

    void NPC::spawnAll(Player *pl) {
        for (auto [id, npc]: loadedNPC) npc->spawn(pl);
    }

    void NPC::tickAll(uint64 tick) {
        for (auto [id, npc]: loadedNPC) npc->tick(tick);
    }

    void NPC::rename(string name) {
        this->name = name;
        updateActorData();
    }

    void NPC::resize(float size) {
        this->size = size;
        updateActorData();
    }

    void NPC::setSkin(string skin) {
        this->skinName = skin;
        updateSkin();
    }

    void NPC::setHand(const ItemStack &item) {
        hand = item.clone();
        auto pkt = make_unique<MobEquipmentPacket>(runtimeId, hand, 0, 0, ContainerID::Inventory);
        newAction(move(pkt));
    }

    NPC *NPC::getByRId(uint64 rId) {
        if (loadedNPC.contains(rId)) return loadedNPC.at(rId);
        return nullptr;
    }

    vector<NPC*> NPC::getAll() {
        vector<NPC*> ret;
        for (auto [id, npc]: loadedNPC) ret.push_back(npc);
        return ret;
    }

    unordered_map<string, SerializedSkin>& NPC::getLoadedSkins() { return loadedSkins; }

    void NPC::saveSkin(string name, SerializedSkin &skin) {
        loadedSkins[name] = skin;
        auto& newSkin = loadedSkins.at(name);
        Util::makeUnique(newSkin);
        BinaryStream bs;
        newSkin.write(bs);
        DB.set(name, bs.getAndReleaseData());
        for (auto [id, npc]: loadedNPC)
            if (npc->skinName == name) npc->updateSkin();
    }

    void NPC::saveEmotion(string name, string emotionUuid) {
        emotionsConfig.emotions[name] = emotionUuid;
        ll::config::saveConfig(emotionsConfig, NATIVE_MOD.getConfigDir() / "emotions.json");
    }

    void NPC::init() {

        for (string name : {"default64", "default128"}) {
            ifstream def(NATIVE_MOD.getModDir() / ("skins/default/" + name));
            if (!def) continue;
            std::ostringstream oss;
            oss << def.rdbuf();
            string data = oss.str();
            BinaryStream bs(data, true);
            SerializedSkin skin;
            skin.read(bs);
            loadedSkins[name] = skin;
        }

        // SerializedSkin skin64 = loadedSkins["default64"];
        // skin64.mSkinImage.mImageBytes = {};
        // SerializedSkin skin128 = loadedSkins["default128"];
        // skin128.mSkinImage.mImageBytes = {};
        //
        // BinaryStream bs;
        // skin64.write(bs);
        // ofstream def64(NATIVE_MOD.getModDir() / "skins/default/default64");
        // def64 << bs.getAndReleaseData();
        // def64.close();
        // bs.reset();
        // skin128.write(bs);
        // ofstream def128(NATIVE_MOD.getModDir() / "skins/default/default128");
        // def128 << bs.getAndReleaseData();
        // def128.close();
    }

}
