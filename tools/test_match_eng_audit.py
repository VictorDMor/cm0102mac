"""Certify MATCH_ENG.CPP's entries, boundaries, flows and production owner.

The private export is required evidence, rather than a reason to skip tests.
"""
from contextlib import closing
import json
from pathlib import Path
import re
import sqlite3
import unittest

import match_eng_audit as audit
from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
EXPORT = ROOT / ".local/current/export"


def require_export():
    if not (EXPORT / "instructions.sqlite").is_file():
        raise RuntimeError("Private MATCH_ENG export unavailable; inventory cannot be certified")


class MatchEngAudit(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        require_export()
        cls.db = sqlite3.connect(EXPORT / "instructions.sqlite")

    @classmethod
    def tearDownClass(cls):
        cls.db.close()

    def test_inventory_is_contiguous_and_pinned(self):
        audit.require_match_eng_index(self.db)
        self.assertEqual(len(audit.CONTRACTS), 235)
        self.assertEqual(sum(count for _, _, count, _ in audit.CONTRACTS), 46271)
        self.assertEqual(audit.CONTRACTS[0][0], audit.MODULE[0])
        self.assertEqual(audit.CONTRACTS[-1][1], audit.MODULE[1])
        for (_, end, _, _), (start, _, _, _) in zip(audit.CONTRACTS, audit.CONTRACTS[1:]):
            self.assertEqual(end, start)

    def test_entries_are_aligned_for_the_adapter_table(self):
        self.assertTrue(all(entry % 16 == 0 for entry in audit.MATCH_ENG_ENTRIES))

    def test_no_flow_crosses_a_routine_boundary(self):
        audit.require_match_eng_flows(self.db)

    def test_data_pointers_are_the_switch_tables_and_the_vector(self):
        audit.require_match_eng_data(EXPORT)
        self.assertEqual(len(audit.SWITCH_TABLES), 7)
        self.assertEqual(sum(count for *_, count, _ in audit.SWITCH_TABLES), 145)

    def _pool_loads(self, slot):
        pattern = re.compile(rf"lwz r\d+,{slot}\(r2\)")
        return [address for address, assembly in self.db.execute(
            "SELECT address,json_extract(body,'$.assembly') FROM instructions WHERE address>=? AND address<?",
            (0x101c0000, 0x10210000)) if pattern.fullmatch(assembly)]

    def test_source_pools_bound_the_module(self):
        low, high = audit.MODULE
        own = self._pool_loads("-0x52cc")
        self.assertEqual((own[0], own[-1]), (0x101d1798, 0x101fd6d0))
        self.assertTrue(all(low <= address < high for address in own))
        self.assertTrue(all(address < low for address in self._pool_loads("-0x52f0") if address < own[0]))
        following = self._pool_loads("-0x52a0")
        self.assertEqual(following[0], 0x101fef88)
        self.assertGreaterEqual(following[0], high)

    def test_constructor_and_next_module_constructor_shape(self):
        # 0x101d16a0 builds this object's arrays and clears +0x3114; 0x101fef60 is
        # MATCH_EVENTS' first routine, the first to load its pool.
        ctor = routine_records(self.db, 0x101d16a0, 0x101d1760)
        text = [r["assembly"] for r in ctor]
        self.assertIn("addi r3,r3,0x311c", text)
        self.assertIn("stw r0,0x3114(r3)", text)

    def test_routine_lists_match_the_inventory(self):
        for entry, end, pinned in audit.rewritten(drafts=True):
            self.assertEqual(dict((a, (b, d)) for a, b, _, d in audit.CONTRACTS)[entry], (end, pinned))
        for entry in audit.MATCH_ENG_LEAVES:
            self.assertIn(entry, audit.MATCH_ENG_ENTRIES)

    def test_every_entry_is_owned_in_production(self):
        import compile_recomp
        audit.require_match_eng_owners(compile_recomp.OWNER_REPLACEMENTS)

    def test_addresses_in_the_cpp_name_what_they_should(self):
        # A wrong return address leaves a wrong word in the stack image, and only a callee that
        # reads it would show. A literal is an entry, a bl, what follows a bl, a fused instruction
        # (multiply-add sites) or a PC the debugger's boundary watches.
        low, high = audit.MODULE
        assembly = dict(self.db.execute(
            "select address, json_extract(body, '$.assembly') from instructions where address >= ? and address < ?", (low, high)))
        entries = set(audit.MATCH_ENG_ENTRIES)
        is_call = lambda address: (assembly.get(address) or "").startswith("bl ")
        fused = re.compile(r"fn?m(add|sub)s? ")
        wrong = []
        for path in sorted((ROOT / "native").glob("match_eng_*")):
            if path.suffix not in (".cpp", ".hpp") or "checks" in path.name:
                continue
            source = re.sub(r"//[^\n]*", "", path.read_text())
            for found in re.finditer(r"(s\.pc = )?0x(101[d-f][0-9a-f]{4})\b", source):
                address = int(found.group(2), 16)
                if not low <= address < high or address in entries or found.group(1):
                    continue
                if not (is_call(address) or is_call(address - 4) or fused.match(assembly.get(address) or "")):
                    wrong.append(f"{path.name}:{source.count(chr(10), 0, found.start()) + 1} {hex(address)} {assembly.get(address)}")
        self.assertEqual(wrong, [])

    def test_a_changed_digest_fails(self):
        entry, end, count, pinned = audit.CONTRACTS[0]
        records = routine_records(self.db, entry, end)
        records[0]["assembly"] = "nop"
        self.assertNotEqual(digest(records), pinned)


if __name__ == "__main__":
    unittest.main()
