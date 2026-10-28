"""Close EURO_CHAMP_QUAL source, flow, indirect-entry and production inventory."""
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3
import tempfile
import unittest
from unittest.mock import patch

import euro_champ_qual_audit as audit
from euro_champ_qual_audit import MODULE, ROUTINES, INCLUDES, rewritten, require_euro_champ_qual_index

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / ".local/current/export/instructions.sqlite"
TVS = {
    0x108ba75c: 0x105026a0, 0x108ba764: 0x104fefe0,
    0x108ba76c: 0x104fef80, 0x108ba774: 0x104fed60,
    0x108ba77c: 0x104fe980, 0x108ba784: 0x104fe5a0,
    0x108ba78c: 0x104fe1d0, 0x108ba794: 0x104fe040,
    0x108ba79c: 0x104fdfc0, 0x108ba7a4: 0x104fdae0,
    0x108ba7ac: 0x104fc770, 0x108ba7b4: 0x104fc5e0,
    0x108ba7bc: 0x104fc5d0, 0x108ba7c4: 0x104fc4b0,
    0x108ba7cc: 0x104fc3c0, 0x108ba7d4: 0x104fab30,
    0x108ba7dc: 0x104fab00, 0x108ba7e4: 0x104fa980,
    0x108ba7ec: 0x104fa3d0, 0x108ba7f4: 0x104fa2a0,
}
VTABLE = {
    0x08: 0x108ba7f4, 0x0c: 0x108ba79c, 0x10: 0x108ba7b4,
    0x1c: 0x108ba7a4, 0x28: 0x108ba78c, 0x2c: 0x108ba784,
    0x30: 0x108ba7ac, 0x34: 0x108ba7bc, 0x38: 0x108ba7dc,
    0x40: 0x108ba7c4, 0x44: 0x108ba7ec, 0x4c: 0x108ba7e4,
    0x50: 0x108ba7d4, 0x64: 0x108ba77c, 0x70: 0x108ba774,
    0x78: 0x108ba76c, 0x7c: 0x108ba764, 0x80: 0x108ba75c,
    0x94: 0x108ba7cc, 0x9c: 0x108ba794,
}
UNINDEXED_NOPS = {
    0x104fa2fc, 0x104fc634, 0x104fcd8c, 0x104fcf04, 0x104fd410,
    0x104fd578, 0x104fda58, 0x104fe124, 0x104fe15c, 0x104fe354,
    0x104fe3cc, 0x104fe6b4, 0x104fea4c, 0x104ff4d0,
}
# Bytewise scans also match parts of data words. None is a descriptor:
# six cross-word Code-segment values, one overlapping external jump-table
# words, and three unaligned pairs of uint16 RNG samples.
DATA_MATCHES = {
    0x108a47f7: 0x10500000, 0x108a64e3: 0x10500000,
    0x108a6f7b: 0x10500000, 0x108ac513: 0x10500000,
    0x108b1c6e: 0x10500000, 0x108b29cf: 0x10500000,
    0x108d4b42: 0x10501047, 0x108e6947: 0x104fd75b,
    0x108fa09e: 0x104fd87e, 0x108fbccd: 0x104fad32,
}


@unittest.skipUnless(INPUT.exists(), "Private export unavailable")
class EuroChampQualTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.program = json.loads((INPUT.parent / "program.json").read_text())
        cls.segments = [(s["address"], (INPUT.parent / s["file"]).read_bytes())
                        for s in cls.program["segments"] if "file" in s]
        with closing(sqlite3.connect(INPUT)) as db:
            cls.records = [json.loads(b) for b, in db.execute(
                "SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address", MODULE)]
        cls.indexed = {r["address"]: r for r in cls.records}

    def original_bytes(self, address, size):
        for start, data in self.segments:
            if start <= address and address + size <= start + len(data):
                return data[address - start:address - start + size]
        self.fail(f"Missing original bytes at {address:x}")

    def word(self, address):
        return int.from_bytes(self.original_bytes(address, 4), "big")

    def assert_production_inventory(self, routines, owners):
        self.assertEqual([(a, b) for a, b, _ in routines], list(ROUTINES))
        for entry, _ in ROUTINES:
            self.assertEqual([owner for at, owner in owners if at == entry], ["euro_champ_qual_adapter"])

    def test_closed_contiguous_original_inventory(self):
        self.assertEqual(MODULE, (0x104f9fa0, 0x105026c0))
        self.assertEqual(len(ROUTINES), 31)
        self.assertEqual(len(self.records), 8590)
        self.assertEqual((ROUTINES[0][0], ROUTINES[-1][1]), MODULE)
        self.assertTrue(all(a[1] == b[0] for a, b in zip(ROUTINES, ROUTINES[1:])))
        prologues = {r["address"] for r in self.records if r["assembly"] == "mfspr r0,LR"}
        self.assertEqual({a for a, _ in ROUTINES}, prologues | {0x104fc5d0, 0x105026a0})
        self.assertEqual(len(prologues), 29)

    def test_source_pool_and_neighbors(self):
        for slot, address, name in (
            (-0x45a8, 0x1083d130, b"EURO_CHAMP.CPP"),
            (-0x4590, 0x1083d3b0, b"EURO_CHAMP_QUAL.CPP"),
            (-0x4588, 0x1083d610, b"GOLDCUP.CPP"),
        ):
            self.assertEqual(self.word(0x108bba90 + slot), address)
            self.assertEqual(self.original_bytes(address, len(name) + 1), name + b"\0")
        with closing(sqlite3.connect(INPUT)) as db:
            at = lambda address: json.loads(db.execute(
                "SELECT body FROM instructions WHERE address=?", (address,)).fetchone()[0])["assembly"]
            self.assertEqual(at(0x104f9f90), "li r3,0x1")
            self.assertEqual(at(0x104f9f94), "blr")
            self.assertEqual(at(MODULE[0] + 0x18), "lwz r31,-0x4590(r2)")
            self.assertEqual(at(MODULE[1]), "mfspr r0,LR")
            self.assertEqual(at(MODULE[1] + 0x18), "lwz r31,-0x4588(r2)")
            pool_loads = [a for a, in db.execute(
                "SELECT address FROM instructions WHERE body LIKE ? ORDER BY address", ("%-0x4590(r2)%",))]
        self.assertEqual(len(pool_loads), 23)
        self.assertEqual((pool_loads[0], pool_loads[-1]), (0x104f9fb8, 0x105022c0))
        self.assertTrue(all(MODULE[0] <= a < MODULE[1] for a in pool_loads))

    def test_returns_and_all_unindexed_words(self):
        missing = set(range(*MODULE, 4)) - set(self.indexed)
        self.assertEqual({a for a in missing if self.word(a) == 0x60000000}, UNINDEXED_NOPS)
        for address in UNINDEXED_NOPS:
            previous = self.indexed[address - 4]
            self.assertEqual(previous["flow"], "UNCONDITIONAL_JUMP")
            self.assertIsNone(previous.get("fallthrough"))
        for entry, end in ROUTINES:
            part = [r for r in self.records if entry <= r["address"] < end]
            self.assertEqual(part[-1]["assembly"], "blr")
            trailing = set(range(part[-1]["address"] + 4, end, 4))
            self.assertLess(len(trailing), 4)
            self.assertEqual({a for a in missing if entry <= a < end},
                             trailing | {a for a in UNINDEXED_NOPS if entry <= a < end})
            for address in trailing:
                self.assertEqual(self.word(address), 0)
        self.assertEqual(len(missing), 58)

    def test_whole_export_incoming_flows_only_constructor(self):
        with closing(sqlite3.connect(INPUT)) as db:
            incoming = []
            for a, body in db.execute(
                "SELECT DISTINCT i.address,i.body FROM instructions i,json_each(i.body,'$.flows') f "
                "WHERE f.value>=? AND f.value<?", MODULE):
                if MODULE[0] <= a < MODULE[1]:
                    continue
                record = json.loads(body)
                incoming.append((a, record["flow"], tuple(record["flows"])))
        self.assertEqual(sorted(incoming), [(0x1034ef9c, "UNCONDITIONAL_CALL", (0x104f9fa0,))])

    def test_bytewise_pointers_are_twenty_tvs_and_accounted_data(self):
        pointers = {}
        for start, data in self.segments:
            for match in re.finditer(b"\x10[\x4f\x50]", data):
                offset = match.start()
                if offset + 4 <= len(data):
                    target = int.from_bytes(data[offset:offset + 4], "big")
                    if MODULE[0] <= target < MODULE[1]:
                        pointers[start + offset] = target
        self.assertEqual(pointers, TVS | DATA_MATCHES)
        for descriptor in TVS:
            self.assertEqual(self.word(descriptor + 4), 0x108bba90)
        for address, target in DATA_MATCHES.items():
            self.assertNotEqual(address % 4, 0)
            self.assertNotEqual(self.word(address + 4), 0x108bba90)
            self.assertNotIn(target, {entry for entry, _ in ROUTINES})
        # The overlapping target is assembled from two external switch labels.
        self.assertEqual((self.word(0x108d4b40), self.word(0x108d4b44)), (0x10471050, 0x10471058))
        rng_base = self.word(0x108bba90 - 0x7b8c)
        rng_end = self.word(0x108bba90 - 0x7b88)
        for address in (0x108e6947, 0x108fa09e, 0x108fbccd):
            self.assertTrue(rng_base <= address < rng_end)

    def test_vtable_closes_indirect_entries_and_direct_calls_close_helpers(self):
        table = self.word(0x108bba90 - 0x606c)
        self.assertEqual(table, 0x108d9ad4)
        self.assertEqual({self.word(table + offset) for offset in VTABLE}, set(TVS))
        for offset, descriptor in VTABLE.items():
            self.assertEqual(self.word(table + offset), descriptor)
        owned_calls = {target for r in self.records if r["flow"] == "UNCONDITIONAL_CALL"
                       for target in r["flows"] if MODULE[0] <= target < MODULE[1]}
        self.assertEqual(owned_calls, {0x104ff090, 0x104fbe40, 0x104faf40, 0x104fcbc0,
                                      0x104fd320, 0x104fb050, 0x104fc840, 0x104fd680,
                                      0x104fe0b0, 0x105022a0})
        self.assertEqual({entry for entry, _ in ROUTINES}, {MODULE[0]} | set(TVS.values()) | owned_calls)

    def test_control_flow_closes_each_routine_without_interior_resume_or_switch(self):
        entries = {entry for entry, _ in ROUTINES}
        for entry, end in ROUTINES:
            for r in (r for r in self.records if entry <= r["address"] < end):
                self.assertNotIn(r["assembly"], ("bctr", "bctrl"))
                self.assertNotIn(r["flow"], ("COMPUTED_JUMP", "COMPUTED_CALL"))
                for target in r["flows"]:
                    if MODULE[0] <= target < MODULE[1]:
                        self.assertIn(target, self.indexed)
                        if not entry <= target < end:
                            self.assertEqual(r["flow"], "UNCONDITIONAL_CALL")
                            self.assertIn(target, entries)
                    else:
                        self.assertEqual(r["flow"], "UNCONDITIONAL_CALL")
        self.assertEqual([s for s in self.program["symbols"] if MODULE[0] <= s["address"] < MODULE[1]], [])

    def test_production_inventory_and_digests(self):
        from compile_recomp import OWNER_REPLACEMENTS
        self.assert_production_inventory(rewritten(), OWNER_REPLACEMENTS)
        with closing(sqlite3.connect(INPUT)) as db:
            require_euro_champ_qual_index(db)

    def test_stale_duplicate_and_out_of_scope_registration_are_rejected(self):
        from tactics_audit import digest, routine_records
        with closing(sqlite3.connect(INPUT)) as db:
            first, end = ROUTINES[0]
            original_digest = digest(routine_records(db, first, end))
            with patch.object(audit, "rewritten", return_value=[(first, end, "0" * 16)]):
                with self.assertRaisesRegex(ValueError, "changed in the export"):
                    require_euro_champ_qual_index(db)
        line = f'EURO_CHAMP_QUAL_ROUTINE({first:#x}, {end:#x}, R3, "{original_digest}", unused)\n'
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "routines.inc"
            path.write_text(line + line)
            with self.assertRaisesRegex(ValueError, "listed twice"):
                rewritten(path)
            path.write_text(line.replace(f"{end:#x}", f"{end + 4:#x}"))
            with self.assertRaisesRegex(ValueError, "not a routine"):
                rewritten(path)

    def test_missing_and_disabled_production_owners_are_rejected(self):
        from compile_recomp import OWNER_REPLACEMENTS
        with self.assertRaises(AssertionError):
            self.assert_production_inventory(rewritten()[1:], OWNER_REPLACEMENTS)
        owners = [(a, owner) for a, owner in OWNER_REPLACEMENTS if a != MODULE[0]]
        with self.assertRaises(AssertionError):
            self.assert_production_inventory(rewritten(), owners)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "disabled.inc"
            path.write_text(INCLUDES.read_text().replace("EURO_CHAMP_QUAL_ROUTINE(", "EURO_CHAMP_QUAL_DRAFT(", 1))
            with self.assertRaises(AssertionError):
                self.assert_production_inventory(rewritten(path), OWNER_REPLACEMENTS)

    def test_default_generator_guards_every_entry_and_reference_bypasses(self):
        from compile_recomp import generate_page
        selected = set(self.indexed)
        for page in sorted({a >> 12 for a, _ in ROUTINES}):
            part = [r for r in self.records if r["address"] >> 12 == page]
            normal = generate_page(page, part, selected)
            self.assertIn('#include "euro_champ_qual_adapter.hpp"', normal)
            for entry, _ in ROUTINES:
                if entry >> 12 == page:
                    self.assertIn(f"if (try_modern_euro_champ_qual(s, 0x{entry:x})) return true;", normal)
            reference = generate_page(page, part, selected, euro_champ_qual_hooks=False)
            self.assertNotIn("try_modern_euro_champ_qual", reference)

    def test_virtual_calls_explicitly_load_the_original_descriptor(self):
        calls = [r for r in self.records if r["assembly"] == "bl 0x10781320"]
        self.assertEqual(len(calls), 15)
        for call in calls:
            loads = [r for r in self.records if call["address"] - 32 <= r["address"] < call["address"]
                     and r["assembly"].startswith("lwz r12,")]
            self.assertTrue(loads)
            self.assertRegex(loads[-1]["assembly"], r"lwz r12,0x[0-9a-f]+\(r12\)")

    def test_no_unmodelled_scratch_register_dependencies(self):
        with closing(sqlite3.connect(INPUT)) as db:
            self.assertEqual(audit.volatile_reads(db), [])

    def test_owned_sources_do_not_call_reference_bodies_or_builders(self):
        paths = list((ROOT / "native").glob("euro_champ_qual*.cpp"))
        self.assertTrue(paths)
        for path in paths:
            if path.name in {"euro_champ_qual_adapter.cpp", "euro_champ_qual_checks.cpp"}:
                continue
            for forbidden in ("SemanticGuestCall", "run_original_body", "euro_champ_qual_reference"):
                self.assertNotIn(forbidden, path.read_text(), path.name)


if __name__ == "__main__":
    unittest.main()
