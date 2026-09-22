#include "summary_scene.h"

#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_unique_ptr.h"

#include "cfg.h"
#include "common_variable_8x16_sprite_font.h"
#include "data_manager.h"
#include "level_manager.h"
#include "player.h"
#include "start_scene.h"

namespace {
// Same small helper as WorldSelectScene's anonymous-namespace
// append_uint() - not a hot path (menu build only, run once), plain
// division/modulo is fine here.
void append_uint(char* buf, int* pos, unsigned int value) {
    char digits[8];
    int digit_count = 0;
    do {
        digits[digit_count++] = char('0' + (value % 10));
        value /= 10;
    } while (value > 0 && digit_count < 8);
    while (digit_count > 0) {
        buf[(*pos)++] = digits[--digit_count];
    }
}
}  // namespace

SummaryScene::SummaryScene(
    Player& player,
    bn::span<const LevelData> levels,
    DataManager& data_manager,
    LevelManager& level_manager)
    : _player(player),
      _levels(levels),
      _data_manager(data_manager),
      _level_manager(level_manager) {}

void SummaryScene::init() {
    _player.set_visible(false);
    _player.set_hud_visible(false);

    _text_gen.emplace(common::variable_8x16_sprite_font);
    _text_gen->set_z_order(Cfg::ZOrder::PAUSE_MENU);
    _text_gen->set_blending_enabled(true);

    const GameState& state = _data_manager.state();

    // Total time across every level's own personal best - a durable
    // record, not just this run's pace, so it stays meaningful even on a
    // slot that has been completed (and reset) more than once.
    uint32_t total_frames = 0;
    for (int i = 0; i < GameState::MAX_LEVELS; ++i) {
        total_frames += state.best_time_frames[i];
    }
    const uint32_t total_seconds = total_frames / 60;
    const unsigned int total_minutes =
        static_cast<unsigned int>(total_seconds / 60);
    const unsigned int remaining_seconds =
        static_cast<unsigned int>(total_seconds % 60);

    const int level_count = static_cast<int>(_levels.size());
    int no_death_count = 0;
    for (int i = 0; i < level_count && i < GameState::MAX_LEVELS; ++i) {
        if (has_no_death_clear(state, i)) {
            ++no_death_count;
        }
    }

    int y = Cfg::Summary::Y;

    _text_gen->generate(Cfg::Summary::X, y, "Game Complete!", _sprites);
    y += Cfg::Summary::LINE_SPACING;

    {
        // "Total Time: M:SS" - minutes left unpadded (this is a sum
        // across every level, not a single clear, so it can run past 99
        // and doesn't need the mm:ss.cc single-level format used
        // elsewhere; centiseconds are dropped as noise at this scale).
        char buf[24];
        int pos = 0;
        const char* label = "Total Time: ";
        for (const char* p = label; *p; ++p)
            buf[pos++] = *p;
        append_uint(buf, &pos, total_minutes);
        buf[pos++] = ':';
        if (remaining_seconds < 10) {
            buf[pos++] = '0';
        }
        append_uint(buf, &pos, remaining_seconds);
        buf[pos] = '\0';
        _text_gen->generate(Cfg::Summary::X, y, buf, _sprites);
        y += Cfg::Summary::LINE_SPACING;
    }

    {
        char buf[16];
        int pos = 0;
        const char* label = "Deaths: ";
        for (const char* p = label; *p; ++p)
            buf[pos++] = *p;
        append_uint(buf, &pos, state.deaths);
        buf[pos] = '\0';
        _text_gen->generate(Cfg::Summary::X, y, buf, _sprites);
        y += Cfg::Summary::LINE_SPACING;
    }

    {
        char buf[24];
        int pos = 0;
        const char* label = "No-Death: ";
        for (const char* p = label; *p; ++p)
            buf[pos++] = *p;
        append_uint(buf, &pos, static_cast<unsigned int>(no_death_count));
        buf[pos++] = '/';
        append_uint(buf, &pos, static_cast<unsigned int>(level_count));
        buf[pos] = '\0';
        _text_gen->generate(Cfg::Summary::X, y, buf, _sprites);
        y += Cfg::Summary::LINE_SPACING;
    }

    _text_gen->generate(
        Cfg::Summary::X, y + Cfg::Summary::LINE_SPACING, "Press A to continue",
        _sprites);

    for (bn::sprite_ptr& sprite : _sprites) {
        sprite.set_blending_enabled(true);
    }
}

void SummaryScene::update() {
    if (!_transition_requested &&
        (bn::keypad::a_pressed() || bn::keypad::start_pressed())) {
        _transition_requested = true;

        // Reset now, not at the end of KissingScene - the numbers above
        // needed the just-finished run's state to still be live when
        // this scene's init() read it.
        _data_manager.reset();

        auto next_scene = bn::make_unique<StartScene>(
            _player, _levels, _data_manager, _level_manager);
        core::SceneManager::instance().set_next_scene(bn::move(next_scene));
    }

    bn::core::update();
}