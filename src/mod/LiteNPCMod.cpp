#include "mod/LiteNPCMod.h"

#include <memory>

#include "ll/api/mod/RegisterHelper.h"

namespace LiteNPC {

static std::unique_ptr<LiteNPCMod> instance;

LiteNPCMod& LiteNPCMod::getInstance() { return *instance; }

bool LiteNPCMod::load() {
    getSelf().getLogger().debug("Loading...");
    // Code for loading the mod goes here.
    return true;
}

bool LiteNPCMod::enable() {
    getSelf().getLogger().debug("Enabling...");
    // Code for enabling the mod goes here.
    return true;
}

bool LiteNPCMod::disable() {
    getSelf().getLogger().debug("Disabling...");
    // Code for disabling the mod goes here.
    return true;
}

} // namespace LiteNPCMod

LL_REGISTER_MOD(LiteNPC::LiteNPCMod, LiteNPC::instance);
