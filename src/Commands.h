#pragma once

#include <llapi/RegCommandAPI.h>

class SaveSkinCommand : public Command {
	string name;
public:
	void execute(CommandOrigin const& ori, CommandOutput& output) const override;

	static void setup(CommandRegistry* registry) {
		registry->registerCommand("saveskin", "Сохранить текущий скин", CommandPermissionLevel::GameMasters, { (CommandFlagValue)0 }, { (CommandFlagValue)0x80 });
		registry->registerOverload<SaveSkinCommand>("saveskin", RegisterCommandHelper::makeMandatory(&SaveSkinCommand::name, "skin name"));
	}
};
