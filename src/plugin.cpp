#include <llapi/LoggerAPI.h>
#include <llapi/EventAPI.h>
#include <llapi/mc/ServerPlayer.hpp>
#include <llapi/ScheduleAPI.h>

#include "version.h"
#include "NPC.h"

extern Logger logger;

void PluginEnable() {
	LiteNPC::NPC::create("test", Vec3(0, 0, 0), Vec2(0, 0), "");
}

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

	Schedule::nextTick(PluginEnable);
}

