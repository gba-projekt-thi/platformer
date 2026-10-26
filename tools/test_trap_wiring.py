"""Regression guard for the trap wiring (PathTrap scaling, CycleTrap).

trap_timing.h is unit-tested on the host, but the glue that calls it lives in
Butano-dependent files. A helper that is never called compiles and links fine
and simply does nothing, so these plain text checks pin the calls down. Run
from the repository root:

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


FACTORY = _find("trap_factory.h")
CYCLE = _find("cycle_trap.cpp")


@unittest.skipIf(FACTORY is None, "trap_factory.h not found")
class FactoryWiringTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        text = FACTORY.read_text()
        cls.text = text
        cls.path = text[text.index("case TrapType::PATH"):text.index("case TrapType::CHASE")]
        cls.cycle = text[text.index("case TrapType::CYCLE"):text.index("default:")]

    def test_path_traps_are_scaled_by_the_speed_multiplier(self):
        self.assertIn("trap_speed_multiplier()", self.path)
        self.assertIn("trap_timing::scaled_frames(trap_data.path_waits", self.path)
        # the scaled count, not the raw one, reaches the constructor
        self.assertEqual(self.path.count("trap_data.path_waits"), 1)

    def test_cycle_traps_are_built_from_the_scaled_round_and_phase(self):
        self.assertIn("trap_timing::scaled_cycle(", self.cycle)
        self.assertIn("trap_data.cycle_extend_frames", self.cycle)
        self.assertIn("trap_data.cycle_wait_frames", self.cycle)
        self.assertIn("trap_timing::scaled_frames(", self.cycle)
        self.assertIn("trap_data.cycle_phase_frames", self.cycle)
        self.assertIn("make_unique<CycleTrap>", self.cycle)

    def test_a_cycle_trigger_is_optional(self):
        self.assertIn("trap_data.trigger_name == nullptr", self.cycle)

    def test_a_round_of_zero_frames_is_rejected_at_load_time(self):
        self.assertIn("cycle_length(timing) > 0", self.cycle)


@unittest.skipIf(CYCLE is None, "cycle_trap.cpp not found")
class CycleTrapWiringTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        text = CYCLE.read_text()
        cls.update = _body(text, r"void\s+CycleTrap::update\s*\(\s*\)")
        cls.reset = _body(text, r"void\s+CycleTrap::reset\s*\(\s*\)")
        cls.apply = _body(text, r"void\s+CycleTrap::_apply\s*\(\s*int\s+\w+\s*\)")

    def test_update_follows_the_timeline_and_advances_the_frame(self):
        self.assertIn("BaseTrap::update()", self.update)
        self.assertIn("trap_timing::cycle_progress(_timing, _frame)", self.update)
        self.assertIn("trap_timing::next_cycle_frame(_timing, _frame)", self.update)

    def test_a_frozen_trap_does_not_advance(self):
        gate = self.update.index("_trigger->is_triggered()")
        self.assertLess(gate, self.update.index("_apply("))
        self.assertLess(gate, self.update.index("next_cycle_frame"))

    def test_reset_returns_to_the_start_frame_of_the_round(self):
        self.assertIn("_frame = _start_frame", self.reset)
        self.assertIn("_apply(", self.reset)

    def test_direct_position_writes_sync_the_shape_and_the_sprite(self):
        self.assertIn("shape_pos.move(pos.x, pos.y)", self.apply)
        self.assertIn("sprite->pos.move(pos.x, pos.y)", self.apply)


if __name__ == "__main__":
    unittest.main()
