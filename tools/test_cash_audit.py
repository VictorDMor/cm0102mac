"""CASH.CPP inventory, original data and default production ownership.

Private source is required: missing evidence must fail this delivery gate.
"""
from contextlib import closing
import hashlib
import inspect
import json
from pathlib import Path
import re
import sqlite3
import tempfile
import unittest
from unittest.mock import patch

import cash_audit as audit
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / ".local/current/export"
INPUT = EXPORT / "instructions.sqlite"
EXPECTED = (
    (0x10009ef0, 0x10009f18, 10, "a6df7bb77313c601"),
    (0x10009f20, 0x1000a008, 58, "bf9ff980d10ed9b2"),
    (0x1000a010, 0x1000a05c, 19, "089b6525b162889b"),
    (0x1000a060, 0x1000a164, 65, "28007081c692be5c"),
    (0x1000a170, 0x1000a26c, 63, "9fa2ff5af4993a06"),
    (0x1000a270, 0x1000a32c, 47, "5f372db28f5b768e"),
    (0x1000a330, 0x1000a440, 68, "5effcc613a9c71e9"),
    (0x1000a440, 0x1000b148, 834, "a9a2eb24f03688f0"),
    (0x1000b150, 0x1000b158, 2, "1179bd78a334bd29"),
    (0x1000b160, 0x1000b410, 172, "2435c4543cc47252"),
    (0x1000b410, 0x1000c764, 1236, "9029175b0fe52f1c"),
    (0x1000c770, 0x1000cbb4, 273, "39b872019226aba8"),
)


def fingerprint(values):
    return hashlib.sha256(json.dumps(values, separators=(",", ":")).encode()).hexdigest()


class CashAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not INPUT.is_file():
            raise AssertionError("CASH audit requires the private original export")
        program = json.loads((EXPORT / "program.json").read_text())
        cls.segments = [(s["address"], s["size"],
                         (EXPORT / s["file"]).read_bytes() if s.get("file") else None)
                        for s in program["segments"]]
        cls.userops = program["userops"]
        with closing(sqlite3.connect(INPUT)) as db:
            cls.records = [json.loads(body) for body, in db.execute(
                "SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address", audit.MODULE)]

    def original(self, address, size=4):
        for base, length, data in self.segments:
            if base <= address and address + size <= base + length:
                return bytes(size) if data is None else data[address-base:address-base+size]
        self.fail(f"Missing original bytes at {address:x}")

    def word(self, address):
        return int.from_bytes(self.original(address), "big")

    def production(self, routines, owners):
        self.assertEqual([(a, b) for a, b, _ in routines], list(audit.ROUTINES))
        for entry, _ in audit.ROUTINES:
            self.assertEqual([owner for at, owner in owners if at == entry], ["cash_adapter"])

    def test_exact_original_inventory_and_reference_digests(self):
        self.assertEqual(audit.MODULE, (0x10009ef0, 0x1000cbc0))
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in EXPECTED))
        self.assertEqual(len(self.records), 2847)
        self.assertEqual(len({r["address"] for r in self.records}), 2847)
        self.assertTrue(all(any(a <= r["address"] < b for a, b, _, _ in EXPECTED) for r in self.records))
        with closing(sqlite3.connect(INPUT)) as db:
            for entry, end, count, expected_digest in EXPECTED:
                body = routine_records(db, entry, end)
                self.assertEqual((len(body), digest(body)), (count, expected_digest))
                self.assertEqual(body[-1]["assembly"], "blr")
            audit.require_cash_index(db)
        self.assertEqual([(a, b, d) for a, b, _, d in EXPECTED], audit.rewritten(drafts=True))

    def test_default_production_owners_are_exclusive(self):
        from compile_recomp import OWNER_REPLACEMENTS, SEMANTIC_OWNERS, direct_owner_entries
        self.production(audit.rewritten(), OWNER_REPLACEMENTS)
        for entry, _ in audit.ROUTINES:
            self.assertEqual(SEMANTIC_OWNERS[entry], "cash_adapter")
            self.assertEqual(direct_owner_entries().get(entry), "cash")

    def test_source_pools_neighbors_and_boundary_code(self):
        toc = 0x108bba90
        for slot, base, text in ((-0x5a08, 0x10781420, b"awol.dat\0AWOL.CPP\0"),
                                 (-0x59d8, 0x10782b5c, b"CASH.CPP\0"),
                                 (-0x59c8, 0x10783098, b"Insert CD\0")):
            self.assertEqual(self.word(toc+slot), base)
            self.assertEqual(self.original(base, len(text)), text)
        self.assertEqual(self.original(0x107830f0, 17), b"CLUB_HISTORY.CPP\0")
        self.assertEqual(self.word(toc-0x59d4), 0x10782eb4)
        self.assertEqual(self.word(toc-0x59cc), 0x108b8624)
        self.assertEqual((self.word(0x108b8624), self.word(0x108b8628)), (0x1000e680, toc))
        with closing(sqlite3.connect(INPUT)) as db:
            for address, assembly in ((0x10009ee4, "blr"), (0x10009ef0, "mfspr r0,LR"),
                                      (0x1000cbb0, "blr"), (0x1000cbc0, "mfspr r0,LR")):
                body = json.loads(db.execute("SELECT body FROM instructions WHERE address=?", (address,)).fetchone()[0])
                self.assertEqual(body["assembly"], assembly)
        self.assertEqual(self.original(0x10009ee8, 8), bytes(8))

    def test_all_incoming_and_cross_entry_flows(self):
        external, cross = [], []
        entries = dict(audit.ROUTINES)
        with closing(sqlite3.connect(INPUT)) as db:
            rows = db.execute("""SELECT i.address,e.value,json_extract(i.body,'$.assembly')
                FROM instructions i,
                json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) e
                WHERE e.value>=? AND e.value<?""", audit.MODULE)
            for source, target, assembly in rows:
                if not audit.MODULE[0] <= source < audit.MODULE[1]:
                    self.assertIn(target, entries)
                    self.assertTrue(assembly.startswith("bl "))
                    external.append((source, target, assembly))
                elif not any(a <= source < b and a <= target < b for a, b in audit.ROUTINES):
                    self.assertIn(target, entries)
                    self.assertTrue(assembly.startswith("bl "))
                    cross.append((source, target, assembly))
        self.assertEqual(len(external), 868)
        self.assertEqual(fingerprint(sorted(external)), "eb3f2a606a162abd25b9dcb17da1bb5672488b70d7e9d9864ea17aff7d6a0cc6")
        self.assertEqual(len(cross), 6)
        self.assertEqual(fingerprint(sorted(cross)), "3b1122e54217cec0e0297ad24553d3e5fe5d72d694597513ac73b71ae41c27d7")
        self.assertEqual({t for _, t, _ in external+cross}, set(entries))

    def test_four_switch_banks_and_no_callback_vectors(self):
        toc = 0x108bba90
        banks = ((-0x59f4, 0x108c7e58, 25, 0x1000a440, 0x1000b148),
                 (-0x59f8, 0x108c7ebc, 25, 0x1000b160, 0x1000b410),
                 (-0x5a00, 0x108c7f60, 16, 0x1000b410, 0x1000c764),
                 (-0x5a04, 0x108c7f20, 16, 0x1000b410, 0x1000c764))
        genuine = {}
        indexed = {r["address"] for r in self.records}
        for slot, base, count, low, high in banks:
            self.assertEqual(self.word(toc+slot), base)
            for at in range(base, base+count*4, 4):
                target = self.word(at)
                self.assertTrue(low < target < high)
                self.assertIn(target, indexed)
                genuine[at] = target
        candidates = {}
        instruction_coincidences = []
        packed_coincidences = []
        with closing(sqlite3.connect(INPUT)) as db:
            for base, _, data in self.segments:
                if data is None: continue
                for match in re.finditer(b"\x10\x00", data):
                    offset = match.start()
                    if offset+4 > len(data): continue
                    target = int.from_bytes(data[offset:offset+4], "big")
                    if not audit.MODULE[0] <= target < audit.MODULE[1]: continue
                    at = base+offset
                    if base == 0x10000000 and at % 4:
                        for word in (at & ~3, (at & ~3)+4):
                            self.assertIsNotNone(db.execute("SELECT address FROM instructions WHERE address=?", (word,)).fetchone())
                        instruction_coincidences.append(at)
                    elif at % 4:
                        packed_coincidences.append((at, target))
                    else: candidates[at] = target
        self.assertEqual(candidates, genuine)
        self.assertEqual(len(genuine), 82)
        self.assertEqual(len(instruction_coincidences), 46)
        self.assertEqual(packed_coincidences, [
            (0x108bf003, 0x1000b406), (0x108bfe59, 0x1000c400),
            (0x108c1ceb, 0x1000bc03), (0x108c3a8d, 0x1000a009),
            (0x108c5d35, 0x1000b004), (0x108c6a4f, 0x1000b805),
            (0x108c76a7, 0x1000b004), (0x108c7aad, 0x1000c000),
            (0x108c7c85, 0x1000b801), (0x108c9949, 0x1000a001)])
        self.assertEqual([r["address"] for r in self.records if r["assembly"] == "bctr"],
                         [0x1000a494, 0x1000b20c, 0x1000b5dc, 0x1000bd34])
        self.assertFalse(any(r["assembly"].startswith(("bctrl", "bcctr")) for r in self.records))

    def test_all_unindexed_words(self):
        indexed = {r["address"] for r in self.records}
        gaps = {a: self.word(a) for a in range(*audit.MODULE, 4) if a not in indexed}
        expected = {a: 0 for a in (
            0x10009f18, 0x10009f1c, 0x1000a008, 0x1000a00c, 0x1000a05c,
            0x1000a164, 0x1000a168, 0x1000a16c, 0x1000a26c, 0x1000a32c,
            0x1000b148, 0x1000b14c, 0x1000b158, 0x1000b15c, 0x1000c764,
            0x1000c768, 0x1000c76c, 0x1000cbb4, 0x1000cbb8, 0x1000cbbc)}
        expected[0x1000b524] = 0x60000000
        self.assertEqual(gaps, expected)

    def test_original_constants_and_table_digests(self):
        import money_value_audit as value
        import money_text_audit as text
        import money_configuration_audit as configuration
        for module in (value, text, configuration):
            getattr(module, "load_"+module.__name__.removesuffix("_audit")+"_tables")(EXPORT)
            with closing(sqlite3.connect(INPUT)) as db:
                getattr(module, "require_"+module.__name__.removesuffix("_audit")+"_index")(db)

    def test_default_generation_and_reference_owner_fallback(self):
        from compile_recomp import generate_page
        self.assertIn("cash_hooks", inspect.signature(generate_page).parameters)
        self.assertTrue(inspect.signature(generate_page).parameters["cash_hooks"].default)
        selected = {r["address"] for r in self.records}
        for page in sorted({entry >> 12 for entry, _ in audit.ROUTINES}):
            part = [r for r in self.records if r["address"] >> 12 == page]
            normal = generate_page(page, part, selected, userops=self.userops)
            reference = generate_page(page, part, selected, userops=self.userops, cash_hooks=False)
            self.assertTrue('#include "cash_adapter.hpp"' in normal, f"Page {page:x} lacks the CASH production adapter")
            self.assertNotIn("try_modern_cash", reference)
            for entry, _ in audit.ROUTINES:
                if entry >> 12 != page: continue
                hook = audit.cash_hook(entry)
                self.assertEqual(normal.count(hook), 1)
                fragment = normal.split(f"i_{entry:x}:", 1)[-1].split(f"i_{entry+4:x}:", 1)[0]
                self.assertIn(hook, fragment)
                legacy = next(name for name, entries in (
                    ("money_value", __import__("money_value_audit").MONEY_VALUE_ENTRIES),
                    ("money_text", __import__("money_text_audit").MONEY_TEXT_ENTRIES),
                    ("money_configuration", __import__("money_configuration_audit").MONEY_CONFIGURATION_ENTRIES)) if entry in entries)
                self.assertNotIn(f"try_modern_{legacy}(s, 0x{entry:x})", normal)
                self.assertIn(f"try_modern_{legacy}(s, 0x{entry:x})", reference)

    def test_reject_stale_duplicate_disabled_and_missing_owner(self):
        from compile_recomp import OWNER_REPLACEMENTS
        all_routines = audit.rewritten(drafts=True)
        stale = [(a, b, "0"*16 if i == 0 else d) for i, (a, b, d) in enumerate(all_routines)]
        with closing(sqlite3.connect(INPUT)) as db, patch.object(audit, "rewritten", return_value=stale):
            with self.assertRaisesRegex(ValueError, "changed in the export"):
                audit.require_cash_index(db)
        source = audit.routine_lines()
        first = next(line for line in source.splitlines() if line.startswith(("CASH_ROUTINE(", "CASH_DRAFT(")))
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "routines.inc"
            path.write_text(source+"\n"+first+"\n")
            with self.assertRaisesRegex(ValueError, "listed twice"): audit.rewritten(path, drafts=True)
            path.write_text(source.replace("CASH_ROUTINE(", "CASH_DRAFT(", 1))
            with self.assertRaises(AssertionError): self.production(audit.rewritten(path), OWNER_REPLACEMENTS)
        expected = [(a, b, d) for a, b, _, d in EXPECTED]
        owners = [(a, "cash_adapter") for a, _, _, _ in EXPECTED]
        with self.assertRaises(AssertionError): self.production(expected, owners[1:])
        with self.assertRaises(AssertionError): self.production(expected, [(a, "wrong") for a, _ in owners])


if __name__ == "__main__":
    unittest.main()
