const NAMESPACE = "LiteNPC";

let plugin = "default", serverStarted = false, queue = [];

const API = {}, API_funcs = ["create", "clear", "setCallback", "emote", "moveTo", "moveToBlock", "lookAt", "swing", "interactBlock"];
API_funcs.forEach(f => API[f] = ll.imports(NAMESPACE, f));

export default class LiteNPC {
	id = 0;
	constructor(args) { this.args = args; }

	load() {
		let [name, pos, rot, skin, callback] = this.args;
		this.id = API.create(plugin, name, pos, rot.pitch, rot.yaw, skin);
		if(callback) this.setCallback(callback);
	}

	setCallback(callback) {
		let func_name = `NPC_${this.id}`;
		ll.exports(callback, NAMESPACE, func_name);
		API.setCallback(this.id);
	}

	moveTo(pos, speed = 1) {
		if (pos instanceof IntPos) API.moveToBlock(this.id, pos, speed);
		else API.moveTo(this.id, pos, speed);
	}

	emote(name) { API.emote(this.id, name);	}
	lookAt(pos) { API.lookAt(this.id, pos);	}
	swing() { API.swing(this.id); }
	interactBlock(pos) { API.interactBlock(this.id, pos); }

	static create(...args) {
		let npc = new LiteNPC(args);
		if (serverStarted) npc.load();
		else queue.push(npc);
		return npc;
	}

	static plugin(p) { plugin = p; }
}

mc.listen("onServerStarted", () => {
	API.clear(plugin);
	serverStarted = true;
	queue.forEach(npc => npc.load());
});
