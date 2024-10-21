#include "Global.h"

#include <ll/api/command/CommandHandle.h>
#include <ll/api/command/CommandRegistrar.h>
#include <ll/api/command/runtime/ParamKind.h>
#include <mc/world/level/Command.h>

#define EXECUTE_CMD(name)                                                                                              \
    void execute##name(const CommandOrigin& ori, CommandOutput& out, const name##Param& param)
#define EXECUTE_NO_PARAM(name) void execute##name(const CommandOrigin& ori, CommandOutput& out)

using namespace ll::command;

namespace LiteNPC {

struct SaveSkinParam {
    string name;
};

EXECUTE_CMD(SaveSkin) {
    if (ori.getOriginType() != CommandOriginType::Player) return out.error("Player only");
    auto pl = static_cast<Player*>(ori.getEntity());
    auto& skin = pl->getSkin();
    LiteNPC::NPC::saveSkin(param.name, skin);
    out.success("Skin saved successfully");
}

struct SaveAnimationParam {
    string name;
};

extern unordered_map<Player*, string> waitingEmotions;
EXECUTE_CMD(SaveAnimation) {
    if (ori.getOriginType() != CommandOriginType::Player) return out.error("Player only");
    auto pl = static_cast<Player*>(ori.getEntity());
    waitingEmotions[pl] = param.name;
    out.success("Waiting for emotion");
}

void registerCommands() {
    if (!ll::service::getCommandRegistry().has_value()) return;
    auto& registrar = CommandRegistrar::getInstance();

    auto& cmd = registrar.getOrCreateCommand("saveskin", "Save your skin for npc", CommandPermissionLevel::GameDirectors);
    cmd.overload<SaveSkinParam>().required("name").execute(&executeSaveSkin);

    auto& cmd1 = registrar.getOrCreateCommand("saveanimation", "Save your emotion for npc", CommandPermissionLevel::GameDirectors);
    cmd1.overload<SaveAnimationParam>().required("name").execute(&executeSaveAnimation);

}
} // namespace PlayerRegister
