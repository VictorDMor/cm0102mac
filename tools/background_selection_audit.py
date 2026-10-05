"""Pin the four Mac background selectors; their accessors remain generated."""
import hashlib
import json


CONTRACTS = (
    (0x107122E0, 0x107124E0, "1a92a426b5789bed300658ff087bfcebd05ffd229e192c3c9b97a84da5b0578b"),
    (0x10712A60, 0x10712C4C, "074912d3cb6b49880517225d7dd83dfd5c0d8a22250def5792f745fb8cf368b8"),
    (0x10713960, 0x107139B4, "1efc413b607f6cc6c32cd45329b54d34b846e968d5f495aeca2b8e6f42bd85a7"),
    (0x107139C0, 0x10713A74, "c206b64c4f3b65e30292264dd4a3ea84ad30b8a794315e2e659ed496a2ed7435"),
)
BACKGROUND_SELECTION_ENTRIES = tuple(start for start, _, _ in CONTRACTS)


def require_background_selection_contract(page, instructions):
    """An included selector requires its complete, unchanged original body.

    Bounded exports containing only unrelated instructions need no selector
    body. Every selector fits in one page; no partial intersection is valid.
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
            raise ValueError(f"Mac background selection contract changed at {start:x}")
