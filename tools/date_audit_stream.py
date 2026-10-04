"""Validate private CMDAT001 framing and summarize actual entry boundaries."""
import gzip
import lzma
from contextlib import ExitStack
from collections import Counter
from calendar_audit import DATE_SUBSYSTEM_ENTRIES


def boundaries(path):
    counts = Counter()
    first = {}
    stack = []
    next_index = 0
    with ExitStack() as resources:
        if str(path).endswith(".xz"):
            raw = resources.enter_context(lzma.open(path, "rb"))
            source = resources.enter_context(gzip.GzipFile(fileobj=raw, mode="rb"))
        else:
            source = resources.enter_context(gzip.open(path, "rb"))
        if source.read(8) != b"CMDAT001":
            raise ValueError("Unrecognized private Date stream")
        while prefix := source.read(21):
            if len(prefix) != 21:
                raise ValueError("Truncated private Date header")
            kind = prefix[0]
            entry = int.from_bytes(prefix[1:5], "big")
            index = int.from_bytes(prefix[5:13], "big")
            size = int.from_bytes(prefix[13:21], "big")
            if entry not in DATE_SUBSYSTEM_ENTRIES or size > 128 * 1024 * 1024:
                raise ValueError("Invalid private Date entry or body size")
            if kind == 1:
                if index != next_index:
                    raise ValueError("Invalid private Date begin sequence")
                next_index += 1
                counts[entry] += 1
                first.setdefault(entry, index)
                stack.append((entry, index))
            elif kind == 2:
                if not stack or stack.pop() != (entry, index):
                    raise ValueError("Invalid private Date nesting")
            else:
                raise ValueError("Invalid private Date chunk kind")
            remaining = size
            while remaining:
                block = source.read(min(remaining, 1024 * 1024))
                if not block:
                    raise ValueError("Truncated private Date body")
                remaining -= len(block)
        if stack:
            raise ValueError("Unconsumed private Date begin boundary")
    return {f"0x{entry:08x}": {"calls": counts[entry], "firstIndex": first.get(entry)}
            for entry in DATE_SUBSYSTEM_ENTRIES}
