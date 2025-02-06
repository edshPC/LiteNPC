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
#include "mc/network/packet/PlaySoundPacket.h"
#include "mc/deps/core/utility/BinaryStream.h"
#include "mc/world/actor/ActorLink.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/Block.h"

#include "ll/api/utils/RandomUtils.h"
#include "ll/api/form/SimpleForm.h"

namespace LiteNPC {
    unordered_map<uint64, NPC*> loadedNPC;
    unordered_map<string, SerializedSkin> loadedSkins;
    const Vec3 eyeHeight = {.0f, 1.62f, .0f};
    deque<string> current_dialogue;
    deque<deque<string>> dialog_history = {{}};

    NPC::NPC(string name, Vec3 pos, int dim, Vec2 rot, string skin, function<void(Player *)> cb) :
            name(name), pos(pos), dim(dim), rot(rot), skinName(skin), callback(cb) {}

    void NPC::remove(bool instant) {
        if (instant) freeTick = 0;
        newAction(make_unique<RemoveActorPacket>(actorId));
        int delay = freeTick + 1 - LEVEL->getCurrentTick().tickID;
        Util::setTimeout([this] {
            loadedNPC.erase(runtimeId);
            delete this;
        }, delay * 50);
    }

    void NPC::newAction(unique_ptr<Packet> pkt, uint64 delay, function<void()> cb) {
        uint64 tick = std::max(LEVEL->getCurrentTick().tickID + 1, freeTick);
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
        if (pl->getDimensionId().id != dim) return;

        AddPlayerPacket pkt;
        pkt.mName = name;
        pkt.mUuid = uuid;
        pkt.mEntityId = actorId;
        pkt.mRuntimeId = runtimeId;
        pkt.mPos = pos;
        pkt.mRot = rot;
        pkt.mYHeadRot = rot.y;
        putActorData(pkt.mUnpack);
        pl->sendNetworkPacket(pkt);

        Util::setTimeout([this, pl] {
            updateSkin(pl);
            setHand(hand);
        });
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
        auto pls = Util::getAllPlayers();
        if (pls.empty()) return;
        auto pkt = make_unique<MovePlayerPacket>(**pls.begin(), Vec3());
        pkt->mPlayerID = runtimeId;
        pkt->mPos = pos + eyeHeight;
        pkt->mRot = rot;
        pkt->mYHeadRot = rot.y;
        pkt->mResetPosition = PlayerPositionModeComponent::PositionMode::Normal;
        pkt->mOnGround = true;
        pkt->mRidingID = 0;
        pkt->mCause = 0;
        pkt->mSourceEntityType = 0;
        newAction(move(pkt));
    }

    void NPC::updateActorData() {
        auto pkt = make_unique<SetActorDataPacket>();
        pkt->mId = runtimeId;
        putActorData(pkt->mPackedItems);
        pkt->mTick = PlayerInputTick(LEVEL->getCurrentTick().tickID);
        newAction(move(pkt));
    }

    void NPC::putActorData(vector<unique_ptr<DataItem>> &data) {
        data.clear();
        data.emplace_back(DataItem::create(ActorDataIDs::Name, name)); // name
        data.emplace_back(DataItem::create(ActorDataIDs::Reserved038, size)); // size
        data.emplace_back(DataItem::create(ActorDataIDs::NametagAlwaysShow, (schar) 1));
        int64 flag = 0, flag_extended = 0;
        for (auto f : flags) {
            flag |= 1ll << static_cast<int>(f);
            flag_extended |= 1ll << static_cast<int>(f) - 64;
        }
        data.emplace_back(DataItem::create(ActorDataIDs::Reserved0, flag));
        data.emplace_back(DataItem::create(ActorDataIDs::Reserved092, flag_extended));
    }

    void NPC::updateDialogue() {
        if (!current_dialogue.empty()) {
            TextPacket pkt;
            pkt.mType = TextPacketType::Popup;
            pkt.mMessage = Util::contatenateDialogue(current_dialogue);
            pkt.sendToClients();
        }
    }

    void NPC::openDialogueHistory(Player* pl) {
        ll::form::SimpleForm form;
        form.setTitle("История диалогов");
        for (const auto& dialogue : dialog_history) {
            if (dialogue.empty()) continue;
            form.appendButton(dialogue.front(), [&dialogue](Player& pl) {
                ll::form::SimpleForm form;
                form.setTitle("Диалог");
                form.setContent(Util::contatenateDialogue(dialogue));
                form.sendTo(pl);
            });
        }
        form.sendTo(*pl);
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
        if (isSitting) sit(false);
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
        flags.insert(ActorFlags::Usingitem);
        updateActorData();
        flags.erase(ActorFlags::Usingitem);
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

    void NPC::say(const string &text, bool saveHistory) {
        if (saveHistory) newAction(nullptr, 1, [this, text] {
            current_dialogue.push_back(text);
            if (current_dialogue.size() > 2) current_dialogue.pop_front();
            updateDialogue();
            dialog_history.front().push_back(text);
        });
        else {
            auto pkt = make_unique<TextPacket>();
            pkt->mType = TextPacketType::Popup;
            pkt->mMessage = text + "\n§r";
            newAction(move(pkt));
        }
    }

    void NPC::delay(uint64 ticks) { newAction(nullptr, ticks); }

    void NPC::sit(bool setSitting) {
        if (setSitting && !isSitting) {
            auto pkt = make_unique<AddPlayerPacket>();
            pkt->mUuid = mce::UUID::random();
            pkt->mEntityId = minecart.actorId;
            pkt->mRuntimeId = minecart.runtimeId;
            pos.y -= .5f;
            pkt->mPos = pos;
            pkt->mUnpack->emplace_back(DataItem::create(ActorDataIDs::Reserved038, .0001f));
            ActorLink link;
            link.type = ActorLinkType::Passenger;
            link.A = minecart.actorId;
            link.B = actorId;
            pkt->mLinks->emplace_back(link);
            newAction(move(pkt));
            isSitting = true;
        } else if (!setSitting && isSitting) {
            pos.y += .5f;
            ActorLink link;
            link.type = ActorLinkType::None;
            link.A = minecart.actorId;
            link.B = actorId;
            newAction(make_unique<SetActorLinkPacket>(link));
            newAction(make_unique<RemoveActorPacket>(minecart.actorId));
            updatePosition();
            isSitting = false;
        }
    }

    void NPC::sleep(bool setSleeping) {
        if (setSleeping) flags.insert(ActorFlags::Sleeping);
        else flags.erase(ActorFlags::Sleeping);
        updateActorData();
    }

    void NPC::sneak(bool setSneaking) {
        if (setSneaking) flags.insert(ActorFlags::Sneaking);
        else flags.erase(ActorFlags::Sneaking);
        updateActorData();
    }

    void NPC::finishDialogue() {
        newAction(nullptr, 1, [this] {
            current_dialogue.clear();
            dialog_history.emplace_front();
        });
        lookRot(rot + Vec2(45, 0));
        lookRot(rot - Vec2(45, 0));
    }

    void NPC::playSound(const string& name, float volume, float pitch) {
        auto pkt = make_unique<PlaySoundPacket>(name, pos, volume, pitch);
        newAction(move(pkt));
    }

    void NPC::stop() {
        actions.clear();
        freeTick = 0;
    }

    void NPC::onUse(Player *pl) {
        try {
            if (callback && freeTick < LEVEL->getCurrentTick().tickID) callback(pl);
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
        if (tick % 20 == 0) updateDialogue();
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
    }

}
