"""Promote registers across four audited, bounded record-field leaf routines.

The normal lowering remains the reference and the finite-budget/observer path.
No original instruction listing is stored here. Generated code stays private.
"""
import hashlib
import json
import re

CONTRACTS = {
    0x100c5ab0: (0x100c614c, 424, "6bf1d37bb76669aef2102bd11358255c9fec18373153ded3ff0dea0d8ac9a75a"),
    0x100c6150: (0x100c63b0, 153, "7d437014fbd7f6bcfc8c0302d72aa603482a62d3889c7de03ee25e39ddcaf4d6"),
    0x100c63c0: (0x100c65c0, 129, "540a825c3f24cd43bc1264df23144cd401856d8234ee194add110fc4c5f54e42"),
    0x100c65d0: (0x100c6a00, 269, "88a007b3501db8f618415bb980ba9f4ad9039de73b33eea9391350e5fc884860"),
}
FIELDS = ("address", "size", "assembly", "flow", "fallthrough", "flows", "ops")
# Only the first routine has a back edge: its local signed-byte counter starts
# at zero, increments once and stops at 63. Every complete execution is below
# 2048 original instructions. Other routines are acyclic and contain no calls.
MAX_INSTRUCTIONS = 2048


def fingerprint(records):
    canonical = [{key: row.get(key) for key in FIELDS} for row in records]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(",", ":")).encode()).hexdigest()


def require_leaf(first, records, selected):
    last, count, expected = CONTRACTS[first]
    addresses = [row["address"] for row in records]
    if addresses != list(range(first, last + 1, 4)) or len(records) != count:
        raise ValueError("Incomplete or duplicate native record-field leaf")
    if any(address not in selected for address in addresses) or fingerprint(records) != expected:
        raise ValueError("Native record-field leaf contract changed or is unselected")
    if records[-1]["assembly"] != "blr" or any(row["assembly"].startswith("bl ") for row in records):
        raise ValueError("Native record-field routine is no longer a leaf")
    if any(target not in selected for row in records for target in row.get("flows", [])):
        raise ValueError("Native record-field leaf has an external branch")


def load_leaves(connection, selected=None):
    leaves = {}
    for first, (last, _, _) in CONTRACTS.items():
        if selected is not None and first not in selected:
            continue
        records = [json.loads(row[0]) for row in connection.execute(
            "SELECT body FROM instructions WHERE address BETWEEN ? AND ? ORDER BY address", (first, last))]
        require_leaf(first, records, selected if selected is not None else {row["address"] for row in records})
        leaves[first] = records
    return leaves


def _registers(text, extra_slots=None):
    accesses = [(int(width), int(offset)) for width, offset in re.findall(r"s\.(?:r|wr)<(\d+)>\((\d+)", text)]
    layout = {**dict.fromkeys(range(0, 128, 4), 4), 1024: 1, 2304: 1, 4128: 4}
    for base, size in (extra_slots or {}).items():
        if size not in (1, 4) or any(base < other + width and base + size > other
                                   for other, width in layout.items()):
            raise ValueError("Overlapping or unsupported native scalar register layout")
        layout[base] = size
    slots = {}
    for width, offset in accesses:
        field = next(((base, size) for base, size in layout.items()
                      if base <= offset and offset + width <= base + size), None)
        if field is None:
            raise ValueError("Unsupported record-field register slice")
        base, size = field
        slots[base] = size
    return slots


# Step and charge for a lowered leaf's local count; charge saturates like
# State::charge.
FRAME_COUNTING = ("void step(std::uint32_t address){pc=address;++count;}\n"
                  "void charge(std::uint64_t instructions){count=instructions>UINT64_MAX-count?UINT64_MAX:count+instructions;}")


def _promote(first, text, *, extra_slots=None, methods=FRAME_COUNTING):
    if set(re.findall(r"s\.([A-Za-z_]+)", text)) - {"r", "wr", "memory", "step", "charge", "pc"}:
        raise ValueError("Record-field leaf has an unhandled State operation")
    slots = _registers(text, extra_slots)

    def locate(offset, width):
        base, size = next((base, size) for base, size in slots.items()
                          if base <= offset and offset + width <= base + size)
        return base, size, (base + size - offset - width) * 8

    def read(match):
        width, offset = int(match[1]), int(match[2])
        base, size, shift = locate(offset, width)
        value = f"std::uint64_t(frame.v{base})"
        if shift:
            value = f"({value} >> {shift})"
        if width < size:
            value = f"({value} & mask<{width}>())"
        return value

    name = f"RecordFieldFrame_{first:x}"
    code = [f"struct {name} {{ State& state; std::uint32_t pc; std::uint64_t count;"]
    for offset, size in sorted(slots.items()):
        code.append(f"std::uint{size * 8}_t v{offset};")
    code.append(f"explicit {name}(State& s):state(s),pc(0x{first:x}),count(s.executed){{")
    for offset, size in sorted(slots.items()):
        code.append(f"v{offset}=s.r<{size}>({offset});")
    code.append("}")
    # Publishing on normal return and stack unwinding retains exact registers,
    # PC and accounting at faults, including partially completed memory writes.
    code.append(f"~{name}(){{state.pc=pc;state.executed=count;")
    for offset, size in sorted(slots.items()):
        code.append(f"state.wr<{size}>({offset},v{offset});")
    code.extend(["}", methods, "};",
                 f"static bool native_record_fields_{first:x}(State& s) {{ {name} frame(s);"])
    text = re.sub(r'#include[^\n]*\n', '', text)
    prefix = f"namespace cm::aot {{\nbool page_{first >> 12:x}(State& s) {{"
    if prefix not in text:
        raise ValueError("Unexpected original leaf lowering wrapper")
    text = text.split(prefix, 1)[1].rsplit("}", 2)[0]
    text = re.sub(r"s\.r<(\d+)>\((\d+)\)", read, text)
    while match := re.search(r"s\.wr<(\d+)>\((\d+),", text):
        width, offset = int(match[1]), int(match[2])
        depth, at = 1, match.end()
        while depth:
            if text[at] == "(":
                depth += 1
            elif text[at] == ")":
                depth -= 1
            at += 1
        value = text[match.end():at - 1]
        base, size, shift = locate(offset, width)
        if width == size:
            replacement = f"frame.v{base}=std::uint{size * 8}_t({value})"
        else:
            replacement = (f"frame.v{base}=std::uint{size * 8}_t((std::uint64_t(frame.v{base})&"
                           f"~(mask<{width}>()<<{shift}))|((std::uint64_t({value})&mask<{width}>())<<{shift}))")
        text = text[:match.start()] + replacement + text[at:]
    text = text.replace("s.step(", "frame.step(").replace("s.charge(", "frame.charge(").replace("s.pc", "frame.pc")
    if "s.r<" in text or "s.wr<" in text:
        raise ValueError("Unpromoted native record-field register")
    code.extend([text, "}"])
    return "\n".join(code)


def generate_native_record_fields(leaves, render):
    code = ['#include "native_record_fields.hpp"', '#include "aot_runtime.hpp"',
            '#include "native_drawing.hpp"', '#include "match_turbo.hpp"', 'namespace cm::aot {']
    for first, records in leaves.items():
        selected = {row["address"] for row in records}
        require_leaf(first, records, selected)
        code.append(_promote(first, render(first >> 12, records, selected)))
    code.append("bool try_native_record_fields(State& s, std::uint32_t entry){")
    if leaves:
        code.extend([
            # Original instructions remain available for exact finite-prefix
            # checks, reentrant observers and sparse original-PC profiling.
            f"if(s.budget!=UINT64_MAX || s.executed>UINT64_MAX-{MAX_INSTRUCTIONS} ||",
            " s.memory.date_observer || s.memory.read_trace || match_pc_profile_enabled() ||",
            " (s.memory.observer && !dynamic_cast<NativeDrawingObserver*>(s.memory.observer)))return false;",
            "switch(entry){"])
        for first in leaves:
            code.append(f"case 0x{first:x}:return native_record_fields_{first:x}(s);")
        code.append("default:return false;}")
    else:
        code.append("(void)s;(void)entry;return false;")
    code.extend(["}", "}"])
    return "\n".join(code) + "\n"
