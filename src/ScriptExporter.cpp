#include "ScriptExporter.h"
#include "NPC.h"

#include <llapi/RemoteCallAPI.h>

const string NAMESPACE = "LiteNPC";

namespace LiteNPC {
	std::unordered_set<NPC*> external;

	void registerExports() {
		RemoteCall::exportAs(NAMESPACE, "load", [](
			std::string const& name,
			RemoteCall::WorldPosType const& wpos,
			float rotX, float rotY,
			std::string const& skin) -> __int64 {
			NPC* npc = NPC::create(name, wpos.pos, wpos.dimId, Vec2(rotX, rotY), skin);
			external.insert(npc);
			return npc->getRId();
		});

		RemoteCall::exportAs(NAMESPACE, "reg_callback", [](
			__int64 rId,
			std::string const& name) {
			NPC* npc = NPC::getByRId(rId);
			if (npc && RemoteCall::hasFunc(NAMESPACE, name))
				npc->setCallback(RemoteCall::importAs<void(Player*)>(NAMESPACE, name));
		});

		RemoteCall::exportAs(NAMESPACE, "clear", []() {
			for (NPC* npc : external) npc->remove();
			external.clear();
		});
	}

}
