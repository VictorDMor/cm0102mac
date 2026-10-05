"""Pin the fourteen Mac basic byte-string bodies, including indexed gaps."""
import hashlib
import json

CONTRACTS = (
    (0x107393f0, 0x107394dc, "6f374bd1dea6768a6421e7051816861b7cbf0c59614b21b308005c6581bec66d"),
    (0x107394e0, 0x107395f0, "c23f8fb6a286136572ec06ecc9eeca532a402f40b68c32b3d067806e79628a53"),
    (0x1074de10, 0x1074de30, "f16b28e9df93c459efaf7236ff6d18a0f15e97eb1126ed662e814e0a6ffe6f03"),
    (0x107395f0, 0x10739640, "ac606d3aa261c47d1fbdaa1f0d5b172bb243fcae4dc0a394155227854fbe39e4"),
    (0x10739640, 0x1073967c, "efa50a960b458ddba3e0d46cac87f6c8058fc784e97abad96bba13fc4006878f"),
    (0x1077dbd0, 0x1077dbec, "d877b190f9567050ea7f43437dff67853054426d5b9841fe83432bfe6680d4fb"),
    (0x1077dbf0, 0x1077dc0c, "774d415f9f2b8f7f1ecddcc18432a8a87aaba3a51506c94eae9dc9b462052838"),
    (0x1077dc10, 0x1077dc54, "b54b65eae1a77dca62311420c12f051c7b5ea5949d047d60287185be2cabe465"),
    (0x1077dc60, 0x1077dc8c, "6dfee147a562bb700896eb5e9f31e620be83e74cbb0b07bb526f32b9f2423a91"),
    (0x1077dc90, 0x1077dcc8, "5717c6b7d0892339f0b3784045f4cc34d6421c7dfbc6f56cb865028ea7c469cc"),
    (0x1077dcd0, 0x1077dd10, "79aca0a48020beeb690115fd1e124bbdcd748aa7c70be9b8501bd6299ad063ae"),
    (0x1077dd10, 0x1077dd44, "5fe3ff62a0b22e9d4c00e4e50c887920c159cbd6af02688617124168cf297b20"),
    (0x1077dd50, 0x1077dd98, "3ac24746630eecd0833c589527c2e401291a2c2ee97bea64f7f156ee8d02425b"),
    (0x1077dda0, 0x1077de0c, "eb0d586f64e7df8aba82ff0f40db56cef3169fe98328dd02254ebdbc96643208"),
)
STRING_CORE_ENTRIES = tuple(start for start, _, _ in CONTRACTS)
INSTRUCTION_COUNTS = (58, 66, 8, 20, 15, 7, 7, 17, 11, 13, 16, 12, 18, 27)


def require_string_core_contract(page, instructions):
    """Reject any changed or partial included body; gaps are intentional."""
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
            raise ValueError(f"Mac string core contract changed at {start:x}")
