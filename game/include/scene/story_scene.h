#pragma once

#include "core_scene.h"
#include "core_scene_manager.h"

#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
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
// A chapter of Hildegard's story (story.h): its pages one after the other,
// centered over the chapter's picture (a world background, dimmed), with a
// prompt. A or Start turns to the next page; after the last one it goes on to
// the level (GameState::level) in a LevelScene, or to the reunion
// (KissingScene) after the ending. Shown by StartScene before a new game, by
// LevelScene when the door leads into a world for the first time, and by main()
// when the game is finished.
// -----------------------------------------------------------------------------
class StoryScene : public core::Scene {
   public:
    enum class Then { Level, Reunion };

    StoryScene(
        const story::Chapter& chapter,
        Player& player,
        bn::span<const LevelData> levels,
        DataManager& data_manager,
        LevelManager& level_manager,
        Then then = Then::Level);

    ~StoryScene() override;

    void init() override;
    void update() override;

   private:
    void _show_page();

    const story::Chapter& _chapter;
    int _page = 0;
    Then _then;
    Player& _player;
    bn::span<const LevelData> _levels;
    DataManager& _data_manager;
    LevelManager& _level_manager;

    bool _transition_requested = false;

    bn::optional<bn::regular_bg_ptr> _bg;
    // Hildegard, and Susanne in the ending: the real in-game sprites.
    bn::vector<bn::sprite_ptr, 2> _ducks;
    bn::optional<bn::sprite_text_generator> _text_gen;
    // story::MAX_GLYPHS plus the prompt ("Press A (1/3)", 11 glyphs).
    bn::vector<bn::sprite_ptr, story::MAX_GLYPHS + 12> _sprites;
};
