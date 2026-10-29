"""Complete source boundary for the in-progress original player decision port.

This contract describes an original dependency, not a semantic replacement.
"""
import hashlib
import json

ENTRY = 0x102836a0
END = 0x10285748
COUNT = 2090
SHA256 = '0e6502a309eb476fbaba0bbde1274e23622b3fba464efb4bb7ec5aca2f094ae6'
FRAGMENTS = (
    (0x10283, 600, 'f80ac80869737a7c029947961c22d58201e502efbc3dfded160763129460abac'),
    (0x10284, 1024, '77b17c2fe97afebc7273c42197f0fad5c7e38f55a9d6ad01b3dfcbf1e488a653'),
    (0x10285, 466, '57a8d2e00691fa54ff104812dfb065871a6c6bc4d63247f20aa5cec81fe00b6d'),
)


def fingerprint(records):
    fields = ('address', 'assembly', 'ops', 'flows', 'fallthrough')
    canonical = [{key: row.get(key) for key in fields} for row in records]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_player_decision_body(records):
    body = [row for row in records if ENTRY <= row['address'] < END]
    if len(body) != COUNT or fingerprint(body) != SHA256:
        raise ValueError('Original player decision body is changed, incomplete or reordered')
    for page, count, digest in FRAGMENTS:
        part = [row for row in body if row['address'] >> 12 == page]
        if len(part) != count or fingerprint(part) != digest:
            raise ValueError(f'Original player decision fragment changed at {page:x}')


def require_player_decision_edges(edges):
    for source, target in edges:
        if ENTRY < target < END and not ENTRY <= source < END:
            raise ValueError(f'External player decision interior entrance {source:x} -> {target:x}')


def load_player_decision(db):
    records = [json.loads(row[0]) for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (ENTRY, END))]
    require_player_decision_body(records)
    require_player_decision_edges(db.execute("""SELECT i.address,e.value FROM instructions AS i,
        json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>? AND e.value<?""", (ENTRY, END)))
    return records

PLAYER_DECISION_ENTRIES = (ENTRY,)


def require_player_decision_contract(page, instructions, selected=None):
    expected = next((item for item in FRAGMENTS if item[0] == page), None)
    if expected is None:
        return
    low, high = max(ENTRY, page << 12), min(END, (page + 1) << 12)
    addresses = {row['address'] for row in instructions} | set(selected or ())
    if not any(low <= address < high for address in addresses):
        return
    body = [row for row in instructions if low <= row['address'] < high]
    if len(body) != expected[1] or fingerprint(body) != expected[2]:
        raise ValueError(f'Original player decision fragment is incomplete or changed at {page:x}')


def require_player_decision_index(db, selected=None):
    if selected is None:
        load_player_decision(db)
    else:
        records = [row for _, row in sorted(selected.items())]
        for page, _, _ in FRAGMENTS:
            require_player_decision_contract(page, records, selected)
        require_player_decision_edges(db.execute("""SELECT i.address,e.value FROM instructions AS i,
            json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
            WHERE e.value>? AND e.value<?""", (ENTRY, END)))


def player_decision_hook(entry):
    return f'if (try_modern_player_decision(s, 0x{entry:x})) return true;'


def require_player_decision_hooks(page, instructions, code, taken_over=()):
    if page != ENTRY >> 12 or ENTRY not in {row['address'] for row in instructions} or ENTRY in taken_over:
        return
    hook = player_decision_hook(ENTRY)
    fragment = code.split(f'i_{ENTRY:x}:', 1)[-1].split(f'i_{ENTRY + 4:x}:', 1)[0]
    steps = [token for token in (f's.step(0x{ENTRY:x});', f'counter.step(0x{ENTRY:x});') if token in fragment]
    if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
        raise ValueError('Missing or misplaced complete player decision hook')
