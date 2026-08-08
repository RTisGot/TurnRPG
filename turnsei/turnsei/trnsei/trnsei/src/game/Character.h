#pragma once


#include<string>
#include<map>


// プレイヤー・味方・敵を表す戦闘用キャラクター
//
// 戦闘で使用するステータスやゲージ、状態異常などの情報を保持
struct Character
{
    // キャラクター名
    std::string name;


    int hp;

    
    int power;

    
    int defense;

    // 行動順を決定する素早さ
    int speed;

    // クリティカル発生率（%）
    int critical;

    // クリティカル時のダメージ倍率（%）
    int criticalDamage;

 
    int currentHp;

    // 味方なら1、敵なら0
    int isAlly;

    int level = 1;
    int experience = 0;
    int experienceReward = 0;
    int scoreReward = 0;
    int attackBuffAmount = 0;
    int attackBuffTurns = 0;

    // ガード状態
    bool isGuarding = false;

    // 行動順計算に使用するゲージ
    int turnGauge = 0;

    // スキルチャージ量
    int charge = 0;

    // 最大チャージ量
    int maxCharge = 5;

    // 耐性ゲージ
    int tideguard = 0;

    // 最大耐性値
    int maxTideguard = 0;

    // ブレイク状態かどうか
    bool isBroken = false;

    // 属性名
    std::string affinity = "Tide";

    // 敵キャラクターなら true を返す
    bool isEnemy() const;
    int experienceToNextLevel() const;
    bool gainExperience(int amount);
};
