#pragma once

#include <string>

#include "ll/api/Logger.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/Level.h"
#include "ll/api/service/Bedrock.h"
#include "nlohmann/json.hpp"
#include "mod/LiteNPCMod.h"

#define NATIVE_MOD LiteNPC::LiteNPCMod::getInstance().getSelf()
#define LOGGER NATIVE_MOD.getLogger()
#define LEVEL ll::service::getLevel()
#define DB LiteNPCMod::getDB()

#include "model/NPC.h"
#include "mod/Util.h"

using namespace std;
using std::string;
using std::to_string;
using nlohmann::json;
