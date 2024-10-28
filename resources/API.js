const NAMESPACE = "LiteNPC";

let plugin = "default", serverStarted = false, queue = [];

const API = {}, API_funcs = ["create", "clear", "setCallback", "emote", "moveTo", "moveToBlock",
	"lookAt", "swing", "interactBlock", "say", "delay", "setHand", "sit"];
API_funcs.forEach(f => API[f] = ll.imports(NAMESPACE, f));

function parsePositionArgs(args, Type = IntPos) {
	if (!(args[0] instanceof IntPos || args[0] instanceof FloatPos))
		args = [new Type(...args.slice(0, 3), 0), ...args.slice(3)];
	return args;
}

export default class LiteNPC {
	id = 0;
	cbId = 0;
	constructor(args) {
		this.name = args[0];
		this.args = args;
	}

	load() {
		let [name, pos, rot, skin, callback] = this.args;
		this.id = API.create(plugin, name, pos, rot.pitch, rot.yaw, skin);
		if(callback) this.setCallback(callback);
	}

	setCallback(callback) {
		ll.exports(pl => { if(callback) callback(pl); }, NAMESPACE, `NPC_${this.id}_${this.cbId}`);
		API.setCallback(this.id, this.cbId++);
	}

	waitCallback(callback) {
		return new Promise(resolve => this.setCallback(pl => { if (callback) callback(pl); resolve(); }));
	}

	moveTo(...args) {
		let [pos, speed] = parsePositionArgs(args);
		if (pos instanceof IntPos) API.moveToBlock(this.id, pos, speed || 1);
		else API.moveTo(this.id, pos, speed || 1);
	}

	emote(name) { API.emote(this.id, name);	}
	lookAt(...args) { API.lookAt(this.id, parsePositionArgs(args, FloatPos)[0]); }
	swing() { API.swing(this.id); }
	interactBlock(...args) { API.interactBlock(this.id, parsePositionArgs(args)[0]); }
	setHand(item) { API.setHand(this.id, item); }
	sit(setSitting = true) { API.sit(this.id, setSitting); }

	say(msg, instant, name = this.name) {
		msg = `§6[§f${name}§6] §f${msg}`;
		if (instant) mc.broadcast(msg);
		else API.say(this.id, msg);
	}

	delay(ticks) {
		API.delay(this.id, ticks);
	}

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
	log(`Plugin ${plugin} loaded`);
});
