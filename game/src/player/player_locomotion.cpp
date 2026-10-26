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

void PlayerLocomotion::update() {
    // Events of the previous frame are over (see PlayerEvents).
    PlayerEvents& events = PlayerEvents::instance();
    events.begin_frame();

    InputState input;
    input.left_held = bn::keypad::left_held();
    input.right_held = bn::keypad::right_held();
    input.a_pressed = bn::keypad::a_pressed();
    input.a_held = bn::keypad::a_held();
    input.b_pressed = bn::keypad::b_pressed();

    World world{_body, _block_mask, _death_handler};
    const locomotion::Effects effects = _core.step(_body, input, world);

    // The picture faces the way the duck runs or dashes.
    if (_core.facing() == locomotion::Facing::Left) {
        _sprite.sprite().set_horizontal_flip(true);
    } else if (_core.facing() == locomotion::Facing::Right) {
        _sprite.sprite().set_horizontal_flip(false);
    }

    AudioSettings& audio = AudioSettings::instance();
    if (effects.dash_started) {
        audio.play_sfx(bn::sound_items::dash);
        events.note_dash();
    }
    if (effects.step_sound) {
        audio.play_sfx(bn::sound_items::duck_step);
    }
    if (effects.jumped) {
        audio.play_sfx(bn::sound_items::jump);
        events.note_jump();
    }
}
