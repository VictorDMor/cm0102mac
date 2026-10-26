"""Pinned complete shortlist quotation context and reference-quote operations."""
import hashlib
import json
import struct

CONTRACTS = ((272000112, 272011380, 2817, '7e04c41cab47745d6beff5b4865517ec5e3812c9a6eed3b5841f1bf6014b82b2'), (272086768, 272098264, 2872, '1b04873d293a5e22eb07e4fa151d114a7038802a48db79d5db11b9d1f655ecf4'))
FRAGMENTS = ((272000112, 66406, 740, '9fd95adca5f09b0bf4b176c7a78d66b8a5596f57cfec8bbfae0e090d29faa73b'), (272000112, 66407, 1024, 'f7b7cbaa90b93a5fa120be79f6348ae14317c731e9c9976fb9a40183a11ddf81'), (272000112, 66408, 1024, '9fd6ea12ee50765d59d52ffeec9dc54bb77ae7c3eb829e41e2bd2c32433e3a5c'), (272000112, 66409, 29, '1e21f11208e0ccf5237eaed2257a8907c0f3646e99d2e3438f03bc9b79f9911f'), (272086768, 66427, 580, '24e32ac154e3d2db2a08389988585d78e82bad64b6671a1131c943e64cc89741'), (272086768, 66428, 1022, '2f1e79eef0b2458523fb1126cecdee4acd68505e31d6f6b07b30f81155511704'), (272086768, 66429, 1024, 'a20894ee6b438dead3f99db2d1bbd97c8b65f841cb99ed2eb69b78fb69dc35b9'), (272086768, 66430, 246, '7e052d1c89a549b1b4cd34c43151d47485477e4a554227b0d0929f7004f91114'))
SHORTLIST_QUOTATION_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)
PREPARED_SHORTLIST_QUOTATION_ENTRIES = SHORTLIST_QUOTATION_ENTRIES


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_shortlist_quotation_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac shortlist-quotation contract changed or missing at {start:x}')


def require_shortlist_quotation_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External shortlist-quotation interior edge from {source:x} to {target:x}')


def require_shortlist_quotation_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac shortlist-quotation contract changed or missing at {start:x} page {page:x}')


def require_shortlist_quotation_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_shortlist_quotation_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_shortlist_quotation_contract(page, [r for _, r in sorted(selected.items())], selected)
    conditions = ' OR '.join('(e.value>=? AND e.value<?)' for _ in CONTRACTS)
    parameters = [value for start, end, _, _ in CONTRACTS for value in (start, end)]
    require_shortlist_quotation_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE """ + conditions, parameters))


TABLES = ((277572788, 276871384, 952, '1594ad5b73f4ff87752f6bf8c34423c139677d1eabf1a9447e91d107cd3495ec'), (277572776, 276871340, 20, '8d2d4214f193c9e4cede06dc092200c76c45291b339ddfd1067511ed706475a3'), (277572792, 276870732, 274, '767b5f96e4009266e54c32b218c3632e9fdb3498f3ca3d8afa07f9629d59123c'), (277572836, 276874552, 584, '602b14ffcc1e60b34b3912c92e513417c070649fbd9ee4a0d1beb1d4f17ec9b6'), (277572840, 276874160, 360, 'f6aa96b47fe4353a4c2ff2009ac3e9ed9755c36f8f0a34c1fe59fd500d686d2c'), (277572844, 276872680, 649, 'c54af9a3d3c86537c24c074d172346853c95e3c89ca9324f2a02fe2f9532253c'), (277572760, 277671884, 68, '8fcce906ebef9da0607731683134104b4e28812c4e1bf90ba6c14c81f35cfac9'), (277572756, 277671820, 64, '9196a666851bd9cd60bfb638cc4f53163f3c92c8adc781064b30743f3a4a593f'), (277572752, 277671756, 64, 'e8695ce074f0e197556a7648a40e712c6e8d8cde7146c5bd36de9ffc6b811733'), (277572748, 277671692, 64, '752f66c84aceffaf66c1e1850d59c7f9d733037bf9d460756c02308bacc2235c'), (277572820, 277672212, 44, 'f25fb17a27226713eb505b003b6f9a470ded3c33437987e4c338e980de67cbaa'))

def require_shortlist_quotation_tables(read):
    for slot,base,size,expected in TABLES:
        if struct.unpack(">I",read(slot,4))[0]!=base or hashlib.sha256(read(base,size)).hexdigest()!=expected:
            raise ValueError("Changed original shortlist quotation table")

def load_shortlist_quotation_tables(directory):
    program=json.loads((directory/"program.json").read_text())
    segments=[(s["address"],(directory/s["file"]).read_bytes()) for s in program["segments"] if s.get("file")]
    def read(a,n):
        for base,data in segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError("Missing original shortlist-quotation table")
    require_shortlist_quotation_tables(read)
    require_shortlist_quotation_padding(read)
    return tuple(read(base,size) for _,base,size,_ in TABLES)


PADDING = ((272011380, '00000000'), (272011384, '00000000'), (272011388, '00000000'), (272091900, '4800001c'), (272091904, '48000020'), (272098264, '00000000'), (272098268, '00000000'))

def require_shortlist_quotation_padding(read):
    for address,expected in PADDING:
        if read(address,4)!=bytes.fromhex(expected):
            raise ValueError("Changed nonindexed quotation alignment word")


def shortlist_quotation_hook(entry):
    return f'if (try_modern_shortlist_quotation(s, 0x{entry:x})) return true;'


def require_shortlist_quotation_hooks(page, instructions, code, taken_over=()):
    """Each entry has its hook, except those another family took over."""
    addresses = {r['address'] for r in instructions}
    for entry in SHORTLIST_QUOTATION_ENTRIES:
        if entry >> 12 != page or entry not in addresses or entry in taken_over: continue
        label, hook = f'i_{entry:x}:', shortlist_quotation_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced shortlist-quotation hook at {entry:x}')
