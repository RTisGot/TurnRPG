#pragma once
#define GLFW_INCLUDE_NONE
#include <imgui.h>
#include <functional>
#include <GLFW/glfw3.h>
#include <string>
#include <vector>

struct Character;

enum class BattleState {
    InProgress,
    Victory,
    Defeat
};

enum class BattleCommand {
    None,
    BasicAttack,
    Skill
};

class CombatSystem
{
public:
    bool isVisible = true;
    std::vector<Character*> participants;
    BattleState battleState = BattleState::InProgress;

    void toggleVisibility();
    void displayTurnOrder();
    void renderUI(int screenWidth, int screenHeight);
    void addParticipant(Character* character);
    void executeSkill(Character* attacker, Character* target);
    void executeGuard(Character* character);
    void resetBattle(int enemyLevel = 1);
    void beginTutorialBattle();
    bool consumeBattleVictory();
    bool consumeBattleDefeat();
    int getScore() const { return totalScore; }
    int getDefeatedEnemyCount() const { return defeatedEnemyCount; }
    int getNextEnemyLevel() const { return nextEnemyLevel; }
    int getDefeatCount() const { return defeatCount; }
    void resetProgression();
    int getAttackItemCount() const { return attackItemCount; }
    int getHealingItemCount() const { return healingItemCount; }
    const std::string& getLastDroppedItem() const { return lastDroppedItem; }
    bool useAttackItem();
    bool useHealingItem();
    void DrawStyledButton(const char* label, const char* desc, ImVec4 color, float width, float height, std::function<void()> onClick);

private:
    enum class TutorialStep {
        None,
        Intro,
        Attack,
        WaitingForAttack,
        Skill,
        Complete
    };

    TutorialStep tutorialStep = TutorialStep::None;
    int currentPoints = 0;
    int maxPoints = 3;
    bool enemyActionQueued = false;
    double enemyActionTime = 0.0;
    bool battleEndQueued = false;
    double battleEndStartTime = 0.0;
    BattleState battleEndResult = BattleState::InProgress;
    bool lastBattleVictory = false;
    bool lastBattleDefeat = false;
    int totalScore = 0;
    int defeatedEnemyCount = 0;
    int nextEnemyLevel = 1;
    int defeatCount = 0;
    int attackItemCount = 0;
    int healingItemCount = 0;
    std::string lastDroppedItem;
    bool isBattleLogOpen = false;
    BattleCommand pendingCommand = BattleCommand::None;
    BattleCommand resolvingCommand = BattleCommand::BasicAttack;
    Character* markedTarget = nullptr;
    // コマンドはアニメーション上のヒットフレームまで確定待ちにする。
    // 攻撃者と対象を保持し、途中のターン順更新で対象が変わることを防ぐ。
    bool playerCommandAnimating = false;
    bool playerCommandHitApplied = false;
    float playerCommandAnimationTime = 0.0f;
    Character* queuedPlayerAttacker = nullptr;
    Character* queuedPlayerTarget = nullptr;
    bool enemyCommandAnimating = false;
    bool enemyCommandHitApplied = false;
    float enemyCommandAnimationTime = 0.0f;
    Character* queuedEnemyAttacker = nullptr;
    Character* queuedEnemyTarget = nullptr;
    std::vector<std::string> battleLog;
    struct DamagePopup {
        Character* target = nullptr;
        int amount = 0;
        bool isCritical = false;
        double startTime = 0.0;
        float xOffset = 0.0f;
    };
    std::vector<DamagePopup> damagePopups;

    void sortTurnOrder();
    void advanceTurn(Character* character);
    void checkBattleState();
    Character* getActiveCharacter();
    Character* getRandomAliveTarget(int isAlly);
    void renderBattleScene(Character* activeChar, int screenWidth, int screenHeight);
    void renderBattleEndOverlay(int screenWidth, int screenHeight);
    void renderBattleCards(Character* activeChar, float screenWidth, float marginX, float marginY, float cardWidth, float cardHeight, float spacingY);
    void renderActionMenu(Character* activeChar, int screenWidth, int screenHeight);
   
    void renderBattleLogWindow(int screenWidth, int screenHeight);
    void chooseCommand(BattleCommand command);
    void executeCommand(Character* attacker, Character* target);
    void addLog(const std::string& text);
    void returnToFieldAfterBattle();
};

struct Skill {
    std::string name;
    int power;
    int cost;
};

void processInput(GLFWwindow* window, CombatSystem& combatSystem);
