#!/usr/bin/env python3
"""Discriminating generator checks for original vector-context operations."""
from copy import deepcopy
import unittest
from compile_recomp import operation, read, write
from vector_context_lowering import lower_vector_intrinsic


def register(offset, size=4):
    return ['register', str(offset), size]


def constant(value, size=4):
    return ['const', str(value), size]


USEROPS = {'7': 'loadVectorElementWordIndexed', '19': 'storeVectorElementWordIndexed',
           '31': 'moveFromVectorStatusAndControlRegister', '47': 'moveToVectorStatusAndControlRegister'}
V31 = register(0x43f0, 16)


class VectorContextLowering(unittest.TestCase):
    def lower(self, op):
        return operation(op, 0x1234, 0, 1, {}, 1, userops=USEROPS)

    def test_full_width_load_store_and_copy(self):
        self.assertEqual(self.lower(['LOAD', V31, constant(417, 8), register(12)]),
                         's.write_vector_register(17392, s.memory.read_vector(s.r<4>(12)));')
        self.assertEqual(self.lower(['STORE', None, constant(417, 8), register(12), V31]),
                         's.memory.write_vector(s.r<4>(12), s.read_vector_register(17392));')
        self.assertEqual(self.lower(['COPY', register(0x4200, 16), V31]),
                         's.write_vector_register(16896, s.read_vector_register(17392));')
        for opcode in ('INT_ADD', 'INT_ZEXT', 'SUBPIECE'):
            with self.subTest(opcode=opcode), self.assertRaisesRegex(ValueError, 'Unsupported vector operation'):
                self.lower([opcode, V31, V31, register(0x4200, 16)])

    def test_vector_spaces_do_not_truncate_to_scalars(self):
        self.assertEqual(read(['unique', '281344', 16]), 's.read_vector_temporary(281344)')
        self.assertEqual(write(['ram', '123', 16], 'value'), 's.memory.write_vector(123, value);')
        with self.assertRaisesRegex(ValueError, 'Unsupported vector value space'):
            read(constant(1, 16))

    def test_encoded_zero_base_ignores_live_r0(self):
        code = self.lower(['CALLOTHER', V31, constant(7), register(0), register(20)])
        self.assertIn('load_vector_word(s, std::uint32_t(UINT64_C(0)), std::uint32_t(s.r<4>(20)))', code)
        self.assertNotIn('s.r<4>(0)', code)
        code = self.lower(['CALLOTHER', V31, constant(7), register(8), register(20)])
        self.assertIn('std::uint32_t(s.r<4>(8))', code)

    def test_word_selection_and_status_intrinsics(self):
        code = self.lower(['CALLOTHER', register(12), constant(19), V31, constant(0), register(20)])
        self.assertIn('vector_word_for_address(s.read_vector_register(17392)', code)
        self.assertIn('s.wr<4>(12,', code)
        self.assertEqual(self.lower(['CALLOTHER', V31, constant(31)]),
                         's.write_vector_register(17392, vector_status_register(s));')
        self.assertEqual(self.lower(['CALLOTHER', None, constant(47), V31]),
                         'move_to_vector_status(s, s.read_vector_register(17392));')

    def test_userop_names_are_authoritative(self):
        op = ['CALLOTHER', V31, constant(7), register(0), register(20)]
        moved = deepcopy(op)
        moved[2] = constant(211)
        self.assertEqual(lower_vector_intrinsic(op, USEROPS, read, write),
                         lower_vector_intrinsic(moved, {'211': USEROPS['7']}, read, write))
        self.assertIn('Unimplemented original intrinsic', operation(op, 0x1234, 0, 1, {}, 1))
        self.assertIn('Unimplemented original intrinsic', operation(op, 0x1234, 0, 1, {}, 1,
                      userops={'7': 'unrelatedOperation'}))

    def test_changed_intrinsic_shapes_fail_closed(self):
        cases = [['CALLOTHER', V31, constant(7), register(0), register(20)],
                 ['CALLOTHER', register(12), constant(19), V31, constant(0), register(20)],
                 ['CALLOTHER', V31, constant(31)], ['CALLOTHER', None, constant(47), V31]]
        for op in cases:
            mutation = deepcopy(op)
            mutation.append(register(100))
            with self.subTest(op=op), self.assertRaisesRegex(ValueError, 'Malformed AltiVec'):
                self.lower(mutation)
        for bad in (register(0x4201, 16), register(0x4400, 16), register(0x43f0, 8)):
            with self.subTest(bad=bad), self.assertRaisesRegex(ValueError, 'Malformed AltiVec'):
                self.lower(['CALLOTHER', bad, constant(31)])


if __name__ == '__main__':
    unittest.main()
