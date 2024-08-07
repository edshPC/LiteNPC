#include <llapi/LoggerAPI.h>
#include <llapi/EventAPI.h>
#include <llapi/ScheduleAPI.h>
#include <llapi/mc/ServerPlayer.hpp>

#include "version.h"
#include "NPC.h"
#include "ScriptExporter.h"
#include "Commands.h"

extern Logger logger;

void PluginInit() {
	Event::PlayerInteractEntityEvent::subscribe([](const Event::PlayerInteractEntityEvent& ev) {
		auto npc = LiteNPC::NPC::getByRId(ev.mTargetId);
		if (npc && ev.mPlayer->isPlayer()) npc->onUse((Player*)ev.mPlayer);
		return true;
	});

	Event::PlayerJoinEvent::subscribe([](const Event::PlayerJoinEvent& ev) {
		LiteNPC::NPC::spawnAll(ev.mPlayer);
		return true;
	});

	Event::PlayerChangeDimEvent::subscribe([](const Event::PlayerChangeDimEvent& ev) {
		Schedule::nextTick([ev]() { LiteNPC::NPC::spawnAll(ev.mPlayer); });
		return true;
	});

	Event::RegCmdEvent::subscribe([](const Event::RegCmdEvent& ev) {
		SaveSkinCommand::setup(ev.mCommandRegistry);
		return true;
	});

	LiteNPC::registerExports();
	LiteNPC::NPC::init();

	std::ofstream file("plugins/LiteNPC/API.js");
	file << LiteNPC::JS_API;
	file.close();
}

