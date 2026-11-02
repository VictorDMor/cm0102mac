// Returned comparisons against private hook-free originals. Scripted external
// boundaries expose call order, arguments, CR, preserved registers and return
// sites in compared memory. Additional cases execute actual date/row services.
#include "eng_first_adapter.hpp"
#include "eng_first_detail.hpp"
#include "match_replay.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <random>
#include <unistd.h>

#if defined(CM_ENG_FIRST_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
extern "C" void __llvm_profile_set_filename(const char*);
#endif
using namespace cm::aot;
using guest::Cpu;
namespace cm::aot { bool eng_first_reference(State&); }
namespace {
constexpr std::uint32_t data=0x60000000, object=data+0x1000, clubs=data+0x10000,
    definitions=data+0x3000, log_at=data+0x40000, sentinel=0xfffffff0,
    failed_configuration=0xf0000020, child_destructor=0xf0000010, child_table=data+0x1100, child_descriptor=data+0x1200;
constexpr std::uint32_t entries[]{0x105267c0,0x10526af0,0x10526c00,0x10528c60,0x10528e50,0x10528f20,0x105291c0,0x10529280,0x10529470,0x105295f0,0x105296d0};
struct Scenario {
    std::uint32_t entry;
    std::string name;
    unsigned club_total=28, members=24, storage=7;
    int day=16, stage=-1, year=2001, fail_allocation=0, fail_call=0, fail_virtual=-1;
    int deleting=0, last_stage=2;
    bool null_object=false, output_flag=true, mixed=false, live_calendar=false, live_clubs=false;
    int mutate_stages=0, output_alias=0;
    int event=1, first=2, second=3, third=6, entrant_count=24, match_row=3, stage_capacity=1;
    unsigned option=0x83, description_detail=0;
    bool special_description=true;

};
Scenario active{};
unsigned allocations=0,calls=0;
std::uint32_t next_allocation=0;
bool reference_mode=false,production_mode=false,spoil_trace=false;
bool real_state_mode=false;
std::uint32_t real_stack=0, real_object=0;
const std::set<std::uint32_t> dependencies{
    0x1049e4f0,0x104a00a0,0x1049eb30,0x10773520,0x104533a0,0x104a6660,
    0x10775170,0x104536c0,0x1049e600,0x107735a0,0x10775130,0x104a60e0,
    0x1049f540,0x1049f6d0,0x104a9810,0x104a9890,0x107156f0,0x10739890,
    0x10768810,0x1077b720,0x10769b60,0x104564a0,0x104aa3a0,0x102d46c0,
    0x104854f0,0x10485570,0x104bc3c0,0x1047ef50,0x10459b50,
    0x10381b70,0x103822e0,0x10384500,0x10382e50,0x10727890,0x1074b830,0x1074bab0,0x10450030};
void trace(Cpu& c,std::uint32_t target) {
    if (++calls>2000) throw std::runtime_error("fixture call limit");
    auto index=c.u32(log_at);
    const auto add=[&](std::uint32_t word){ c.put32(log_at+4+index++*4,word); };
    add(target);add(c.lr());
    for(unsigned r=3;r<11;++r)add(c.r(r) ^ (spoil_trace && r==4?1u:0u));
    for(unsigned r=13;r<32;++r)add(c.r(r));
    for(unsigned f=0;f<8;++f)add(c.cr(f));
    if(target==0x104a9810 || target==0x10485570)add(c.u32(c.sp()+0x38));
    if(target==0x104bc3c0)for(unsigned offset=0x38;offset<=0x54;offset+=4)add(c.u32(c.sp()+offset));
    if(target==0x10450030)for(unsigned offset=0x38;offset<=0x40;offset+=4)add(c.u32(c.sp()+offset));
    c.put32(log_at,index);
}
bool probe(State& s) {
    Cpu c(s);const auto target=s.pc;
    if(run_leaf(s))return true;
    if(eng_first_lists(target)) {
        if(reference_mode)return eng_first_reference(s);
        if(production_mode)return false;
        eng_first_run_rewrite(s,target);return true;
    }
    if(reference_mode && target>=0x105267c0 && target<0x10529834)return eng_first_reference(s);
    if(real_state_mode)return false;
    if(target==0x10781320) {
        const auto descriptor=c.r(12),entry=c.u32(descriptor);
        c.put32(c.sp()+0x14,c.r(2));
        const unsigned method=(entry==0x10528e50 || entry==failed_configuration)?0x94:entry==0x10526c00?0x44:
            entry==c.u32(c.u32(c.u32(object)+0x20))?0x20:entry==child_destructor?8:0x64;
        if(eng_first_lists(entry)) {
            c.set(2,c.u32(descriptor+4));s.pc=entry;return true;
        }
        trace(c,entry);
        if(entry==child_destructor && active.mutate_stages==1)c.put32(object+0x30,0);
        if(entry==child_destructor && active.mutate_stages==2)c.put32(object+0x30,2);
        if(entry==child_destructor && active.mutate_stages==3)c.put32(object+0xc,data+0xe100);
        c.set(3,method==0x20?(active.fail_virtual==0x20?1:0):active.fail_virtual==int(method)?0:1);
        s.pc=c.lr();return true;
    }
    if(active.live_calendar && (target==0x104a9810 || target==0x104a9890 || target==0x107156f0 ||
                               target==0x104854f0 || target==0x10485570))return false;
    if(active.live_clubs && (target==0x104a60e0 || target==0x1049f540 || target==0x1049f6d0))return false;
    if(!dependencies.contains(target))return false;
    trace(c,target);
    if(target==0x10775130 || target==0x10773520) {
        ++allocations;
        if(active.fail_allocation==int(allocations))c.set(3,0);
        else { const auto size=c.r(3),allocated=next_allocation;
            next_allocation+=(size+31)&~15u;c.set(3,allocated); }
    } else if(target==0x104a60e0)c.put32(c.u32(c.r(3)+0xb0)+c.r(4)*0x44,c.r(5));
    else if(target==0x10459b50) {
        if(c.r(3)!=c.u32(object+8) || c.r(4)!=7)throw std::runtime_error("fixture lookup arguments");
        c.set(3,c.r(3)+0x100);
    }
    else if(target==0x104bc3c0) {
        c.put32(c.r(3),child_table);
        const auto entrants=c.r(6);
        for(unsigned i=0;i<4;++i)c.put32(c.r(3)+0x20+i*4,c.u32(entrants+i*4));
    }
    else if(target==0x10727890)c.put32(c.r(3),c.r(5));
    else if(target==0x1074b830)c.put32(c.r(3),c.u32(c.r(4)));
    else if(target==0x1074bab0)c.put32(c.r(3)+4,c.r(5));
    else if(target==0x1049f540)c.set(3,0x12345);
    else if(target==0x107156f0)c.put16(c.r(3),100);
    else if(target==0x104a9810) { if(c.r(4)==22)c.put16(c.r(3)+0x688,100+active.day-1); }
    else if(target==0x10739890) { c.put32(c.r(6),0x454e4700);c.put32(c.r(7),0x43505000); }
    else if(target==0x1077b720)c.put32(c.r(3),c.r(8));
    else if(target!=0x104a9890 && target!=0x104854f0 && target!=0x10485570)
        c.set(3,active.fail_call==int(target)?0:1);
    s.pc=c.lr();return true;
}
void initialize(Cpu& c,const Scenario& test) {
    c.s.memory.map(data,0x200000);c.s.memory.map(0x71000000,0x20000);
    c.s.memory.copy_in(0x71000000,std::vector<std::uint8_t>(0x20000,0xa5));
    c.s.memory.copy_in(object,std::vector<std::uint8_t>(0x100,0xa6));
    for(unsigned r=0;r<32;++r)c.set(r,0x51000000+r*0x101);
    c.set(1,0x71010000);c.set(2,0x108bba90);c.set_lr(sentinel);
    for(unsigned f=0;f<8;++f)c.set_cr(f,f+1);
    const auto global=[&](int slot,std::uint32_t value){ c.put32(c.r(2)+std::uint32_t(slot),value); };
    const auto variable=[&](int slot,unsigned offset,std::uint32_t value){global(slot,data+offset);c.put32(data+offset,value);};
    variable(-0x7b50,0x100,test.club_total);variable(-0x7be8,0x104,clubs);
    variable(-0x67a4,0x108,4);variable(-0x67a0,0x10c,3);variable(-0x67a8,0x118,5);
    variable(-0x7bbc,0x11c,definitions);variable(-0x7b48,0x120,data+0x7200);
    global(-0x7bb8,data+0x9400); global(-0x7b80,data+0x7000);c.put16(data+0x7002,test.year);
    variable(-0x7ab8,0x110,0);variable(-0x7428,0x114,data+0x7100);
    global(-0x7ba8,data+0x8000);global(-0x7bac,data+0x9000);c.put8(data+0x9000,0xa5);
    if(test.fail_virtual==0x94) {
        // Canonical configure always succeeds. Exercise the caller's assertion
        // using an external overridden method, never a canned owned routine.
        const auto original_table=c.toc(-0x6024);
        for(unsigned offset=0;offset<0x98;offset+=4)c.put32(data+0x1300+offset,c.u32(original_table+offset));
        c.put32(data+0x1394,data+0x1400);c.put32(data+0x1400,failed_configuration);c.put32(data+0x1404,c.r(2));
        global(-0x6024,data+0x1300);
    }
    c.put32(object,c.toc(-0x6024));c.put32(object+4,definitions+4*0x6c);
    c.put32(object+0xbc,test.storage&1?data+0xc000:0);
    c.put32(object+8,test.storage&2?data+0xd000:0);
    c.put32(object+0xc,test.storage&4?data+0xe000:0);
    c.put32(object+0x30,std::uint32_t(test.storage&4?test.last_stage:-1));
    c.put16(object+0x40,test.year);
    c.put16(object+0x3e,test.entrant_count);c.put8(object+0xc1,4);
    c.put32(object+0x2c,test.stage_capacity); c.put32(object+0xb0,data+0x18000);
    for(unsigned i=0;i<32;++i)c.put32(data+0x18000+i*0x44,clubs+i*0x248);
    c.put8(clubs+7*0x248+0x23b,0x85);
    c.put32(child_descriptor,child_destructor);c.put32(child_descriptor+4,c.r(2));
    c.put32(child_table+8,child_descriptor);
    for(unsigned i=0;i<8;++i) {
        c.put32(definitions+i*0x6c,i);
        const auto child=data+0xf000+i*0x100;
        c.put32(child,child_table);c.put32(data+0xe000+i*4,i==1?0:child);
        c.put32(data+0xe100+i*4,i==2?child:0);
    }
    for(unsigned i=0;i<test.club_total;++i) {
        c.put32(clubs+i*0x248,i);
        c.put32(clubs+i*0x248+0x30,test.mixed && i==0?0:definitions+(i<test.members?4:3)*0x6c);
    }
    c.set({{3,test.null_object?0:object},{4,std::uint32_t(test.stage)},
           {5,data+0x2000},{6,data+0x2004},{7,test.output_flag?data+0x2008:0}});
    c.put16(data+0x2000,0xabcd);c.put16(data+0x2004,0xabcd);c.put8(data+0x2008,0xab);
    if(test.entry==0x105267c0)c.set({{4,std::uint32_t(test.year)},{5,definitions+4*0x6c}});
    if(test.entry==0x10526af0)c.set(4,std::uint32_t(test.deleting));
    if(test.output_alias==1)c.set(7,c.r(5));
    if(test.output_alias==2)c.set(6,c.r(5));
    if(test.output_alias==3)c.set(5,object+0x40);
    if(test.output_alias==4)c.set(6,object+0x40);
    if(test.entry==0x105291c0)c.put32(object+0x30,test.last_stage);
    if(test.entry==0x10529470)c.set({{4,clubs+std::uint32_t(test.match_row)*0x248},{5,std::uint32_t(test.event)},
        {6,std::uint32_t(test.stage)},{7,data+0x2008}});
    if(test.entry==0x105295f0) {
        c.set({{4,7},{5,test.special_description?1u:2u},{6,std::uint32_t(test.event)}});
        c.put32(c.sp()+0x38,test.option);c.put32(c.sp()+0x3c,test.description_detail);c.put32(c.sp()+0x40,data+0x9600);
    }
    if(test.entry==0x105296d0)c.set({{4,7},{5,std::uint32_t(test.stage)},{6,std::uint32_t(test.first)},
        {7,std::uint32_t(test.second)},{8,std::uint32_t(test.third)}});
    c.s.budget=2000000;
}
void postconditions(State& s) {
    Cpu c(s);
    if(c.sp()!=(real_state_mode?real_stack:0x71010000) || c.lr()!=sentinel || s.pc!=sentinel)throw std::runtime_error("fixture did not return");
    if(real_state_mode) {
        if(active.entry==0x10526c00 && (!c.r(3) || c.u16(real_stack-0x2000)!=(active.stage==-1?46:2)))
            throw std::runtime_error("real database calendar did not complete");
        if((active.entry==0x10528c60 || active.entry==0x10528e50) && c.r(3)!=1)
            throw std::runtime_error("real database competition setup did not complete");
        if(active.entry==0x10528c60 && c.u16(real_object+0x3e)!=24)
            throw std::runtime_error("real database entrant count");
        return;
    }
    if(!active.output_alias && active.entry==0x10526c00 && (std::int8_t(active.stage)==-1 || std::int8_t(active.stage)==0)) {
        const bool league=std::int8_t(active.stage)==-1;
        if(c.u16(data+0x2000)!=(league?46:2) || c.u16(data+0x2004)!=(league?0:160) ||
           (active.output_flag && c.u8(data+0x2008)!=(league?1:0)))throw std::runtime_error("calendar output fields");
        if(active.fail_allocation!=1 && !c.r(3))throw std::runtime_error("missing calendar");
    }
}
void flush_profile() {
#if defined(CM_ENG_FIRST_COVERAGE)
    // LLVM caches its initial %p expansion across fork; expand the actual child
    // PID ourselves so every returned rewrite leaves independent evidence.
    const auto* pattern=std::getenv("LLVM_PROFILE_FILE");
    std::string filename=pattern?pattern:CM_ENG_FIRST_PROFILE_DIR "/check-%p.profraw";
    const auto pid=std::to_string(getpid());
    for(auto at=filename.find("%p");at!=std::string::npos;at=filename.find("%p"))filename.replace(at,2,pid);
    __llvm_profile_set_filename(filename.c_str());
    if(__llvm_profile_write_file())throw std::runtime_error("coverage profile write failed");
#endif
}
void original(State& s,std::uint32_t entry){reference_mode=true;s.pc=entry;run_until(s,sentinel);postconditions(s);}
void rewrite(State& s,std::uint32_t entry){
    reference_mode=false;
    if(production_mode){s.pc=entry;run_until(s,sentinel);}else eng_first_run_rewrite(s,entry);
    postconditions(s);
    flush_profile();
}
void wrong_trace(State& s,std::uint32_t entry){spoil_trace=true;rewrite(s,entry);}
void wrong_memory(State& s,std::uint32_t entry){rewrite(s,entry);Cpu c(s);c.put8(object+0x40,c.u8(object+0x40)^1);}
// A continuous fixture replay uses the normal owned call graph on both routes.
// Allocation, calendars, collected standings and playoff records survive between
// entries; no state or expected image is refreshed from the rewrite.
void cycle(State& s, bool original_route) {
    Cpu c(s); reference_mode=original_route;
    const auto invoke=[&](std::uint32_t entry, std::initializer_list<std::uint32_t> args) {
        unsigned r=3;for(auto value:args)c.set(r++,value);
        c.set_lr(sentinel);s.pc=entry;run_until(s,sentinel);postconditions(s);
    };
    invoke(0x105267c0,{object,2001,definitions+4*0x6c});
    if(c.r(3)!=object || c.u16(object+0x3e)!=24 || !c.u32(object+8))throw std::runtime_error("cycle construction failed");
    invoke(0x105291c0,{object});
    if(c.i32(object+0x30)!=0 || !c.u32(c.u32(object+0xc)))throw std::runtime_error("cycle playoff missing");
    invoke(0x10529470,{object,clubs+3*0x248,1,0,data+0x2008});
    invoke(0x105295f0,{object,7,1,0});
    invoke(0x105296d0,{object,7,0,1,2,3});
    invoke(0x10528f20,{object});
    invoke(0x10526af0,{object,1});
}
void original_cycle(State& s,std::uint32_t){cycle(s,true);}
void rewritten_cycle(State& s,std::uint32_t){cycle(s,false);
    flush_profile();
}
bool routing_selected=false;
void routing_dispatch(State& s,std::uint32_t) {
    if(!routine_check_child())throw std::runtime_error("routing control requires checker child");
    Cpu(s).call(0x10526af0,sentinel);
}
void routing_internal(State& s,std::uint32_t) {
    if(!routine_check_child())throw std::runtime_error("routing control requires checker child");
    const auto before=s.executed;Cpu c(s);eng_first::call(c,0x10526af0,sentinel);c.finish();
    if(routing_selected?s.executed!=before:s.executed==before)throw std::runtime_error("incorrect internal child route");
    flush_profile();
}
struct ObservedWrites final:MemoryObserver {
    void mapped(std::uint32_t,std::uint64_t)override{}
    void written(std::uint32_t,const std::uint8_t*,std::size_t)override{}
    void cleared(std::uint32_t,std::uint64_t)override{}
};
}
int main(int argc,char** argv){
    if(argc!=2 && argc!=3){std::cerr<<"Usage: cm-eng-first-checks <original-segments> [career.snapshot]\n";return 2;}
    try {
        auto base=std::make_unique<State>();load_program(*base,argv[1]);dispatch_probe=probe;
        std::vector<Scenario> cases;
        for(auto entry:entries){
            cases.push_back({entry,"default"});
            if(entry==0x105267c0 || entry==0x10528f20 || entry==0x10526c00 || entry==0x10528c60 || entry==0x10529280)
            for(int allocation=1;allocation<=(entry==0x105267c0?4:entry==0x10528f20?2:entry==0x10529280?3:1);++allocation){
                Scenario t{entry,"allocation "+std::to_string(allocation)};t.fail_allocation=allocation;cases.push_back(t);
            }
        }
        for(int stage:{-129,-128,-2,0,1,127,255,256})for(bool output:{false,true}) {
            Scenario t{0x10526c00,"stage "+std::to_string(stage)};t.stage=stage;t.output_flag=output;cases.push_back(t);
            if(std::int8_t(stage)==0){t.fail_allocation=1;t.name+=" allocation failure";cases.push_back(t);}
        }
        for(int day=12;day<=20;++day){Scenario t{0x10526c00,"December "+std::to_string(day)};t.day=day;cases.push_back(t);}
        for(int day:{-65520,-32768,-1,0,32767,65552}) {
            Scenario t{0x10526c00,"relative date narrowing "+std::to_string(day)};t.day=day;cases.push_back(t);
        }
        for(int alias=1;alias<=4;++alias)for(int stage:{-1,0}) {
            Scenario t{0x10526c00,"output alias "+std::to_string(alias)+" stage "+std::to_string(stage)};
            t.output_alias=alias;t.stage=stage;cases.push_back(t);
        }
        for(auto entry:{0x105267c0u,0x10526c00u,0x10528f20u})for(int year:{-32768,-1,0,32767,65535}) {
            Scenario t{entry,"signed year "+std::to_string(year)};t.year=year;cases.push_back(t);
        }
        for(auto entry:{0x10526c00u,0x10528e50u})for(int year=1998;year<=2012;++year)for(int stage:{-1,0}) {
            Scenario t{entry,"real calendar "+std::to_string(year)+" stage "+std::to_string(stage)};
            t.year=year;t.stage=stage;t.live_calendar=true;cases.push_back(t);
        }
        for(auto entry:{0x105267c0u,0x10528c60u,0x10528f20u})for(unsigned count:{0u,23u,24u,25u}) {
            Scenario t{entry,"members "+std::to_string(count)};t.members=count;cases.push_back(t);
        }
        for(auto entry:{0x105267c0u,0x10528f20u}) {
            Scenario t{entry,"configure failure"};t.fail_virtual=0x94;cases.push_back(t);
            for(int target:{0x104a00a0,0x1049eb30}){t.fail_virtual=-1;t.fail_call=target;t.name="dependency failure "+hex_address(target);cases.push_back(t);}
        }
        Scenario early{0x105267c0,"existing object"};early.fail_virtual=0x20;cases.push_back(early);
        for(auto entry:{0x10526af0u,0x10528f20u})for(unsigned storage=0;storage<8;++storage)for(int last:{-2,-1,0,2,4}) {
            Scenario t{entry,"storage "+std::to_string(storage)+" last "+std::to_string(last)};t.storage=storage;t.last_stage=last;cases.push_back(t);
        }
        for(int deleting:{-32768,-1,0,1,32767,65536,65535}){Scenario t{0x10526af0,"deleting "+std::to_string(deleting)};t.deleting=deleting;cases.push_back(t);}
        Scenario null{0x10526af0,"null object"};null.null_object=true;cases.push_back(null);
        for(auto entry:{0x10526af0u,0x10528f20u})for(int mutation=1;mutation<=3;++mutation) {
            Scenario t{entry,"callback stage mutation "+std::to_string(mutation)};
            t.mutate_stages=mutation;if(mutation==2)t.last_stage=0;cases.push_back(t);
        }
        for(bool live:{false,true})for(unsigned count:{0u,23u,24u,25u})for(bool mixed:{false,true}) {
            Scenario t{0x10528c60,"club scan "+std::to_string(count)+(live?" real services":" scripted")+(mixed?" null division":"")};
            t.members=count;t.live_clubs=live;t.mixed=mixed;if(!count)t.club_total=0;cases.push_back(t);
        }
        for(auto entry:{0x105291c0u,0x10529280u})for(int last:{-2,-1,0,1,2}) {
            Scenario v{entry,"stage progression " + std::to_string(last)};v.last_stage=last;cases.push_back(v);
        }
        for(int stage:{-129,-128,-2,-1,0,1,127,255,256})for(int event:{-129,-128,-2,-1,0,1,2,3,4,5,127,255,256})
            for(int row:{-1,0,3,23,24}) {
                Scenario v{0x10529470,"event "+std::to_string(event)+" stage "+std::to_string(stage)+" row "+std::to_string(row)};
                v.event=event;v.stage=stage;v.match_row=row;cases.push_back(v);
            }
        for(int count:{-1,0,1,3})for(int event:{-1,0,1,2,3}) {
            Scenario v{0x10529470,"short standings"};v.entrant_count=count;v.event=event;v.stage=0;cases.push_back(v);
        }
        for(bool special:{false,true})for(int event:{0,1,-1,256})for(unsigned detail:{0u,1u,128u,255u,256u}) {
            Scenario v{0x105295f0,"description"};v.special_description=special;v.event=event;v.description_detail=detail;cases.push_back(v);
        }
        for(int stage:{-257,-129,-128,-2,-1,0,1,127,255,256})for(int rank=0;rank<9;++rank) {
            Scenario v{0x105296d0,"rank "+std::to_string(rank)};v.stage=stage;v.first=v.second=v.third=rank;cases.push_back(v);
        }
        std::mt19937 random(0x454e4731);
        for(unsigned i=0;i<64;++i) {
            Scenario v{0x105296d0,"seed 454e4731 case "+std::to_string(i)};v.stage=int(i%3)-1;
            v.first=int(random());v.second=int(random());v.third=int(random());cases.push_back(v);
        }
        for(auto entry:entries)for(unsigned i=0;i<12;++i) {
            Scenario v{entry,"seed 454e4731 mutation "+std::to_string(i)};
            v.year=int(std::uint16_t(random()));v.day=12+int(random()%9);v.members=23+random()%3;
            v.event=int(random()%7)-1;v.stage=int(i%3)-1;v.first=int(random());v.second=int(random());v.third=int(random());
            v.storage=random()%8;v.last_stage=int(random()%4)-1;v.deleting=int(std::int16_t(random()));
            v.match_row=int(random()%26)-1;v.option=random()%256;v.description_detail=random()%256;
            // These entries dereference storage established by construction;
            // scalar mutations retain those pointer relationships.
            if(entry==0x105291c0 || entry==0x10529280)v.storage|=4;
            if(entry==0x105296d0)v.storage|=2;
            cases.push_back(v);
        }
        unsigned failures=0;std::map<std::uint32_t,unsigned> counts;
        if(argc==3 && std::string_view(argv[2])=="--original-routing") {
            active={0x10526af0,"original override"};allocations=calls=0;next_allocation=data+0x80000;
            auto state=cm::semantic_io_checks::clone(*base);Cpu c(*state);initialize(c,active);c.set(3,0);
            production_mode=true;reference_mode=false;state->pc=active.entry;
            if(try_modern_eng_first(*state,active.entry) || eng_first_dispatch_probe(*state))
                throw std::runtime_error("original override accepted a replacement");
            eng_first::call(c,active.entry,sentinel);c.finish();postconditions(*state);
            if(!state->executed || c.r(3))throw std::runtime_error("original override did not execute original body");
            std::cout<<"original override entry/probe/internal routes passed\n";return 0;
        }
        if(argc==3) {
            auto career=std::make_unique<State>();load_match_snapshot(*career,argv[2]);Cpu saved(*career);
            // Session snapshots may begin outside a guest call. Supply the
            // original TOC and a private call stack for the isolated invocation.
            saved.set(2,0x108bba90);
            if(!saved.sp()){career->memory.map(0x71000000,0x20000);saved.set(1,0x71010000);}
            career->budget=career->executed+20000000;saved.set_lr(sentinel);
            real_state_mode=production_mode=true;real_stack=saved.sp();
            const auto table=saved.u32(saved.toc(-0x7b5c)),id=saved.u32(saved.toc(-0x67a4));
            auto league=saved.u32(table+id*4);
            if(!league) {
                // The captured career does not enable the First Division. Recover
                // the same fresh 0xec-byte object as its new-game factory, with
                // the real base constructor and untouched database/finance state.
                saved.set(3,0xec);saved.call(0x10773520,sentinel);league=saved.r(3);
                if(!league)throw std::runtime_error("cannot allocate real-database competition fixture");
                saved.call(0x1049e4f0,sentinel);
                saved.put32(league,saved.toc(-0x6024));
                saved.put32(league+4,saved.u32(saved.toc(-0x7bbc))+id*0x6c);
                saved.put16(league+0x40,saved.u16(saved.toc(-0x7b80)+2));
                saved.put8(league+0x4a,9);
                saved.put8(league+0x44,0xff);saved.put32(league+0x30,0xffffffff);saved.put32(league+0x2c,1);
                std::cout<<"Original-initialized First Division fixture using real career database\n";
            } else std::cout<<"Registered career First Division object\n";
            if(saved.u32(league)!=saved.toc(-0x6024) || saved.u32(saved.u32(league+4))!=id)
                throw std::runtime_error("invalid First Division identity in real-database fixture");
            real_object=league;
            for(const auto [entry,stage]:{std::pair{0x10526c00u,-1}, {0x10526c00u,0}, {0x10528c60u,0}, {0x10528e50u,0}}) {
                active={entry,"real database"};active.stage=stage;
                auto state=cm::semantic_io_checks::clone(*career);Cpu c(*state);
                c.set_lr(sentinel);c.set({{3,league},{4,std::uint32_t(stage)},
                    {5,c.sp()-0x2000},{6,c.sp()-0x2004},{7,c.sp()-0x2008}});
                state->budget=state->executed+20000000;
                const auto differences=compare_routine_call(*state,entry,original,rewrite,RoutineChecker::result_r3|RoutineChecker::result_f1);
                std::cout<<(differences.empty()?"ok ":"DIFFERS ")<<hex_address(entry)<<" real career stage "<<stage<<'\n';
                for(const auto& difference:differences)std::cout<<"  "<<difference<<'\n';
                failures+=!differences.empty();
            }
            real_state_mode=production_mode=false;
            std::cout<<"4 real-career comparisons, "<<failures<<" failed\n";return failures?1:0;
        }
        for(bool production:{false,true}) {
            production_mode=production;
            for(const auto& test:cases) {
                auto state=cm::semantic_io_checks::clone(*base);Cpu c(*state);active=test;allocations=calls=0;next_allocation=data+0x80000;
                initialize(c,test);state->pc=test.entry;ObservedWrites observer;if(production)state->memory.observer=&observer;
                const auto differences=compare_routine_call(*state,test.entry,original,rewrite,RoutineChecker::result_r3|RoutineChecker::result_f1);
                std::cout<<(differences.empty()?"ok ":"DIFFERS ")<<(production?"production ":"controlled ")<<hex_address(test.entry)<<" "<<test.name<<'\n';
                for(const auto& difference:differences)std::cout<<"  "<<difference<<'\n';
                failures+=!differences.empty();++counts[test.entry];
            }
        }
        active={0x105267c0,"continuous owned graph replay"};allocations=calls=0;next_allocation=data+0x80000;
        auto cycle_state=cm::semantic_io_checks::clone(*base);Cpu cycle_cpu(*cycle_state);initialize(cycle_cpu,active);
        cycle_cpu.put32(cycle_cpu.sp()+0x38,0);cycle_cpu.put32(cycle_cpu.sp()+0x3c,0);cycle_cpu.put32(cycle_cpu.sp()+0x40,data+0x9600);
        production_mode=true;ObservedWrites cycle_observer;cycle_state->memory.observer=&cycle_observer;
        const auto cycle_differences=compare_routine_call(*cycle_state,active.entry,original_cycle,rewritten_cycle,RoutineChecker::result_r3|RoutineChecker::result_f1);
        for(const auto& difference:cycle_differences)std::cout<<"  cycle "<<difference<<'\n';
        failures+=!cycle_differences.empty();std::cout<<(cycle_differences.empty()?"ok":"DIFFERS")<<" continuous owned graph replay\n";
        production_mode=false;active=cases.front();allocations=calls=0;next_allocation=data+0x80000;
        auto state=cm::semantic_io_checks::clone(*base);Cpu c(*state);initialize(c,active);
        for(int memory:{0,1,2}) {
            const auto differences=compare_routine_call(*state,active.entry,original,memory==1?wrong_memory:memory==2?wrong_trace:rewrite,RoutineChecker::result_r3,memory==0);
            if(differences.empty()){++failures;std::cout<<"FAIL corruption control\n";}else std::cout<<"ok "<<(memory==2?"trace":memory==1?"memory":"result")<<" corruption detected\n";
        }
        spoil_trace=false;
        // A numerical instruction budget distinguishes actual C++ ownership
        // from two equivalent runs that accidentally both use generated code.
        for(auto entry:entries) {
            active={entry,"production ownership"};
            const auto prepared=[&] {
                auto s=cm::semantic_io_checks::clone(*base);Cpu cpu(*s);
                allocations=calls=0;next_allocation=data+0x80000;initialize(cpu,active);return s;
            };
            auto reference=prepared();original(*reference,entry);
            const auto limit=reference->executed-1;
            auto normal=prepared();normal->budget=limit;ObservedWrites observer;normal->memory.observer=&observer;
            production_mode=true;rewrite(*normal,entry);
            auto rejected=prepared();rejected->budget=limit;bool exhausted=false;
            try{original(*rejected,entry);}catch(const std::exception& e){
                exhausted=std::string(e.what()).find("budget")!=std::string::npos;
            }
            if(!exhausted || normal->executed>=reference->executed)throw std::runtime_error("production ownership budget control failed");
            std::cout<<"ok default observed production ownership "<<hex_address(entry)<<'\n';
        }
        reference_mode=false;production_mode=true;
        for(bool selected:{true,false}) {
            active={0x10526af0,"checker selection"};allocations=calls=0;next_allocation=data+0x80000;
            auto state=cm::semantic_io_checks::clone(*base);Cpu c(*state);initialize(c,active);c.set(3,0);
            RoutineChecker parent([=](auto entry){return selected && entry==0x10526af0;},1,0);
            routine_checker=&parent;const auto before=state->executed;
            eng_first::call(c,0x10526af0,sentinel);c.finish();
            if((selected && (parent.totals().size()!=1 || parent.totals().at(0x10526af0).checked!=1 || parent.differences())) ||
               (!selected && (!parent.totals().empty() || state->executed==before)))throw std::runtime_error("incorrect parent selection route");
            routine_checker=nullptr;
            constexpr auto caller=0xf0001000u;
            RoutineChecker child([=](auto entry){return entry==caller || (selected && entry==0x10526af0);},1,0);
            routine_checker=&child;routing_selected=selected;
            state=cm::semantic_io_checks::clone(*base);Cpu child_cpu(*state);initialize(child_cpu,active);child_cpu.set(3,0);
            if(!child.check(*state,caller,routing_dispatch,routing_internal,RoutineChecker::result_r3) || child.differences())
                throw std::runtime_error("incorrect child selection route");
            routine_checker=nullptr;
        }
        std::cout<<"4 checker parent/child selection controls passed\n";
        for(const auto& [entry,count]:counts)std::cout<<"entry "<<hex_address(entry)<<": "<<count<<" comparisons\n";
        std::cout<<cases.size()*2<<" comparisons, "<<failures<<" failed\n";return failures?1:0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 2;}
}
