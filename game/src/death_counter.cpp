#include "death_counter.h"

void DeathCounter::on_player_death() {
    // Increment the death counter each time the player dies.
    ++count_value;
}

auto DeathCounter::count() const -> unsigned int {
    return count_value;
}

DeathCounterHUD::DeathCounterHUD(
    bn::sprite_text_generator& text_gen_arg,
    const DeathCounter& counter_arg)
    : text_gen(text_gen_arg), counter(counter_arg), last_count(UINT32_MAX) {
    text_gen.set_z_order(Cfg::ZOrder::DEATH_COUNTER);  // total foreground
    // Initialize the HUD text when the HUD object is created.
    refresh();
}

void DeathCounterHUD::update()  // UI update
{
    // Only redraw the HUD when the death count changes.
    if (last_count != counter.count()) {
        last_count = counter.count();
        refresh();
    }
}

void DeathCounterHUD::refresh() {
    sprites.clear();
    bn::string<Cfg::DeathCounter::STRING_LEN> text = "Deaths: ";
    text += bn::to_string<Cfg::DeathCounter::COUNT_LEN>(counter.count());
    text_gen.generate(
        Cfg::DeathCounter::X, Cfg::DeathCounter::Y, text, sprites);
}
