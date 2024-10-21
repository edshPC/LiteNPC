#pragma once

#include "Global.h"
#include "mc/math/Vec3.h"
#include "mc/math/Vec2.h"
#include "mc/world/actor/player/SerializedSkin.h"
#include "mc/world/ActorRuntimeID.h"

using namespace std;

namespace LiteNPC {

	class NPC {
	public:
		NPC(string name, Vec3 pos, int dim, Vec2 rot, string skin, function<void(Player*)> cb) :
			name(name),
			pos(pos + Vec3(0.5f, 0.0f, 0.5f)),
			dim(dim),
			rot(rot),
			skinName(skin),
			actorId(LEVEL->getNewUniqueID()),
			runtimeId(LEVEL->getNextRuntimeID()),
			callback(cb)
			{}

		void onUse(Player*);
		void spawn(Player*);
		void updateSkin(Player*);
		void emote(string emoteName);
		void moveTo(Vec3 pos, float speed = 1);
		void remove();
		void setCallback(function<void(Player*)>);

		unsigned long long getRId() { return runtimeId; }

		void setName(string);
		void setSkin(string);

		static NPC* create(string name, Vec3 pos, int dim = 0, Vec2 rot = {}, string skin = {}, function<void(Player*)> cb = {});
		static void spawnAll(Player* pl);
		static NPC* getByRId(unsigned long long rId);
		static void saveSkin(string name, SerializedSkin&);
		static void saveEmotion(string name, string emotionUuid);
		static void init();

	private:
		string name;
		Vec3 pos;
		Vec2 rot;
		int dim;
		string skinName;
		const ActorUniqueID actorId;
		const ActorRuntimeID runtimeId;
		const mce::UUID uuid;
		function<void(Player* pl)> callback;
	};

}
