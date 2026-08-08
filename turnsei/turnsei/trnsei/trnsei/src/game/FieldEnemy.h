#pragma once
#include <glm/glm.hpp>

// フィールド上を移動する敵キャラクター状態を保持
struct FieldEnemy
{
    // 現在のワールド座標
    glm::vec3 position = glm::vec3(0.0f);

    // 次に移動する目標座標
    glm::vec3 targetPos = glm::vec3(0.0f);

    // Y軸の向き（ラジアン）
    float rotationY = 0.0f;

    // 次の移動先決定までの経過時間
    float wanderTimer = 0.0f;

    // 移動先を再決定する間隔（秒）
    float wanderInterval = 3.0f;

  
    float speed = 1.5f;

    // フィールド上で有効か
    bool active = true;

    // 現在移動中か
    bool moving = false;
};

// フィールド上に配置できる敵の最大数
constexpr int kMaxFieldEnemies = 4;

// フィールド敵を初期状態に設定する
void InitFieldEnemies(FieldEnemy* enemies);

// フィールド敵の移動や行動を更新
//
// `dt` は前フレームからの経過時間（秒）
void UpdateFieldEnemies(FieldEnemy* enemies, float dt);