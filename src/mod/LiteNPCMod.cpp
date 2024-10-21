#include "mod/LiteNPCMod.h"

#include <memory>
#include <model/NPC.h>

#include "ll/api/mod/RegisterHelper.h"
#include "Global.h"

namespace LiteNPC {
void registerEvents();
void registerCommands();

static std::unique_ptr<LiteNPCMod> instance;
static  std::unique_ptr<ll::data::KeyValueDB> db;

LiteNPCMod& LiteNPCMod::getInstance() { return *instance; }
ll::data::KeyValueDB &LiteNPCMod::getDB() { return *db; }
EmotionsConfig emotionsConfig;

bool LiteNPCMod::load() {
    db = std::make_unique<ll::data::KeyValueDB>(NATIVE_MOD.getDataDir());
    ll::config::loadConfig(emotionsConfig, NATIVE_MOD.getConfigDir() / "emotions.json");
    registerEvents();
    return true;
}

bool LiteNPCMod::enable() {
    registerCommands();
    NPC::init();
    auto npc = NPC::create("motion", Vec3(152, -3, 770), 0, Vec2(0, 90), "motion");
    npc->setCallback([npc](Player * player) {
        npc->emote("Wave");
        player->sendMessage("Hi!");
    });
    Util::setInterval([npc] {
        npc->moveTo(Vec3(152, -3, 770));
        Util::setTimeout([npc] {
            npc->moveTo(Vec3(140, -3, 770));
        }, 5000);
    }, 10000);
    return true;
}

bool LiteNPCMod::disable() {
    return true;
}

} // namespace LiteNPCMod

LL_REGISTER_MOD(LiteNPC::LiteNPCMod, LiteNPC::instance);
