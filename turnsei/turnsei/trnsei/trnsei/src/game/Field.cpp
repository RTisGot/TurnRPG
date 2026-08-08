#include "Field.h"
#include "ActorRenderer.h"
#include "CombatSystem.h"
#include "FieldEnemy.h"
#include "Player.h"
#include "Scene.h"
#include "SimpleMap.h"
#include "StoryEvent.h"
#include "../../assets/ImportedModel.h"
#include "imgui.h"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

void EnsureSharedEnemyModelsLoaded();
const ImportedModel* GetSharedEnemyModel(int index);
void UpdateSharedEnemyModels(float deltaTime);

Player player;
Shader* fieldShader = nullptr;
extern GLFWwindow* window;

namespace
{
    float deltaTime = 0.0f;
    float previousFrameTime = 0.0f;
    ImportedModel playerModel;
    bool playerModelLoadAttempted = false;
    float cameraYaw = 0.0f;
    float cameraPitch = glm::radians(32.0f);
    float cameraDistance = 20.0f;
    float cameraSensitivity = 0.006f;
    bool settingsOpen = false;
    bool escapeWasDown = false;
    bool encounterTransitionActive = false;
    float encounterTransitionTime = 0.0f;
    const float encounterTransitionDuration = 0.82f;
    glm::vec3 encounterTargetPosition(0.0f);
    FieldEnemy fieldEnemies[kMaxFieldEnemies];
    int fieldEnemyLevels[kMaxFieldEnemies] = { 1, 1, 1, 1 };
    int activeEncounterEnemy = -1;
    int pendingEncounterLevel = 1;
    bool resetCombatProgressionPending = true;
    WalkAnimState playerWalkAnim;
    WalkAnimState enemyWalkAnim[kMaxFieldEnemies];
    bool initialDistrictSecured = false;
    bool transitTerminalActivated = false;
    bool openingRouteComplete = false;
    bool firstEncounterTutorialShown = false;
    const glm::vec3 transitTerminalPosition(0.0f, 0.0f, 14.0f);
    const glm::vec3 evacuationBoatPosition(0.0f, 0.0f, -12.0f);

    struct MapNpc {
        std::string name;
        glm::vec3 position;
        float rotationY;
        glm::vec3 color;
        std::vector<std::string> beforeBattleDialogue;
        std::vector<std::string> afterBattleDialogue;
        std::vector<std::string> afterTerminalDialogue;
    };

    struct MapDialogueState {
        bool isOpen = false;
        bool interactWasDown = false;
        const MapNpc* activeNpc = nullptr;
        size_t pageIndex = 0;
    };

    MapNpc mapNpcs[] = {
        {
            u8"記憶技師ミオ", glm::vec3(3.8f, 0.0f, 4.0f), 210.0f,
            glm::vec3(0.20f, 0.78f, 0.92f),
            {
                u8"来てくれたのね。私はミオ。沈んだ街に残された記録を調べているの。",
                u8"旧交通層の端末を動かすには、周囲の漂流体をすべて止める必要がある。",
                u8"戦いが終わったら北の端末へ向かって。私が水路を開ける。"
            },
            {
                u8"漂流体の反応が消えた……無事でよかった。",
                u8"北の端末を調べて。認証が通れば、南側ドックへの水路を開けられる。"
            },
            {
                u8"水路ゲートは開いた。南側ドックの避難艇で旧鐘楼へ向かって",
                u8"鐘楼には、この街が沈んだ日の記録が残っているはず。"
            }
        },
    };
    MapDialogueState mapDialogue;

    bool FileExists(const std::string& path)
    {
        std::ifstream file(path, std::ios::binary);
        return file.good();
    }

    float Saturate(float v) { return std::max(0.0f, std::min(v, 1.0f)); }
    float SmoothStep(float v) { v = Saturate(v); return v * v * (3.0f - 2.0f * v); }

    // アニメーションFBX{ファイル名パターン, クリップ名}
    struct AnimFile { const char* pattern; const char* clipName; };
    const AnimFile ANIM_FILES[] = {
        { "Walk",       "walk"  },
        { "walk",       "walk"  },
        { "Run",        "run"   },
        { "run",        "run"   },
        { "Idle",       "idle"  },
        { "idle",       "idle"  },
        { "Stand",      "idle"  },
        { "Locomotion", "walk"  },
    };

    void TryLoadAnimations(const std::string& root)
    {
        for (const auto& af : ANIM_FILES) {
            std::string path = root + af.pattern + ".fbx";
            if (FileExists(path)) {
                playerModel.loadAnimationsFrom(path, af.clipName);
            }
        }
    }

    void LoadPlayerModel()
    {
        if (playerModelLoadAttempted) return;
        playerModelLoadAttempted = true;

        const char* exts[] = { ".fbx", ".obj", ".gltf", ".glb" };
        const char* roots[] = { "Resource/", "../trnsei/Resource/" };
        const char* names[] = { "Character_Gameplay", "Character", "Player", "Hero" };
        for (const char* root : roots)
            for (const char* name : names)
                for (const char* ext : exts) {
                    std::string path = std::string(root) + name + ext;
                    if (FileExists(path) && playerModel.load(path)) {
                        if (std::string(name) == "Character_Gameplay") {
                            const std::string walkJson = std::string(root) + "Character_Walk.json";
                            playerModel.loadAnimationJson(walkJson);
                        }
                      
                        TryLoadAnimations(std::string(root));
                        return;
                    }
                }
    }

    void UpdateCameraInput()
    {
        ImGuiIO& io = ImGui::GetIO();
        const bool altPressed = window && (
            glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS);
        if (!altPressed && !io.WantCaptureMouse) {
            // マウスを動かした方向へカメラを回転
            cameraYaw -= io.MouseDelta.x * cameraSensitivity;
            cameraPitch -= io.MouseDelta.y * cameraSensitivity;
            cameraPitch = std::max(glm::radians(3.0f), std::min(cameraPitch, glm::radians(75.0f)));
        }
        if (!io.WantCaptureMouse && io.MouseWheel != 0.0f) {
            cameraDistance -= io.MouseWheel * 1.4f;
            cameraDistance = std::max(6.0f, std::min(cameraDistance, 35.0f));
        }
    }

    void DrawEncounterOverlay(int width, int height, float progress)
    {
        if (!encounterTransitionActive) return;
        float eased = SmoothStep(progress);
        float flash = std::max(0.0f, 1.0f - progress * 5.0f);
        float fade = SmoothStep((progress - 0.42f) / 0.58f);
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        dl->AddRectFilled({0,0}, {(float)width,(float)height}, IM_COL32(255,246,210,(int)(flash*92)));
        dl->AddRectFilled({0,0}, {(float)width,(float)height}, IM_COL32(4,8,14,(int)(fade*236)));
        float lineAlpha = Saturate(1.0f - std::abs(eased - 0.55f) * 2.0f) * 145.0f;
        float cy = (float)height * 0.5f;
        dl->AddLine({(float)width*0.18f, cy}, {(float)width*0.82f, cy}, IM_COL32(255,214,124,(int)lineAlpha), 2.0f);
    }

    bool AllFieldEnemiesDefeated()
    {
        for (int i = 0; i < kMaxFieldEnemies; ++i) {
            if (fieldEnemies[i].active) return false;
        }
        return true;
    }

    bool StartEncounterIfNeeded(CombatSystem& cs)
    {
        if (encounterTransitionActive || mapDialogue.isOpen) return false;
        for (int i = 0; i < kMaxFieldEnemies; ++i) {
            if (!fieldEnemies[i].active) continue;
            glm::vec2 pxz(player.position.x, player.position.z);
            glm::vec2 exz(fieldEnemies[i].position.x, fieldEnemies[i].position.z);
            if (glm::distance(pxz, exz) <= 2.0f) {
                if (!firstEncounterTutorialShown) {
                    firstEncounterTutorialShown = true;
                    cs.beginTutorialBattle();
                }
                fieldEnemies[i].active = false;
                activeEncounterEnemy = i;
                pendingEncounterLevel = fieldEnemyLevels[i];
                encounterTransitionActive = true;
                encounterTransitionTime = 0.0f;
                encounterTargetPosition = fieldEnemies[i].position;
                return true;
            }
        }
        return false;
    }

    const MapNpc* FindTalkableNpc(float* outDistance = nullptr)
    {
        const MapNpc* nearest = nullptr;
        float nearestDist = 9999.0f;
        glm::vec2 pxz(player.position.x, player.position.z);
        for (const MapNpc& npc : mapNpcs) {
            glm::vec2 nxz(npc.position.x, npc.position.z);
            float dist = glm::distance(pxz, nxz);
            if (dist < nearestDist) {
                nearestDist = dist;
                nearest = &npc;
            }
        }
        if (outDistance) *outDistance = nearestDist;
        return nearestDist <= 2.7f ? nearest : nullptr;
    }

    void OpenNpcDialogue(const MapNpc* npc)
    {
        if (!npc) return;
        mapDialogue.activeNpc = npc;
        mapDialogue.pageIndex = 0;
        mapDialogue.isOpen = true;
    }

    void UpdateNpcInteraction()
    {
        ImGuiIO& io = ImGui::GetIO();
        bool interactDown = window && glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
        if (interactDown && !mapDialogue.interactWasDown && !io.WantCaptureKeyboard) {
            if (initialDistrictSecured && !transitTerminalActivated &&
                glm::distance(glm::vec2(player.position.x, player.position.z),
                              glm::vec2(transitTerminalPosition.x, transitTerminalPosition.z)) <= 2.8f) {
                transitTerminalActivated = true;
                g_CurrentEventID = "AfterDistrict";
                g_StoryNeedsLoad = true;
                currentScene = Scene::StoryEvent;
                mapDialogue.interactWasDown = interactDown;
                return;
            }
            if (transitTerminalActivated && !openingRouteComplete &&
                glm::distance(glm::vec2(player.position.x, player.position.z),
                              glm::vec2(evacuationBoatPosition.x, evacuationBoatPosition.z)) <= 3.2f) {
                openingRouteComplete = true;
                g_CurrentEventID = "Departure";
                g_StoryNeedsLoad = true;
                currentScene = Scene::StoryEvent;
                mapDialogue.interactWasDown = interactDown;
                return;
            }
            float dist = 0.0f;
            const MapNpc* npc = FindTalkableNpc(&dist);
            if (npc) OpenNpcDialogue(npc);
        }
        mapDialogue.interactWasDown = interactDown;
    }

    void RenderNpcPrompt(int width, int height)
    {
        if (mapDialogue.isOpen) return;

        const glm::vec2 playerXZ(player.position.x, player.position.z);
        const bool nearTerminal = initialDistrictSecured &&
            !transitTerminalActivated &&
            glm::distance(playerXZ, glm::vec2(
                transitTerminalPosition.x, transitTerminalPosition.z)) <= 2.8f;
        const bool nearBoat = transitTerminalActivated &&
            !openingRouteComplete &&
            glm::distance(playerXZ, glm::vec2(
                evacuationBoatPosition.x, evacuationBoatPosition.z)) <= 3.2f;

        float dist = 0.0f;
        const MapNpc* npc = FindTalkableNpc(&dist);
        if (!npc && !nearTerminal && !nearBoat) return;

        const char* text = nearTerminal ? "E : Activate terminal"
            : nearBoat ? "E : Board boat"
            : "E : Talk";
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        ImVec2 size = ImGui::CalcTextSize(text);
        ImVec2 pad(18.0f, 10.0f);
        ImVec2 minPos(((float)width - size.x) * 0.5f - pad.x, (float)height - 122.0f);
        ImVec2 maxPos(((float)width + size.x) * 0.5f + pad.x, minPos.y + size.y + pad.y * 2.0f);
        dl->AddRectFilled(minPos, maxPos, IM_COL32(8, 18, 28, 210), 6.0f);
        dl->AddRect(minPos, maxPos, IM_COL32(80, 205, 240, 180), 6.0f, 0, 1.5f);
        dl->AddText(ImVec2(minPos.x + pad.x, minPos.y + pad.y), IM_COL32(235, 250, 255, 255), text);
    }

    void RenderFieldMissionHud(int width, int height, const CombatSystem& combatSystem)
    {
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        const ImVec2 panelMin(24.0f, 24.0f);
        const ImVec2 panelMax(std::min(440.0f, width * 0.42f), 142.0f);
        dl->AddRectFilledMultiColor(panelMin, panelMax,
            IM_COL32(3, 16, 25, 224), IM_COL32(4, 30, 37, 190),
            IM_COL32(3, 22, 30, 205), IM_COL32(2, 12, 21, 230));
        dl->AddLine(ImVec2(panelMin.x, panelMin.y), ImVec2(panelMin.x, panelMax.y),
            IM_COL32(73, 224, 213, 235), 3.0f);
        dl->AddText(ImGui::GetFont(), 17.0f, ImVec2(42.0f, 38.0f),
            IM_COL32(112, 225, 218, 255), u8"第1章  水面に残る星");
        dl->AddText(ImGui::GetFont(), 22.0f, ImVec2(42.0f, 66.0f),
            IM_COL32(237, 249, 248, 255), u8"鐘楼へ続く旧交通層を調査する");
        dl->AddText(ImGui::GetFont(), 15.0f, ImVec2(42.0f, 104.0f),
            IM_COL32(151, 190, 194, 245), u8"漂流体を排除  /  ミオに E で話しかける");

        if (initialDistrictSecured) {
            dl->AddRectFilled(ImVec2(24.0f, 148.0f), ImVec2(panelMax.x, 190.0f),
                IM_COL32(4, 35, 38, 224), 3.0f);
            dl->AddText(ImGui::GetFont(), 16.0f, ImVec2(42.0f, 160.0f),
                IM_COL32(116, 235, 199, 255),
                openingRouteComplete
                    ? "ROUTE OPEN  /  Chapter 1 route secured"
                    : transitTerminalActivated
                        ? "OBJECTIVE  /  Board the boat at the south dock"
                        : "AREA SECURED  /  Proceed to the transit terminal");
            if (!openingRouteComplete) {
                const glm::vec3 target = transitTerminalActivated
                    ? evacuationBoatPosition : transitTerminalPosition;
                const float distance = glm::distance(
                    glm::vec2(player.position.x, player.position.z),
                    glm::vec2(target.x, target.z));
                char distanceText[96];
                std::snprintf(distanceText, sizeof(distanceText),
                    "%s  %.1f m", transitTerminalActivated ? "SOUTH DOCK" : "TRANSIT TERMINAL",
                    distance);
                dl->AddText(ImGui::GetFont(), 14.0f, ImVec2(42.0f, 184.0f),
                    IM_COL32(155, 214, 220, 255), distanceText);
            }
        }

        const float tide = 0.5f + 0.5f * std::sin((float)ImGui::GetTime() * 0.10f);
        ImVec2 tideMin((float)width - 272.0f, 27.0f);
        ImVec2 tideMax((float)width - 28.0f, 74.0f);
        dl->AddRectFilled(tideMin, tideMax, IM_COL32(2, 18, 28, 205), 2.0f);
        dl->AddText(ImGui::GetFont(), 14.0f, ImVec2(tideMin.x + 12.0f, tideMin.y + 8.0f),
            IM_COL32(137, 221, 219, 255), u8"潮位予測");
        dl->AddRectFilled(ImVec2(tideMin.x + 82.0f, tideMin.y + 18.0f),
            ImVec2(tideMax.x - 12.0f, tideMin.y + 23.0f), IM_COL32(18, 55, 63, 255), 2.0f);
        dl->AddRectFilled(ImVec2(tideMin.x + 82.0f, tideMin.y + 18.0f),
            ImVec2(tideMin.x + 82.0f + (tideMax.x - tideMin.x - 94.0f) * tide, tideMin.y + 23.0f),
            IM_COL32(70, 217, 210, 255), 2.0f);

        char progressionText[128];
        std::snprintf(progressionText, sizeof(progressionText),
            "SCORE  %d   /   DEFEATED  %d   /   NEXT Lv. %d   /   LOSSES %d/3",
            combatSystem.getScore(), combatSystem.getDefeatedEnemyCount(),
            combatSystem.getNextEnemyLevel(), combatSystem.getDefeatCount());
        ImVec2 scoreSize = ImGui::CalcTextSize(progressionText);
        ImVec2 scoreMin((float)width - scoreSize.x - 54.0f, 82.0f);
        ImVec2 scoreMax((float)width - 28.0f, 118.0f);
        dl->AddRectFilled(scoreMin, scoreMax, IM_COL32(2, 18, 28, 215), 3.0f);
        dl->AddRect(scoreMin, scoreMax, IM_COL32(75, 215, 206, 155), 3.0f);
        dl->AddText(ImVec2(scoreMin.x + 13.0f, scoreMin.y + 9.0f),
            IM_COL32(224, 248, 241, 255), progressionText);
    }

    const std::vector<std::string>& GetActiveNpcDialogue(const MapNpc& npc)
    {
        if (transitTerminalActivated) return npc.afterTerminalDialogue;
        if (initialDistrictSecured) return npc.afterBattleDialogue;
        return npc.beforeBattleDialogue;
    }

    void RenderMapDialogue()
    {
        if (!mapDialogue.isOpen || !mapDialogue.activeNpc) return;

        const std::vector<std::string>& dialogue = GetActiveNpcDialogue(*mapDialogue.activeNpc);
        if (dialogue.empty()) {
            mapDialogue.isOpen = false;
            return;
        }
        mapDialogue.pageIndex = std::min(mapDialogue.pageIndex, dialogue.size() - 1);

        ImVec2 screenSize = ImGui::GetIO().DisplaySize;
        float dlgW = std::min(430.0f, screenSize.x - 40.0f);
        float dlgH = 230.0f;
        ImGui::SetNextWindowPos(ImVec2(screenSize.x - dlgW - 24.0f, screenSize.y - dlgH - 28.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(dlgW, dlgH), ImGuiCond_Always);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.035f, 0.075f, 0.095f, 0.96f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.28f, 0.80f, 0.90f, 0.75f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);

        bool dlgOpen = mapDialogue.isOpen;
        if (ImGui::Begin("##MapDialogue", &dlgOpen,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)) {
            ImGui::TextColored(ImVec4(0.55f, 0.92f, 1.0f, 1.0f), "%s", mapDialogue.activeNpc->name.c_str());
            ImGui::SameLine(dlgW - 64.0f);
            if (ImGui::SmallButton(u8"閉じる")) dlgOpen = false;
            ImGui::Separator();

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.04f, 0.05f, 0.68f));
            ImGui::BeginChild("##MapDialogueText", ImVec2(0, 112.0f), true);
            ImGui::TextWrapped("%s", dialogue[mapDialogue.pageIndex].c_str());
            ImGui::EndChild();
            ImGui::PopStyleColor();

            ImGui::TextDisabled("%d / %d", static_cast<int>(mapDialogue.pageIndex + 1), static_cast<int>(dialogue.size()));
            ImGui::SameLine(dlgW - 112.0f);
            const bool hasNext = mapDialogue.pageIndex + 1 < dialogue.size();
            if (ImGui::Button(hasNext ? u8"次へ" : u8"会話を終える", ImVec2(92.0f, 0.0f))) {
                if (hasNext) ++mapDialogue.pageIndex;
                else dlgOpen = false;
            }
        }
        ImGui::End();
        if (!dlgOpen) mapDialogue.isOpen = false;
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
    }

    void RenderFieldSettings(int width, int height, CombatSystem& combatSystem)
    {
        if (!settingsOpen) return;

        ImDrawList* background = ImGui::GetBackgroundDrawList();
        background->AddRectFilled(
            ImVec2(0.0f, 0.0f), ImVec2((float)width, (float)height),
            IM_COL32(2, 5, 12, 185));

        const ImVec2 panelSize(500.0f, 430.0f);
        ImGui::SetNextWindowPos(
            ImVec2(width * 0.5f, height * 0.5f),
            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(panelSize, ImGuiCond_Always);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.025f, 0.045f, 0.075f, 0.98f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.30f, 0.72f, 0.84f, 0.80f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);

        if (ImGui::Begin("##FieldSettings", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)) {
            ImGui::TextColored(ImVec4(0.65f, 0.92f, 1.0f, 1.0f), u8"設定");
            ImGui::SameLine(panelSize.x - 112.0f);
            ImGui::TextDisabled("ESC : CLOSE");
            ImGui::Separator();
            ImGui::Spacing();

            float sensitivityDisplay = cameraSensitivity * 1000.0f;
            ImGui::TextUnformatted(u8"カメラ感度");
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::SliderFloat("##CameraSensitivity", &sensitivityDisplay, 1.5f, 12.0f, "%.1f")) {
                cameraSensitivity = sensitivityDisplay / 1000.0f;
            }

            ImGui::Spacing();
            ImGui::SeparatorText(u8"所持アイテム");
            ImGui::Text(u8"攻撃強化アンプル  x%d", combatSystem.getAttackItemCount());
            ImGui::TextDisabled(u8"攻撃力+10、3ターン継続");

            ImGui::Text(u8"回復キット  x%d", combatSystem.getHealingItemCount());
            ImGui::TextDisabled(u8"HPを60回復");
            ImGui::TextDisabled(u8"アイテムは戦闘中のみ使用できます");
            if (!combatSystem.getLastDroppedItem().empty()) {
                ImGui::TextColored(ImVec4(0.55f, 0.90f, 0.78f, 1.0f),
                    u8"最近のドロップ: %s", combatSystem.getLastDroppedItem().c_str());
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            if (ImGui::Button(u8"ゲームに戻る", ImVec2(-1.0f, 42.0f))) {
                settingsOpen = false;
            }
            if (ImGui::Button(u8"タイトルへ戻る", ImVec2(-1.0f, 42.0f))) {
                settingsOpen = false;
                currentScene = Scene::Title;
            }
        }
        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
    }
}

bool IsFieldSettingsOpen()
{
    return settingsOpen;
}

void FieldInit()
{
    static bool done = false;
    if (done) return;
    done = true;

    player.position = glm::vec3(0.0f, 0.0f, 7.0f);
    player.rotationY = 0.0f;
    InitActorRenderer();
    EnsureSharedEnemyModelsLoaded();
    InitSimpleMap();
    InitFieldEnemies(fieldEnemies);
    LoadPlayerModel();
}

void ResetFieldProgress()
{
    settingsOpen = false;
    escapeWasDown = false;
    encounterTransitionActive = false;
    encounterTransitionTime = 0.0f;
    initialDistrictSecured = false;
    transitTerminalActivated = false;
    openingRouteComplete = false;
    firstEncounterTutorialShown = false;
    activeEncounterEnemy = -1;
    pendingEncounterLevel = 1;
    for (int& level : fieldEnemyLevels) level = 1;
    resetCombatProgressionPending = true;
    mapDialogue = MapDialogueState{};
    player.position = glm::vec3(0.0f, 0.0f, 7.0f);
    player.rotationY = 0.0f;
    playerWalkAnim = WalkAnimState{};
    for (WalkAnimState& animation : enemyWalkAnim) {
        animation = WalkAnimState{};
    }
    InitFieldEnemies(fieldEnemies);
}

void FieldUpdate(CombatSystem& combatSystem)
{
    if (resetCombatProgressionPending) {
        combatSystem.resetProgression();
        resetCombatProgressionPending = false;
    }
    float currentTime = (float)glfwGetTime();
    deltaTime = previousFrameTime > 0.0f ? currentTime - previousFrameTime : 0.0f;
    previousFrameTime = currentTime;

    const bool escapeDown = window && glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    if (escapeDown && !escapeWasDown) {
        settingsOpen = !settingsOpen;
    }
    escapeWasDown = escapeDown;
    if (settingsOpen) deltaTime = 0.0f;

    const bool battleWon = combatSystem.consumeBattleVictory();
    const bool battleLost = combatSystem.consumeBattleDefeat();
    if (battleLost && combatSystem.getDefeatCount() >= 3) {
        ResetFieldProgress();
        SceneUpdate(Scene::Result);
        return;
    }
    if (battleWon || battleLost) {
        const int nextLevel = battleWon
            ? combatSystem.getNextEnemyLevel() : pendingEncounterLevel;
        if (battleWon) {
            for (int& level : fieldEnemyLevels) level = nextLevel;
        }

        if (activeEncounterEnemy >= 0 && activeEncounterEnemy < kMaxFieldEnemies) {
            const glm::vec3 respawnPoints[] = {
                glm::vec3(-12.0f, 0.0f, -10.0f), glm::vec3(8.0f, 0.0f, -14.0f),
                glm::vec3(-6.0f, 0.0f, -20.0f), glm::vec3(15.0f, 0.0f, -5.0f)
            };
            int bestSpawn = 0;
            float bestDistance = -1.0f;
            for (int i = 0; i < 4; ++i) {
                const float distance = glm::distance(
                    glm::vec2(player.position.x, player.position.z),
                    glm::vec2(respawnPoints[i].x, respawnPoints[i].z));
                if (distance > bestDistance) {
                    bestDistance = distance;
                    bestSpawn = i;
                }
            }
            FieldEnemy& enemy = fieldEnemies[activeEncounterEnemy];
            enemy.position = respawnPoints[bestSpawn];
            enemy.targetPos = enemy.position;
            enemy.wanderTimer = 2.0f;
            enemy.moving = false;
            enemy.active = true;
        }
        activeEncounterEnemy = -1;
    }

    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (settingsOpen) {
        // 設定画面中はフィールドの進行と入力を停止する。
    }
    else if (encounterTransitionActive) {
        encounterTransitionTime += deltaTime;
    }
    else {
        UpdateNpcInteraction();
        if (!mapDialogue.isOpen) {
            player.Update(deltaTime, window, cameraYaw);
        }
        player.position.y = GetTerrainHeight(player.position.x, player.position.z);
        if (!mapDialogue.isOpen) UpdateCameraInput();
        UpdateFieldEnemies(fieldEnemies, deltaTime);
        for (int i = 0; i < kMaxFieldEnemies; ++i)
            if (fieldEnemies[i].active)
                fieldEnemies[i].position.y = GetTerrainHeight(fieldEnemies[i].position.x, fieldEnemies[i].position.z);
        for (MapNpc& npc : mapNpcs)
            npc.position.y = GetTerrainHeight(npc.position.x, npc.position.z);
    }
    if (!settingsOpen) StartEncounterIfNeeded(combatSystem);

    int fbW = 1280, fbH = 720;
    if (window) glfwGetFramebufferSize(window, &fbW, &fbH);
    if (fbW <= 0 || fbH <= 0) return;
    glViewport(0, 0, fbW, fbH);

    float encProgress = encounterTransitionActive ? Saturate(encounterTransitionTime / encounterTransitionDuration) : 0.0f;
    float encEase = SmoothStep(encProgress);
    glm::vec3 camTarget = player.position + glm::vec3(0, 1, 0);
    if (encounterTransitionActive)
        camTarget = glm::mix(camTarget, encounterTargetPosition + glm::vec3(0,1.15f,0), encEase * 0.55f);

    float camDist = encounterTransitionActive ? glm::mix(cameraDistance, 7.4f, encEase) : cameraDistance;
    glm::vec3 camOff(camDist*std::cos(cameraPitch)*std::sin(cameraYaw),
                     camDist*std::sin(cameraPitch),
                     camDist*std::cos(cameraPitch)*std::cos(cameraYaw));
    glm::vec3 camPos = camTarget + camOff;
    if (encounterTransitionActive) {
        float shake = (1.0f - encEase) * 0.18f;
        camPos.x += std::sin(encounterTransitionTime * 68.0f) * shake;
        camPos.y += std::cos(encounterTransitionTime * 53.0f) * shake * 0.55f;
    }
    glm::mat4 view = glm::lookAt(camPos, camTarget, glm::vec3(0,1,0));
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)fbW/(float)fbH, 0.1f, 500.0f);

    DrawSimpleMap(view, proj, camPos);

    bool moving = player.isMoving();
    bool hasPlayerWalkClip = false;
    if (playerModel.isLoaded() && playerModel.hasAnimation()) {
        const int walkIdx = playerModel.findAnimationByKeywords({
            "walk","run","locomotion","move","jog","sprint","forward","go"
        });
        const int idleIdx = playerModel.findAnimationByKeywords({
            "idle","stand","rest","wait","pose"
        });

        if (moving) {
            if (walkIdx >= 0) {
                playerModel.playAnimationByIndex((size_t)walkIdx);
                hasPlayerWalkClip = true;
            }
            else if (idleIdx >= 0) {
                // 名前不明のクリップを歩行扱いしない。待機姿勢に手続き型の歩行を重ねる。
                playerModel.playAnimationByIndex((size_t)idleIdx);
            }
        }
        else {
            if (idleIdx >= 0) {
                playerModel.playAnimationByIndex((size_t)idleIdx);
            }
            else if (playerModel.getAnimationCount() > 0) {
                playerModel.resetAnimationPose();
            }
        }
        playerModel.updateAnimation(deltaTime);
    }
    UpdateWalkAnimation(playerWalkAnim, deltaTime, moving && !hasPlayerWalkClip);


    // 自然な比率へ
    glm::vec3 pScale = playerModel.isLoaded()
        ? glm::vec3(1.72f, 1.78f, 1.72f)
        : glm::vec3(0.82f, 1.78f, 0.82f);
    glm::vec3 pColor = playerModel.isLoaded() ? glm::vec3(0.92f,0.76f,0.58f) : glm::vec3(0.12f,0.55f,1.0f);
    DrawActor(player.position, pScale, pColor, view, proj, camPos, player.rotationY, &playerModel, moving, &playerWalkAnim);

    for (const MapNpc& npc : mapNpcs) {
        DrawActor(npc.position, glm::vec3(1.05f, 2.0f, 1.05f), npc.color,
                  view, proj, camPos, npc.rotationY, nullptr, false, nullptr);
    }

    UpdateSharedEnemyModels(deltaTime);
    for (int i = 0; i < kMaxFieldEnemies; ++i) {
        if (!fieldEnemies[i].active) continue;
        const ImportedModel* enemyModel = GetSharedEnemyModel(i);
        if (!enemyModel) continue;
        UpdateWalkAnimation(enemyWalkAnim[i], deltaTime, fieldEnemies[i].moving);
        DrawActor(fieldEnemies[i].position, glm::vec3(1.85f,2.08f,1.85f), glm::vec3(0.9f,0.12f,0.1f),
                  view, proj, camPos, fieldEnemies[i].rotationY, enemyModel,
                  fieldEnemies[i].moving, &enemyWalkAnim[i]);
    }
    glBindVertexArray(0);

    DrawEncounterOverlay(fbW, fbH, encProgress);
    RenderNpcPrompt(fbW, fbH);
    RenderFieldMissionHud(fbW, fbH, combatSystem);
    RenderMapDialogue();
    RenderFieldSettings(fbW, fbH, combatSystem);
    if (encounterTransitionActive && encounterTransitionTime >= encounterTransitionDuration) {
        encounterTransitionActive = false;
        combatSystem.resetBattle(pendingEncounterLevel);
        SceneUpdate(Scene::Battle);
    }
}

float getDeltaTime() { return deltaTime; }

glm::vec3 GetBattleWorldOrigin()
{
    // エンカウント地点のワールド座標
    return encounterTargetPosition.x == 0.0f && encounterTargetPosition.z == 0.0f
        ? player.position
        : encounterTargetPosition;
}
