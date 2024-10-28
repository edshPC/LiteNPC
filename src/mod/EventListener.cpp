#include "Global.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/event/player/PlayerJoinEvent.h"
#include "ll/api/memory/Hook.h"
#include <mc/network/packet/EmotePacket.h>
#include <mc/network/packet/PlayerSkinPacket.h>

#include "mc/network/ServerNetworkHandler.h"
#include "mc/world/inventory/transaction/ItemUseOnActorInventoryTransaction.h"
#include "mc/network/PacketObserver.h"
#include "mc/network/packet/SetActorLinkPacket.h"

using namespace ll::event;

#define EVENT_FUNCTION(event) void m## event(event& ev)
#define EVENT_REGISTER(event) bus.emplaceListener<event>(&m## event)
#define EVENT_REGISTER_PRIORITY(event, priority) bus.emplaceListener<event>(&m## event, EventPriority::##priority)
#define AUTO_TI_HOOK(name, type, method, ...) LL_VA_EXPAND(LL_TYPE_INSTANCE_HOOK(name##Hook, HookPriority::Normal, type, &type::method, __VA_ARGS__))

namespace LiteNPC {
    EVENT_FUNCTION(PlayerJoinEvent) {
        Player* pl = &ev.self();
        NPC::spawnAll(pl);
        NPC* npc = NPC::create("", pl->getFeetPos() - Vec3(0, 64, 0),pl->getDimensionId());
        for (auto& [name, uuid] : emotionsConfig.emotions) {
            auto pkt = make_unique<EmotePacket>();
            pkt->mRuntimeId = npc->getRId();
            pkt->mPieceId = uuid;
            pkt->mFlags = 0x2;
            npc->newAction(move(pkt));
        }
        npc->remove();
    }

    unordered_map<Player*, string> waitingEmotions;
    AUTO_TI_HOOK(SendEmotion, ServerNetworkHandler, handle, void, NetworkIdentifier const& source,
                 EmotePacket const& packet) {
        auto sp = getServerPlayer(source, packet.mClientSubId);
        if (sp && waitingEmotions.contains(sp)) {
            NPC::saveEmotion(waitingEmotions.at(sp), packet.mPieceId);
            waitingEmotions.erase(sp);
            sp->sendMessage("Emotion saved");
        }
    }

    AUTO_TI_HOOK(ChangeSkin, ServerNetworkHandler, handle, void, NetworkIdentifier const& source,
                 PlayerSkinPacket const& pkt) {
        auto pl = getServerPlayer(source, pkt.mClientSubId);
        if (pl) {
            auto& skin = pkt.mSkin;
            pl->updateSkin(skin, 1);
            pkt.sendTo(*static_cast<Actor*>(pl));
            pl->sendMessage("§eSkin changed");
        }
    }

    AUTO_TI_HOOK(PlayerChangeDimension, Level, requestPlayerChangeDimension, void, Player& pl, ChangeDimensionRequest&&) {
        Util::setTimeout([&pl] { NPC::spawnAll(&pl); });
    }

    LL_TYPE_INSTANCE_HOOK(OnUseNPCHook, HookPriority::Normal, ItemUseOnActorInventoryTransaction,
                          "?handle@ItemUseOnActorInventoryTransaction@@UEBA?AW4InventoryTransactionError@@AEAVPlayer@@_N@Z",
                          InventoryTransactionError, class Player& player, bool isSenderAuthority) {
        auto rId = ll::memory::dAccess<ActorRuntimeID>(this, 104);
        if (auto npc = NPC::getByRId(rId)) npc->onUse(&player);
        return origin(player, isSenderAuthority);
    }

    AUTO_TI_HOOK(Tick, Level, tick, void) {
        origin();
        NPC::tickAll(getCurrentTick().t);
    }

    string lastPkt;
    LL_TYPE_INSTANCE_HOOK(TmpHook, HookPriority::Normal, PacketObserver,
        "?packetSentTo@PacketObserver@@UEAAXAEBVNetworkIdentifier@@AEBVPacket@@I@Z",
        void, NetworkIdentifier const& id, Packet const& pkt, uint size) {
        origin(id, pkt, size);
        if (pkt.getName() != lastPkt) {
            LOGGER.info(pkt.getName());
            lastPkt = pkt.getName();
            if (lastPkt == "SetActorLinkPacket") {
                auto lpkt = static_cast<SetActorLinkPacket const*>(&pkt);
                LOGGER.info("type {}\na {}\nb {}\nim {}\npas {}",
                    (int)lpkt->mLink.mType, LEVEL->fetchEntity(lpkt->mLink.mA)->getTypeName(), LEVEL->fetchEntity(lpkt->mLink.mB)->getTypeName(),
                    lpkt->mLink.mImmediate, lpkt->mLink.mPassengerInitiated);
            }
        }
    }

    void registerEvents() {
        auto &bus = EventBus::getInstance();
        EVENT_REGISTER(PlayerJoinEvent);
        SendEmotionHook::hook();
        ChangeSkinHook::hook();
        OnUseNPCHook::hook();
        PlayerChangeDimensionHook::hook();
        TickHook::hook();
        //TmpHook::hook();
    }
} // namespace OreShop
