#pragma once

#include <mc/network/ServerNetworkHandler.h>
#include <mc/network/packet/Packet.h>

#include "Global.h"
#include "mc/math/Vec3.h"
#include "mc/math/Vec2.h"
#include "mc/world/actor/player/SerializedSkin.h"
#include "mc/world/ActorRuntimeID.h"

using namespace std;

namespace LiteNPC {
	struct Action {
		unique_ptr<Packet> pkt;
		function<void()> cb;
	};
	class NPC {
	public:
		NPC(string name, Vec3 pos, int dim, Vec2 rot, string skin, function<void(Player*)> cb) :
			name(name),
			pos(pos),
			dim(dim),
			rot(rot),
			skinName(skin),
			callback(cb) {}

		void onUse(Player*);
		void spawn(Player*);
		void updateSkin(Player* = nullptr);
		void updatePosition();
		void updateActorData();
		void remove(bool instant = false);
		void newAction(unique_ptr<Packet> pkt, uint64 delay = 1, function<void()> cb = {});
		void tick(uint64 tick);
		void setCallback(function<void(Player*)>);

		void rename(string);
		void resize(float);
		void setSkin(string);
		void setHand(const ItemStack &item);
		void emote(string emoteName);
		void moveTo(Vec3 pos, float speed = 1);
		void moveTo(BlockPos pos, float speed = 1);
		void moveToBlock(BlockPos pos, float speed = 1);
		void lookAt(Vec3 pos);
		void lookRot(Vec2 dest);
		void lookRot(float rotX, float rotY);
		void swing();
		void interactBlock(BlockPos bp);
		void say(const string &text);
		void delay(uint64 ticks);
		void sit(bool setSitting = true);

		static NPC* create(string name, Vec3 pos, int dim = 0, Vec2 rot = {}, string skin = {}, function<void(Player*)> cb = {});
		static void spawnAll(Player* pl);
		static NPC* getByRId(unsigned long long rId);
		static vector<NPC*> getAll();
		static unordered_map<string, SerializedSkin>& getLoadedSkins();
		static void saveSkin(string name, SerializedSkin&);
		static void saveEmotion(string name, string emotionUuid);
		static void init();
		static void tickAll(uint64 tick);

		uint64 getRId() { return runtimeId; }

	private:
		string name;
		Vec3 pos;
		Vec2 rot;
		int dim;
		string skinName;
		float size = 1;
		const ActorUniqueID actorId = LEVEL->getNewUniqueID();
		const ActorRuntimeID runtimeId = LEVEL->getNextRuntimeID();
		const mce::UUID uuid = mce::UUID::random();
		function<void(Player* pl)> callback;
		std::map<uint64, Action> actions;
		uint64 freeTick = 0;
		ItemStack hand = ItemStack::EMPTY_ITEM;
		bool isSitting = false;
		struct Minecart {
			ActorUniqueID actorId = LEVEL->getNewUniqueID();
			ActorRuntimeID runtimeId = LEVEL->getNextRuntimeID();
		} minecart;
	};

}
