#pragma once
#include <string>

//-宣言--
extern char g_playerName[32] ; // 入力用バッファ
extern bool g_isNamingPhase ;   // 名前入力中かどうか
extern bool g_StoryNeedsLoad;
/// 次にストーリー画面へ入ったとき読み込むイベントID
extern std::string g_CurrentEventID;

void StoryEvent();
/// ID指定で読み込む
void LoadStoryData(std::string);

//ストーリーを更新する処理
void UpdateStory();

//ストーリーを画面に描画
void DrawStory();
//bool IsStoryFinished();

