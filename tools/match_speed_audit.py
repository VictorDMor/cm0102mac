"""Pin the original speed menu ABI and presentation-only integration sites."""
import hashlib
import json

CONTRACTS = {
    (0x10147ce0, 0x10147d14): "fcd4d472efba0f2f6e28233832e6afdc9b0daf66c4842bfccc59c9fe96909c1e",
    (0x10147e38, 0x10147e6c): "db52b8006f8ce65372229974c2b3f24149ff3f703fa30c5321297a94c2af31b2",
    (0x101fef60, 0x101fefcc): "040f0036a4f2608b13cbd2b33897e0248d637d5b38216105a2d979fa99032ba7",
    (0x101cce44, 0x101cce70): "c05bba19315e9af2f498c2e5d98a1490bc82468b641bebb673cad4a6af8ef271",
    (0x10203574, 0x10203574): "3f136e42ccecebc40aa543738d42716a9e0f74a4c566eace057235c3f7bc756c",
    (0x102036ac, 0x102036ac): "eb5ec2d8124dc7d7c2edc8dac3214c1d06687ea5199850279749af3dd873b720",
    (0x1028d438, 0x1028d678): "356d1b85dd685da50b6acb764f77fba0c7dca043b376905d31a37c69cf34698b",
}
MATCH_SPEED_PAGES = {first >> 12 for first, _ in CONTRACTS}
ANCHORS = (0x10147cf8, 0x10147e50, 0x101fefc8, 0x101cce50,
           0x10203574, 0x102036ac, 0x1028d558)

def require_match_speed_context(page, instructions, selected):
    records = {row["address"]: row for row in instructions}
    fields = ("address", "size", "assembly", "flow", "fallthrough", "flows", "ops")
    for ((first, last), expected), anchor in zip(CONTRACTS.items(), ANCHORS):
        if first >> 12 != page or (anchor not in records and anchor not in selected):
            continue
        addresses = range(first, last + 1, 4)
        if any(a not in records or a not in selected for a in addresses):
            raise ValueError("Incomplete original match-speed integration contract")
        if any(sum(row["address"] == a for row in instructions) != 1 for a in addresses):
            raise ValueError("Duplicate original match-speed integration instruction")
        canonical = [{key: records[a].get(key) for key in fields}
                     for a in range(first, last + 1, 4)]
        actual = hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(",", ":")).encode()).hexdigest()
        if actual != expected:
            raise ValueError("Original match-speed integration contract changed")
