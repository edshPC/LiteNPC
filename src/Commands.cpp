#include "Commands.h"
#include "NPC.h"

#include <llapi/mc/SerializedSkin.hpp>
#include <llapi/SendPacketAPI.h>
#include <llapi/utils/Bstream.h>

void SaveSkinCommand::execute(CommandOrigin const& ori, CommandOutput& output) const {
	ServerPlayer* sp = ori.getPlayer();
	if (!sp->isPlayer()) return output.error("Команда доступна только для игроков");

	auto skin = sp->getSkin();

	skin.mId = "Custom" + mce::UUID().asString();
	skin.mFullId = skin.mId + mce::UUID().asString();
	skin.mCapeId = mce::UUID().asString();
	std::stringstream stream;
	stream << std::hex << RNG::rand<uint64_t>();
	skin.mPlayFabId = stream.str();
	skin.setIsTrustedSkin(true);

	LiteNPC::NPC::saveSkin(name, skin);

	output.success();
}
