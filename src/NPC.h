#pragma once

#include <llapi/mc/Vec3.hpp>
#include <llapi/mc/Vec2.hpp>
#include <llapi/mc/Player.hpp>
#include <llapi/mc/Level.hpp>

using namespace std;

namespace LiteNPC {

	class NPC {
	public:
		NPC(string name, Vec3 pos, Vec2 rot, string skin) :
			name(name),
			pos(pos.add(0.5f, 0.0f, 0.5f)),
			rot(rot),
			skinName(skin),
			actorId(Global<Level>->getNewUniqueID()),
			runtimeId(Global<Level>->getNextRuntimeID()),
			uuid() {}

		void onUse(Player*);
		void spawn(Player*);

		static void create(string name, Vec3 pos, Vec2 rot, string skin);
		static void spawnAll(Player* pl);
		static NPC* getByRId(unsigned long long rId);

	private:
		string name;
		Vec3 pos;
		Vec2 rot;
		string skinName;
		const long long actorId;
		const unsigned long long runtimeId;
		const mce::UUID uuid;
	};
	
}
