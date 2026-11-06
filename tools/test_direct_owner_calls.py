"""Production pages call C++ owners in place and continue at the return address."""
import tempfile
import unittest
from unittest import mock
from pathlib import Path
from compile_recomp import (DirectOwnerCalls, direct_owner_entries, generate_page, owners_counting_declines,
                            plain_direct_call, render_direct_owners, direct_owner_table, require_watched_in_place,
                            CPP_IN_PLACE_WATCHED)

PAGE = 0x10001


def call(address, callee, ops=None):
    return {'address': address, 'size': 4, 'assembly': f'bl 0x{callee:x}', 'flow': 'UNCONDITIONAL_CALL',
            'flows': [callee], 'fallthrough': address + 4,
            'ops': ops or [['COPY', ['register', '2440', 4], ['register', '8', 4]],
                           ['COPY', ['register', '4128', 4], ['const', str(address + 4), 4]],
                           ['CALL', None, ['ram', str(callee), 4]]]}


def ret(address):
    return {'address': address, 'size': 4, 'assembly': 'blr', 'flow': 'TERMINATOR', 'flows': [],
            'ops': [['RETURN', None, ['register', '4128', 4]]]}


class InPlaceOwnerCalls(unittest.TestCase):
    def setUp(self):
        self.callee, self.family = min(direct_owner_entries().items())

    def generate(self, instructions, calls):
        return generate_page(PAGE, instructions, {i['address'] for i in instructions}, block_accounting=True,
                             direct_owner_calls=calls)

    def test_turbo_page_owners_keep_their_counter_boundary(self):
        from turbo_pages import TURBO_PAGES
        self.assertTrue(all(entry >> 12 not in TURBO_PAGES for entry in direct_owner_entries()))

    def test_production_continues_at_the_return_address_unless_the_dispatcher_stops_there(self):
        calls = DirectOwnerCalls()
        code = self.generate([call(0x10001000, self.callee), ret(0x10001004)], calls)
        self.assertIn(f'if (try_modern_{self.family}(s, 0x{self.callee:x}) && continues_after_owner(s, 0x10001004)) '
                      'goto i_10001004;\nreturn true;', code)
        self.assertIn(f'#include "{self.family}_adapter.hpp"', code)
        self.assertIn('#include "direct_owner_call.hpp"', code)
        self.assertEqual(calls.called, {self.callee: self.family})

    def test_a_return_address_off_the_page_goes_back_through_the_dispatcher(self):
        calls = DirectOwnerCalls()
        code = self.generate([call(0x10001ffc, self.callee)], calls)
        self.assertIn(f'try_modern_{self.family}(s, 0x{self.callee:x});\nreturn true;', code)
        self.assertNotIn('continues_after_owner', code)

    def test_references_and_unusual_call_shapes_keep_the_dispatcher(self):
        code = self.generate([call(0x10001000, self.callee), ret(0x10001004)], None)
        self.assertNotIn(f'try_modern_{self.family}', code)
        odd = call(0x10001000, self.callee, [['COPY', ['register', '4128', 4], ['const', str(0x10001004), 4]],
                                             ['CALL', None, ['ram', str(self.callee), 4]]])
        self.assertIsNone(plain_direct_call(odd))
        calls = DirectOwnerCalls()
        self.assertNotIn(f'try_modern_{self.family}', self.generate([odd, ret(0x10001004)], calls))
        self.assertEqual(calls.called, {})

    def test_every_callee_must_keep_its_single_standard_hook(self):
        calls = DirectOwnerCalls()
        calls.called[self.callee] = self.family
        with self.assertRaises(ValueError):
            calls.require_standard()
        calls.entry_hooks[self.callee] = [f'if (try_modern_{self.family}(s, 0x{self.callee:x})) return true;']
        calls.require_standard()
        calls.entry_hooks[self.callee].append('if (try_modern_other(s, 0x1)) return true;')
        with self.assertRaises(ValueError):
            calls.require_standard()

    def test_cpp_callers_get_only_entries_with_the_single_standard_hook(self):
        standard = lambda entry, family: [f'if (try_modern_{family}(s, 0x{entry:x})) return true;']
        plain = lambda page: ''.join(f'case 0x{entry:x}: goto i_{entry:x};\n' for entry in range(page << 12, (page + 1) << 12, 0x100))
        with mock.patch('compile_recomp.owners_counting_declines', return_value=frozenset({'counted'})):
            owners, charged = direct_owner_table({0x10002000: standard(0x10002000, 'alpha'),
                                                  0x10003000: standard(0x10003000, 'beta') + ['if (try_modern_x(s, 0x1)) return true;'],
                                                  0x10005000: standard(0x10006000, 'delta'),
                                                  0x10007000: [],
                                                  0x10008000: standard(0x10008000, 'counted')}, plain)
        self.assertEqual((owners, charged), ({0x10002000: 'alpha'}, []))
        table = render_direct_owners(owners)
        self.assertIn('case 0x10002000: return try_modern_alpha(s, 0x10002000);', table)
        self.assertIn('#include "alpha_adapter.hpp"', table)
        self.assertIn('default: return false;', table)

    def test_an_owner_entered_through_a_charged_case_stays_off_the_table(self):
        hooks = {self.callee: [f'if (try_modern_{self.family}(s, 0x{self.callee:x})) return true;']}
        plain = f'case 0x{self.callee:x}: goto i_{self.callee:x};'
        self.assertEqual(direct_owner_table(hooks, lambda page: plain), ({self.callee: self.family}, []))
        charged = f'case 0x{self.callee:x}: s.step(0x{self.callee:x}); s.charge(2); goto i_{self.callee:x};'
        self.assertEqual(direct_owner_table(hooks, lambda page: charged), ({}, [self.callee]))

    def test_the_watched_contract_queries_follow_their_rules(self):
        table = {entry: 'contract_manager' for entry in CPP_IN_PLACE_WATCHED}
        require_watched_in_place(table, table, frozenset())
        require_watched_in_place({}, {}, frozenset())  # a bounded run that generated neither
        first = min(CPP_IN_PLACE_WATCHED)
        with self.assertRaisesRegex(ValueError, 'lost a watched entry'):
            require_watched_in_place({}, {first: []}, frozenset())
        with self.assertRaisesRegex(ValueError, 'Host code compares the PC'):
            require_watched_in_place(table, table, frozenset({first}) | set())
        with mock.patch('compile_recomp.SPECIAL_LOWERINGS', {first: None}):
            import compile_recomp
            compile_recomp.owner_entries_in_place.cache_clear()
            try:
                with self.assertRaisesRegex(ValueError, 'break an in-place owner rule'):
                    require_watched_in_place(table, table, frozenset())
            finally:
                compile_recomp.owner_entries_in_place.cache_clear()

    def test_owners_that_count_before_declining_keep_the_dispatcher(self):
        with tempfile.TemporaryDirectory() as directory:
            Path(directory, 'hooks.cpp').write_text(
                'bool try_modern_counted(State& s, std::uint32_t entry) {\n'
                '    ++s.original_calls;\n    if (s.original) return false;\n    return true;\n}\n'
                'bool try_modern_plain(State& s, std::uint32_t entry) {\n'
                '    if (s.original) return false;\n    ++s.calls;\n    return true;\n}\n')
            self.assertEqual(owners_counting_declines(Path(directory)), {'counted'})
        self.assertIn('string_core', owners_counting_declines())
        self.assertNotIn('string_core', set(direct_owner_entries().values()))


if __name__ == '__main__':
    unittest.main()
