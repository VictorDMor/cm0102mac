"""AWOL.CPP inventory, original data and default production ownership.

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

import awol_audit as audit
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / ".local/current/export"
INPUT = EXPORT / "instructions.sqlite"
EXPECTED = (
    (0x100031b0, 0x100032f8, 82, "dbf37186a498cfc2"),
    (0x10003300, 0x100033b8, 46, "5b89d70e4fcc287d"),
    (0x100033c0, 0x100036dc, 196, "4ee3f3c51a4d25e6"),
    (0x100036e0, 0x10003dc0, 437, "a3109ac4c3e09732"),
    (0x10003dc0, 0x10003ec0, 64, "9ff568548cf1a915"),
    (0x10003ec0, 0x10003fa0, 56, "77323f62df4e3602"),
    (0x10003fa0, 0x10004068, 50, "1269007001441ea0"),
    (0x10004070, 0x10006074, 2044, "783cc15561657a16"),
    (0x10006080, 0x100065f8, 350, "ea0af6047ffb0dcd"),
    (0x10006600, 0x100066d0, 52, "c82af434b401e1d4"),
    (0x100066d0, 0x100067d0, 64, "5b22d52e4b0f6056"),
    (0x100067d0, 0x10006828, 22, "0cf548df18428cff"),
    (0x10006830, 0x10006900, 52, "74ba3afe627dfc00"),
    (0x10006900, 0x10007100, 512, "49a26ee7f456a334"),
    (0x10007100, 0x100072e0, 120, "b0fbef965fac1da8"),
    (0x100072e0, 0x10007a4c, 475, "4060d1a8db059dcc"),
    (0x10007a50, 0x10008770, 840, "b7ac942845d68ff8"),
    (0x10008770, 0x10008b58, 250, "6e35594b1281fdf2"),
    (0x10008b60, 0x1000909c, 335, "0b6afeb918a04801"),
    (0x100090a0, 0x10009b54, 685, "ef751c922abb5245"),
    (0x10009b60, 0x10009ee8, 226, "80527d78abad7ab5"),
)
ZERO_PADDING = (
    0x100032f8, 0x100032fc, 0x100033b8, 0x100033bc, 0x100036dc, 0x10004068, 0x1000406c,
    0x10006074, 0x10006078, 0x1000607c, 0x100065f8, 0x100065fc, 0x10006828, 0x1000682c,
    0x10007a4c, 0x10008b58, 0x10008b5c, 0x1000909c, 0x10009b54, 0x10009b58, 0x10009b5c,
    0x10009ee8, 0x10009eec)
NOP_PADDING = (
    0x10003540, 0x100035dc, 0x10003694, 0x100039e0, 0x10003b78, 0x10003d10, 0x100040c4,
    0x1000413c, 0x1000484c, 0x10005588, 0x10005b5c)


def fingerprint(values):
    return hashlib.sha256(json.dumps(values, separators=(",", ":")).encode()).hexdigest()


class AwolAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not INPUT.is_file():
            raise AssertionError("AWOL audit requires the private original export")
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
            self.assertEqual([owner for at, owner in owners if at == entry], ["awol_adapter"])

    def test_exact_original_inventory_and_reference_digests(self):
        self.assertEqual(audit.MODULE, (0x100031b0, 0x10009ef0))
        self.assertEqual(audit.ROUTINES, tuple((a, b) for a, b, _, _ in EXPECTED))
        self.assertEqual(len(self.records), 6958)
        self.assertEqual(len({r["address"] for r in self.records}), 6958)
        self.assertTrue(all(any(a <= r["address"] < b for a, b, _, _ in EXPECTED) for r in self.records))
        with closing(sqlite3.connect(INPUT)) as db:
            for entry, end, count, expected_digest in EXPECTED:
                body = routine_records(db, entry, end)
                self.assertEqual((len(body), digest(body)), (count, expected_digest))
                self.assertEqual(body[-1]["assembly"], "blr")
            audit.require_awol_index(db)
        self.assertEqual([(a, b, d) for a, b, _, d in EXPECTED], audit.rewritten(drafts=True))

    def test_default_production_owners_are_exclusive(self):
        from compile_recomp import OWNER_REPLACEMENTS, SEMANTIC_OWNERS, direct_owner_entries
        self.production(audit.rewritten(), OWNER_REPLACEMENTS)
        for entry, _ in audit.ROUTINES:
            self.assertEqual(SEMANTIC_OWNERS[entry], "awol_adapter")
            self.assertEqual(direct_owner_entries().get(entry), "awol")

    def test_source_pools_neighbors_and_boundary_code(self):
        toc = 0x108bba90
        # The pool's TOC slot is loaded only inside the module, first by load
        # and last by describe_fitness; the slots around it by the neighbors.
        self.assertEqual(self.word(toc-0x5a08), 0x10781420)
        self.assertEqual(self.original(0x10781420, 18), b"awol.dat\0AWOL.CPP\0")
        self.assertEqual(self.word(toc-0x5a0c), 0x10782b3c)
        self.assertEqual(self.word(toc-0x59d8), 0x10782b5c)
        self.assertEqual(self.original(0x10782b5c, 9), b"CASH.CPP\0")
        with closing(sqlite3.connect(INPUT)) as db:
            def loaders(offset):
                return [row[0] for row in db.execute(
                    "SELECT address FROM instructions WHERE json_extract(body,'$.assembly') LIKE ? ORDER BY address",
                    (f"%{offset}(r2)",))]
            self.assertEqual(loaders("-0x5a08"), [0x10003404, 0x100036f8, 0x10007304, 0x10008b80, 0x10009b78])
            self.assertEqual(len(loaders("-0x5a0c")), 5)
            self.assertTrue(all(audit.MODULE[0] <= a < audit.MODULE[1] for a in loaders("-0x5a0c")))
            self.assertLess(max(loaders("-0x5a10")), audit.MODULE[0])
            self.assertGreaterEqual(min(loaders("-0x5a04")), audit.MODULE[1])
            for address, assembly in ((0x100031ac, "blr"), (0x100031b0, "mfspr r0,LR"),
                                      (0x10009ee4, "blr"), (0x10009ef0, "mfspr r0,LR")):
                body = json.loads(db.execute("SELECT body FROM instructions WHERE address=?", (address,)).fetchone()[0])
                self.assertEqual(body["assembly"], assembly)

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
        self.assertEqual(len(external), 49)
        self.assertEqual(fingerprint(sorted(external)), "ec6053de4f15557e1ca3b50a1f82c1d84e889a435ad89dbab410045fcd5968bc")
        self.assertEqual(len(cross), 48)
        self.assertEqual(fingerprint(sorted(cross)), "5c91a9b49660f4f2f168c5ab2c2854fefd9a43abeebcb64e6d2e9d83b0140069")
        # Reached only from inside the module.
        self.assertEqual({t for _, t, _ in external} & {0x100033c0, 0x10006080, 0x100066d0, 0x100067d0, 0x10007100, 0x10008770}, set())
        self.assertEqual({t for _, t, _ in external+cross}, set(entries))

    def test_no_callback_vectors_or_computed_transfers(self):
        indexed = {r["address"] for r in self.records}
        aligned = {}
        for base, _, data in self.segments:
            if data is None: continue
            for match in re.finditer(b"\x10\x00", data):
                offset = match.start()
                if offset+4 > len(data): continue
                target = int.from_bytes(data[offset:offset+4], "big")
                if audit.MODULE[0] <= target < audit.MODULE[1] and (base+offset) % 4 == 0:
                    aligned[base+offset] = target
        self.assertEqual(aligned, {})
        self.assertFalse(any(r["assembly"].startswith(("bctr", "bcctr", "bcctrl")) for r in self.records))
        self.assertTrue(indexed)

    def test_inline_draw_bounds_cannot_reach_the_wide_arm(self):
        # 31 inline draws have an arm for bounds beyond 65535 in size. Each
        # bound is defined just before its draw from a byte or a halfword, so
        # the C++ has no such arm. sample's own deeper arms hold a value already
        # reduced by 65535.
        records = {r["address"]: r for r in self.records}
        ordered = sorted(records)
        at = {a: i for i, a in enumerate(ordered)}
        narrow = re.compile(r"(extsb|extsh)\.? r\d+,r\d+$")
        sites = [a for a in ordered if records[a]["assembly"] == "bl 0x10006080"]
        seen = 0
        for site in sites:
            start = at[site]
            while "0x4330" not in records[ordered[start]]["assembly"]: start -= 1
            bound = re.search(r"xoris r\d+,(r\d+),", records[ordered[start+1]]["assembly"]).group(1)
            if 0x10006080 <= site < 0x100065f8: continue
            seen += 1
            look = start
            while not re.match(rf"\S+ {bound},", records[ordered[look]]["assembly"]) or \
                    records[ordered[look]]["assembly"].startswith(("xoris", "cmp", "stw", "stb", "sth", "mulli")):
                look -= 1
            definition = records[ordered[look]]["assembly"]
            if definition.startswith("addic."):
                # A byte plus ten: its source was sign-extended just before.
                source = re.search(r"addic\. r\d+,(r\d+),0xa$", definition).group(1)
                self.assertTrue(any(records[ordered[look-n]]["assembly"].startswith(f"extsb {source},") for n in range(1, 4)), hex(site))
            else:
                self.assertRegex(definition, narrow, hex(site))
        self.assertEqual(seen, 31)
        self.assertEqual(len(sites), 32)

    def test_all_unindexed_words(self):
        indexed = {r["address"] for r in self.records}
        gaps = {a: self.word(a) for a in range(*audit.MODULE, 4) if a not in indexed}
        expected = {a: 0 for a in ZERO_PADDING} | {a: 0x60000000 for a in NOP_PADDING}
        self.assertEqual(gaps, expected)

    def test_original_constants_and_text_digests(self):
        self.assertEqual(hashlib.sha256(self.original(0x10781420, 5916)).hexdigest(),
                         "fae7ca030e5b50008dc72666d6f4bae42388b45c4ad81524c26f904534ad7119")
        self.assertEqual(hashlib.sha256(self.original(0x10782b3c, 32)).hexdigest(),
                         "b41e5252074a70916a9a3594bab9b4fdf58a0042b8ea4aeedb09d70655768bc3")

    def test_default_generation_and_reference_owner_fallback(self):
        from compile_recomp import generate_page
        self.assertTrue(inspect.signature(generate_page).parameters["awol_hooks"].default)
        selected = {r["address"] for r in self.records}
        for page in sorted({entry >> 12 for entry, _ in audit.ROUTINES}):
            part = [r for r in self.records if r["address"] >> 12 == page]
            normal = generate_page(page, part, selected, userops=self.userops)
            reference = generate_page(page, part, selected, userops=self.userops, awol_hooks=False)
            self.assertIn('#include "awol_adapter.hpp"', normal)
            self.assertNotIn("try_modern_awol(", reference)
            for entry, _ in audit.ROUTINES:
                if entry >> 12 != page: continue
                hook = audit.awol_hook(entry)
                self.assertEqual(normal.count(hook), 1)
                fragment = normal.split(f"i_{entry:x}:", 1)[-1].split(f"i_{entry+4:x}:", 1)[0]
                self.assertIn(hook, fragment)
                self.assertNotIn(f"try_modern_awol_records(s, 0x{entry:x})", normal)
                self.assertIn(f"try_modern_awol_records(s, 0x{entry:x})", reference)

    def test_reject_stale_duplicate_disabled_and_missing_owner(self):
        from compile_recomp import OWNER_REPLACEMENTS
        all_routines = audit.rewritten(drafts=True)
        stale = [(a, b, "0"*16 if i == 0 else d) for i, (a, b, d) in enumerate(all_routines)]
        with closing(sqlite3.connect(INPUT)) as db, patch.object(audit, "rewritten", return_value=stale):
            with self.assertRaisesRegex(ValueError, "changed in the export"):
                audit.require_awol_index(db)
        source = audit.routine_lines()
        first = next(line for line in source.splitlines() if line.startswith(("AWOL_ROUTINE(", "AWOL_DRAFT(")))
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "routines.inc"
            path.write_text(source+"\n"+first+"\n")
            with self.assertRaisesRegex(ValueError, "listed twice"): audit.rewritten(path, drafts=True)
            path.write_text(source.replace("AWOL_ROUTINE(", "AWOL_DRAFT(", 1))
            with self.assertRaises(AssertionError): self.production(audit.rewritten(path), OWNER_REPLACEMENTS)
        expected = [(a, b, d) for a, b, _, d in EXPECTED]
        owners = [(a, "awol_adapter") for a, _, _, _ in EXPECTED]
        with self.assertRaises(AssertionError): self.production(expected, owners[1:])
        with self.assertRaises(AssertionError): self.production(expected, [(a, "wrong") for a, _ in owners])


if __name__ == "__main__":
    unittest.main()
