"""Certify AWARD_MANAGER.CPP boundaries, transfer targets and routing."""
from contextlib import closing
import inspect
import json
from pathlib import Path
import re
import sqlite3
import tempfile
import unittest

import award_manager_audit as audit
from tactics_audit import routine_records
from main_program_audit import module_volatile_reads

EXPORT = audit.ROOT / ".local/current/export"
ENTRIES = tuple(start for start, _, _, _ in audit.ROUTINES)
TOC = 0x108bba90
VECTORS = {0x108b960c: 0x10437b50, 0x108b9614: 0x10437b20}
BYTE_COINCIDENCES = {0x104931b9: 0x10437883, 0x1074aac5: 0x10437883, 0x108e5461: 0x10436d24}
INTERNAL_CALLS = {0x1043622c: 0x10436dc0, 0x10436248: 0x104365b0, 0x10436ecc: 0x104365b0}
INCOMING_CALLS = {
    0x10345138: 0x10436080, 0x10349ecc: 0x10436080, 0x1034e02c: 0x104362d0,
    0x101cdd10: 0x104363e0, 0x10371cec: 0x104363e0, 0x10119270: 0x104364e0,
    0x10119840: 0x104364e0, 0x1034d040: 0x10436f60, 0x1044cfcc: 0x104370f0,
    0x1044d230: 0x10437130, 0x1044b4e8: 0x10437160, 0x1044b678: 0x104371a0,
    0x102a9f14: 0x104371d0, 0x102be364: 0x104372c0, 0x102be1ac: 0x10437310,
    0x1043849c: 0x10437730, 0x10002d34: 0x10437850, 0x10002db4: 0x10437850,
    0x1038a9fc: 0x10437910, 0x10000128: 0x10437a00, 0x100021e8: 0x10437a00,
    0x10065844: 0x10437a90,
}
NOP_GAPS = {
    0x1043647c: "b 0x10436488", 0x1043725c: "b 0x10437298",
    0x10437a2c: "b 0x10437a68", 0x10437a5c: "blr",
    0x10437abc: "b 0x10437af8", 0x10437aec: "blr",
}


def database():
    return closing(sqlite3.connect((EXPORT / "instructions.sqlite").resolve().as_uri() + "?mode=ro", uri=True))


class AwardManagerAudit(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.program = json.loads((EXPORT / "program.json").read_text())
        cls.segments = [(s["address"], (EXPORT / s["file"]).read_bytes())
                        for s in cls.program["segments"] if "file" in s]
        with database() as db:
            cls.records = routine_records(db, *audit.MODULE)
        cls.instructions = {r["address"]: r["assembly"] for r in cls.records}

    def bytes_at(self, address, size):
        for start, contents in self.segments:
            if start <= address and address + size <= start + len(contents):
                return contents[address - start:address - start + size]
        self.fail(f"Original bytes absent at {address:x}")

    def word(self, address):
        return int.from_bytes(self.bytes_at(address, 4), "big")

    def test_partition_digests_and_padding(self):
        start, end = audit.MODULE
        self.assertEqual(tuple(zip(ENTRIES, ENTRIES[1:] + (end,))), tuple(r[:2] for r in audit.ROUTINES))
        self.assertEqual(len(self.instructions), 1775)
        missing = set(range(start, end, 4)) - self.instructions.keys()
        self.assertEqual(len(missing), 33)
        for address in missing:
            self.assertEqual(self.word(address), 0x60000000 if address in NOP_GAPS else 0)
        for address, predecessor in NOP_GAPS.items():
            self.assertEqual(self.instructions[address - 4], predecessor)
        for r in self.records:
            self.assertFalse(set(r.get("flows", ())) & missing)
            self.assertNotIn(r.get("fallthrough"), missing)
        with database() as db:
            audit.require_award_manager_index(db)

    def test_source_pools_and_neighbor_classes(self):
        manager_source = b"AWARD_MANAGER.CPP\0"
        pool = self.word(TOC - 0x4a84)
        self.assertEqual(self.bytes_at(pool + 0x6a, len(manager_source)), manager_source)
        self.assertEqual(self.bytes_at(self.word(TOC - 0x4a64), 20), b"AWARD_SHORTLIST.CPP\0")
        self.assertEqual(self.word(TOC - 0x4a88), 0x1082b630)
        # The preceding base-award class has a distinct vtable and numeric
        # pool. Its source filename is not encoded in those objects.
        vtable = self.word(TOC - 0x6208)
        self.assertEqual(vtable, 0x108cf394)
        self.assertEqual([self.word(self.word(vtable + offset)) for offset in range(8, 24, 4)],
                         [0x10435c00, 0x10436050, 0x10436060, 0x10436070])
        with database() as db:
            rows = [(a, json.loads(b)["assembly"]) for a, b in db.execute(
                "SELECT address, body FROM instructions WHERE body LIKE '%-0x4a84(r2)%' OR "
                "body LIKE '%-0x4a88(r2)%' ORDER BY address")]
            self.assertEqual([a for a, asm in rows if "-0x4a88(r2)" in asm], [0x10435c68])
            self.assertEqual([a for a, asm in rows if "-0x4a84(r2)" in asm],
                             [0x10436088, 0x10436dcc, 0x10436f6c, 0x104371dc,
                              0x1043733c, 0x10437738, 0x10437868, 0x1043791c])
            self.assertEqual(routine_records(db, 0x10437cc0, 0x10437cc4)[0]["assembly"], "mfspr r0,LR")

    def test_direct_and_indirect_edges(self):
        start, end = audit.MODULE
        with database() as db:
            records = (json.loads(row[0]) for row in db.execute(
                "SELECT body FROM instructions WHERE body LIKE '%1043%' OR "
                "address BETWEEN ? AND ?", (start, end - 1)))
            direct = {(r["address"], target): r["flow"] for r in records for target in r.get("flows", ())
                      if start <= target < end and (target in ENTRIES or not start <= r["address"] < end)}
        self.assertEqual(direct, {(site, target): "UNCONDITIONAL_CALL"
                                  for site, target in (INTERNAL_CALLS | INCOMING_CALLS).items()})
        self.assertFalse(any(r["flow"].startswith("COMPUTED") for r in self.records))
        self.assertEqual([r["address"] for r in self.records if r["assembly"] == "bl 0x10781320"],
                         [0x10436470, 0x10436528, 0x104372f8])
        references = {}
        for base, contents in self.segments:
            for candidate in re.finditer(b"\x10\x43", contents):
                offset = candidate.start()
                target = int.from_bytes(contents[offset:offset + 4], "big")
                if start <= target < end:
                    references[base + offset] = target
        self.assertEqual(references, VECTORS | BYTE_COINCIDENCES)
        for address, target in VECTORS.items():
            self.assertIn(target, ENTRIES)
            self.assertEqual(self.word(address + 4), TOC)
        self.assertEqual(self.word(TOC - 0x6200), 0x108b9614)
        self.assertEqual(self.word(TOC - 0x6204), 0x108b960c)
        self.assertTrue(all(address % 4 for address in BYTE_COINCIDENCES))

    def test_scratch_reads_at_call_boundaries(self):
        with database() as db:
            reads = module_volatile_reads(db, [(a, b) for a, b, _, _ in audit.ROUTINES],
                                          audit.MODULE, dict.fromkeys(ENTRIES, "R3"))
        # The only scratch input found is the explicitly marshaled descriptor.
        self.assertEqual(reads, [f"r12 read at 10781320 in 10781320, called by the module at {site:x}"
                                 for site in (0x10436470, 0x10436528, 0x104372f8)])

    def test_rejects_wrong_digest_end_and_duplicate(self):
        contents = audit.INCLUDES.read_text()
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "routines.inc"
            for modified, error in (
                (contents + contents.splitlines()[0] + "\n", "Duplicate"),
                (contents.replace(audit.ROUTINES[0][3], "0" * 16), "Invalid"),
                (contents.replace("0x104362d0,", "0x104362d4,", 1), "Invalid"),
            ):
                path.write_text(modified)
                with self.assertRaisesRegex(ValueError, error):
                    audit.rewritten(path)

    def test_normal_entry_hooks_and_original_bypass(self):
        from compile_recomp import OWNER_REPLACEMENTS, generate_page

        self.assertEqual(audit.rewritten(), ENTRIES)
        start, end = audit.MODULE
        self.assertEqual({(entry, owner) for entry, owner in OWNER_REPLACEMENTS if start <= entry < end},
                         {(entry, "award_manager_adapter") for entry in ENTRIES})
        selected = set(self.instructions)
        no_hooks = {name: False for name in inspect.signature(generate_page).parameters
                    if name.endswith("_hooks") or name == "guest_context_transfers"}
        for page in sorted({entry >> 12 for entry in ENTRIES}):
            part = [r for r in self.records if r["address"] >> 12 == page]
            production = generate_page(page, part, selected, nontrapping_divisions=frozenset())
            reference = generate_page(page, part, selected, nontrapping_divisions=frozenset(), **no_hooks)
            self.assertNotIn("try_modern_award_manager", reference)
            for entry in ENTRIES:
                if entry >> 12 != page:
                    continue
                hook = audit.award_manager_hook(entry)
                block = production.split(f"i_{entry:x}:")[1].split(f"i_{entry + 4:x}:")[0]
                self.assertEqual(block.count(hook), 1)
                self.assertLess(block.index(hook), block.index(f"s.step(0x{entry:x});"))


if __name__ == "__main__":
    unittest.main()
