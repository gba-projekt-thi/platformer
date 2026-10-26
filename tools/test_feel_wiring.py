"""Regression guard for the game-feel wiring (hint, camera, death feedback).

The pure rules (hint_logic.h, camera_rig.h, death_fx.h) are unit-tested on the
host, but the glue that calls them lives in Butano-dependent files. A function
that is declared but never called compiles and links fine and simply does
nothing (that is how the dash once lost its wiring), so these plain text checks
pin the calls down. They run anywhere Python does.

Run from the repository root:

    python3 -m unittest discover -s tools -p 'test_*.py'
"""

import re
import unittest
from pathlib import Path


def _find(name):
    here = Path(__file__).resolve().parent
    if (here / name).exists():
        return here / name
    for path in sorted(here.parent.glob(f"game/**/{name}")):
        return path
    return None


def _body(text, signature):
    match = re.search(signature + r"\s*\{", text)
    if not match:
        return None
    depth, start = 1, match.end()
    for i in range(start, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[start:i]
    return None


SCENE = _find("level_scene.cpp")
MANAGER_H = _find("level_manager.h")
MANAGER = _find("level_manager.cpp")
PAUSE = _find("pause_controller.cpp")
PAUSE_H = _find("pause_controller.h")
PLAYER = _find("player.cpp")
PLAYER_H = _find("player.h")


@unittest.skipIf(SCENE is None or MANAGER_H is None, "level_scene.cpp / level_manager.h not found")
class HintWiringTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.text = SCENE.read_text()
        cls.update = _body(cls.text, r"void\s+LevelScene::update\s*\(\s*\)")
        cls.update_hint = _body(cls.text, r"void\s+LevelScene::_update_hint\s*\(\s*\)")
        cls.show = _body(cls.text, r"void\s+LevelScene::_show_hint_if_needed\s*\(\s*\)")
        cls.draw = _body(cls.text, r"void\s+LevelScene::_draw_hint\s*\(\s*\)")
        cls.manager_h = MANAGER_H.read_text()

    def test_helpers_are_defined(self):
        for name in ("update_hint", "show", "draw", "update"):
            self.assertIsNotNone(getattr(self, name), name)

    def test_level_manager_exposes_the_pause_state(self):
        self.assertRegex(self.manager_h, r"auto\s+paused\s*\(\s*\)\s*const\s*->\s*bool")
        self.assertIn("_pause_controller.paused()", self.manager_h)

    def test_the_hint_follows_the_pause_state(self):
        self.assertIn("_level_manager.paused()", self.update_hint)
        self.assertIn("hint::should_draw(", self.update_hint)
        self.assertIn("_draw_hint()", self.update_hint)

    def test_the_countdown_and_b_only_run_while_the_game_does(self):
        gate = self.update_hint.index("if (!paused)")
        self.assertLess(gate, self.update_hint.index("b_pressed()"))
        self.assertLess(gate, self.update_hint.index("_hint_timer.tick()"))

    def test_the_hint_sprites_are_freed_before_the_pause_overlay_allocates(self):
        # LevelManager::update() opens the pause menu, which allocates ~57
        # sprites in that very call, so the hint has to go first.
        free = self.update.index("_hint_sprites.clear()")
        self.assertIn("start_pressed()", self.update[:free])
        self.assertLess(free, self.update.index("_level_manager.update()"))

    def test_the_hint_text_is_kept_to_draw_it_again_after_a_pause(self):
        self.assertIn("_hint_text = text", self.show)
        self.assertIn("_hint_text", self.draw)
        self.assertIn("_draw_hint()", self.show)


@unittest.skipIf(MANAGER is None or MANAGER_H is None, "level_manager.cpp / level_manager.h not found")
class CameraWiringTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.text = MANAGER.read_text()
        cls.header = MANAGER_H.read_text()
        cls.update = _body(cls.text, r"LevelManager::UpdateResult\s+LevelManager::update\s*\(\s*\)")
        cls.setup = _body(cls.text, r"void\s+LevelManager::_setup_camera\s*\(\s*const LevelData&\s*\w+\s*\)")
        cls.step = _body(cls.text, r"void\s+LevelManager::_update_camera\s*\(\s*\)")

    def test_helpers_are_defined(self):
        self.assertIsNotNone(self.setup)
        self.assertIsNotNone(self.step)
        self.assertIn("CameraRig<bn::fixed> _camera_rig", self.header)

    def test_update_steps_the_rig_instead_of_gluing_the_camera_to_the_duck(self):
        self.assertIn("_update_camera()", self.update)
        self.assertNotIn("follow(_player.pos.x", self.update)
        cam = self.update.index("_update_camera()")
        self.assertLess(cam, self.update.index("_render_frame()", cam))

    def test_the_step_feeds_the_rig_and_follows_its_result(self):
        self.assertIn("_camera_rig.update(", self.step)
        self.assertIn("_player.vel_x", self.step)
        self.assertIn("up_held()", self.step)
        self.assertIn("down_held()", self.step)
        self.assertIn("follow(_camera_rig.x(), _camera_rig.y())", self.step)

    def test_level_load_sets_the_bounds_and_cuts_to_the_duck(self):
        self.assertIn("_camera_rig.set_bounds(", self.setup)
        self.assertIn("camera_rig::max_offset(", self.setup)
        self.assertLess(self.setup.index("set_bounds("), self.setup.index("snap("))
        self.assertLess(self.setup.index("snap("), self.setup.index("follow(_camera_rig.x()"))

    def test_a_respawn_cuts_instead_of_gliding(self):
        # The respawn (immediately, or when the death freeze ends) goes through
        # _finish_respawn(), which re-arms the level and then cuts the camera.
        death = self.update[self.update.index("_save_sync.sync()"):]
        self.assertIn("_finish_respawn()", death)
        finish = _body(self.text, r"void\s+LevelManager::_finish_respawn\s*\(\s*\)")
        self.assertIsNotNone(finish)
        self.assertLess(finish.index("_reset_entities()"), finish.index("_camera_rig.snap("))
        self.assertLess(finish.index("_camera_rig.snap("), finish.index("follow(_camera_rig.x()"))


@unittest.skipIf(MANAGER is None or MANAGER_H is None, "level_manager.cpp / level_manager.h not found")
class DeathFeedbackWiringTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.text = MANAGER.read_text()
        cls.header = MANAGER_H.read_text()
        cls.update = _body(cls.text, r"LevelManager::UpdateResult\s+LevelManager::update\s*\(\s*\)")
        cls.check = _body(cls.text, r"void\s+LevelManager::_check_player_died\s*\(\s*\)")
        cls.render = _body(cls.text, r"void\s+LevelManager::_render_frame\s*\(\s*\)")
        cls.advance = _body(cls.text, r"void\s+LevelManager::_advance_death_fx\s*\(\s*\)")
        cls.finish = _body(cls.text, r"void\s+LevelManager::_finish_respawn\s*\(\s*\)")
        cls.load = _body(cls.text, r"void\s+LevelManager::_load_player_spawn\s*\(\s*const LevelData&\s*\w+\s*\)")
        cls.unload = _body(cls.text, r"void\s+LevelManager::unload\s*\(\s*\)")

    def test_helpers_are_defined(self):
        for name in ("check", "render", "advance", "finish", "load", "unload"):
            self.assertIsNotNone(getattr(self, name), name)
        self.assertIn("death_fx::Sequence _death_fx", self.header)

    def test_a_death_is_detected_right_after_the_physics_step(self):
        self.assertIn("_check_player_died()", self.update)
        check = self.update.index("_check_player_died()")
        self.assertLess(self.update.index("update_all()"), check)
        self.assertLess(check, self.update.index("_render_frame()", check))

    def test_the_check_starts_the_freeze_and_hides_the_duck(self):
        self.assertIn("_death_fx.start(", self.check)
        self.assertIn("Cfg::DeathFx::HITSTOP_FRAMES", self.check)
        self.assertIn("Cfg::DeathFx::SHAKE_FRAMES", self.check)
        self.assertIn("_player.begin_death_fx()", self.check)
        self.assertIn("_skip_death_fx", self.check)

    def test_restart_level_is_a_death_on_purpose_without_feedback(self):
        restart = self.update[self.update.index("RestartRequested"):]
        self.assertLess(restart.index("_skip_death_fx = true"), restart.index("_player.death()"))
        # ... and a freeze that was still running is cancelled first.
        self.assertLess(restart.index("_player.cancel_death_fx()"), restart.index("_player.death()"))
        self.assertLess(restart.index("_death_fx.clear()"), restart.index("_player.death()"))

    def test_a_frozen_world_skips_the_simulation_and_the_level_clock(self):
        frozen = self.update.index("_death_fx.frozen()")
        self.assertLess(frozen, self.update.index("++_level_frame_count"))
        self.assertLess(frozen, self.update.index("update_all()"))
        branch = self.update[frozen:self.update.index("++_level_frame_count")]
        self.assertIn("_render_frame()", branch)
        self.assertIn("_advance_death_fx()", branch)
        self.assertIn("return UpdateResult::None", branch)

    def test_the_pause_check_comes_before_the_freeze(self):
        self.assertLess(self.update.index("_pause_controller.paused()"), self.update.index("_death_fx.frozen()"))

    def test_the_camera_stays_on_the_death_spot_while_frozen(self):
        self.assertRegex(self.update, r"if\s*\(!_death_fx\.frozen\(\)\)\s*\{\s*_update_camera\(\);")

    def test_every_rendered_frame_goes_through_render_and_advance(self):
        self.assertIn("set_shake(", self.render)
        self.assertIn("shake_offset(Cfg::DeathFx::SHAKE_AMPLITUDE)", self.render)
        self.assertLess(self.render.index("set_shake("), self.render.index("sync_all("))
        self.assertLess(self.render.index("sync_all("), self.render.index("bn::core::update()"))
        self.assertNotIn("sync_all(", self.update)

    def test_the_end_of_the_freeze_brings_the_duck_back_and_runs_the_deferred_reset(self):
        self.assertIn("_death_fx.advance()", self.advance)
        self.assertIn("_player.end_death_fx()", self.advance)
        self.assertIn("_respawn_pending", self.advance)
        self.assertIn("_finish_respawn()", self.advance)

    def test_the_trap_reset_waits_for_the_freeze(self):
        death = self.update[self.update.index("_save_sync.sync()"):]
        self.assertIn("_death_fx.frozen()", death)
        self.assertIn("_respawn_pending = true", death)
        self.assertIn("_finish_respawn()", death)

    def test_the_respawn_cuts_the_camera(self):
        # The reset order is pinned in test_level_manager_wiring.py.
        self.assertIn("_camera_rig.snap(", self.finish)

    def test_level_load_and_unload_leave_no_feedback_behind(self):
        for body in (self.load, self.unload):
            self.assertIn("_death_fx.clear()", body)
            self.assertIn("_player.cancel_death_fx()", body)
            self.assertIn("set_shake(0, 0)", body)
        self.assertIn("_deaths_seen = _player.get_deaths()", self.load)


@unittest.skipIf(PLAYER is None or PLAYER_H is None, "player.cpp / player.h not found")
class PlayerDeathFeedbackWiringTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.text = PLAYER.read_text()
        cls.header = PLAYER_H.read_text()
        cls.update = _body(cls.text, r"void\s+Player::update\s*\(\s*\)")
        cls.death = _body(cls.text, r"void\s+Player::death\s*\(\s*\)")
        cls.begin = _body(cls.text, r"void\s+Player::begin_death_fx\s*\(\s*\)")
        cls.end = _body(cls.text, r"void\s+Player::end_death_fx\s*\(\s*\)")
        cls.cancel = _body(cls.text, r"void\s+Player::cancel_death_fx\s*\(\s*\)")
        cls.blink = _body(cls.text, r"void\s+Player::_update_respawn_blink\s*\(\s*\)")

    def test_helpers_are_defined(self):
        for name in ("update", "death", "begin", "end", "cancel", "blink"):
            self.assertIsNotNone(getattr(self, name), name)
        for name in ("begin_death_fx", "end_death_fx", "cancel_death_fx"):
            self.assertIn(name, self.header)

    def test_death_remembers_where_it_happened_before_the_teleport(self):
        self.assertLess(self.death.index("_death_x = pos.x"), self.death.index("pos.x = _restart_x"))
        self.assertLess(self.death.index("_death_y = pos.y"), self.death.index("pos.y = _restart_y"))

    def test_the_ghost_is_a_registered_copy_of_the_last_frame_and_the_duck_is_hidden(self):
        self.assertIn("_death_ghost.emplace(", self.begin)
        self.assertIn("set_tiles(_player_sprite.sprite().tiles())", self.begin)
        self.assertIn("_player_sprite.disable()", self.begin)

    def test_ending_the_freeze_shows_the_duck_and_starts_the_flicker(self):
        self.assertIn("cancel_death_fx()", self.end)
        self.assertIn("Cfg::DeathFx::RESPAWN_BLINK_FRAMES", self.end)
        self.assertIn("_player_sprite.enable()", self.cancel)
        self.assertIn("_death_ghost.reset()", self.cancel)

    def test_the_flicker_runs_in_the_physics_step_and_uses_the_pure_pattern(self):
        self.assertIn("_update_respawn_blink()", self.update)
        self.assertIn("death_fx::blink_visible(", self.blink)
        self.assertIn("is_enabled()", self.blink)


@unittest.skipIf(PAUSE is None or PAUSE_H is None, "pause_controller.cpp / pause_controller.h not found")
class PauseOverlayWiringTests(unittest.TestCase):
    """The pause overlay must not keep sprites while the level runs: Butano
    counts hidden sprites too (see tools/Check_sprite_budget.py)."""

    @classmethod
    def setUpClass(cls):
        cls.text = PAUSE.read_text()
        cls.header = PAUSE_H.read_text()
        cls.init = _body(cls.text, r"void\s+PauseController::_init_pause_menu\s*\(\s*\)")
        cls.release = _body(cls.text, r"void\s+PauseController::_release_sprites\s*\(\s*\)")
        cls.show_title = _body(cls.text, r"void\s+PauseController::_show_title\s*\(\s*\)")
        cls.reset = _body(cls.text, r"void\s+PauseController::reset\s*\(\s*\)")
        cls.update = _body(cls.text, r"PauseController::Action\s+PauseController::update\s*\(\s*\)")
        cls.controls = _body(cls.text, r"void\s+PauseController::_rebuild_controls_view\s*\(\s*\)")

    def test_helpers_are_defined(self):
        for name in ("init", "release", "show_title", "reset", "update", "controls"):
            self.assertIsNotNone(getattr(self, name), name)

    def test_nothing_is_hidden_any_more(self):
        # a hidden sprite keeps its slot: the old _set_visible(false) left the
        # last menu allocated for the rest of the level after the first pause
        self.assertNotIn("_set_visible", self.text)
        self.assertNotIn("_set_visible", self.header)

    def test_the_title_is_not_created_at_level_load(self):
        self.assertNotIn("generate(", self.init)
        self.assertNotIn("_title_sprites", self.init)

    def test_the_release_frees_the_title_and_the_rows(self):
        self.assertIn("_title_sprites.clear()", self.release)
        self.assertIn("_menu_sprites.clear()", self.release)

    def test_the_title_is_created_when_the_game_pauses_and_when_leaving_the_controls(self):
        self.assertIn('"Paused"', self.show_title)
        opened = self.update[self.update.index("if (_paused) {"):]
        self.assertLess(opened.index("_rebuild_menu()"), opened.index("_show_title()"))
        back = self.update[self.update.index("if (_showing_controls)"):]
        self.assertIn("_show_title()", back[: back.index("_showing_options")])

    def test_every_way_of_resuming_releases_the_sprites(self):
        # level load, Start (also from inside a sub-menu), Continue / Restart / Title
        self.assertIn("_release_sprites()", self.reset)
        toggle = self.update[: self.update.index("if (_paused) {\n        if (_showing_controls)")]
        self.assertIn("_release_sprites()", toggle)
        action = self.update[self.update.index("Leave the pause state immediately"):]
        self.assertLess(action.index("_paused = false"), action.index("_release_sprites()"))
        self.assertLess(action.index("_release_sprites()"), action.index("switch (option)"))

    def test_the_controls_view_drops_the_title(self):
        self.assertIn("_title_sprites.clear()", self.controls)


if __name__ == "__main__":
    unittest.main()
