#pragma once

#include <llapi/mc/Vec3.hpp>
#include <llapi/mc/Vec2.hpp>
#include <llapi/mc/Player.hpp>
#include <llapi/mc/Level.hpp>
#include <llapi/mc/SerializedSkin.hpp>

using namespace std;

namespace LiteNPC {

	class NPC {
	public:
		NPC(string name, Vec3 pos, int dim, Vec2 rot, string skin, function<void(Player* pl)> callback = [](Player* pl) {}) :
			name(name),
			pos(pos.add(0.5f, 0.0f, 0.5f)),
			dim(dim),
			rot(rot),
			skinName(skin),
			actorId(Global<Level>->getNewUniqueID()),
			runtimeId(Global<Level>->getNextRuntimeID()),
			callback(callback)
			{}

		void onUse(Player*);
		void spawn(Player*);
		void updateSkin(Player*);
		void remove();
		void setCallback(function<void(Player* pl)>);

		unsigned long long getRId() {
			return runtimeId;
		}

		static NPC* create(string name, Vec3 pos, int dim, Vec2 rot, string skin);
		static void spawnAll(Player* pl);
		static NPC* getByRId(unsigned long long rId);
		static void saveSkin(string name, SerializedSkin&);
		static void init();

	private:
		string name;
		Vec3 pos;
		Vec2 rot;
		int dim;
		string skinName;
		const long long actorId;
		const unsigned long long runtimeId;
		const mce::UUID uuid;
		function<void(Player* pl)> callback;
	};
	
}
