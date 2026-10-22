"""Check that the zero-divisor quotient proof accepts only the modulo idiom."""
import unittest

from compile_recomp import zero_divisor_quotient_unused

R0, R3, R4, R5 = (["register", str(4 * n), 4] for n in (0, 3, 4, 5))


def graph():
    # divw r3,r5,r4; mullw r0,r3,r4; then both branches overwrite r3.
    return {
        0: {"flow": "FALL_THROUGH", "fallthrough": 4, "ops": [["INT_SDIV", R3, R5, R4]]},
        4: {"flow": "CONDITIONAL_JUMP", "flows": [16], "fallthrough": 8, "ops": [["INT_MULT", R0, R3, R4]]},
        8: {"flow": "UNCONDITIONAL_JUMP", "flows": [20], "ops": []},
        16: {"flow": "FALL_THROUGH", "fallthrough": 20, "ops": [["COPY", R3, ["const", "7", 4]]]},
        20: {"flow": "FALL_THROUGH", "fallthrough": 24, "ops": [["COPY", R3, ["const", "8", 4]]]},
    }


def proven(records):
    return zero_divisor_quotient_unused(records.get, 0)


class QuotientAuditTests(unittest.TestCase):
    def test_modulo_idiom_is_proven(self):
        self.assertTrue(proven(graph()))

    def test_multiply_operands_in_either_order(self):
        records = graph()
        records[4]["ops"] = [["INT_MULT", R0, R4, R3]]
        self.assertTrue(proven(records))

    def test_other_reads_rejected(self):
        for address in (8, 16):
            for offset, size in ((12, 4), (14, 2), (15, 1), (8, 8)):
                with self.subTest(address=address, offset=offset, size=size):
                    records = graph()
                    records[address]["ops"].insert(0, ["COPY", ["unique", "0", size], ["register", str(offset), size]])
                    self.assertFalse(proven(records))

    def test_multiply_by_another_register_rejected(self):
        records = graph()
        records[4]["ops"] = [["INT_MULT", R0, R3, R5]]
        self.assertFalse(proven(records))

    def test_changed_divisor_rejected(self):
        records = graph()
        records[0]["fallthrough"] = 2
        records[2] = {"flow": "FALL_THROUGH", "fallthrough": 4, "ops": [["COPY", R4, ["const", "1", 4]]]}
        self.assertFalse(proven(records))

    def test_divisor_changed_on_one_merging_path_rejected(self):
        # The multiply is reached first with the divisor intact, then again
        # after `li r4,1`, where it would expose the quotient.
        records = {
            0: {"flow": "FALL_THROUGH", "fallthrough": 4, "ops": [["INT_SDIV", R3, R5, R4]]},
            4: {"flow": "CONDITIONAL_JUMP", "flows": [12], "fallthrough": 8, "ops": []},
            8: {"flow": "FALL_THROUGH", "fallthrough": 12, "ops": [["COPY", R4, ["const", "1", 4]]]},
            12: {"flow": "FALL_THROUGH", "fallthrough": 16, "ops": [["INT_MULT", R4, R3, R4]]},
            16: {"flow": "FALL_THROUGH", "fallthrough": 20, "ops": [["COPY", R3, ["const", "0", 4]]]},
        }
        self.assertFalse(proven(records))
        del records[8]["ops"][0]
        self.assertTrue(proven(records))

    def test_userops_and_blocked_addresses_rejected(self):
        records = graph()
        records[8]["ops"].append(["CALLOTHER", None, ["const", "1", 4]])
        self.assertFalse(proven(records))
        self.assertFalse(zero_divisor_quotient_unused(graph().get, 0, frozenset({16})))
        self.assertFalse(zero_divisor_quotient_unused(graph().get, 0, frozenset({0})))

    def test_read_modify_write_is_not_a_kill(self):
        records = graph()
        records[16]["ops"] = [["INT_ADD", R3, R3, ["const", "1", 4]]]
        self.assertFalse(proven(records))

    def test_partial_writes_rejected(self):
        records = graph()
        records[16]["ops"] = [["COPY", ["register", "14", 2], ["const", "0", 2]]]
        self.assertFalse(proven(records))

    def test_calls_indirect_flow_and_returns_rejected(self):
        for flow in ("UNCONDITIONAL_CALL", "COMPUTED_JUMP", "TERMINATOR"):
            records = graph()
            records[8]["flow"] = flow
            with self.subTest(flow=flow):
                self.assertFalse(proven(records))

    def test_missing_target_and_cycles_rejected(self):
        for targets in ([8], [100], []):
            records = graph()
            records[8]["flows"] = targets
            with self.subTest(targets=targets):
                self.assertFalse(proven(records))

    def test_missing_fallthrough_rejected(self):
        records = graph()
        del records[4]["fallthrough"]
        self.assertFalse(proven(records))

    def test_record_form_and_divisor_quotient_rejected(self):
        records = graph()
        records[0]["ops"].append(["INT_SLESS", ["register", "2304", 1], R3, ["const", "0", 4]])
        self.assertFalse(proven(records))
        records = graph()
        records[0]["ops"] = [["INT_SDIV", R4, R5, R4]]
        self.assertFalse(proven(records))


if __name__ == "__main__":
    unittest.main()
