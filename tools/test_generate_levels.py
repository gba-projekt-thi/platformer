"""Unit tests for tools/Generate_levels.py.

Run from the repository root:

    python3 -m unittest discover -s tools -p 'test_*.py'
"""

import importlib.util
import json
import re
import tempfile
import unittest
from pathlib import Path

_SCRIPT = Path(__file__).with_name("Generate_levels.py")
_spec = importlib.util.spec_from_file_location("generate_levels", _SCRIPT)
gen = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(gen)

REPO = Path(__file__).resolve().parent.parent
STRUCTS = gen.read_structs(REPO / "game/include/level/level_structure.h")


def squash(text):
    """C++ text without whitespace, for layout-independent comparisons."""
    return re.sub(r"\s+", "", text)


def level(**fields):
    base = {
        "name": "LEVEL_X",
        "platforms": {
            "name": "x_platforms",
            "items": [{"x": 1, "y": 2, "width": 16, "height": 8, "offset_x": 0,
                       "offset_y": 0, "sprite": "stone", "sprite_index": 0}],
        },
        "background": "world9",
        "music": "world9",
        "spawn": [-10, 20],
        "door": {"x": 30, "y": 4},
    }
    base.update(fields)
    return base


def world(*levels, note=None):
    data = {"levels": list(levels)}
    if note:
        data["note"] = note
    return data


class StructTests(unittest.TestCase):
    def test_field_order_and_defaults_come_from_level_structure_h(self):
        platform = STRUCTS["PlatformData"]
        self.assertEqual([f for f, _ in platform[:3]], ["x", "y", "width"])
        self.assertEqual(dict(platform)["active_when_on"], "true")
        self.assertEqual(dict(STRUCTS["TriggerData"])["mode"], "TriggerMode::Latch")
        self.assertEqual(STRUCTS["TrapData"][0][0], "type")


class GenerateTests(unittest.TestCase):
    def header(self, *levels, note=None):
        return gen.generate(world(*levels, note=note), STRUCTS)

    def test_platform_items_are_positional_with_sprite_items(self):
        out = squash(self.header(level()))
        self.assertIn("constexprPlatformDatax_platforms[]={{1,2,16,8,0,0,bn::sprite_items::stone,0}};", out)

    def test_level_data_points_at_its_arrays_and_counts_them(self):
        out = squash(self.header(level()))
        self.assertIn(
            "constLevelDataLEVEL_X={x_platforms,sizeof(x_platforms)/sizeof(x_platforms[0]),"
            "nullptr,0,nullptr,0,bn::regular_bg_items::world9,bn::music_items::world9,"
            "{-10,20},{30,4}};",
            out,
        )

    def test_a_field_after_skipped_defaults_gets_the_defaults_in_between(self):
        items = [{"x": 0, "y": 0, "width": 16, "height": 8, "offset_x": 0, "offset_y": 0,
                  "sprite": "stone", "sprite_index": 0, "crumble_frames": 30}]
        out = squash(self.header(level(platforms={"name": "p", "items": items})))
        self.assertIn("bn::sprite_items::stone,0,nullptr,true,30}", out)

    def test_trigger_symbols_strings_and_sprite_pointer(self):
        triggers = {"name": "t", "items": [{"x": 0, "y": 0, "width": 8, "height": 8,
                                            "default_on": False, "name": "lever1",
                                            "mode": "Toggle", "sprite": "lever16x16"}]}
        out = squash(self.header(level(triggers=triggers)))
        self.assertIn('{0,0,8,8,false,"lever1",TriggerMode::Toggle,&bn::sprite_items::lever16x16}', out)
        self.assertIn("t,sizeof(t)/sizeof(t[0])", out)

    def test_trap_type_and_named_spans(self):
        traps = {"name": "tr", "items": [{"type": "PATH", "x": 0, "y": 0, "width": 32, "height": 32,
                                          "offset_x": 0, "offset_y": 0, "sprite": "bubbles",
                                          "sprite_waits": 10, "graphic_indexes": "TRAP_GRAPHICS_INDEXES_3",
                                          "velocity_x": 0, "velocity_y": 0, "max_vel": 0,
                                          "path": "FIGURE_8_PATH", "path_waits": 15,
                                          "trigger_name": "gate"}]}
        out = squash(self.header(level(traps=traps)))
        self.assertIn(
            '{TrapType::PATH,0,0,32,32,0,0,bn::sprite_items::bubbles,10,TRAP_GRAPHICS_INDEXES_3,'
            '0,0,0,FIGURE_8_PATH,15,0,0,"gate"}',
            out,
        )

    def test_fields_without_an_in_class_default_are_always_written(self):
        # TrapData::path has no default member initializer: leaving it out of
        # an aggregate literal is a -Wmissing-field-initializers warning.
        traps = {"name": "tr", "items": [{"type": "BASE", "x": 0, "y": 0, "width": 8, "height": 8,
                                          "offset_x": 0, "offset_y": 0, "sprite": "b"}]}
        self.assertIn(
            "bn::sprite_items::b,1,TRAP_GRAPHICS_INDEXES_0,0,0,0,NO_PATH_TRAP}",
            squash(self.header(level(traps=traps))),
        )

    def test_numbers_keep_their_value(self):
        traps = {"name": "tr", "items": [{"type": "MOVING", "x": 0, "y": 0, "width": 8, "height": 8,
                                          "offset_x": 0, "offset_y": 0, "sprite": "b",
                                          "sprite_waits": 1, "graphic_indexes": "TRAP_GRAPHICS_INDEXES_0",
                                          "velocity_x": -0.1, "velocity_y": 1.25}]}
        self.assertIn("TRAP_GRAPHICS_INDEXES_0,-0.1,1.25,0,NO_PATH_TRAP}", squash(self.header(level(traps=traps))))

    def test_other_expressions_are_written_as_they_are(self):
        traps = {"name": "tr", "items": [{"type": "BASE", "x": 0, "y": 0, "width": 8, "height": 8,
                                          "offset_x": 0, "offset_y": 0, "sprite": "door32x32",
                                          "sprite_z_order": "Cfg::ZOrder::DOOR",
                                          "graphic_indexes": "TRAP_GRAPHICS_INDEXES_0"}]}
        self.assertIn(",Cfg::ZOrder::DOOR}", squash(self.header(level(traps=traps))))

    def test_optional_level_fields_and_checkpoints(self):
        lvl = level(world_width=624, intro_hint="New: Dash with B",
                    checkpoints={"name": "c", "items": [{"x": 5, "y": 6}]})
        out = squash(self.header(lvl))
        self.assertIn('{30,4},624,Cfg::Screen::HEIGHT,1,"New:DashwithB",c,sizeof(c)/sizeof(c[0])};', out)

    def test_door_with_its_own_sprite(self):
        door = {"x": 160, "y": 40, "sprite": "susanne16x16",
                "sprite_graphics_indexes": "TRAP_GRAPHICS_INDEXES_8",
                "sprite_animation_wait": 8, "sprite_flip_horizontal": True}
        out = squash(self.header(level(door=door)))
        self.assertIn("{160,40,bn::sprite_items::susanne16x16,TRAP_GRAPHICS_INDEXES_8,8,true}", out)

    def test_a_running_door_gets_its_path_as_an_array(self):
        door = {"x": 30, "y": 4, "flee_path": [[60, 0], [60, -40]],
                "flee_range": 24, "flee_frames": 12}
        text = squash(gen.generate(world(level(door=door)), STRUCTS))
        self.assertIn("constexprbn::fixed_pointLEVEL_X_door_flee_path[]={{60,0},{60,-40}};", text)
        self.assertIn("LEVEL_X_door_flee_path,24,12}", text)
        self.assertLess(text.index("LEVEL_X_door_flee_path[]"), text.index("constLevelDataLEVEL_X"))

    def test_a_trapped_door_names_its_trigger(self):
        door = {"x": 30, "y": 4, "trap_trigger_name": "door_trap"}
        out = squash(self.header(level(door=door)))
        self.assertIn(
            '{30,4,bn::sprite_items::door32x32,DOOR32X32_GRAPHICS_INDEXES,8,false,{},0,0,"door_trap"}',
            out,
        )

    def test_a_moving_door_gets_its_attempt_positions_as_an_array(self):
        door = {"x": 30, "y": 4, "attempt_positions": [[0, 0], [-60, 8]], "attempt_every": 2}
        text = squash(gen.generate(world(level(door=door)), STRUCTS))
        self.assertIn("constexprbn::fixed_pointLEVEL_X_door_attempt_positions[]={{0,0},{-60,8}};", text)
        self.assertIn(",{},0,0,nullptr,LEVEL_X_door_attempt_positions,2}", text)
        self.assertLess(text.index("LEVEL_X_door_attempt_positions[]"), text.index("constLevelDataLEVEL_X"))

    def test_a_door_can_run_away_and_move_between_attempts(self):
        door = {"x": 30, "y": 4, "flee_path": [[0, -40]], "flee_range": 24, "flee_frames": 10,
                "attempt_positions": [[0, 0], [90, 0]], "attempt_every": 1}
        text = squash(gen.generate(world(level(door=door)), STRUCTS))
        self.assertIn("LEVEL_X_door_flee_path,24,10,nullptr,LEVEL_X_door_attempt_positions,1}", text)

    def test_attempt_positions_need_attempt_every(self):
        # attempt_every 0 would keep the door at its first spot for good.
        door = {"x": 30, "y": 4, "attempt_positions": [[0, 0], [-60, 0]]}
        with self.assertRaises(ValueError):
            gen.generate(world(level(door=door)), STRUCTS)

    def test_a_running_platform_gets_its_path_as_an_array(self):
        lv = level()
        lv["platforms"]["items"][0].update(
            {"flee_path": [[48, 0]], "flee_range": 32, "flee_frames": 8})
        text = squash(gen.generate(world(lv), STRUCTS))
        self.assertIn("constexprbn::fixed_pointx_platforms_0_flee_path[]={{48,0}};", text)
        self.assertIn("x_platforms_0_flee_path,32,8}", text)
        self.assertLess(text.index("x_platforms_0_flee_path[]"), text.index("constexprPlatformDatax_platforms"))

    def test_death_window_is_written_last_for_platforms_and_traps(self):
        lv = level()
        lv["platforms"]["items"][0].update({"from_death": 3})
        lv["traps"] = {"name": "tr", "items": [
            {"type": "BASE", "x": 0, "y": 0, "width": 8, "height": 8, "offset_x": 0,
             "offset_y": 0, "sprite": "b", "until_death": 3}]}
        text = squash(gen.generate(world(lv), STRUCTS))
        # PlatformData: the defaults up to flee_frames, then from 3, until 0
        self.assertIn("bn::sprite_items::stone,0,nullptr,true,0,0,{},0,0,3}", text)
        self.assertIn(",false,0,3}", text)  # ...ambush_vertical, from 0, until 3

    def test_death_window_fields_come_from_level_structure_h(self):
        for struct in ("PlatformData", "TrapData"):
            fields = [f for f, _ in STRUCTS[struct]]
            self.assertEqual(fields[-2:], ["from_death", "until_death"])
            self.assertEqual(dict(STRUCTS[struct])["from_death"], "0")

    def test_an_empty_death_window_is_an_error(self):
        lv = level()
        lv["platforms"]["items"][0].update({"from_death": 3, "until_death": 3})
        with self.assertRaisesRegex(ValueError, "never there"):
            gen.generate(world(lv), STRUCTS)

    def test_a_negative_death_window_is_an_error(self):
        lv = level()
        lv["platforms"]["items"][0].update({"from_death": -1})
        with self.assertRaisesRegex(ValueError, "from_death"):
            gen.generate(world(lv), STRUCTS)

    def test_a_boss_cannot_have_a_death_window(self):
        lv = level(traps={"name": "tr", "items": [
            {"type": "BOSS", "x": 0, "y": 0, "width": 8, "height": 8, "offset_x": 0,
             "offset_y": 0, "sprite": "b", "from_death": 2}]})
        with self.assertRaisesRegex(ValueError, "BOSS"):
            gen.generate(world(lv), STRUCTS)

    def test_includes_follow_the_assets_used(self):
        out = self.header(level())
        for include in ("bn_music_items.h", "bn_regular_bg_items_world9.h",
                        "bn_sprite_items_stone.h", "level_structure.h", "levels_common.h"):
            self.assertIn(f'#include "{include}"', out)

    def test_notes_become_comments(self):
        lvl = level(note="The first level.\nJust a walk.")
        lvl["platforms"]["items"][0]["note"] = "Start ledge"
        lvl["field_notes"] = {"door": "Door position"}
        out = self.header(lvl, note="World 9")
        for comment in ("// World 9", "// The first level.", "// Just a walk.",
                        "// Start ledge", "// Door position"):
            self.assertIn(comment, out)

    def test_end_note_closes_the_header(self):
        data = world(level())
        data["end_note"] = "Last words."
        self.assertTrue(gen.generate(data, STRUCTS).endswith("// Last words.\n"))

    def test_header_says_it_is_generated(self):
        self.assertIn("Generated by tools/Generate_levels.py", self.header(level()))


class DumpsTests(unittest.TestCase):
    def test_round_trip_keeps_everything(self):
        data = world(level(note="a\nb"))
        self.assertEqual(json.loads(gen.dumps(data)), data)

    def test_one_line_per_item_and_inline_spawn(self):
        text = gen.dumps(world(level()))
        self.assertIn('{"x": 1, "y": 2, "width": 16', text)
        self.assertIn('"spawn": [-10, 20]', text)


class CheckTests(unittest.TestCase):
    def test_check_ignores_layout_but_not_values(self):
        data = world(level())
        text = gen.generate(data, STRUCTS)
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "levels_world9.h"
            path.write_text(text.replace(",", ",   \n"))
            self.assertTrue(gen.matches(path, data, STRUCTS))
            path.write_text(text.replace("{1, 2,", "{1, 3,"))
            self.assertFalse(gen.matches(path, data, STRUCTS))

    def test_check_ignores_comments(self):
        data = world(level())
        text = gen.generate(data, STRUCTS)
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "levels_world9.h"
            path.write_text(text.replace("#pragma once", "#pragma once\n// extra"))
            self.assertTrue(gen.matches(path, data, STRUCTS))


@unittest.skipUnless((REPO / "game/levels").is_dir(), "game/levels/ not found")
class RealLevelsTests(unittest.TestCase):
    def test_every_header_matches_its_json(self):
        for source in sorted((REPO / "game/levels").glob("world*.json")):
            header = REPO / "game/include/level" / f"levels_{source.stem}.h"
            self.assertTrue(
                gen.matches(header, gen.load(source), STRUCTS),
                f"{header.name} is out of date: run python3 tools/Generate_levels.py",
            )


if __name__ == "__main__":
    unittest.main()
