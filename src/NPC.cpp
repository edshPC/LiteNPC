#include "NPC.h"
#include "Database.h"

#include <llapi/SendPacketAPI.h>
#include <llapi/ScheduleAPI.h>
#include <llapi/utils/Bstream.h>
#include <llapi/mc/MinecraftPackets.hpp>
#include <llapi/mc/DataItem.hpp>

extern Logger logger;

namespace LiteNPC {
	unordered_map<unsigned long long, NPC*> loadedNPC;
	unordered_map<string, SerializedSkin> loadedSkins;

	void NPC::remove() {
		loadedNPC.erase(runtimeId);

		BinaryStream bs;
		std::shared_ptr<Packet> pkt;
		pkt = MinecraftPackets::createPacket(MinecraftPacketIds::RemoveActor);
		bs.writeUnsignedVarInt64(ZigZag(actorId));
		pkt->read(bs);
		Level::sendPacketForAllPlayers(*pkt);
	}

	void NPC::spawn(Player* pl) {
		if ((int)pl->getDimensionId() != dim) return;

		BinaryStream bs;
		bs.writeType(uuid); //uuid
		bs.writeString(name); //nick
		bs.writeUnsignedVarInt64(runtimeId); //rId
		bs.writeString("-"); //platformChatId
		bs.writeType(pos); //pos
		bs.writeType(Vec3(0, 0, 0)); //motion
		bs.writeFloat(rot.x); //pitch
		bs.writeFloat(rot.y); //yaw
		bs.writeFloat(rot.y); //head
		bs.writeBool(false); //item
		bs.writeVarInt(1); //gamemode
		vector<std::unique_ptr<DataItem>> items;
		bs.writeType(items); //meta
		bs.writeUnsignedVarInt(0); //syncedProperties1
		bs.writeUnsignedVarInt(0); //syncedProperties2
		bs.writeSignedInt64(actorId); //uId
		bs.writeByte(0); //playerPerm
		bs.writeByte(1); //commandPerm
		bs.writeByte(0); //abilities
		bs.writeUnsignedVarInt(0); //links
		bs.writeString("-"); //device
		bs.writeUnsignedInt(0); //platform

		NetworkPacket<12> pkt(bs.getAndReleaseData());
		pl->sendNetworkPacket(pkt);

		Schedule::nextTick([this, pl]() { this->updateSkin(pl); });
	}

	void NPC::updateSkin(Player* pl) {
		if (!loadedSkins.contains(skinName)) return;
		auto skin = loadedSkins.at(skinName);

		BinaryStream bs;
		bs.writeType(uuid);
		skin.write(bs);
		bs.writeString(skinName);
		bs.writeString("steve");
		bs.writeBool(true);

		NetworkPacket<93> pkt(bs.getAndReleaseData());
		pl->sendNetworkPacket(pkt);
	}

	void NPC::onUse(Player* pl) {
		callback(pl);
	}

	void NPC::setCallback(function<void(Player* pl)> callback) {
		this->callback = callback;
	}

	NPC* NPC::create(string name, Vec3 pos, int dim, Vec2 rot, string skin, function<void(Player* pl)> callback) {
		NPC* npc = new NPC(name, pos, dim, rot, skin, callback);
		loadedNPC[npc->runtimeId] = npc;
		for (auto pl : Level::getAllPlayers()) npc->spawn(pl);
		return npc;
	}

	void NPC::spawnAll(Player* pl) {
		for (auto entry : loadedNPC) {
			entry.second->spawn(pl);
		}
	}

	void NPC::setName(string name) {
		this->name = name;

		BinaryStream bs;
		bs.writeUnsignedVarInt64(runtimeId);
		vector<std::unique_ptr<DataItem>> items;
		auto nametag = DataItem::create(ActorDataIDs::NAMETAG, name); //text
		items.push_back(move(nametag));
		bs.writeType(items); //meta
		bs.writeUnsignedVarInt(0); //syncedProperties1
		bs.writeUnsignedVarInt(0); //syncedProperties2
		bs.writeUnsignedVarInt64(0); //tick?
		std::shared_ptr<Packet> pkt = MinecraftPackets::createPacket(MinecraftPacketIds::SetActorData);
		pkt->read(bs);
		Level::sendPacketForAllPlayers(*pkt);
	}

	void NPC::setSkin(string skin) {
		this->skinName = skin;
		for (auto pl : Level::getAllPlayers()) updateSkin(pl);
	}

	NPC* NPC::getByRId(unsigned long long rId) {
		try {
			return loadedNPC.at(rId);
		} catch (...) {
			return nullptr;
		}
	}

	void NPC::saveSkin(string name, SerializedSkin& skin) {
		loadedSkins[name] = skin;
		auto& newSkin = loadedSkins.at(name);

		newSkin.mId = "Custom" + mce::UUID().asString();
		newSkin.mFullId = newSkin.mId + mce::UUID().asString();
		newSkin.mCapeId = mce::UUID().asString();
		std::stringstream stream;
		stream << std::hex << RNG::rand<uint64_t>();
		newSkin.mPlayFabId = stream.str();
		newSkin.setIsTrustedSkin(true);

		BinaryStream bs;
		newSkin.write(bs);
		skinDB->set(name, bs.getAndReleaseData());
	}

	void NPC::init() {
		skinDB->iter([](string_view k, string_view v) -> bool {
			string name(k), data(v);
			loadedSkins[name] = SerializedSkin::createTrustedDefaultSerializedSkin();
			SerializedSkin& skin = loadedSkins.at(name);
			BinaryStream bs(data, true);
			skin.read(bs);
			return true;
		});
	}

}

