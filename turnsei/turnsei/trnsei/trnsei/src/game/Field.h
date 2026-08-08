#pragma once
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "../../assets/Shader.h"
#include <glm/glm.hpp>

class CombatSystem;

extern Shader* fieldShader;
float getDeltaTime();
void FieldUpdate(CombatSystem& combatSystem);
void FieldInit();
void ResetFieldProgress();
bool IsFieldSettingsOpen();
// 戦闘開始地点
glm::vec3 GetBattleWorldOrigin();
