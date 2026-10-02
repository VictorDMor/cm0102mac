"""Pin the Mac calendar instruction contract used by the semantic adapter."""
import hashlib
import json

CONTRACTS = (
    (0x103317F0, 0x10331858, "e93ed9feee78922e44a69beafbc099f515429b887d7ead794132f8bb59a1f18b"),
    (0x10331860, 0x103318C8, "5a74c4775a352a92e713d648900ade2eae03254c96bb84200d5b2378f24b29d2"),
    (0x10718130, 0x107182F0, "caf9175e8cf1dcf26a7cdd61e644ab9f38576ef2516f3208d1938e36b42fb30b"),
    (0x10717890, 0x10717A0C, "eece70ad391c75b1d55fa58bfbdac11cf92dbeb51fcd619c426b53872eea3021"),
    (0x10717B20, 0x10717CA8, "1d4bea0c9cdbac143a0d55b455d8f3f485a41f1774eb9d1115f328ff626af838"),
    (0x10718440, 0x107184CC, "3cd3bf960d288bf41c7d004205b57d0399a315382c3d7f97822f5b431e1d83e5"),
)


def require_calendar_contract(page, instructions):
    for start, end, fingerprint in CONTRACTS:
        if start >> 12 != page:
            continue
        records = [{key: item.get(key) for key in ("address", "assembly", "ops", "flows", "fallthrough")}
                   for item in instructions if start <= item["address"] < end]
        # Bounded unrelated exports need not include calendar routines. Any
        # included fragment requires the whole audited body and exact P-code.
        if not records:
            continue
        actual = hashlib.sha256(json.dumps(records, sort_keys=True, separators=(",", ":")).encode()).hexdigest()
        if actual != fingerprint:
            raise ValueError(f"Mac calendar contract changed at {start:x}")
