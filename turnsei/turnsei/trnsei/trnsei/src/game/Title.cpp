#include "Title.h"

#include "imgui.h"
#include "Scene.h"

#include <cmath>
#include <cstdlib>
// ストーリー画面へ遷移した際に、イベントデータを
// 再読み込みする必要があるかを示すフラグ。
extern bool g_StoryNeedsLoad;

namespace
{
    // タイトル画面の背景描画
    void DrawTitleBackdrop(ImDrawList* drawList, const ImVec2& screenSize)
    {
     
        drawList->AddRectFilledMultiColor(
            ImVec2(0.0f, 0.0f),
            screenSize,
            IM_COL32(3, 13, 23, 255),
            IM_COL32(5, 30, 43, 255),
            IM_COL32(5, 18, 30, 255),
            IM_COL32(1, 8, 17, 255));

      
        const float horizon = screenSize.y * 0.58f;

        drawList->AddRectFilledMultiColor(
            ImVec2(0.0f, horizon),
            screenSize,
            IM_COL32(6, 47, 59, 245),
            IM_COL32(8, 55, 66, 245),
            IM_COL32(1, 12, 25, 255),
            IM_COL32(1, 10, 20, 255));

     
        for (int i = 0; i < 18; ++i)
        {
            const float x =
                screenSize.x * (0.02f + i * 0.058f);

            const float width =
                screenSize.x * (0.025f + (i % 4) * 0.007f);

            const float height =
                screenSize.y * (0.10f + (i % 5) * 0.035f);

            drawList->AddRectFilled(
                ImVec2(x, horizon - height),
                ImVec2(x + width, horizon),
                IM_COL32(5, 20, 29, 235));

            // 一部の建物にアンテナ状の突起を追加し、
            // シルエットが単調になるのを防ぐ。
            if (i % 3 == 0)
            {
                drawList->AddRectFilled(
                    ImVec2(
                        x + width * 0.44f,
                        horizon - height - screenSize.y * 0.035f),
                    ImVec2(
                        x + width * 0.56f,
                        horizon - height),
                    IM_COL32(7, 22, 31, 225));
            }

            // 建物の反射は下に向かうほど透明にする。
            drawList->AddRectFilledMultiColor(
                ImVec2(x, horizon),
                ImVec2(x + width, horizon + height * 0.55f),
                IM_COL32(10, 38, 48, 105),
                IM_COL32(10, 38, 48, 105),
                IM_COL32(4, 15, 26, 0),
                IM_COL32(4, 15, 26, 0));
        }

        const float time =
            static_cast<float>(ImGui::GetTime());

       
        for (int i = 0; i < 9; ++i)
        {
            const float y =
                horizon
                + 18.0f
                + i * 19.0f
                + std::sin(time * 0.45f + i) * 3.0f;

            const float halfWidth =
                screenSize.x * (0.18f + i * 0.045f);

            drawList->AddLine(
                ImVec2(screenSize.x * 0.5f - halfWidth, y),
                ImVec2(screenSize.x * 0.5f + halfWidth, y),
                IM_COL32(72, 206, 213, 28 + i * 4),
                1.0f);
        }

        
        const ImVec2 beacon(
            screenSize.x * 0.78f,
            screenSize.y * 0.25f);


        for (int radius = 70; radius > 8; radius -= 8)
        {
            drawList->AddCircleFilled(
                beacon,
                static_cast<float>(radius),
                IM_COL32(
                    92,
                    226,
                    218,
                    2 + (70 - radius) / 2));
        }

        drawList->AddCircleFilled(
            beacon,
            7.0f,
            IM_COL32(205, 255, 242, 235));

        // ビーコンから水面へ伸びる光の筋。
        drawList->AddLine(
            beacon,
            ImVec2(beacon.x, horizon + 90.0f),
            IM_COL32(106, 232, 224, 75),
            1.0f);
    }
}

// タイトル画面の背景とメニューUIを更新・描画
void TitleUpdate()
{
    const ImVec2 screenSize =
        ImGui::GetIO().DisplaySize;

    ImDrawList* backgroundDrawList =
        ImGui::GetBackgroundDrawList();

    DrawTitleBackdrop(
        backgroundDrawList,
        screenSize);

    // ウィンドウ。
    const ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoDecoration
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoBackground;

    ImGui::SetNextWindowPos(
        ImVec2(0.0f, 0.0f),
        ImGuiCond_Always);

    ImGui::SetNextWindowSize(
        screenSize,
        ImGuiCond_Always);

    ImGui::Begin(
        "##TideglassTitle",
        nullptr,
        windowFlags);

    // タイトルやメニュー配置
    const float left =
        screenSize.x * 0.095f;

    ImGui::SetCursorPos(
        ImVec2(left, screenSize.y * 0.18f));

    ImGui::TextColored(
        ImVec4(0.70f, 0.96f, 0.95f, 1.0f),
        "T I D E G L A S S");

    ImGui::SetCursorPosX(left);

    ImGui::TextColored(
        ImVec4(0.92f, 0.97f, 1.0f, 1.0f),
        u8"潮鏡都市 ― 沈んだ空の残響");

    ImGui::SetCursorPos(
        ImVec2(left, screenSize.y * 0.34f));

    ImGui::TextColored(
        ImVec4(0.58f, 0.76f, 0.80f, 1.0f),
        u8"海面下に眠る記憶を回収し、都市の最後の夜を取り戻せ。");

    ImGui::SetCursorPos(
        ImVec2(left, screenSize.y * 0.56f));

    // タイトルメニューボタン適用
    ImGui::PushStyleVar(
        ImGuiStyleVar_FrameRounding,
        2.0f);

    ImGui::PushStyleVar(
        ImGuiStyleVar_FrameBorderSize,
        1.0f);

    ImGui::PushStyleColor(
        ImGuiCol_Button,
        ImVec4(0.03f, 0.16f, 0.20f, 0.90f));

    ImGui::PushStyleColor(
        ImGuiCol_ButtonHovered,
        ImVec4(0.06f, 0.31f, 0.35f, 0.95f));

    ImGui::PushStyleColor(
        ImGuiCol_ButtonActive,
        ImVec4(0.12f, 0.43f, 0.45f, 1.0f));

    ImGui::PushStyleColor(
        ImGuiCol_Border,
        ImVec4(0.31f, 0.78f, 0.78f, 0.75f));

    if (ImGui::Button(
        u8"物語をはじめる",
        ImVec2(320.0f, 54.0f)))
    {
        // ストーリー画面へ遷移
        currentScene = Scene::StoryEvent;
        g_StoryNeedsLoad = true;
    }

    ImGui::SetCursorPosX(left);

    if (ImGui::Button(
        u8"終了",
        ImVec2(320.0f, 42.0f)))
    {
        std::exit(0);
    }

    // Push必ず元に戻す
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(2);

    ImGui::SetCursorPos(
        ImVec2(left, screenSize.y - 48.0f));

    ImGui::TextColored(
        ImVec4(0.38f, 0.58f, 0.62f, 1.0f),
        u8"WASD 移動  /  マウス カメラ  /  Alt カーソル  /  E 会話  /  F11 全画面");

    ImGui::End();
}
