"""Pin the three Mac RNG core bodies; consumers remain generated."""
import hashlib
import json

CONTRACTS = (
    (0x1077B770, 0x1077B790, "947d4acb79bb75eb056f1720faf3b72a658c702e3390df399e637ed8b3b54760"),
    (0x1077B790, 0x1077B798, "b5ea8440c2bbee7a391956377ff65b3816799c5b73fe78852d08bb6671eb0cb8"),
    (0x107474A0, 0x107475AC, "b189ae6daae4a6fc7170607de6a91f982df7bba630bf562ad1792497003203db"),
)
RNG_CORE_ENTRIES = tuple(start for start, _, _ in CONTRACTS)


def require_rng_core_contract(page, instructions):
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
            raise ValueError(f"Mac RNG core contract changed at {start:x}")
