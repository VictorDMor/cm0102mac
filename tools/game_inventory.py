#!/usr/bin/env python3
"""Whole-game routine inventory: what stays generated PowerPC, where it sits,
what it calls, and in which order it can be rewritten.

  game_inventory.py [--export DIR] [--generated DIR] [--profile NAME=FILE ...]
                    [--out DIR]

Writes inventory.json (one record per routine and per module) and report.md
to --out (default .local/inventory).

Routines. Entries are the targets of `bl`, the code addresses of transition
vectors (a code word followed by the TOC word, the form every function
pointer takes in this executable), the exported symbols and `tools/
discovery_roots.txt`. A routine's body is every instruction its control flow
reaches from its entry: branches and conditional branches are followed, a
`bl` continues after the call, `blr` ends the path, a `b` to another entry is
a tail call (an edge, not body), and a `bctr` follows its jump table (the
pattern of `check_jump_tables.py`); a `bctr` without a resolvable table is
recorded as unresolved. The import glue (symbols `.glue:*`) and the
indirect-call glue 0x10781320 are bridges, not game routines, and are not
counted as code to rewrite. Code no entry reaches is listed as orphan code.

Static reachability starts at `main`, every transition vector (any function
pointer may be called) and the discovery roots, and follows calls and tail
calls. A routine outside that closure is referenced only from unreachable
code: a dead-code candidate, not proof (a pointer built at run time would be
missed).

Modules. A routine inside an audited module's range (MODULE or MODULES in
`tools/*_audit.py`) belongs to it. Otherwise the module comes from the
source-name strings the compiler leaves in each unit's literal pool: a
routine that loads the TOC slot of a pool holding exactly one source name
belongs to that unit ("evidence"). A routine without evidence between two
routines of the same unit, in address order, belongs to that unit too
("filled"); between different units it is "boundary" and between none
"unnamed". The audited ranges calibrate the heuristic: the report shows how
often the evidence and filling agree with them.

Ownership. A routine is C++ when the generator's compilation.json lists its
entry in semanticReplacements (the hooks production pages call). Some owners
decline in some states and run the generated body; that is not visible here.

Order. Modules call each other too densely to order (on 2026-11-06 all but a
few formed one cycle), so the order is per routine. A generated routine is at
layer 0 when it calls no generated routine (every callee is C++ or the OS),
at layer n when nothing it calls is above n-1; routines that call each other
share a layer. For each module with C++ owners the report also lists the
generated routines those owners still reach, judged by the original's calls:
the module runs with no PowerPC under it once that list is empty.

Weights. --profile takes samples from `cm-world-replay --profile`. A
sample's PC charges the routine whose body holds it (self). The guest PC
stays put while C++ runs, so self samples measure generated code only; the
glue's samples are C++ time and are left out.
"""
import argparse
import bisect
import collections
from contextlib import closing
import json
import re
import sqlite3
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(Path(__file__).resolve().parent))
from check_jump_tables import TOC, table_cases  # noqa: E402

CODE_START = 0x10000000
CODE_END = 0x10781330      # first byte after the last instruction the export holds
POINTER_GLUE = 0x10781320  # indirect calls: lwz r0,0(r12) / ... / bctr
SOURCE_NAME = re.compile(rb'(?:[A-Z][A-Z0-9_]*\.(?:CPP|C)|[a-z][a-z0-9_]*\.(?:cpp|c))\x00')
TOC_LOAD = re.compile(r'lwz r\d+,(-?0x[0-9a-f]+|-?\d+)\(r2\)$')
AUDIT_NAME = re.compile(r'\b([A-Za-z][A-Za-z0-9_]*\.(?:CPP|cpp))\b')
AUDIT_MODULE = re.compile(r'^MODULE = \((0x[0-9a-fA-F]+), (0x[0-9a-fA-F]+)\)', re.M)
AUDIT_MODULES = re.compile(r'^MODULES = \{(.*?)\}', re.M | re.S)
AUDIT_PAIR = re.compile(r'"(\w+)": \((0x[0-9a-fA-F]+), (0x[0-9a-fA-F]+)\)')


class Export:
    """The parts of a Ghidra export the inventory reads."""

    def __init__(self, directory):
        directory = Path(directory)
        self.meta = json.loads((directory / "program.json").read_text())
        self.segments = [(s["address"], (directory / s["file"]).read_bytes()) for s in self.meta["segments"]
                         if s.get("initialized") and "file" in s and (directory / s["file"]).exists()]
        self.flow, self.target, self.next, self.assembly = {}, {}, {}, {}
        with closing(sqlite3.connect(f"file:{directory / 'instructions.sqlite'}?mode=ro", uri=True)) as db:
            rows = db.execute("SELECT address, json_extract(body, '$.flow'), json_extract(body, '$.flows'), "
                              "json_extract(body, '$.fallthrough'), json_extract(body, '$.assembly') "
                              "FROM instructions")
            for address, flow, flows, fallthrough, text in rows:
                self.flow[address] = flow
                targets = json.loads(flows) if flows else []
                if targets:
                    self.target[address] = targets[0]
                if fallthrough is not None:
                    self.next[address] = fallthrough
                self.assembly[address] = text
        self.symbols = [s for s in self.meta["symbols"] if s["space"] == "ram"]

    def word(self, address):
        for start, data in self.segments:
            if start <= address and address + 4 <= start + len(data):
                return struct.unpack_from(">I", data, address - start)[0]
        return None


def transition_vectors(export):
    """Code addresses stored next to the TOC word: function pointers."""
    found = set()
    for start, data in export.segments:
        for offset in range(0, len(data) - 7, 4):
            code, toc = struct.unpack_from(">II", data, offset)
            if toc == TOC and CODE_START <= code < CODE_END and code in export.flow:
                found.add(code)
    return found


def discovery_roots(path=ROOT / "tools/discovery_roots.txt"):
    if not path.exists():
        return set()
    return {int(line.split("#")[0].strip(), 16) for line in path.read_text().splitlines()
            if line.split("#")[0].strip()}


def glue_entries(export):
    """Bridges to the OS imports and the indirect-call glue."""
    found = {s["address"] for s in export.symbols if s["name"].startswith(".glue")}
    found.add(POINTER_GLUE)
    return found


def entries(export, vectors, roots):
    """{entry: set of reasons} for every routine start the export shows."""
    found = collections.defaultdict(set)
    for address, flow in export.flow.items():
        target = export.target.get(address)
        if flow == "UNCONDITIONAL_CALL" and target is not None:
            found[target].add("call")
    for address in vectors:
        found[address].add("pointer")
    for symbol in export.symbols:
        if symbol["address"] in export.flow and not symbol["name"].startswith(("PTR_", "s_", ".TVect", ".import")):
            found[symbol["address"]].add("symbol")
    for address in roots:
        if address in export.flow:
            found[address].add("root")
    return {address: reasons for address, reasons in found.items() if address in export.flow}


def body(export, entry, starts, tables):
    """(instructions, calls, tail calls, unresolved bctr addresses, missing
    targets) of the routine at `entry`. Calls are (site, target)."""
    seen, calls, tails, unresolved, missing = set(), [], [], [], []
    work = [entry]
    while work:
        address = work.pop()
        if address in seen:
            continue
        if address not in export.flow:
            missing.append(address)
            continue
        seen.add(address)
        flow = export.flow[address]
        target = export.target.get(address)
        following = export.next.get(address)
        if flow == "UNCONDITIONAL_CALL":
            calls.append((address, target))
            if following is not None:
                work.append(following)
        elif flow == "UNCONDITIONAL_JUMP":
            # `bl` to the next instruction (reading the PC) is classed as a jump.
            if export.assembly[address].startswith("bl ") and target == following:
                work.append(following)
            elif target in starts and target != entry:
                tails.append((address, target))
            elif target is not None:
                work.append(target)
        elif flow == "CONDITIONAL_JUMP":
            if target in starts and target != entry:
                tails.append((address, target))
            elif target is not None:
                work.append(target)
            if following is not None:
                work.append(following)
        elif flow == "CONDITIONAL_TERMINATOR":
            if following is not None:
                work.append(following)
        elif flow == "TERMINATOR":
            pass
        elif flow == "COMPUTED_JUMP":
            cases = tables.get(address)
            if cases:
                work.extend(case for case in cases if case is not None)
            else:
                unresolved.append(address)
        elif following is not None:
            work.append(following)
    return seen, calls, tails, unresolved, missing


def fold_case_roots(routines, flow):
    """A discovery root that only the root list names and that lies inside
    the routine before it, one holding a `bctr`, is a case of a jump table
    the pattern can't read (MAIN.CPP's event loop, MANAGER_MANAGER's result
    table): its body joins that routine. Roots outside such a routine (the
    exception runtime's landing pads) stay routines. Returns {case: host}."""
    cases_of = {}
    ordered = sorted(routines)
    hosts = [entry for entry in ordered if routines[entry]["reasons"] != ["root"]]
    for entry in ordered:
        if routines[entry]["reasons"] != ["root"]:
            continue
        i = bisect.bisect_left(hosts, entry)
        if i == 0:
            continue
        host = routines[hosts[i - 1]]
        if entry < max(host["instructions"]) + 4 and any(flow[a] == "COMPUTED_JUMP" for a in host["instructions"]):
            cases_of[entry] = hosts[i - 1]
    for entry, host in cases_of.items():
        merged = routines.pop(entry)
        target = routines[host]
        for key in ("calls", "tails", "unresolved", "missing"):
            target[key] = target[key] + merged[key]
        target["instructions"] = target["instructions"] | merged["instructions"]
        target["case_roots"] = target.get("case_roots", []) + [entry]
    return cases_of


def source_pools(export):
    """{TOC offset: [source names]}: the TOC slots that point at a literal
    pool holding source-name strings, as analysed on 2026-10-26."""
    code = next(data for start, data in export.segments if start == CODE_START)
    names = {CODE_START + m.start(): m.group()[:-1].decode() for m in SOURCE_NAME.finditer(code)}
    slots = {}
    for offset in range(-32768, 32768, 4):
        value = export.word((TOC + offset) & 0xFFFFFFFF)
        if value is not None:
            slots[offset] = value
    pools = collections.defaultdict(list)
    for address, name in names.items():
        candidates = [value for value in slots.values() if CODE_END <= value <= address]
        if not candidates:
            continue
        base = max(candidates)
        for offset, value in slots.items():
            if value == base:
                pools[offset].append(name)
    return dict(pools)


def audited_modules(tools=ROOT / "tools", known_names=()):
    """[(start, end, name)] of the modules with an audit's exact range."""
    lower = {name.lower(): name for name in known_names}
    special = {"main_program": "MAIN.CPP", "screen_manager": "SCRMAN.CPP"}
    found = []
    for path in sorted(tools.glob("*_audit.py")):
        text = path.read_text(errors="replace")
        stem = path.stem.removesuffix("_audit")
        if match := AUDIT_MODULES.search(text):
            for name, start, end in AUDIT_PAIR.findall(match.group(1)):
                found.append((int(start, 16), int(end, 16), lower.get(name.lower() + ".cpp", name + ".CPP")))
            continue
        match = AUDIT_MODULE.search(text)
        if not match:
            continue
        head = "\n".join(text.splitlines()[:8])
        named = AUDIT_NAME.search(head)
        name = (special.get(stem) or lower.get(stem + ".cpp")
                or (lower.get(named.group(1).lower(), named.group(1)) if named else None) or stem.upper() + ".CPP")
        found.append((int(match.group(1), 16), int(match.group(2), 16), name))
    found.sort()
    for (_, end, a), (start, _, b) in zip(found, found[1:]):
        if start < end:
            raise ValueError(f"Audited modules {a} and {b} overlap")
    return found


def attribute(routines, evidence, audited, neighbours=None):
    """{entry: (module, how)}; `routines` sorted, `evidence` {entry: set of
    names}, `neighbours` {entry: callers and callees}. Audited ranges win;
    evidence with one name next; then filling between two routines of one
    unit; then, for a routine between two units, the unit that all its named
    callers and callees in either of those two units belong to."""
    starts = [start for start, _, _ in audited]
    result = {}

    def audited_name(entry):
        i = bisect.bisect_right(starts, entry) - 1
        if i >= 0 and entry < audited[i][1]:
            return audited[i][2]
        return None

    for entry in routines:
        names = evidence.get(entry, set())
        if (name := audited_name(entry)) is not None:
            result[entry] = (name, "audited")
        elif len(names) == 1:
            result[entry] = (next(iter(names)), "evidence")
        elif len(names) > 1:
            result[entry] = ("+".join(sorted(names)), "conflict")
    named = [(entry, result[entry][0]) for entry in routines if entry in result
             and result[entry][1] in ("audited", "evidence")]
    keys = [entry for entry, _ in named]
    sides = {}
    for entry in routines:
        if entry in result:
            continue
        i = bisect.bisect_left(keys, entry)
        before = named[i - 1][1] if i > 0 else None
        after = named[i][1] if i < len(named) else None
        if before is not None and before == after:
            result[entry] = (before, "filled")
        elif before is None and after is None:
            result[entry] = (None, "unnamed")
        else:
            result[entry] = (None, "boundary")
            sides[entry] = {before, after} - {None}
    changed = bool(neighbours)
    while changed:
        changed = False
        for entry, candidates in sides.items():
            if result[entry][0] is not None:
                continue
            votes = {result[other][0] for other in neighbours.get(entry, ())
                     if other in result and result[other][0] in candidates}
            if len(votes) == 1:
                result[entry] = (votes.pop(), "graph")
                changed = True
    return result


def calibration(routines, evidence, audited, neighbours=None):
    """How the heuristic alone names the routines of audited modules:
    {"agree", "differ", "unnamed"} counts, per method."""
    alone = attribute(routines, evidence, [], neighbours)
    counts = collections.Counter()
    starts = [start for start, _, _ in audited]
    for entry in routines:
        i = bisect.bisect_right(starts, entry) - 1
        if i < 0 or entry >= audited[i][1]:
            continue
        name, how = alone[entry]
        counts[how + (" unnamed" if name is None else " agree" if name == audited[i][2] else " differ")] += 1
    return dict(sorted(counts.items()))


def owners(generated):
    path = Path(generated) / "compilation.json"
    data = json.loads(path.read_text())
    return {int(item["entry"], 16): item["module"].removesuffix("_adapter") for item in data["semanticReplacements"]}


def profile_samples(path):
    data = Path(path).read_bytes()
    offset = 0
    while offset < len(data):
        (count,) = struct.unpack_from("<I", data, offset)
        yield struct.unpack_from(f"<{count}I", data, offset + 4)
        offset += 4 * (1 + count)


def strongly_connected(nodes, edges):
    """Tarjan, iterative. Components in reverse topological order (a
    component comes after every component it reaches)."""
    index, low, on_stack, stack, components = {}, {}, set(), [], []
    counter = 0
    for root in nodes:
        if root in index:
            continue
        work = [(root, iter(sorted(edges.get(root, ()))))]
        index[root] = low[root] = counter
        counter += 1
        stack.append(root)
        on_stack.add(root)
        while work:
            node, children = work[-1]
            advanced = False
            for child in children:
                if child not in index:
                    index[child] = low[child] = counter
                    counter += 1
                    stack.append(child)
                    on_stack.add(child)
                    work.append((child, iter(sorted(edges.get(child, ())))))
                    advanced = True
                    break
                if child in on_stack:
                    low[node] = min(low[node], index[child])
            if advanced:
                continue
            work.pop()
            if work:
                low[work[-1][0]] = min(low[work[-1][0]], low[node])
            if low[node] == index[node]:
                component = []
                while True:
                    member = stack.pop()
                    on_stack.discard(member)
                    component.append(member)
                    if member == node:
                        break
                components.append(sorted(component))
    return components


def build(export, generated, profiles=(), tools=ROOT / "tools", roots_path=ROOT / "tools/discovery_roots.txt"):
    vectors = transition_vectors(export)
    roots = discovery_roots(roots_path)
    glue = glue_entries(export)
    starts = entries(export, vectors, roots)
    tables = {}
    for address, flow in export.flow.items():
        if flow == "COMPUTED_JUMP":
            cases = table_cases(export.assembly, export.word, address)
            if cases:
                tables[address] = cases
    routines = {}
    for entry in sorted(starts):
        if entry in glue:
            continue
        instructions, calls, tails, unresolved, missing = body(export, entry, starts, tables)
        routines[entry] = {"instructions": instructions, "calls": calls, "tails": tails,
                           "unresolved": unresolved, "missing": missing, "reasons": sorted(starts[entry])}
    cases_of = fold_case_roots(routines, export.flow)
    claimed = collections.Counter()
    for r in routines.values():
        claimed.update(r["instructions"])
    glue_code = set()
    for entry in glue:
        if entry in export.flow:
            glue_code |= body(export, entry, starts, tables)[0]
    orphan = sorted(set(export.flow) - set(claimed) - glue_code)

    # Reachability over calls and tail calls; glue passes nothing on.
    edges = {entry: {target for _, target in r["calls"] + r["tails"] if target in routines}
             for entry, r in routines.items()}
    main = next((s["address"] for s in export.symbols if s["name"] == "main"), None)
    reached, work = set(), [entry for entry in routines if entry == main or entry in vectors or entry in roots]
    while work:
        entry = work.pop()
        if entry in reached:
            continue
        reached.add(entry)
        work.extend(edges[entry] - reached)

    # Modules.
    pools = source_pools(export)
    all_names = sorted({name for names in pools.values() for name in names})
    audited = audited_modules(tools, all_names)
    evidence = {}
    for entry, r in routines.items():
        names = set()
        for address in r["instructions"]:
            if (match := TOC_LOAD.match(export.assembly[address])) and len(pools.get(int(match.group(1), 0), ())) == 1:
                names.add(pools[int(match.group(1), 0)][0])
        if names:
            evidence[entry] = names
    order = sorted(routines)
    neighbours = collections.defaultdict(set)
    for entry, targets in edges.items():
        for target in targets:
            if target != entry:
                neighbours[entry].add(target)
                neighbours[target].add(entry)
    attribution = attribute(order, evidence, audited, neighbours)
    calibrated = calibration(order, evidence, audited, neighbours)

    owned = owners(generated)

    # Weights: self samples of generated code, per profile.
    by_address = {}
    for entry, r in routines.items():
        for address in r["instructions"]:
            by_address.setdefault(address, entry)
    imports = next(((s["address"], s["address"] + s["size"]) for s in export.meta["segments"]
                    if s["name"] == "IMPORTS"), (0, 0))
    weights = {}
    for label, path in profiles:
        counts, buckets = collections.Counter(), collections.Counter()
        for sample in profile_samples(path):
            if not sample:
                continue
            buckets["total"] += 1
            pc = sample[0]
            if pc in glue_code:
                buckets["glue"] += 1
            elif imports[0] <= pc < imports[1]:
                buckets["imports"] += 1
            elif (entry := by_address.get(pc)) is not None:
                counts[entry] += 1
            else:
                buckets["elsewhere"] += 1
        weights[label] = {**{key: buckets[key] for key in ("total", "glue", "imports", "elsewhere")}, "self": counts}

    records = []
    for entry in order:
        r = routines[entry]
        module, how = attribution[entry]
        records.append({
            "entry": entry, "module": module, "attribution": how, "owner": owned.get(entry),
            "instructions": len(r["instructions"]),
            "end": max(r["instructions"]) + 4 if r["instructions"] else entry,
            "calls": sorted({target for _, target in r["calls"] if target is not None}),
            "tails": sorted({target for _, target in r["tails"]}),
            "indirect_calls": sum(1 for _, target in r["calls"] if target == POINTER_GLUE),
            "unresolved_bctr": r["unresolved"], "missing": r["missing"], "reasons": r["reasons"],
            "reachable": entry in reached, "case_roots": r.get("case_roots", []),
            "self_samples": {label: w["self"].get(entry, 0) for label, w in weights.items()},
        })
    shared = sum(1 for count in claimed.values() if count > 1)
    return {
        "routines": records, "orphan": orphan, "shared_instructions": shared, "glue": sorted(glue),
        "glue_instructions": len(glue_code), "audited": audited, "calibration": calibrated,
        "unowned_owner_entries": sorted(set(owned) - set(routines)),
        "profiles": {label: {key: w[key] for key in ("total", "glue", "imports", "elsewhere")}
                     for label, w in weights.items()},
        "case_roots": sorted(cases_of),
        "instructions": len(export.flow), "jump_tables": len(tables),
    }


def generated_graph(inventory):
    """{entry: generated callees} over statically reachable generated routines."""
    generated = {r["entry"] for r in inventory["routines"] if r["owner"] is None and r["reachable"]}
    return {r["entry"]: {t for t in r["calls"] + r["tails"] if t in generated and t != r["entry"]}
            for r in inventory["routines"] if r["entry"] in generated}


def layers(graph):
    """({entry: layer}, [components]). Layer 0 calls no generated routine
    (it can be rewritten now, every callee being C++ or the OS); layer n
    calls nothing above layer n-1. Routines that call each other share a
    component and a layer."""
    components = strongly_connected(sorted(graph), graph)
    layer = {}
    for component in components:  # callees first
        members = set(component)
        below = [layer[t] for e in component for t in graph[e] if t not in members]
        value = max(below) + 1 if below else 0
        for e in component:
            layer[e] = value
    return layer, components


def closure(graph, starts):
    seen, work = set(), [s for s in starts if s in graph]
    while work:
        entry = work.pop()
        if entry in seen:
            continue
        seen.add(entry)
        work.extend(graph[entry] - seen)
    return seen


def modules(inventory, graph, layer):
    """Per-module totals: what is generated, what can be rewritten now, and
    the generated code below the module's C++ (what its owners still reach,
    judged by the original's calls)."""
    by_module = collections.defaultdict(list)
    for r in inventory["routines"]:
        by_module[r["module"] or f"({r['attribution']})"].append(r)
    module_of = {r["entry"]: r["module"] or f"({r['attribution']})" for r in inventory["routines"]}
    record = {r["entry"]: r for r in inventory["routines"]}
    labels = list(inventory["profiles"])
    audited = {name for _, _, name in inventory["audited"]}
    summary = {}
    for name, rs in sorted(by_module.items()):
        generated = [r for r in rs if r["entry"] in graph]
        ready = [r for r in generated if layer[r["entry"]] == 0]
        owned = [r for r in rs if r["owner"]]
        below = closure(graph, {t for r in owned for t in r["calls"] + r["tails"]})
        below_modules = collections.Counter(module_of[e] for e in below)
        summary[name] = {
            "routines": len(rs), "owned": len(owned), "audited": name in audited,
            "generated": len(generated), "generated_instructions": sum(r["instructions"] for r in generated),
            "ready": len(ready), "ready_instructions": sum(r["instructions"] for r in ready),
            "max_layer": max((layer[r["entry"]] for r in generated), default=None),
            "attribution": dict(collections.Counter(r["attribution"] for r in rs)),
            "self_samples": {label: sum(r["self_samples"][label] for r in generated) for label in labels},
            "below": {"routines": len(below), "instructions": sum(record[e]["instructions"] for e in below),
                      "self_samples": {label: sum(record[e]["self_samples"][label] for e in below) for label in labels},
                      "modules": dict(below_modules.most_common())},
        }
    return summary


def report(inventory, summary, layer, components):
    routines = inventory["routines"]
    labels = list(inventory["profiles"])
    owned = [r for r in routines if r["owner"]]
    generated = [r for r in routines if r["entry"] in layer]
    dead = [r for r in routines if not r["reachable"] and not r["owner"]]
    lines = ["# Whole-game routine inventory", ""]
    lines.append(f"Export: {inventory['instructions']:,} instructions, {inventory['jump_tables']} resolved jump tables. "
                 f"Glue (OS bridges and indirect calls): {len(inventory['glue'])} entries, "
                 f"{inventory['glue_instructions']:,} instructions, not counted below.")
    lines.append("")
    lines.append("| Routines | Count | Instructions |")
    lines.append("| --- | ---: | ---: |")
    lines.append(f"| All | {len(routines):,} | {sum(r['instructions'] for r in routines):,} |")
    lines.append(f"| C++ owner (any style) | {len(owned):,} | {sum(r['instructions'] for r in owned):,} |")
    lines.append(f"| Generated, statically reachable | {len(generated):,} | {sum(r['instructions'] for r in generated):,} |")
    lines.append(f"| Generated, dead-code candidates | {len(dead):,} | {sum(r['instructions'] for r in dead):,} |")
    lines.append("")
    lines.append(f"Orphan instructions (no entry reaches them): {len(inventory['orphan']):,}. "
                 f"Instructions in more than one routine body: {inventory['shared_instructions']:,}. "
                 f"Routines with an unresolved `bctr`: {sum(1 for r in routines if r['unresolved_bctr'])}; "
                 f"with flow into code the export lacks: {sum(1 for r in routines if r['missing'])}. "
                 f"Jump-table cases found only through discovery roots, folded into their routine: "
                 f"{len(inventory['case_roots'])}. C++ hooks that are not routine starts: "
                 f"{len(inventory['unowned_owner_entries'])} ({len(set(inventory['unowned_owner_entries']) & set(inventory['glue']))} "
                 f"of them on glue; the rest are entries into the middle of a body).")
    lines.append("")
    counts = collections.Counter(r["attribution"] for r in routines)
    lines.append("Attribution: " + ", ".join(f"{how} {counts[how]:,}" for how in
                                             ("audited", "evidence", "filled", "graph", "conflict", "boundary", "unnamed")) + ".")
    lines.append("Calibration on the audited modules (the heuristic alone, by method): " +
                 ", ".join(f"{key} {value}" for key, value in inventory["calibration"].items()) + ".")
    for label, p in inventory["profiles"].items():
        charged = sum(r["self_samples"][label] for r in routines)
        on_generated = sum(r["self_samples"][label] for r in generated)
        lines.append(f"Profile {label}: {p['total']:,} samples; indirect-call glue {p['glue']:,} and import stubs "
                     f"{p['imports']:,} (C++ and OS time); elsewhere {p['elsewhere']:,}; in routine bodies "
                     f"{charged:,}, of which generated code {on_generated:,}.")
    lines.append("")

    lines.append("## Layers")
    lines.append("")
    lines.append("Layer 0 calls no generated routine: it can be rewritten now. Layer n calls nothing above n-1. "
                 "Routines that call each other share a layer.")
    lines.append("")
    lines.append("| Layer | Routines | Instructions | " + " | ".join(f"{label} self" for label in labels) + " |")
    lines.append("| ---: | ---: | ---: |" + " ---: |" * len(labels))
    by_layer = collections.defaultdict(list)
    for r in generated:
        by_layer[layer[r["entry"]]].append(r)
    for value in sorted(by_layer):
        rs = by_layer[value]
        lines.append(f"| {value} | {len(rs):,} | {sum(r['instructions'] for r in rs):,} | " +
                     " | ".join(str(sum(r["self_samples"][label] for r in rs)) for label in labels) + " |")
    lines.append("")
    big = sorted((c for c in components if len(c) > 1), key=len, reverse=True)
    module_of = {r["entry"]: r["module"] or f"({r['attribution']})" for r in routines}
    lines.append(f"Recursive groups (routines that call each other): {len(big)}, holding "
                 f"{sum(len(c) for c in big):,} routines. Largest:")
    lines.append("")
    for c in big[:8]:
        mods = collections.Counter(module_of[e] for e in c)
        lines.append(f"- {len(c)} routines, layer {layer[c[0]]}: " +
                     ", ".join(f"{m} {n}" for m, n in mods.most_common(6)) + (" ..." if len(mods) > 6 else ""))
    lines.append("")

    lines.append("## Modules with generated code")
    lines.append("")
    lines.append("Sorted by generated self samples, then generated instructions. Ready: generated routines at "
                 "layer 0. Attribution other than audited is inferred (see the tool's docstring).")
    lines.append("")
    header = ("| Module | Routines | C++ | Generated | Gen. instr. | Ready | Ready instr. | Max layer | " +
              " | ".join(f"{label} self" for label in labels) + " | Attribution |")
    lines.append(header)
    lines.append("| --- |" + " ---: |" * (header.count("|") - 3) + " --- |")
    rows = [(name, m) for name, m in summary.items() if m["generated"]]
    rows.sort(key=lambda item: (-sum(item[1]["self_samples"].values()), -item[1]["generated_instructions"]))
    for name, m in rows:
        attribution = ", ".join(f"{k} {v}" for k, v in m["attribution"].items())
        lines.append(f"| {name} | {m['routines']} | {m['owned']} | {m['generated']} | {m['generated_instructions']:,} | "
                     f"{m['ready']} | {m['ready_instructions']:,} | {m['max_layer']} | " +
                     " | ".join(str(m["self_samples"][label]) for label in labels) + f" | {attribution} |")
    lines.append("")

    lines.append("## Generated code below the C++ modules")
    lines.append("")
    lines.append("For each module with C++ owners: the generated routines its owners still reach (closure over the "
                 "original's calls). A module runs with no PowerPC under it once this is empty.")
    lines.append("")
    lines.append("| Module | C++ | Generated below | Instructions | " + " | ".join(f"{label} self" for label in labels) +
                 " | Mostly in |")
    lines.append("| --- | ---: | ---: | ---: |" + " ---: |" * len(labels) + " --- |")
    rows = [(name, m) for name, m in summary.items() if m["owned"]]
    rows.sort(key=lambda item: -item[1]["below"]["routines"])
    for name, m in rows:
        b = m["below"]
        mostly = ", ".join(f"{k} {v}" for k, v in list(b["modules"].items())[:4])
        lines.append(f"| {name} | {m['owned']} | {b['routines']:,} | {b['instructions']:,} | " +
                     " | ".join(str(b["self_samples"][label]) for label in labels) + f" | {mostly} |")
    return "\n".join(lines) + "\n"


def json_ready(inventory, summary, layer, components):
    def hexed(values):
        return [f"0x{v:x}" for v in values]
    routines = []
    for r in inventory["routines"]:
        record = dict(r)
        record["entry"] = f"0x{r['entry']:x}"
        record["end"] = f"0x{r['end']:x}"
        record["layer"] = layer.get(r["entry"])
        for key in ("calls", "tails", "unresolved_bctr", "missing", "case_roots"):
            record[key] = hexed(r[key])
        routines.append(record)
    return {
        "routines": routines, "modules": summary,
        "recursive_groups": [hexed(c) for c in components if len(c) > 1],
        "orphan": hexed(inventory["orphan"]), "glue": hexed(inventory["glue"]),
        "audited": [{"start": f"0x{a:x}", "end": f"0x{b:x}", "module": n} for a, b, n in inventory["audited"]],
        "calibration": inventory["calibration"], "profiles": inventory["profiles"],
        "unowned_owner_entries": hexed(inventory["unowned_owner_entries"]),
        "case_roots": hexed(inventory["case_roots"]),
        "shared_instructions": inventory["shared_instructions"],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--export", type=Path, default=ROOT / ".local/current/export")
    parser.add_argument("--generated", type=Path, default=ROOT / ".local/current/generated")
    parser.add_argument("--profile", action="append", default=[], metavar="NAME=FILE")
    parser.add_argument("--out", type=Path, default=ROOT / ".local/inventory")
    args = parser.parse_args()
    profiles = []
    for item in args.profile:
        label, _, path = item.partition("=")
        if not path:
            parser.error("--profile takes NAME=FILE")
        profiles.append((label, Path(path)))
    inventory = build(Export(args.export), args.generated, profiles)
    graph = generated_graph(inventory)
    layer, components = layers(graph)
    summary = modules(inventory, graph, layer)
    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / "inventory.json").write_text(json.dumps(json_ready(inventory, summary, layer, components), indent=1))
    text = report(inventory, summary, layer, components)
    (args.out / "report.md").write_text(text)
    print(text.split("## ")[0])
    print(f"Wrote {args.out / 'inventory.json'} and {args.out / 'report.md'}")


if __name__ == "__main__":
    main()
