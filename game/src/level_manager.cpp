#include "level_manager.h"

#include "level_flow.h"

#include "attempt_objects.h"
#include "attempt_variant.h"
#include "audio_settings.h"
#include "background.h"
#include "bn_keypad.h"
#include "boss_trap.h"
#include "level_limits.h"
#include "player_events.h"
#include "replay_capture.h"
#include "respawn_logic.h"
#include "statistics.h"
#include "switch_logic.h"
#include "tour.h"
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

// -----------------------------------------------------------------------------
// The steps level_flow.h puts in order
// -----------------------------------------------------------------------------

struct LevelManager::FlowOps {
    LevelManager& m;
    // The level being loaded, else the loaded one (nullptr after unload()).
    const LevelData* level;

    // -- load / unload ---------------------------------------------------
    void reset_pause() { m._pause_controller.reset(); }
    void reset_player_for_level() { m._load_player_spawn(*level); }
    void clear_death_fx() { m._death_fx.clear(); }
    void cancel_player_death_fx() { m._player.cancel_death_fx(); }
    void reset_shake() { Camera::instance().set_shake(0, 0); }
    void enter_level_attempt() {
        m._attempt.enter(m._data_manager.state(), m._player.get_deaths());
    }
    void cancel_pending_respawn() { m._pending_respawn.cancel(); }
    void load_door() { m._load_door(*level); }
    void place_door() { m._place_door(); }
    void load_music() { m._load_music(*level); }
    void load_background() { m._load_background(*level); }
    void setup_camera() { m._setup_camera(*level); }
    void validate_level() { _validate_level(*level); }
    void load_platforms() { m._load_platforms(*level); }
    void load_triggers() { m._load_triggers(*level); }
    void find_door_trap() {
        m._door_trap = m.find_trigger(level->door.trap_trigger_name);
    }
    void forget_door_trap_in_checkpoints() {
        m._checkpoint_run.forget_trigger(
            m._door_trap != nullptr ? m._door_trap - m._triggers.data()
                                    : CheckpointRun::NO_TRIGGER);
    }
    void load_dynamic_platforms() { m._load_dynamic_platforms(*level); }
    void load_traps() { m._load_traps(*level); }
    void load_checkpoints() { m._load_checkpoints(*level); }
    void remember_level() { m._level = level; }
    void apply_attempt() { m._apply_attempt(*level); }
    void sync_switched() { m._dynamic_platforms.sync_switched(m._player.pos); }
    void release_scenery() {
        m._background.reset();
        m._door.reset();
        m._music.reset();
    }
    void forget_level() { m._level = nullptr; }

    void forget_boss() { m._boss = nullptr; }
    void forget_door_trap() { m._door_trap = nullptr; }
    void clear_dynamic_platforms() { m._dynamic_platforms.clear(); }
    void clear_fleeing_platforms() { m._fleeing_platforms.clear(); }
    void clear_platforms() {
        m._platforms.clear();
        m._platform_bodies.clear();
    }
    void clear_triggers() { m._triggers.clear(); }
    void clear_traps() { m._traps.clear(); }
    void clear_checkpoints() { m._checkpoints.clear(); }
    void forget_checkpoint_run() { m._checkpoint_run = CheckpointRun(); }
    void clear_resettables() { m._resettables.clear(); }

    // -- respawn ---------------------------------------------------------
    void reset_entities() { m._reset_entities(); }
    [[nodiscard]] bool has_level() const { return level != nullptr; }
    level_flow::Attempt& attempt() { return m._attempt; }
    void set_player_deaths_in_level(unsigned deaths) {
        m._player.set_deaths(m._attempt.at_entry + deaths);
    }
    [[nodiscard]] unsigned deaths() const { return m._player.get_deaths(); }
    bool layout_changes(unsigned before, unsigned after) {
        return attempt_objects::layout_changes(*level, before, after);
    }
    void cue_layout_change(int shake_frames) {
        m._cue_layout_change(shake_frames);
    }
    void reset_crumbling() { m._dynamic_platforms.reset_crumbling(); }
    void reset_fleeing() { m._fleeing_platforms.reset(); }
    void respawn_triggers() {
        m._trigger_chains.respawn(m._triggers, [this](auto& triggers) {
            m._checkpoint_run.restore(triggers);
        });
    }
    void snap_camera() {
        // The duck is back at the spawn point or a checkpoint, possibly far
        // from where it died: cut there instead of gliding across the level.
        m._camera_rig.snap(m._player.pos.x, m._player.pos.y);
        Camera::instance().follow(m._camera_rig.x(), m._camera_rig.y());
    }
    bool request_respawn(respawn::Cause cause) {
        // If it has to wait, the traps stay as they are until the freeze ends.
        return m._pending_respawn.request(cause, m._death_fx.frozen());
    }
    death_fx::Event advance_death_fx() { return m._death_fx.advance(); }
    void end_player_death_fx() { m._player.end_death_fx(); }
    bool freeze_ended_respawn() { return m._pending_respawn.freeze_ended(); }

    // -- restart level / deaths -------------------------------------------
    void clear_checkpoint() { m._clear_checkpoint(); }
    void note_restart() { replay_capture::level_restarted(); }
    void kill_player() { m._player.death(); }
    bool sync_save() { return m._save_sync.sync(); }
    void force_save() { m._save_sync.force_save(); }
    void start_death_fx(int hitstop_frames, int shake_frames) {
        m._death_fx.start(hitstop_frames, shake_frames);
    }
    void begin_player_death_fx() { m._player.begin_death_fx(); }

    // -- door ------------------------------------------------------------
    [[nodiscard]] bool has_door() const { return m._door.has_value(); }
    bool door_trap_on() { return door_logic::trapped(m._door_trap); }
    bool door_reached() { return m._door->reached(); }
    [[nodiscard]] bool door_locked() const { return m._door_locked; }
    void clear_door_reached() { m._door->clear_reached(); }

    // -- one frame -------------------------------------------------------
    level_flow::PauseAction pause_update() {
        switch (m._pause_controller.update()) {
            case PauseController::Action::RestartRequested:
                return level_flow::PauseAction::Restart;
            case PauseController::Action::ReturnToTitleRequested:
                return level_flow::PauseAction::ReturnToTitle;
            default:
                return level_flow::PauseAction::None;
        }
    }
    [[nodiscard]] bool paused() const { return m._pause_controller.paused(); }
    [[nodiscard]] bool frozen() const { return m._death_fx.frozen(); }
    void clear_dash_trail() { m._player.clear_dash_trail(); }
    void count_frame() {
        ++m._level_frame_count;
        statistics::tick_play_time(m._data_manager.state());
    }
    void update_collisions() { CollisionRegistry::instance().update_all(); }
    void propagate_triggers() {
        // After every trigger ticked and the duck pressed its triggers: one
        // hop of every delayed trap chain (trigger_logic.h).
        m._trigger_chains.propagate(m._triggers);
    }
    void sync_checkpoints() { m._sync_checkpoints(); }
    void sync_crumbling() { m._sync_crumble_platforms(); }
    void update_fleeing() {
        m._fleeing_platforms.update(m._player.pos.x, m._player.pos.y);
    }
    void sync_boss_door() { m._sync_boss_door(); }
    void feel_boss_hits() { m._feel_boss_hits(); }
    void update_boss_music() { m._update_boss_music(); }
    void flee_door() {
        if (m._door) {
            m._door->flee_from(m._player.pos.x, m._player.pos.y);
        }
    }
    void update_camera() { m._update_camera(); }
    void apply_shake(int amplitude) {
        // The shake moves what is registered with SpriteRegistry (the duck's
        // ghost, platforms, traps, door); the HUD and the background stay put.
        const death_fx::Offset shake = m._death_fx.shake_offset(amplitude);
        Camera::instance().set_shake(shake.x, shake.y);
    }
    void sync_sprites() {
        SpriteRegistry::instance().sync_all(Camera::instance());
    }
    void core_update() { bn::core::update(); }
};

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
}

void LevelManager::_load_door(const LevelData& level) {
    _door.emplace(level.door);
}

unsigned LevelManager::_deaths_in_level() const {
    return _attempt.in_level(_player.get_deaths());
}

void LevelManager::set_deaths_in_level(unsigned deaths) {
    FlowOps ops{*this, _level};
    level_flow::start_replay_at(ops, deaths);
}

// The door that moves between attempts stands where the deaths in this level
// put it (on load, and after every death, checkpoint respawn and Restart
// Level - they all end in _finish_respawn()).
void LevelManager::_place_door() {
    if (_door) {
        _door->place_for_attempt(_deaths_in_level());
    }
}

void LevelManager::_load_music(const LevelData& level) {
    if (!_music.has_value() || *_music != level.music) {
        _music.emplace(level.music);
        bn::music::play(*_music);
        _boss_music = false;
    }
}

// The world theme plays until the boss fight starts (the duck is in the
// arena), then the module's boss section: the same instruments and tune, but
// faster and driving (tools/Make_boss_music.py). A cut at the moment the
// arena is entered. When the fight ends - the boss is beaten, or a death
// starts it over - the theme begins again from its soft start, which is also
// the relief after the last hit. Only the song position changes, so the
// music volume (AudioSettings) stays as it is.
void LevelManager::_update_boss_music() {
    if (_level == nullptr || _level->boss_music_order <= 0 ||
        !bn::music::playing()) {
        return;
    }
    const bool fighting = boss_fighting();
    if (fighting != _boss_music) {
        _boss_music = fighting;
        const int order = fighting ? _level->boss_music_order : 0;
        bn::music::set_position(order);
        tour::boss_music(order);
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
    _camera_rig.place(
        camera_rig::max_offset(level.world_width, Cfg::Screen::WIDTH),
        camera_rig::max_offset(level.world_height, Cfg::Screen::HEIGHT),
        _player.pos.x, _player.pos.y);
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
    camera_rig::aim(
        _camera_rig, _player.pos.x, _player.pos.y, _player.vel_x,
        bn::keypad::up_held(), bn::keypad::down_held());
    Camera::instance().follow(_camera_rig.x(), _camera_rig.y());
}

// The level is not the one of the last attempt: a low rustle and a short
// shake as the new layout appears (after the death freeze - _finish_respawn()
// runs when it ends).
void LevelManager::_cue_layout_change(int shake_frames) {
    _death_fx.shake(shake_frames);
    AudioSettings::instance().play_sfx(bn::sound_items::layout_shift);
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
    int entity_sprites =
        level.platform_count + level.trap_count + level.checkpoint_count;
    for (int i = 0; i < level.trigger_count; ++i) {
        entity_sprites += level.triggers[i].sprite != nullptr ? 1 : 0;
    }
    BN_ASSERT(
        level_limits::within(
            entity_sprites, Cfg::Level::Limits::ENTITY_SPRITES),
        "Too many platform, trap, checkpoint and switch sprites together");
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
        raw_sprite.set_horizontal_flip(platform.flip_horizontal);

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
    // _platforms / _platform_bodies hold exactly one entry per platform, in
    // the same order as level.platforms (see _load_platforms()).
    struct Ops {
        LevelManager& m;
        const LevelData& level;
        [[nodiscard]] bool present(unsigned from, unsigned until) const {
            return m._present(from, until);
        }
        void clear_dynamic() { m._dynamic_platforms.clear(); }
        void clear_fleeing() { m._fleeing_platforms.clear(); }
        void add_fleeing(int i) {
            const PlatformData& platform = level.platforms[i];
            m._fleeing_platforms.add(
                m._platform_bodies[i], *m._platforms[i], platform.x, platform.y,
                platform.flee_path.data(), platform.flee_path.size(),
                platform.flee_range, platform.flee_frames);
        }
        void add_switched(int i) {
            const PlatformData& platform = level.platforms[i];
            BN_ASSERT(
                platform.crumble_frames == 0,
                "A platform cannot be switched and crumbling");
            m._dynamic_platforms.add_switched(
                m.get_trigger_by_name(platform.trigger_name),
                m._platform_bodies[i], *m._platforms[i],
                platform.active_when_on);
        }
        void add_crumbling(int i) {
            const PlatformData& platform = level.platforms[i];
            m._dynamic_platforms.add_crumbling(
                m._platform_bodies[i], *m._platforms[i],
                platform.crumble_frames, platform.crumble_respawn_frames);
        }
    };
    Ops ops{*this, level};
    attempt_objects::load_dynamic(level, ops);
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
    // Objects without a window are never touched. _traps holds one trap per
    // level.traps entry, in order (_load_traps()).
    struct Ops {
        LevelManager& m;
        [[nodiscard]] bool present(unsigned from, unsigned until) const {
            return m._present(from, until);
        }
        void show_platform(int i) {
            m._platform_bodies[i].enable();
            m._platforms[i]->enable();
        }
        void hide_platform(int i) {
            m._platform_bodies[i].disable();
            m._platforms[i]->disable();
        }
        void set_trap_present(int i, bool present) {
            m._traps[i]->set_present(present);
        }
    };
    Ops ops{*this};
    attempt_objects::apply(level, ops);
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
    if (!locked) {
        _door->unlock_glow();
    }
}

auto LevelManager::boss_fighting() const -> bool {
    return _boss != nullptr && _boss->fighting();
}

void LevelManager::flash_door(bool lit) {
    if (_door) {
        _door->set_flash(lit);
    }
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

void LevelManager::load(const LevelData& level) {
    FlowOps ops{*this, &level};
    level_flow::load(ops);
}

void LevelManager::unload() {
    FlowOps ops{*this, _level};
    level_flow::unload(ops);
}

LevelManager::UpdateResult LevelManager::update() {
    FlowOps ops{*this, _level};
    switch (level_flow::update(ops)) {
        case level_flow::Result::LevelComplete:
            return UpdateResult::LevelComplete;
        case level_flow::Result::ReturnToTitle:
            return UpdateResult::ReturnToTitle;
        default:
            return UpdateResult::None;
    }
}
