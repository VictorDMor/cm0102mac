"""Production pages call C++ owners in place and continue at the return address."""
import tempfile
import unittest
from pathlib import Path
from compile_recomp import DirectOwnerCalls, direct_owner_entries, generate_page, owners_counting_declines, plain_direct_call

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
