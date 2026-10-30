// Complete returned comparisons against hook-free generated originals.
// IO, assertions and game services are explicit caller-boundary fixtures;
// sorting, callbacks, date construction and memset execute actual bodies.
#include "award_shortlist_adapter.hpp"
#include "award_shortlist.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <array>
#include <bit>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>

#if defined(CM_AWARD_SHORTLIST_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
#endif
namespace cm::aot { bool award_shortlist_reference(State&); }
using namespace cm::aot;
using guest::Cpu;
namespace {
constexpr unsigned data=0x60000000,object=data+0x1000,records=data+0x2000,
    spec=data+0x3000,people=data+0x10000,clubs=data+0x20000,nation=data+0x30000,
    region=data+0x31000,output=data+0x4000,date=data+0x5000,
    rng_base=data+0x6000,trace_at=data+0x80000,io_at=data+0x100000,
    stack=0x71010000,sentinel=0xfffffff0;
constexpr std::array entries{0x10437cc0u,0x10437e40u,0x10437ea0u,0x104380c0u,
    0x104382c0u,0x10438450u,0x104386b0u,0x104394c0u,0x10439a40u,
    0x10439c10u,0x10439ea0u,0x10439ee0u,0x10439fa0u,0x10439fd0u};
struct Case {
    unsigned entry;
    std::string name="default";
    int count=3,capacity=4,electorate=6,file=1,specification=1,allocation=1,
        fail_io=0,partial=-1,deleting=0,storage=1,person=1,club=1,
        manager_id=16,ballot=-1,match=1,relation=3,enabled=1,service=0,
        rng=17,wrap=0,limit=3,actual_sort=1,peer=1,selected_club=7,
        selected_nation=8,selected_region=9,left_id=0,right_id=1,votes=-999;
    unsigned bound=100,bias=0,score_bits=0x3f800000,other_bits=0x40000000;
};
Case active{entries[0]};
bool original_mode=false,production_mode=false;
unsigned calls=0,io_calls=0;
const std::set<unsigned> dependencies{0x100caeb0,0x1014c2e0,0x102bdf90,0x102cb4b0,
    0x102cc140,0x10437730,0x10739890,0x107465f0,0x10747a80,0x10747cc0,
    0x10768300,0x10768810,0x10769b60,0x107735a0,0x10775130,0x10775170,0x1077b720,0x1077b770};
void trace(Cpu& c,unsigned target) {
    if (++calls>2000) throw std::runtime_error("fixture dependency call limit");
    auto count=c.u32(trace_at);
    const auto add=[&](unsigned value) { c.put32(trace_at+4+count++*4,value); };
    add(target);add(c.lr());
    for (unsigned reg=3;reg<=10;++reg) add(c.r(reg));
    for (unsigned reg=13;reg<32;++reg) add(c.r(reg));
    for (unsigned field=0;field<8;++field) add(c.cr(field));
    c.put32(trace_at,count);
}
bool probe(State& s) {
    Cpu c(s);const auto target=s.pc;
    // Cpu::call executes generated leaves before the dispatcher probe. Use
    // that same boundary for the reference, rather than modeling a leaf on
    // only one side of a comparison.
    if (run_leaf(s)) return true;
    if (target>=entries[0] && target<0x10439fe0) {
        if (original_mode) return award_shortlist_reference(s);
        if (production_mode) return false;
        if (award_shortlist_lists(target)) { award_shortlist_run_rewrite(s,target);return true; }
        return false;
    }
    if (!dependencies.contains(target)) return false;
    trace(c,target);
    if (target==0x10768300 && active.actual_sort) return false;
    if (target==0x10775130) c.set(3,active.allocation?records:0);
    else if (target==0x10747a80 || target==0x10747cc0) {
        ++io_calls;
        const auto width=c.r(4),count=c.r(5),address=c.r(3);
        if (count>64 || width>32) throw std::runtime_error("invalid IO fixture size");
        const auto failed=int(io_calls)==active.fail_io;
        const auto total=width*count;
        const auto bytes=failed?(active.partial<0?0u:std::min(total?total-1:0,unsigned(active.partial))):total;
        if (target==0x10747a80) {
            for (unsigned i=0;i<bytes;++i) {
                unsigned value=0;
                if (io_calls==1) value=((unsigned(active.count)<<16)|unsigned(active.capacity))>>(24-i*8);
                else value=0x30+io_calls+i;
                c.put8(address+i,value);
            }
        } else {
            const auto length=c.u32(io_at);
            for (unsigned i=0;i<bytes;++i) c.put8(io_at+4+length+i,c.u8(address+i));
            c.put32(io_at,length+bytes);
        }
        // The original stream reports whole transferred items, including
        // short transfers; a partial item may still modify its byte prefix.
        c.set(3,bytes/width);
    } else if (target==0x10739890) { c.put32(c.r(6),0x41574400);c.put32(c.r(7),0x43505000); }
    else if (target==0x10768810) c.set(3,spec+0x100);
    else if (target==0x1077b720) c.put32(c.r(3),c.r(8));
    else if (target==0x1077b770) c.set(3,0x1234);
    else if (target==0x1014c2e0) c.set(3,active.service);
    else if (target==0x10437730) c.set(3,9);
    else if (target==0x102bdf90) { c.put32(c.r(3),c.r(4));c.put32(c.r(3)+4,c.r(5)); }
    else if (target==0x102cb4b0) c.put32(c.r(3)+8+c.r(4)*4,c.r(5));
    else if (target==0x102cc140) {
        auto n=c.u32(data+0x7000);c.put32(data+0x7000,n+1);
        for (unsigned i=0;i<28;++i) c.put8(data+0x7100+n*32+i,c.u8(c.r(4)+i));
        c.put32(data+0x7100+n*32+28,c.r(5));
    } else if (target==0x100caeb0) {
        // The name formatter owns this string contract, not this module.
        c.put8(c.r(4),'A'+c.u32(c.r(3)));c.put8(c.r(4)+1,0);
    } else if (target==0x107465f0) {
        c.set(3,unsigned(int(c.u8(c.r(3)))-int(c.u8(c.r(4)))));
    }
    s.pc=c.lr();return true;
}
void initialize(Cpu& c,const Case& test) {
    c.s.memory.map(data,0x200000);c.s.memory.map(stack-0x10000,0x20000);
    c.s.memory.copy_in(stack-0x10000,std::vector<std::uint8_t>(0x20000,0xa5));
    for (unsigned r=0;r<32;++r) c.set(r,0x51000000+r*0x101);
    c.set(1,stack);c.set(2,0x108bba90);c.set_lr(sentinel);
    for (unsigned field=0;field<8;++field) c.set_cr(field,field+1);
    c.set_f(1,double(std::bit_cast<float>(test.score_bits)));
    const auto variable=[&](int slot,unsigned offset,unsigned value) {
        c.put32(c.r(2)+unsigned(slot),data+offset);c.put32(data+offset,value);
    };
    variable(-0x7be4,0x100,32);variable(-0x7be0,0x104,people);variable(-0x7be8,0x108,clubs);
    variable(-0x7b50,0x10c,6);variable(-0x7b04,0x110,2);
    variable(-0x7bb4,0x114,0);variable(-0x7bf0,0x118,0);
    c.put32(c.r(2)-0x7bac,data+0x120);c.put8(data+0x120,1);
    c.put32(c.r(2)-0x7ba8,data+0x140);
    variable(-0x7b90,0x200,test.bias);variable(-0x7b84,0x204,rng_base+(test.wrap?16:0));
    variable(-0x7b88,0x208,rng_base+16);c.put32(c.r(2)-0x7b8c,rng_base);
    for (unsigned i=0;i<32;i+=2) c.put16(rng_base+i,test.rng+i);
    c.put32(object,test.storage?records:0);c.put16(object+4,test.count);c.put16(object+6,test.capacity);
    c.put8(object+0x28,test.electorate);c.put32(object+0x2c,test.match?test.selected_club:99);
    c.put32(object+0x30,test.match?test.selected_nation:99);c.put32(object+0x34,test.match?test.selected_region:99);
    c.put16(object+0x3c,0xffff);
    for (unsigned i=0;i<16;++i) c.put16(object+8+i*2,test.ballot);
    c.put8(spec,test.electorate);c.put32(spec+4,7);c.put32(spec+8,8);c.put32(spec+12,9);c.put8(spec+16,test.capacity);
    c.put16(date,300);c.put16(date+2,2002);c.put8(date+4,5);
    for (unsigned i=0;i<64;++i) {
        c.put32(records+i*16,i%32);c.put32(records+i*16+4,0xffffffff);
        c.put32(records+i*16+8,i%2?test.other_bits:test.score_bits);c.put16(records+i*16+12,i%3);
        const auto person=people+i*0x70;c.put32(person,i);c.put32(person+0x24,test.club?clubs:0);
        c.put32(person+0x20,test.peer?clubs+0x248*2:0);c.put8(person+0x6b,test.enabled);
    }
    if (test.entry>=entries[11]) { c.put32(records,test.left_id);c.put32(records+16,test.right_id); }
    if (test.votes!=-999) { c.put16(records+12,test.votes);c.put16(records+28,test.ballot); }
    c.put32(people+16*0x70,test.manager_id);
    for (unsigned i=0;i<8;++i) {
        const auto club=clubs+i*0x248;c.put32(club,i);
        c.put32(club+0x30,test.relation>=1?nation:0);c.put32(club+0x2c,test.relation>=2?nation:0);
    }
    c.put32(nation,7);c.put32(nation+0x68,8);c.put32(nation+0x74,test.relation>=3?region:0);c.put32(region+8,9);
    c.set({{3,object},{4,people+16*0x70},{5,test.person?people+0x70:0}});
    if (test.entry==entries[0]) { c.set(4,test.specification?spec:0);c.set(5,test.file?spec+0x200:0); }
    if (test.entry==entries[1]) { c.set(3,test.person?object:0);c.set(4,test.deleting); }
    if (test.entry==entries[2] || test.entry==entries[3]) c.set(4,test.file?spec+0x200:0);
    if (test.entry==entries[4]) { c.set(4,test.person?people:0);c.set(5,test.club?clubs:0); }
    if (test.entry==entries[5]) { c.set(4,date);c.set(5,11);c.set(6,12); }
    if (test.entry==entries[6]) { c.set(4,output);c.set(5,test.limit); }
    if (test.entry==entries[7]) c.set(3,test.bound);
    if (test.entry>=entries[11]) { c.set(3,records);c.set(4,records+16); }
    c.s.budget=1000000;
}
void returned(State& s) {
    Cpu c(s);if (c.sp()!=stack || c.lr()!=sentinel || s.pc!=sentinel) throw std::runtime_error("fixture did not return");
}
void original(State& s,unsigned entry) { original_mode=true;s.pc=entry;run_until(s,sentinel);returned(s); }
void rewrite(State& s,unsigned entry) {
    original_mode=false;
    if (production_mode) { s.pc=entry;run_until(s,sentinel); }
    else award_shortlist_run_rewrite(s,entry);
    returned(s);
#if defined(CM_AWARD_SHORTLIST_COVERAGE)
    __llvm_profile_write_file();
#endif
}
void wrong_result(State& s,unsigned entry) { rewrite(s,entry);Cpu c(s);c.set(3,c.r(3)^1); }
void wrong_memory(State& s,unsigned entry) { rewrite(s,entry);Cpu c(s);c.put8(object,c.u8(object)^1); }
void wrong_rng(State& s,unsigned entry) { rewrite(s,entry);Cpu c(s);c.put32(data+0x204,c.u32(data+0x204)+2); }
struct ObservedWrites final:MemoryObserver {
    unsigned count=0;
    void mapped(unsigned,std::uint64_t) override {}
    void written(unsigned,const std::uint8_t*,std::size_t) override { ++count; }
    void cleared(unsigned,std::uint64_t) override { ++count; }
};
}
int main(int argc,char** argv) {
    if (argc!=2) { std::cerr<<"Usage: cm-award-shortlist-checks <original-segments>\n";return 2; }
    try {
        auto base=std::make_unique<State>();load_program(*base,argv[1]);dispatch_probe=probe;
        std::vector<Case> cases;
        for (auto entry:entries) cases.push_back(Case{entry});
        for (auto entry:{entries[0],entries[2],entries[3]}) for (int n:{0,1,4}) for (int fail=0;fail<=4;++fail) {
            Case t{entry,"IO/count"};t.count=n;t.fail_io=fail;cases.push_back(t);
        }
        for (int allocation:{0,1}) for (int spec_present:{0,1}) for (int file:{0,1}) {
            Case t{entries[0],"constructor gates"};t.allocation=allocation;t.specification=spec_present;t.file=file;cases.push_back(t);
        }
        for (int object_present:{0,1}) for (int storage:{0,1}) for (int deleting:{-32768,-1,0,1,32767,65535}) {
            Case t{entries[1],"destructor"};t.person=object_present;t.storage=storage;t.deleting=deleting;cases.push_back(t);
        }
        for (auto entry:{entries[2],entries[3]}) for (int fail=1;fail<=4;++fail) for (int partial:{0,1,3,7,15}) {
            Case t{entry,"partial IO"};t.fail_io=fail;t.partial=partial;cases.push_back(t);
        }
        for (int count:{0,3,4,5}) for (int person:{0,1}) for (int club:{0,1}) {
            Case t{entries[4],"candidate limits"};t.count=count;t.person=person;t.club=club;cases.push_back(t);
        }
        for (int mode:{-1,0,1,2,3,4,5,6,7}) for (int relation:{0,1,2,3}) for (int match:{0,1}) {
            for (auto entry:{entries[5],entries[6],entries[9]}) {
                Case t{entry,"electorate/relation"};t.electorate=mode;t.relation=relation;t.match=match;cases.push_back(t);
            }
        }
        for (auto entry:{entries[5],entries[6]}) for (int count:{0,1,4}) for (int service:{0,1,255}) {
            Case t{entry,"population gates"};t.count=count;t.service=service;cases.push_back(t);
        }
        for (auto entry:{entries[8],entries[9]}) for (int manager:{15,16,31,32}) for (int ballot:{-32768,-2,-1,0,2,32767}) {
            Case t{entry,"ballot/manager bounds"};t.manager_id=manager;t.ballot=ballot;cases.push_back(t);
        }
        for (int count:{0,1,2}) { Case t{entries[8],"candidate search"};t.count=count;cases.push_back(t); }
        { Case t{entries[9],"empty eligibility"};t.count=0;cases.push_back(t); }
        for (int mode=0;mode<4;++mode) for (int service:{1,255}) {
            Case t{entries[6],"controlled electorate"};t.electorate=mode;t.service=service;cases.push_back(t);
        }
        for (int mode:{0,1,2,3}) for (int selected:{-1,0,7,8,9}) for (int region_id:{-1,9}) {
            Case t{entries[9],"eligibility field asymmetry/sentinel"};t.electorate=mode;
            t.selected_club=selected;t.selected_nation=selected;t.selected_region=region_id;cases.push_back(t);
        }
        for (auto entry:{entries[5],entries[9]}) for (int mode=0;mode<8;++mode)
            for (int club:{0,1}) for (int peer:{0,1}) for (int enabled:{0,1,255}) {
                Case t{entry,"optional electorate/recipient"};t.electorate=mode;t.club=club;t.peer=peer;t.enabled=enabled;cases.push_back(t);
            }
        for (int left:{-1,0,1,31}) for (int right:{-1,0,1,31}) {
            Case t{entries[11],"name sentinel/tie"};t.left_id=left;t.right_id=right;cases.push_back(t);
        }
        for (int left:{-32768,-1,0,1,32767}) for (int right:{-32768,-1,0,1,32767}) {
            Case t{entries[13],"signed vote ordering"};t.votes=left;t.ballot=right;cases.push_back(t);
        }
        for (auto bound:{0u,1u,2u,100u,65535u,65536u,655350u,0xffffffffu,0xffff0001u,0xffff0000u,0x7fffffffu,0x80000000u,0x80000001u})
            for (int wrap:{0,1}) { Case t{entries[7],"sample "+hex_address(bound)};t.bound=bound;t.wrap=wrap;cases.push_back(t); }
        for (unsigned bias:{0u,65535u,0x7fffffffu,0x80000000u,0xfffffff0u})
            for (unsigned bound:{1u,2u,65536u,655350u,0xffff0000u,0x7fffffffu,0x80000000u})
                for (int rng:{0,65535}) {
                    Case t{entries[7],"sample bias/wrapped arithmetic"};t.bias=bias;t.bound=bound;t.rng=rng;cases.push_back(t);
                }
        for (int mode=0;mode<8;++mode) for (int wrap:{0,1}) for (int limit:{-1,0,1,3,4,5}) {
            Case t{entries[6],"vote draw limits"};t.electorate=mode;t.wrap=wrap;t.limit=limit;cases.push_back(t);
        }
        for (unsigned bits:{0u,0x80000000u,0x3f800000u,0xbf800000u,0x7f800000u,0xff800000u,0x7fc12345u})
            for (unsigned other:{0u,0x3f800000u,0x7f800000u,0x7fc12345u}) {
                Case t{entries[12],"score FP edges"};t.score_bits=bits;t.other_bits=other;cases.push_back(t);
            }
        if (const auto path=std::getenv("CM_AWARD_SHORTLIST_CASE_MANIFEST")) {
            std::ofstream manifest(path);if (!manifest) throw std::runtime_error("Cannot write case manifest");
            for (unsigned i=0;i<cases.size();++i) manifest<<i<<' '<<hex_address(cases[i].entry)<<' '<<cases[i].name<<'\n';
        }
        unsigned failures=0,completed=0;std::map<unsigned,std::array<unsigned,2>> per_entry;
        for (bool production:{false,true}) {
            production_mode=production;
            if (production && std::getenv("CM_AWARD_SHORTLIST_CONTROLLED_ONLY")) continue;
            for (unsigned i=0;i<cases.size();++i) {
                const auto& test=cases[i];
                if (const auto only=std::getenv("CM_AWARD_SHORTLIST_CASE")) if (i!=std::stoul(only)) continue;
                auto state=cm::semantic_io_checks::clone(*base);Cpu c(*state);active=test;calls=io_calls=0;
                initialize(c,test);state->pc=test.entry;ObservedWrites observer;if (production) state->memory.observer=&observer;
                const auto found=compare_routine_call(*state,test.entry,original,rewrite,RoutineChecker::result_r3|RoutineChecker::result_f1);
                if (found.empty()) { ++completed;++per_entry[test.entry][production?1:0]; }
                else {
                    std::cout<<"DIFFERS "<<(production?"production ":"controlled ")<<i<<' '<<hex_address(test.entry)<<' '<<test.name<<'\n';
                    for (const auto& difference:found) std::cout<<"  "<<difference<<'\n';
                    if (++failures>=12) break;
                }
            }
            if (failures>=12) break;
        }
        production_mode=false;
        if (!std::getenv("CM_AWARD_SHORTLIST_CONTROLLED_ONLY")) for (auto entry:entries) {
            active=Case{entry,"production ownership"};calls=io_calls=0;
            auto controlled=cm::semantic_io_checks::clone(*base);Cpu cc(*controlled);initialize(cc,active);
            rewrite(*controlled,entry);
            const auto allowance=controlled->executed;
            auto normal=cm::semantic_io_checks::clone(*base);Cpu nc(*normal);initialize(nc,active);
            normal->budget=allowance;ObservedWrites observer;normal->memory.observer=&observer;
            production_mode=true;calls=io_calls=0;
            try { rewrite(*normal,entry);if (normal->executed!=allowance) throw std::runtime_error("unexpected original instruction count"); }
            catch (const std::exception& error) { ++failures;std::cout<<"FAIL production owner "<<hex_address(entry)<<' '<<error.what()<<'\n'; }
            production_mode=false;
            // An original-only route must exhaust precisely that same budget.
            auto reference=cm::semantic_io_checks::clone(*base);Cpu rc(*reference);initialize(rc,active);
            reference->budget=allowance;calls=io_calls=0;bool exhausted=false;
            try { original(*reference,entry); } catch (const std::exception& error) {
                exhausted=std::string(error.what()).find("budget")!=std::string::npos;
            }
            if (!exhausted) { ++failures;std::cout<<"FAIL original ownership control "<<hex_address(entry)<<'\n'; }
        }
        for (const auto runner:{wrong_result,wrong_memory,wrong_rng}) {
            active=Case{runner==wrong_rng?entries[7]:entries[0],"corruption control"};calls=io_calls=0;
            auto state=cm::semantic_io_checks::clone(*base);Cpu c(*state);initialize(c,active);
            const auto found=compare_routine_call(*state,active.entry,original,runner,RoutineChecker::result_r3|RoutineChecker::result_f1);
            if (found.empty() || found.front().find("stopped")!=std::string::npos) { ++failures;std::cout<<"FAIL corruption oracle\n"; }
            else std::cout<<"ok corruption detected\n";
        }
        for (const auto& [entry,count]:per_entry) std::cout<<hex_address(entry)<<" returned "<<count[0]<<" controlled, "<<count[1]<<" production\n";
        std::cout<<completed<<" returned comparisons, "<<failures<<" failures\n";
        return failures?1:0;
    } catch (const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; }
}
