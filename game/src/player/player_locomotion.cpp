#include "player_locomotion.h"

#include "audio_settings.h"
#include "bn_keypad.h"
#include "bn_sound_items.h"
#include "player_events.h"

PlayerLocomotion::PlayerLocomotion(
    PhysicsBody& body,
    Sprite& sprite,
    uint16_t block_mask,
    PlayerDeathHandler& death_handler)
    : _body(body),
      _sprite(sprite),
      _block_mask(block_mask),
      _death_handler(death_handler) {}

namespace {

// The buttons of the real keypad, for locomotion::run_frame().
struct DeviceKeypad {
    static bool left_held() { return bn::keypad::left_held(); }
    static bool right_held() { return bn::keypad::right_held(); }
    static bool a_pressed() { return bn::keypad::a_pressed(); }
    static bool a_held() { return bn::keypad::a_held(); }
    static bool b_pressed() { return bn::keypad::b_pressed(); }
    static bool r_pressed() { return bn::keypad::r_pressed(); }
};

// Sounds and PlayerEvents of the real game, for locomotion::run_frame().
struct DeviceOut {
    void begin_frame() {
        // Events of the previous frame are over (see PlayerEvents).
        PlayerEvents::instance().begin_frame();
    }
    void note_dash() { PlayerEvents::instance().note_dash(); }
    void note_jump() { PlayerEvents::instance().note_jump(); }
    void sound(locomotion::Sound sound) {
        AudioSettings& audio = AudioSettings::instance();
        switch (sound) {
            case locomotion::Sound::Dash:
                audio.play_sfx(bn::sound_items::dash);
                break;
            case locomotion::Sound::Step:
                audio.play_sfx(bn::sound_items::duck_step);
                break;
            case locomotion::Sound::Pogo:
                audio.play_sfx(bn::sound_items::pogo);
                break;
            case locomotion::Sound::TrapHit:
                audio.play_sfx(bn::sound_items::trap_hit);
                break;
            case locomotion::Sound::Jump:
                audio.play_sfx(bn::sound_items::jump);
                break;
            default:
                break;
        }
    }
};

}  // namespace

void PlayerLocomotion::update() {
    World world{_body, _block_mask, _death_handler};
    DeviceOut out;
    locomotion::run_frame(
        _core, _body, DeviceKeypad{}, _input_hook, world, out);

    // The picture faces the way the duck runs or dashes.
    if (_core.facing() == locomotion::Facing::Left) {
        _sprite.sprite().set_horizontal_flip(true);
    } else if (_core.facing() == locomotion::Facing::Right) {
        _sprite.sprite().set_horizontal_flip(false);
    }
}
