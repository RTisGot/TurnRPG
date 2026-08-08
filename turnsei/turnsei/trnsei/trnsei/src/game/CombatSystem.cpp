#include <glew.h>
#include <GLFW/glfw3.h>
#include "imgui.h"

#include "Character.h"
#include "CombatSystem.h"
#include "Scene.h"
#include "../../assets/ImportedModel.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>
#include <vector>

// --------------------------------------------------
// ターン順表示
// --------------------------------------------------
void CombatSystem::displayTurnOrder()
{
    sortTurnOrder();

    //全員を順番に並べる
    for (size_t i = 0; i < participants.size(); i++)
    {
        if (!participants[i] || participants[i]->currentHp <= 0) continue;//キャラが存在しないか、HPがなかったら返す
        std::cout << (i + 1) << u8": " << participants[i]->name << std::endl;
    }
}
//----------------------------------------------------
//表示を切り替える
void CombatSystem::toggleVisibility()
{
    isVisible = !isVisible;
}

//
//コマンド切り替え
//
static const char* GetCommandName(BattleCommand command)
{
    switch (command) {
    case BattleCommand::BasicAttack: return "Basic";//基本攻撃
    case BattleCommand::Skill: return "Skill";      //スキル
    default: return "None";
    }
}

//
//戦闘画面か分ける
//
void CombatSystem::renderUI(int screenWidth, int screenHeight)
{
    if (!isVisible || participants.empty()) return;

    sortTurnOrder();
    checkBattleState();

    Character* activeChar = getActiveCharacter();//現在行動中のキャラクターのポインタを取得
    if (battleState != BattleState::InProgress) {//戦闘が終わった後の場合
        if (!battleEndQueued) {                  
            battleEndQueued = true;              //戦闘処理を開始
            battleEndStartTime = ImGui::GetTime();//
            battleEndResult = battleState;
            pendingCommand = BattleCommand::None;
            enemyActionQueued = false;
        }

        renderBattleScene(activeChar, screenWidth, screenHeight);//戦闘シーンの描画
        renderBattleEndOverlay(screenWidth, screenHeight);
        if (ImGui::GetTime() - battleEndStartTime >= 2.4) {
            returnToFieldAfterBattle();
        }
        return;
    }
    if (markedTarget && markedTarget->currentHp <= 0) markedTarget = nullptr;
    if (!activeChar || battleState != BattleState::InProgress) {
        pendingCommand = BattleCommand::None;
    }

    renderBattleScene(activeChar, screenWidth, screenHeight);
    renderBattleCards(activeChar, (float)screenWidth, 18.f, 18.f, 210.0f, 48.0f, 5.f);
    renderActionMenu(activeChar, screenWidth, screenHeight);
    renderBattleLogWindow(screenWidth, screenHeight);
}
static bool pKeyWasPressed = false;

//キーボード入力でゲームの処理を行う関数
void processInput(GLFWwindow* window, CombatSystem& combatSystem) {
    if (currentScene != Scene::Field &&
        glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        //ウィンドウを閉じる処理
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        if (!pKeyWasPressed) {
            combatSystem.toggleVisibility();
            pKeyWasPressed = true;
        }
    }
    else {
        pKeyWasPressed = false;
    }
}
//戦闘に参加するキャラクターを追加する
void CombatSystem::addParticipant(Character* character) {
    if (character != nullptr) {
        participants.push_back(character);
    }
}
//----------
//スキル攻撃の処理
//----
void CombatSystem::executeSkill(Character* attacker, Character* target)
{
    if (!attacker || !target ||
        battleState != BattleState::InProgress)
    {
        return;
    }

    bool brokeThisHit = false;

    if (target->isAlly == 0 &&
        target->maxTideguard > 0 &&
        !target->isBroken)
    {
        int pressureDamage =
            resolvingCommand == BattleCommand::Skill ? 2 : 1;

        target->tideguard =
            std::max(0, target->tideguard - pressureDamage);

        if (target->tideguard == 0)
        {
            target->isBroken = true;

            // ブレイク時は行動順を遅らせる
            target->turnGauge -= 24;

            brokeThisHit = true;

            addLog(target->name + " RESISTANCE BROKEN");
        }
    }

    // 防御力を考慮してダメージを計算
    int damage =
        attacker->power + attacker->attackBuffAmount - (target->defense / 2);

    if (damage < 1)
    {
        damage = 1;
    }

  
    const bool isCritical =
        (rand() % 100) < attacker->critical;

    if (isCritical)
    {
        damage =
            damage * attacker->criticalDamage / 100;
    }

    // ガード1回の攻撃のみ有効
    if (target->isGuarding)
    {
        damage /= 2;

        if (damage < 1)
        {
            damage = 1;
        }

        target->isGuarding = false;
    }

    // ブレイク中受けるダメージを増加させる
    if (target->isBroken)
    {
        damage = damage * 135 / 100;

        if (!brokeThisHit)
        {
            target->isBroken = false;
            target->tideguard = target->maxTideguard;
        }
    }

    target->currentHp -= damage;

    if (target->currentHp < 0)
    {
        target->currentHp = 0;
    }

    // 画面上に表示するダメージポップアップを登録
    DamagePopup popup;
    popup.target = target;
    popup.amount = damage;
    popup.isCritical = isCritical;
    popup.startTime = ImGui::GetTime();

    // 同時に表示された数値が完全に重ならないよう、
    // 水平方向へ少しランダムにずらす。
    popup.xOffset =
        static_cast<float>((rand() % 41) - 20);

    damagePopups.push_back(popup);

    std::string log =
        attacker->name +
        " attacked " +
        target->name +
        " for " +
        std::to_string(damage) +
        " damage";

    if (isCritical)
    {
        log += " (critical)";
    }

    addLog(log);
    std::cout << log << std::endl;

    if (target->currentHp == 0)
    {
        addLog(target->name + " defeated");
        std::cout
            << target->name
            << " defeated"
            << std::endl;
    }

    if (attacker->isAlly == 1 && attacker->attackBuffTurns > 0) {
        --attacker->attackBuffTurns;
        if (attacker->attackBuffTurns == 0) attacker->attackBuffAmount = 0;
    }
    advanceTurn(attacker);
    checkBattleState();
}

void CombatSystem::executeGuard(Character* character)
{
    if (!character || battleState != BattleState::InProgress) return;
    character->isGuarding = true;
    addLog(character->name + " guarded");
    advanceTurn(character);
}
void CombatSystem::chooseCommand(BattleCommand command)
{
    pendingCommand = command;
}

void CombatSystem::executeCommand(Character* attacker, Character* target)
{
    if (!attacker || !target || pendingCommand == BattleCommand::None) return;

    BattleCommand command = pendingCommand;
    pendingCommand = BattleCommand::None;

    int originalPower = attacker->power;
    if (command == BattleCommand::Skill) {
        attacker->power += 10;
    }
    if (command == BattleCommand::BasicAttack) {
        attacker->charge = std::min(attacker->charge + 1, attacker->maxCharge);
    }

    addLog(attacker->name + " used " + GetCommandName(command));
    resolvingCommand = command;
    executeSkill(attacker, target);
    resolvingCommand = BattleCommand::BasicAttack;

    attacker->power = originalPower;
    if (target->currentHp <= 0 && markedTarget == target) markedTarget = nullptr;
}

//勝利フラグ
bool CombatSystem::consumeBattleVictory()
{
    bool won = lastBattleVictory;
    lastBattleVictory = false;
    return won;
}

bool CombatSystem::consumeBattleDefeat()
{
    const bool lost = lastBattleDefeat;
    lastBattleDefeat = false;
    return lost;
}

void CombatSystem::beginTutorialBattle()
{
    tutorialStep = TutorialStep::Intro;
}

void CombatSystem::resetProgression()
{
    totalScore = 0;
    defeatedEnemyCount = 0;
    nextEnemyLevel = 1;
    defeatCount = 0;
    attackItemCount = 0;
    healingItemCount = 0;
    lastDroppedItem.clear();
}

bool CombatSystem::useAttackItem()
{
    if (battleState != BattleState::InProgress || attackItemCount <= 0) return false;
    for (Character* character : participants) {
        if (character && !character->isEnemy()) {
            character->attackBuffAmount = 10;
            character->attackBuffTurns = 3;
        }
    }
    --attackItemCount;
    return true;
}

bool CombatSystem::useHealingItem()
{
    if (battleState != BattleState::InProgress || healingItemCount <= 0) return false;
    bool healed = false;
    for (Character* character : participants) {
        if (character && !character->isEnemy() && character->currentHp < character->hp) {
            character->currentHp = std::min(character->hp, character->currentHp + 60);
            healed = true;
        }
    }
    if (healed) --healingItemCount;
    return healed;
}

void CombatSystem::resetBattle(int enemyLevel)
{
    enemyLevel = std::max(1, enemyLevel);
    // タイトルのデバッグ用戦闘入口に依存せず、
    // フィールドのエンカウントごとに戦闘UIを必ず表示する。
    isVisible = true;
    for (auto it = participants.begin(); it != participants.end(); ) {
        if (*it && (*it)->isAlly == 0) {
            delete *it;
            it = participants.erase(it);
        }
        else {
            ++it;
        }
    }

    const char* enemyNames[] = { u8"漂流殻", u8"錆潮の番兵", u8"溺光クラゲ" };
    const char* affinities[] = { u8"帯電", u8"腐食", u8"水圧" };
    int enemyHp[]      = { 86 + rand() % 10, 104 + rand() % 10, 72 + rand() % 10};
    int enemyPower[]    = { 7 + rand() % 5, 8 + rand() % 5, 6 + rand() % 5};
    int enemyDefense[]  = { 2, 7, 4 };
    int enemySpeed[]    = { 10, 8, 14 };
    int enemyCrit[]     = { 5, 8, 10 };
    int enemyCritDmg[]  = { 130, 140, 145 };

    int enemyCount = 1 + rand() % 3;
    for (int i = 0; i < enemyCount; ++i) {
        const int hpGrowth = (enemyLevel - 1) * 22;
        const int powerGrowth = (enemyLevel - 1) * 3;
        const int defenseGrowth = (enemyLevel - 1) * 2;
        const int speedGrowth = (enemyLevel - 1) / 3;
        Character* enemy = new Character{
            enemyNames[i], enemyHp[i] + hpGrowth, enemyPower[i] + powerGrowth,
            enemyDefense[i] + defenseGrowth, enemySpeed[i] + speedGrowth,
            enemyCrit[i] + (enemyLevel - 1) / 4, enemyCritDmg[i],
            enemyHp[i] + hpGrowth, 0
        };
        enemy->maxTideguard = 3 + i;
        enemy->tideguard = enemy->maxTideguard;
        enemy->affinity = affinities[i];
        enemy->level = enemyLevel;
        enemy->experienceReward = 30 + enemyLevel * 10 + i * 8;
        enemy->scoreReward = enemyLevel * 100 + i * 40;
        addParticipant(enemy);
    }

    for (auto* c : participants) {
        if (!c) continue;
        if (c->isEnemy()) c->currentHp = c->hp;
        else if (c->currentHp <= 0) c->currentHp = std::max(1, c->hp / 3);
        else c->currentHp = std::min(c->currentHp, c->hp);
        c->isGuarding = false;
        c->turnGauge = c->speed;
        c->charge = 0;
        if (c->isAlly == 0) {
            c->tideguard = c->maxTideguard;
            c->isBroken = false;
        }
    }
    if (tutorialStep == TutorialStep::Intro) {
        // 初回チュートリアルは必ずプレイヤーから操作を始める。
        for (auto* c : participants) {
            if (c && c->isAlly == 1) c->turnGauge = 200;
        }
    }
    battleState = BattleState::InProgress;
    enemyActionQueued = false;
    enemyActionTime = 0.0;
    battleEndQueued = false;
    battleEndStartTime = 0.0;
    battleEndResult = BattleState::InProgress;
    lastBattleVictory = false;
    lastBattleDefeat = false;
    pendingCommand = BattleCommand::None;
    playerCommandAnimating = false;
    playerCommandHitApplied = false;
    playerCommandAnimationTime = 0.0f;
    queuedPlayerAttacker = nullptr;
    queuedPlayerTarget = nullptr;
    enemyCommandAnimating = false;
    enemyCommandHitApplied = false;
    enemyCommandAnimationTime = 0.0f;
    queuedEnemyAttacker = nullptr;
    queuedEnemyTarget = nullptr;
    markedTarget = nullptr;
    damagePopups.clear();
    battleLog.clear();
    addLog("Battle start");
    sortTurnOrder();
}

void CombatSystem::returnToFieldAfterBattle()
{
    bool won = battleEndResult == BattleState::Victory;

    int earnedExperience = 0;
    if (won) {
        for (const Character* character : participants) {
            if (character && character->isEnemy()) {
                earnedExperience += character->experienceReward;
                totalScore += character->scoreReward;
                ++defeatedEnemyCount;
            }
        }
        for (Character* character : participants) {
            if (character && !character->isEnemy()) {
                character->gainExperience(earnedExperience);
            }
        }
        nextEnemyLevel += 1 + rand() % 3;
        const int dropRoll = rand() % 100;
        if (dropRoll < 40) {
            ++attackItemCount;
            lastDroppedItem = u8"攻撃強化アンプル";
        }
        else if (dropRoll < 80) {
            ++healingItemCount;
            lastDroppedItem = u8"回復キット";
        }
        else {
            lastDroppedItem = u8"ドロップなし";
        }
    }

    for (auto it = participants.begin(); it != participants.end(); ) {
        if (*it && (*it)->isAlly == 0) {
            delete *it;
            it = participants.erase(it);
        }
        else {
            if (*it) {
                if (!won && (*it)->currentHp <= 0)
                    (*it)->currentHp = std::max(1, (*it)->hp / 3);
                (*it)->isGuarding = false;
                (*it)->turnGauge = (*it)->speed;
                (*it)->charge = 0;
            }
            ++it;
        }
    }

    battleState = BattleState::InProgress;
    enemyActionQueued = false;
    enemyActionTime = 0.0;
    battleEndQueued = false;
    battleEndStartTime = 0.0;
    battleEndResult = BattleState::InProgress;
    lastBattleVictory = won;
    lastBattleDefeat = !won;
    if (!won) ++defeatCount;
    pendingCommand = BattleCommand::None;
    playerCommandAnimating = false;
    playerCommandHitApplied = false;
    playerCommandAnimationTime = 0.0f;
    queuedPlayerAttacker = nullptr;
    queuedPlayerTarget = nullptr;
    enemyCommandAnimating = false;
    enemyCommandHitApplied = false;
    enemyCommandAnimationTime = 0.0f;
    queuedEnemyAttacker = nullptr;
    queuedEnemyTarget = nullptr;
    markedTarget = nullptr;
    damagePopups.clear();
    battleLog.clear();
    currentPoints = 0;
    currentScene = Scene::Field;
}

//
// 生存しているキャラクターの行動ゲージが高い順に並べ替える。
void CombatSystem::sortTurnOrder()
{
    std::sort(
        participants.begin(),
        participants.end(),
        [](Character* a, Character* b)
        {
            if (!a) return false;
            if (!b) return true;

            // 生存キャラクターを戦闘不能キャラクターより前に並べる。
            if ((a->currentHp > 0) != (b->currentHp > 0))
            {
                return a->currentHp > 0;
            }

            // 行動ゲージが高いキャラクターほど先に行動する。
            return a->turnGauge > b->turnGauge;
        });
}


// 指定キャラクターの行動終了処理を行い、
// 次のターンへ進めるために行動ゲージを更新する。
void CombatSystem::advanceTurn(Character* character)
{
    if (!character)
    {
        return;
    }
    // 行動したキャラクターのゲージを消費する。
    character->turnGauge -= 100;

    // 敵行動の待機状態をリセットする。
    enemyActionQueued = false;
    enemyActionTime = 0.0;

    bool allSlow = true;

    // 誰かの行動ゲージが残っている場合は、
    // そのまま次の行動キャラクターへ進む。
    for (auto* c : participants)
    {
        if (c && c->currentHp > 0 && c->turnGauge > 0)
        {
            allSlow = false;
            break;
        }
    }

    // 全員のゲージが尽きた場合のみ、
    // 生存キャラクターへ速度分のゲージを加算する。
    if (allSlow)
    {
        for (auto* c : participants)
        {
            if (c && c->currentHp > 0)
            {
                c->turnGauge += c->speed;
            }
        }
    }

    sortTurnOrder();
}


// 生存している味方と敵を確認し、
// 勝利または敗北状態へ更新する。
void CombatSystem::checkBattleState()
{
    bool hasAlly = false;
    bool hasEnemy = false;

    for (auto* c : participants)
    {
        if (!c || c->currentHp <= 0)
        {
            continue;
        }

        if (c->isAlly == 1)
        {
            hasAlly = true;
        }
        else
        {
            hasEnemy = true;
        }
    }

    // 敵が全滅していれば勝利、味方が全滅していれば敗北とする。
    if (!hasEnemy)
    {
        battleState = BattleState::Victory;
    }
    else if (!hasAlly)
    {
        battleState = BattleState::Defeat;
    }
}


// 現在行動可能な先頭キャラクター
// 生存キャラクターがいない場合は nullptr を返す
Character* CombatSystem::getActiveCharacter()
{
    sortTurnOrder();

    for (auto* c : participants)
    {
        if (c && c->currentHp > 0)
        {
            return c;
        }
    }

    return nullptr;
}


// 指定された陣営から、生存しているキャラクターをランダムに1体選ぶ
Character* CombatSystem::getRandomAliveTarget(int isAlly)
{
    std::vector<Character*> targets;

    for (auto* c : participants)
    {
        if (c &&
            c->isAlly == isAlly &&
            c->currentHp > 0)
        {
            targets.push_back(c);
        }
    }

    if (targets.empty())
    {
        return nullptr;
    }

    return targets[rand() % targets.size()];
}


// 戦闘ログに新しいメッセージを追加
void CombatSystem::addLog(const std::string& text)
{
    battleLog.insert(
        battleLog.begin(),
        text);

    if (battleLog.size() > 5)
    {
        battleLog.pop_back();
    }
}
