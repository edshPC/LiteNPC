#include <llapi/HookAPI.h>
#include <llapi/mc/PlayerSkinPacket.hpp>
#include <llapi/mc/SerializedSkin.hpp>
#include <llapi/mc/ServerPlayer.hpp>
#include <llapi/mc/Dimension.hpp>

TInstanceHook(void, "?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBVPlayerSkinPacket@@@Z",
    ServerNetworkHandler, NetworkIdentifier* id, PlayerSkinPacket* pkt) {

    auto& skin = dAccess<SerializedSkin, 64>(pkt);
    auto pl = pkt->getPlayerFromPacket(this, id);
    if (!pl) return;
    pl->updateSkin(skin, 1);
    pl->getDimension().sendPacketForPosition(pl->getBlockPos(), *pkt, pl);
    pl->sendNetworkPacket(*pkt);
    pl->sendText("§eСкин изменён");

}
