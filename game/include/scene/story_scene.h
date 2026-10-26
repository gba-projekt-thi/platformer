#pragma once

#include "core_scene.h"
#include "core_scene_manager.h"

#include "bn_optional.h"
#include "bn_span.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "level_structure.h"
#include "story.h"

class Player;
class DataManager;
class LevelManager;

// -----------------------------------------------------------------------------
// StoryScene
//
// One page of Hildegart's story (story.h) on a black screen, centered, with
// a prompt; A or Start goes on to the level (GameState::level) in a
// LevelScene. Shown by StartScene before a new game and by LevelScene when
// the door leads into a world for the first time.
// -----------------------------------------------------------------------------
class StoryScene : public core::Scene {
   public:
    StoryScene(
        const story::Page& page,
        Player& player,
        bn::span<const LevelData> levels,
        DataManager& data_manager,
        LevelManager& level_manager);

    void init() override;
    void update() override;

   private:
    const story::Page& _page;
    Player& _player;
    bn::span<const LevelData> _levels;
    DataManager& _data_manager;
    LevelManager& _level_manager;

    bool _transition_requested = false;

    bn::optional<bn::sprite_text_generator> _text_gen;
    // story::MAX_GLYPHS plus the prompt ("Press A", 6 glyphs).
    bn::vector<bn::sprite_ptr, story::MAX_GLYPHS + 10> _sprites;
};
