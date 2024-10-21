#pragma once

#include "ll/api/mod/NativeMod.h"
#include "ll/api/data/KeyValueDB.h"

namespace LiteNPC {

class LiteNPCMod {

public:
    static LiteNPCMod& getInstance();
    static ll::data::KeyValueDB& getDB();

    LiteNPCMod(ll::mod::NativeMod& self) : mSelf(self) {}

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    /// @return True if the mod is loaded successfully.
    bool load();

    /// @return True if the mod is enabled successfully.
    bool enable();

    /// @return True if the mod is disabled successfully.
    bool disable();

    // Implement this method if you need to unload the mod.
    // /// @return True if the mod is unloaded successfully.
    // bool unload();

private:
    ll::mod::NativeMod& mSelf;
};

} // namespace my_mod
