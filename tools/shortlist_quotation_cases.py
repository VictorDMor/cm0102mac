"""Validate and emit compact synthetic, complete-entry regression inputs."""
import json
from pathlib import Path

CASE_PATH = Path(__file__).resolve().parent.parent/'native/testdata/shortlist_quotation_cases.json'
DEPENDENCIES = {
    0x107737f8,0x10773848,0x10775540,0x10773780,0x10082f20,0x10082f90,
    0x1077b770,0x100cdef0,0x100f4790,0x100c7d00,0x100de260,0x100d9440,
    0x1036ff00,0x100ce250,0x1077b720,0x100cab10,0x10103510,0x10718130,
    0x103721f0,0x10780fc0,0x100da050,0x100da0b0,0x10464160,0x100d9520,
    0x100d9580,0x103f43b0,0x100f55b0,0x103092f0,0x100f2680,
}
BOOL_OPTIONS = {'primary','secondary','floating_stop'}
WORD_OPTIONS = {'renewal','classification'}


def require(condition, message):
    if not condition: raise ValueError(message)


def word(value, bits=32):
    require(type(value) is int and 0<=value<1<<bits, 'Invalid synthetic fixture integer')
    return value


def patches(items, registers=False, maps=()):
    occupied = set()
    for address, encoded in items:
        require(isinstance(address,str) and address.startswith('0x'), 'Invalid fixture patch address')
        spelling=address;address = int(address,16)
        require(spelling==hex(address),'Noncanonical fixture patch address')
        require(isinstance(encoded,str) and encoded and len(encoded)%2==0, 'Invalid fixture bytes')
        data = bytes.fromhex(encoded)
        for byte in range(address,address+len(data)):
            allowed = 0<=byte<0x6000 if registers else (
                0x30000000<=byte<0x30080000 or 0x700f9f00<=byte<0x70100f00 or
                0x108b3a90<=byte<0x108c4a90 or any(a<=byte<a+n for a,n in maps))
            require(allowed and byte not in occupied, 'Overlapping or non-synthetic fixture patch')
            occupied.add(byte)


def load_shortlist_quotation_cases(path=CASE_PATH):
    document = json.loads(path.read_text())
    require(set(document)=={'schema','origin','cases'} and document['schema']==1,
            'Changed synthetic fixture schema')
    require(document['origin']=='Synthetic complete-entry inputs retained from original-reference characterization.',
            'Undeclared fixture provenance')
    cases = document['cases']; require(isinstance(cases,list) and len(cases)>=2, 'Missing complete-entry fixtures')
    seen = set(); roots = set()
    for case in cases:
        require(set(case)=={'root','registers','maps','memory','words','first_words','second_words','reals','writes','options'},
                'Changed fixture fields')
        require(type(case['root']) is int and case['root'] in (0,1),'Unknown complete fixture root')
        roots.add(case['root']); maps = case['maps']
        require(maps in ([],[[0,4096]]), 'Undeclared synthetic map')
        patches(case['registers'],True); patches(case['memory'],maps=maps)
        for field in ('words','first_words','second_words','reals','writes'):
            keys = set()
            for entry in case[field]:
                expected = 3 if field in {'first_words','second_words','writes'} else 2
                require(len(entry)==expected,'Malformed dependency fixture')
                target = word(entry[0]); require(target in DEPENDENCIES,'Unknown dependency fixture target')
                key = tuple(entry[:-1]); require(key not in keys,'Repeated dependency fixture key'); keys.add(key)
                if field=='writes':
                    patches([[hex(word(entry[1])),word(entry[2]).to_bytes(4,'big').hex()]],maps=maps)
                elif field=='reals': word(entry[1],64)
                else:
                    for value in entry[1:]: word(value)
        require(set(case['options'])<=BOOL_OPTIONS|WORD_OPTIONS,'Unknown synthetic fixture option')
        for key,value in case['options'].items():
            if key in BOOL_OPTIONS: require(type(value) is bool,'Invalid fixture Boolean')
            else: word(value)
        canonical = json.dumps(case,sort_keys=True,separators=(',',':'))
        require(canonical not in seen,'Duplicate complete-entry input'); seen.add(canonical)
    require(roots=={0,1}, 'Both complete roots require retained positive inputs')
    return cases


def render_shortlist_quotation_cases(cases):
    source = '// Generated synthetic complete-entry inputs. No original instruction execution.\n'
    source += f'constexpr unsigned retained_quotation_cases={len(cases)};\n'
    source += 'CorpusCase retained_quotation_case(unsigned index) {\n    switch(index) {\n'
    for index,case in enumerate(cases):
        source += f'    case {index}: {{auto state=seed({case["root"]});Options options;\n'
        for a,n in case['maps']: source += f'        state->memory.map({a}u,{n}u);\n'
        for a,encoded in case['registers']:
            for offset,value in enumerate(bytes.fromhex(encoded)):
                source += f'        state->wr<1>({int(a,16)+offset},{value});\n'
        for a,encoded in case['memory']:
            source += f'        state->memory.copy_in({a}u,std::vector<std::uint8_t>{{'+','.join(str(v) for v in bytes.fromhex(encoded))+'});\n'
        for target,value in case['words']:
            source += f'        options.words[{hex(target)}]={value}u;\n'
        for field in ('first_words','second_words'):
            for target,argument,value in case[field]:
                source += f'        options.{field}[{{{hex(target)},{argument}u}}]={value}u;\n'
        for target,bits in case['reals']:
            source += f'        options.reals[{hex(target)}]=std::bit_cast<double>(UINT64_C({bits}));\n'
        for target,address,value in case['writes']:
            source += f'        options.writes[{hex(target)}].push_back({{{address}u,{value}u}});\n'
        for key,value in case['options'].items():
            source += f'        options.{key}='+('true' if value is True else 'false' if value is False else f'{value}u')+';\n'
        source += '        return {std::move(state),std::move(options)};}\n'
    return source+'    default:throw std::runtime_error("Unknown retained quotation fixture");\n    }\n}\n'
