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

# The adapter records exact fault PCs through table-driven read/write helpers.
# Keep each corresponding original instruction observable with block accounting.
BACKGROUND_SELECTION_ACCESS_INSTRUCTIONS = frozenset({
    0x107122E4, 0x107122EC, 0x107122F0, 0x107122F8, 0x107122FC, 0x1071230C,
    0x10712330, 0x10712334, 0x10712338, 0x10712344, 0x1071234C, 0x10712350,
    0x1071235C, 0x10712458, 0x10712460, 0x10712464, 0x107124A0, 0x107124CC,
    0x107124D8, 0x10712A64, 0x10712A68, 0x10712A6C, 0x10712A70, 0x10712A74,
    0x10712A78, 0x10712A80, 0x10712A84, 0x10712A94, 0x10712AB8, 0x10712ABC,
    0x10712AC0, 0x10712ACC, 0x10712AD4, 0x10712AD8, 0x10712AE4, 0x10712BE0,
    0x10712BE8, 0x10712BEC, 0x10712C20, 0x10712C2C, 0x10712C38, 0x10712C3C,
    0x10712C40, 0x10712C44, 0x10713964, 0x10713968, 0x1071396C, 0x10713980,
    0x10713988, 0x107139A4, 0x107139C4, 0x107139CC, 0x107139D0, 0x107139D4,
    0x107139EC, 0x107139F4, 0x10713A34, 0x10713A60, 0x10713A6C,
})


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
