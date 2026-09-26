#!/usr/bin/env python3
"""Deterministic ABI cases for actual routines in the supplied Mac executable."""
import json
from pathlib import Path
import random
import struct
import sys

cases = []


def add(name, entry, initial, expected, memory=False):
    cases.append({"name": name, "entry": entry,
                  "initialRegisters": {k: f"{v:x}" for k, v in initial.items()},
                  "expectedABI": {k: f"{v:x}" for k, v in expected.items()},
                  "checkStack": memory})


add("stack-chain-initialization", 0x107736f0, {"r3": 0x12345678}, {"r3": 0}, True)
add("get-table-of-contents", 0x10773700, {}, {"r3": 0x108bba90})

values = [0, 1, 2, 31, 32, 33, 0xffff, 0xffffffff, 0x100000000,
          0x7fffffffffffffff, 0x8000000000000000, 0xffffffffffffffff]
pairs = [(a, b) for a in values for b in values if b]
rng = random.Random(0x0102)
pairs += [(rng.getrandbits(64), rng.getrandbits(64) or 1) for _ in range(80)]
for i, (a, b) in enumerate(pairs):
    q = a // b
    add(f"unsigned-divide-{i}", 0x10773870,
        {"r3": a >> 32, "r4": a & 0xffffffff, "r5": b >> 32, "r6": b & 0xffffffff},
        {"r3": q >> 32, "r4": q & 0xffffffff})

for i, (a, b) in enumerate([(-a if i & 1 else a, -b if i & 2 else b)
                             for i, (a, b) in enumerate(pairs[:80])]):
    # Signed ABI cases must fit int64 and exclude its overflowing quotient.
    if not -(1 << 63) <= a < (1 << 63) or not -(1 << 63) <= b < (1 << 63): continue
    q = (abs(a) // abs(b)) * (-1 if (a < 0) != (b < 0) else 1)
    if q >= 1 << 63: continue
    add(f"signed-divide-{i}", 0x10773960,
        {"r3": (a >> 32) & 0xffffffff, "r4": a & 0xffffffff,
         "r5": (b >> 32) & 0xffffffff, "r6": b & 0xffffffff},
        {"r3": (q >> 32) & 0xffffffff, "r4": q & 0xffffffff}, True)

for i, value in enumerate([-1.0, -0.0, 0.0, 0.9, 1.0, 1.9, 65535.9,
                           2147483647.0, 2147483648.0, 4294967294.0, 4294967295.0,
                           4294967296.0, float("inf"), float("-inf")]):
    bits = struct.unpack(">Q", struct.pack(">d", value))[0]
    result = 0 if value < 0 else 0xffffffff if value >= 4294967296 else int(value)
    add(f"double-to-unsigned-{i}", 0x10773780, {"f1": bits}, {"r3": result}, True)

output = Path(sys.argv[1])
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps({"executableSHA256": "91692f95df17cf851e7c1d6dbdecd0bd0a5809ce1937133e209aee859a6b516b",
                             "cases": cases}, indent=2) + "\n")
print(f"Prepared {len(cases)} original-routine cases")
