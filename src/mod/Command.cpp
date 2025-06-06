#include "Global.h"

#include <ll/api/command/CommandHandle.h>
#include <ll/api/command/CommandRegistrar.h>
#include <ll/api/command/runtime/ParamKind.h>
#include <mc/network/packet/EmotePacket.h>
#include <ll/api/command/Command.h>
#include <mc/server/commands/CommandOutput.h>

#define EXECUTE_CMD(name)                                                                                              \
    void execute##name(const CommandOrigin& ori, CommandOutput& out, const name##Param& param)
#define EXECUTE_NO_PARAM(name) void execute##name(const CommandOrigin& ori, CommandOutput& out)

using namespace ll::command;

namespace LiteNPC {

struct SaveSkinParam { string name; };
EXECUTE_CMD(SaveSkin) {
    if (ori.getOriginType() != CommandOriginType::Player) return out.error("Player only");
    auto pl = static_cast<Player*>(ori.getEntity());
    NPC::saveSkin(param.name, *pl->mSkin);
    pl->sendMessage("Skin saved");
    out.mSuccessCount++;
}

struct RemoveSkinParam { string name; };
EXECUTE_CMD(RemoveSkin) {
    if (ori.getOriginType() != CommandOriginType::Player) return out.error("Player only");
    auto pl = static_cast<Player*>(ori.getEntity());
    if (NPC::getLoadedSkins().erase(param.name)) pl->sendMessage("Skin unloaded");
    if (DB.has(param.name) && DB.del(param.name)) pl->sendMessage("Skin removed");
    out.mSuccessCount++;
}

struct SaveAnimationParam { string name; };
extern unordered_map<Player*, string> waitingEmotions;
EXECUTE_CMD(SaveAnimation) {
    if (ori.getOriginType() != CommandOriginType::Player) return out.error("Player only");
    auto pl = static_cast<Player*>(ori.getEntity());
    waitingEmotions[pl] = param.name;
    pl->sendMessage("Waiting for emotion");
    out.mSuccessCount++;
}

struct TestAnimationParam { CommandRawText name; };
EXECUTE_CMD(TestAnimation) {
    if (ori.getOriginType() != CommandOriginType::Player) return out.error("Player only");
    auto pl = static_cast<Player*>(ori.getEntity());
    if (!emotionsConfig.emotions.contains(param.name.mText)) return out.error("Invalid emotion");
    Vec3 off = pl->getHeadLookVector();
    off.y = 0;
    NPC* npc = NPC::create("testanimation", pl->getFeetPos() + off.normalize()*2,
        pl->getDimensionId(), Vec2(0, pl->getYBodyRotation() + 180));
    npc->emote(param.name.mText);
    Util::setTimeout([npc] { npc->remove(); }, 10000);
    out.mSuccessCount++;
}

void registerCommands() {
    if (!ll::service::getCommandRegistry().has_value()) return;
    auto& registrar = CommandRegistrar::getInstance();

    auto& cmd = registrar.getOrCreateCommand("saveskin", "Save your skin for npc", CommandPermissionLevel::GameDirectors);
    cmd.overload<SaveSkinParam>().required("name").execute(&executeSaveSkin);

    auto& cmd0 = registrar.getOrCreateCommand("removeskin", "Unloads and removs skin from DB", CommandPermissionLevel::GameDirectors);
    cmd0.overload<RemoveSkinParam>().required("name").execute(&executeRemoveSkin);

    auto& cmd1 = registrar.getOrCreateCommand("saveanimation", "Save your emotion for npc", CommandPermissionLevel::GameDirectors);
    cmd1.overload<SaveAnimationParam>().required("name").execute(&executeSaveAnimation);

    auto& cmd2 = registrar.getOrCreateCommand("testanimation", "Test your emotion for npc", CommandPermissionLevel::GameDirectors);
    cmd2.overload<TestAnimationParam>().required("name").execute(&executeTestAnimation);

}
} // namespace PlayerRegister
