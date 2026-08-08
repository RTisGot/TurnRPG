/*
 * nlohmann/json is licensed under the MIT License.
 * Copyright (c) 2013-2022 Niels Lohmann
 * https://github.com/nlohmann/json
 */

#include "StoryEvent.h"
#include "Scene.h"
#include "../../imgui/imgui.h"
#include "../../json.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>


int g_Scene = 0;
bool g_StoryNeedsLoad = false;
static size_t g_currentIndex = 0;
using json = nlohmann::json;
char g_playerName[32] = "プレイヤー"; // 入力用バッファ
std::string g_CurrentEventID = "Intro";
extern bool g_isNamingPhase = false;   // 名前入力中かどうか


struct Message {
	std::string name;   //json側の名前
	std::string text;  //         文字列
};

static std::vector<Message> g_messages;

static void LoadIntroFallback()
{
    g_messages = {
        { u8"潮汐記録", u8"海暦217年。第七防潮壁は、夜明けを待たずに沈黙した。" },
        { u8"ミオ", u8"聞こえる？　次の満潮まで、あと三十分しかない。" },
        { "SYSTEM_NAMING", u8"潜航者の識別名を登録してください。" },
        { u8"ミオ", u8"鐘楼へ向かって。沈んだ日の本当の記録が、そこにある。" },
        { u8"潮汐記録", u8"第一章『水面に残る星』―― 旧交通層へ向かえ。" }
    };
}

static void LoadAfterDistrictFallback()
{
    g_messages = {
        { u8"ミオ", u8"周辺の反応は沈黙した。端末を再起動するね。" },
        { u8"潮汐記録", u8"旧交通層の水路ゲートが解錠された。" },
        { u8"ミオ", u8"南側ドックに避難艇が残っている。次の満潮が来る前に乗って。" },
        { "PLAYER", u8"水路から鐘楼へ向かう。" }
    };
}

static void LoadDepartureFallback()
{
    g_messages = {
        { u8"潮汐記録", u8"避難艇の推進器が低い唸りを上げ、濁った水面を切り開いた。" },
        { u8"ミオ", u8"進路は旧鐘楼。水没区画の奥で、記録信号がまだ脈打っている。" },
        { u8"潮汐記録", u8"第一章・前半終了――次の区画への航路が開かれた。" }
    };
}

static void LoadFallbackFor(const std::string& storyID)
{
    if (storyID == "AfterDistrict") LoadAfterDistrictFallback();
    else if (storyID == "Departure") LoadDepartureFallback();
    else LoadIntroFallback();
}

void StoryEvent() {
    if (g_StoryNeedsLoad) {
        LoadStoryData(g_CurrentEventID);
        g_currentIndex = 0;       //セリフの最初を一行目にコピー  
        g_StoryNeedsLoad = false; // ロード完了
    }

    
    // メッセージウィンドウ表示
    UpdateStory();
}

//セリフデータをメモリにコピー
void LoadStoryData(std::string storyID)
{
    
    const char* candidates[] = {
        "data.json",
        "trnsei/data.json",
        "../trnsei/data.json",
        "../../trnsei/data.json",
        "../../../trnsei/data.json"
    };
    std::ifstream file;
    std::string loadedPath;
    for (const char* path : candidates) {
        file.open(path);
        if (file.is_open()) {
            loadedPath = path;
            break;
        }
        file.clear();
    }

    g_messages.clear();
    if (!file.is_open()) {
        std::cerr << "[Story] data.json not found; using embedded chapter data." << std::endl;
        LoadFallbackFor(storyID);
        return;
    }

    try {
        json data;
        file >> data;
        if (!data.contains("events") || !data["events"].is_array())
            throw std::runtime_error("events array is missing");

        for (const auto& event : data["events"]) {
            if (!event.contains("id") || !event.contains("messages")) continue;
            if (event["id"].get<std::string>() == storyID && event["messages"].is_array()) {
                for (const auto& item : event["messages"]) {
                    if (!item.contains("name") || !item.contains("text")) continue;
                    g_messages.push_back({
                        item["name"].get<std::string>(),
                        item["text"].get<std::string>()
                    });
                }
                break;
            }
        }
        std::cout << "[Story] loaded " << loadedPath << " (" << g_messages.size() << " messages)" << std::endl;
    }
    catch (const std::exception& error) {
        std::cerr << "[Story] invalid chapter data: " << error.what()
            << "; using embedded chapter data." << std::endl;
        g_messages.clear();
    }

    if (g_messages.empty()) {
        LoadFallbackFor(storyID);
    }
}

void UpdateStory()
{
    
    if (g_messages.empty() || g_currentIndex >= g_messages.size()) {
        currentScene = Scene::Field;
        return;
    }
    const auto& msg = g_messages[g_currentIndex];

    // --- 名前入力の判定 ---
    if (msg.name == "SYSTEM_NAMING" && !g_isNamingPhase) {
        g_isNamingPhase = true;
    }

    if (g_isNamingPhase) {
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::Begin("Name Entry", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar);
        ImGui::Text("名前を入力してください");
        ImGui::InputText("##name", g_playerName, IM_ARRAYSIZE(g_playerName));

        if (ImGui::Button("決定", ImVec2(120, 0))) {
            if (strlen(g_playerName) > 0) {
                g_isNamingPhase = false;
                g_currentIndex++;
            }
        }
        ImGui::End();
        return; 
    }

    // --- スタイル設定 ---
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    float windowWidth = screenSize.x * 0.8f;
    float windowHeight = 150.0f;
    ImVec2 pos = ImVec2((screenSize.x - windowWidth) * 0.5f, screenSize.y - windowHeight - 50.0f);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.7f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight));

    // --- セリフウィンドウの表示 ---
    ImGui::Begin("DialogueWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

    // キャラクター名表示のカスタマイズ
    // 名前が "PLAYER" なら入力した名前に置き換える処理を追加
    std::string displayName = msg.name;
    if (displayName == "PLAYER") displayName = g_playerName;

    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "[ %s ]", displayName.c_str());
    ImGui::Dummy(ImVec2(0.0f, 5.0f));

    ImGui::TextWrapped("%s", msg.text.c_str());

    // クリック待ちアイコン
    float alpha = (sinf((float)ImGui::GetTime() * 5.0f) + 1.0f) * 0.5f;
    ImGui::SetCursorPos(ImVec2(windowWidth - 40, windowHeight - 30));
    ImGui::TextColored(ImVec4(1, 1, 1, alpha), "▼");

   
    Scene nextScene = currentScene;
    bool transitionRequested = false;

    // クリック判定
    if (ImGui::IsMouseClicked(0)) {
        if (g_currentIndex + 1 < g_messages.size()) {
            g_currentIndex++;
        }
        else {
            // イベント終了後の処理
            if (g_CurrentEventID == "Intro") {
                nextScene = Scene::Field;
                transitionRequested = true;
            }
            else if (g_CurrentEventID == "Ending") {
                nextScene = Scene::Title;
                transitionRequested = true;
            }
            else {
                nextScene = Scene::Field;
                transitionRequested = true;
            }
        }
    }

    ImGui::End();

    // Popして元に戻す
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(1);

    if (transitionRequested) {
        currentScene = nextScene;
    }
}
void DrawStory()
{

}
