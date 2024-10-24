#include "Global.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/event/player/PlayerJoinEvent.h"
#include "ll/api/memory/Hook.h"
#include <mc/network/packet/EmotePacket.h>
#include "mc/network/ServerNetworkHandler.h"
#include "mc/world/inventory/transaction/ItemUseOnActorInventoryTransaction.h"

using namespace ll::event;

#define EVENT_FUNCTION(event) void m## event(event& ev)
#define EVENT_REGISTER(event) bus.emplaceListener<event>(&m## event)
#define EVENT_REGISTER_PRIORITY(event, priority) bus.emplaceListener<event>(&m## event, EventPriority::##priority)
#define AUTO_TI_HOOK(name, type, method, ...) LL_VA_EXPAND(LL_TYPE_INSTANCE_HOOK(name##Hook, HookPriority::Normal, type, &type::method, __VA_ARGS__))

namespace LiteNPC {
    EVENT_FUNCTION(PlayerJoinEvent) {
        NPC::spawnAll(&ev.self());
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

    void registerEvents() {
        auto &bus = EventBus::getInstance();
        EVENT_REGISTER(PlayerJoinEvent);
        SendEmotionHook::hook();
        OnUseNPCHook::hook();
        PlayerChangeDimensionHook::hook();
        TickHook::hook();
    }
} // namespace OreShop
