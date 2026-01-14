#include "Chaos.h"
#include <variant>
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"

extern "C" {
#include "variables.h"
uint64_t GetUnixTimestamp();
}

namespace Chaos {

bool controlsInverted = false;

struct ChaosCuccoStorm {
    s32 timeRemaining;
};

typedef std::variant<ChaosCuccoStorm> ChaosEvent;

std::unordered_map<ChaosEffect, u64> timedEffectsStartTimes;

static void ProcessChaos(Actor* _) {
    Player* player = GET_PLAYER(gPlayState);

    // Player on title screen
    if (gSaveContext.gameMode != GAMEMODE_NORMAL) {
        return;
    }

    // If the player has a message active, stop
    if (gPlayState->msgCtx.msgMode != 0) {
        return;
    }

    // If the player is in a blocking cutscene, stop
    if (Player_InBlockingCsMode(gPlayState, player)) {
        return;
    }

    // If player is dead, stop
    if (player->stateFlags1 & PLAYER_STATE1_DEAD) {
        return;
    }

    if (rand() % 500 == 0) {
        SPDLOG_INFO("Applying random chaos effect");
        if (!timedEffectsStartTimes.count(CE_INVERT_CONTROLS)) {
            controlsInverted = true;
            timedEffectsStartTimes[CE_INVERT_CONTROLS] = GetUnixTimestamp();
        }
    }

    auto now = GetUnixTimestamp();
    for (auto it = timedEffectsStartTimes.begin(); it != timedEffectsStartTimes.end(); ) {
        auto effect = it->first;
        auto startTime = it->second;
        s32 duration = 10; // Default duration of 30 seconds
        if (now - startTime >= (duration * 1000)) {
            it = timedEffectsStartTimes.erase(it);
            SPDLOG_INFO("Removing chaos effect");
            switch (effect) {
                case CE_INVERT_CONTROLS:
                    controlsInverted = false;
                    break;
                case CE_PLAYER_SIZE:
                    break;
                case CE_HIDE_UI:
                    break;
                default:
                    break;
            }
        } else {
            ++it;
        }
    }
}

static RegisterShipInitFunc initFunc([]() {
    COND_ID_HOOK(OnActorUpdate, ACTOR_PLAYER, true, ProcessChaos);
}, {});

} // namespace Chaos
