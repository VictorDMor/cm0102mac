"""Pinned complete original match target and eligibility boundaries."""
import hashlib
import json

CONTRACTS = ((270358368, 270358456, 22, '34638d8b7412d8a73bc988995af8f436821e634db74724b4132f5ec2abc9c2dc'), (270411952, 270413916, 491, '5d0940156ee5fd0e3362a69617defdefd229752ffa1051471db3ac0f4b51f524'), (270413920, 270414160, 60, 'c97767c96c6c519acc441f47707b15f732993dd0046930d9f76505c0ed7bbacd'), (270414160, 270414196, 9, 'd46850ed60b5276bb71ec67e1e3292b870f027273207234641e43ea6c0d82a77'), (270414240, 270414548, 77, 'eb61e8fb4819f45c3d1109986d626f795522a20ef5edeab72b9a8c3416dc9ba5'), (271081664, 271081896, 58, 'dc779879c9e72652c51581a2e01e9386eeaa9c43f7e2626153e709794a3dcd5f'), (271081904, 271082152, 62, 'e1da25af30e5a79daced2a136950ac536d4431c27afeada01b721e3e13da578b'), (269565456, 269565480, 6, '3f63129c5b2efbfbec04f777e5adf55341907222a8fec6d10e44519652969628'))
FRAGMENTS = ((270358368, 66005, 22, '34638d8b7412d8a73bc988995af8f436821e634db74724b4132f5ec2abc9c2dc'), (270411952, 66018, 468, '13b9c7db62edddbf78c7f0eef671e49eb2b947f029a32b9b94d5898825c008a2'), (270411952, 66019, 23, '05998d6b2aa870135fe74e75572de2813ad84f21adba7ea182808dda1fd7dede'), (270413920, 66019, 60, 'c97767c96c6c519acc441f47707b15f732993dd0046930d9f76505c0ed7bbacd'), (270414160, 66019, 9, 'd46850ed60b5276bb71ec67e1e3292b870f027273207234641e43ea6c0d82a77'), (270414240, 66019, 77, 'eb61e8fb4819f45c3d1109986d626f795522a20ef5edeab72b9a8c3416dc9ba5'), (271081664, 66182, 58, 'dc779879c9e72652c51581a2e01e9386eeaa9c43f7e2626153e709794a3dcd5f'), (271081904, 66182, 62, 'e1da25af30e5a79daced2a136950ac536d4431c27afeada01b721e3e13da578b'), (269565456, 65811, 6, '3f63129c5b2efbfbec04f777e5adf55341907222a8fec6d10e44519652969628'))
MATCH_TARGET_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_match_target_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac match-target contract changed or missing at {start:x}')


def require_match_target_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External match-target interior edge from {source:x} to {target:x}')


def require_match_target_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac match-target contract changed or missing at {start:x} page {page:x}')


def require_match_target_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_match_target_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_match_target_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_match_target_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (min(v[0] for v in CONTRACTS), max(v[1] for v in CONTRACTS))))


def match_target_hook(entry):
    return f'if (try_modern_match_target(s, 0x{entry:x})) return true;'


def require_match_target_hooks(page, instructions, code, taken_over=()):
    """Each entry has its hook, except those another family took over."""
    addresses = {r['address'] for r in instructions}
    for entry in MATCH_TARGET_ENTRIES:
        if entry >> 12 != page or entry not in addresses or entry in taken_over: continue
        label, hook = f'i_{entry:x}:', match_target_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced match-target hook at {entry:x}')
