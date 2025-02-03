const NAMESPACE = "LiteNPC";

let plugin = "default", serverStarted = false, queue = [], customPrefix = "";

const API = {}, API_funcs = ["create", "remove", "clear", "setCallback", "emote", "moveTo", "moveToBlock",
	"lookAt", "lookRot", "swing", "interactBlock", "say", "delay", "setHand", "setSkin", "sit", "rename", "resize", "eat",
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
	}

	waitCallback(callback) {
		return new Promise(resolve => this.setCallback(pl => { if (callback) callback(pl, this); resolve(); }));
	}

	moveTo(...args) {
		let [pos, speed] = parsePositionArgs(args);
		if (pos instanceof IntPos) API.moveToBlock(this.id, pos, speed || 1);
		else API.moveTo(this.id, pos, speed || 1);
	}

	remove() { API.remove(this.id); }
	emote(name) { API.emote(this.id, name);	}
	lookAt(...args) { API.lookAt(this.id, parsePositionArgs(args, FloatPos)[0]); }
	lookRot(x, y) { API.lookRot(this.id, x, y); }
	swing() { API.swing(this.id); }
	interactBlock(...args) { API.interactBlock(this.id, parsePositionArgs(args)[0]); }
	setHand(item) { API.setHand(this.id, item); }
	setSkin(name) { API.setSkin(this.id, name); }
	sit(setSitting = true) { API.sit(this.id, setSitting); }
	sleep(setSleeping = true) { API.sleep(this.id, setSleeping); }
	sneak(setSneaking = true) { API.sneak(this.id, setSneaking); }
	rename(name) { API.rename(this.id, name); this.name = name; }
	resize(size) { API.resize(this.id, size); }
	eat(times = 30) { API.eat(this.id, times); }
	setPrefix(prefix) { this.prefix = prefix; }
	finishDialogue() { API.finishDialogue(this.id); }
	stop() { API.stop(this.id); }

	say(msg, name, saveHistory = true) {
		msg = `${name ? customPrefix : this.prefix}§6[${name || this.name}§6] §f${msg}`;
		API.say(this.id, msg, saveHistory);
		if (saveHistory) this.last_msg = msg;
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
	}

	playSound(name, volume = 1, pitch = 1) { API.playSound(name, volume, pitch); }
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

	API.clear(plugin);
	serverStarted = true;
	queue.forEach(npc => npc.load());
});
