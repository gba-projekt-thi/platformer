#include "story_scene.h"

#include "audio_settings.h"
#include "background.h"
#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_regular_bg_items_story_castle.h"
#include "bn_regular_bg_items_story_ending.h"
#include "bn_regular_bg_items_story_forest.h"
#include "bn_regular_bg_items_story_intro.h"
#include "bn_regular_bg_items_story_sewer.h"
#include "bn_regular_bg_items_story_village.h"
#include "bn_sound_items.h"
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

auto picture_item(story::Picture picture) -> const bn::regular_bg_item& {
    switch (picture) {
        case story::Picture::Sewer:
            return bn::regular_bg_items::story_sewer;
        case story::Picture::Forest:
            return bn::regular_bg_items::story_forest;
        case story::Picture::Village:
            return bn::regular_bg_items::story_village;
        case story::Picture::Castle:
            return bn::regular_bg_items::story_castle;
        case story::Picture::Ending:
            return bn::regular_bg_items::story_ending;
        case story::Picture::Intro:
        default:
            break;
    }
    return bn::regular_bg_items::story_intro;
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
    // The 256 x 256 map is centered on the screen: move the picture (its
    // top-left 240 x 160 px) onto it.
    _bg->set_position(Cfg::Story::BG_X, Cfg::Story::BG_Y);

    _text_gen.emplace(common::variable_8x16_sprite_font);
    _text_gen->set_center_alignment();
    _text_gen->set_z_order(Cfg::ZOrder::PAUSE_MENU);

    _show_page();
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
