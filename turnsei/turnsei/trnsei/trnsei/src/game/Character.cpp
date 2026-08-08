#include "Character.h"

bool Character::isEnemy() const
{
    return isAlly == 0;
}

int Character::experienceToNextLevel() const
{
    return 60 + (level - 1) * 40;
}

bool Character::gainExperience(int amount)
{
    if (amount <= 0 || isEnemy()) return false;

    experience += amount;
    bool leveledUp = false;
    while (experience >= experienceToNextLevel()) {
        experience -= experienceToNextLevel();
        ++level;
        hp += 18;
        power += 4;
        defense += 3;
        speed += 1;
        currentHp = hp;
        leveledUp = true;
    }
    return leveledUp;
}
