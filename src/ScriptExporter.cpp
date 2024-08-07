#include "ScriptExporter.h"
#include "NPC.h"

#include <llapi/RemoteCallAPI.h>

const string NAMESPACE = "LiteNPC";

namespace LiteNPC {
	unordered_map<string, unordered_set<NPC*>> externalNPC;

	void registerExports() {
		RemoteCall::exportAs(NAMESPACE, "load", [](
			std::string const& plugin,
			std::string const& name,
			RemoteCall::WorldPosType const& wpos,
			float rotX, float rotY,
			std::string const& skin) -> __int64 {
			NPC* npc = NPC::create(name, wpos.pos, wpos.dimId, Vec2(rotX, rotY), skin);
			externalNPC[plugin].insert(npc);
			return npc->getRId();
		});

		RemoteCall::exportAs(NAMESPACE, "reg_callback", [](
			__int64 rId,
			std::string const& name) {
			NPC* npc = NPC::getByRId(rId);
			if (npc && RemoteCall::hasFunc(NAMESPACE, name))
				npc->setCallback(RemoteCall::importAs<void(Player*)>(NAMESPACE, name));
		});

		RemoteCall::exportAs(NAMESPACE, "clear", [](
			std::string const& plugin) {
			for (NPC* npc : externalNPC[plugin]) npc->remove();
			externalNPC[plugin].clear();
		});

	}

	const std::string JS_API = R"CODE(const NAMESPACE = "LiteNPC";

export default class LiteNPC {
	static INSTANCE;
	plugin = "default";
	running = false;
	queue = [];

	constructor() {
		if (LiteNPC.INSTANCE) return LiteNPC.INSTANCE;
		LiteNPC.INSTANCE = this
	}

	regNPC(name, pos, rot, skin, callback) {
		let rId = this.load(this.plugin, name, pos, rot.pitch, rot.yaw, skin);
		if(callback) {
			let func_name = `NPC_${rId}`;
			ll.exports(callback, NAMESPACE, func_name);
			this.reg_callback(rId, func_name);
		}
	}

	init() {
		this.load = ll.imports(NAMESPACE, "load");
		this.clear = ll.imports(NAMESPACE, "clear");
		this.reg_callback = ll.imports(NAMESPACE, "reg_callback");
		this.running = true;
		this.clear(this.plugin);
		this.queue.forEach(args => this.regNPC(...args));
	}

	static create(...args) {
		let ins = new LiteNPC();
		if (!ins.running) return ins.queue.push(args);
		ins.regNPC(...args);
	}

	static plugin(plugin) {
		new LiteNPC().plugin = plugin;
	}
}

mc.listen("onServerStarted", () => new LiteNPC().init());
)CODE";

}
