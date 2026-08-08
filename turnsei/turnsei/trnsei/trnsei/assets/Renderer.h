#pragma once
#include "Shader.h"
#include "../src/game/Character.h"

#include<glm/glm.hpp>
#include <iostream>
#include <string>
#include <vector>
#include<map>

// フォント描画用文字情報
struct Glyph {
    unsigned int TextureID; // 文字テクスチャのID
    glm::ivec2   Size;      // 文字のサイズ
    glm::ivec2   Bearing;   // ベースラインからのオフセット
    unsigned int Advance;   // 次の文字へのオフセット
};

class Renderer {
public:
    //  初期化と基本設定
   Renderer();
    ~Renderer();
   
    
    void drawText(std::string text, float x, float y, float scale, glm::vec3 color);

private:
    // テキスト描画用データ
    unsigned int textVAO, textVBO;
    Shader textShader; //TextShader
    
    // ウィンドウサイズ情報
    int screenWidth;
    int screenHeight;
    glm::mat4 projection; // UI用の正射影行列
};

// フォント描画用のtexture構造体
struct Text {
    unsigned int TextureID; // 文字のテクスチャID
    glm::ivec2   Size;      // 文字のサイズ
    glm::ivec2   Bearing;   // ベースラインから左上へのオフセット
    unsigned int Advance;   // 次の文字までの距離
};
extern std::map<char, Text> Texts;