#pragma once

#include <string>

#include "ll/api/io/Logger.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/Level.h"
#include "mc/common/ActorRuntimeID.h"
#include "mc/common/ActorUniqueID.h"
#include "ll/api/service/Bedrock.h"
#include "nlohmann/json.hpp"
#include "mod/LiteNPCMod.h"
#include <mc/deps/core/math/Vec2.h>
#include <mc/deps/core/math/Vec3.h>
#include <mc/deps/core/utility/AutomaticID.h>

#define NATIVE_MOD LiteNPC::LiteNPCMod::getInstance().getSelf()
#define LOGGER NATIVE_MOD.getLogger()
#define LEVEL ll::service::getLevel()
#define DB LiteNPCMod::getDB()

#include "model/LiteNPC.h"
#include "mod/Util.h"
#include "ll/api/Config.h"

using namespace std;
using std::string;
using std::to_string;
using nlohmann::json;
