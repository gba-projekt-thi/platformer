#pragma once

#include "bn_bg_palettes.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_ptr.h"

// -----------------------------------------------------------------------------
// create_background
//
// Creates a full-screen background and makes the color at palette index 0 of
// its art the backdrop color. The GBA draws index 0 of a background as
// transparent, showing the backdrop instead, and every background here paints
// large areas with index 0 (the dark parts of the art). Without an explicit
// backdrop Butano only rewrites it when a palette lands in slot 0: a 16-color
// background (world3) loaded after a 256-color one (world2) then showed
// world2's dark brown where world3 is near-black.
// -----------------------------------------------------------------------------
inline auto create_background(const bn::regular_bg_item& item)
    -> bn::regular_bg_ptr {
    bn::bg_palettes::set_transparent_color(item.palette_item().colors_ref()[0]);
    return item.create_bg(0, 0);
}
