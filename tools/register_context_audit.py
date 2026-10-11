"""Pinned Mac floating-register suffixes and scalar nonlocal contexts."""
import hashlib
import json

ROOT_CONTRACTS = (
    (0x107737e0, 0x1077381c, 15, '7e6b803134120073784fa22c315b224cc0253cafdaff16ab7093650f0fb6e73a'),
    (0x10773830, 0x1077386c, 15, '1ab163b6027a9a5a3f503503cde708ae1cd3f0cee9daf62dfe627e2e7f2194b0'),
    (0x10773eb0, 0x10773f24, 29, 'e05ca982d2741a838453e7a62994a2526ce7c0b658652d0664962a3b8a80ab95'),
    (0x10773f30, 0x10773fb0, 32, '1e968cb147e30f34867b6035a658f38a0d30d910c84918916fc9394133d42ade'),
)
ENTRY_CONTRACTS = (
    (0x107737e0, 0x1077381c, 15, '7e6b803134120073784fa22c315b224cc0253cafdaff16ab7093650f0fb6e73a'),
    (0x107737e4, 0x1077381c, 14, '7b1080e5c73074a6a3dfeb6a778882ecfbdc59bb1cf062b441a6778d3848c35a'),
    (0x107737e8, 0x1077381c, 13, '0a00e9cd25d6efa94dbde9531ddadd03cc6756dfc88788cbbb3358970c7a2b2f'),
    (0x107737ec, 0x1077381c, 12, 'fd198da04e2ca3314d6fc6fe8693a00cfcea4ebe02615a76ed19e729757b1f9f'),
    (0x107737f0, 0x1077381c, 11, '08868ab2fdaaf3e4f4644f6cd88d55e8b9d9f3bd606baf7db302f3ea0fc2a462'),
    (0x107737f4, 0x1077381c, 10, '2e6f80d39cb4a3726c5e702d367e38023f0401522c3ebc4fc7303333efae7e09'),
    (0x107737f8, 0x1077381c, 9, '24dd8dae135eb8bbaa994ecd6ad64d249798aa944adf40f09c99a94bb07572c5'),
    (0x107737fc, 0x1077381c, 8, 'de31f1ad0d67031dc180e0df93b2885421cd7fcec1c96fe899cfd75cf07a63b4'),
    (0x10773800, 0x1077381c, 7, '38fa1e036d8681241365df27426b881cd680803c8ee9802e93edc0c28e1b2b51'),
    (0x10773804, 0x1077381c, 6, 'e2de3baf889d1fb74f0276b748a70e778a2e922f85db2c80359187b704f79e70'),
    (0x10773808, 0x1077381c, 5, '87e9f60a85f728b48f51b4741e4a2b426159003def9459dd8bf0c71e2903bc72'),
    (0x1077380c, 0x1077381c, 4, '5b6fb0aa6fa0004e35ffaf3577a5686f7fbca1033a59fb6484fb8bc61b56b40e'),
    (0x10773810, 0x1077381c, 3, 'de7bbacddef161df5576f4915daeb1a7feeafc1eaa011bf9676e405a6645bb75'),
    (0x10773814, 0x1077381c, 2, 'a5290fc01962d53c2600a3dba54b90244d4a3501f9bfc6d14401b68c3ba8a013'),
    (0x10773830, 0x1077386c, 15, '1ab163b6027a9a5a3f503503cde708ae1cd3f0cee9daf62dfe627e2e7f2194b0'),
    (0x10773834, 0x1077386c, 14, 'a8c82982a11c311385c1c9a646426cbda1cec5d9b43e1dd6ec555205af5fcdcf'),
    (0x10773838, 0x1077386c, 13, '146a763379c71aa8fb271220ee484151ee4bf3129a43ff26d95f0456b176c8e6'),
    (0x1077383c, 0x1077386c, 12, 'ef1742e6c2e35fff6794be2842a6aebca0d1954712511b35c75cac3adb0c4bb3'),
    (0x10773840, 0x1077386c, 11, '44423f83f79a0bd6f9c8d1cb49fee9b45ac761ee7dc9688e2e3d45982a7bc740'),
    (0x10773844, 0x1077386c, 10, '5149259f98a23fe224da2e8ad82b12df7344eb9f814e2671b67e7c2c31efee66'),
    (0x10773848, 0x1077386c, 9, '5ac74893585cbd00854e13219080f4281ccc25b8ace441e164b770d91f1760ea'),
    (0x1077384c, 0x1077386c, 8, '7a0720ce0c6b77c1dd62e50c894fe636918b14e832661aa69897a4462adc6749'),
    (0x10773850, 0x1077386c, 7, 'c248bef0801818d637bab29bd06a1c5f07ca892acc8ba772f1eec853c2943dbf'),
    (0x10773854, 0x1077386c, 6, '883b024f84eb49dc5766f8d4829b5ca601f997f8a3ad0a947307ca3ed5b138a4'),
    (0x10773858, 0x1077386c, 5, '0c8fab36aa85ba9ff7e05b3f0fb7f3550723a6e0bbbdedbf2414307dbdc02fef'),
    (0x1077385c, 0x1077386c, 4, '505810912fd76dc02925a436e8bff86a6923cdb197501b1e5aa5e35a364f37fd'),
    (0x10773860, 0x1077386c, 3, '0227fdb107d2a656b8ddf60830f9acdc8b9112775ce4084d4dced358cdec4055'),
    (0x10773864, 0x1077386c, 2, '3d6e12d9027b3b9b5c8d4282fc22daf12363ed5b58505efae4dbb6b0f9868269'),
    (0x10773eb0, 0x10773f24, 29, 'e05ca982d2741a838453e7a62994a2526ce7c0b658652d0664962a3b8a80ab95'),
    (0x10773f30, 0x10773fb0, 32, '1e968cb147e30f34867b6035a658f38a0d30d910c84918916fc9394133d42ade'),
)
REGISTER_CONTEXT_ENTRIES = tuple(start for start, _, _, _ in ENTRY_CONTRACTS)


def _fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_register_context_bodies(instructions, complete=False):
    for start, end, count, expected in ROOT_CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body:
            if complete:
                raise ValueError(f'Missing complete Mac register-context body at {start:x}')
            continue
        if len(body) != count or _fingerprint(body) != expected:
            raise ValueError(f'Mac register-context contract changed at {start:x}')
    for start, end, count, expected in ENTRY_CONTRACTS:
        tail = [r for r in instructions if start <= r['address'] < end]
        if tail and (len(tail) != count or _fingerprint(tail) != expected):
            raise ValueError(f'Mac register-context suffix changed at {start:x}')


def require_register_context_edges(edges):
    entries = set(REGISTER_CONTEXT_ENTRIES)
    for source, target in edges:
        for start, end, _, _ in ROOT_CONTRACTS:
            if start < target < end and not start <= source < end and target not in entries:
                raise ValueError(f'External register-context interior edge from {source:x} to {target:x}')


def require_register_context_contract(page, instructions, selected=None):
    if page != 0x10773:
        return
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, count, expected in ENTRY_CONTRACTS:
        if start in addresses:
            tail = [r for r in instructions if start <= r['address'] < end]
            if len(tail) != count or _fingerprint(tail) != expected:
                raise ValueError(f'Mac register-context contract changed at {start:x}')


def require_register_context_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in ROOT_CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None:
        require_register_context_bodies(records, complete=True)
    else:
        require_register_context_contract(0x10773, [r for _, r in sorted(selected.items())], selected)
    require_register_context_edges(db.execute("""SELECT i.address, e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body, '$.flows'),
                                '$[#]', json_extract(i.body, '$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (ROOT_CONTRACTS[0][0], ROOT_CONTRACTS[-1][1])))


def register_context_hook(entry):
    return (f'if (counter.semantic_boundary(0x{entry:x}, [&] {{ return '
            f'try_modern_register_context(s, 0x{entry:x}); }})) return true;')


CAPTURE_OBSERVATIONS = {
    0x10773eb8: 'scalar_context_capture_link(s, std::uint32_t(s.r<4>(20)));',
    0x10773ec0: 'scalar_context_capture_stack(s, std::uint32_t(s.r<4>(4)));',
}


def require_register_context_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in REGISTER_CONTEXT_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        label, hook, step = f'i_{entry:x}:', register_context_hook(entry), f'counter.step(0x{entry:x});'
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Missing or misplaced register-context hook at {entry:x}')
    for address, hook in CAPTURE_OBSERVATIONS.items():
        if address not in addresses:
            continue
        fragment = code.split(f'i_{address:x}:', 1)[-1].split(f'i_{address + 4:x}:', 1)[0]
        step = f'counter.step(0x{address:x});'
        if code.count(hook) != 1 or hook not in fragment or step not in fragment or fragment.index(hook) < fragment.index(step):
            raise ValueError(f'Missing or misplaced scalar capture observation at {address:x}')
