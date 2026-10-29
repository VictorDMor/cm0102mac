// Returned original/C++ comparisons. External services are deterministic
// boundary fixtures; owned nested routines execute their complete bodies.
#include "dispute_adapter.hpp"
#include "dispute.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#if defined(CM_DISPUTE_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
#endif
using namespace cm::aot;
using guest::Cpu;
namespace cm::aot { bool dispute_reference(State&); }
namespace {
constexpr std::uint32_t data=0x60000000,object=data+0x1000,flags=data+0x2000,
    list=data+0x3000,nodes=data+0x3800,records=data+0x4000,people=data+0x10000,clubs=data+0x20000,
    trace_at=data+0x80000,io_at=data+0x100000,stack=0x71010000,sentinel=0xfffffff0;
constexpr std::array entries{0x1070bd00u,0x1070bde0u,0x1070be60u,0x1070c040u,
    0x1070c3f0u,0x1070c510u,0x1070c710u,0x1070c7f0u,0x1070c8b0u,
    0x1070c8f0u,0x1070d0d0u,0x1070d650u,0x1070d820u};
struct Case {
    std::uint32_t entry;
    std::string name="default";
    int count=2,storage=3,person=1,marked=1,file=1,version=0x3e046,
        fail_allocation=0,fail_io=0,broken_iterator=-1,expiry=0,club=1,
        phase=0,reason=1,temporary=0,detailed=1,attribute=8000,trait=20,
        peer_trait=1,rng=10,wrap=0,unchecked=0,enabled=1,filter=0,
        service=1,person_id=1,record_club=1,contract=0,mutate=0,
        expiry_year=2002,flag_count=16,nation=1,recipient=1,attributes=1,partial_bytes=-1,
        argument_club=1,miss=0,news_club=1;
    std::uint32_t bound=100;
    std::uint32_t bias=0;
};
Case active{entries[0]};
unsigned calls=0,allocations=0,io_calls=0;
bool original_mode=false,production_mode=false,fatal_boundary=false;
struct NullListBoundary {};
std::uint32_t next_allocation=0;
const std::set<std::uint32_t> dependencies{
    0x10003dc0,0x10082f20,0x100cb590,0x100cbde0,0x100d9480,0x100d9520,
    0x100d97a0,0x100d9b80,0x100d9db0,0x100d9e10,0x1014c2e0,
    0x102bdf90,0x102cb4b0,0x102cb570,0x102cc400,0x102cc8e0,0x103e6290,
    0x10715880,0x10717890,0x10727890,0x10738c00,0x10738c20,0x10738d10,
    0x10738e00,0x10738e60,0x10739890,0x107475b0,0x10747900,0x10747a80,
    0x10747cc0,0x1074b830,0x1074bab0,0x10768810,0x10769b60,0x10773520,
    0x107735a0,0x10775130,0x10775170,0x10775200,0x10775550,0x1077b720,0x1077b770};
void trace(Cpu& c,std::uint32_t target) {
    if (++calls>2000) throw std::runtime_error("fixture call limit");
    auto count=c.u32(trace_at);
    const auto add=[&](std::uint32_t value) { c.put32(trace_at+4+count++*4,value); };
    add(target);add(c.lr());
    for (unsigned r=3;r<=10;++r) add(c.r(r));
    for (unsigned r=13;r<32;++r) add(c.r(r));
    for (unsigned f=0;f<8;++f) add(c.cr(f));
    c.put32(trace_at,count);
}
bool probe(State& s) {
    Cpu c(s);const auto target=s.pc;
    if (run_leaf(s)) return true;
    if (target>=entries[0] && target<0x1070dd80) {
        if (original_mode) return dispute_reference(s);
        if (production_mode) return false;
        if (dispute_lists(target)) { dispute_run_rewrite(s,target);return true; }
        return false;
    }
    if (!dependencies.contains(target)) return false;
    trace(c,target);
    if (target==0x10773520 || target==0x10775130 || target==0x10775200) {
        ++allocations;
        if (int(allocations)==active.fail_allocation) c.set(3,0);
        else {
            const auto at=next_allocation;
            const auto size=c.r(3)*(target==0x10775200?c.r(4):1);
            next_allocation+=(size+31)&~15u;
            if (target==0x10775200) for (unsigned i=0;i<size;++i) c.put8(at+i,0);
            c.set(3,at);
        }
    } else if (target==0x10775550) c.set(3,active.version);
    else if (target==0x107475b0) c.set(3,active.fail_io==1?0:data+0x9000);
    else if (target==0x10747a80 || target==0x10747cc0) {
        ++io_calls;
        const auto requested=c.r(5),width=c.r(4),address=c.r(3);
        const auto failed=active.fail_io==int(io_calls+1);
        const auto bytes=failed && active.partial_bytes>=0?std::min(width*requested,std::uint32_t(active.partial_bytes)):width*requested;
        if (target==0x10747a80) {
            for (unsigned i=0;i<bytes;++i) {
                const auto value=io_calls==1?std::uint32_t(active.flag_count):std::uint32_t(active.count);
                const auto byte=io_calls==1 || io_calls==3?(value>>(24-i*8))&0xff:io_calls==2?i&1:0x30+io_calls+i;
                c.put8(address+i,byte);
            }
        } else {
            const auto at=io_at+c.u32(io_at);
            for (unsigned i=0;i<bytes;++i) c.put8(at+4+i,c.u8(address+i));
            c.put32(io_at,c.u32(io_at)+bytes);
        }
        c.set(3,failed?(active.partial_bytes<0?0:bytes/width):requested);
    } else if (target==0x10738c00) {
        c.put16(c.r(3),0);c.put32(c.r(3)+4,0);c.put32(c.r(3)+8,0);c.put32(c.r(3)+12,0);
    } else if (target==0x10738d10) {
        if (fatal_boundary && !c.r(3)) throw NullListBoundary{};
        const auto count=c.i16(c.r(3));
        const auto destination=records+std::uint32_t(count)*20;
        for (unsigned i=0;i<c.r(5);++i) c.put8(destination+i,c.u8(c.r(4)+i));
        const auto node=nodes+std::uint32_t(count)*16,tail=c.u32(c.r(3)+8);
        c.put32(node,destination);c.put16(node+4,20);c.put32(node+8,0);c.put32(node+12,tail);
        if (tail) c.put32(tail+8,node);else c.put32(c.r(3)+4,node);
        c.put32(c.r(3)+8,node);
        c.put16(c.r(3),count+1);
    } else if (target==0x10738e00) {
        const auto at=c.u32(c.r(3)+12);
        c.set(5,at);c.compare_logical(0,at,0);
        if (at) { c.set(4,c.u32(at));c.put32(c.r(3)+12,c.u32(at+8));c.set(3,c.r(4)); }
        else c.set(3,0);
    } else if (target==0x10738e60) {
        const auto owner=c.r(3),payload=c.r(4);
        auto node=c.u32(owner+4);
        while (node && c.s.memory.copy_out(c.u32(node),20)!=c.s.memory.copy_out(payload,20)) node=c.u32(node+8);
        if (node) {
            const auto next=c.u32(node+8),previous=c.u32(node+12);
            if (previous) c.put32(previous+8,next);else c.put32(owner+4,next);
            if (next) c.put32(next+12,previous);else c.put32(owner+8,previous);
            if (c.u32(owner+12)==node) c.put32(owner+12,next);
            c.put16(owner,c.i16(owner)-1);c.set(3,1);
        } else c.set(3,0);
    } else if (target==0x10739890) {
        c.put32(c.r(6),0x44495300);c.put32(c.r(7),0x43505000);
    } else if (target==0x1077b720) c.put32(c.r(3),c.r(8));
    else if (target==0x1077b770) c.set(3,0x1234);
    else if (target==0x10715880) { c.put32(c.r(3),0);c.put32(c.r(3)+4,0); }
    else if (target==0x10717890) { c.put16(c.r(3),123);c.put16(c.r(3)+2,2002);c.put8(c.r(3)+4,3); }
    else if (target==0x10082f20) c.set(3,active.contract?data+0xc000:0);
    else if (target==0x10003dc0) { c.set(3,active.filter); if (active.mutate) c.put8(flags+active.person_id,1); }
    else if (target==0x1014c2e0 || target==0x100d9520 || target==0x100d9480 || target==0x100cbde0) c.set(3,active.service);
    else if (target==0x102bdf90) { c.put32(c.r(3),c.r(4)); }
    else if (target==0x102cb4b0) c.put32(c.r(3)+8+c.r(4)*4,c.r(5));
    else if (target==0x102cb570) c.set(3,c.u32(c.r(3)+8+c.r(4)*4));
    else if (target==0x100cb590 || target==0x10727890) c.put32(target==0x100cb590?c.r(6):c.r(3),0x41424300);
    else if (target==0x102cc400 || target==0x102cc8e0) {
        // Capture all published fields independently of formatter behavior.
        for (unsigned i=0;i<48;++i) c.put8(data+0xa000+i,c.u8(c.r(4)+i));
    }
    s.pc=c.lr();return true;
}
void initialize(Cpu& c,const Case& test) {
    c.s.memory.map(data,0x200000);c.s.memory.map(stack-0x10000,0x20000);
    c.s.memory.copy_in(stack-0x10000,std::vector<std::uint8_t>(0x20000,0xa5));
    for (unsigned r=0;r<32;++r) c.set(r,0x51000000+r*0x101);
    c.set(1,stack);c.set(2,0x108bba90);c.set_lr(sentinel);
    for (unsigned f=0;f<8;++f) c.set_cr(f,f+1);
    const auto variable=[&](int slot,unsigned off,std::uint32_t value) {
        c.put32(c.r(2)+std::uint32_t(slot),data+off);c.put32(data+off,value);
    };
    variable(-0x7be4,0x100,16);variable(-0x7be0,0x104,people);variable(-0x7be8,0x108,clubs);
    variable(-0x7b50,0x10c,8);variable(-0x7b74,0x110,test.version);variable(-0x7b7c,0x114,data+0xb000);
    variable(-0x7b78,0x118,0);variable(-0x7a68,0x11c,test.enabled);variable(-0x7b24,0x120,0);
    c.put8(data+0x11c,test.enabled);
    variable(-0x7b9c,0x12c,data+0xd000);c.put32(data+0xb000,data+0xc000);
    for (unsigned i=0;i<16;++i) c.put32(data+0xd000+i*100,0xffffffff);
    if (test.contract && test.person_id>=0 && test.person_id<16) c.put32(data+0xd000+test.person_id*100,0);
    variable(-0x7bf0,0x124,0);variable(-0x7b40,0x128,0);
    c.put32(c.r(2)-0x7bac,data+0x130);c.put8(data+0x130,1);
    c.put32(c.r(2)-0x7ba8,data+0x140);c.put32(c.r(2)-0x7bb8,data+0x180);
    c.put32(c.r(2)-0x7b80,data+0x1c0);c.put16(data+0x1c0,100);c.put16(data+0x1c2,2002);
    constexpr unsigned cursor_offsets[]={0,16,14,10,12,8};
    variable(-0x7b90,0x200,test.bias);variable(-0x7b84,0x204,data+0xe000+cursor_offsets[test.wrap]);
    variable(-0x7b88,0x208,data+0xe010);c.put32(c.r(2)-0x7b8c,data+0xe000);
    for (unsigned i=0;i<32;i+=2) c.put16(data+0xe000+i,test.rng+i);
    c.put32(object,test.storage&1?list:0);c.put32(object+4,test.storage&2?flags:0);
    c.put16(list,test.count);c.put32(list+4,test.count?nodes:0);
    c.put32(list+8,test.count?nodes+(test.count-1)*16:0);c.put32(list+12,test.count?nodes:0);
    for (unsigned i=0;i<16;++i) {
        const auto person=people+i*0x70,club=clubs+(i%8)*0x248;
        c.put32(person,i);c.put32(person+0x14,data+0x30000+i*0x40);
        c.put32(person+0x24,club);c.put32(person+12,data+0x6000);
        for (unsigned k=0;k<3;++k) { c.put32(person+0x28+k*4,data+0x7000+k*0x40);c.put32(data+0x7020+k*0x40,k+1); }
        c.put8(person+0x65,test.trait);c.put8(person+0x67,test.peer_trait);
        for (unsigned off:{8u,10u,12u}) c.put16(data+0x30000+i*0x40+off,test.attribute);
        c.put32(club,i%8);c.put32(club+0x90,people+2*0x70);c.put32(club+0x80,people+3*0x70);
        c.put32(club+0x2c,data+0x6000);c.put32(data+0x6068,7);
        c.put8(club+0x23b,1);
    }
    c.put32(clubs+0x248+0x98,people+4*0x70);
    c.put32(clubs+0x248+0x9c,people+0x70); // self must be skipped.
    for (int i=0;i<test.count;++i) {
        c.put32(nodes+i*16,i==test.broken_iterator?0:records+i*20);c.put16(nodes+i*16+4,20);
        c.put32(nodes+i*16+8,i+1<test.count?nodes+(i+1)*16:0);c.put32(nodes+i*16+12,i?nodes+(i-1)*16:0);
        c.put32(records+i*20,i==0 && !test.miss?test.person_id:5);
        c.put32(records+i*20+4,test.record_club);
        c.put16(records+i*20+8,100+test.expiry);c.put16(records+i*20+10,test.expiry_year);
        c.put8(records+i*20+16,test.temporary);
    }
    c.put8(flags+test.person_id,test.marked);c.put32(people+0x70,test.person_id);
    const auto person=people+0x70;
    if (!test.club) c.put32(person+0x24,0);
    if (!test.nation) c.put32(clubs+0x248+0x2c,0);
    if (!test.recipient) c.put32(clubs+0x248+0x90,0);
    if (!test.attributes) c.put32(person+0x14,0);
    c.put32(data+0xc004,test.contract==2?0:1);
    c.set({{3,object},{4,test.person?person:0},{5,std::uint32_t(test.unchecked)}});
    if (test.entry==entries[0] || test.entry==entries[2] || test.entry==entries[3]) c.set(4,test.file?data+0x9000:0);
    if (test.entry==entries[1]) { if (!test.person) c.set(3,0);c.set(4,test.phase); }
    if (test.entry==entries[8]) c.set(4,test.person?records:0);
    if (test.entry==entries[9]) { c.set(5,clubs+0x248);c.set(6,test.reason);c.set(7,test.temporary); }
    if (test.entry==entries[10]) c.set(3,test.bound);
    if (test.entry==entries[11]) { c.set(5,test.argument_club?clubs+0x248:0);c.set(6,test.phase);c.set(7,test.reason);c.set(8,test.temporary); }
    if (test.entry==entries[12]) {
        c.set(4,data+0x5000);c.set(5,test.detailed);c.set(6,data+0x5800);
        const std::array<std::uint32_t,10> values{1,1,2,3,std::uint32_t(test.news_club),1,std::uint32_t(test.reason),0,0,std::uint32_t(test.phase)};
        for (unsigned i=0;i<values.size();++i) c.put32(data+0x5008+i*4,values[i]);
    }
    c.s.budget=1000000;
}
void postconditions(State& s) {
    Cpu c(s);if (c.sp()!=stack || c.lr()!=sentinel) throw std::runtime_error("fixture did not return");
}
void original(State& s,std::uint32_t entry) {
    original_mode=true;s.pc=entry;
    try { run_until(s,sentinel); }
    catch (const NullListBoundary&) {
        Cpu c(s);if (!fatal_boundary || s.pc!=0x10738d10 || c.r(3)!=0 || c.lr()!=0x1070c008)
            throw std::runtime_error("wrong fatal dependency boundary");
        return;
    }
    postconditions(s);
}
void rewrite(State& s,std::uint32_t entry) {
    original_mode=false;
    try {
        if (production_mode) { s.pc=entry;run_until(s,sentinel); }
        else dispute_run_rewrite(s,entry);
        postconditions(s);
    } catch (const NullListBoundary&) {
        Cpu c(s);if (!fatal_boundary || s.pc!=0x10738d10 || c.r(3)!=0 || c.lr()!=0x1070c008)
            throw std::runtime_error("wrong fatal dependency boundary");
    }
#if defined(CM_DISPUTE_COVERAGE)
    __llvm_profile_write_file();
#endif
}
void wrong_memory(State& s,std::uint32_t entry) {
    rewrite(s,entry);Cpu c(s);c.put8(flags,c.u8(flags)^1);
}
void wrong_result(State& s,std::uint32_t entry) {
    rewrite(s,entry);Cpu c(s);c.set(3,c.r(3)^1);
}
void wrong_rng(State& s,std::uint32_t entry) {
    rewrite(s,entry);Cpu c(s);c.put32(data+0x204,c.u32(data+0x204)+2);
}
void wrong_bias(State& s,std::uint32_t entry) {
    rewrite(s,entry);Cpu c(s);c.put32(data+0x200,c.u32(data+0x200)^1);
}
struct ObservedWrites final:MemoryObserver {
    unsigned count=0;
    void mapped(std::uint32_t,std::uint64_t) override {}
    void written(std::uint32_t,const std::uint8_t*,std::size_t) override { ++count; }
    void cleared(std::uint32_t,std::uint64_t) override { ++count; }
};
}
int main(int argc,char** argv) {
    if (argc!=2) { std::cerr<<"Usage: cm-dispute-checks <original-segments>\n";return 2; }
    try {
        auto base=std::make_unique<State>();load_program(*base,argv[1]);dispatch_probe=probe;
        std::vector<Case> cases;
        for (auto entry:entries) { Case t{entry};if (entry==entries[9]) t.marked=0;cases.push_back(t); }
        for (auto entry:{entries[0],entries[1],entries[2],entries[3]}) {
            for (int n=0;n<=3;++n) { Case t{entry,"count "+std::to_string(n)};t.count=n;cases.push_back(t); }
            for (int n=1;n<=3;++n) {
                Case t{entry,"allocation "+std::to_string(n)};t.fail_allocation=n;
                // A failed list allocation with populated input makes the
                // original dereference null in its external append service.
                // Empty input separately exercises the returned null-list path.
                if (n==2) t.count=0;
                cases.push_back(t);
            }
            for (int n=1;n<=7;++n) { Case t{entry,"IO "+std::to_string(n)};t.fail_io=n;cases.push_back(t); }
            Case t{entry,"no file"};t.file=0;cases.push_back(t);
        }
        for (auto entry:{entries[4],entries[6],entries[7],entries[8],entries[11]}) {
            for (int p:{0,1}) for (int m:{0,1,255}) { Case t{entry,"person/mark"};t.person=p;t.marked=m;cases.push_back(t); }
        }
        for (auto entry:{entries[5],entries[6],entries[7]}) for (int n:{0,1,3}) for (int broken:{-1,0,1}) {
            Case t{entry,"iterator"};t.count=n;t.broken_iterator=broken;cases.push_back(t);
        }
        for (auto entry:{entries[4],entries[9]}) for (int enabled:{0,1}) for (int unchecked:{0,1,255}) {
            Case t{entry,"enabled/unchecked"};t.enabled=enabled;t.unchecked=unchecked;t.marked=entry==entries[9]?0:1;cases.push_back(t);
        }
        for (int delta:{-1,0,1}) for (int temp:{0,1}) for (int club:{0,1}) {
            Case t{entries[5],"daily"};t.expiry=delta;t.temporary=temp;t.club=club;cases.push_back(t);
        }
        for (auto bound:{0u,1u,2u,100u,0xffffffffu,0xfffffff0u,65535u,65536u,655350u,0xffff0001u,0xffff0000u,0x7fffffffu,0x80000000u,0x80000001u})
            for (int wrap:{0,1,2}) { Case t{entries[10],"sample "+hex_address(bound)};t.bound=bound;t.wrap=wrap;cases.push_back(t); }
        for (int detailed:{0,1,255}) for (int phase:{-1,0,1,2,3,4}) for (int reason:{0,1,2,3,4}) {
            Case t{entries[12],"render"};t.detailed=detailed;t.phase=phase;t.reason=reason;cases.push_back(t);
        }
        for (int temp:{0,1,255}) for (int rng:{0,8,20,65535}) for (int wrap:{0,1}) {
            Case t{entries[9],"start"};t.marked=0;t.temporary=temp;t.rng=rng;t.wrap=wrap;cases.push_back(t);
        }
        for (auto entry:{entries[1],entries[5],entries[6],entries[7]}) for (int storage=0;storage<4;++storage) {
            Case t{entry,"optional storage"};t.storage=storage;
            cases.push_back(t);
        }
        for (int deleting:{-32768,-1,0,1,32767,65535}) { Case t{entries[1],"destructor mode"};t.phase=deleting;cases.push_back(t); }
        { Case t{entries[1],"null destructor"};t.person=0;cases.push_back(t); }
        for (int version:{0,0x3e045,0x3e046,0x3e047}) { Case t{entries[2],"save version"};t.version=version;cases.push_back(t); }
        for (int count:{0,1,16}) { Case t{entries[2],"flag bytes"};t.flag_count=count;cases.push_back(t); }
        for (int broken:{0,1}) { Case t{entries[3],"save missing iterator"};t.broken_iterator=broken;cases.push_back(t); }
        for (int id:{-1,0,14,15,16}) {
            Case t{entries[4],"person index"};t.person_id=id;
            if (id>=0 && id<16) cases.push_back(t);
            // The original active() range check permits negative IDs; these
            // mapped predecessor bytes characterize that compatibility path.
            else if (id<0) cases.push_back(t);
            else { t.marked=0;cases.push_back(t); }
        }
        for (int year:{2001,2002,2003}) for (int id:{-1,0,14,15,16}) for (int record_club:{0,1,2}) {
            Case t{entries[5],"daily record eligibility"};t.expiry=-1;t.expiry_year=year;t.person_id=id;
            // The caller validates record IDs before deriving person pointers.
            t.record_club=record_club;cases.push_back(t);
        }
        for (auto entry:{entries[9],entries[11]}) for (int attribute:{0,7499,7500,7501,32767,-32768})
            for (int service:{0,1}) for (int nation:{0,1}) {
                Case t{entry,"attributes/service/nation"};t.attribute=attribute;t.service=service;t.nation=nation;t.marked=0;cases.push_back(t);
            }
        for (int trait:{-128,-1,0,1,6,20,127}) for (int peer_trait:{-128,-1,0,1,127}) {
            Case t{entries[9],"peer traits"};t.marked=0;t.trait=trait;t.peer_trait=peer_trait;cases.push_back(t);
        }
        for (int enabled:{0,1}) for (int filter:{0,1}) for (int mutate:{0,1}) {
            Case t{entries[9],"start gates"};t.marked=0;t.enabled=enabled;t.filter=filter;t.mutate=mutate;cases.push_back(t);
        }
        for (int marked:{0,1}) { Case t{entries[9],"existing flag"};t.marked=marked;cases.push_back(t); }
        for (int entry_index:{4,9,11}) for (int person:{0,1}) for (int club:{0,1}) {
            Case t{entries[entry_index],"null input gate"};t.person=person;t.club=club;t.marked=entry_index==9?0:1;cases.push_back(t);
        }
        for (auto entry:{entries[9],entries[11]}) { Case t{entry,"missing attributes"};t.attributes=0;t.marked=0;cases.push_back(t); }
        for (int contract:{1,2}) { Case t{entries[4],"present contract"};t.contract=contract;cases.push_back(t); }
        for (int recipient:{0,1}) for (int service:{0,1}) {
            Case t{entries[5],"daily recipients"};t.expiry=-1;t.recipient=recipient;t.service=service;cases.push_back(t);
        }
        for (auto entry:{entries[2],entries[3]}) for (int partial:{0,1,7,15}) {
            Case t{entry,"partial flag IO"};t.fail_io=3;t.partial_bytes=partial;cases.push_back(t);
        }
        for (auto entry:{entries[2],entries[3]}) for (int partial:{0,1,10,19}) {
            Case t{entry,"partial record IO"};t.fail_io=5;t.partial_bytes=partial;cases.push_back(t);
        }
        { Case t{entries[11],"missing publication club argument"};t.argument_club=0;cases.push_back(t); }
        for (auto entry:{entries[6],entries[7]}) { Case t{entry,"list scan without match"};t.miss=1;cases.push_back(t); }
        for (int service:{0,1}) for (int club:{-1,0,6,7,8}) for (int detailed:{0,1}) {
            Case t{entries[12],"link gates and sentinel club"};t.service=service;t.news_club=club;t.detailed=detailed;cases.push_back(t);
        }
        for (auto bias:{0x7fffffffu,0x80000000u,0xfffffff0u}) {
            // Deliberately wide bias words characterize mapped arithmetic
            // boundaries; ordinary RNG reseeding stores only sixteen bits.
            for (auto bound:{65536u,655350u,0xffff0000u,0x7fffffffu}) {
                Case t{entries[10],"noncanonical wide RNG bias"};t.bias=bias;t.bound=bound;cases.push_back(t);
            }
            Case t{entries[9],"noncanonical wide RNG bias"};t.marked=0;t.bias=bias;cases.push_back(t);
        }
        for (int wrap:{3,4,5}) for (int temp:{0,1}) {
            Case t{entries[9],"late RNG wrap"};t.marked=0;t.wrap=wrap;t.temporary=temp;t.trait=0;cases.push_back(t);
        }
        { Case t{entries[9],"invalid person upper bound"};t.marked=0;t.person_id=16;cases.push_back(t); }
        if (const auto path=std::getenv("CM_DISPUTE_CASE_MANIFEST")) {
            std::ofstream manifest(path);
            if (!manifest) throw std::runtime_error("Cannot create private case manifest");
            for (unsigned i=0;i<cases.size();++i)
                manifest<<i<<' '<<hex_address(cases[i].entry)<<' '<<cases[i].name<<'\n';
        }
        unsigned failures=0;
        unsigned completed=0;
        std::map<std::uint32_t,std::array<unsigned,2>> per_entry;
        for (bool production:{false,true}) {
          production_mode=production;
          for (const auto& test:cases) {
            auto s=cm::semantic_io_checks::clone(*base);Cpu c(*s);active=test;calls=allocations=io_calls=0;next_allocation=data+0x40000;
            initialize(c,test);s->pc=test.entry;
            ObservedWrites observer;
            if (production) s->memory.observer=&observer;
            const auto differences=compare_routine_call(*s,test.entry,original,rewrite,RoutineChecker::result_r3|RoutineChecker::result_f1);
            ++completed;
            ++per_entry[test.entry][production?1:0];
            if (!differences.empty()) {
                std::cout<<"DIFFERS "<<(production?"production ":"controlled ")<<hex_address(test.entry)<<" "<<test.name<<'\n';
                for (const auto& difference:differences) std::cout<<"  "<<difference<<'\n';
                if (++failures>=15) break;
            }
          }
          if (failures>=15) break;
        }
        production_mode=false;
        active=cases.front();calls=allocations=io_calls=0;next_allocation=data+0x40000;
        auto s=cm::semantic_io_checks::clone(*base);Cpu c(*s);initialize(c,active);
        const auto negative=compare_routine_call(*s,active.entry,original,wrong_result,RoutineChecker::result_r3);
        if (negative.size()!=1 || !negative.front().starts_with("r3 ")) { ++failures;std::cout<<"FAIL corrupted result oracle\n"; }
        else std::cout<<"ok corrupted result detected\n";
        for (const auto runner:{wrong_memory,wrong_rng,wrong_bias}) {
            active=Case{runner==wrong_memory?entries[0]:entries[10],"corruption control"};active.wrap=1;
            calls=allocations=io_calls=0;next_allocation=data+0x40000;
            auto control=cm::semantic_io_checks::clone(*base);Cpu cc(*control);initialize(cc,active);
            const auto found=compare_routine_call(*control,active.entry,original,runner,RoutineChecker::result_r3);
            if (found.empty() || found.front().find("stopped")!=std::string::npos) { ++failures;std::cout<<"FAIL memory/RNG oracle\n"; }
            else std::cout<<"ok memory/RNG corruption detected\n";
        }
        for (auto entry:{entries[0],entries[2]}) {
            Case diagnostic{entry,"original populated load allocation fault"};diagnostic.fail_allocation=2;
            active=diagnostic;calls=allocations=io_calls=0;next_allocation=data+0x40000;
            auto diagnostic_state=cm::semantic_io_checks::clone(*base);Cpu dc(*diagnostic_state);initialize(dc,diagnostic);
            const auto found=compare_routine_call(*diagnostic_state,entry,original,rewrite,RoutineChecker::result_r3);
            if (found.size()!=2 || found[0]!="the original stopped: Unmapped data access at 0x00000000" ||
                found[1]!="the rewrite stopped: Unmapped data access at 0x00000000") { ++failures;std::cout<<"FAIL nonreturn diagnostic oracle\n"; }
            else std::cout<<"diagnostic: matching internal faults excluded from equivalence\n";
            fatal_boundary=true;
            const auto prefix=compare_routine_call(*diagnostic_state,entry,original,rewrite,RoutineChecker::result_r3|RoutineChecker::result_f1);
            fatal_boundary=false;
            if (!prefix.empty()) { ++failures;for (const auto& difference:prefix) std::cout<<"boundary DIFFERS "<<difference<<'\n'; }
            else std::cout<<"ok expected null-list dependency boundary (prefix only)\n";
        }
        for (const auto& [entry,counts]:per_entry)
            std::cout<<"entry "<<hex_address(entry)<<": "<<counts[0]<<" controlled, "<<counts[1]<<" production\n";
        std::cout<<completed<<" of "<<cases.size()*2<<" returned comparison attempts (controlled + observed production), "<<failures<<" failed\n";
        return failures?1:0;
    } catch (const std::exception& error) { std::cerr<<error.what()<<'\n';return 2; }
}
