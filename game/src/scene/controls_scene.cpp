#include "controls_scene.h"

#include "audio_settings.h"
#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_sound_items.h"
#include "bn_unique_ptr.h"

#include "cfg.h"
#include "controls_text.h"
#include "data_manager.h"
#include "level_manager.h"
#include "player.h"
#include "start_scene.h"

ControlsScene::ControlsScene(
    Player& player,
    bn::span<const LevelData> levels,
    DataManager& data_manager,
    LevelManager& level_manager)
    : _player(player),
      _levels(levels),
      _data_manager(data_manager),
      _level_manager(level_manager) {}

void ControlsScene::init() {
    _player.set_visible(false);
    _player.set_hud_visible(false);

    _text_gen.emplace(common::variable_8x16_sprite_font);
    _text_gen->set_z_order(Cfg::ZOrder::PAUSE_MENU);
    _text_gen->set_blending_enabled(true);

    // Same anchor and line spacing as SummaryScene: a title, a list of short
    // lines, and a prompt right below them, laid out to fit the screen
    // without a background (tests/host/test_controls_text.cpp).
    int y = Cfg::Summary::Y;
    _text_gen->generate(Cfg::Summary::X, y, "Controls", _sprites);
    y += Cfg::Summary::LINE_SPACING;

    for (const char* line : ControlsText::LINES) {
        _text_gen->generate(Cfg::Summary::X, y, line, _sprites);
        y += Cfg::Summary::LINE_SPACING;
    }

    _text_gen->generate(Cfg::Summary::X, y, "Press B to go back", _sprites);

    for (bn::sprite_ptr& sprite : _sprites) {
        sprite.set_blending_enabled(true);
    }
}

void ControlsScene::update() {
    if (!_transition_requested &&
        (bn::keypad::a_pressed() || bn::keypad::b_pressed() ||
         bn::keypad::start_pressed())) {
        _transition_requested = true;

        auto next_scene = bn::make_unique<StartScene>(
            _player, _levels, _data_manager, _level_manager);
        core::SceneManager::instance().set_next_scene(bn::move(next_scene));

        AudioSettings::instance().play_sfx(bn::sound_items::cancel);
    }

    bn::core::update();
}
