#include "bn_core.h"

#include "player.h"

int main()
{
    bn::core::init();

    Player player;

    while(true)
    {
        player.update();
        bn::core::update();
    }
}