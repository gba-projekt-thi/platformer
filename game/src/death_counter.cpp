#include "death_counter.h"

void DeathCounter::on_player_death() {
    // Increment the death counter each time the player dies.
    ++_count;
}

auto DeathCounter::count() const -> unsigned int {
    return _count;
}

DeathCounterHUD::DeathCounterHUD(
    bn::sprite_text_generator& text_gen,
    const DeathCounter& counter)
    : _text_gen(text_gen), _counter(counter), _last_count(UINT32_MAX) {
    _text_gen.set_z_order(Cfg::ZOrder::DEATH_COUNTER);  // total foreground
    // Initialize the HUD text when the HUD object is created.
    refresh();
}

void DeathCounterHUD::update()  // UI update
{
    // Only redraw the HUD when the death count changes.
    if (_last_count != _counter.count()) {
        _last_count = _counter.count();
        refresh();
    }
}

void DeathCounterHUD::refresh() {
    _sprites.clear();
    bn::string<Cfg::DeathCounter::STRING_LEN> text = "Deaths: ";
    text += bn::to_string<Cfg::DeathCounter::COUNT_LEN>(_counter.count());
    _text_gen.generate(
        Cfg::DeathCounter::X, Cfg::DeathCounter::Y, text, _sprites);

    for (bn::sprite_ptr& sprite : _sprites) {
        sprite.set_blending_enabled(true);
    }
}
