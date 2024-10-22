#include "Global.h"

#include "RemoteCallAPI.h"

const string NAMESPACE = "LiteNPC";

using namespace RemoteCall;

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

		exportAs(NAMESPACE, "emote", [](int64 rId, std::string const& name) {
			NPC* npc = NPC::getByRId(rId);
			if (npc) npc->emote(name);
		});

		exportAs(NAMESPACE, "moveTo", [](
		int64 rId,
		WorldPosType const& pos,
		float speed) {
			NPC* npc = NPC::getByRId(rId);
			if (npc) npc->moveTo(pos.pos, speed);
		});

		exportAs(NAMESPACE, "moveToBlock", [](
		int64 rId,
		BlockPosType const& pos,
		float speed) {
			NPC* npc = NPC::getByRId(rId);
			if (npc) npc->moveTo(pos.pos, speed);
		});

		exportAs(NAMESPACE, "lookAt", [](int64 rId, WorldPosType const& pos) {
			NPC* npc = NPC::getByRId(rId);
			if (npc) npc->lookAt(pos.pos);
		});

		exportAs(NAMESPACE, "swing", [](int64 rId) {
			NPC* npc = NPC::getByRId(rId);
			if (npc) npc->swing();
		});

		exportAs(NAMESPACE, "interactBlock", [](int64 rId, BlockPosType const& pos) {
			NPC* npc = NPC::getByRId(rId);
			if (npc) npc->interactBlock(pos.pos);
		});

		exportAs(NAMESPACE, "say", [](int64 rId, string const& text) {
			NPC* npc = NPC::getByRId(rId);
			if (npc) npc->say(text);
		});

		exportAs(NAMESPACE, "delay", [](int64 rId, int64 delay) {
			NPC* npc = NPC::getByRId(rId);
			if (npc) npc->delay(delay);
		});

	}

}
