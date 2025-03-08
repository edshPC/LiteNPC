const NAMESPACE = "LiteNPC";

let plugin = "default", serverStarted = false, queue = [], customPrefix = "", emptyItem;

const API = {}, API_funcs = ["create", "remove", "clear", "setCallback", "emote", "moveTo", "moveToBlock",
	"lookAt", "lookRot", "swing", "interactBlock", "say", "sayTo", "delay", "setHand", "setSkin", "sit", "rename", "resize", "eat",
	"finishDialogue", "openDialogueHistory", "stop", "playSound", "sendPlaySound", "sleep", "sneak"];
API_funcs.forEach(f => API[f] = ll.imports(NAMESPACE, f));

function parsePositionArgs(args, Type = IntPos) {
	if (!(args[0] instanceof IntPos || args[0] instanceof FloatPos))
		args = [new Type(...args.slice(0, 3), 0), ...args.slice(3)];
	return args;
}

export default class LiteNPC {
	id = 0;
	cbId = 0;
	prefix = "";
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
		ll.exports(pl => { if(callback) callback(pl, this); }, NAMESPACE, `NPC_${this.id}_${this.cbId}`);
		API.setCallback(this.id, this.cbId++);
		return this;
	}

	waitCallback(callback) {
		return new Promise(resolve => this.setCallback(pl => { if (callback) callback(pl, this); resolve(); }));
	}

	moveTo(...args) {
		let [pos, speed] = parsePositionArgs(args);
		if (pos instanceof IntPos) API.moveToBlock(this.id, pos, speed || 1);
		else API.moveTo(this.id, pos, speed || 1);
		return this;
	}

	remove() { API.remove(this.id); }
	emote(name) { API.emote(this.id, name); return this; }
	lookAt(...args) { API.lookAt(this.id, parsePositionArgs(args, FloatPos)[0]); return this; }
	lookRot(x, y) { API.lookRot(this.id, x, y); return this; }
	swing() { API.swing(this.id); return this; }
	interactBlock(...args) { API.interactBlock(this.id, parsePositionArgs(args)[0]); return this; }
	setHand(item) { API.setHand(this.id, item || emptyItem); return this; }
	setSkin(name) { API.setSkin(this.id, name); return this; }
	sit(setSitting = true) { API.sit(this.id, setSitting); return this; }
	sleep(setSleeping = true) { API.sleep(this.id, setSleeping); return this; }
	sneak(setSneaking = true) { API.sneak(this.id, setSneaking); return this; }
	rename(name) { API.rename(this.id, name); this.name = name; return this; }
	resize(size) { API.resize(this.id, size); return this; }
	eat(times = 30) { API.eat(this.id, times); return this; }
	setPrefix(prefix) { this.prefix = prefix; return this; }
	finishDialogue() { API.finishDialogue(this.id); return this; }
	stop() { API.stop(this.id); }

	say(msg, name, saveHistory = true) {
		msg = `${name ? customPrefix : this.prefix}§6[${name || this.name}§6] §f${msg}`;
		API.say(this.id, msg, saveHistory);
		if (saveHistory) this.last_msg = msg;
		return this;
	}
	sayTo(pl, msg, name) {
		msg = `${name ? customPrefix : this.prefix}§6[${name || this.name}§6] §f${msg}`;
		API.sayTo(this.id, pl, msg);
		return this;
	}

	decision(pl, choices, name = this.name) {
		let fm = mc.newSimpleForm();
		fm.setTitle(this.name);
		fm.setContent(this.last_msg);
		choices.forEach(choice => fm.addButton(choice || "§kAAAAAAAAAAAAAAAAAA"));
		return new Promise(resolve => {
			function callback(pl, id) {
				if (id === undefined || choices[id] === null) return pl.sendForm(fm, callback);
				resolve([id, choices[id].replace(/ ?\[.*]/g, "")]);
			}
			pl.sendForm(fm, callback);
		});
	}

	delay(ticks) {
		API.delay(this.id, ticks);
		return this;
	}

	playSound(name, volume = 1, pitch = 1) { API.playSound(name, volume, pitch); return this; }
	static sendPlaySound(...args) {
		let [pos, name, volume, pitch] = parsePositionArgs(args);
		API.sendPlaySound(pos, name, volume || 1, pitch || 1);
	}
	static setCustomPrefix(prefix) { customPrefix = prefix; }
	static openDialogueHistory(pl) { API.openDialogueHistory(pl); }

	static create(...args) {
		let npc = new LiteNPC(args);
		if (serverStarted) npc.load();
		else queue.push(npc);
		return npc;
	}

	static plugin(p) { plugin = p; }
}

mc.listen("onServerStarted", () => {
	if (plugin == "default") {
		const log_path = "logs/LiteNPC.log";
		File.writeTo(log_path, "");
		logger.setFile(log_path);
		logger.info("Loading...");
		logger.setFile(" ");
		let logs = File.readFrom(log_path);
		plugin = /\[.*?].*\[(.*?)]/g.exec(logs)[1];
	}
	logger.setTitle(NAMESPACE);
	logger.info(`Plugin ${plugin} loaded`);
	logger.setTitle(plugin);

	emptyItem = mc.newItem("minecraft:stone", 0);
	API.clear(plugin);
	serverStarted = true;
	queue.forEach(npc => npc.load());
});
