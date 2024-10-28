#include "NPC.h"

#include <ll/api/chrono/GameChrono.h>

#include "mc/network/packet/AddPlayerPacket.h"
#include "mc/network/packet/PlayerSkinPacket.h"
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
#include "mc/world/level/Spawner.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/Block.h"

#include "ll/api/utils/RandomUtils.h"

namespace LiteNPC {
    unordered_map<uint64, NPC*> loadedNPC;
    unordered_map<string, SerializedSkin> loadedSkins;
    const Vec3 eyeHeight = {.0f, 1.62f, .0f};

    void NPC::remove() {
        loadedNPC.erase(runtimeId);
        RemoveActorPacket pkt{actorId};
        pkt.sendToClients();
        delete this;
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
        BinaryStream bs;
        NetworkItemStackDescriptor(hand).write(bs);
        pkt.mCarriedItem.read(bs);
        pl->sendNetworkPacket(pkt);

        Util::setTimeout([this, pl]() { updateSkin(pl); });
        if (isSitting) {
            sit(false);
            sit();
        }
        //Util::setInterval([this] {setHand(ItemStack("minecraft:stone"));}, 1000);
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

    void NPC::updatePosition() {
        auto pkt = make_unique<MovePlayerPacket>();
        pkt->mPlayerID = runtimeId;
        pkt->mPos = pos + eyeHeight;
        pkt->mRot = rot;
        pkt->mYHeadRot = rot.y;
        pkt->mOnGround = true;
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
        if (offset.y > 180) offset.y -= 360;
        else if (offset.y < -180) offset.y += 360;
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

    void NPC::delay(uint64 ticks) {
        if (ticks) newAction(nullptr, ticks);
    }

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
        try { if (callback) callback(pl); }
        catch (...) { LOGGER.error("{}: Cant fire callback", name); }
    }

    void NPC::setCallback(function<void(Player *pl)> callback) {
        this->callback = callback;
    }

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

    void NPC::setName(string name) {
        this->name = name;

        SetActorDataPacket pkt;
        pkt.mId = runtimeId;
        pkt.mPackedItems.emplace_back(DataItem::create(ActorDataIDs::Name, name)); // text
        pkt.sendToClients();
    }

    void NPC::setSkin(string skin) {
        this->skinName = skin;
        for (auto pl: Util::getAllPlayers()) updateSkin(pl);
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

    void NPC::saveSkin(string name, SerializedSkin &skin) {
        loadedSkins[name] = skin;
        auto &newSkin = loadedSkins.at(name);

        newSkin.mId = "Custom" + mce::UUID::random().asString();
        newSkin.mFullId = newSkin.mId + mce::UUID::random().asString();
        newSkin.mCapeId = mce::UUID::random().asString();
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
