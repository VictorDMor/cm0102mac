"""Pinned Mac C++ object-storage contracts; production integration is separately verified."""
import hashlib
import json

CONTRACTS = (
    (0x10773150, 0x10773258, 'a6afda68307478477bd8922640c4873c67f5788eacb600dafc664f56d50f9fe3'),
    (0x10773260, 0x1077331c, '9a9ffcdf4dfb20cb017811be54708dbee238686951a7f960d86b1247cebf2952'),
    (0x10773320, 0x10773420, '91e81b26c6de2bb5f8e6243cb1f973e3ebaa6472f61887fa6ea9747430fb5da6'),
    (0x10773420, 0x1077349c, '34c36507e32d34a513b1f61a34dcaefe79463b18acf2829530e084547eec6898'),
    (0x107734a0, 0x10773520, '54ca31aad92b91b5353c2332088aa902f83729fafefab468dbbf61ffbd9f6022'),
    (0x10773520, 0x1077359c, 'fc0742057d9526d3608997abb956343feb8e37d062f86492bd5e3c0b72cce7a1'),
    (0x107735a0, 0x107735e4, '257718e3cdbaa24cc942a38e938ee0e2f82d1ba82e5114e9caf54865bd06397e'),
    (0x107735f0, 0x10773610, 'b791ed62d9dec139ca9ad9bf42d03582af82d555681d44cd8254e760bf4f2ea1'),
    (0x10773610, 0x10773630, '184a701e68a139ab7c1f31cf289d8cc55a11c54c659e28869429c9c77bc20025'),
    (0x10773630, 0x1077367c, '556e2a1e82811199adc13acd050fec0f4112fc9daa79edc05f95f90cf52667da'),
    (0x10773680, 0x107736d4, '53b70fe40e832cc6e2038ccf69d8e9e25e75f3342e681422fff10f536c304bc2'),
    (0x107736e0, 0x107736e8, '9c50a478e66513910adc7af8ebdbcd62612840d9d710274434a71fe8f280b7bc'),
)
CPP_OBJECT_STORAGE_ENTRIES = tuple(start for start, _, _ in CONTRACTS)
INSTRUCTION_COUNTS = (64, 46, 62, 30, 31, 30, 14, 8, 8, 19, 21, 2)
PAGE_FRAGMENTS = {}


def _require(records, expected, start):
    body = [{key: item.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')}
            for item in records]
    actual = hashlib.sha256(json.dumps(body, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    if actual != expected:
        raise ValueError(f'Mac cpp_object_storage contract changed at {start:x}')


def require_cpp_object_storage_contract(page, instructions):
    for start, end, expected in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12:
            continue
        records = [r for r in instructions if start <= r['address'] < end]
        if records:
            fragment = PAGE_FRAGMENTS.get((start, page))
            if fragment:
                count, digest = fragment
                if len(records) != count:
                    raise ValueError(f'Mac cpp_object_storage fragment size changed at {start:x}/{page:x}')
                _require(records, digest, start)
            else:
                _require(records, expected, start)


def require_cpp_object_storage_bodies(instructions, complete=False):
    for start, end, expected in CONTRACTS:
        records = [r for r in instructions if start <= r['address'] < end]
        if complete and not records:
            raise ValueError(f'Missing complete Mac cpp_object_storage body at {start:x}')
        if records:
            _require(records, expected, start)


def require_cpp_object_storage_edges(edges):
    for source, target in edges:
        for start, end, _ in CONTRACTS:
            if start <= target < end and not start <= source < end and target != start:
                raise ValueError(f'External cpp_object_storage interior edge from {source:x} to {target:x}')


def require_cpp_object_storage_index(db, selected=None):
    instructions = ([json.loads(row[0]) for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
        (CONTRACTS[0][0], CONTRACTS[-1][1]))] if selected is None
        else [item for _, item in sorted(selected.items())])
    require_cpp_object_storage_bodies(instructions, complete=selected is None)
    require_cpp_object_storage_edges(db.execute("""SELECT i.address, edge.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body, '$.flows'),
                                '$[#]', json_extract(i.body, '$.fallthrough'))) AS edge
        WHERE edge.value>=? AND edge.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def require_cpp_object_storage_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in CPP_OBJECT_STORAGE_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        label = f'i_{entry:x}:'
        hook = f'if (try_modern_cpp_object_storage(s, 0x{entry:x})) return true;'
        if code.count(hook) != 1 or label not in code:
            raise ValueError(f'Missing or duplicate cpp_object_storage hook at {entry:x}')
        fragment = code.split(label, 1)[1].split(f'i_{entry + 4:x}:', 1)[0]
        step = f'counter.step(0x{entry:x});' if f'counter.step(0x{entry:x});' in fragment else f's.step(0x{entry:x});'
        if hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Misplaced cpp_object_storage hook at {entry:x}')
