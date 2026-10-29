// Returned comparisons against private hook-free originals. Scripted external
// boundaries expose call order, arguments, CR, preserved registers and return
// sites in compared memory. Additional cases execute actual date/row services.
#include "eng_conf_adapter.hpp"
#include "eng_conf_detail.hpp"
#include "match_replay.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <iostream>
#include <map>
#include <set>
#include <vector>

#if defined(CM_ENG_CONF_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
#endif
using namespace cm::aot;
using guest::Cpu;
namespace cm::aot { bool eng_conf_reference(State&); }
namespace {
constexpr std::uint32_t data=0x60000000, object=data+0x1000, clubs=data+0x10000,
    definitions=data+0x3000, log_at=data+0x40000, sentinel=0xfffffff0,
    child_destructor=0xf0000010, child_table=data+0x1100, child_descriptor=data+0x1200;
constexpr std::uint32_t entries[]{0x10524600,0x10524940,0x10524a50,0x105262f0,0x10526460,0x10526520};
struct Scenario {
    std::uint32_t entry;
    std::string name;
    unsigned club_total=26, members=22, storage=7;
    int day=19, stage=-1, year=2001, fail_allocation=0, fail_call=0, fail_virtual=-1;
    int deleting=0, last_stage=2;
    bool null_object=false, output_flag=true, mixed=false, live_calendar=false, live_clubs=false;
    int mutate_stages=0, output_alias=0;
};
Scenario active{};
unsigned allocations=0,calls=0;
std::uint32_t next_allocation=0;
bool reference_mode=false,production_mode=false;
bool real_state_mode=false;
std::uint32_t real_stack=0, real_object=0;
const std::set<std::uint32_t> dependencies{
    0x1049e4f0,0x104a00a0,0x1049eb30,0x10773520,0x104533a0,0x104a6660,
    0x10775170,0x104536c0,0x1049e600,0x107735a0,0x10775130,0x104a60e0,
    0x1049f540,0x1049f6d0,0x104a9810,0x104a9890,0x107156f0,0x10739890,
    0x10768810,0x1077b720,0x10769b60,0x104564a0,0x104aa3a0,0x102d46c0,
    0x104854f0,0x10485570};
void trace(Cpu& c,std::uint32_t target) {
    if (++calls>2000) throw std::runtime_error("fixture call limit");
    auto index=c.u32(log_at);
    const auto add=[&](std::uint32_t word){ c.put32(log_at+4+index++*4,word); };
    add(target);add(c.lr());
    for(unsigned r=3;r<11;++r)add(c.r(r));
    for(unsigned r=13;r<32;++r)add(c.r(r));
    for(unsigned f=0;f<8;++f)add(c.cr(f));
    if(target==0x104a9810 || target==0x10485570)add(c.u32(c.sp()+0x38));
    c.put32(log_at,index);
}
bool probe(State& s) {
    Cpu c(s);const auto target=s.pc;
    if(run_leaf(s))return true;
    if(eng_conf_lists(target)) {
        if(reference_mode)return eng_conf_reference(s);
        if(production_mode)return false;
        eng_conf_run_rewrite(s,target);return true;
    }
    if(reference_mode && target>=0x10524600 && target<0x105267c0)return eng_conf_reference(s);
    if(real_state_mode)return false;
    if(target==0x10781320) {
        const auto descriptor=c.r(12),entry=c.u32(descriptor);
        c.put32(c.sp()+0x14,c.r(2));
        const unsigned method=entry==0x10526460?0x94:entry==0x10524a50?0x44:
            entry==c.u32(c.u32(c.u32(object)+0x20))?0x20:entry==child_destructor?8:0x64;
        if(eng_conf_lists(entry) && active.fail_virtual!=int(method)) {
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
    else if(target==0x1049f540)c.set(3,0x12345);
    else if(target==0x107156f0)c.put16(c.r(3),100);
    else if(target==0x104a9810) { if(c.r(4)==20)c.put16(c.r(3)+0x5f0,100+active.day-1); }
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
    variable(-0x6874,0x108,4);variable(-0x67ac,0x10c,3);
    variable(-0x7ab8,0x110,0);variable(-0x7428,0x114,data+0x7100);
    global(-0x7ba8,data+0x8000);global(-0x7bac,data+0x9000);c.put8(data+0x9000,0xa5);
    c.put32(object,c.toc(-0x6028));c.put32(object+4,definitions+4*0x6c);
    c.put32(object+0xbc,test.storage&1?data+0xc000:0);
    c.put32(object+8,test.storage&2?data+0xd000:0);
    c.put32(object+0xc,test.storage&4?data+0xe000:0);
    c.put32(object+0x30,std::uint32_t(test.storage&4?test.last_stage:-1));
    c.put16(object+0x40,test.year);
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
    if(test.entry==0x10524600)c.set({{4,std::uint32_t(test.year)},{5,definitions+4*0x6c}});
    if(test.entry==0x10524940)c.set(4,std::uint32_t(test.deleting));
    if(test.output_alias==1)c.set(7,c.r(5));
    if(test.output_alias==2)c.set(6,c.r(5));
    if(test.output_alias==3)c.set(5,object+0x40);
    if(test.output_alias==4)c.set(6,object+0x40);
    c.s.budget=2000000;
}
void postconditions(State& s) {
    Cpu c(s);
    if(c.sp()!=(real_state_mode?real_stack:0x71010000) || c.lr()!=sentinel || s.pc!=sentinel)throw std::runtime_error("fixture did not return");
    if(real_state_mode) {
        if(active.entry==0x10524a50 && (!c.r(3) || c.u16(real_stack-0x2000)!=(active.stage==-1?42:3)))
            throw std::runtime_error("real database calendar did not complete");
        if((active.entry==0x105262f0 || active.entry==0x10526460) && c.r(3)!=1)
            throw std::runtime_error("real database competition setup did not complete");
        if(active.entry==0x105262f0 && c.u16(real_object+0x3e)!=22)
            throw std::runtime_error("real database entrant count");
        return;
    }
    if(!active.output_alias && active.entry==0x10524a50 && (std::int8_t(active.stage)==-1 || std::int8_t(active.stage)==0)) {
        const bool league=std::int8_t(active.stage)==-1;
        if(c.u16(data+0x2000)!=(league?42:3) || c.u16(data+0x2004)!=(league?0:160) ||
           (active.output_flag && c.u8(data+0x2008)!=(league?1:0)))throw std::runtime_error("calendar output fields");
        if(active.fail_allocation!=1 && !c.r(3))throw std::runtime_error("missing calendar");
    }
}
void original(State& s,std::uint32_t entry){reference_mode=true;s.pc=entry;run_until(s,sentinel);postconditions(s);}
void rewrite(State& s,std::uint32_t entry){
    reference_mode=false;
    if(production_mode){s.pc=entry;run_until(s,sentinel);}else eng_conf_run_rewrite(s,entry);
    postconditions(s);
#if defined(CM_ENG_CONF_COVERAGE)
    __llvm_profile_write_file();
#endif
}
void wrong_memory(State& s,std::uint32_t entry){rewrite(s,entry);Cpu c(s);c.put8(object+0x40,c.u8(object+0x40)^1);}
struct ObservedWrites final:MemoryObserver {
    void mapped(std::uint32_t,std::uint64_t)override{}
    void written(std::uint32_t,const std::uint8_t*,std::size_t)override{}
    void cleared(std::uint32_t,std::uint64_t)override{}
};
}
int main(int argc,char** argv){
    if(argc!=2 && argc!=3){std::cerr<<"Usage: cm-eng-conf-checks <original-segments> [career.snapshot]\n";return 2;}
    try {
        auto base=std::make_unique<State>();load_program(*base,argv[1]);dispatch_probe=probe;
        std::vector<Scenario> cases;
        for(auto entry:entries){
            cases.push_back({entry,"default"});
            if(entry!=0x10524940)for(int allocation=1;allocation<=(entry==0x10524600?3:entry==0x10526520?2:1);++allocation){
                Scenario t{entry,"allocation "+std::to_string(allocation)};t.fail_allocation=allocation;cases.push_back(t);
            }
        }
        for(int stage:{-129,-128,-2,0,1,127,255,256})for(bool output:{false,true}) {
            Scenario t{0x10524a50,"stage "+std::to_string(stage)};t.stage=stage;t.output_flag=output;cases.push_back(t);
            if(std::int8_t(stage)==0){t.fail_allocation=1;t.name+=" allocation failure";cases.push_back(t);}
        }
        for(int day=15;day<=23;++day){Scenario t{0x10524a50,"December "+std::to_string(day)};t.day=day;cases.push_back(t);}
        for(int day:{-65520,-32768,-1,0,32767,65552}) {
            Scenario t{0x10524a50,"relative date narrowing "+std::to_string(day)};t.day=day;cases.push_back(t);
        }
        for(int alias=1;alias<=4;++alias)for(int stage:{-1,0}) {
            Scenario t{0x10524a50,"output alias "+std::to_string(alias)+" stage "+std::to_string(stage)};
            t.output_alias=alias;t.stage=stage;cases.push_back(t);
        }
        for(auto entry:{0x10524600u,0x10524a50u,0x10526520u})for(int year:{-32768,-1,0,32767,65535}) {
            Scenario t{entry,"signed year "+std::to_string(year)};t.year=year;cases.push_back(t);
        }
        for(auto entry:{0x10524a50u,0x10526460u})for(int year=1998;year<=2012;++year)for(int stage:{-1,0}) {
            Scenario t{entry,"real calendar "+std::to_string(year)+" stage "+std::to_string(stage)};
            t.year=year;t.stage=stage;t.live_calendar=true;cases.push_back(t);
        }
        for(auto entry:{0x10524600u,0x105262f0u,0x10526520u})for(unsigned count:{0u,21u,22u,23u}) {
            Scenario t{entry,"members "+std::to_string(count)};t.members=count;cases.push_back(t);
        }
        for(auto entry:{0x10524600u,0x10526520u}) {
            Scenario t{entry,"configure failure"};t.fail_virtual=0x94;cases.push_back(t);
            for(int target:{0x104a00a0,0x1049eb30}){t.fail_virtual=-1;t.fail_call=target;t.name="dependency failure "+hex_address(target);cases.push_back(t);}
        }
        Scenario early{0x10524600,"existing object"};early.fail_virtual=0x20;cases.push_back(early);
        for(auto entry:{0x10524940u,0x10526520u})for(unsigned storage=0;storage<8;++storage)for(int last:{-2,-1,0,2,4}) {
            Scenario t{entry,"storage "+std::to_string(storage)+" last "+std::to_string(last)};t.storage=storage;t.last_stage=last;cases.push_back(t);
        }
        for(int deleting:{-32768,-1,0,1,32767,65536,65535}){Scenario t{0x10524940,"deleting "+std::to_string(deleting)};t.deleting=deleting;cases.push_back(t);}
        Scenario null{0x10524940,"null object"};null.null_object=true;cases.push_back(null);
        for(auto entry:{0x10524940u,0x10526520u})for(int mutation=1;mutation<=3;++mutation) {
            Scenario t{entry,"callback stage mutation "+std::to_string(mutation)};
            t.mutate_stages=mutation;if(mutation==2)t.last_stage=0;cases.push_back(t);
        }
        for(bool live:{false,true})for(unsigned count:{0u,21u,22u,23u})for(bool mixed:{false,true}) {
            Scenario t{0x105262f0,"club scan "+std::to_string(count)+(live?" real services":" scripted")+(mixed?" null division":"")};
            t.members=count;t.live_clubs=live;t.mixed=mixed;if(!count)t.club_total=0;cases.push_back(t);
        }
        unsigned failures=0;std::map<std::uint32_t,unsigned> counts;
        if(argc==3) {
            auto career=std::make_unique<State>();load_match_snapshot(*career,argv[2]);Cpu saved(*career);
            // Session snapshots may begin outside a guest call. Supply the
            // original TOC and a private call stack for the isolated invocation.
            saved.set(2,0x108bba90);
            if(!saved.sp()){career->memory.map(0x71000000,0x20000);saved.set(1,0x71010000);}
            career->budget=career->executed+20000000;saved.set_lr(sentinel);
            real_state_mode=production_mode=true;real_stack=saved.sp();
            const auto table=saved.u32(saved.toc(-0x7b5c)),id=saved.u32(saved.toc(-0x6874));
            auto league=saved.u32(table+id*4);
            if(!league) {
                // The captured career does not enable the Conference. Recover
                // the same fresh 0xec-byte object as its new-game factory, with
                // the real base constructor and untouched database/finance state.
                saved.set(3,0xec);saved.call(0x10773520,sentinel);league=saved.r(3);
                if(!league)throw std::runtime_error("cannot allocate real-database competition fixture");
                saved.call(0x1049e4f0,sentinel);
                saved.put32(league,saved.toc(-0x6028));
                saved.put32(league+4,saved.u32(saved.toc(-0x7bbc))+id*0x6c);
                saved.put16(league+0x40,saved.u16(saved.toc(-0x7b80)+2));
                saved.put8(league+0x4a,9);saved.put16(league+0xe2,1000);saved.put16(league+0xe4,600);
                saved.put8(league+0x44,0xff);saved.put32(league+0x30,0xffffffff);saved.put32(league+0x2c,0);
                std::cout<<"Original-initialized Conference fixture using real career database\n";
            } else std::cout<<"Registered career Conference object\n";
            if(saved.u32(league)!=saved.toc(-0x6028) || saved.u32(saved.u32(league+4))!=id)
                throw std::runtime_error("invalid Conference identity in real-database fixture");
            real_object=league;
            for(const auto [entry,stage]:{std::pair{0x10524a50u,-1}, {0x10524a50u,0}, {0x105262f0u,0}, {0x10526460u,0}}) {
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
        production_mode=false;active=cases.front();allocations=calls=0;next_allocation=data+0x80000;
        auto state=cm::semantic_io_checks::clone(*base);Cpu c(*state);initialize(c,active);
        for(bool memory:{false,true}) {
            const auto differences=compare_routine_call(*state,active.entry,original,memory?wrong_memory:rewrite,RoutineChecker::result_r3,!memory);
            if(differences.empty()){++failures;std::cout<<"FAIL corruption control\n";}else std::cout<<"ok "<<(memory?"memory":"result")<<" corruption detected\n";
        }
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
        for(const auto& [entry,count]:counts)std::cout<<"entry "<<hex_address(entry)<<": "<<count<<" comparisons\n";
        std::cout<<cases.size()*2<<" comparisons, "<<failures<<" failed\n";return failures?1:0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 2;}
}
