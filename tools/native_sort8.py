"""Fuse an audited 8-byte heapsort with its descriptor and two pair comparators.

Keep every original instruction, memory effect, callback order and diagnostic
charge. Unsupported policies/destinations retain ordinary generated execution.
Game-derived instruction bodies are emitted only into the private build tree.
"""
import json
from native_record_fields import fingerprint, _promote

ENTRY = 0x10768660
CONTRACTS = {
    ENTRY: (0x10768804, 106, 'bba70d1d94677f1331b639619e407a41b59cf26a7d1f3e44e2defdc505e8767f'),
    0x10781320: (0x10781330, 5, 'fe433621488023321344e62909bba23658510017d7f5041c05f32f79ccf0858e'),
    0x103663b0: (0x1036640c, 24, '13d83e403273c3feceec7e5b862f4f2fd3dd6cdd18fa2324fe1c028b5c5beabd'),
    0x10366410: (0x1036646c, 24, '04bca74890225e21fce36329fe835f1d14624aca01ae0b3cb527a966c34219ae'),
}


def require_fragment(first, records, selected):
    last, count, expected = CONTRACTS[first]
    if ([r['address'] for r in records] != list(range(first, last + 1, 4)) or
            len(records) != count or fingerprint(records) != expected or
            any(r['address'] not in selected for r in records)):
        raise ValueError(f'Changed, missing, duplicate or unselected native sort contract at {first:x}')


def require_sort8_page(page, records, selected):
    if page == ENTRY >> 12 and any(r['address'] == ENTRY for r in records):
        require_fragment(ENTRY, [r for r in records if ENTRY <= r['address'] <= CONTRACTS[ENTRY][0]], selected)


def load_sort8(db, selected=None):
    if selected is not None and ENTRY not in selected:
        return []
    result = []
    for first, (last, _, _) in CONTRACTS.items():
        rows = [json.loads(row[0]) for row in db.execute(
            'SELECT body FROM instructions WHERE address BETWEEN ? AND ? ORDER BY address', (first, last))]
        require_fragment(first, rows, selected if selected is not None else {r['address'] for r in rows})
        result.extend(rows)
    return sorted(result, key=lambda r: r['address'])


def replace_once(text, old, new):
    if text.count(old) != 1:
        raise ValueError('Changed native sort lowering boundary')
    return text.replace(old, new, 1)


def fuse_returns(raw, records):
    for item in records:
        address = item['address']
        start = raw.index(f'i_{address:x}:')
        end = raw.find('\ni_', start + 1)
        if end < 0:
            end = len(raw)
        block = raw[start:end]
        if item['assembly'] == 'bctr':
            block = replace_once(block, 's.pc = std::uint32_t(s.r<4>(4132)); return true;', '''s.pc = std::uint32_t(s.r<4>(4132));
frame.descriptor_tail();
if(s.pc==0x103663b0)goto i_103663b0;
if(s.pc==0x10366410)goto i_10366410;
return true;''')
        elif item['assembly'] == 'blr' and address < 0x10400000:
            index = 3 if address < 0x10366410 else 4
            block = replace_once(block, 's.pc = std::uint32_t(s.r<4>(4128)); return true;', f'''s.pc = std::uint32_t(s.r<4>(4128)); frame.category_return({index});
if(s.pc==0x10768774)goto i_10768774;
if(s.pc==0x10768798)goto i_10768798;
return true;''')
        if address == 0x10781320:
            block = replace_once(block, 's.step(0x10781320);', '''if(frame.count>UINT64_MAX-5){s.pc=0x10781320;return true;}
frame.descriptor_enter();
s.step(0x10781320);''')
        if address in (0x103663b0, 0x10366410):
            index = 3 if address == 0x103663b0 else 4
            block = replace_once(block, f's.step(0x{address:x});',
                                 f'frame.category_enter({index});\ns.step(0x{address:x});')
        raw = raw[:start] + block + raw[end:]
    return raw


FRAME_METHODS = '''void step(std::uint32_t address){pc=address;if(count++==UINT64_MAX)instruction_budget_exhausted();}
int category=-1;bool descriptor=false;ShortlistCategoryDiagnostics* category_stats=nullptr;
void descriptor_enter(){descriptor=true;++state.import_descriptor_calls[import_descriptor::import_count];}
void descriptor_tail(){++state.import_descriptor_tails[import_descriptor::import_count];descriptor=false;}
void category_enter(unsigned i){if(!category_stats)category_stats=&shortlist_category_diagnostics(state);category=int(i);++category_stats->calls[i];}
void category_return(unsigned i){++category_stats->returns[i];category=-1;}
void fault(bool budget){
 if(descriptor){if(budget)++state.import_descriptor_budget_stops[import_descriptor::import_count];else ++state.import_descriptor_faults[import_descriptor::import_count];}
 if(category>=0){if(budget)++category_stats->budget_stops[category];else ++category_stats->faults[category];}
}
'''


def generate_sort8(records, render):
    code = ['#include "native_sort8.hpp"', '#include "aot_runtime.hpp"', '#include "native_drawing.hpp"',
            '#include "match_turbo.hpp"', '#include "shortlist_category_adapter.hpp"',
            '#include "import_descriptor.hpp"', '#include "performance_trace.hpp"',
            '#include <atomic>', '#include <string_view>', 'namespace cm::aot {',
            'namespace { std::atomic<std::uint64_t> eligible_calls{0}; }',
            'std::uint64_t native_sort8_calls(){return eligible_calls.load(std::memory_order_relaxed);}']
    if not records:
        return '\n'.join(code + ['bool try_native_sort8(State&){return false;}', '}']) + '\n'
    chosen = {r['address'] for r in records}
    expected = {address for first, (last, _, _) in CONTRACTS.items()
                for address in range(first, last + 1, 4)}
    if chosen != expected:
        raise ValueError('Unexpected native sort instruction closure')
    for first, (last, _, _) in CONTRACTS.items():
        require_fragment(first, [r for r in records if first <= r['address'] <= last], chosen)
    raw = fuse_returns(render(ENTRY >> 12, records, chosen), records)
    frame = _promote(ENTRY, raw, extra_slots={1027: 1, 2432: 4, 2440: 4, 4132: 4})
    frame = frame.replace('RecordFieldFrame_10768660', 'Sort8Frame').replace('native_record_fields_10768660', 'native_sort8_body')
    frame = replace_once(frame, 'void step(std::uint32_t address){pc=address;++count;}', FRAME_METHODS)
    frame = replace_once(frame, 'Sort8Frame frame(s);', 'Sort8Frame frame(s); try{')
    end = frame.rfind('\n}')
    frame = frame[:end] + '''
}catch(const std::runtime_error& e){frame.fault(std::string_view(e.what())=="Native instruction budget exhausted");throw;}
catch(...){frame.fault(false);throw;}
''' + frame[end:]
    code.append(frame)
    code.append('''bool try_native_sort8(State& s){
 if(s.budget!=UINT64_MAX || s.memory.date_observer || s.memory.read_trace || match_pc_profile_enabled() ||
    (s.memory.observer && !dynamic_cast<NativeDrawingObserver*>(s.memory.observer)))return false;
 if(s.r<4>(20)!=8 || s.r<4>(16)<2)return false;
 if(s.import_descriptor_observer || s.import_descriptor_original ||
    s.import_descriptor_entry_original[import_descriptor::import_count] || s.import_descriptor_original_body)return false;
 const auto& d=shortlist_category_diagnostics(static_cast<const State&>(s));
 if(d.observer || d.original || d.entry_original[3] || d.entry_original[4] ||
    d.original_body==0x103663b0 || d.original_body==0x10366410)return false;
 // Reads during selection have no active diagnostic observer and do not
 // modify state. An invalid descriptor retains the original fault prefix.
 try{const auto target=s.memory.read<4>(s.r<4>(24));if(target!=0x103663b0 && target!=0x10366410)return false;}
 catch(const std::runtime_error&){return false;}
 const auto calls=eligible_calls.fetch_add(1,std::memory_order_relaxed);
 const auto records=std::uint32_t(s.r<4>(16));
 const bool result=native_sort8_body(s);
 if(calls==0){static const PerformanceTrace trace;trace.mark("native-sort8-return",s,records);}
 return result;
}''')
    code.append('}')
    return '\n'.join(code) + '\n'
