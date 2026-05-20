#pragma once

// Butano
#include "bn_array.h"
#include "bn_fixed.h"
#include "bn_keypad.h"
#include "bn_optional.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_items_ente.h"
#include "bn_sprite_text_generator.h"

// Game
#include "cfg.h"
#include "common_variable_8x16_sprite_font.h"
#include "death_counter.h"
#include "physics_body.h"
#include "sprite.h"
#include "timer.h"

/**
 * Main controllable player entity.
 *
 * Responsibilities:
 * - Player movement and physics
 * - Input processing
 * - Animation state machine
 * - Respawn handling
 * - HUD updates (timer/death counter)
 *
 * Physics and collision behavior are inherited from PhysicsBody.
 */
class Player : public PhysicsBody {
   public:
    // Layers this player can interact with using overlap tests.
    static constexpr uint16_t MASK = Cfg::Layer::TRAP | Cfg::Layer::DOOR;

    // Layers treated as solid collision geometry.
    static constexpr uint16_t BLOCK =
        0xFFFF & ~Cfg::Layer::TRAP & ~Cfg::Layer::TRIGGER & ~Cfg::Layer::DOOR;

   public:
    Player(
        bn::fixed start_x,
        bn::fixed start_y,
        bn::fixed width,
        bn::fixed height);

    /**
     * Main per-frame update.
     *
     * Handles:
     * - Input
     * - Movement
     * - Physics
     * - State machine
     * - Animation
     * - HUD updates
     */
    void update() override;

    // Kill and respawn the player.
    void death();

    // Set the respawn position.
    void set_spawn_point(bn::fixed x, bn::fixed y);

    // Instantly move the player to a new position.
    void teleport_to(bn::fixed x, bn::fixed y);

    // Current player death count.
    [[nodiscard]]
    unsigned int get_deaths() const;

    // Restore/set death counter value.
    void set_deaths(unsigned int deaths);

    // Access the gameplay timer.
    [[nodiscard]]
    Timer& get_timer();

   private:
    /**
     * Cached player input snapshot.
     *
     * Input is read once per frame to:
     * - avoid repeated keypad polling
     * - simplify logic flow
     * - improve readability
     */
    struct Input {
        bool left = false;
        bool right = false;

        bool up = false;
        bool down = false;

        bool jump_pressed = false;
        bool jump_held = false;

        [[nodiscard]]
        bool moving() const {
            return left || right;
        }
    };

    /**
     * High-level player movement state.
     *
     * Used to drive animation and gameplay logic.
     */
    enum class PlayerState { Idle, Run, Jump, Fall, LookUp, Crouch };

   private:
    // Main visual sprite wrapper.
    Sprite player_sprite;

    /**
     * Cached animation tile handles.
     *
     * Preloaded once at startup to avoid:
     * - runtime VRAM allocation
     * - tile creation during gameplay
     */
    bn::optional<
        bn::array<bn::sprite_tiles_ptr, Cfg::Player::PLAYER_TILE_CACHE_SIZE>>
        cached_tiles;

    // Walking animation action.
    bn::sprite_animate_action<Cfg::Player::ANIMATION_FRAME_COUNT> walk_action;

    // Jump animation action.
    bn::sprite_animate_action<Cfg::Player::ANIMATION_FRAME_COUNT> jump_action;

    // Death tracking.
    DeathCounter death_counter;

    // Text generator for HUD rendering.
    bn::sprite_text_generator death_text_generator;

    // Death counter HUD widget.
    DeathCounterHUD death_hud;

    // Gameplay timer.
    Timer timer;

    // Timer HUD widget.
    TimerHUD timer_hud;

    // Respawn position.
    bn::fixed restart_x;
    bn::fixed restart_y;

    // Whether the player is currently grounded.
    bool on_ground = true;

    // Remaining coyote-time frames.
    int coyote_timer = 0;

    // Remaining jump-buffer frames.
    int jump_buffer_timer = 0;

    // Current active player state.
    PlayerState state = PlayerState::Idle;

    // Previous state for transition tracking.
    PlayerState previous_state = PlayerState::Idle;

    /**
     * Currently displayed tile index.
     *
     * Used to avoid redundant VRAM tile swaps.
     */
    int current_tile = -1;

   private:
    // Read all player input for the current frame.
    void process_input(Input& input);

    // Apply horizontal movement acceleration and friction.
    void handle_horizontal_input(const Input& input);

    // Execute buffered jump logic.
    void handle_jump();

    // Apply gravity to vertical velocity.
    inline void apply_gravity();

    // Allow variable jump height by cutting jump momentum.
    inline void apply_variable_jump(const Input& input);

    // Prevent excessive falling speed.
    inline void clamp_velocity();

    // Detect grounded state and update coyote timer.
    void update_ground_state();

    // Update player state machine.
    void update_state(const Input& input);

    // Handle state transitions.
    void enter_state(PlayerState new_state);

    // Update sprite animation from current state.
    void update_animation();

    // Prevent leaving horizontal play area.
    void check_bounds();

    // Detect death conditions.
    void check_death();

    /**
     * Change sprite tile only if necessary.
     *
     * Prevents redundant VRAM updates.
     */
    inline void set_tile(int index);
};