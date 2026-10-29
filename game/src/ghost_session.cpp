#include "ghost_session.h"

#include "bn_color.h"
#include "bn_optional.h"
#include "bn_sprite_items_ente.h"
#include "bn_sprite_palette_ptr.h"

#include "cfg.h"
#include "ghost_logic.h"
#include "player.h"
#include "sprite.h"

namespace ghost_session {

namespace {

// `make record` stores the buttons of a run; a ghost has no place there.
#if defined(PLATFORMER_RECORD)
constexpr bool ACTIVE = false;
#else
constexpr bool ACTIVE = true;
#endif

// How pale the ghost is: its palette faded towards this by this much.
constexpr int TINT_RED = 20;
constexpr int TINT_GREEN = 26;
constexpr int TINT_BLUE = 31;
constexpr bn::fixed TINT_AMOUNT = 0.6;

// Static storage in EWRAM's zeroed section (.sbss: no ROM size, the 32 KiB of
// IWRAM could not hold it): see ghost_logic.h for the sizes.
ghost::Store<ghost::STORE_BYTES, ghost::KEYS> store
    __attribute__((section(".sbss")));
ghost::Recorder<ghost::RECORD_BYTES> recorder __attribute__((section(".sbss")));
ghost::Playback playback;
bn::optional<Sprite> sprite;
int tile = -1;  // of the ghost's sprite
bool on = true;
ghost::Key key = 0;
bool level_running = false;
int last_frame = -1;  // the level clock of the last sample offered

auto pose_of(Player& player) -> ghost::Pose {
    ghost::Pose pose;
    pose.x = int16_t(player.pos.x.round_integer());
    pose.y = int16_t(player.pos.y.round_integer());
    pose.tile = uint8_t(player.sprite_frame());
    pose.flip = player.sprite_flipped();
    return pose;
}

void make_sprite(const ghost::Pose& pose) {
    bn::optional<bn::sprite_palette_ptr> palette =
        bn::sprite_items::ente.palette_item().create_new_palette_optional();
    if (!palette) {
        return;  // all 16 sprite palettes are taken: no ghost
    }
    palette->set_fade(bn::color(TINT_RED, TINT_GREEN, TINT_BLUE), TINT_AMOUNT);
    bn::sprite_ptr raw = bn::sprite_items::ente.create_sprite(pose.x, pose.y);
    raw.set_palette(bn::move(*palette));
    raw.set_z_order(1);  // behind the duck
    // Dimmed with the level while the game is paused (pause_flow.h).
    raw.set_blending_enabled(true);
    sprite.emplace(bn::move(raw), pose.x, pose.y);
    sprite->pos.offset_y = Cfg::Player::SPRITE_OFFSET_Y;
    tile = -1;
}

}  // namespace

bool enabled() {
    return on;
}

void set_enabled(bool enabled) {
    on = enabled;
}

void level_started(Player& player, unsigned level_index, bool mirrored) {
    if (!ACTIVE) {
        return;
    }
    sprite.reset();
    key = ghost::key_for(level_index, mirrored);
    playback = store.open(key);
    recorder.clear();
    last_frame = -1;
    level_running = true;
    after_frame(player, 0);
}

void before_frame(unsigned level_frame) {
    if (!ACTIVE || !level_running) {
        return;
    }
    ghost::Pose pose;
    if (!on || !playback.pose_at(level_frame + 1, pose)) {
        sprite.reset();
        return;
    }
    if (!sprite) {
        make_sprite(pose);
        if (!sprite) {
            return;
        }
    }
    sprite->pos.move(pose.x, pose.y);
    sprite->sprite().set_horizontal_flip(pose.flip);
    if (pose.tile != tile) {
        tile = pose.tile;
        sprite->sprite().set_tiles(
            bn::sprite_items::ente.tiles_item().create_tiles(pose.tile));
    }
}

void after_frame(Player& player, unsigned level_frame) {
    if (!ACTIVE || !level_running || int(level_frame) == last_frame) {
        return;  // the clock stands (pause, death freeze)
    }
    last_frame = int(level_frame);
    recorder.sample(level_frame, pose_of(player));
}

void level_finished(unsigned level_frame, bool records_allowed) {
    if (!ACTIVE || !level_running) {
        return;
    }
    level_running = false;
    sprite.reset();
    if (records_allowed && recorder.usable()) {
        // The playback in use points into the store: it is not needed again.
        playback = ghost::Playback();
        store.offer(key, level_frame, recorder.data(), recorder.size());
    }
}

void level_left() {
    level_running = false;
    sprite.reset();
    playback = ghost::Playback();
}

}  // namespace ghost_session
