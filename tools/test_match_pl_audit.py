"""Original MATCH_PL boundaries and incremental production ownership checks.

The private export is required: absence is a failing evidence gate, never a skip.
"""
from contextlib import closing
import hashlib
import importlib
import inspect
import json
from pathlib import Path
import re
import sqlite3
import struct
import tempfile
import unittest
from unittest.mock import patch

import match_pl_audit as audit

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / ".local/current/export"


class MatchPlAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not (INPUT / "instructions.sqlite").is_file():
            raise FileNotFoundError("Private MATCH_PL original export unavailable")
        cls.program = json.loads((INPUT / "program.json").read_text())
        cls.segments = [(s["address"], (INPUT / s["file"]).read_bytes())
                        for s in cls.program["segments"] if "file" in s]
        with closing(sqlite3.connect(INPUT / "instructions.sqlite")) as db:
            cls.records = {a: json.loads(b) for a, b in db.execute(
                "SELECT address,body FROM instructions WHERE address>=? AND address<? ORDER BY address",
                audit.MODULE)}
        cls.entries = {a for a, _ in audit.ROUTINES}
        cls.bounds = {pc: (a, b) for a, b in audit.ROUTINES for pc in range(a, b, 4)}

    def original_bytes(self, address, size):
        for base, data in self.segments:
            if base <= address and address + size <= base + len(data):
                return data[address-base:address-base+size]
        self.fail(f"Missing original bytes {address:x}+{size:x}")

    def word(self, address):
        return int.from_bytes(self.original_bytes(address, 4), "big")

    def test_inventory_partition_and_all_original_digests(self):
        self.assertEqual(audit.MODULE, (0x102238a0, 0x1028ad60))
        self.assertEqual(len(audit.ROUTINES), 95)
        self.assertEqual(len(self.entries), 95)
        self.assertEqual((audit.ROUTINES[0][0], audit.ROUTINES[-1][1]), audit.MODULE)
        self.assertTrue(all(a[1] == b[0] for a, b in zip(audit.ROUTINES, audit.ROUTINES[1:])))
        self.assertEqual(len(self.records), 105650)
        with closing(sqlite3.connect(INPUT / "instructions.sqlite")) as db:
            audit.require_match_pl_index(db)

    def test_source_pool_constructor_and_neighbor_boundaries(self):
        toc = 0x108bba90
        self.assertEqual(self.word(toc - 0x51e4), 0x107e4e3c)
        self.assertEqual(self.original_bytes(0x107e4e3c + 0x8f, 13), b"MATCH_PL.CPP\0")
        self.assertEqual(self.word(toc - 0x51e0), 0x107e53d8)
        self.assertEqual(self.original_bytes(self.word(toc - 0x5188), 18), b"MATCH_SCREENS.CPP\0")
        # The TV constructor reads MATCH_PL's own constants before the first
        # assertion pool load in the parameterized constructor.
        self.assertEqual(self.records[0x102238a8]["assembly"], "lwz r4,-0x51e0(r2)")
        with closing(sqlite3.connect(INPUT / "instructions.sqlite")) as db:
            at = lambda pc: json.loads(db.execute("SELECT body FROM instructions WHERE address=?", (pc,)).fetchone()[0])["assembly"]
            self.assertEqual(at(0x10223890), "blr")
            self.assertEqual(at(0x1028ad78), "lwz r31,-0x5188(r2)")
            pool_reads = [a for a, in db.execute(
                "SELECT address FROM instructions WHERE body LIKE ? ORDER BY address", ("%-0x51e4(r2)%",))]
        self.assertTrue(pool_reads)
        self.assertTrue(all(audit.MODULE[0] <= a < audit.MODULE[1] for a in pool_reads))

    def test_alignment_and_unreachable_duplicate_branches(self):
        missing = set()
        for entry, end in audit.ROUTINES:
            records = [r for pc, r in self.records.items() if entry <= pc < end]
            self.assertEqual(records[0]["address"], entry)
            self.assertEqual(records[-1]["assembly"], "blr")
            last = records[-1]["address"] + 4
            self.assertLess(end - last, 16)
            self.assertEqual(self.original_bytes(last, end - last), bytes(end - last))
            for pc in set(range(entry, last, 4)) - self.records.keys():
                missing.add(pc)
                # Unindexed words repeat the preceding unconditional branch.
                # No direct edge or bounded switch-table target reaches them.
                self.assertEqual(self.word(pc), {0x1026a888: 0x48002dc0,
                                                0x1026dc60: 0x4800269c}[pc])
                displacement = self.word(pc) & 0x03fffffc
                self.assertEqual(pc + displacement, self.records[pc - 4]["flows"][0])
                self.assertIsNone(self.records[pc - 4].get("fallthrough"))
        self.assertEqual(missing, {0x1026a888, 0x1026dc60})
        for record in self.records.values():
            self.assertFalse(missing.intersection(record["flows"]))

    def test_every_incoming_and_cross_entry_flow(self):
        incoming, cross, reached = [], [], set()
        with closing(sqlite3.connect(INPUT / "instructions.sqlite")) as db:
            for a, body in db.execute(
                    "SELECT DISTINCT i.address,i.body FROM instructions i,json_each(i.body,'$.flows') f "
                    "WHERE f.value>=? AND f.value<? ORDER BY i.address", audit.MODULE):
                record = json.loads(body)
                for target in record["flows"]:
                    if not audit.MODULE[0] <= target < audit.MODULE[1]:
                        continue
                    self.assertIn(target, self.records)
                    if not audit.MODULE[0] <= a < audit.MODULE[1]:
                        self.assertIn(target, self.entries)
                        incoming.append((a, record["flow"], target))
                        reached.add(target)
                    elif self.bounds[a] != self.bounds[target]:
                        self.assertIn(target, self.entries)
                        self.assertEqual(record["flow"], "UNCONDITIONAL_CALL")
                        cross.append((a, record["flow"], target))
                        reached.add(target)
        self.assertEqual((len(incoming), len(cross)), (619, 1240))
        self.assertEqual(self.entries - reached, {0x102238a0, 0x102270f0})

    def test_complete_cfg_has_no_external_tail_or_fallthrough_resumes(self):
        for pc, record in self.records.items():
            if record.get("fallthrough") is not None:
                target = record["fallthrough"]
                self.assertIn(target, self.records)
                self.assertEqual(self.bounds[pc], self.bounds[target])
            for target in record["flows"]:
                if not audit.MODULE[0] <= target < audit.MODULE[1]:
                    self.assertTrue(record["flow"].endswith("CALL"))

    def test_transition_vectors_and_symbols_close_remaining_entries(self):
        found = []
        for base, data in self.segments:
            for offset in range(0, len(data) - 7, 4):
                target, toc = struct.unpack_from(">II", data, offset)
                if audit.MODULE[0] <= target < audit.MODULE[1] and toc == 0x108bba90:
                    found.append((base + offset, target))
        self.assertEqual(tuple(found), audit.TRANSITION_VECTORS)
        self.assertEqual([s for s in self.program["symbols"]
                          if audit.MODULE[0] <= s["address"] < audit.MODULE[1]], [])

    def test_all_computed_jumps_are_bounded_local_switch_tables(self):
        self.assertEqual({r["address"] for r in self.records.values()
                          if r["flow"].startswith("COMPUTED")}, {a for a, *_ in audit.SWITCH_TABLES})
        table_words = set()
        for site, slot, table, count, pinned in audit.SWITCH_TABLES:
            self.assertEqual(self.records[site]["assembly"], "bctr")
            self.assertEqual(self.word(0x108bba90 + slot), table)
            prefix = [r for pc, r in self.records.items() if site - 120 <= pc < site]
            bound = next(r for r in reversed(prefix) if r["assembly"].startswith("cmplwi"))
            self.assertEqual(int(bound["assembly"].split(",")[-1], 16) + 1, count)
            load = next(r for r in reversed(prefix) if "(r2)" in r["assembly"])
            self.assertIn(f"-0x{-slot:x}(r2)", load["assembly"])
            self.assertEqual(self.records[site - 4]["assembly"].split()[0], "mtspr")
            self.assertEqual(hashlib.sha256(self.original_bytes(table, count * 4)).hexdigest(), pinned)
            for offset in range(count):
                at = table + offset * 4
                self.assertNotIn(at, table_words)
                table_words.add(at)
                target = self.word(at)
                self.assertIn(target, self.records)
                self.assertEqual(self.bounds[site], self.bounds[target])
                self.assertNotIn(target, self.entries)
        self.assertEqual(len(audit.SWITCH_TABLES), 30)
        self.assertEqual(table_words, set(range(0x108cd550, 0x108ce788, 4)))

    def test_raw_pointer_scan_distinguishes_tables_from_false_byte_matches(self):
        pointers = []
        for base, data in self.segments:
            for match in re.finditer(b"\x10[\x22-\x28]", data):
                offset = match.start()
                if offset + 4 > len(data):
                    continue
                target = int.from_bytes(data[offset:offset+4], "big")
                if audit.MODULE[0] <= target < audit.MODULE[1]:
                    pointers.append((base + offset, target))
        self.assertEqual(len(pointers), 3272)
        self.assertEqual(hashlib.sha256(json.dumps(pointers, separators=(",", ":")).encode()).hexdigest(),
                         "cac0f29854eb7f5abf87576dc7234be7bbbfc664f6d3f7649d85b10643986abb")
        tvs = dict(audit.TRANSITION_VECTORS)
        for at, target in pointers:
            if at < 0x108b3a90:
                # Instruction immediates and floating constants contain byte
                # substrings, never aligned relocated PPC function pointers.
                self.assertNotEqual(at % 4, 0)
            elif at in tvs:
                self.assertEqual(target, tvs[at])
            elif 0x108cd550 <= at < 0x108ce788:
                self.assertEqual(at % 4, 0)
            elif 0x108c98dd <= at <= 0x108c98f9:
                self.assertEqual(at % 4, 1)
                self.assertTrue(0x10100000 <= self.word(at & ~3) < 0x10110000)
                self.assertTrue(0x10100000 <= self.word((at & ~3) + 4) < 0x10110000)
            elif at == 0x108d4b2e:
                self.assertEqual(self.word(0x108d4b2c), 0x10471028)
                self.assertEqual(self.word(0x108d4b30), 0x10471030)
            else:
                # The fixed random halfword pool is data, consumed through
                # TOC -0x7b8c; accidental address-sized substrings are not TVs.
                self.assertTrue(0x108e3140 <= at < 0x108fb7e0, f"Unclassified pointer at {at:x}")
                self.assertEqual(self.word(0x108bba90 - 0x7b8c), 0x108e3140)
                self.assertNotEqual(self.word(at + 4), 0x108bba90)

    def test_promoted_entries_have_unique_normal_production_ownership(self):
        from compile_recomp import OWNER_REPLACEMENTS, SEMANTIC_REPLACEMENTS, MATCH_PL_TAKEOVERS
        audit.require_match_pl_owners(audit.rewritten(), OWNER_REPLACEMENTS)
        self.assertEqual(tuple(a for a, _, _ in audit.rewritten()), audit.MATCH_PL_ENTRIES)
        self.assertEqual(MATCH_PL_TAKEOVERS, frozenset(
            a for a, _ in SEMANTIC_REPLACEMENTS if a in audit.MATCH_PL_ENTRIES))

    def test_manifest_candidates_replace_only_explicit_legacy_hooks(self):
        import compile_recomp as compiler

        # Drafts are temporarily selected here to exercise the structural
        # takeover machinery before promotion. This is not production evidence
        # for those entries; the separate owner test checks the real manifest.
        candidates = frozenset(a for a, _, _ in audit.rewritten(drafts=True))
        old_owners = dict(compiler.SEMANTIC_REPLACEMENTS)
        taken = candidates.intersection(old_owners)
        families = {}
        for entry in sorted(taken):
            family = old_owners[entry].removesuffix("_adapter")
            families.setdefault(family, []).append(entry)
        self.assertTrue(families, "No legacy-owner candidate exercises takeover")
        disabled = {name: False for name in inspect.signature(compiler.generate_page).parameters
                    if name.endswith("_hooks") or name == "guest_context_transfers"}
        with closing(sqlite3.connect(INPUT / "instructions.sqlite")) as db:
            pages = {entry >> 12 for entry in taken}
            records = {page: [json.loads(body) for body, in db.execute(
                "SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address",
                (page << 12, (page + 1) << 12))] for page in pages}
        with patch.object(compiler, "MATCH_PL_ENTRIES", tuple(sorted(candidates))), \
                patch.object(compiler, "MATCH_PL_TAKEOVERS", frozenset(taken)):
            for family, entries in sorted(families.items()):
                legacy = importlib.import_module(family + "_audit")
                require_hooks = getattr(legacy, f"require_{family}_hooks")
                old_hook = getattr(legacy, family + "_hook")
                for page in sorted({entry >> 12 for entry in entries}):
                    with self.subTest(family=family, page=hex(page)):
                        rows = records[page]
                        selected = {r["address"] for r in rows}
                        options = dict(disabled, **{family + "_hooks": True, "match_pl_hooks": True})
                        source = compiler.generate_page(page, rows, selected,
                            userops=self.program["userops"], **options)
                        require_hooks(page, rows, source, taken)
                        # Omitting the explicit exemption must still reject
                        # a missing legacy hook; do not loosen its oracle.
                        with self.assertRaises(ValueError):
                            require_hooks(page, rows, source)
                        for entry in entries:
                            if entry >> 12 != page:
                                continue
                            self.assertNotIn(old_hook(entry), source)
                            self.assertEqual(source.count(audit.match_pl_hook(entry)), 1)
                            fragment = source.split(f"i_{entry:x}:", 1)[1].split(f"i_{entry+4:x}:", 1)[0]
                            self.assertEqual(re.findall(r"try_modern_(\w+)\(s,", fragment), ["match_pl"])

                        # Normal generation disables several legacy match
                        # families. The new route must stand independently.
                        promoted_only = compiler.generate_page(page, rows, selected,
                            userops=self.program["userops"], **dict(disabled, match_pl_hooks=True))
                        for entry in entries:
                            if entry >> 12 == page:
                                self.assertNotIn(old_hook(entry), promoted_only)
                                self.assertEqual(promoted_only.count(audit.match_pl_hook(entry)), 1)

                        options["match_pl_hooks"] = False
                        isolated = compiler.generate_page(page, rows, selected,
                            userops=self.program["userops"], **options)
                        require_hooks(page, rows, isolated)
                        self.assertNotIn("try_modern_match_pl(", isolated)
                        for entry in entries:
                            if entry >> 12 == page:
                                self.assertEqual(isolated.count(old_hook(entry)), 1)

                        reference = compiler.generate_page(page, rows, selected,
                            userops=self.program["userops"], **disabled)
                        self.assertNotIn("try_modern_", reference)

    def test_missing_duplicate_and_wrong_production_owner_are_rejected(self):
        entry, end, _, pinned = audit.CONTRACTS[0]
        routines = [(entry, end, pinned)]
        audit.require_match_pl_owners(routines, [(entry, "match_pl_adapter")])
        for owners in ([], [(entry, "actor_construction_adapter")],
                       [(entry, "match_pl_adapter"), (entry, "match_pl_adapter")]):
            with self.assertRaisesRegex(ValueError, "production ownership"):
                audit.require_match_pl_owners(routines, owners)

    def test_partial_manifest_drafts_and_duplicate_rejection(self):
        entry, end, _, pinned = audit.CONTRACTS[0]
        line = f'MATCH_PL_ROUTINE(0x{entry:x}, 0x{end:x}, R3, "{pinned}", hook)\n'
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "match_pl_routines.inc"
            path.write_text(line)
            self.assertEqual(audit.rewritten(path), [(entry, end, pinned)])
            path.write_text(line.replace("_ROUTINE(", "_DRAFT("))
            self.assertEqual(audit.rewritten(path), [])
            self.assertEqual(audit.rewritten(path, drafts=True), [(entry, end, pinned)])
            path.write_text(line + line)
            with self.assertRaisesRegex(ValueError, "twice"):
                audit.rewritten(path)
            path.write_text(line.replace(f"0x{end:x}", f"0x{end+4:x}"))
            with self.assertRaisesRegex(ValueError, "boundary"):
                audit.rewritten(path)

    def test_stale_unimplemented_original_digest_is_rejected(self):
        contracts = list(audit.CONTRACTS)
        promoted = set(audit.MATCH_PL_ENTRIES)
        index = next((i for i, contract in enumerate(contracts) if contract[0] not in promoted), 0)
        entry, end, count, _ = contracts[index]
        contracts[index] = (entry, end, count, "0" * 16)
        with patch.object(audit, "CONTRACTS", tuple(contracts)):
            with closing(sqlite3.connect(INPUT / "instructions.sqlite")) as db:
                with self.assertRaisesRegex(ValueError, "original changed"):
                    audit.require_match_pl_index(db)

    def test_stale_promoted_digest_is_rejected(self):
        entry, end, _, _ = audit.CONTRACTS[0]
        with patch.object(audit, "rewritten", return_value=[(entry, end, "0" * 16)]):
            with closing(sqlite3.connect(INPUT / "instructions.sqlite")) as db:
                with self.assertRaisesRegex(ValueError, "promoted digest"):
                    audit.require_match_pl_index(db)


if __name__ == "__main__":
    unittest.main()
