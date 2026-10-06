"""Pinned original Mac block-memory bodies; no proprietary listings."""
import hashlib
import json

CONTRACTS = (
    (0x10739ef0, 0x10739f2c, "05753a5ef9136a3145548dc38d58233103e180dba5e5bd108b49975c7f3f80cc"),
    (0x10778300, 0x10778330, "6b9670af9fecd70c81015ca9a73e470266ec3eb4ff3467391ac3ac859f5db50f"),
    (0x10778330, 0x10778360, "8da31292109a0dec94712af7b85a3179ffeffd3002d8a706600ae37c4fe56b12"),
    (0x10778360, 0x10778390, "32b1246d369b4edcef213b8d941f0bad79142c76d12188af939051d43a4c187b"),
    (0x10778390, 0x107783e0, "f95bcb514d99050544dfba105b2be24722695b33bb32e80c6fc60580e90be7f0"),
    (0x107783e0, 0x107784a0, "0a98cb1d92b0f33ccdd4204b7b7ec3cce64d573dc1bb2c1453dea7e9a59a1a4e"),
)
MEMORY_PRIMITIVES_ENTRIES = tuple(start for start, _, _ in CONTRACTS)
INSTRUCTION_COUNTS = (15, 12, 12, 12, 20, 48)


def require_memory_primitives_contract(page, instructions):
    """Reject changed, partial or duplicated included original bodies."""
    for start, end, expected in CONTRACTS:
        if start >> 12 != page:
            continue
        records = [{key: item.get(key) for key in
                    ("address", "assembly", "ops", "flows", "fallthrough")}
                   for item in instructions if start <= item["address"] < end]
        if not records:
            continue
        actual = hashlib.sha256(json.dumps(records, sort_keys=True, separators=(",", ":")).encode()).hexdigest()
        if actual != expected:
            raise ValueError(f"Mac memory primitives contract changed at {start:x}")
