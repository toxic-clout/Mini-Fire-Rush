#include <iostream>
#include <vector>
#include <string>

class BulletObject {
public:
    BulletObject(int damage) : damage(damage) {}
    int getDamage() const { return damage; }
private:
    int damage;
};

class Weapon {
public:
    Weapon(std::string name, int damage) : name(name), bullet(damage) {}
    void fire() {
        std::cout << "Firing " << name << " with damage: " << bullet.getDamage() << std::endl;
    }
private:
    std::string name;
    BulletObject bullet;
};

class GamePlayer {
public:
    GamePlayer(std::string playerName) : name(playerName), health(100) {}
    void takeDamage(int amount) {
        health -= amount;
        std::cout << name << " takes " << amount << " damage! Health: " << health << std::endl;
    }
private:
    std::string name;
    int health;
};

class GameMaster {
public:
    void startGame() {
        std::cout << "Game Started!" << std::endl;
    }
    void endGame() {
        std::cout << "Game Over!" << std::endl;
    }
};

int main() {
    GameMaster gameMaster;
    GamePlayer player("Player1");
    Weapon weapon("Pistol", 10);
    
    gameMaster.startGame();
    weapon.fire();
    player.takeDamage(weapon.fire());
    gameMaster.endGame();
    
    return 0;
}