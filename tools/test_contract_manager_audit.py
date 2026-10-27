"""Pin the original manager census and every staged semantic body to it."""
from contextlib import closing
import json
from pathlib import Path
import sqlite3
import unittest

from contract_manager_audit import MODULE, ROUTINES, rewritten, require_contract_manager_index

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / ".local/current/export"


@unittest.skipUnless((EXPORT / "instructions.sqlite").exists(), "Private reference export unavailable")
class ContractManagerAuditTests(unittest.TestCase):
    def test_original_inventory_and_module_end(self):
        self.assertEqual(len(ROUTINES), 95)
        self.assertEqual((ROUTINES[0][0], ROUTINES[-1][1]), MODULE)
        self.assertTrue(all(a[1] == b[0] for a, b in zip(ROUTINES, ROUTINES[1:])))
        self.assertEqual([entry for entry, _ in ROUTINES[-3:]], [0x10093860, 0x10093950, 0x100939b0])

    def test_staged_entries_match_original_instructions(self):
        rewritten(drafts=True)  # Also rejects duplicate and non-original entries.
        with closing(sqlite3.connect(EXPORT / "instructions.sqlite")) as db:
            require_contract_manager_index(db)

    def test_production_owners_take_over_only_completed_entries(self):
        from compile_recomp import OWNER_REPLACEMENTS
        owners = dict(OWNER_REPLACEMENTS)
        for entry, _, _ in rewritten():
            self.assertEqual(owners[entry], "contract_manager_adapter")
        for entry, _, _ in rewritten(drafts=True):
            if entry not in {a for a, _, _ in rewritten()}:
                self.assertNotEqual(owners.get(entry), "contract_manager_adapter")

    def test_unindexed_interior_branch_is_part_of_original_body(self):
        # This original branch was absent from the Ghidra index, not padding
        # or a separate routine. Keep its destination inside the same body.
        program = json.loads((EXPORT / "program.json").read_text())
        address = 0x10080bf0
        segment = next(s for s in program["segments"]
                       if s.get("file") and s["address"] <= address < s["address"] + s["size"])
        data = (EXPORT / segment["file"]).read_bytes()
        offset = address - segment["address"]
        self.assertEqual(int.from_bytes(data[offset:offset + 4], "big"), 0x48000108)
        owner = next((entry, end) for entry, end in ROUTINES if entry <= address < end)
        self.assertEqual(owner, (0x10080bb0, 0x10080d30))
        self.assertTrue(owner[0] <= address + 0x108 < owner[1])
        # The preceding branch returns directly to the epilogue. The loop
        # enters at 0x10080bf4, so this unindexed word is unreachable from
        # the routine entry, not missing live reference behaviour.
        self.assertEqual(int.from_bytes(data[offset - 4:offset], "big"), 0x4800011c)
        with closing(sqlite3.connect(EXPORT / "instructions.sqlite")) as db:
            for (body,) in db.execute("SELECT body FROM instructions WHERE address>=? AND address<?", MODULE):
                record = json.loads(body)
                self.assertNotIn(address, record.get("flows", []))
                self.assertNotEqual(address, record.get("fallthrough"))


if __name__ == "__main__":
    unittest.main()
