#include "NPC.h"

#include <llapi/SendPacketAPI.h>
#include <llapi/utils/Bstream.h>
#include <llapi/mc/MinecraftPackets.hpp>
#include <llapi/mc/DataItem.hpp>
#include <llapi/mc/Player.hpp>

extern Logger logger;

namespace LiteNPC {
	unordered_map<unsigned long long, NPC*> loadedNPC;

	void NPC::remove() {
		loadedNPC.erase(this->runtimeId);

		BinaryStream bs;
		std::shared_ptr<Packet> pkt;
		pkt = MinecraftPackets::createPacket(MinecraftPacketIds::RemoveActor);
		bs.writeUnsignedVarInt64(ZigZag(this->actorId));
		pkt->read(bs);
		Level::sendPacketForAllPlayers(*pkt);
	}

	void NPC::spawn(Player* pl) {
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
	}

	void NPC::onUse(Player* pl) {
		callback(pl);
	}

	void NPC::setCallback(function<void(Player* pl)> callback) {
		this->callback = callback;
	}

	NPC* NPC::create(string name, Vec3 pos, int dim, Vec2 rot, string skin) {
		NPC* npc = new NPC(name, pos, dim, rot, skin);
		loadedNPC[npc->runtimeId] = npc;
		for (auto pl : Level::getAllPlayers()) npc->spawn(pl);
		return npc;
	}

	void NPC::spawnAll(Player* pl) {
		for (auto entry : loadedNPC) {
			entry.second->spawn(pl);
		}
	}

	NPC* NPC::getByRId(unsigned long long rId) {
		try {
			return loadedNPC.at(rId);
		} catch (...) {
			return nullptr;
		}
	}
}

