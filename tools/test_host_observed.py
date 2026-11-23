"""The guard against routing a routine that holds an instruction the host observes or patches (tools/host_observed.py).

For the four modules that own routines the host's HD drawing and adaptive layout watch (FONT.CPP, graph_carbon.cpp, the font engine, the
assertion helpers): each routed entry holds only host points its module lists (HANDLED_POINTS) and calls only host entries its module
lists (HANDLED_CALLS); flipping any draft to a routed entry without the listing is refused, as is each listing removed.
"""
from pathlib import Path
import sqlite3
import tempfile
import unittest
from unittest import mock

import assertion_audit
import font_cpp_audit
import font_engine_audit
import graph_carbon_audit
import host_observed

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / '.local/current/export'
MODULES = (("FONT_CPP", font_cpp_audit), ("GRAPH_CARBON", graph_carbon_audit), ("FONT_ENGINE", font_engine_audit),
           ("ASSERTION", assertion_audit))


@unittest.skipUnless((EXPORT / 'instructions.sqlite').exists(), 'Private Mac export unavailable')
class HostObservedTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from compile_recomp import SPECIAL_LOWERINGS
        cls.special = SPECIAL_LOWERINGS
        cls.db = sqlite3.connect((EXPORT / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True)

    @classmethod
    def tearDownClass(cls):
        cls.db.close()

    def test_the_points_include_every_set_the_host_runs_code_at(self):
        points = host_observed.host_points(self.special)
        self.assertEqual(points[0x1074fb58], ('drawing', 'watched'))
        self.assertEqual(points[0x1076e6c0], ('adaptive', 'special'))
        self.assertEqual(points[0x10719120], ('adaptive', 'special'))
        self.assertEqual(points[0x1076e7a4], ('profile',))
        self.assertEqual(points[0x10773eb8], ('capture', 'special') if 0x10773eb8 in self.special else ('capture',))
        self.assertIn('special', points[0x10772768])
        # A rewrite's own `pc ==` tests are not host code: the font engine's setjmp landing is no host point.
        self.assertNotIn(0x10756490, points)
        self.assertEqual(host_observed.points_inside(0x1076e5f0, 0x1076e5f4), [(0x1076e5f0, ('profile',))])

    def test_every_routed_entry_holds_and_calls_only_what_its_module_lists(self):
        for name, module in MODULES:
            for entry, end, _ in module.rewritten():
                # The full set (SPECIAL_LOWERINGS included), which the audits cannot import.
                host_observed.require_handled(name, entry, end, module.HANDLED_POINTS, self.special)
                host_observed.require_calls_handled(name, self.db, entry, end, module.HANDLED_CALLS, self.special)
            # A listing names only entries of the module that hold the point or make the call.
            routines = dict(module.ROUTINES)
            for entry, addresses in module.HANDLED_POINTS.items():
                inside = {a for a, _ in host_observed.points_inside(entry, routines[entry], self.special)}
                self.assertEqual(set(addresses) - inside, set(), hex(entry))
            for entry, callees in module.HANDLED_CALLS.items():
                made = set(host_observed.calls_into_points(self.db, entry, routines[entry], self.special).values())
                self.assertEqual(set(callees) - made, set(), hex(entry))

    def test_a_draft_flipped_to_routed_is_refused_unless_listed(self):
        # Every owned entry that holds a host point, as the drafts of 2026-11-23 did: listed only when the module's C++ passes the points.
        for name, module in MODULES:
            with tempfile.TemporaryDirectory() as directory:
                path = Path(directory) / 'routines.inc'
                for entry, end, _ in module.rewritten(drafts=True):
                    holding = host_observed.points_inside(entry, end, self.special)
                    if not holding:
                        continue
                    # Only this entry flipped to ROUTINE (the others stay drafts), with the module's listing for it taken away.
                    lines = ''.join(f'{name}_{"ROUTINE" if e == entry else "DRAFT"}(0x{e:x}, 0x{x:x}, R3, "{d}", name)\n'
                                    for e, x, d in module.rewritten(drafts=True))
                    path.write_text(lines)
                    unlisted = {e: points for e, points in module.HANDLED_POINTS.items() if e != entry}
                    with mock.patch.object(module, 'HANDLED_POINTS', unlisted):
                        with self.assertRaisesRegex(ValueError, f'{entry:x} holds host-observed instructions'):
                            module.rewritten(path)
                        # Drafts are never refused: the game runs the original for them.
                        path.write_text(lines.replace('ROUTINE', 'DRAFT'))
                        module.rewritten(path)
                    path.write_text(lines)
                    module.rewritten(path)  # with the module's own listing
                    self.assertIn(entry, module.HANDLED_POINTS, f'{name} {entry:x}')

    def test_every_listed_point_and_call_is_needed_and_removing_one_is_refused(self):
        for name, module in MODULES:
            for entry, addresses in module.HANDLED_POINTS.items():
                if entry not in {e for e, _, _ in module.rewritten()}:
                    continue
                for address in addresses:
                    listed = {**module.HANDLED_POINTS, entry: frozenset(addresses - {address})}
                    with mock.patch.object(module, 'HANDLED_POINTS', listed):
                        with self.assertRaisesRegex(ValueError, f'{address:x}'):
                            module.rewritten()
            for entry, callees in module.HANDLED_CALLS.items():
                if entry not in {e for e, _, _ in module.rewritten()}:
                    continue
                for callee in callees:
                    listed = {**module.HANDLED_CALLS, entry: frozenset(callees - {callee})}
                    with mock.patch.object(module, 'HANDLED_CALLS', listed):
                        with self.assertRaisesRegex(ValueError, f'{callee:x}'):
                            getattr(module, f'require_{name.lower()}_index')(self.db)

    def test_a_routed_caller_of_a_host_entry_is_refused_unless_listed(self):
        # FONT.CPP's string width calls the glyph cache (an entry carrying a drawing point): a typed call would skip the page's point.
        font = font_cpp_audit
        entry, end = 0x107500f0, dict(font.ROUTINES)[0x107500f0]
        self.assertEqual(set(host_observed.calls_into_points(self.db, entry, end).values()), {0x10750580})
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'routines.inc'
            lines = ''.join(f'FONT_CPP_{"ROUTINE" if e == 0x107500f0 else "DRAFT"}(0x{e:x}, 0x{x:x}, R3, "{d}", name)\n'
                            for e, x, d in font.rewritten(drafts=True))
            path.write_text(lines)
            font.rewritten(path)  # the body holds no point of its own
            # The string width calls the cache typed: without the listing it is refused, with it accepted.
            with mock.patch.object(font, 'HANDLED_CALLS', {}):
                with self.assertRaisesRegex(ValueError, 'HANDLED_CALLS'):
                    font.require_font_cpp_index(self.db, path, complete=False)
            font.require_font_cpp_index(self.db, path, complete=False)
            self.assertIn(0x10750580, font.HANDLED_CALLS[entry])

    def test_a_handled_entry_point_stays_out_of_the_in_place_owner_tables(self):
        # An entry's own point is the page's: it runs the host's code before the owner hook, so no caller may enter the owner in place.
        from compile_recomp import direct_owner_entries, owner_entries_in_place
        found = 0
        for name, module in MODULES:
            for entry, addresses in module.HANDLED_POINTS.items():
                if entry in addresses:
                    found += 1
                    self.assertNotIn(entry, direct_owner_entries(), f'{name} {entry:x}')
                    self.assertNotIn(entry, owner_entries_in_place(frozenset()), f'{name} {entry:x}')
        self.assertGreaterEqual(found, 13)

    def test_the_scan_converter_constructor_is_reached_through_its_page_only(self):
        # 0x107620d0 carries a drawing point at its entry (the host's outline capture reads r4-r9 there): the engine's one caller, the glyph
        # renderer, calls it with c.call, so the page runs the observation; only the owner hook (after the page's point) names the typed function.
        import re
        native = ROOT / 'native'
        typed = sorted(p.name for p in native.glob('*') if p.suffix in ('.cpp', '.hpp', '.inc') and 'checks' not in p.name
                       and re.search(r'\bnew_scan_converter\s*\(', p.read_text(errors='replace')))
        self.assertEqual(typed, ['font_engine.hpp', 'font_engine_routines_t2ksc.inc', 'font_engine_t2ksc.cpp'])
        core = (native / 'font_engine_t2k_core.cpp').read_text()
        self.assertEqual(len(re.findall(r'c\.call\(scan_new,', core)), 1)
        self.assertEqual(font_engine_audit.HANDLED_CALLS, {0x1075fa10: frozenset({0x107620d0})})
        self.assertEqual(font_engine_audit.HANDLED_POINTS, {0x107620d0: frozenset({0x107620d0})})

    def test_the_guard_function_alone(self):
        with self.assertRaisesRegex(ValueError, '1076e6c0 adaptive'):
            host_observed.require_handled('m', 0x1076e5f0, 0x1076e7c0, {0x1076e5f0: frozenset({0x1076e5f0})})
        host_observed.require_handled('m', 0x1076e5f0, 0x1076e5f4, {0x1076e5f0: frozenset({0x1076e5f0})})
        with self.assertRaisesRegex(ValueError, '1076e5f0 profile'):
            host_observed.require_handled('m', 0x1076e5f0, 0x1076e5f4, {})


if __name__ == '__main__':
    unittest.main()
