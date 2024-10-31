#include "mod/LiteNPCMod.h"

#include <memory>
#include <model/NPC.h>

#include "ll/api/mod/RegisterHelper.h"
#include "Global.h"
#include "mc/network/packet/TextPacket.h"
#include "mc/resources/ResourcePackRepository.h"
#include "mc/resources/ResourcePack.h"


namespace LiteNPC {
void registerEvents();
void registerCommands();
void registerExports();

static std::unique_ptr<LiteNPCMod> instance;
static  std::unique_ptr<ll::data::KeyValueDB> db;

LiteNPCMod& LiteNPCMod::getInstance() { return *instance; }
ll::data::KeyValueDB &LiteNPCMod::getDB() { return *db; }
EmotionsConfig emotionsConfig;

bool LiteNPCMod::load() {
    db = std::make_unique<ll::data::KeyValueDB>(NATIVE_MOD.getDataDir());
    ll::config::loadConfig(emotionsConfig, NATIVE_MOD.getConfigDir() / "emotions.json");
    registerEvents();
    registerExports();
    Util::loadSkinPacks();
    return true;
}

bool LiteNPCMod::enable() {
    registerCommands();
    NPC::init();
    return true;
}

bool LiteNPCMod::disable() {
    return true;
}

} // namespace LiteNPCMod

LL_REGISTER_MOD(LiteNPC::LiteNPCMod, LiteNPC::instance);
