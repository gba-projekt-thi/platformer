// Butano
#include "bn_bg_palettes.h"
#include "bn_core.h"
#include "bn_span.h"
#include "bn_unique_ptr.h"

// Engine team
#include "collision_registry.h"
#include "sprite.h"

// Our team
#include "moving_trap.h"
#include "player.h"
#include "trigger.h"

// Level 1
#include "bn_sprite_items_bubbles.h"
#include "bn_sprite_items_connector16x16.h"
#include "bn_sprite_items_door32x32.h"
#include "bn_sprite_items_kugelfisch.h"
#include "bn_sprite_items_platformslvl1.h"

// Level 2
#include "bn_sprite_items_box32x32.h"
#include "bn_sprite_items_can32x32.h"
#include "bn_sprite_items_ladder.h"
#include "bn_sprite_items_pipe16x16.h"
#include "bn_sprite_items_platformslvl2.h"
#include "bn_sprite_items_rustynails32x16.h"

// Level 3
#include "bn_sprite_items_baumstamm.h"
#include "bn_sprite_items_branch32x16.h"
#include "bn_sprite_items_mushroom32x32.h"
#include "bn_sprite_items_nebel.h"
#include "bn_sprite_items_slithermanarms32x32.h"

// Wallpapers
#include "bn_regular_bg_items_level1.h"
#include "bn_regular_bg_items_level2.h"
#include "bn_regular_bg_items_level3.h"
#include "bn_regular_bg_ptr.h"

// Music
#include "bn_music_items.h"

// Levels
#include "core_scene.h"
#include "core_scene_manager.h"
#include "level_manager.h"
#include "levels.h"

namespace {

class LevelScene : public core::Scene {
   public:
    LevelScene(
        Player& player,
        bn::span<const LevelData> levels,
        unsigned int level_index)
        : _player(player),
          _levels(levels),
          _level_index(level_index),
          _level_manager(&player) {
    }

    void init() override {
        _level_manager.load(_levels[_level_index]);
    }

    void update() override {
        if (_level_manager.update()) {
            const unsigned int next_level_index = _level_index + 1u;
            if (next_level_index < static_cast<unsigned int>(_levels.size())) {
                core::SceneManager::instance().set_next_scene(
                    bn::make_unique<LevelScene>(
                        _player, _levels, next_level_index));
            } else {
                core::SceneManager::instance().clear();
            }
        }
    }

   private:
    Player& _player;
    bn::span<const LevelData> _levels;
    unsigned int _level_index;
    LevelManager _level_manager;
};

}  // namespace

int main() {
    // Initialize the Butano engine core before using any rendering or input
    // features.
    bn::core::init();

    // Create the ordered level list for game progression.
    const LevelData levels[] = {LEVEL_0, LEVEL_1, LEVEL_2, LEVEL_3};

    // Create the player entity and apply an offset for the sprite anchor point.
    Player player(0, 0, 8, 8);

    auto first_scene = bn::make_unique<LevelScene>(
        player, bn::span<const LevelData>(levels), 0);
    core::SceneManager::instance().set_next_scene(bn::move(first_scene));

    while (core::SceneManager::instance().update()) {
    }

    for (int i = 0; i < Cfg::Sleep::FINISHED_GAME; ++i)  // sleep 2s
        bn::core::update();
}
