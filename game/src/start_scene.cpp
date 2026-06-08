#include "start_scene.h"

#include <cstdio>
#include "bn_keypad.h"
#include "bn_log.h"
#include "bn_music_items.h"
#include "bn_sprite_items_titel64x128.h"

StartScene::StartScene(
    Player& player,
    bn::span<const LevelData> levels,
    DataManager& data_manager)
    : _player(player), _levels(levels), _data_manager(data_manager) {
    _selected_slot = 0;
    _transition_requested = false;
}

void StartScene::init() {
    // Hide the persistent player sprite while the start screen is visible.
    _player.set_visible(false);
    _player.set_hud_visible(false);

    // Title graphics - two tiles placed at top right with spacing
    _title_sprites.clear();
    // Tile 0 at top right
    bn::sprite_ptr title0 =
        bn::sprite_items::titel64x128.create_sprite(-70, -50);
    title0.set_tiles(
        bn::sprite_items::titel64x128.tiles_item().create_tiles(0));
    title0.set_blending_enabled(true);
    _title_sprites.push_back(bn::move(title0));
    // Tile 1 next to it with spacing
    bn::sprite_ptr title1 =
        bn::sprite_items::titel64x128.create_sprite(-6, -50);
    title1.set_tiles(
        bn::sprite_items::titel64x128.tiles_item().create_tiles(1));
    title1.set_blending_enabled(true);
    _title_sprites.push_back(bn::move(title1));

    // Background
    _bg.emplace(bn::regular_bg_items::startscreen.create_bg(0, 0));
    _bg->set_priority(3);
    _bg->set_blending_enabled(true);

    // Text generator
    _text_gen.emplace(common::variable_8x16_sprite_font);
    _text_gen->set_z_order(4);
    _text_gen->set_blending_enabled(true);

    // Start screen music
    bn::music_items::startscreen.play();

    // Initial render of slots
    _slot_sprites.clear();
    for (int i = 0; i < Cfg::StartScreen::SAVE_SLOT_COUNT; ++i) {
        char buf[32];
        int pos = 0;
        buf[pos++] = (i == _selected_slot) ? '>' : ' ';
        buf[pos++] = ' ';
        const char* label = "Slot ";
        for (const char* p = label; *p; ++p)
            buf[pos++] = *p;
        int num = i + 1;
        buf[pos++] = char('0' + (num % 10));
        buf[pos] = '\0';
        _text_gen->generate(
            Cfg::StartScreen::X, Cfg::StartScreen::Y + i * 16, buf,
            _slot_sprites);
    }
    for (bn::sprite_ptr& s : _slot_sprites) {
        s.set_blending_enabled(true);
    }
}

void StartScene::update() {
    if (!_transition_requested) {
        bool changed = false;
        if (bn::keypad::down_pressed()) {
            _selected_slot =
                (_selected_slot + 1) % Cfg::StartScreen::SAVE_SLOT_COUNT;
            changed = true;
        } else if (bn::keypad::up_pressed()) {
            _selected_slot =
                (_selected_slot + Cfg::StartScreen::SAVE_SLOT_COUNT - 1) %
                Cfg::StartScreen::SAVE_SLOT_COUNT;
            changed = true;
        }

        if (changed) {
            // Re-generate slot text
            for (bn::sprite_ptr& s : _slot_sprites) {
                s.set_visible(false);
            }
            _slot_sprites.clear();
            for (int i = 0; i < Cfg::StartScreen::SAVE_SLOT_COUNT; ++i) {
                char buf[32];
                int pos = 0;
                buf[pos++] = (i == _selected_slot) ? '>' : ' ';
                buf[pos++] = ' ';
                const char* label = "Slot ";
                for (const char* p = label; *p; ++p)
                    buf[pos++] = *p;
                int num = i + 1;
                buf[pos++] = char('0' + (num % 10));
                buf[pos] = '\0';
                _text_gen->generate(
                    Cfg::StartScreen::X, Cfg::StartScreen::Y + i * 16, buf,
                    _slot_sprites);
            }
            for (bn::sprite_ptr& s : _slot_sprites) {
                s.set_blending_enabled(true);
            }
        }

        if (bn::keypad::a_pressed()) {
            // Confirm selection: set active slot, load save, and transition
            _transition_requested = true;
            _data_manager.set_slot_index(_selected_slot);
            _data_manager.load_from_save();

            // Transition to level scene
            auto next =
                bn::make_unique<LevelScene>(_player, _levels, _data_manager);
            core::SceneManager::instance().set_next_scene(bn::move(next));
        }
    }

    // Process Butano core updates for the start screen.
    // This must run even during transition so audio fade commands are flushed.
    bn::core::update();
}
