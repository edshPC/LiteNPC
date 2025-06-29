#pragma once

#include <mc/network/ServerNetworkHandler.h>
#include <mc/network/packet/Packet.h>
#include <mc/world/actor/ActorFlags.h>
#include <mc/world/actor/DataItem.h>
#include <mc/world/actor/player/SerializedSkin.h>
#include <mc/legacy/ActorRuntimeID.h>
#include <mc/legacy/ActorUniqueID.h>
#include <mc/world/actor/player/Player.h>

#ifdef LITENPC_EXPORTS
#define LNAPI __declspec(dllexport)
#else
#define LNAPI __declspec(dllimport)
#endif

namespace LiteNPC {

	using namespace std;

	class Entity {
	protected:
		string name;
		Vec3 pos;
		Vec2 rot;
		int dim;
		const ActorUniqueID actorId;
		const ActorRuntimeID runtimeId;
		float size = 1;
		bool showNametag = true;
		bool enabled = true;
		std::unordered_set<ActorFlags> flags;
	public:
		Entity(const string& name, Vec3 pos, int dim, Vec2 rot);
		virtual ~Entity();

		virtual void spawn(Player* pl) = 0;

		virtual void tick(uint64 tick);
		virtual void updateActorData();
		virtual void remove();

		void despawn(Player* pl = nullptr);
		void putActorData(vector<unique_ptr<DataItem>>& data);

		LNAPI void disable();
		LNAPI void enable();
		LNAPI void rename(const string &name);
		LNAPI void resize(float size);
		LNAPI void setShowNametag(bool showNametag);

		uint64 getRId() { return runtimeId; }

		LNAPI static void spawnAll(Player* pl);
		static void tickAll(uint64 tick);
		static void loadEntity(Entity* entity);
	};

	struct Action {
		unique_ptr<Packet> pkt;
		function<void()> cb;
	};

	class NPC : public Entity {
		string skinName;
		const mce::UUID uuid;
		function<void(Player* pl)> callback;
		std::map<uint64, Action> actions;
		uint64 freeTick;
		ItemStack hand;
		bool isSitting;
		struct Minecart {
			ActorUniqueID actorId;
			ActorRuntimeID runtimeId;
		} minecart;

	public:
		LNAPI NPC(const string& name, Vec3 pos, int dim, Vec2 rot, const string& skin, const function<void(Player*)>& cb);
		~NPC() override;

		void spawn(Player* pl) override;
		void tick(uint64 tick) override;
		void updateActorData() override;

		void onUse(Player* pl);
		void updateSkin(Player* pl = nullptr);
		void updatePosition();
		LNAPI void remove(bool instant);
		LNAPI void remove() override;
		LNAPI void newAction(unique_ptr<Packet> pkt, uint64 delay = 1, function<void()> cb = {});
		LNAPI void setCallback(function<void(Player*)> cb);

		LNAPI void setSkin(const string &skinName);
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
		LNAPI void sayTo(Player *pl, const string &text);
		LNAPI void delay(uint64 ticks);
		LNAPI void sit(bool setSitting = true);
		LNAPI void sleep(bool setSleeping = true);
		LNAPI void sneak(bool setSneaking = true);
		LNAPI void finishDialogue();
		LNAPI void playSound(const string& name, float volume = 1, float pitch = 1);
		LNAPI void stop();

		LNAPI static NPC* create(string name, Vec3 pos, int dim = 0, Vec2 rot = {}, string skin = {}, function<void(Player*)> cb = {});
		LNAPI static NPC* getByRId(unsigned long long rId);
		LNAPI static vector<NPC*> getAll();
		LNAPI static unordered_map<string, SerializedSkin>& getLoadedSkins();
		LNAPI static void saveSkin(string name, SerializedSkin& skin);
		LNAPI static void saveEmotion(string name, string emotionUuid);
		LNAPI static void openDialogueHistory(Player* pl);
		static void init();
		static void updateDialogue();
	};

	class FloatingText : public Entity {
	public:
		FloatingText(const string& text, Vec3 pos, int dim);

		void spawn(Player* pl) override;

		LNAPI static FloatingText* create(string text, Vec3 pos, int dim = 0);
	};

}
