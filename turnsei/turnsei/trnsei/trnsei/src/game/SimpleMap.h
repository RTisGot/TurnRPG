#pragma once
#include <glm/glm.hpp>

// マップ描画に必要なリソースを初期化
void InitSimpleMap();

// マップ全体を描画
void DrawSimpleMap(
    const glm::mat4& view,
    const glm::mat4& projection,
    const glm::vec3& cameraPos);

// 指定座標の地形の高さを返す。
float GetTerrainHeight(float x, float z);



// `from`    : 現在位置
// `desired` : 移動後の希望位置
// `radius`  : 当たり判定半径
glm::vec3 ResolveMapCollision(
    const glm::vec3& from,
    const glm::vec3& desired,
    float radius);

// 指定位置の移動速度倍率
float GetMapMoveSpeedMultiplier(const glm::vec3& position);

// 地面の種類
enum class MapSurfaceType
{
    Soil,          // 土
    WetAsphalt,    // 濡れたアスファルト
    Concrete,      // コンクリート
    ShallowWater   // 浅瀬
};

// 地面ごとの特性
struct MapSurfaceProperties
{
    // 地面の種類
    MapSurfaceType type = MapSurfaceType::Soil;

    // 移動速度倍率
    float speedMultiplier = 1.0f;

    // 地面の滑りにくさ
    float traction = 0.82f;

    // 移動速度に加える減衰率
    float linearDrag = 0.10f;
};

// 指定位置の地面情報の取得
MapSurfaceProperties GetMapSurfaceProperties(
    const glm::vec3& position);