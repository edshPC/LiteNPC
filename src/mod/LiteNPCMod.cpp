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


bool LiteNPCMod::load() {
    db = std::make_unique<ll::data::KeyValueDB>(NATIVE_MOD.getDataDir());
    registerEvents();
    return true;
}

bool LiteNPCMod::enable() {
    registerCommands();
    auto npc = NPC::create("test", Vec3(156, -3, 784), 0, Vec2(0, 90), "edsh");
    Util::setInterval([npc] { npc->emote("test"); }, 3000);
    return true;
}

bool LiteNPCMod::disable() {
    return true;
}

} // namespace LiteNPCMod

LL_REGISTER_MOD(LiteNPC::LiteNPCMod, LiteNPC::instance);
