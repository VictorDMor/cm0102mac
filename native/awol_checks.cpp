// AWOL original/C++ returned comparisons. List, date, allocation and text
// routines run for real; files, alerts, the world services around the staff
// tables and the reseed are scripted, each recording its inputs in guest
// memory and scrambling the volatile registers afterwards, so both a wrong
// argument and a stale leftover register show up as a memory or register
// difference.
#include "awol_adapter.hpp"
#include "awol_detail.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#if defined(CM_AWOL_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
#endif
using namespace cm::aot;
using guest::Cpu;
namespace cm::aot {bool awol_reference(State&);}
namespace {
constexpr unsigned data=0x60000000,data_size=0x80000,stack=0x71010000,sentinel=0xfffffff0;
constexpr unsigned trace_at=data+0x60000,trace_end=data+0x80000;
constexpr unsigned module_low=0x100031b0,module_high=0x10009ef0;
constexpr std::array entries{0x100031b0u,0x10003300u,0x100033c0u,0x100036e0u,0x10003dc0u,0x10003ec0u,0x10003fa0u,
    0x10004070u,0x10006080u,0x10006600u,0x100066d0u,0x100067d0u,0x10006830u,0x10006900u,0x10007100u,0x100072e0u,
    0x10007a50u,0x10008770u,0x10008b60u,0x100090a0u,0x10009b60u};
// The list constructor and other leaves of other modules run inside Cpu::call,
// without reaching the probe, and charge the budget: at most two follow any
// other dependency, in at most 9 instructions. No path through a generated
// AWOL body with a dependency or a frame of its own fits in this allowance.
constexpr unsigned leaf_allowance=12;
bool original_mode=false,production_mode=false,tracing=false,ownership_mode=false;
// The scripted world of one case. Forked children inherit a copy.
struct Script {
    std::vector<std::uint8_t> input;   // the file being read
    std::size_t read_at=0;
    unsigned calls=0,reads=0,writes=0,fail_read=0,fail_write=0,version=0x3e006;
    bool open_fails=false;
    unsigned contract=0;               // 0x10082f20's answer, 0: none
    std::vector<unsigned> contracts;   // successive answers, when given
    unsigned contract_calls=0,reseed=0x1234;
    unsigned heap=data+0x30000;        // allocations come from the fixture's bump area
    std::map<unsigned,std::function<unsigned(Cpu&)>> services;  // per-target override
};
Script script;
void trace(Cpu& c,unsigned target) {
    auto at=c.u32(trace_at);
    if(at==0)at=trace_at+4;
    const auto add=[&](unsigned value){if(at>=trace_end)throw std::runtime_error("AWOL trace bound");c.put32(at,value);at+=4;};
    add(target);add(c.lr());
    for(unsigned r=3;r<=10;++r)add(c.r(r));
    for(unsigned r=13;r<32;++r)add(c.r(r));
    for(unsigned f=0;f<8;++f)add(c.cr(f));
    c.put32(trace_at,at);
}
void scramble(Cpu& c) {
    ++script.calls;
    for(unsigned r=4;r<=12;++r)c.set(r,0xde000000|script.calls<<8|r);
    for(unsigned f=0;f<14;++f)c.set_f(f,-20.0-f-script.calls);
    for(unsigned f:{0u,1u,5u,6u,7u})c.set_cr(f,std::uint8_t(5+(script.calls&1)));
}
void read_file(Cpu& c) {
    const auto into=c.r(3),unit=c.r(4),items=c.r(5);
    ++script.reads;
    const auto size=unit*items;
    const bool failed=script.reads==script.fail_read;
    const auto bytes=std::min<std::size_t>(failed&&size?size-1:size,script.input.size()-std::min(script.read_at,script.input.size()));
    if(bytes)c.s.memory.copy_in(into,std::vector<std::uint8_t>(script.input.begin()+std::ptrdiff_t(script.read_at),script.input.begin()+std::ptrdiff_t(script.read_at+bytes)));
    script.read_at+=bytes;
    c.set(3,failed||bytes<size?(unit==1?unsigned(bytes):0):items);
}
void write_file(Cpu& c) {
    ++script.writes;
    const auto from=c.r(3),unit=c.r(4),items=c.r(5);
    // The written bytes reach the comparison through the trace region.
    auto at=c.u32(trace_at);if(at==0)at=trace_at+4;
    for(unsigned n=0;n<unit*items && n<0x400;++n)c.put8(at++,c.u8(from+n));
    while(at&3)c.put8(at++,0);
    c.put32(trace_at,at);
    c.set(3,script.writes==script.fail_write?0:(unit==1?items:items));
}
// The allocator (its real body needs the Memory Manager the fixture lacks),
// for any caller, or null if the case fails this routine.
bool allocator(Cpu& c,unsigned target) {
    const auto carve=[&](unsigned size,bool clear){
        const auto at=script.heap;script.heap+=(size+15)&~15u;
        if(script.heap>data+0x40000)throw std::runtime_error("AWOL fixture heap bound");
        if(clear)for(unsigned n=0;n<size;++n)c.put8(at+n,0);
        return at;
    };
    switch(target) {
    case 0x10773520:case 0x10775130:c.set(3,carve(c.r(3)?c.r(3):4,false));return true;
    case 0x10775200:c.set(3,carve(c.r(3)*c.r(4),true));return true;
    case 0x10775170:case 0x107735a0:return true;
    default:return false;
    }
}
// A scripted routine: true when it ran. Everything else runs for real.
bool scripted(Cpu& c,unsigned target) {
    if(const auto found=script.services.find(target);found!=script.services.end()){c.set(3,found->second(c));return true;}
    switch(target) {
    case 0x107475b0:c.set(3,script.open_fails?0:data+0x50000);return true;
    case 0x10747a80:read_file(c);return true;
    case 0x10747cc0:write_file(c);return true;
    case 0x10747900:c.put32(c.r(3),0);c.set(3,0);return true;
    case 0x10739890:c.put32(c.r(6),0x41574f4c);c.put32(c.r(7),0x2e435050);return true;
    case 0x10769b60:c.set(3,1);return true;
    case 0x1077b770:c.set(3,script.reseed);return true;
    case 0x10082f20: {
        const auto n=script.contract_calls++;
        c.set(3,n<script.contracts.size()?script.contracts[n]:script.contract);return true;
    }
    case 0x102bdf90:case 0x102cb4b0:case 0x102cc8e0:case 0x102cc400:c.set(3,0);return true;
    default:return false;
    }
}
bool probe(State& s) {
    Cpu c(s);const auto target=s.pc;
    if(module_low<=target && target<module_high) {
        if(original_mode)return awol_reference(s);
        if(production_mode)return false;
        if(awol_lists(target)){awol_run_rewrite(s,target);return true;}
        return false;
    }
    if(ownership_mode && module_low<=c.lr() && c.lr()<module_high) {
        const auto stop=c.lr();s.budget=s.executed+2000000;ownership_mode=false;
        try{run_until(s,stop);}catch(...){ownership_mode=true;throw;}
        ownership_mode=true;s.budget=s.executed+leaf_allowance;return true;
    }
    if(run_leaf(s))return true;
    const auto caller=c.lr();
    // The Toolbox BlockMoveData that string and list copies reach.
    if(target==0xfff10000){const auto bytes=s.memory.copy_out(c.r(3),c.r(5));s.memory.copy_in(c.r(4),bytes);s.pc=caller;return true;}
    const bool direct=tracing && module_low<=caller && caller<module_high;
    if(direct)trace(c,target);
    if(direct && scripted(c,target)){const auto r3=c.r(3);scramble(c);c.set(3,r3);s.pc=caller;return true;}
    if(allocator(c,target)){
        if(direct){const auto r3=c.r(3);scramble(c);c.set(3,r3);}
        s.pc=caller;return true;
    }
    return false;
}
void seed(Cpu& c) {
    for(unsigned r=0;r<32;++r)c.set(r,0x51000000+r*0x101);
    c.set(1,stack);c.set(2,0x108bba90);c.set_lr(sentinel);
    for(unsigned f=0;f<8;++f)c.set_cr(f,f+1);
    for(unsigned f=0;f<32;++f)c.set_f(f,1.25+f);
    for(unsigned bit=1280;bit<1312;++bit)c.s.wr<1>(bit,0);
    c.s.budget=c.s.executed+20000000;
}
// The fixture's world, at fixed addresses inside the data region.
namespace world {
constexpr unsigned records=data+0x100,flags=data+0x200,staff_table=data+0x1000,club_table=data+0x10000,
    profile=data+0x28000,contract=data+0x2a000,tables=data+0x2b000,
    manager=data+0x2c000,message_service=data+0x2d000;
constexpr unsigned staff_count=16,club_count=8;
inline unsigned staff(unsigned id){return staff_table+id*0x70;}
inline unsigned club(unsigned id){return club_table+id*0x248;}
}
struct Case {
    unsigned entry;
    std::string name;
    std::function<void(Cpu&)> prepare;
};
// Makes a list with the original's real routines, `items` of them with
// staff ids from `first`, at `object`.
void append_records(Cpu& c,unsigned object,unsigned size,unsigned first,unsigned items) {
    c.set(3,object);c.set_lr(sentinel);c.s.pc=0x10738c00;run_until(c.s,sentinel);
    for(unsigned n=0;n<items;++n) {
        const auto record=world::tables+0x800;
        c.put32(record,first+n);
        for(unsigned i=4;i<size;++i)c.put8(record+i,std::uint8_t(0x30+n*3+i));
        c.set({{3,object},{4,record},{5,size}});c.set_lr(sentinel);c.s.pc=0x10738d10;run_until(c.s,sentinel);
    }
}
void build_world(Cpu& c) {
    using namespace world;
    c.s.memory.map(data,data_size);c.s.memory.map(stack-0x10000,0x20000);
    c.s.memory.copy_in(data,std::vector<std::uint8_t>(data_size,0xa5));
    c.s.memory.copy_in(stack-0x10000,std::vector<std::uint8_t>(0x20000,0xa5));
    c.put32(trace_at,0);
    c.put32(c.r(2)-0x7e64,data+0x38000);c.put32(data+0x38000,0xfff10000);c.put32(data+0x38004,c.r(2));
    c.put32(c.toc(-0x7be4),staff_count);
    c.put32(c.toc(-0x7be0),staff_table);
    c.put32(c.toc(-0x7be8),club_table);
    c.put32(c.toc(-0x7b7c),data+0x2f000);
    c.put32(c.toc(-0x7b78),0);
    c.put32(c.toc(-0x7bf0),message_service);
    c.put32(c.toc(-0x7b74),0x3e006);
    c.put32(c.toc(-0x7bac),0x01000000);
    c.put8(c.toc(-0x7bac),1);
    // Staff: profile, club, a contract's relations, ids.
    for(unsigned id=0;id<staff_count;++id) {
        const auto at=staff(id);
        c.put32(at,id);c.put32(at+0xc,manager+id*0x80);
        c.put32(at+0x14,profile+id*0x20);
        c.put32(at+0x24,club(id%club_count));
        for(unsigned off:{0x28u,0x2cu,0x30u})c.put32(at+off,data+0x2e800+off);
        c.put16(profile+id*0x20+8,std::uint16_t(100+id));c.put16(profile+id*0x20+0xa,std::uint16_t(200+id));c.put16(profile+id*0x20+0xc,std::uint16_t(300+id*500));
    }
    for(unsigned off:{0x28u,0x2cu,0x30u})c.put32(data+0x2e800+off+0x20,0x1000+off);
    for(unsigned id=0;id<club_count;++id) {
        const auto at=club(id);
        c.put32(at,id);c.put32(at+0x2c,manager+id*0x80);c.put32(at+0x80,staff(id));c.put32(at+0x90,staff(id));
        for(unsigned slot=0;slot<50;++slot)c.put32(at+0x98+slot*4,slot<4?staff((id*4+slot)%staff_count):0);
    }
    for(unsigned n=0;n<8;++n)c.put32(manager+n*0x80+0x68,0x7000+n);
    c.put32(contract+4,1);
    // The random words: a real slot layout with the cursor at the start.
    const auto base=c.toc(-0x7b8c);
    for(unsigned n=0;n<64;++n)c.put16(base+n*2,std::uint16_t(0x3039*(n+1)));
    c.put32(c.toc(-0x7b84),base);
    c.put32(c.toc(-0x7b88),base+64*2);
    c.put32(c.toc(-0x7b90),0x5a5a);
    // The records object: flags and three lists, built by the real routines.
    c.put32(records+12,flags);
    for(unsigned n=0;n<staff_count;++n)c.put8(flags+n,0);
    for(unsigned n=0;n<3;++n)c.put32(records+n*4,tables+n*0x40);
    tracing=false;original_mode=true;
    append_records(c,tables,0xc,0,0);
    append_records(c,tables+0x40,0x10,0,0);
    append_records(c,tables+0x80,0x14,0,0);
    original_mode=false;
}
#include "awol_checks_cases.inc"
void returned(State& s) {
    Cpu c(s);if(c.sp()!=stack || c.lr()!=sentinel || s.pc!=sentinel)throw std::runtime_error("AWOL fixture did not return");
}
void original(State& s,unsigned entry){original_mode=true;tracing=true;s.pc=entry;run_until(s,sentinel);returned(s);}
void rewrite(State& s,unsigned entry) {
    original_mode=false;tracing=true;
    if(production_mode){s.pc=entry;run_until(s,sentinel);}else awol_run_rewrite(s,entry);
    returned(s);
#if defined(CM_AWOL_COVERAGE)
    if(__llvm_profile_write_file()!=0)throw std::runtime_error("Cannot flush AWOL coverage");
#endif
}
void corrupt_register(State& s,unsigned entry){rewrite(s,entry);Cpu c(s);c.set(3,c.r(3)^1);}
void corrupt_memory(State& s,unsigned entry){rewrite(s,entry);Cpu c(s);c.put8(world::flags,c.u8(world::flags)^1);}
struct Observed final:MemoryObserver {
    void mapped(unsigned,std::uint64_t)override{}
    void written(unsigned,const std::uint8_t*,std::size_t)override{}
    void cleared(unsigned,std::uint64_t)override{}
};
std::unique_ptr<State> prepared(const State& base,const Case& t) {
    auto s=cm::semantic_io_checks::clone(base);Cpu c(*s);
    script=Script{};
    seed(c);build_world(c);seed(c);
    t.prepare(c);
    tracing=true;
    c.s.pc=t.entry;
    return s;
}
}
int main(int argc,char** argv) {
    try {
        if(argc!=2)throw std::runtime_error("Expected private original export");
        auto base=std::make_unique<State>();load_program(*base,argv[1]);dispatch_probe=probe;
        const auto cases=all_cases();
        const auto only=std::getenv("CM_AWOL_ONLY");const auto filter=only?std::strtoul(only,nullptr,0):0;
        const auto name_filter=std::getenv("CM_AWOL_NAME");
        unsigned completed=0,attempted=0,failures=0;std::map<unsigned,std::array<unsigned,2>> counts;
        for(bool production:{false,true}) {
            if(production && std::getenv("CM_AWOL_CONTROLLED_ONLY"))break;
            production_mode=production;
            for(const auto& test:cases) {
                if(failures>=8)break;
                if(filter && test.entry!=filter)continue;
                if(name_filter && test.name.find(name_filter)==std::string::npos)continue;
                                auto s=prepared(*base,test);Observed observer;if(production)s->memory.observer=&observer;
                const auto differences=compare_routine_call(*s,test.entry,original,rewrite,RoutineChecker::result_r3|RoutineChecker::result_f1);++attempted;
                if(differences.empty()){++completed;++counts[test.entry][production?1:0];}
                else{++failures;std::cout<<"DIFFERS "<<(production?"production ":"controlled ")<<hex_address(test.entry)<<' '<<test.name<<'\n';for(const auto& d:differences)std::cout<<"  "<<d<<'\n';}
            }
        }
        production_mode=false;
        if(!failures && !filter && !name_filter) {
            for(const auto& test:cases) {
                if(test.entry!=0x10003fa0)continue;
                auto s=prepared(*base,test);
                if(compare_routine_call(*s,test.entry,original,corrupt_register,RoutineChecker::result_r3).empty()){++failures;std::cout<<"FAIL register corruption control\n";}
                else std::cout<<"ok deliberate register corruption detected\n";
                auto m=prepared(*base,test);
                if(compare_routine_call(*m,test.entry,original,corrupt_memory,RoutineChecker::result_r3).empty()){++failures;std::cout<<"FAIL memory corruption control\n";}
                else std::cout<<"ok deliberate memory corruption detected\n";
                break;
            }
        }
        // Every case, with no instruction budget for generated code: an owned
        // path that fell back to a generated body would stop here.
        unsigned ownership=0,owned=0;
        if(!std::getenv("CM_AWOL_CONTROLLED_ONLY")) {
            for(const auto& test:cases) {
                if(failures>=8)break;
                if(filter && test.entry!=filter)continue;
                if(name_filter && test.name.find(name_filter)==std::string::npos)continue;
                ++owned;
                auto state=prepared(*base,test);
                Observed observer;state->memory.observer=&observer;state->budget=state->executed+leaf_allowance;
                production_mode=true;ownership_mode=true;
                try{rewrite(*state,test.entry);++ownership;}catch(const std::exception& e){++failures;std::cout<<"FAIL ownership "<<hex_address(test.entry)<<' '<<test.name<<' '<<e.what()<<'\n';}
                ownership_mode=false;production_mode=false;
            }
            for(auto entry:entries) {
                const auto found=std::ranges::find(cases,entry,&Case::entry);
                if(found==cases.end()){++failures;std::cout<<"FAIL no case for "<<hex_address(entry)<<'\n';continue;}
                auto reference=prepared(*base,*found);reference->budget=reference->executed;bool rejected=false;
                try{original(*reference,entry);}catch(const std::exception& e){rejected=std::string(e.what())=="Native instruction budget exhausted" && reference->pc==entry;}
                if(!rejected){++failures;std::cout<<"FAIL original budget control "<<hex_address(entry)<<'\n';}
            }
        }
        if(owned)std::cout<<ownership<<'/'<<owned<<" observed normal-production ownership controls\n";
        for(const auto& [entry,n]:counts)std::cout<<hex_address(entry)<<" returned "<<n[0]<<" controlled + "<<n[1]<<" production\n";
        std::cout<<completed<<" returned comparisons / "<<attempted<<" attempts; "<<failures<<" failures\n";return failures?1:0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
