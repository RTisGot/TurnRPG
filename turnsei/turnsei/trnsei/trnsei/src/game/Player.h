#pragma once
#define GLFW_INCLUDE_NONE
#include <glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <vector>

//Playerのクラス
class Player {
public:
    glm::vec3 position;
    float rotationY;

    // 初期化
    void Init();
    // 描画
    void Draw(GLuint shaderProgram, glm::mat4, glm::mat4);

    // 毎フレームの更新（移動処理）
    void Update(float deltaTime, GLFWwindow* window, float cameraYaw = 0.0f);
    bool isMoving() const { return moving; }
private:
    
    GLuint vao, vbo, ebo;
    int indexCount;
    bool moving = false;
    glm::vec3 moveVelocity = glm::vec3(0.0f);
};
