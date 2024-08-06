const NAMESPACE = "LiteNPC";

export default class LiteNPC {
	static INSTANCE;
	running = false;
	queue = [];

	constructor() {
		if (LiteNPC.INSTANCE) return LiteNPC.INSTANCE;
		LiteNPC.INSTANCE = this
	}

	regNPC(name, pos, rot, skin, callback) {
		let rId = this.load(name, pos, rot.pitch, rot.yaw, skin);
		if(callback) {
			let func_name = `NPC_${rId}`;
			ll.exports(callback, NAMESPACE, func_name);
			this.reg_callback(rId, func_name);
		}
	}

	init() {
		this.load = ll.imports(NAMESPACE, "load");
		this.clear = ll.imports(NAMESPACE, "clear");
		this.clear();
		this.reg_callback = ll.imports(NAMESPACE, "reg_callback");
		this.running = true;
		this.queue.forEach(args => this.regNPC(...args));
		log("LiteNPC Loaded");
	}

	static create(...args) {
		let ins = new LiteNPC();
		if (!ins.running) return ins.queue.push(args);
		ins.regNPC(...args);
	}
}

mc.listen("onServerStarted", () => new LiteNPC().init());
