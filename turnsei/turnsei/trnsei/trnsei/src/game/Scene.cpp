#include <glew.h>
#include "Scene.h"
#include "Title.h"
#include "StoryEvent.h"
#include "CombatSystem.h"
#include "Field.h"
#include "Player.h"
#include "imgui.h"
#include <iostream>



Player g_Player;

//シーン遷移を管理する

//現在のシーンを保持する変数(Title)
Scene currentScene = Scene::Title;

void SceneUpdate(Scene nextScene)
{
	currentScene = nextScene;//次の画面へ遷移
}

static void ResultUpdate(int screenWidth, int screenHeight, GLFWwindow* window)
{
	static double gameOverShownAt = -1.0;
	if (gameOverShownAt < 0.0) gameOverShownAt = glfwGetTime();
	glDisable(GL_DEPTH_TEST);
	glClearColor(0.015f, 0.018f, 0.030f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	drawList->AddRectFilled(
		ImVec2(0.0f, 0.0f),
		ImVec2((float)screenWidth, (float)screenHeight),
		IM_COL32(4, 8, 20, 255)
	);
	drawList->AddRectFilled(
		ImVec2(0.0f, (float)screenHeight * 0.58f),
		ImVec2((float)screenWidth, (float)screenHeight),
		IM_COL32(18, 30, 45, 255)
	);

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoBackground;
	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2((float)screenWidth, (float)screenHeight), ImGuiCond_Always);
	if (ImGui::Begin("##GameOver", nullptr, flags)) {
		ImGui::SetCursorPosY((float)screenHeight * 0.34f);
		const char* title = "GAME OVER";
		float titleWidth = ImGui::CalcTextSize(title).x;
		ImGui::SetCursorPosX(((float)screenWidth - titleWidth) * 0.5f);
		ImGui::TextColored(ImVec4(1.0f, 0.30f, 0.34f, 1.0f), "%s", title);

		ImGui::Spacing();
		const char* message = "Defeated three times";
		float messageWidth = ImGui::CalcTextSize(message).x;
		ImGui::SetCursorPosX(((float)screenWidth - messageWidth) * 0.5f);
		ImGui::TextColored(ImVec4(0.92f, 0.95f, 1.0f, 1.0f), "%s", message);

		ImGui::SetCursorPosY((float)screenHeight * 0.54f);
		const float buttonWidth = 280.0f;
		ImGui::SetCursorPosX(((float)screenWidth - buttonWidth) * 0.5f);
		if (ImGui::Button(u8"タイトルに戻る", ImVec2(buttonWidth, 48.0f))) {
			ResetFieldProgress();
			gameOverShownAt = -1.0;
			currentScene = Scene::Title;
		}

		ImGui::SetCursorPosX(((float)screenWidth - buttonWidth) * 0.5f);
		if (ImGui::Button(u8"ゲームを終了する", ImVec2(buttonWidth, 48.0f))) {
			if (window) glfwSetWindowShouldClose(window, GLFW_TRUE);
		}
	}
	ImGui::End();

	if (glfwGetTime() - gameOverShownAt >= 3.5) {
		gameOverShownAt = -1.0;
		currentScene = Scene::Title;
	}
}

void MainUpdate(CombatSystem& combatSystem, GLFWwindow* window) {
	int screenWidth = 1280;
	int screenHeight = 720;
	if (window) {
		glfwGetWindowSize(window, &screenWidth, &screenHeight);

		const bool altPressed =
			glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
			glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;
		const bool fieldHasFocus =
			currentScene == Scene::Field &&
			glfwGetWindowAttrib(window, GLFW_FOCUSED) == GLFW_TRUE;
		const int desiredCursorMode = fieldHasFocus && !altPressed && !IsFieldSettingsOpen()
			? GLFW_CURSOR_DISABLED
			: GLFW_CURSOR_NORMAL;

		if (glfwGetInputMode(window, GLFW_CURSOR) != desiredCursorMode) {
			glfwSetInputMode(window, GLFW_CURSOR, desiredCursorMode);
			// カーソルモード切り替え時の座標差でカメラが跳ねるのを防ぐ。
			ImGui::GetIO().MouseDelta = ImVec2(0.0f, 0.0f);
		}
	}

	switch (currentScene) {
	case Scene::Title:
		TitleUpdate();
		break;
	case Scene::StoryEvent:
		StoryEvent();
		break;
	case Scene::Field:
		FieldUpdate(combatSystem);
		break;
	case Scene::Battle:
		combatSystem.renderUI(screenWidth, screenHeight);
		break;
	case Scene::Result:
		ResultUpdate(screenWidth, screenHeight, window);
		break;
	}
}
