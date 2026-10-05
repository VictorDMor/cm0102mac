"""Pin the five Mac collection lookups and their generated destination hooks."""
import hashlib
import json


CONTRACTS = (
    (0x1074DD40, 0x1074DD94, "6616dd13a4bb0987aea301ef1223f923e4ba1eb9c6ff19dd4f8be72015e23780"),
    (0x1074DF60, 0x1074DF8C, "2f1b87f82d1b17d062d211efef96e018ca1037a33ecbf7949cc798bdecc1fab1"),
    (0x1074DF90, 0x1074DFB8, "cc4f5a4343d038fa57d92058142c9b659d3d975729e068973098227a5936d5d0"),
    (0x1074E150, 0x1074E5A4, "7d90da28bd1aa7a1b61e53cd33f2b9a4b25040330a8b92df582e341f3f45582c"),
    (0x1074E5B0, 0x1074EA30, "aac339e33d1c6f7e3d8ec5e88d6ce4e915cd9d00317e6f4e21a76b88785882e5"),
)
COLLECTION_LOOKUP_ENTRIES = tuple(start for start, _, _ in CONTRACTS)
INSTRUCTION_COUNTS = (21, 11, 10, 277, 288)


def require_collection_lookup_contract(page, instructions):
    """Reject changed or partial lookup bodies before emitting their hooks.

    Each body fits in one page. Bounded exports containing only unrelated
    instructions need no lookup body; every included body must match in full.
    """
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
            raise ValueError(f"Mac collection lookup contract changed at {start:x}")
