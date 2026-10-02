"""Fail closed when the original Mac commentary wait contract changes."""
from collections import defaultdict
import hashlib
import json


def require_match_presentation_context(page, instructions, selected):
    # Canonical original instruction exports, including complete P-code, widths,
    # register offsets, calls and flow. The caller establishes speed at +0x4070;
    # the whole callee pins timestamp comparison, sound calls, and -1/-2 states.
    contracts = {
        0x101cc: (0x101cce28, 0x101cce70, (),
                  "2ccde13b3d2a32ce4e1f941f560f3df466e6daca1227f0eb6d057e311f54de2d"),
        0x10203: (0x102036f0, 0x102037f4, (0x10203720,),
                  "a4368617e3ed246e6e89b259a5d5bd4f38f649f2e4d9f52e36d49a714c39163d"),
    }
    if page not in contracts:
        return
    first, last, padding, expected = contracts[page]
    addresses = set(range(first, last + 1, 4)) - set(padding)
    records = defaultdict(list)
    for item in instructions:
        if item.get("address") in addresses:
            records[item["address"]].append(item)
    for address in addresses:
        if len(records[address]) != 1 or address not in selected:
            raise ValueError(f"Missing or ambiguous Mac commentary instruction at {address:x}")
    fields = ("address", "size", "assembly", "flow", "fallthrough", "flows", "ops")
    canonical = [{key: records[address][0].get(key) for key in fields}
                 for address in sorted(addresses)]
    actual = hashlib.sha256(json.dumps(canonical, sort_keys=True,
                                      separators=(",", ":")).encode()).hexdigest()
    if actual != expected:
        raise ValueError(f"Mac commentary contract no longer matches audited page {page:x}")
