"""Inventory and exclusive ownership checks for CONTRACT_SCREENS.CPP."""
import json
import re
import sqlite3
import struct
import unittest

import contract_screens_audit as audit
from compile_recomp import OWNER_REPLACEMENTS, SEMANTIC_REPLACEMENTS
from tactics_audit import routine_records

EXPORT = audit.ROOT / ".local/current/export"
GENERATED = audit.ROOT / ".local/current/generated"
TOC = 0x108bba90
VECTORS = (
    (0x108b892c, 0x1009ed20),
    (0x108b8934, 0x1009eb30),
    (0x108b893c, 0x1009e8f0),
    (0x108b8944, 0x1009e290),
    (0x108b894c, 0x1009dc00),
    (0x108b8954, 0x1009ee00),
    (0x108b895c, 0x100980d0),
    (0x108b8964, 0x10097b70),
    (0x108b896c, 0x10096c60),
    (0x108b8974, 0x10095db0),
    (0x108b897c, 0x100952b0),
    (0x108b8984, 0x10093ae0),
    (0x108b898c, 0x10093e60),
    (0x108b8994, 0x10093bd0),
    (0x108b899c, 0x10093a50),
)
SWITCH_TABLE = 0x108c873c
SWITCH_TARGETS = (
    0x1009a654, 0x1009a7a8, 0x1009a89c, 0x1009a8ac, 0x1009a624, 0x1009ad90,
    0x1009af34, 0x1009b1f4, 0x1009b5f8, 0x1009b5f8, 0x1009b5f8, 0x1009b5f8,
    0x1009b5f8, 0x1009b5f8, 0x1009b5f8, 0x1009b5f8, 0x1009b5f8, 0x1009b5f8,
    0x1009a91c, 0x1009a93c, 0x1009aa90, 0x1009a8dc, 0x1009b244, 0x1009b5f8,
    0x1009b2bc, 0x1009abdc, 0x1009b4bc, 0x1009b4e8, 0x1009b5f8, 0x1009b5f8,
    0x1009b5f8, 0x1009b5f8, 0x1009b5f8, 0x1009b5f8, 0x1009b5f8, 0x1009b5f8,
    0x1009b5f8, 0x1009b5f8, 0x1009b5f8, 0x1009b5f8, 0x1009b5f8, 0x1009b5f8,
    0x1009b5f8, 0x1009b5f8, 0x1009b280,
)
ENTRY_CALLS = (
    (0x10093abc, 0x10094ae0),
    (0x10094158, 0x10094560),
    (0x100942b0, 0x10094560),
    (0x1009493c, 0x10094560),
    (0x10094d5c, 0x100965c0),
    (0x10094d74, 0x100960d0),
    (0x10094db0, 0x100965c0),
    (0x10094dc8, 0x100960d0),
    (0x10094e98, 0x100965c0),
    (0x10094eb4, 0x100960d0),
    (0x10096068, 0x100965c0),
    (0x10096080, 0x100960d0),
    (0x100976f4, 0x10098130),
    (0x10097710, 0x1009c870),
    (0x1009772c, 0x1009b620),
    (0x10097e3c, 0x1009dab0),
    (0x1009803c, 0x1009dab0),
    (0x1009804c, 0x1009a440),
    (0x10098060, 0x1009c020),
    (0x10098074, 0x1009d410),
    (0x1009edbc, 0x10094ae0),
    (0x1009eddc, 0x10094ae0),
    (0x103353b0, 0x1009ef90),
    (0x10342224, 0x1009ef90),
    (0x103424b0, 0x1009ef90),
    (0x103437f4, 0x1009ef90),
    (0x10346df0, 0x1009ef30),
    (0x1034bc98, 0x1009ef60),
    (0x1034bcac, 0x1009ef60),
    (0x103a7664, 0x1009e880),
    (0x103a7fec, 0x10094ae0),
    (0x103a85a4, 0x10094ae0),
    (0x103a87bc, 0x10094ae0),
    (0x103a95a4, 0x1009e880),
    (0x103a9f2c, 0x10094ae0),
    (0x103aa2c8, 0x10094ae0),
    (0x103aa4e0, 0x10094ae0),
    (0x10421cb8, 0x10094ae0),
)
DIRECT_BRANCH = re.compile(r"(b\w*) (?:cr\d,)?0x([0-9a-f]+)$")


class ContractScreensAudit(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.db = sqlite3.connect((EXPORT / "instructions.sqlite").resolve().as_uri() + "?mode=ro", uri=True)
        program = json.loads((EXPORT / "program.json").read_text())
        cls.segments = [(segment["address"], (EXPORT / segment["file"]).read_bytes())
                        for segment in program["segments"] if segment["initialized"]]

    @classmethod
    def tearDownClass(cls):
        cls.db.close()

    def word(self, address):
        for base, data in self.segments:
            if base <= address < base + len(data):
                return struct.unpack_from(">I", data, address - base)[0]
        self.fail(f"Unmapped export word {address:#x}")

    def text(self, address):
        for base, data in self.segments:
            if base <= address < base + len(data):
                offset = address - base
                return data[offset:data.index(b"\0", offset)]
        self.fail(f"Unmapped export string {address:#x}")

    def assembly(self, address):
        return routine_records(self.db, address, address + 4)[0]["assembly"]

    def test_entries_and_digests(self):
        audit.require_contract_screens_index(self.db)
        self.assertEqual(len(audit.CONTRACTS), 30)
        self.assertEqual(sum(count for _, _, count, _ in audit.CONTRACTS), 11549)
        previous_end = audit.MODULE[0]
        for entry, end, _, _ in audit.CONTRACTS:
            self.assertLessEqual(previous_end, entry)
            self.assertEqual(self.assembly(entry), "mfspr r0,LR")
            self.assertEqual(self.assembly(end - 4), "blr")
            previous_end = end
        self.assertLessEqual(previous_end, audit.MODULE[1])

    def test_source_and_neighbors(self):
        self.assertEqual(self.assembly(0x100939b0), "mfspr r0,LR")
        self.assertEqual(self.assembly(0x10093a48), "blr")
        self.assertEqual(self.word(0x10093a4c), 0)
        self.assertEqual(self.assembly(audit.MODULE[1]), "mfspr r0,LR")
        self.assertEqual(self.text(self.word(TOC - 0x56cc) + 0x87), b"CONTRACT_SCREENS.CPP")
        self.assertEqual(self.text(self.word(TOC - 0x5618)), b"DATABASE.CPP")
        references = [address for address, in self.db.execute(
            "SELECT address FROM instructions WHERE body LIKE '%-0x56cc(r2)%' ORDER BY address")]
        self.assertEqual(len(references), 12)
        self.assertTrue(all(audit.MODULE[0] <= address < audit.MODULE[1] for address in references))

    def test_direct_edges(self):
        entries = {entry for entry, _, _, _ in audit.CONTRACTS}
        calls = []
        for address, body in self.db.execute(
                "SELECT address, body FROM instructions WHERE body LIKE '%0x1009%' ORDER BY address"):
            branch = DIRECT_BRANCH.fullmatch(json.loads(body)["assembly"])
            if not branch:
                continue
            mnemonic, target_text = branch.groups()
            target = int(target_text, 16)
            if not audit.MODULE[0] <= target < audit.MODULE[1]:
                continue
            if mnemonic == "bl" or not audit.MODULE[0] <= address < audit.MODULE[1]:
                self.assertEqual(mnemonic, "bl")
                self.assertIn(target, entries)
                calls.append((address, target))
        self.assertEqual(tuple(calls), ENTRY_CALLS)
        self.assertEqual(entries, {target for _, target in calls} | {target for _, target in VECTORS})

    def test_vectors_and_switch_labels(self):
        pointers = {base + index * 4: target for base, data in self.segments
                    for index, (target,) in enumerate(struct.iter_unpack(">I", data[:len(data) & ~3]))
                    if audit.MODULE[0] <= target < audit.MODULE[1]}
        expected = dict(VECTORS)
        expected.update({SWITCH_TABLE + 4 * i: target for i, target in enumerate(SWITCH_TARGETS)})
        self.assertEqual(pointers, expected)
        self.assertTrue(all(self.word(address + 4) == TOC for address, _ in VECTORS))
        self.assertEqual(self.word(TOC - 0x56f8), SWITCH_TABLE)
        self.assertEqual(self.assembly(0x1009a620), "bctr")
        self.assertTrue(all(0x1009a440 < target < 0x1009b614 for target in SWITCH_TARGETS))

    def test_padding_and_unreachable_nops(self):
        addresses = {address for address, in self.db.execute(
            "SELECT address FROM instructions WHERE address>=? AND address<?", audit.MODULE)}
        gaps = {address for address in range(*audit.MODULE, 4) if address not in addresses}
        self.assertEqual(len(gaps), 63)
        nonzero = {address for address in gaps if self.word(address)}
        self.assertEqual(nonzero, audit.UNREACHABLE_NOPS)
        for address in nonzero:
            self.assertEqual(self.word(address), 0x60000000)
            self.assertTrue(self.assembly(address - 4).startswith("b "))

    def test_production_owners(self):
        entries = set(audit.CONTRACT_SCREENS_ENTRIES)
        self.assertTrue(entries)
        bounds = {entry for entry, _, _, _ in audit.CONTRACTS}
        self.assertTrue(entries <= bounds)
        self.assertTrue(bounds.isdisjoint(dict(SEMANTIC_REPLACEMENTS)))
        self.assertEqual({entry for entry, owner in OWNER_REPLACEMENTS
                          if owner == "contract_screens_adapter"}, entries)
        for entry in entries:
            page = entry >> 12
            code = (GENERATED / f"page_{page:x}.cpp").read_text()
            audit.require_contract_screens_hooks(page, [{"address": entry}], code)


if __name__ == "__main__":
    unittest.main()
