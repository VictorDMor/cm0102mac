"""Record-group field, chain and mutation boundaries."""
import hashlib
import json

CONTRACTS = (
    (0x1073edb0, 0x1073eddc, 11, '6879f911dd4df4306f389d6e570b272b03168770b37a662344506e174da0b2dd'),
    (0x1073ede0, 0x1073ee30, 20, '927c9a322346bc89d0dc59d76d4b783ecddbaedf955997185c8473ce7e2cdbb5'),
    (0x1073ee30, 0x1073ef68, 77, 'a79ec9349f9093db1036be68f8a9742abc51743d67dacb478dbd9f51242decdb'),
    (0x1073ef70, 0x1073f09c, 75, '43553ee94bc15ef648e52485a715b80b3a2948e5d59ce188cbd91eecc8eec686'),
    (0x1073f0a0, 0x1073f1dc, 79, '763b9bea9d9c75b0f2eadfaa6ca8305e38cf9dfc5ae9306ed1bab820b8dc88ad'),
    (0x1073f1e0, 0x1073f44c, 155, '6a241d9180201519f94d7c674910dac6f93fe886ffb5949b607f57cf1fea101e'),
    (0x1073f530, 0x1073f6fc, 113, '5eff2c09a8e33e6b24f343dc3703f385d3a0c3aeb20a894aa36cd683a639d952'),
    (0x1073f700, 0x1073f79c, 39, 'a38624b085d086245cf70bff39459853546e3061f51185e8bec5f69fa2617908'),
    (0x1073f7a0, 0x1073f860, 47, 'e9ccc54169ad8263349bd2c5e5ed4bf448c913f0de9babd041fb472b38cf3ed2'),
    (0x1073f860, 0x1073f8fc, 39, '0591136f306daf8428da28ca06f75f722504ec4cfd37385f0fe0dcac28d8f6d2'),
    (0x1073f900, 0x1073fc18, 198, '2a3e4ed62d20bc34a7899ace43861c9355d4df059bd4c9b37057198807bdcba4'),
)
FRAGMENTS = (
    (0x1073ef70, 0x1073e, 36, 'a61981385025ca1b0748b57c9b5781d47fb29ce0724ef48e8229fb3deb1b1fb3'),
    (0x1073ef70, 0x1073f, 39, '36b35828ba81e9d09f9a803df3d9518b0e81f2e1669d369a366fa9e5b83e1097'),
)
RECORD_GROUP_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_record_group_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac record-group contract changed or missing at {start:x}')


def require_record_group_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External record-group interior edge from {source:x} to {target:x}')


def require_record_group_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, count, expected in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        fragment = next((f for f in FRAGMENTS if f[0] == start and f[1] == page), None)
        if fragment: count, expected = fragment[2:]
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac record-group contract changed or missing at {start:x} page {page:x}')


def require_record_group_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_record_group_bodies(records, complete=True)
    else:
        for page in tuple(sorted({page for start, end, _, _ in CONTRACTS for page in range(start >> 12, ((end - 1) >> 12) + 1)})):
            require_record_group_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_record_group_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def record_group_hook(entry):
    return f'if (try_modern_record_group(s, 0x{entry:x})) return true;'


def require_record_group_hooks(page, instructions, code, taken_over=()):
    addresses = {r['address'] for r in instructions}
    for entry in RECORD_GROUP_ENTRIES:
        if entry >> 12 != page or entry not in addresses or entry in taken_over: continue
        label, hook = f'i_{entry:x}:', record_group_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced record-group hook at {entry:x}')
