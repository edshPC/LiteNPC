#include "LiteNPC.h"
#include "Global.h"
#include "mod/NetworkPacket.h"

#include <ll/api/chrono/GameChrono.h>

#include "mc/network/MinecraftPackets.h"
#include "mc/network/packet/AddPlayerPacket.h"
#include "mc/network/packet/PlayerSkinPacket.h"
#include "mc/network/packet/RemoveActorPacket.h"
#include "mc/network/packet/SetActorDataPacket.h"
#include "mc/network/packet/MovePlayerPacket.h"
#include "mc/network/packet/AnimatePacket.h"
#include "mc/network/packet/EmotePacket.h"
#include "mc/network/packet/AddActorPacket.h"
#include "mc/network/packet/SetActorLinkPacket.h"
#include "mc/network/packet/MobEquipmentPacket.h"
#include "mc/network/packet/TextPacket.h"
#include "mc/network/packet/PlaySoundPacket.h"
#include "mc/deps/core/utility/BinaryStream.h"
#include "mc/world/actor/ActorLink.h"
#include "mc/world/actor/SynchedActorDataEntityWrapper.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/actor/ActorDefinitionIdentifier.h"
#include "mc/entity/components/AttributesComponent.h"

#include "ll/api/utils/RandomUtils.h"
#include "ll/api/form/SimpleForm.h"

namespace LiteNPC {
    set<Entity*> tickingEntities;
    unordered_map<uint64, NPC*> loadedNPC;
    unordered_map<string, SerializedSkin> loadedSkins;
    const Vec3 eyeHeight = {.0f, 1.62f, .0f};
    deque<string> current_dialogue;
    deque<deque<string>> dialog_history = {{}};

    Entity::Entity(const string& name, Vec3 pos, int dim, Vec2 rot):
        name(name), pos(pos), dim(dim), rot(rot),
        actorId(LEVEL->getNewUniqueID()), runtimeId(LEVEL->getNextRuntimeID()) {}

    Entity::~Entity() {
        tickingEntities.erase(this);
    }

    void Entity::spawnAll(Player *pl) {
        for (auto en : tickingEntities) en->spawn(pl);
    }

    void Entity::tickAll(uint64 tick) {
        for (auto en : tickingEntities) en->tick(tick);
        if (tick % 20 == 0) NPC::updateDialogue();
    }

    void Entity::loadEntity(Entity *entity) {
        tickingEntities.insert(entity);
        for (auto pl: Util::getAllPlayers()) entity->spawn(pl);
    }

    void Entity::tick(uint64 tick) {
    }

    void Entity::updateActorData() {
        auto pkt = static_pointer_cast<SetActorDataPacket>(MinecraftPackets::createPacket(MinecraftPacketIds::SetActorData));
        pkt->mId = runtimeId;
        putActorData(pkt->mPackedItems);
        pkt->sendToClients();
    }

    void Entity::remove() {
        for (auto pl : Util::getAllPlayers()) despawn(pl);
        delete this;
    }

    void Entity::despawn(Player *pl) {
        RemoveActorPacket pkt;
        pkt.mEntityId = actorId;
        pl->sendNetworkPacket(pkt);
    }

    void Entity::putActorData(vector<unique_ptr<DataItem>> &data) {
        data.clear();
        data.emplace_back(DataItem::create(ActorDataIDs::Name, name)); // name
        data.emplace_back(DataItem::create(ActorDataIDs::Reserved038, size)); // size
        data.emplace_back(DataItem::create(ActorDataIDs::NametagAlwaysShow, (schar) showNametag));
        int64 flag = 0, flag_extended = 0;
        for (auto f : flags) {
            flag |= 1ll << static_cast<int>(f);
            flag_extended |= 1ll << static_cast<int>(f) - 64;
        }
        data.emplace_back(DataItem::create(ActorDataIDs::Reserved0, flag));
        data.emplace_back(DataItem::create(ActorDataIDs::Reserved092, flag_extended));
    }

    void Entity::rename(const string &name) {
        if (this->name != name) {
            this->name = name;
            updateActorData();
        }
    }

    void Entity::resize(float size) {
        if (this->size != size) {
            this->size = size;
            updateActorData();
        }
    }

    void Entity::setShowNametag(bool showNametag) {
        if (this->showNametag != showNametag) {
            this->showNametag = showNametag;
            updateActorData();
        }
    }

    // ---------- NPC ----------

    NPC::NPC(const string& name, Vec3 pos, int dim, Vec2 rot, const string &skin, const function<void(Player *)> &cb):
        Entity(name, pos, dim, rot),
        skinName(skin), callback(cb), uuid(mce::UUID::random()),
        minecart(LEVEL->getNewUniqueID(), LEVEL->getNextRuntimeID()),
        freeTick(0), hand(ItemStack::EMPTY_ITEM()), isSitting(false) {}

    NPC::~NPC() {
        loadedNPC.erase(runtimeId);
    }

    void NPC::remove(bool instant) {
        if (instant) freeTick = 0;
        auto pkt = make_unique<RemoveActorPacket>();
        pkt->mEntityId = actorId;
        newAction(move(pkt));
        int delay = freeTick + 1 - LEVEL->getCurrentTick().tickID;
        Util::setTimeout([this] {
            delete this;
        }, delay * 50);
    }

    void NPC::remove() { remove(false); }

    void NPC::newAction(unique_ptr<Packet> pkt, uint64 delay, function<void()> cb) {
        uint64 tick = std::max(LEVEL->getCurrentTick().tickID + 1, freeTick);
        freeTick = tick + delay;
        actions[tick] = { move(pkt), cb };
    }

    void NPC::tick(uint64 tick) {
        Entity::tick(tick);
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
            if (isSitting) {
                sit(false);
                sit();
            }
        }, 100);
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
        auto act = Util::getRandomActor();
        if (!act) return;
        auto pkt = make_unique<SetActorDataPacket>(
            runtimeId,
            act->mEntityData,
            nullptr,
            LEVEL->getCurrentTick().tickID,
            false
        );
        putActorData(pkt->mPackedItems);
        newAction(move(pkt));
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
        Vec2 dest = Util::rotationFromDirection(pos - this->pos - eyeHeight);
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
            auto dim = LEVEL->getDimension(this->dim).lock();
            if (!dim) return;
            auto& bs = dim->getBlockSourceFromMainChunkSource();
            if (auto bl = &const_cast<Block&>(bs.getBlock(bp))) {
                 bl->getLegacyBlock()._onHitByActivatingAttack(bs, bp, nullptr);
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

    void NPC::sayTo(Player *pl, const string &text) {
        newAction(nullptr, 1, [this, text, pl] {
            TextPacket pkt;
            pkt.mType = TextPacketType::Popup;
            pkt.mMessage = text + "\n§r";
            pl->sendNetworkPacket(pkt);
        });
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
            pkt->mUnpack->emplace_back(DataItem::create(ActorDataIDs::Reserved038, .001f));
            ActorLink link;
            link.type = ActorLinkType::Passenger;
            link.A = minecart.actorId;
            link.B = actorId;
            pkt->mLinks->push_back(move(link));
            newAction(move(pkt));
            isSitting = true;
        } else if (!setSitting && isSitting) {
            pos.y += .5f;
            auto link_pkt = make_unique<SetActorLinkPacket>();
            link_pkt->mLink->type = ActorLinkType::None;
            link_pkt->mLink->A = minecart.actorId;
            link_pkt->mLink->B = actorId;
            newAction(move(link_pkt));
            auto rm_pkt = make_unique<RemoveActorPacket>();
            rm_pkt->mEntityId = minecart.actorId;
            newAction(move(rm_pkt));
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
        loadEntity(npc);
        return npc;
    }

    void NPC::setSkin(const string &skin) {
        this->skinName = skin;
        updateSkin();
    }

    void NPC::setHand(const ItemStack &item) {
        hand = ItemStack(item);
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
        DB.set(name, bs.mBuffer);
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

    FloatingText::FloatingText(const string &text, Vec3 pos, int dim):
        Entity(text, pos, dim, {}) {
        size = 0.0001f;
    }

    void FloatingText::spawn(Player *pl) {
        BinaryStream bs;
        bs.writeVarInt64(actorId.rawID, nullptr, nullptr); //actorId
        bs.writeUnsignedVarInt64(runtimeId.rawID, nullptr, nullptr); //actorRId
        bs.writeString("minecraft:armor_stand", nullptr, nullptr); //type
        bs.writeFloat(pos.x, nullptr, nullptr); // pos
        bs.writeFloat(pos.y, nullptr, nullptr);
        bs.writeFloat(pos.z, nullptr, nullptr);
        for (int i = 0; i < 7; i++) bs.writeFloat(0, nullptr, nullptr);
        bs.writeUnsignedVarInt(0, nullptr, nullptr); //0 atts
        vector<std::unique_ptr<DataItem>> items;
        putActorData(items);
        bs.writeType(items);
        bs.writeUnsignedVarInt(0, nullptr, nullptr); //0 props
        bs.writeUnsignedVarInt(0, nullptr, nullptr); //0 links
        bs.writeUnsignedVarInt(0, nullptr, nullptr); //end
        auto pkt = lse::api::NetworkPacket<MinecraftPacketIds::AddActor>(std::move(bs.mBuffer));
        pl->sendNetworkPacket(pkt);
    }

    FloatingText* FloatingText::create(string text, Vec3 pos, int dim) {
        auto ft = new FloatingText(text, pos, dim);
        loadEntity(ft);
        return ft;
    }
}
