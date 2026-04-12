#pragma once

#include <bn_sprite_text_generator.h>
#include <bn_vector.h>
#include <bn_string.h>
#include "bn_sprite_ptr.h"  

class DeathCounter
{
private:
    unsigned int _count = 0;
public:
    void on_player_death();
    unsigned int count() const;   // simpelste Variante für GBA;
};

class DeathCounterHUD
{
public:
    DeathCounterHUD(bn::sprite_text_generator& text_gen,
                    const DeathCounter& counter);
    void update();

private:
    void refresh();

    bn::sprite_text_generator&      _text_gen;
    const DeathCounter&             _counter;
    bn::vector<bn::sprite_ptr, 16>  _sprites;
    uint32_t                        _last_count;
};
