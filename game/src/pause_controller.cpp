#include "pause_controller.h"

#include "audio_settings.h"
#include "bn_blending.h"
#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_music.h"
#include "bn_sprite_text_generator.h"

#include "cfg.h"
#include "common_variable_8x16_sprite_font.h"
#include "controls_text.h"
#include "data_manager.h"
#include "game_modes.h"
#include "ghost_session.h"
#include "level_manager.h"
#include "menu_nav.h"
#include "pause_flow.h"
#include "text_buffer.h"

namespace {
constexpr const char* MENU_LABELS[5] = {
    "Continue", "Restart Level", "Options", "Controls", "Title Screen"};
}  // namespace

// The steps pause_flow.h puts in order.
struct PauseController::FlowOps {
    PauseController& p;

    void select_first_option() { p._selected_index = 0; }
    void leave_submenus() {
        p._showing_options = false;
        p._showing_controls = false;
    }
    void rebuild_menu() { p._rebuild_menu(); }
    void show_title() { p._show_title(); }
    static void dim_level() {
        bn::blending::set_fade_alpha(pause_flow::dimmed_alpha<bn::fixed>(
            bn::blending::fade_alpha(), Cfg::PauseMenu::DIM_ALPHA));
    }
    static void undim_level() { bn::blending::set_fade_alpha(0); }
    static void pause_music() { bn::music::pause(); }
    static void resume_music() { bn::music::resume(); }
    static void sfx_menu() {
        AudioSettings::instance().play_sfx(bn::sound_items::menu);
    }
    static void sfx_cancel() {
        AudioSettings::instance().play_sfx(bn::sound_items::cancel);
    }
    void release_sprites() { p._release_sprites(); }
    void set_showing_controls() { p._showing_controls = true; }
    void release_title() { p._title_sprites.clear(); }
    void rebuild_controls_view() { p._rebuild_controls_view(); }
    void set_unpaused() {
        p._paused = false;
        p._prev_paused = false;
    }
    void init_text_generator() { p._init_pause_menu(); }
    void reset_state() {
        p._paused = false;
        p._prev_paused = false;
        p._selected_index = 0;
        p._showing_options = false;
        p._showing_controls = false;
        p._option_row = 0;
    }
};

PauseController::PauseController(DataManager& data_manager)
    : _data_manager(data_manager) {}

void PauseController::_init_pause_menu() {
    if (_pause_menu_initialized) {
        return;
    }

    _text_gen.emplace(common::variable_8x16_sprite_font);
    _text_gen->set_z_order(Cfg::ZOrder::PAUSE_MENU);
    // Blending stays off: the dimming of the level must not touch the text.

    // Only the generator: it owns no sprites. The title and the menu rows are
    // created when the game is paused and released when it resumes.
    _pause_menu_initialized = true;
}

void PauseController::_show_title() {
    _title_sprites.clear();
    _text_gen->generate(
        Cfg::PauseMenu::X, Cfg::PauseMenu::Y_TITLE, "Paused", _title_sprites);
}

void PauseController::_release_sprites() {
    // Butano counts a hidden sprite like a visible one, so the overlay is
    // released, not hidden: it needs up to ~63 of the 128 sprites while it is
    // open and must not keep any of them while the level runs.
    pause_flow::release(_title_sprites, _menu_sprites);
}

void PauseController::_rebuild_menu() {
    // Rebuild the option text so the ">" cursor tracks the current
    // selection, the same pattern used by the save-slot list in
    // StartScene.
    for (bn::sprite_ptr& sprite : _menu_sprites) {
        sprite.set_visible(false);
    }
    _menu_sprites.clear();

    for (int i = 0; i < int(MenuOption::Count); ++i) {
        TextBuffer<24> line;
        line.append(i == _selected_index ? "> " : "  ").append(MENU_LABELS[i]);

        _text_gen->generate(
            Cfg::PauseMenu::X,
            Cfg::PauseMenu::Y_OPTION_0 + i * Cfg::PauseMenu::OPTION_SPACING,
            line.c_str(), _menu_sprites);
    }

    for (bn::sprite_ptr& sprite : _menu_sprites) {
        sprite.set_visible(_paused);
    }
}

int PauseController::_assist_row() const {
    return modes::assist_row(_data_manager.state().hard_mode_unlocked);
}

void PauseController::_append_toggle_row(int row, const char* label, bool on) {
    TextBuffer<24> line;
    line.append(_option_row == row ? "> " : "  ")
        .append(label)
        .append(on ? "On >" : "Off >");

    _text_gen->generate(
        Cfg::PauseMenu::X,
        Cfg::PauseMenu::Y_OPTION_0 + row * Cfg::PauseMenu::OPTION_SPACING,
        line.c_str(), _menu_sprites);
}

void PauseController::_rebuild_options_menu() {
    for (bn::sprite_ptr& sprite : _menu_sprites) {
        sprite.set_visible(false);
    }
    _menu_sprites.clear();

    const AudioSettings& audio = AudioSettings::instance();
    const uint8_t levels[2] = {audio.music_level(), audio.sfx_level()};
    const char* labels[2] = {"Music", "SFX"};

    for (int i = 0; i < 2; ++i) {
        TextBuffer<24> line;
        line.append(i == _option_row ? "> " : "  ")
            .append(labels[i])
            .append(" < ")
            .append_uint(levels[i])
            .append('/')
            .append_uint(AudioSettings::MAX_LEVEL)
            .append(" >");

        _text_gen->generate(
            Cfg::PauseMenu::X,
            Cfg::PauseMenu::Y_OPTION_0 + i * Cfg::PauseMenu::OPTION_SPACING,
            line.c_str(), _menu_sprites);
    }

    // Toggle rows (not 0-4 levels): Hard Mode - only shown once unlocked
    // (see GameState::hard_mode_unlocked) - then Assist Mode, which is
    // always available. The two are mutually exclusive, see
    // _change_option_level().
    const GameState& state = _data_manager.state();
    if (state.hard_mode_unlocked) {
        _append_toggle_row(2, "Hard Mode < ", state.hard_mode_enabled);
    }
    const int assist_row = _assist_row();
    _append_toggle_row(assist_row, "Assist Mode < ", state.assist_mode_enabled);
    // RAM only (ghost_session.h), not part of the save.
    const int ghost_row = assist_row + 1;
    _append_toggle_row(ghost_row, "Ghost < ", ghost_session::enabled());

    for (bn::sprite_ptr& sprite : _menu_sprites) {
        sprite.set_visible(_paused);
    }
}

void PauseController::_rebuild_controls_view() {
    // The menu rows make way for the controls list (the title is already
    // gone, see pause_flow::open_controls). Sprite budget: the overview is
    // drawn on top of a running level, so ControlsText keeps its lines short
    // (see controls_text.h).
    for (bn::sprite_ptr& sprite : _menu_sprites) {
        sprite.set_visible(false);
    }
    _menu_sprites.clear();

    // No "Controls" title: the menu row just chosen says it, and every glyph
    // is a sprite on top of a running level (tools/Check_sprite_budget.py).
    for (int i = 0; i < ControlsText::LINE_COUNT; ++i) {
        _text_gen->generate(
            Cfg::PauseMenu::X,
            Cfg::PauseMenu::Y_OPTION_0 + i * Cfg::PauseMenu::OPTION_SPACING,
            ControlsText::LINES[i], _menu_sprites);
    }

    for (bn::sprite_ptr& sprite : _menu_sprites) {
        sprite.set_visible(_paused);
    }
}

void PauseController::_change_option_level(int delta) {
    AudioSettings& audio = AudioSettings::instance();
    GameState& state = _data_manager.state();

    if (_option_row == 0) {
        int level = int(audio.music_level()) + delta;
        if (level < 0) {
            level = 0;
        }
        if (level > int(AudioSettings::MAX_LEVEL)) {
            level = AudioSettings::MAX_LEVEL;
        }
        audio.set_music_level(uint8_t(level));
        state.music_volume = audio.music_level();

        // Apply immediately so the player hears the change live. During a
        // fade-in the scene manager sets the volume again every frame, from
        // music_scale(), which already holds the new level.
        bn::music::set_volume(audio.music_scale());
    } else if (_option_row == 1) {
        int level = int(audio.sfx_level()) + delta;
        if (level < 0) {
            level = 0;
        }
        if (level > int(AudioSettings::MAX_LEVEL)) {
            level = AudioSettings::MAX_LEVEL;
        }
        audio.set_sfx_level(uint8_t(level));
        state.sfx_volume = audio.sfx_level();

        // Audible preview at the just-set level, so raising SFX from 0
        // is itself audible feedback.
        audio.play_sfx(bn::sound_items::select);
    } else if (_option_row == modes::ghost_row(state.hard_mode_unlocked)) {
        ghost_session::set_enabled(!ghost_session::enabled());
        AudioSettings::instance().play_sfx(bn::sound_items::select);
    } else if (_option_row == _assist_row()) {
        // Assist Mode row - a toggle; either direction flips it. Turning
        // it on turns Hard Mode off (mutually exclusive). Committed to
        // SRAM once on leaving the sub-menu, like the rows above.
        modes::toggle_assist_mode(state);
        AudioSettings::instance().play_sfx(bn::sound_items::select);
    } else {
        // Hard Mode row - a toggle, not a 0-4 level; either direction
        // flips it. Turning it on turns Assist Mode off. Not saved here -
        // PauseController commits to SRAM once, on leaving the Options
        // sub-menu (see the b_pressed() handling in update()), same as
        // Music/SFX.
        modes::toggle_hard_mode(state);
        AudioSettings::instance().play_sfx(bn::sound_items::select);
    }
}

void PauseController::reset() {
    FlowOps ops{*this};
    pause_flow::reset(ops);
}

PauseController::Action PauseController::update() {
    if (bn::keypad::start_pressed()) {
        _paused = !_paused;
    }

    if (_prev_paused != _paused) {
        _prev_paused = _paused;

        FlowOps ops{*this};
        if (_paused) {
            pause_flow::open(ops);
        } else {
            pause_flow::close(ops);
        }
    }

    if (_paused) {
        // A fade-in of the scene manager overwrites the dimming every frame.
        FlowOps::dim_level();

        if (_showing_controls) {
            if (bn::keypad::a_pressed() || bn::keypad::b_pressed()) {
                _showing_controls = false;
                FlowOps ops{*this};
                pause_flow::leave_controls(ops);
            }

            bn::core::update();
            return Action::None;
        }

        if (_showing_options) {
            bool changed = false;

            if (bn::keypad::down_pressed() || bn::keypad::up_pressed()) {
                // Music, SFX, [Hard Mode], Assist Mode, Ghost.
                const int row_count = modes::options_row_count(
                    _data_manager.state().hard_mode_unlocked);
                _option_row = bn::keypad::down_pressed()
                                  ? (_option_row + 1) % row_count
                                  : (_option_row + row_count - 1) % row_count;
                changed = true;
                AudioSettings::instance().play_sfx(bn::sound_items::select);
            } else if (bn::keypad::right_pressed()) {
                _change_option_level(1);
                changed = true;
            } else if (bn::keypad::left_pressed()) {
                _change_option_level(-1);
                changed = true;
            }

            if (changed) {
                _rebuild_options_menu();
            }

            if (bn::keypad::b_pressed()) {
                // Commit to SRAM once, on leaving the sub-menu - not per
                // keypress inside it. SRAM writes are relatively
                // expensive (see save-sync policy elsewhere in the code).
                _data_manager.save();
                _showing_options = false;
                _rebuild_menu();
                AudioSettings::instance().play_sfx(bn::sound_items::cancel);
            }

            bn::core::update();
            return Action::None;
        }

        bool selection_changed = false;

        if (bn::keypad::down_pressed()) {
            _selected_index =
                menu_nav::next_index(_selected_index, int(MenuOption::Count));
            selection_changed = true;
            AudioSettings::instance().play_sfx(bn::sound_items::select);
        } else if (bn::keypad::up_pressed()) {
            _selected_index =
                menu_nav::prev_index(_selected_index, int(MenuOption::Count));
            selection_changed = true;
            AudioSettings::instance().play_sfx(bn::sound_items::select);
        }

        if (selection_changed) {
            _rebuild_menu();
        }

        if (bn::keypad::a_pressed()) {
            const MenuOption option = static_cast<MenuOption>(_selected_index);

            AudioSettings::instance().play_sfx(bn::sound_items::confirm);

            if (option == MenuOption::Options) {
                // Stay paused, switch into the sub-menu - no Action
                // returned, no scene transition involved.
                _showing_options = true;
                _option_row = 0;
                _rebuild_options_menu();
                bn::core::update();
                return Action::None;
            }

            if (option == MenuOption::Controls) {
                // Stay paused, show the overview in place of the menu - no
                // Action returned, no scene transition (leaving the level
                // would destroy it).
                FlowOps ops{*this};
                pause_flow::open_controls(ops);
                bn::core::update();
                return Action::None;
            }

            // Leave the pause state immediately so the menu closes on the
            // same frame the action is confirmed, regardless of what the
            // caller does with the returned action afterwards (e.g. a
            // scene transition that destroys this controller).
            FlowOps ops{*this};
            pause_flow::leave_for_action(ops);

            return pause_flow::action_for(option);
        }

        bn::core::update();
    }

    return Action::None;
}

bool PauseController::paused() const {
    return _paused;
}