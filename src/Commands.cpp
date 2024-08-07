#include "Commands.h"
#include "NPC.h"

#include <llapi/mc/SerializedSkin.hpp>
#include <llapi/SendPacketAPI.h>
#include <llapi/utils/Bstream.h>

void SaveSkinCommand::execute(CommandOrigin const& ori, CommandOutput& output) const {
	ServerPlayer* sp = ori.getPlayer();
	if (!sp->isPlayer()) return output.error("Команда доступна только для игроков");

	auto& skin = sp->getSkin();
	LiteNPC::NPC::saveSkin(name, skin);

	output.success();
}
