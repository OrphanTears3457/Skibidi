#include "ActorBehavior.h"
#include <libultraship/bridge/consolevariablebridge.h>

#include "2s2h/CustomItem/CustomItem.h"
#include "2s2h/Rando/Rando.h"
#include "2s2h/ShipInit.hpp"
#include "2s2h/ObjectExtension/ActorListIndex.h"
#include "2s2h/ObjectExtension/ObjectExtension.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "assets/2s2h_assets.h"
#include <spdlog/spdlog.h>

extern "C" {
#include "variables.h"
#include "overlays/actors/ovl_Obj_Mure/z_obj_mure.h"

void ObjMure_CulledState(ObjMure*, PlayState*);
void ObjMure_ActiveState(ObjMure*, PlayState*);
}

// Key: (sceneId, room, actor list index)
// Value: (starting RandoCheckId, number of butterflies)
std::map<std::tuple<s16, u8, u8>, std::tuple<RandoCheckId, u8>> butterflyMap = {
    { { SCENE_00KEIKOKU, 0, 171 }, { RC_TERMINA_FIELD_BUTTERFLY_01, 2 } },
};

void DrawButterfly(Actor* actor, PlayState* play) {
    OPEN_DISPS(gPlayState->state.gfxCtx);
    RandoCheckId randoCheckId = Rando::ActorBehavior::GetObjectRandoCheckId(actor);
    Matrix_Scale(30.0f, 30.0f, 30.0f, MTXMODE_APPLY);
    Rando::DrawItem(Rando::ConvertItem(RANDO_SAVE_CHECKS[randoCheckId].randoItemId, randoCheckId), randoCheckId, actor);
    CLOSE_DISPS(gPlayState->state.gfxCtx);
}

std::unordered_map<Actor*, void*> lastActionFuncMap;

void Rando::ActorBehavior::InitEnButteBehavior() {
    bool shouldRegister = IS_RANDO && RANDO_SAVE_OPTIONS[RO_SHUFFLE_TREE_DROPS]; // piggybacking off trees for now

    COND_ID_HOOK(OnActorUpdate, ACTOR_OBJ_MURE, shouldRegister, [](Actor* actor) {
        ObjMure* objMure = (ObjMure*)actor;

        void* lastActionFunc = nullptr;
        if (lastActionFuncMap.contains(actor)) {
            lastActionFunc = lastActionFuncMap[actor];
        }

        if (lastActionFunc == (void*)ObjMure_CulledState && (void*)objMure->actionFunc == (void*)ObjMure_ActiveState) {
            // Transitioned from Culled to Active
            auto it = butterflyMap.find({ gPlayState->sceneId, gPlayState->roomCtx.curRoom.num, GetActorListIndex(actor) });
            if (it != butterflyMap.end()) {
                RandoCheckId startingRandoCheckId = std::get<0>(it->second);
                u8 butterflyCount = std::get<1>(it->second);
                for (u8 i = 0; i < butterflyCount; i++) {
                    RandoCheckId randoCheckId = static_cast<RandoCheckId>(static_cast<int>(startingRandoCheckId) + i);
                    if (!RANDO_SAVE_CHECKS[randoCheckId].shuffled || RANDO_SAVE_CHECKS[randoCheckId].cycleObtained || objMure->children[i] == nullptr) {
                        continue;
                    }
                    Actor* butterfly = objMure->children[i];
                    SetObjectRandoCheckId(butterfly, randoCheckId);
                    butterfly->draw = DrawButterfly;
                    // Probably replace the update func too, as the butterfly will need new behavior to fly towards player and be collected
                }
            }
        }
        lastActionFuncMap[actor] = (void*)objMure->actionFunc;
    });
}
