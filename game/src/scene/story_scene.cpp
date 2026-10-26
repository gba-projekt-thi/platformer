#include "story_scene.h"

#include "audio_settings.h"
#include "bn_bg_palettes.h"
#include "bn_color.h"
#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_sound_items.h"
#include "bn_unique_ptr.h"

#include "cfg.h"
#include "common_variable_8x16_sprite_font.h"
#include "data_manager.h"
#include "level_manager.h"
#include "level_scene.h"
#include "player.h"

StoryScene::StoryScene(
    const story::Page& page,
    Player& player,
    bn::span<const LevelData> levels,
    DataManager& data_manager,
    LevelManager& level_manager)
    : _page(page),
      _player(player),
      _levels(levels),
      _data_manager(data_manager),
      _level_manager(level_manager) {}

void StoryScene::init() {
    _player.set_visible(false);
    _player.set_hud_visible(false);
    // No background: the backdrop is the page.
    bn::bg_palettes::set_transparent_color(bn::color(0, 0, 0));

    _text_gen.emplace(common::variable_8x16_sprite_font);
    _text_gen->set_center_alignment();
    _text_gen->set_z_order(Cfg::ZOrder::PAUSE_MENU);

    int y = Cfg::Story::FIRST_LINE_Y;
    for (int i = 0; i < _page.line_count; ++i) {
        _text_gen->generate(0, y, _page.lines[i], _sprites);
        y += Cfg::Story::LINE_SPACING;
    }
    _text_gen->generate(0, Cfg::Story::PROMPT_Y, "Press A", _sprites);
}

void StoryScene::update() {
    if (!_transition_requested &&
        (bn::keypad::a_pressed() || bn::keypad::start_pressed())) {
        _transition_requested = true;
        _sprites.clear();

        auto next_scene = bn::make_unique<LevelScene>(
            _player, _levels, _data_manager, _level_manager);
        core::SceneManager::instance().set_next_scene(bn::move(next_scene));

        AudioSettings::instance().play_sfx(bn::sound_items::confirm);
    }

    bn::core::update();
}
