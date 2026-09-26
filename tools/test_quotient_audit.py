"""Check that the match quotient liveness proof rejects unsafe continuations."""
import unittest

from compile_recomp import require_dead_register


def graph():
    # Branches either kill r3 directly or merge at a full overwrite.
    return {
        4: {"flow": "CONDITIONAL_JUMP", "flows": [16], "fallthrough": 8, "ops": []},
        8: {"flow": "UNCONDITIONAL_JUMP", "flows": [20], "ops": []},
        16: {"flow": "FALL_THROUGH", "fallthrough": 20, "ops": [
            ["COPY", ["register", "12", 4], ["const", "7", 4]]]},
        20: {"flow": "FALL_THROUGH", "fallthrough": 24, "ops": [
            ["COPY", ["register", "12", 4], ["const", "8", 4]]]},
    }


class QuotientAuditTests(unittest.TestCase):
    def test_every_branch_overwrites(self):
        require_dead_register(graph(), 4, 12, 4)

    def test_read_on_either_branch_rejected(self):
        for address in (8, 16):
            for offset, size in ((12, 4), (14, 2), (15, 1), (8, 8)):
                with self.subTest(address=address, offset=offset, size=size):
                    records = graph()
                    records[address]["ops"].insert(0, ["COPY", ["unique", "0", size], ["register", str(offset), size]])
                    with self.assertRaises(ValueError):
                        require_dead_register(records, 4, 12, 4)

    def test_read_modify_write_is_not_a_kill(self):
        records = graph()
        records[16]["ops"] = [["INT_ADD", ["register", "12", 4], ["register", "12", 4], ["const", "1", 4]]]
        with self.assertRaises(ValueError):
            require_dead_register(records, 4, 12, 4)

    def test_partial_writes_rejected(self):
        records = graph()
        records[16]["ops"] = [["COPY", ["register", "14", 2], ["const", "0", 2]]]
        with self.assertRaises(ValueError):
            require_dead_register(records, 4, 12, 4)

    def test_calls_indirect_flow_and_returns_rejected(self):
        for flow in ("UNCONDITIONAL_CALL", "COMPUTED_JUMP", "TERMINATOR"):
            records = graph()
            records[8]["flow"] = flow
            with self.subTest(flow=flow), self.assertRaises(ValueError):
                require_dead_register(records, 4, 12, 4)

    def test_missing_target_and_cycles_rejected(self):
        for targets in ([8], [100], []):
            records = graph()
            records[8]["flows"] = targets
            with self.subTest(targets=targets), self.assertRaises(ValueError):
                require_dead_register(records, 4, 12, 4)

    def test_missing_fallthrough_rejected(self):
        records = graph()
        del records[4]["fallthrough"]
        with self.assertRaises(ValueError):
            require_dead_register(records, 4, 12, 4)


if __name__ == "__main__":
    unittest.main()
