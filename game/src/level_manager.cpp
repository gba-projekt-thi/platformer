#include "level_manager.h"

#include "attempt_objects.h"
#include "attempt_variant.h"
#include "audio_settings.h"
#include "background.h"
#include "bn_keypad.h"
#include "boss_trap.h"
#include "level_limits.h"
#include "player_events.h"
#include "replay_capture.h"
#include "switch_logic.h"
#include "trap_factory.h"
#include "world_index.h"

LevelManager::LevelManager(Player& player, DataManager& data_manager)
    : _player(player),
      // Constructor parameter used directly, not _data_manager - the
      // member is declared *after* _pause_controller in the header, so
      // referencing it here would read before it's initialized.
      _pause_controller(data_manager),
      _data_manager(data_manager),
      _save_sync(player, data_manager) {
    restoreHUD();
}

void LevelManager::restoreHUD() {
    // -------------------------------------------------------------------------
    // Restore persistent runtime state
    // -------------------------------------------------------------------------

    Timer& timer = _player.get_timer();

    // Access already-loaded runtime state.
    auto& game_state = _data_manager.state();
    _player.set_deaths(game_state.deaths);
    timer.set_time(game_state.centis, game_state.seconds, game_state.minutes);

    // Sound settings are per-save-slot; restore them into the engine-level
    // AudioSettings singleton whenever a slot is (re)loaded. Covers both
    // this constructor and StartScene's post-load_from_save() call.
    AudioSettings::instance().set_music_level(game_state.music_volume);
    AudioSettings::instance().set_sfx_level(game_state.sfx_volume);
}

Trigger* LevelManager::find_trigger(const char* name) {
    if (name == nullptr) {
        return nullptr;
    }
    for (Trigger& trigger : _triggers) {
        const char* candidate = trigger.name();
        if (candidate != nullptr &&
            bn::string_view(candidate) == bn::string_view(name)) {
            return &trigger;
        }
    }
    return nullptr;
}

Trigger& LevelManager::get_trigger_by_name(const char* name) {
    if (name != nullptr) {
        for (Trigger& trigger : _triggers) {
            const char* candidate = trigger.name();
            if (candidate != nullptr &&
                bn::string_view(candidate) == bn::string_view(name)) {
                return trigger;
            }
        }
    }
    BN_LOG_ERROR(
        LogCategory::Level, "trigger name not found, fallback to trigger 0");
    return _triggers[0];
}

void LevelManager::_reset_entities() {
    for (auto* entity : _resettables) {
        entity->reset();
    }
}

// -----------------------------------------------------------------------------
// load() stages
// -----------------------------------------------------------------------------

void LevelManager::_load_player_spawn(const LevelData& level) {
    _player.teleport_to(level.player_data.x, level.player_data.y);
    _player.set_spawn_point(level.player_data.x, level.player_data.y);
    _spawn_x = level.player_data.x;
    _spawn_y = level.player_data.y;
    _player.set_ground_friction(level.ground_friction);
    _player.set_assist_mode(assist_mode_active());
    _loaded_in_assist_mode = assist_mode_active();
    // Dash is available from Cfg::Dash::UNLOCK_WORLD onward. Compared
    // against the level being loaded (GameState::level is already set to
    // it by every path into LevelScene), so replaying an earlier world via
    // Level Select plays as authored, without dash.
    _player.set_dash_enabled(
        _data_manager.state().level >=
        WorldIndex::WORLDS[Cfg::Dash::UNLOCK_WORLD].start_index);
    // The pogo and wall jumps likewise from their own UNLOCK_WORLD onward -
    // the pogo also in every level with a boss, the one weapon against it.
    bool has_boss = false;
    for (int i = 0; i < level.trap_count; ++i) {
        has_boss |= level.traps[i].type == TrapType::BOSS;
    }
    _player.set_pogo_enabled(
        has_boss ||
        _data_manager.state().level >=
            WorldIndex::WORLDS[Cfg::Pogo::UNLOCK_WORLD].start_index);
    _player.set_wall_jump_enabled(
        _data_manager.state().level >=
        WorldIndex::WORLDS[Cfg::WallJump::UNLOCK_WORLD].start_index);
    _save_sync.reset_baseline();
    _level_frame_count = 0;

    // A jump or dash of the previous level must not fire a trigger here.
    PlayerEvents::instance().clear();

    // Nothing of an earlier level's death feedback carries over.
    _death_fx.clear();
    _player.cancel_death_fx();
    Camera::instance().set_shake(0, 0);
    _deaths_seen = _player.get_deaths();
    _deaths_at_entry = _player.get_deaths();
    _skip_death_fx = false;
    _respawn_pending = false;
}

void LevelManager::_load_door(const LevelData& level) {
    _door.emplace(level.door);
}

unsigned LevelManager::_deaths_in_level() const {
    return attempt_variant::deaths_in_level(
        _player.get_deaths(), _deaths_at_entry);
}

// The door that moves between attempts stands where the deaths in this level
// put it (on load, and after every death, checkpoint respawn and Restart
// Level - they all end in _finish_respawn()).
void LevelManager::_place_door() {
    if (_door) {
        _door->place_for_attempt(_deaths_in_level());
    }
}

// The door that turns into a trap: touched while its trigger is on, it kills
// like a trap does - right after the collision update, so this frame's death
// starts the death feedback as usual. No tell: the door looks the same.
void LevelManager::_check_door_trap() {
    if (!_door) {
        return;
    }
    const door_logic::Touch touch = door_logic::on_touch(
        _door->reached(), _door_locked, door_logic::trapped(_door_trap));
    if (touch == door_logic::Touch::Kill) {
        _door->clear_reached();
        _player.death();
    }
}

void LevelManager::_load_music(const LevelData& level) {
    if (!_music.has_value() || *_music != level.music) {
        _music.emplace(level.music);
        bn::music::play(*_music);
    }
}

void LevelManager::_load_background(const LevelData& level) {
    // -------------------------------------------------------------------------
    // Background
    //
    // Always created at its authored position (0,0) and NEVER repositioned
    // afterwards - neither here nor in update(). The current background art
    // (level1/level2/level3/world4/world5) was authored as a single static
    // 240x160 screen, not a wide seamlessly-tileable world backdrop;
    // repositioning it with the camera wraps it at the GBA tilemap block
    // boundary (visible seam) and offsets it away from screen center in
    // wide levels. Until dedicated wide/tileable background art exists per
    // scrolling level, the background is a static backdrop while the
    // foreground (player/platforms/traps/door) scrolls normally via
    // SpriteRegistry.
    // -------------------------------------------------------------------------

    _background.reset();

    // Force a full engine frame between releasing the previous background's
    // tiles/palette and allocating the new one's. Without this, a normal
    // level-to-level scene transition destroys the old background and
    // creates the new one back-to-back within the same SceneManager::update()
    // call (no bn::core::update() runs between LevelManager::unload() and
    // load() during that transition), so the new background's VRAM write can
    // land in the same uncommitted batch as the old one's pending release -
    // observed as a level's background using stale palette colors from a
    // previously loaded level's background (but only when reached via normal
    // progression, not when loaded fresh from a save, where no prior
    // background was ever allocated). The backdrop color (palette index 0,
    // the dark parts of every background) had the same cause and is now set
    // explicitly by create_background().
    bn::core::update();

    _background.emplace(create_background(level.back_ground));
    _background->set_priority(3);
    _background->set_blending_enabled(true);
}

void LevelManager::_setup_camera(const LevelData& level) {
    Camera::instance().init(level.world_width, level.world_height);
    // The rig keeps its aim inside the same limits Camera::follow() clamps to.
    _camera_rig.set_bounds(
        camera_rig::max_offset(level.world_width, Cfg::Screen::WIDTH),
        camera_rig::max_offset(level.world_height, Cfg::Screen::HEIGHT));
    _camera_rig.snap(_player.pos.x, _player.pos.y);
    Camera::instance().follow(_camera_rig.x(), _camera_rig.y());

    // Bounce boundary must match this level's world width, not the fixed
    // 240px screen default.
    _player.set_horizontal_bound(
        level.world_width / 2 - Cfg::Screen::EDGE_MARGIN);
}

void LevelManager::_update_camera() {
    // D-Pad up / down looks up / down once held for a moment; it only shows
    // in levels taller than the screen (the camera cannot move vertically in
    // a 160px world).
    int look = 0;
    if (bn::keypad::up_held()) {
        --look;
    }
    if (bn::keypad::down_held()) {
        ++look;
    }
    _camera_rig.update(_player.pos.x, _player.pos.y, _player.vel_x, look);
    Camera::instance().follow(_camera_rig.x(), _camera_rig.y());
}

void LevelManager::_check_player_died() {
    const unsigned deaths = _player.get_deaths();
    if (deaths == _deaths_seen) {
        return;
    }
    _deaths_seen = deaths;

    // Restart Level is a death on purpose: no freeze, no shake.
    if (_skip_death_fx) {
        _skip_death_fx = false;
        return;
    }
    _death_fx.start(Cfg::DeathFx::HITSTOP_FRAMES, Cfg::DeathFx::SHAKE_FRAMES);
    _player.begin_death_fx();
}

void LevelManager::_render_frame() {
    // The shake moves what is registered with SpriteRegistry (the duck's
    // ghost, platforms, traps, door); the HUD and the background stay put.
    const death_fx::Offset shake =
        _death_fx.shake_offset(Cfg::DeathFx::SHAKE_AMPLITUDE);
    Camera::instance().set_shake(shake.x, shake.y);
    SpriteRegistry::instance().sync_all(Camera::instance());
    bn::core::update();
}

void LevelManager::_advance_death_fx() {
    if (_death_fx.advance() != death_fx::Event::FreezeEnded) {
        return;
    }
    _player.end_death_fx();
    if (_respawn_pending) {
        _respawn_pending = false;
        _finish_respawn();
    }
}

void LevelManager::_finish_respawn() {
    _reset_entities();
    _place_door();
    // Level Devil: this attempt's traps and platforms, then the dynamic
    // platforms that are there start over (the resets below put them back).
    if (_level != nullptr) {
        _apply_attempt(*_level);
        _load_dynamic_platforms(*_level);
    }
    _dynamic_platforms.reset_crumbling();
    _fleeing_platforms.reset();
    _checkpoint_run.restore(_triggers);
    // Nothing from before the death may reach a follower: the chain starts
    // from the reset (or checkpoint-restored) triggers.
    _trigger_chains.capture(_triggers);
    // The duck is back at the spawn point or a checkpoint, possibly far from
    // where it died: cut there instead of gliding across the level.
    _camera_rig.snap(_player.pos.x, _player.pos.y);
    Camera::instance().follow(_camera_rig.x(), _camera_rig.y());
}

void LevelManager::_clear_runtime_state() {
    _boss = nullptr;
    _door_trap = nullptr;
    _dynamic_platforms.clear();
    _fleeing_platforms.clear();
    _platforms.clear();
    _platform_bodies.clear();
    _triggers.clear();
    _traps.clear();
    _checkpoints.clear();
    _checkpoint_run = CheckpointRun();
    _resettables.clear();
}

void LevelManager::_validate_level(const LevelData& level) {
    // Cfg::Level::Limits are container capacities: exactly `limit` entries is
    // fine, one more is not (see level_limits.h). tools/Check_sprite_budget.py
    // reports the same at commit time and also checks the sprite budgets.
    BN_ASSERT(
        level_limits::within(
            level.platform_count, Cfg::Level::Limits::PLATFORMS),
        "Too many platforms");
    BN_ASSERT(
        level_limits::within(level.trigger_count, Cfg::Level::Limits::TRIGGERS),
        "Too many triggers");
    BN_ASSERT(
        level_limits::within(level.trap_count, Cfg::Level::Limits::TOTAL_TRAPS),
        "Too many traps");
    BN_ASSERT(
        level_limits::within(
            level.checkpoint_count, Cfg::Level::Limits::CHECKPOINTS),
        "Too many checkpoints");
}

void LevelManager::_load_platforms(const LevelData& level) {
    for (int i = 0; i < level.platform_count; ++i) {
        const PlatformData& platform = level.platforms[i];
        const int graphics_count =
            platform.sprite.tiles_item().graphics_count();

        bn::sprite_ptr raw_sprite =
            platform.sprite.create_sprite(platform.x, platform.y);
        raw_sprite.set_tiles(platform.sprite.tiles_item().create_tiles(
            platform.sprite_index % graphics_count));
        raw_sprite.set_blending_enabled(true);

        // Wrap in the shared Sprite type - same pattern as Door and every
        // trap. This is what registers the sprite with SpriteRegistry so
        // sync_all() moves it in step with the camera each frame. A bare
        // bn::sprite_ptr never gets touched again after creation and stays
        // pinned to its initial screen position on scrolling levels.
        auto wrapped_sprite = bn::make_unique<Sprite>(
            bn::move(raw_sprite), platform.x, platform.y);

        _platform_bodies.emplace_back(
            platform.x, platform.y, platform.width, platform.height,
            Cfg::Layer::PLATFORM);
        _platform_bodies.back().pos.offset_x = platform.offset_x;
        _platform_bodies.back().pos.offset_y = platform.offset_y;
        _platform_bodies.back().sprite = wrapped_sprite.get();

        _platforms.push_back(bn::move(wrapped_sprite));
    }
}

void LevelManager::_load_triggers(const LevelData& level) {
    for (int i = 0; i < level.trigger_count; ++i) {
        const TriggerData& trigger = level.triggers[i];
        _triggers.emplace_back(
            trigger.x, trigger.y, trigger.width, trigger.height,
            trigger.default_on, trigger.name, trigger.mode, trigger.sprite,
            trigger.sprite_index_off, trigger.sprite_index_on,
            trigger.delay_frames, trigger.action, trigger.follows != nullptr);
    }

    // Fallback trigger prevents invalid references.
    if (_triggers.empty()) {
        _triggers.emplace_back(1000, 1000, 0, 0, true, nullptr);
    }

    // Delayed trap chains: every follower is linked to its source by name
    // once all triggers exist (a source may come later in the array).
    _trigger_chains.clear();
    for (int i = 0; i < level.trigger_count; ++i) {
        const char* follows = level.triggers[i].follows;
        if (follows == nullptr) {
            continue;
        }
        const Trigger* source = find_trigger(follows);
        if (source == nullptr) {
            BN_LOG_ERROR(LogCategory::Level, "follows: trigger name not found");
            continue;
        }
        _trigger_chains.link(i, source - _triggers.data());
    }
    _trigger_chains.capture(_triggers);

    // Triggers reset themselves on death/restart, so one that drives only
    // platforms (no trap to reset it) still returns to its default state.
    for (Trigger& trigger : _triggers) {
        _resettables.push_back(&trigger);
    }
}

void LevelManager::_load_dynamic_platforms(const LevelData& level) {
    _dynamic_platforms.clear();
    _fleeing_platforms.clear();
    // _platforms / _platform_bodies hold exactly one entry per platform, in
    // the same order as level.platforms (see _load_platforms()).
    for (int i = 0; i < level.platform_count; ++i) {
        const PlatformData& platform = level.platforms[i];
        // One that is not there this attempt stays hidden (_apply_attempt()):
        // no switch, crumble or flee may bring it back.
        if (!_present(platform.from_death, platform.until_death)) {
            continue;
        }
        if (!platform.flee_path.empty()) {
            _fleeing_platforms.add(
                _platform_bodies[i], *_platforms[i], platform.x, platform.y,
                platform.flee_path.data(), platform.flee_path.size(),
                platform.flee_range, platform.flee_frames);
        }
        if (platform.trigger_name != nullptr) {
            BN_ASSERT(
                platform.crumble_frames == 0,
                "A platform cannot be switched and crumbling");
            _dynamic_platforms.add_switched(
                get_trigger_by_name(platform.trigger_name), _platform_bodies[i],
                *_platforms[i], platform.active_when_on);
        } else if (platform.crumble_frames > 0) {
            _dynamic_platforms.add_crumbling(
                _platform_bodies[i], *_platforms[i], platform.crumble_frames,
                platform.crumble_respawn_frames);
        }
    }
}

void LevelManager::_sync_crumble_platforms() {
    if (!_dynamic_platforms.has_crumbling()) {
        return;
    }
    // What the duck stands on right now (1px probe below its feet).
    const CollisionResult below = _player.probe_bottom(Cfg::Layer::PLATFORM);
    _dynamic_platforms.sync_crumbling(
        _player.pos, [&below](const StaticBody& body) {
            for (int hit = 0; hit < below.count; ++hit) {
                if (below.hits[hit].body == &body) {
                    return true;
                }
            }
            return false;
        });
}

bool LevelManager::_present(unsigned from_death, unsigned until_death) const {
    return attempt_variant::active_for_deaths(
        _deaths_in_level(), from_death, until_death);
}

void LevelManager::_apply_attempt(const LevelData& level) {
    // Objects without a window are never touched: every level without one
    // plays exactly as before.
    for (int i = 0; i < level.platform_count; ++i) {
        const PlatformData& platform = level.platforms[i];
        if (!attempt_objects::has_window(
                platform.from_death, platform.until_death)) {
            continue;
        }
        switch (attempt_objects::platform_action(
            _present(platform.from_death, platform.until_death),
            platform.trigger_name != nullptr)) {
            case attempt_objects::PlatformAction::Show:
                _platform_bodies[i].enable();
                _platforms[i]->enable();
                break;
            case attempt_objects::PlatformAction::Hide:
                _platform_bodies[i].disable();
                _platforms[i]->disable();
                break;
            case attempt_objects::PlatformAction::LeaveToSwitch:
            default:
                break;
        }
    }
    // _traps holds one trap per level.traps entry, in order (_load_traps()).
    for (int i = 0; i < level.trap_count; ++i) {
        const TrapData& trap = level.traps[i];
        if (attempt_objects::has_window(trap.from_death, trap.until_death)) {
            _traps[i]->set_present(_present(trap.from_death, trap.until_death));
        }
    }
}

void LevelManager::_load_traps(const LevelData& level) {
    for (int i = 0; i < level.trap_count; ++i) {
        BN_ASSERT(
            level.traps[i].type != TrapType::BOSS ||
                !attempt_objects::has_window(
                    level.traps[i].from_death, level.traps[i].until_death),
            "A boss cannot have from_death / until_death");
        _traps.push_back(TrapFactory::create(level.traps[i], *this));
        _resettables.push_back(_traps.back().get());
        if (level.traps[i].type == TrapType::BOSS) {
            _boss = static_cast<BossTrap*>(_traps.back().get());
        }
    }
    _door_locked = false;
    _sync_boss_door();
    _boss_hits_seen = _boss != nullptr ? _boss->hits_left() : 0;
}

void LevelManager::_feel_boss_hits() {
    if (_boss == nullptr) {
        return;
    }
    const int left = _boss->hits_left();
    if (boss::was_hit(_boss_hits_seen, left)) {
        _death_fx.start(Cfg::Boss::HITSTOP_FRAMES, Cfg::Boss::SHAKE_FRAMES);
    }
    _boss_hits_seen = left;
}

// The door opens once the level's boss is beaten (and shuts again when a
// death starts the fight over).
void LevelManager::_sync_boss_door() {
    const bool locked = _boss != nullptr && !_boss->defeated();
    if (locked == _door_locked || !_door) {
        return;
    }
    if (_door_locked && !locked) {
        AudioSettings::instance().play_sfx(bn::sound_items::confirm);
    }
    _door_locked = locked;
    _door->set_locked(locked);
}

void LevelManager::_load_checkpoints(const LevelData& level) {
    for (int i = 0; i < level.checkpoint_count; ++i) {
        _checkpoints.emplace_back(
            level.checkpoints[i].x, level.checkpoints[i].y);
        _checkpoints.back().set_active(false);
    }
}

void LevelManager::_sync_checkpoints() {
    const int reached = _checkpoint_run.sync(_checkpoints, _triggers);
    if (reached < 0) {
        return;
    }
    _player.set_spawn_point(
        _checkpoints[reached].pos.x, _checkpoints[reached].pos.y);
    AudioSettings::instance().play_sfx(bn::sound_items::confirm);
}

void LevelManager::_clear_checkpoint() {
    _checkpoint_run.clear(_checkpoints);
    _player.set_spawn_point(_spawn_x, _spawn_y);
}

// -----------------------------------------------------------------------------
// load()
// -----------------------------------------------------------------------------

void LevelManager::load(const LevelData& level) {
    _pause_controller.reset();

    _load_player_spawn(level);
    _load_door(level);
    _place_door();
    _load_music(level);
    _load_background(level);
    _setup_camera(level);

    _clear_runtime_state();
    _validate_level(level);

    _load_platforms(level);
    _load_triggers(level);
    _door_trap = find_trigger(level.door.trap_trigger_name);
    _load_dynamic_platforms(level);
    _load_traps(level);
    _load_checkpoints(level);
    _level = &level;
    _apply_attempt(level);
    _dynamic_platforms.sync_switched(_player.pos);
}

void LevelManager::unload() {
    _death_fx.clear();
    _respawn_pending = false;
    _player.cancel_death_fx();
    Camera::instance().set_shake(0, 0);
    _background.reset();
    _door.reset();
    _music.reset();
    _clear_runtime_state();
    _level = nullptr;
}

LevelManager::UpdateResult LevelManager::update() {
    // -------------------------------------------------------------------------
    // Pause
    // -------------------------------------------------------------------------

    const auto pause_action = _pause_controller.update();
    if (pause_action == PauseController::Action::RestartRequested) {
        // Restart Level starts over from the level's spawn point, not from a
        // checkpoint. Still counts as a death, as before.
        _clear_checkpoint();
        replay_capture::level_restarted();
        // Pausing during a death freeze and restarting must not leave the
        // rest of that freeze (or its ghost) behind; the restart's own death
        // triggers the reset below.
        _death_fx.clear();
        _player.cancel_death_fx();
        _respawn_pending = false;
        _skip_death_fx = true;
        _player.death();
    } else if (
        pause_action == PauseController::Action::ReturnToTitleRequested) {
        // Persist progress before leaving to the title screen.
        _save_sync.force_save();
        _player.clear_dash_trail();
        return UpdateResult::ReturnToTitle;
    }

    if (_pause_controller.paused()) {
        return UpdateResult::None;
    }

    // -------------------------------------------------------------------------
    // Death freeze
    //
    // For a few frames after a death the world stands still: no physics, no
    // traps, no camera movement, no level clock (the feedback must not cost
    // the player record time). The duck's ghost, the trap that killed it and
    // the shaking picture are all there is to see.
    // -------------------------------------------------------------------------

    if (_death_fx.frozen()) {
        _render_frame();
        _advance_death_fx();
        return UpdateResult::None;
    }

    // Level clock: counts frames the level has actually been playable
    // (i.e. not paused or frozen), used to compute this attempt's clear time
    // for the best-time record. Deliberately NOT reset on death - matches
    // the existing overall run Timer, which also keeps counting through
    // deaths; a level's recorded time is its total time-to-clear
    // including any retries within that attempt, not a speedrun-style
    // per-segment split.
    ++_level_frame_count;

    // -------------------------------------------------------------------------
    // Physics
    // -------------------------------------------------------------------------

    CollisionRegistry::instance().update_all();
    // After every trigger ticked and the duck pressed its triggers: one hop
    // of every delayed trap chain (trigger_logic.h).
    _trigger_chains.propagate(_triggers);
    _sync_checkpoints();
    _dynamic_platforms.sync_switched(_player.pos);
    _sync_crumble_platforms();
    _fleeing_platforms.update(_player.pos.x, _player.pos.y);
    _sync_boss_door();
    _feel_boss_hits();
    if (_door) {
        _door->flee_from(_player.pos.x, _player.pos.y);
    }
    _check_door_trap();
    _check_player_died();

    // -------------------------------------------------------------------------
    // Camera
    //
    // Only the foreground (player/platforms/traps/door, via SpriteRegistry)
    // follows the camera each frame. The background stays screen-locked -
    // see the note in _load_background().
    // -------------------------------------------------------------------------

    // On the death frame the camera stays where the duck died.
    if (!_death_fx.frozen()) {
        _update_camera();
    }

    // -------------------------------------------------------------------------
    // Rendering
    // -------------------------------------------------------------------------

    _render_frame();
    _advance_death_fx();

    // -------------------------------------------------------------------------
    // Door Completion
    // -------------------------------------------------------------------------

    if (_door && _door->reached()) {
        _player.clear_dash_trail();
        return UpdateResult::LevelComplete;
    }

    // -------------------------------------------------------------------------
    // Death Synchronization
    // -------------------------------------------------------------------------

    if (_save_sync.sync()) {
        if (_death_fx.frozen()) {
            // The traps stay as they are until the freeze ends.
            _respawn_pending = true;
        } else {
            _finish_respawn();
        }
    }
    return UpdateResult::None;
}