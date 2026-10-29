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

    def test_a_following_trigger_names_its_source_last(self):
        triggers = {"name": "t", "items": [
            {"x": 0, "y": 0, "width": 8, "height": 8, "name": "plate"},
            {"x": 0, "y": 0, "width": 8, "height": 8, "name": "spikes",
             "delay_frames": 40, "follows": "plate"}]}
        out = squash(self.header(level(triggers=triggers)))
        self.assertIn(
            '{0,0,8,8,false,"spikes",TriggerMode::Latch,nullptr,0,1,40,'
            'TriggerAction::Enter,"plate"}',
            out,
        )
        self.assertEqual(STRUCTS["TriggerData"][-1], ("follows", "nullptr"))

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
        self.assertIn(",false,0,0,3}", text)  # ...ambush_vertical, ambush_rest, from 0, until 3

    def test_ambush_rest_is_written_after_ambush_vertical(self):
        lv = level()
        lv["traps"] = {"name": "tr", "items": [
            {"type": "AMBUSH", "x": 0, "y": 0, "width": 8, "height": 8, "offset_x": 0,
             "offset_y": 0, "sprite": "b", "ambush_vertical": True, "ambush_rest": 45}]}
        text = squash(gen.generate(world(lv), STRUCTS))
        self.assertIn(",true,45}", text)

    def test_death_window_fields_come_from_level_structure_h(self):
        for struct in ("PlatformData", "TrapData"):
            fields = [f for f, _ in STRUCTS[struct]]
            self.assertEqual(fields[-3:], ["from_death", "until_death", "flip_horizontal"])
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

    def test_a_misspelled_field_is_an_error(self):
        # Dropped without a word it would turn the Level Devil part off: a
        # trap with "until_deth" is simply always there.
        lv = level()
        lv["platforms"]["items"][0]["until_deth"] = 3
        with self.assertRaisesRegex(ValueError, "until_deth"):
            gen.generate(world(lv), STRUCTS)

    def test_a_door_field_that_does_not_exist_is_an_error(self):
        lv = level()
        lv["door"] = {"x": 1, "y": 2, "from_death": 3}
        with self.assertRaisesRegex(ValueError, "from_death"):
            gen.generate(world(lv), STRUCTS)

    def test_a_boss_cannot_have_a_death_window(self):
        lv = level(traps={"name": "tr", "items": [
            {"type": "BOSS", "x": 0, "y": 0, "width": 8, "height": 8, "offset_x": 0,
             "offset_y": 0, "sprite": "b", "from_death": 2}]})
        with self.assertRaisesRegex(ValueError, "BOSS"):
            gen.generate(world(lv), STRUCTS)

    def test_roster_lists_the_worlds_in_numeric_order_and_file_order(self):
        with tempfile.TemporaryDirectory() as tmp:
            for name, levels in (("world10", ["J"]), ("world2", ["C", "D"]),
                                 ("world1", ["A", "B"])):
                Path(tmp, name + ".json").write_text(
                    json.dumps({"levels": [{"name": n} for n in levels]}))
            self.assertEqual(gen.roster(tmp), [["A", "B"], ["C", "D"], ["J"]])

    def test_roster_header_holds_the_counts_names_and_flat_list(self):
        out = gen.generate_roster([["A", "B"], ["C"]])
        self.assertIn("WORLD_LEVEL_COUNTS[] = {2, 1}", out)
        self.assertIn('{"World 1", "World 2"}', out)
        self.assertIn("#define LEVEL_ROSTER A, B, C", out)

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


def trap(**fields):
    base = {"type": "BASE", "x": 40, "y": 10, "width": 16, "height": 16, "offset_x": 2,
            "offset_y": 0, "sprite": "bubbles"}
    base.update(fields)
    return base


def mirrored(item, kind="traps"):
    return gen.mirror_item(kind, item, {"SWING": [[0, 0], [20, 5]]})


class MirrorTests(unittest.TestCase):
    def test_positions_and_hitbox_offsets_change_sign_and_the_picture_flips(self):
        item = mirrored(trap())
        self.assertEqual((item["x"], item["offset_x"], item["y"]), (-40, -2, 10))
        self.assertTrue(item["flip_horizontal"])
        plat = mirrored({"x": -96, "y": 60, "offset_x": 0, "offset_y": 1}, "platforms")
        self.assertEqual((plat["x"], plat["offset_x"], plat["flip_horizontal"]), (96, 0, True))
        self.assertEqual(mirrored({"x": 0, "y": 1}, "triggers"), {"x": 0, "y": 1})

    def test_zero_stays_zero_and_floats_work(self):
        item = mirrored(trap(x=0, offset_x=0, velocity_x=0.25))
        self.assertEqual((item["x"], item["velocity_x"]), (0, -0.25))
        self.assertEqual(str(item["x"]), "0")

    def test_a_named_path_becomes_its_mirror_image(self):
        item = mirrored(trap(type="PATH", path="SWING"))
        self.assertEqual(item["path"], [[0, 0], [-20, 5]])

    def test_sideways_motion_flips_but_vertical_motion_does_not(self):
        self.assertEqual(mirrored(trap(type="AMBUSH", ambush_speed=-2.5))["ambush_speed"], 2.5)
        up = mirrored(trap(type="AMBUSH", ambush_speed=-2.5, ambush_vertical=True))
        self.assertEqual(up["ambush_speed"], -2.5)
        self.assertEqual(mirrored(trap(type="CYCLE", cycle_dx=6, cycle_dy=-8))["cycle_dx"], -6)
        self.assertEqual(mirrored(trap(type="MOVING", velocity_x=0.1, velocity_y=4))["velocity_y"], 4)

    def test_a_chaser_trails_to_the_right(self):
        item = mirrored(trap(type="CHASE", chase_follow_distance=40, chase_limit=100))
        self.assertEqual((item["chase_follow_distance"], item["chase_limit"]), (-40, 100))

    def test_notes_do_not_travel(self):
        self.assertNotIn("note", mirrored(trap(note="x", blank_before=True)))

    def test_a_level_is_mirrored_with_door_spawn_and_paths(self):
        lv = level(door={"x": 30, "y": 4, "flee_path": [[10, 0]], "attempt_positions": [[0, 0], [8, 2]]},
                   checkpoints={"name": "c", "items": [{"x": -40, "y": 1}]},
                   traps={"name": "t", "items": [trap()]})
        out = gen.mirror_level(lv, {})
        self.assertEqual(out["name"], "LEVEL_X_MIRROR")
        self.assertEqual(out["spawn"], [10, 20])
        self.assertEqual(out["door"]["x"], -30)
        self.assertEqual(out["door"]["flee_path"], [[-10, 0]])
        self.assertEqual(out["door"]["attempt_positions"], [[0, 0], [-8, 2]])
        self.assertTrue(out["door"]["sprite_flip_horizontal"])
        self.assertEqual(out["checkpoints"]["items"], [{"x": 40, "y": 1}])
        self.assertEqual(out["traps"]["name"], "t_mirror")
        self.assertEqual(out["background"], "world9")

    def test_mirroring_twice_gives_the_original_geometry(self):
        lv = level(traps={"name": "t", "items": [trap(velocity_x=-0.5)]})
        back = gen.mirror_level(gen.mirror_level(lv, {}), {})
        self.assertEqual(back["traps"]["items"][0]["x"], 40)
        self.assertEqual(back["traps"]["items"][0]["velocity_x"], -0.5)
        self.assertEqual(back["spawn"], lv["spawn"])
        self.assertFalse(back["door"]["sprite_flip_horizontal"])

    def test_the_level_links_to_its_mirror_and_the_mirror_header_defines_it(self):
        data = world(level())
        text = squash(gen.generate(data, STRUCTS, mirror_header="mirror_x.h"))
        self.assertIn('#include"mirror_x.h"', text)
        self.assertIn("&LEVEL_X_MIRROR};", text)
        mirror = squash(gen.generate_mirror(data, STRUCTS))
        self.assertIn("constLevelDataLEVEL_X_MIRROR=", mirror)
        self.assertNotIn("&LEVEL_X_MIRROR", mirror)

    def test_every_named_path_of_levels_common_h_is_known(self):
        paths = gen.common_paths(REPO / "game/include/level/levels_common.h")
        self.assertEqual(paths["BOSS_PATH"], [[0, 0], [72, 0]])
        self.assertIn("FIGURE_8_PATH", paths)


@unittest.skipUnless((REPO / "game/levels").is_dir(), "game/levels/ not found")
class RealLevelsTests(unittest.TestCase):
    def test_every_header_matches_its_json(self):
        for source in sorted((REPO / "game/levels").glob("world*.json")):
            header = REPO / "game/include/level" / f"levels_{source.stem}.h"
            mirror = REPO / "game/include/level" / f"mirror_{source.stem}.h"
            data = gen.load(source)
            self.assertTrue(
                gen.matches(header, data, STRUCTS, mirror.name),
                f"{header.name} is out of date: run python3 tools/Generate_levels.py",
            )
            self.assertTrue(
                gen.matches_mirror(mirror, data, STRUCTS),
                f"{mirror.name} is out of date: run python3 tools/Generate_levels.py",
            )

    def test_every_level_mirrored_twice_is_the_level_again(self):
        paths = gen.common_paths()
        for source in sorted((REPO / "game/levels").glob("world*.json")):
            for lv in gen.load(source)["levels"]:
                back = gen.mirror_level(gen.mirror_level(lv, paths), paths)
                for kind in ("platforms", "triggers", "traps", "checkpoints"):
                    for a, b in zip((lv.get(kind) or {}).get("items", []),
                                    (back.get(kind) or {}).get("items", [])):
                        for key in ("x", "offset_x", "velocity_x", "cycle_dx"):
                            self.assertEqual(a.get(key, 0), b.get(key, 0), (lv["name"], kind, key))
                self.assertEqual(lv["spawn"], back["spawn"])
                self.assertEqual(lv["door"]["x"], back["door"]["x"])


if __name__ == "__main__":
    unittest.main()
