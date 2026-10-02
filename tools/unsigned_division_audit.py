"""Fail-closed source guard for the direct unsigned 64-bit Mac division helper."""
import hashlib
import json


def require_unsigned_division_routine(instructions, selected):
    """Pin the entire original routine, including both returns and every branch.

    This fingerprint covers all 59 instruction records from the audited local
    Mac export, including all P-code inputs, outputs, widths, offsets, targets,
    assembly, sizes, and flow metadata. It is not regenerated from new input.
    Any changed original export requires a fresh semantic and oracle audit.
    """
    addresses = list(range(0x10773870, 0x1077395C, 4))
    records = sorted((item for item in instructions
                      if 0x10773870 <= item.get("address", -1) < 0x1077395C),
                     key=lambda item: item["address"])
    if [item["address"] for item in records] != addresses or not set(addresses) <= selected:
        raise ValueError("Missing or ambiguous original Mac unsigned division instruction")
    fields = ("address", "size", "assembly", "flow", "fallthrough", "flows", "ops")
    canonical = [{key: item[key] for key in fields if key in item} for item in records]
    fingerprint = hashlib.sha256(json.dumps(canonical, sort_keys=True,
                                           separators=(",", ":")).encode()).hexdigest()
    if fingerprint != "0278cee977f16957c67f40073d061820cc8a6cc6e5b070a3c1f144252a9bd0a7":
        raise ValueError("Mac unsigned division no longer matches the audited original routine")
