#!/usr/bin/env python3
"""Checks of the whole-game inventory's pieces on small synthetic programs."""
import tempfile
import unittest
from pathlib import Path

import game_inventory as gi


class Program:
    """An export stand-in: {address: (flow, target, assembly)}, each
    instruction falling through to the next word unless it ends a path."""

    def __init__(self, code):
        self.flow, self.target, self.next, self.assembly = {}, {}, {}, {}
        for address, (flow, target, text) in code.items():
            self.flow[address] = flow
            if target is not None:
                self.target[address] = target
            if flow not in ("TERMINATOR", "UNCONDITIONAL_JUMP", "COMPUTED_JUMP"):
                self.next[address] = address + 4
            self.assembly[address] = text


def plain(text="nop"):
    return ("FALL_THROUGH", None, text)


class Body(unittest.TestCase):
    def test_flow(self):
        code = {
            0x100: plain(),
            0x104: ("CONDITIONAL_JUMP", 0x110, "beq 0x110"),
            0x108: ("UNCONDITIONAL_CALL", 0x200, "bl 0x200"),
            0x10c: ("TERMINATOR", None, "blr"),
            0x110: ("UNCONDITIONAL_JUMP", 0x300, "b 0x300"),     # tail call
            0x200: ("TERMINATOR", None, "blr"),
            0x300: ("TERMINATOR", None, "blr"),
        }
        program = Program(code)
        seen, calls, tails, unresolved, missing = gi.body(program, 0x100, {0x100, 0x200, 0x300}, {})
        self.assertEqual(seen, {0x100, 0x104, 0x108, 0x10c, 0x110})
        self.assertEqual(calls, [(0x108, 0x200)])
        self.assertEqual(tails, [(0x110, 0x300)])
        self.assertEqual((unresolved, missing), ([], []))

    def test_branch_into_body_is_not_a_tail_call(self):
        code = {0x100: ("UNCONDITIONAL_JUMP", 0x108, "b 0x108"), 0x104: plain(),
                0x108: ("TERMINATOR", None, "blr")}
        seen, _, tails, _, _ = gi.body(Program(code), 0x100, {0x100}, {})
        self.assertEqual(seen, {0x100, 0x108})
        self.assertEqual(tails, [])

    def test_pc_read_and_tables(self):
        code = {
            0x100: ("UNCONDITIONAL_JUMP", 0x104, "bl 0x104"),   # reads the PC
            0x104: ("COMPUTED_JUMP", None, "bctr"),
            0x108: ("TERMINATOR", None, "blr"),
            0x10c: ("TERMINATOR", None, "blr"),
            0x110: ("COMPUTED_JUMP", None, "bctr"),
        }
        program = Program(code)
        seen, _, _, unresolved, _ = gi.body(program, 0x100, {0x100}, {0x104: [0x108, None, 0x10c]})
        self.assertEqual(seen, {0x100, 0x104, 0x108, 0x10c})
        self.assertEqual(unresolved, [])
        seen, _, _, unresolved, missing = gi.body(program, 0x110, {0x110}, {})
        self.assertEqual((seen, unresolved, missing), ({0x110}, [0x110], []))

    def test_flow_out_of_the_export(self):
        code = {0x100: ("CONDITIONAL_JUMP", 0x400, "beq 0x400"), 0x104: ("TERMINATOR", None, "blr")}
        _, _, _, _, missing = gi.body(Program(code), 0x100, {0x100}, {})
        self.assertEqual(missing, [0x400])


class CaseRoots(unittest.TestCase):
    def routine(self, instructions, reasons):
        return {"instructions": set(instructions), "calls": [], "tails": [], "unresolved": [], "missing": [],
                "reasons": reasons}

    def test_folds_only_inside_a_routine_with_bctr(self):
        flow = {0x100: "COMPUTED_JUMP", 0x104: "FALL_THROUGH", 0x120: "TERMINATOR", 0x130: "TERMINATOR",
                0x200: "TERMINATOR", 0x210: "TERMINATOR", 0x300: "TERMINATOR"}
        routines = {
            0x100: self.routine({0x100, 0x104, 0x130}, ["call"]),
            0x120: self.routine({0x120}, ["root"]),       # inside, host has bctr: a case
            0x200: self.routine({0x200, 0x210}, ["call"]),
            0x204: self.routine({0x204}, ["root"]),       # inside, host has no bctr: a routine
            0x300: self.routine({0x300}, ["root"]),       # after its host: a routine
        }
        flow[0x204] = "TERMINATOR"
        folded = gi.fold_case_roots(routines, flow)
        self.assertEqual(folded, {0x120: 0x100})
        self.assertNotIn(0x120, routines)
        self.assertIn(0x120, routines[0x100]["instructions"])
        self.assertEqual(routines[0x100]["case_roots"], [0x120])
        self.assertIn(0x204, routines)
        self.assertIn(0x300, routines)


class Attribution(unittest.TestCase):
    def test_methods(self):
        routines = [0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80]
        evidence = {0x10: {"A.CPP"}, 0x30: {"A.CPP"}, 0x50: {"B.CPP"}, 0x80: {"C.CPP", "D.CPP"}}
        audited = [(0x70, 0x78, "E.CPP")]
        neighbours = {0x40: {0x50, 0x10}, 0x60: {0x10}}
        result = gi.attribute(routines, evidence, audited, neighbours)
        self.assertEqual(result[0x10], ("A.CPP", "evidence"))
        self.assertEqual(result[0x20], ("A.CPP", "filled"))
        # Between A and B; its named neighbours are in both: no vote.
        self.assertEqual(result[0x40], (None, "boundary"))
        # Between B and the audited E; its one neighbour is in A, neither side.
        self.assertEqual(result[0x60], (None, "boundary"))
        self.assertEqual(result[0x70], ("E.CPP", "audited"))
        self.assertEqual(result[0x80], ("C.CPP+D.CPP", "conflict"))

    def test_graph_vote_propagates(self):
        routines = [0x10, 0x20, 0x30, 0x40]
        evidence = {0x10: {"A.CPP"}, 0x40: {"B.CPP"}}
        neighbours = {0x20: {0x10}, 0x30: {0x20}}
        result = gi.attribute(routines, evidence, [], neighbours)
        self.assertEqual(result[0x20], ("A.CPP", "graph"))
        self.assertEqual(result[0x30], ("A.CPP", "graph"))

    def test_audited_beats_evidence_and_calibration_ignores_it(self):
        routines = [0x10, 0x20, 0x30]
        evidence = {0x10: {"A.CPP"}, 0x20: {"X.CPP"}, 0x30: {"A.CPP"}}
        audited = [(0x10, 0x40, "A.CPP")]
        self.assertEqual(gi.attribute(routines, evidence, audited)[0x20], ("A.CPP", "audited"))
        self.assertEqual(gi.calibration(routines, evidence, audited),
                         {"evidence agree": 2, "evidence differ": 1})


class Layers(unittest.TestCase):
    def test_layers_and_cycles(self):
        graph = {1: set(), 2: {1}, 3: {4}, 4: {3, 2}, 5: {3}}
        layer, components = gi.layers(graph)
        self.assertEqual(layer, {1: 0, 2: 1, 3: 2, 4: 2, 5: 3})
        self.assertIn([3, 4], components)
        # Callees come first.
        self.assertLess(components.index([1]), components.index([3, 4]))
        self.assertEqual(gi.closure(graph, {5}), {5, 3, 4, 2, 1})
        self.assertEqual(gi.closure(graph, {9}), set())


class AuditedModules(unittest.TestCase):
    def test_parse(self):
        with tempfile.TemporaryDirectory() as directory:
            tools = Path(directory)
            (tools / "cash_audit.py").write_text('"""CASH.CPP entries."""\nMODULE = (0x100, 0x200)\n')
            (tools / "gaucho_audit.py").write_text('"""Brazilian championship."""\nMODULE = (0x200, 0x300)\n')
            (tools / "tactics_audit.py").write_text('MODULES = {"TACTICS": (0x300, 0x380), "FORMATION": (0x380, 0x400)}\n')
            (tools / "main_program_audit.py").write_text('"""Main."""\nMODULE = (0x400, 0x500)\n')
            found = gi.audited_modules(tools, ["gaucho.cpp", "CASH.CPP"])
            self.assertEqual(found, [(0x100, 0x200, "CASH.CPP"), (0x200, 0x300, "gaucho.cpp"),
                                     (0x300, 0x380, "TACTICS.CPP"), (0x380, 0x400, "FORMATION.CPP"),
                                     (0x400, 0x500, "MAIN.CPP")])
            (tools / "other_audit.py").write_text('"""OTHER.CPP."""\nMODULE = (0x1f0, 0x210)\n')
            with self.assertRaisesRegex(ValueError, "overlap"):
                gi.audited_modules(tools)


if __name__ == "__main__":
    unittest.main()
