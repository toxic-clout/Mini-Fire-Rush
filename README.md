#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <string>
#include <cmath>
#include <algorithm>
#include <thread>
#include <chrono>
#include <iomanip>

// ========== SIMPLE MATH (No GLM dependency needed) ==========
struct Vec2 {
    float x, y;
    
    Vec2(float x = 0, float y = 0) : x(x), y(y) {}
    
    Vec2 operator+(const Vec2& v) const { return Vec2(x + v.x, y + v.y); }
    Vec2 operator-(const Vec2& v) const { return Vec2(x - v.x, y - v.y); }
    Vec2 operator*(float s) const { return Vec2(x * s, y * s); }
    
    float length() const { return std::sqrt(x * x + y * y); }
    
    Vec2 normalized() const {
        float len = length();
        if (len < 0.001f) return Vec2(1, 0);
        return Vec2(x / len, y / len);
    }
};

// ========== WEAPON CLASS ==========
class Weapon {
public:
    std::string weapon_name = "Pistol";
    int damage = 10;
    float fire_rate = 0.3f;
    int ammo_per_magazine = 15;
    float reload_time = 1.0f;
    int bullet_speed = 400;
    
    Weapon() = default;
    virtual ~Weapon() = default;
};

// ========== PLAYER STATS ==========
struct PlayerStats {
    int kills = 0;
    int deaths = 0;
    int damage_dealt = 0;
    int damage_taken = 0;
    int level = 1;
    int experience = 0;
    std::string skin_id = "default";
};

// ========== SKIN DATA ==========
class SkinData {
public:
    std::string skin_name = "Default";
    std::string skin_id = "default";
    std::string description;
    int price = 0;
    bool is_premium = false;
    
    SkinData() = default;
    virtual ~SkinData() = default;
};

// ========== BULLET CLASS ==========
class BulletObject {
public:
    Vec2 position;
    Vec2 direction = Vec2(1.0f, 0.0f);
    float speed = 400.0f;
    float lifetime = 5.0f;
    float elapsed_time = 0.0f;
    int damage = 10;
    int owner_id = 0;
    std::vector<int> hit_targets;
    
    BulletObject(const Vec2& start_pos, const Vec2& dir, int dmg, int owner)
        : position(start_pos), direction(dir.normalized()), damage(dmg), owner_id(owner) {}
    
    virtual ~BulletObject() = default;
    
    void update(float delta_time) {
        elapsed_time += delta_time;
        position = position + direction * speed * delta_time;
    }
    
    bool is_alive() const { return elapsed_time < lifetime; }
    Vec2 get_position() const { return position; }
};

// Forward declaration
class GameMaster;

// ========== GAME PLAYER CLASS ==========
class GamePlayer {
public:
    int player_id = 0;
    Vec2 position = Vec2(100.0f);
    Vec2 velocity = Vec2(0.0f);
    float rotation = 0.0f;
    
    bool is_dead = false;
    std::string current_skin = "default";
    int current_weapon_index = 0;
    
    float speed = 200.0f;
    float run_speed = 400.0f;
    bool is_running = false;
    float acceleration = 1000.0f;
    
    int max_health = 100;
    int current_health = 100;
    float health_flash_duration = 0.1f;
    float health_flash_timer = 0.0f;
    bool is_flashing = false;
    
    std::vector<std::shared_ptr<Weapon>> weapons;
    std::map<std::string, int> current_ammo;
    bool is_reloading = false;
    bool can_shoot = true;
    float shoot_cooldown_timer = 0.0f;
    float shoot_cooldown = 0.3f;
    
    std::string current_animation = "idle";
    float animation_timer = 0.0f;
    
    float last_update_time = 0.0f;
    float update_interval = 0.1f;
    
    GamePlayer(int id) : player_id(id), current_health(max_health) {
        setup_weapons();
        setup_animations();
    }
    
    virtual ~GamePlayer() = default;
    
    void setup_weapons() {
        auto pistol = std::make_shared<Weapon>();
        pistol->weapon_name = "Pistol";
        pistol->damage = 10;
        pistol->fire_rate = 0.3f;
        pistol->ammo_per_magazine = 15;
        pistol->reload_time = 1.0f;
        pistol->bullet_speed = 400;
        weapons.push_back(pistol);
        current_ammo["Pistol"] = 100;
        
        auto shotgun = std::make_shared<Weapon>();
        shotgun->weapon_name = "Shotgun";
        shotgun->damage = 40;
        shotgun->fire_rate = 0.7f;
        shotgun->ammo_per_magazine = 8;
        shotgun->reload_time = 1.5f;
        shotgun->bullet_speed = 350;
        weapons.push_back(shotgun);
        current_ammo["Shotgun"] = 40;
    }
    
    void setup_animations() {
        current_animation = "idle";
        animation_timer = 0.0f;
    }
    
    void update(float delta_time, const Vec2& input_dir, bool is_running_input) {
        if (is_dead) return;
        
        is_running = is_running_input;
        float current_speed = is_running ? run_speed : speed;
        
        if (input_dir.length() > 0.01f) {
            velocity = input_dir.normalized() * current_speed;
        } else {
            velocity = Vec2(0.0f);
        }
        
        move_and_slide();
        update_animation(input_dir);
        
        if (!can_shoot) {
            shoot_cooldown_timer += delta_time;
            if (shoot_cooldown_timer >= shoot_cooldown) {
                can_shoot = true;
                shoot_cooldown_timer = 0.0f;
            }
        }
        
        update_health_flash(delta_time);
        
        last_update_time += delta_time;
        if (last_update_time >= update_interval) {
            last_update_time = 0.0f;
        }
    }
    
    void move_and_slide() {
        position = position + velocity;
        
        position.x = std::max(0.0f, std::min(800.0f, position.x));
        position.y = std::max(0.0f, std::min(600.0f, position.y));
    }
    
    void update_animation(const Vec2& direction) {
        std::string new_animation = "idle";
        
        if (direction.length() > 0.01f) {
            new_animation = is_running ? "run" : "walk";
            
            if (direction.x < 0) {
                rotation = 3.14159f;
            } else if (direction.x > 0) {
                rotation = 0.0f;
            }
        }
        
        if (new_animation != current_animation) {
            play_animation(new_animation);
        }
    }
    
    void play_animation(const std::string& anim_name) {
        if (anim_name == current_animation) return;
        current_animation = anim_name;
        animation_timer = 0.0f;
        std::cout << "  \033[36m[Animation]\033[0m Player " << player_id << ": " << anim_name << std::endl;
    }
    
    std::shared_ptr<Weapon> get_current_weapon() const {
        if (current_weapon_index < weapons.size()) {
            return weapons[current_weapon_index];
        }
        return weapons[0];
    }
    
    void switch_weapon() {
        current_weapon_index = (current_weapon_index + 1) % weapons.size();
        std::cout << "  \033[33m[Weapon]\033[0m Player " << player_id << " switched to: " 
                  << get_current_weapon()->weapon_name << std::endl;
    }
    
    void shoot(GameMaster* game_master);
    
    void reload() {
        if (is_reloading) return;
        
        is_reloading = true;
        auto weapon = get_current_weapon();
        std::cout << "  \033[32m[Reload]\033[0m Player " << player_id << " reloading " 
                  << weapon->weapon_name << "..." << std::endl;
        
        current_ammo[weapon->weapon_name] = weapon->ammo_per_magazine;
        is_reloading = false;
        
        std::cout << "  \033[32m[Reload]\033[0m Player " << player_id << " finished! Ammo: " 
                  << current_ammo[weapon->weapon_name] << std::endl;
    }
    
    void take_damage(int damage, int attacker_id = 0) {
        current_health -= damage;
        is_flashing = true;
        health_flash_timer = 0.0f;
        
        std::cout << "  \033[31m[Damage]\033[0m Player " << player_id << " took " << damage 
                  << " damage! Health: " << current_health << "/" << max_health << std::endl;
        
        if (current_health <= 0) {
            die(attacker_id);
        }
    }
    
    void update_health_flash(float delta_time) {
        if (!is_flashing) return;
        
        health_flash_timer += delta_time;
        if (health_flash_timer >= health_flash_duration) {
            is_flashing = false;
        }
    }
    
    void die(int killer_id = 0) {
        is_dead = true;
        play_animation("die");
        
        std::cout << "  \033[91m[Death]\033[0m Player " << player_id << " died! Killed by: Player " << killer_id << std::endl;
    }
    
    void respawn() {
        is_dead = false;
        current_health = max_health;
        is_flashing = false;
        position = Vec2(100.0f + player_id * 100.0f, 100.0f);
        play_animation("idle");
        velocity = Vec2(0.0f);
        
        std::cout << "  \033[92m[Respawn]\033[0m Player " << player_id << " respawned at (" 
                  << std::fixed << std::setprecision(1) << position.x << ", " << position.y << ")" << std::endl;
    }
    
    bool is_alive() const { return !is_dead; }
    Vec2 get_position() const { return position; }
    int get_health() const { return current_health; }
};

// ========== GAME MASTER CLASS ==========
class GameMaster {
private:
    std::map<int, std::shared_ptr<GamePlayer>> players;
    std::map<int, PlayerStats> player_stats;
    std::map<int, int> player_currency;
    std::map<int, std::map<std::string, int>> player_quests;
    std::map<int, std::string> player_skins;
    std::map<std::string, std::shared_ptr<SkinData>> available_skins;
    std::vector<std::shared_ptr<BulletObject>> bullets;
    
    std::string game_state = "lobby";
    bool is_server = false;
    
    float delta_accumulator = 0.0f;
    
public:
    GameMaster() {
        setup_managers();
        setup_multiplayer();
        setup_skins();
    }
    
    virtual ~GameMaster() = default;
    
    void setup_managers() {
        std::cout << "\033[34m[GameMaster]\033[0m Setting up managers..." << std::endl;
    }
    
    void setup_multiplayer() {
        std::cout << "\033[34m[GameMaster]\033[0m Setting up multiplayer..." << std::endl;
    }
    
    void setup_skins() {
        auto default_skin = std::make_shared<SkinData>();
        default_skin->skin_name = "Default";
        default_skin->skin_id = "default";
        default_skin->price = 0;
        available_skins["default"] = default_skin;
        
        auto neon_skin = std::make_shared<SkinData>();
        neon_skin->skin_name = "Neon Warrior";
        neon_skin->skin_id = "neon";
        neon_skin->price = 500;
        neon_skin->is_premium = true;
        available_skins["neon"] = neon_skin;
        
        std::cout << "\033[34m[GameMaster]\033[0m Skins setup complete! (2 skins registered)" << std::endl;
    }
    
    void initialize() {
        std::cout << "\033[34m[GameMaster]\033[0m Initializing GameMaster..." << std::endl;
    }
    
    void update(float delta_time) {
        for (auto& [player_id, player] : players) {
            if (player) {
                Vec2 input_dir(0.0f);
                bool is_running = false;
                player->update(delta_time, input_dir, is_running);
            }
        }
        
        update_bullets(delta_time);
        check_bullet_collisions();
    }
    
    void render() {}
    
    void spawn_player(int peer_id) {
        auto player = std::make_shared<GamePlayer>(peer_id);
        player->position = Vec2(100.0f + peer_id * 100.0f, 100.0f);
        
        players[peer_id] = player;
        
        player_stats[peer_id] = {
            .kills = 0,
            .deaths = 0,
            .damage_dealt = 0,
            .damage_taken = 0
        };
        
        std::cout << "\033[34m[GameMaster]\033[0m Spawned Player " << peer_id << " at (" 
                  << std::fixed << std::setprecision(1) << player->position.x << ", " << player->position.y << ")" << std::endl;
    }
    
    std::shared_ptr<GamePlayer> get_player_by_id(int player_id) const {
        auto it = players.find(player_id);
        return (it != players.end()) ? it->second : nullptr;
    }
    
    std::vector<std::shared_ptr<GamePlayer>> get_all_players() const {
        std::vector<std::shared_ptr<GamePlayer>> result;
        for (const auto& [id, player] : players) {
            if (player) {
                result.push_back(player);
            }
        }
        return result;
    }
    
    void record_kill(int killer_id, int victim_id) {
        if (player_stats.find(killer_id) != player_stats.end()) {
            player_stats[killer_id].kills++;
        }
        
        if (player_stats.find(victim_id) != player_stats.end()) {
            player_stats[victim_id].deaths++;
        }
        
        add_experience(killer_id, 100);
        add_currency(killer_id, 50);
        update_quest("kills", killer_id, 1);
        
        std::cout << "\033[35m[GameMaster]\033[0m KILL: Player " << killer_id << " killed Player " << victim_id 
                  << " (Total Kills: " << player_stats[killer_id].kills << ")" << std::endl;
    }
    
    void start_game() {
        game_state = "playing";
        std::cout << "\033[34m[GameMaster]\033[0m \033[1m========== GAME STARTED ==========\033[0m" << std::endl;
        play_background_music("level_music");
    }
    
    void end_game(int winner_id = 0) {
        game_state = "ended";
        std::cout << "\033[34m[GameMaster]\033[0m \033[1m========== GAME ENDED ==========\033[0m" << std::endl;
        
        if (winner_id > 0) {
            add_experience(winner_id, 500);
            add_currency(winner_id, 200);
            std::cout << "\033[34m[GameMaster]\033[0m \033[1;92mWINNER: Player " << winner_id << "\033[0m" << std::endl;
        }
        
        print_stats();
    }
    
    void add_experience(int player_id, int amount) {
        if (player_stats.find(player_id) == player_stats.end()) {
            player_stats[player_id] = {};
        }
        
        player_stats[player_id].experience += amount;
        
        const int xp_per_level = 500;
        int required_xp = player_stats[player_id].level * xp_per_level;
        
        if (player_stats[player_id].experience >= required_xp) {
            level_up(player_id);
        }
        
        update_ui_experience(player_id);
    }
    
    void level_up(int player_id) {
        player_stats[player_id].level++;
        player_stats[player_id].experience = 0;
        std::cout << "\033[1;32m[GameMaster] *** LEVEL UP *** Player " << player_id << " reached Level " 
                  << player_stats[player_id].level << "!\033[0m" << std::endl;
    }
    
    void update_ui_experience(int player_id) const {
        const auto& stats = player_stats.at(player_id);
        int required_xp = stats.level * 500;
        std::cout << "  \033[34m[UI]\033[0m Player " << player_id << " XP: " << stats.experience 
                  << "/" << required_xp << " (Level " << stats.level << ")" << std::endl;
    }
    
    void add_currency(int player_id, int amount) {
        if (player_currency.find(player_id) == player_currency.end()) {
            player_currency[player_id] = 0;
        }
        
        player_currency[player_id] += amount;
        std::cout << "  \033[33m[Economy]\033[0m Player " << player_id << " earned " << amount 
                  << " currency! Total: " << player_currency[player_id] << std::endl;
    }
    
    int get_currency(int player_id) const {
        auto it = player_currency.find(player_id);
        return (it != player_currency.end()) ? it->second : 0;
    }
    
    void update_quest(const std::string& quest_type, int player_id, int amount = 1) {
        if (player_quests.find(player_id) == player_quests.end()) {
            player_quests[player_id] = {
                {"kills", 0},
                {"wins", 0},
                {"games_played", 0}
            };
        }
        
        if (player_quests[player_id].find(quest_type) != player_quests[player_id].end()) {
            player_quests[player_id][quest_type] += amount;
            std::cout << "  \033[36m[Quest]\033[0m Player " << player_id << " - " << quest_type << ": " 
                      << player_quests[player_id][quest_type] << std::endl;
        }
    }
    
    void set_player_skin(int player_id, const std::string& skin_id) {
        if (available_skins.find(skin_id) != available_skins.end()) {
            player_skins[player_id] = skin_id;
            auto skin = available_skins[skin_id];
            std::cout << "  \033[35m[Cosmetics]\033[0m Player " << player_id << " equipped skin: " 
                      << skin->skin_name << std::endl;
        }
    }
    
    void play_sfx(const std::string& sound_name, float volume_db = 0.0f) {
        std::cout << "  \033[32m[Audio]\033[0m Playing SFX: " << sound_name << " (Volume: " << volume_db << "dB)" << std::endl;
    }
    
    void play_sfx_3d(const std::string& sound_name, const Vec2& position, float volume_db = 0.0f) {
        std::cout << "  \033[32m[Audio]\033[0m Playing 3D SFX: " << sound_name << " at (" 
                  << std::fixed << std::setprecision(1) << position.x << ", " << position.y 
                  << ") (Volume: " << volume_db << "dB)" << std::endl;
    }
    
    void play_background_music(const std::string& track_name) {
        std::cout << "  \033[32m[Audio]\033[0m Now playing: " << track_name << " 🎵" << std::endl;
    }
    
    void print_stats() const {
        std::cout << "\n\033[1;36m╔════════════════════════════════════════════════════════╗\033[0m" << std::endl;
        std::cout << "\033[1;36m║                  FINAL GAME STATISTICS                  ║\033[0m" << std::endl;
        std::cout << "\033[1;36m╚════════════════════════════════════════════════════════╝\033[0m" << std::endl;
        
        for (const auto& [player_id, stats] : player_stats) {
            std::cout << "  \033[1;33mPlayer " << player_id << ":\033[0m" << std::endl;
            std::cout << "    • \033[32mKills:\033[0m " << stats.kills << std::endl;
            std::cout << "    • \033[31mDeaths:\033[0m " << stats.deaths << std::endl;
            std::cout << "    • \033[33mLevel:\033[0m " << stats.level << std::endl;
            std::cout << "    • \033[36mExperience:\033[0m " << stats.experience << std::endl;
            std::cout << "    • \033[35mCurrency:\033[0m " << get_currency(player_id) << std::endl;
            std::cout << std::endl;
        }
    }
    
    PlayerStats get_player_stats(int player_id) const {
        auto it = player_stats.find(player_id);
        return (it != player_stats.end()) ? it->second : PlayerStats();
    }
    
    int get_player_level(int player_id) const {
        auto it = player_stats.find(player_id);
        return (it != player_stats.end()) ? it->second.level : 1;
    }
    
    int get_player_experience(int player_id) const {
        auto it = player_stats.find(player_id);
        return (it != player_stats.end()) ? it->second.experience : 0;
    }
    
    std::string get_game_state() const { return game_state; }
    
    bool is_alive(int player_id) const {
        auto player = get_player_by_id(player_id);
        return player && player->is_alive();
    }
    
    void add_bullet(std::shared_ptr<BulletObject> bullet) {
        bullets.push_back(bullet);
        std::cout << "    \033[37m[Bullet]\033[0m Spawned at (" << std::fixed << std::setprecision(1) 
                  << bullet->position.x << ", " << bullet->position.y << "), damage: " << bullet->damage << std::endl;
    }
    
    void update_bullets(float delta_time) {
        for (auto it = bullets.begin(); it != bullets.end();) {
            (*it)->update(delta_time);
            if (!(*it)->is_alive()) {
                it = bullets.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    void check_bullet_collisions() {
        const float collision_radius = 20.0f;
        
        for (auto& bullet : bullets) {
            for (auto& [player_id, player] : players) {
                if (!player || player->player_id == bullet->owner_id || !player->is_alive()) {
                    continue;
                }
                
                Vec2 diff = player->get_position() - bullet->get_position();
                float distance = diff.length();
                
                if (distance < collision_radius) {
                    if (std::find(bullet->hit_targets.begin(), bullet->hit_targets.end(), player_id) 
                        != bullet->hit_targets.end()) {
                        continue;
                    }
                    
                    bullet->hit_targets.push_back(player_id);
                    player->take_damage(bullet->damage, bullet->owner_id);
                    
                    bullet->lifetime = 0.0f;
                }
            }
        }
    }
    
    void debug_spawn_test_players(int count = 2) {
        for (int i = 1; i <= count; ++i) {
            spawn_player(i);
        }
    }
    
    void debug_test_progression(int player_id) {
        for (int i = 0; i < 5; ++i) {
            add_experience(player_id, 100);
        }
    }
};

// Implement GamePlayer::shoot after GameMaster is defined
void GamePlayer::shoot(GameMaster* game_master) {
    auto weapon = get_current_weapon();
    
    if (!can_shoot || is_reloading) {
        return;
    }
    
    if (current_ammo.at(weapon->weapon_name) <= 0) {
        reload();
        return;
    }
    
    can_shoot = false;
    shoot_cooldown_timer = 0.0f;
    current_ammo[weapon->weapon_name]--;
    play_animation("shoot");
    
    Vec2 bullet_direction = Vec2(std::cos(rotation), std::sin(rotation));
    Vec2 bullet_spawn = position + bullet_direction * 20.0f;
    
    auto bullet = std::make_shared<BulletObject>(bullet_spawn, bullet_direction, weapon->damage, player_id);
    game_master->add_bullet(bullet);
    
    std::cout << "  \033[31m[Combat]\033[0m Player " << player_id << " shot " << weapon->weapon_name 
              << "! Ammo remaining: " << current_ammo[weapon->weapon_name] << std::endl;
}

// ========== MAIN PROGRAM ==========

int main() {
    // Clear screen
    system("clear || cls");
    
    std::cout << "\n";
    std::cout << "\033[1;46m╔════════════════════════════════════════════════════════╗\033[0m" << std::endl;
    std::cout << "\033[1;46m║                                                        ║\033[0m" << std::endl;
    std::cout << "\033[1;46m║     MINI MILITIA CLONE - GAME MASTER (C++ Version)    ║\033[0m" << std::endl;
    std::cout << "\033[1;46m║                                                        ║\033[0m" << std::endl;
    std::cout << "\033[1;46m╚════════════════════════════════════════════════════════╝\033[0m" << std::endl;
    std::cout << "\n";
    
    // Create game master
    GameMaster game;
    game.initialize();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Spawn test players
    std::cout << "\n\033[1;34m▶ SPAWNING TEST PLAYERS...\033[0m" << std::endl;
    std::cout << "\033[90m─────────────────────────────────────────────────────────\033[0m" << std::endl;
    game.spawn_player(1);
    game.spawn_player(2);
    game.spawn_player(3);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Start game
    std::cout << "\n\033[1;34m▶ STARTING GAME...\033[0m" << std::endl;
    std::cout << "\033[90m─────────────────────────────────────────────────────────\033[0m" << std::endl;
    game.start_game();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Simulate gameplay
    std::cout << "\n\033[1;34m▶ SIMULATING GAMEPLAY...\033[0m" << std::endl;
    std::cout << "\033[90m─────────────────────────────────────────────────────────\033[0m" << std::endl;
    
    auto player1 = game.get_player_by_id(1);
    if (player1) {
        std::cout << "\n\033[1;37m[ACTION]\033[0m Player 1 is shooting..." << std::endl;
        player1->shoot(&game);
        player1->shoot(&game);
        player1->shoot(&game);
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    if (player1) {
        std::cout << "\n\033[1;37m[ACTION]\033[0m Player 1 switching weapon..." << std::endl;
        player1->switch_weapon();
        player1->shoot(&game);
        player1->shoot(&game);
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Record kills
    std::cout << "\n\033[1;34m▶ RECORDING KILLS...\033[0m" << std::endl;
    std::cout << "\033[90m─────────────────────────────────────────────────────────\033[0m" << std::endl;
    game.record_kill(1, 2);
    game.record_kill(1, 3);
    game.record_kill(2, 3);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Test progression
    std::cout << "\n\033[1;34m▶ TESTING PROGRESSION...\033[0m" << std::endl;
    std::cout << "\033[90m─────────────────────────────────────────────────────────\033[0m" << std::endl;
    game.debug_test_progression(1);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Test currency
    std::cout << "\n\033[1;34m▶ TESTING ECONOMY...\033[0m" << std::endl;
    std::cout << "\033[90m─────────────────────────────────────────────────────────\033[0m" << std::endl;
    game.add_currency(1, 500);
    game.add_currency(2, 300);
    game.add_currency(3, 100);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    std::cout << "\n\033[1;37m[Info]\033[0m Player 1 Currency: " << game.get_currency(1) << std::endl;
    std::cout << "\033[1;37m[Info]\033[0m Player 2 Currency: " << game.get_currency(2) << std::endl;
    std::cout << "\033[1;37m[Info]\033[0m Player 3 Currency: " << game.get_currency(3) << std::endl;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Test skins
    std::cout << "\n\033[1;34m▶ TESTING COSMETICS...\033[0m" << std::endl;
    std::cout << "\033[90m─────────────────────────────────────────────────────────\033[0m" << std::endl;
    game.set_player_skin(1, "neon");
    game.set_player_skin(2, "default");
    game.set_player_skin(3, "neon");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Test combat
    std::cout << "\n\033[1;34m▶ TESTING COMBAT & DAMAGE...\033[0m" << std::endl;
    std::cout << "\033[90m─────────────────────────────────────────────────────────\033[0m" << std::endl;
    player1->take_damage(25, 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    player1->take_damage(35, 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    player1->take_damage(40, 3);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Test respawn
    std::cout << "\n\033[1;34m▶ TESTING RESPAWN...\033[0m" << std::endl;
    std::cout << "\033[90m─────────────────────────────────────────────────────────\033[0m" << std::endl;
    player1->respawn();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Run game loop
    std::cout << "\n\033[1;34m▶ RUNNING GAME LOOP (5 FRAMES)...\033[0m" << std::endl;
    std::cout << "\033[90m─────────────────────────────────────────────────────────\033[0m" << std::endl;
    for (int i = 0; i < 5; ++i) {
        std::cout << "\n\033[1;37m[Frame " << (i + 1) << "]\033[0m Updating game..." << std::endl;
        game.update(0.016f);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // End game
    std::cout << "\n\033[1;34m▶ ENDING GAME...\033[0m" << std::endl;
    std::cout << "\033[90m─────────────────────────────────────────────────────────\033[0m" << std::endl;
    game.end_game(1);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    std::cout << "\n\033[1;32m✓ Game Master shutdown complete!\033[0m" << std::endl;
    std::cout << "\n";
    
    return 0;
}
