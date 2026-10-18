"""Pinned complete staff contract query boundaries; production integration pending."""
import hashlib
import json

CONTRACTS = ((268971808, 268971908, 25, 'ebd9944ba545347cb9357787f6d364658660101ba15fb97c90c11a20592a1d70'), (268971920, 268972296, 94, 'c8c2cc33c693d93956580a63be370ab260c001b11e5485c130b579f12649786d'), (268972304, 268972408, 26, 'cfb432a5ebf071473994a1c8bf8f159be774397e4d07c8a6f70d4ffcb6dc07b6'))
FRAGMENTS = ((268971808, 65666, 25, 'ebd9944ba545347cb9357787f6d364658660101ba15fb97c90c11a20592a1d70'), (268971920, 65666, 28, '3ee7d703e4007fe7b832c885cb3babd7b855904dcfe3eeb2a32b86f548898696'), (268971920, 65667, 66, '8c53745cbabb63c390668b44a7aca866025195d63be6d29d8fd9f9703dc4c248'), (268972304, 65667, 26, 'cfb432a5ebf071473994a1c8bf8f159be774397e4d07c8a6f70d4ffcb6dc07b6'))
CONTRACT_QUERY_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_contract_query_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac contract-query contract changed or missing at {start:x}')


def require_contract_query_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External contract-query interior edge from {source:x} to {target:x}')


def require_contract_query_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac contract-query contract changed or missing at {start:x} page {page:x}')


def require_contract_query_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_contract_query_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_contract_query_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_contract_query_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (min(c[0] for c in CONTRACTS), max(c[1] for c in CONTRACTS))))


def contract_query_hook(entry):
    return f'if (try_modern_contract_query(s, 0x{entry:x})) return true;'


def require_contract_query_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in CONTRACT_QUERY_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', contract_query_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced contract-query hook at {entry:x}')
