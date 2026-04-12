#include "death_counter.h"


void DeathCounter::on_player_death()
{
    ++_count;
}

unsigned int DeathCounter::count() const
{
    return _count;
}

DeathCounterHUD::DeathCounterHUD(bn::sprite_text_generator& text_gen,
                                 const DeathCounter& counter)
    : _text_gen(text_gen), _counter(counter), _last_count(UINT32_MAX)
{
    refresh();
}

void DeathCounterHUD::update()
{
    if (_last_count != _counter.count())
    {
        _last_count = _counter.count();
        refresh();
    }
}

void DeathCounterHUD::refresh()
{
    _sprites.clear();
    bn::string<16> text = "Deaths: ";
    text += bn::to_string<8>(_counter.count());
    _text_gen.generate(-110, -70, text, _sprites);
}