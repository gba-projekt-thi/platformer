#include "boss_trap.h"

#include "audio_settings.h"
#include "bn_sound_items.h"

#include "level_manager.h"
#include "text_buffer.h"
#include "trigger.h"

BossTrap::BossTrap(
    const TrapData& data,
    unsigned path_waits,
    LevelManager& level_manager)
    : BaseTrap(
          data,
          data.path.empty() ? data.x : data.x + data.path[0].x(),
          data.path.empty() ? data.y : data.y + data.path[0].y(),
          Cfg::Physics::NO_MAX_VELOCITY),
      _fight(int(data.boss_hits)),
      _start_x(data.x),
      _start_y(data.y),
      _offset_x(data.offset_x),
      _offset_y(data.offset_y),
      _path(data.path),
      _path_waits(path_waits),
      _arena(level_manager.find_trigger(data.trigger_name)) {
    BN_ASSERT(
        data.boss_hits > 0 && data.boss_hits <= Cfg::Boss::MAX_PHASES,
        "BOSS trap needs 1..MAX_PHASES boss_hits");
    const char* arena =
        data.trigger_name != nullptr ? data.trigger_name : "boss";
    for (int phase = 2; phase <= int(data.boss_hits); ++phase) {
        TextBuffer<40> name;
        name.append(arena).append("_p").append_uint(unsigned(phase));
        _phase_triggers[phase] = level_manager.find_trigger(name.c_str());
    }
    TextBuffer<40> won;
    won.append(arena).append("_won");
    _won = level_manager.find_trigger(won.c_str());
    StrikeTargets::instance().add(static_cast<StaticBody*>(this), *this);
}

BossTrap::~BossTrap() {
    StrikeTargets::instance().remove(static_cast<StaticBody*>(this));
}

bool BossTrap::fighting() const {
    return !_fight.defeated() && (_arena == nullptr || _arena->is_triggered());
}

void BossTrap::update() {
    BaseTrap::update();
    _fight.tick();
    if (!_fight.defeated()) {
        _trap_sprite.sprite().set_visible(_fight.visible());
    }
    if (!fighting() || _path.size() < 2 || _path_waits == 0) {
        return;
    }

    // Along the path like PathTrap, at the current phase's speed.
    const unsigned frames = boss::phase_frames(_path_waits, _fight.phase());
    if (_frame >= frames) {
        _frame = 0;
        _segment = (_segment + 1) % unsigned(_path.size());
    }
    ++_frame;
    const unsigned next = (_segment + 1) % unsigned(_path.size());
    const bn::fixed_point& from = _path[_segment];
    const bn::fixed_point& to = _path[next];
    // distance * frame / frames: bn::fixed * bn::fixed would round the
    // ratio away (72 * 1/80 comes out as 0, then 1.125, ...) and make the
    // boss jerk; an integer factor keeps the steps even.
    _place(
        _start_x + from.x() + (to.x() - from.x()) * int(_frame) / int(frames),
        _start_y + from.y() + (to.y() - from.y()) * int(_frame) / int(frames));
}

void BossTrap::on_enter(uint16_t hit_layers, StaticBody* body) {
    if (!_fight.defeated()) {
        BaseTrap::on_enter(hit_layers, body);
    }
}

void BossTrap::on_struck() {
    if (!fighting()) {
        return;
    }
    const boss::Hit hit = _fight.strike();
    if (hit == boss::Hit::Hurt) {
        AudioSettings::instance().play_sfx(bn::sound_items::trap_fall);
        Trigger* trigger = _phase_triggers[_fight.phase()];
        if (trigger != nullptr) {
            trigger->restore(true);
        }
    } else if (hit == boss::Hit::Defeated) {
        AudioSettings::instance().play_sfx(bn::sound_items::duck_death);
        disable();
        _trap_sprite.sprite().set_visible(false);
        if (_won != nullptr) {
            _won->restore(true);
        }
    }
}

void BossTrap::reset() {
    _fight.reset();
    _segment = 0;
    _frame = 0;
    enable();
    _trap_sprite.sprite().set_visible(true);
    _place(
        _path.empty() ? _start_x : _start_x + _path[0].x(),
        _path.empty() ? _start_y : _start_y + _path[0].y());
}

// The picture at (x, y), the hitbox at its offset from it. Direct position
// writes bypass PhysicsBody::move(): shape and sprite follow by hand.
void BossTrap::_place(bn::fixed x, bn::fixed y) {
    pos.x = x + _offset_x;
    pos.y = y + _offset_y;
    shape_pos.move(pos.x, pos.y);
    if (sprite) {
        sprite->pos.move(x, y);
    }
}
