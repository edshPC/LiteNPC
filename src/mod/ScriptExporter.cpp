#include "Global.h"

#include "RemoteCallAPI.h"

const string NAMESPACE = "LiteNPC";

using namespace RemoteCall;

#define EXPORT_NO_ARGS(method) exportAs(NAMESPACE, #method, [](int64 rId) { \
	if (auto npc = NPC::getByRId(rId)) npc->method(); })

#define EXPORT_ONE_ARG(method, arg, unwrap) exportAs(NAMESPACE, #method, [](int64 rId, arg) { \
	if (auto npc = NPC::getByRId(rId)) npc->method(unwrap); })

#define EXPORT_TWO_ARGS(method, arg1, unwrap1, arg2, unwrap2) \
	exportAs(NAMESPACE, #method, [](int64 rId, arg1, arg2) { \
	if (auto npc = NPC::getByRId(rId)) npc->method(unwrap1, unwrap2); })

namespace LiteNPC {
	unordered_map<string, unordered_set<NPC*>> externalNPC;

	void registerExports() {
		exportAs(NAMESPACE, "create", [](
			std::string const& plugin,
			std::string const& name,
			WorldPosType const& wpos,
			float rotX, float rotY,
			std::string const& skin) -> int64 {
			NPC* npc = NPC::create(name, wpos.pos, wpos.dimId, Vec2(rotX, rotY), skin);
			externalNPC[plugin].insert(npc);
			return npc->getRId();
		});

		exportAs(NAMESPACE, "clear", [](std::string const& plugin) {
			for (NPC* npc : externalNPC[plugin]) npc->remove();
			externalNPC[plugin].clear();
		});

		// NPC functions
		exportAs(NAMESPACE, "setCallback", [](int64 rId, int64 cbId) {
			NPC* npc = NPC::getByRId(rId);
			string func = std::format("NPC_{}_{}", rId, cbId);
			if (npc && hasFunc(NAMESPACE, func)) {
				npc->setCallback(importAs<void(Player*)>(NAMESPACE, func));
			}
			if (cbId) removeFunc(NAMESPACE, std::format("NPC_{}_{}", rId, cbId - 1));
		});

		EXPORT_ONE_ARG(emote, string const& name, name);
		EXPORT_TWO_ARGS(moveTo, WorldPosType const& pos, pos.pos, float speed, speed);
		EXPORT_TWO_ARGS(moveToBlock, BlockPosType const& pos, pos.pos, float speed, speed);
		EXPORT_ONE_ARG(lookAt, WorldPosType const& pos, pos.pos);
		EXPORT_TWO_ARGS(lookRot, float x, x, float y, y);
		EXPORT_NO_ARGS(swing);
		EXPORT_ONE_ARG(interactBlock, BlockPosType const& pos, pos.pos);
		EXPORT_ONE_ARG(say, string const& text, text);
		EXPORT_ONE_ARG(delay, int64 delay, delay);
		EXPORT_ONE_ARG(setHand, ItemType const& item, *item.ptr);
		EXPORT_ONE_ARG(sit, bool setSitting, setSitting);

	}

}
