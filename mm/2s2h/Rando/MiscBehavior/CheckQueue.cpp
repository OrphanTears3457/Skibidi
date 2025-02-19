#include "MiscBehavior.h"
#include <libultraship/libultraship.h>
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/CustomItem/CustomItem.h"
#include "2s2h/CustomMessage/CustomMessage.h"
#include "2s2h/BenGui/Notification.h"
#include "2s2h/Rando/StaticData/StaticData.h"
#include "2s2h/ShipUtils.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"

extern "C" {
#include "variables.h"
#include "functions.h"
#include "objects/object_fall/object_fall.h"
extern TexturePtr gItemIcons[131];
extern s16 D_801CFF94[250];
}

static bool queued = false;

void DrawTrap(Actor* actor, PlayState* play) {
    SPDLOG_INFO("DrawTrap {} {}", CUSTOM_ITEM_FLAGS & CustomItem::CALLED_ACTION, CUSTOM_ITEM_PARAM);

    if (CUSTOM_ITEM_FLAGS & CustomItem::CALLED_ACTION) {
        OPEN_DISPS(play->state.gfxCtx);

        Gfx_SetupDL25_Opa(play->state.gfxCtx);
        Matrix_Scale(0.15f, 0.15f, 0.15f, MTXMODE_APPLY);
        gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        // Matrix_MultVec3f(sFocusOffset, &this->actor.focus.pos);

        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0x80, 255, 255, 255, 255);

        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)gMoonDL);

        CLOSE_DISPS(play->state.gfxCtx);
    } else {
        Matrix_Scale(30.0f, 30.0f, 30.0f, MTXMODE_APPLY);
        GetItem_Draw(gPlayState, GID_MASK_FIERCE_DEITY);
    }
}

// This function handles queuing up item gives that the player has been marked as eligible for. If you are looking for
// the behavior of the actual giving itself, the heavy lifting is done by the GameInteractor queue. This function is
// currently called every frame, and loops through the entire list of checks, this works for now but as the check list
// grows we should keep an eye on performance.
//
// Though it seems counter-intuitive, we currently only allow one thing from randommizer to be queued at a time,
// primarily because of the way an item can be converted may change as you queue up multiple items. This is actually
// fine for both the giving/drawing, as we can call ConvertItem() inside the Give/Draw lambda, but the message we
// pass to the queue is static and would need to be updated if we allowed multiple items to be queued at once.
void Rando::MiscBehavior::CheckQueue() {
    if (queued) {
        return;
    }

    for (auto& [randoCheckId, randoStaticCheck] : Rando::StaticData::Checks) {
        auto randoSaveCheck = RANDO_SAVE_CHECKS[randoCheckId];

        if (randoSaveCheck.eligible) {
            queued = true;

            SPDLOG_INFO("CheckQueue {} {}", (int)randoSaveCheck.randoItemId, (int)randoCheckId);

            GameInteractor::Instance->events.emplace_back(GIEventGiveItem{
                .showGetItemCutscene =
                    Rando::StaticData::ShouldShowGetItemCutscene(ConvertItem(randoSaveCheck.randoItemId, randoCheckId)),
                .param = (int16_t)randoCheckId,
                .giveItem =
                    [](Actor* actor, PlayState* play) {
                        auto& randoSaveCheck = RANDO_SAVE_CHECKS[CUSTOM_ITEM_PARAM];

                        if (randoSaveCheck.randoItemId == RI_TRAP) {
                            CustomMessage::Entry entry = {
                                .textboxType = 2,
                                .msg = "Merry Christmas ya filthy animal!",
                            };

                            if (CUSTOM_ITEM_FLAGS & CustomItem::GIVE_ITEM_CUTSCENE) {
                                CustomMessage::SetActiveCustomMessage(entry.msg, entry);
                            } else {
                                CustomMessage::StartTextbox(entry.msg + "\x1C\x02\x10", entry);
                            }
                            Rando::GiveItem(RI_TRAP);
                            randoSaveCheck.obtained = true;
                            randoSaveCheck.eligible = false;
                            queued = false;
                            CUSTOM_ITEM_PARAM = RI_TRAP;
                            return;
                        }

                        RandoItemId randoItemId =
                            Rando::ConvertItem(randoSaveCheck.randoItemId, (RandoCheckId)CUSTOM_ITEM_PARAM);
                        std::string prefix = "You found";
                        std::string message = Rando::StaticData::GetItemName(randoItemId);

                        if (randoItemId == RI_JUNK) {
                            randoItemId = Rando::CurrentJunkItem();
                        }

                        CustomMessage::Entry entry = {
                            .textboxType = 2,
                            .icon = Rando::StaticData::GetIconForZMessage(randoItemId),
                            .msg = prefix + " " + message + "!",
                        };

                        if (CUSTOM_ITEM_FLAGS & CustomItem::GIVE_ITEM_CUTSCENE) {
                            CustomMessage::SetActiveCustomMessage(entry.msg, entry);
                        } else if (Rando::StaticData::ShouldShowGetItemCutscene(randoItemId)) {
                            CustomMessage::StartTextbox(entry.msg + "\x1C\x02\x10", entry);
                        } else {
                            Notification::Emit({
                                .itemIcon = Rando::StaticData::GetIconTexturePath(randoItemId),
                                .message = prefix,
                                .suffix = message,
                            });
                        }
                        Rando::GiveItem(randoItemId);
                        randoSaveCheck.cycleObtained = true;
                        randoSaveCheck.obtained = true;
                        randoSaveCheck.eligible = false;
                        queued = false;
                        CUSTOM_ITEM_PARAM = randoItemId;
                    },
                .drawItem =
                    [](Actor* actor, PlayState* play) {
                        RandoItemId randoItemId;

                        // If the item has been given, the CUSTOM_ITEM_PARAM is set to the RI, prior to that it's the RC
                        if (CUSTOM_ITEM_FLAGS & CustomItem::CALLED_ACTION) {
                            randoItemId = (RandoItemId)CUSTOM_ITEM_PARAM;
                        } else {
                            auto& randoSaveCheck = RANDO_SAVE_CHECKS[CUSTOM_ITEM_PARAM];
                            randoItemId =
                                Rando::ConvertItem(randoSaveCheck.randoItemId, (RandoCheckId)CUSTOM_ITEM_PARAM);
                        }

                        if (randoItemId == RI_TRAP) {
                            DrawTrap(actor, play);
                            return;
                        }

                        Matrix_Scale(30.0f, 30.0f, 30.0f, MTXMODE_APPLY);
                        Rando::DrawItem(randoItemId);
                    } });
            return;
        }
    }
}

void Rando::MiscBehavior::CheckQueueReset() {
    queued = false;
    GameInteractor::Instance->currentEvent = GIEventNone{};
    GameInteractor::Instance->events.clear();
}
