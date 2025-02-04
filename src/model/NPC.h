#pragma once

#include <mc/network/ServerNetworkHandler.h>
#include <mc/network/packet/Packet.h>
#include <mc/world/actor/ActorFlags.h>

#include "Global.h"
#include "mc/world/actor/player/SerializedSkin.h"

#ifdef LITENPC_EXPORTS
#define LNAPI __declspec(dllexport)
#else
#define LNAPI __declspec(dllimport)
#endif

using namespace std;

namespace LiteNPC {
	struct Action {
		unique_ptr<Packet> pkt;
		function<void()> cb;
	};
	class NPC {
	public:
		NPC(string name, Vec3 pos, int dim, Vec2 rot, string skin, function<void(Player*)> cb);

		void onUse(Player* pl);
		void spawn(Player* pl);
		void updateSkin(Player* pl = nullptr);
		void updatePosition();
		void updateActorData();
		LNAPI void remove(bool instant = false);
		void newAction(unique_ptr<Packet> pkt, uint64 delay = 1, function<void()> cb = {});
		void tick(uint64 tick);
		LNAPI void setCallback(function<void(Player*)> cb);

		LNAPI void rename(string name);
		LNAPI void resize(float size);
		LNAPI void setSkin(string skinName);
		LNAPI void setHand(const ItemStack &item);
		LNAPI void emote(string emoteName);
		LNAPI void moveTo(Vec3 pos, float speed = 1);
		LNAPI void moveTo(BlockPos pos, float speed = 1);
		LNAPI void moveToBlock(BlockPos pos, float speed = 1);
		LNAPI void lookAt(Vec3 pos);
		LNAPI void lookRot(Vec2 dest);
		LNAPI void lookRot(float rotX, float rotY);
		LNAPI void swing();
		LNAPI void eat(int64 time = 7);
		LNAPI void interactBlock(BlockPos bp);
		LNAPI void say(const string &text, bool saveHistory = true);
		LNAPI void delay(uint64 ticks);
		LNAPI void sit(bool setSitting = true);
		LNAPI void sleep(bool setSleeping = true);
		LNAPI void sneak(bool setSneaking = true);
		LNAPI void finishDialogue();
		LNAPI void playSound(const string& name, float volume = 1, float pitch = 1);
		LNAPI void stop();

		LNAPI static NPC* create(string name, Vec3 pos, int dim = 0, Vec2 rot = {}, string skin = {}, function<void(Player*)> cb = {});
		static void spawnAll(Player* pl);
		static NPC* getByRId(unsigned long long rId);
		LNAPI static vector<NPC*> getAll();
		static unordered_map<string, SerializedSkin>& getLoadedSkins();
		static void saveSkin(string name, SerializedSkin& skin);
		static void saveEmotion(string name, string emotionUuid);
		static void init();
		static void tickAll(uint64 tick);
		static void updateDialogue();
		LNAPI static void openDialogueHistory(Player* pl);

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
		std::unordered_set<ActorFlags> flags;
		uint64 freeTick = 0;
		ItemStack hand = ItemStack::EMPTY_ITEM();
		bool isSitting = false;
		struct Minecart {
			ActorUniqueID actorId = LEVEL->getNewUniqueID();
			ActorRuntimeID runtimeId = LEVEL->getNextRuntimeID();
		} minecart;
	};

}
