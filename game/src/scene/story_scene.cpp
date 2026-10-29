#include "story_scene.h"

#include "audio_settings.h"
#include "background.h"
#include "bn_bg_palette_ptr.h"
#include "bn_colors.h"
#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_regular_bg_items_world1.h"
#include "bn_regular_bg_items_world2.h"
#include "bn_regular_bg_items_world3.h"
#include "bn_regular_bg_items_world4.h"
#include "bn_regular_bg_items_world5.h"
#include "bn_sound_items.h"
#include "bn_sprite_items_ente.h"
#include "bn_sprite_items_susanne16x16.h"
#include "bn_unique_ptr.h"

#include "cfg.h"
#include "common_variable_8x16_sprite_font.h"
#include "data_manager.h"
#include "kissing_scene.h"
#include "level_manager.h"
#include "level_scene.h"
#include "player.h"
#include "text_buffer.h"

namespace {

// The pictures are the world backgrounds; the ending is staged in the castle.
auto picture_item(story::Picture picture) -> const bn::regular_bg_item& {
    switch (picture) {
        case story::Picture::Sewer:
            return bn::regular_bg_items::world2;
        case story::Picture::Forest:
            return bn::regular_bg_items::world3;
        case story::Picture::Village:
            return bn::regular_bg_items::world4;
        case story::Picture::Castle:
        case story::Picture::Ending:
            return bn::regular_bg_items::world5;
        case story::Picture::Intro:
        default:
            break;
    }
    return bn::regular_bg_items::world1;
}

}  // namespace

StoryScene::StoryScene(
    const story::Chapter& chapter,
    Player& player,
    bn::span<const LevelData> levels,
    DataManager& data_manager,
    LevelManager& level_manager,
    Then then)
    : _chapter(chapter),
      _then(then),
      _player(player),
      _levels(levels),
      _data_manager(data_manager),
      _level_manager(level_manager) {}

void StoryScene::init() {
    _player.set_visible(false);
    _player.set_hud_visible(false);
    _bg.emplace(create_background(picture_item(_chapter.picture)));
    // Dim the picture with a black fade of its own palette so the text stays
    // readable (SceneManager owns the global fade for its transitions); the
    // sprites (text, ducks) are not dimmed.
    bn::bg_palette_ptr palette = _bg->palette();
    palette.set_fade(bn::colors::black, Cfg::Story::DIM_ALPHA);

    // Hildegard (with her top hat) at the bottom; Susanne joins her at the end.
    const bool ending = _chapter.picture == story::Picture::Ending;
    _ducks.push_back(bn::sprite_items::ente.create_sprite(
        ending ? -64 : Cfg::Story::DUCK_X, Cfg::Story::DUCK_Y));
    if (ending) {
        _ducks.push_back(bn::sprite_items::susanne16x16.create_sprite(
            64, Cfg::Story::DUCK_Y));
    }
    for (bn::sprite_ptr& duck : _ducks) {
        duck.set_scale(2);
    }

    _text_gen.emplace(common::variable_8x16_sprite_font);
    _text_gen->set_center_alignment();
    _text_gen->set_z_order(Cfg::ZOrder::PAUSE_MENU);

    _show_page();
}

StoryScene::~StoryScene() {
    if (_bg) {
        bn::bg_palette_ptr palette = _bg->palette();
        palette.set_fade_intensity(0);
    }
}

void StoryScene::_show_page() {
    _sprites.clear();
    const story::Page& page = _chapter.pages[_page];
    int y = Cfg::Story::FIRST_LINE_Y;
    for (int i = 0; i < page.line_count; ++i) {
        _text_gen->generate(0, y, page.lines[i], _sprites);
        y += Cfg::Story::LINE_SPACING;
    }

    TextBuffer<16> prompt;
    prompt.append("Press A");
    if (_chapter.page_count > 1) {
        prompt.append(" (")
            .append_uint(_page + 1)
            .append('/')
            .append_uint(_chapter.page_count)
            .append(')');
    }
    _text_gen->generate(0, Cfg::Story::PROMPT_Y, prompt.c_str(), _sprites);
}

void StoryScene::update() {
    if (!_transition_requested &&
        (bn::keypad::a_pressed() || bn::keypad::start_pressed())) {
        if (story::has_next_page(_chapter, _page)) {
            ++_page;
            _show_page();
            AudioSettings::instance().play_sfx(bn::sound_items::confirm);
            bn::core::update();
            return;
        }
        _transition_requested = true;
        _sprites.clear();

        if (_then == Then::Reunion) {
            auto next_scene = bn::make_unique<KissingScene>(
                _player, _levels, _data_manager, _level_manager);
            core::SceneManager::instance().set_next_scene(bn::move(next_scene));
        } else {
            auto next_scene = bn::make_unique<LevelScene>(
                _player, _levels, _data_manager, _level_manager);
            core::SceneManager::instance().set_next_scene(bn::move(next_scene));
        }

        AudioSettings::instance().play_sfx(bn::sound_items::confirm);
    }

    bn::core::update();
}
