#include <libultraship/bridge/consolevariablebridge.h>
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"
#include <spdlog/spdlog.h>
#include <unordered_map>
#include "2s2h/ShipUtils.h"

extern "C" {
#include "variables.h"
#include "functions.h"
}

#define CVAR_NAME "gModes.EnemyRando.Enabled"
#define CVAR CVarGetInteger(CVAR_NAME, 1)

#define DEFINE_ACTOR(name, _enumValue, _allocType, _debugName, _humanName) { _enumValue, _debugName },
#define DEFINE_ACTOR_INTERNAL(_name, _enumValue, _allocType, _debugName, _humanName) { _enumValue, _debugName },
#define DEFINE_ACTOR_UNSET(_enumValue) { _enumValue, "Unset" },

static std::unordered_map<s16, const char*> readableName = {
#include "tables/actor_table.h"
};

#undef DEFINE_ACTOR
#undef DEFINE_ACTOR_INTERNAL
#undef DEFINE_ACTOR_UNSET

std::string getReadableName(u16 actorId) {
    return readableName.count(actorId) ? readableName[actorId] : "???";
}

static std::unordered_map<s16, bool> shouldRandomize = {
    { ACTOR_EN_ST, true }, // Skulltula (large suspended one)
    { ACTOR_EN_MKK, true }, // Black and White Boe
    { ACTOR_EN_KAREBABA, true }, // Wilted Deku Baba and Mini Baba
    { ACTOR_EN_DEKUBABA, true }, // Deku Baba
    { ACTOR_EN_DINOFOS, true }, // Dinolfos
    { ACTOR_EN_GRASSHOPPER, true }, // Dragonfly
    { ACTOR_EN_KAME, true }, // Snapper
    { ACTOR_EN_TITE, true }, // Tektites
    { ACTOR_EN_FIREFLY, true }, // Keese (Normal, Fire, Ice)
    { ACTOR_EN_WF, true }, // Wolfos and White Wolfos
    { ACTOR_EN_FZ, true }, // Freezard
    { ACTOR_EN_RAT, true }, // Real Bombchu
    { ACTOR_EN_CROW, true }, // Guay
    { ACTOR_EN_VM, true }, // Beamos
    { ACTOR_EN_RD, true }, // Redead/Gibdo that cannot talk to the player.
    { ACTOR_EN_BAGUO, true }, // Nejiron
    { ACTOR_EN_SLIME, true }, // Chuchu
};

static bool ignoreNextSpawn = false;

void RegisterEnemyRando() {
    COND_HOOK(ShouldActorInit, CVAR, [](Actor* actor, bool* should) {
        if (ignoreNextSpawn) {
            ignoreNextSpawn = false;
            return;
        }

        if (!shouldRandomize.count(actor->id)) {
            SPDLOG_INFO("EnemyRando: Actor ID: {} {}", actor->id, getReadableName(actor->id));
            return;
        }

        *should = false;

        int index = Ship_Random(0, shouldRandomize.size() - 1);
        auto it = shouldRandomize.begin();
        std::advance(it, index);

        s16 actorIdToSpawn = it->first;
        ignoreNextSpawn = true;
        Actor_Spawn(&gPlayState->actorCtx, gPlayState, actorIdToSpawn, actor->world.pos.x, actor->world.pos.y,
            actor->world.pos.z, actor->shape.rot.x, actor->shape.rot.y, actor->shape.rot.z, 0);
    });
}

static RegisterShipInitFunc initFunc(RegisterEnemyRando, { CVAR_NAME });
