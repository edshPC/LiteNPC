#include "mod/LiteNPCMod.h"

#include <memory>
#include <model/NPC.h>

#include "ll/api/mod/RegisterHelper.h"
#include "Global.h"
#include "mc/network/packet/TextPacket.h"

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
    return true;
}

bool LiteNPCMod::enable() {
    registerCommands();
    NPC::init();
    // auto npc = NPC::create("motion", Vec3(152, -3, 770), 0, Vec2(0, 90), "motion");
    // npc->setCallback([npc](Player* player) {
    //     npc->emote("Wave");
    //     npc->lookAt(player->getPosition());
    //     npc->moveTo(BlockPos(152, -3, 770));
    //     npc->interactBlock(BlockPos(153, -3, 770));
    //     npc->moveTo(BlockPos(140, -3, 760));
    // });
    // Util::setInterval([npc] {
    //     npc->interactBlock(BlockPos(153, -3, 770));
    // }, 1000);
    return true;
}

bool LiteNPCMod::disable() {
    return true;
}

} // namespace LiteNPCMod

LL_REGISTER_MOD(LiteNPC::LiteNPCMod, LiteNPC::instance);
