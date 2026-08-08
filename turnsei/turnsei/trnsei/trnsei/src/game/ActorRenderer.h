#pragma once
#include <glew.h>
#include <glm/glm.hpp>

class ImportedModel;

// 歩行アニメーション再生状態を保持
struct WalkAnimState
{
    // アニメーション経過時間（秒）
    float time = 0.0f;

    // 歩行モーションへのブレンド率（0.0～1.0）
    float blend = 0.0f;
};

// キャラクター描画用GPUリソースを初期化
void InitActorRenderer();

// キャラクター描画用シェーダープログラム
GLuint GetActorShader();

// キャラクターを描画
//
// isWalking と walkAnimを指定で歩行アニメーションを適用
void DrawActor(
    const glm::vec3& position,
    const glm::vec3& scale,
    const glm::vec3& color,
    const glm::mat4& view,
    const glm::mat4& projection,
    const glm::vec3& cameraPosition,
    float rotationY,
    const ImportedModel* importedModel = nullptr,
    bool isWalking = false,
    WalkAnimState* walkAnim = nullptr);

// 歩行状態に応じてアニメーション状態を更新
//
// deltaTime 前フレームからの経過時間（秒）
void UpdateWalkAnimation(
    WalkAnimState& state,
    float deltaTime,
    bool isWalking);