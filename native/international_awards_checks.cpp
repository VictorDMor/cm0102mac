// Independent original/C++ comparisons; external services have scripted ABI
// boundaries, while the constructor's owned virtual initialization runs fully.
#include "international_awards_adapter.hpp"
#include "international_awards.hpp"
#include "guest_values.hpp"
#include "nation_awards_adapter.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>

#if defined(CM_INTERNATIONAL_AWARDS_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
#endif
using namespace cm::aot;
using namespace cm::aot::guest;
namespace cm::aot { bool international_awards_reference(State&); }
namespace {
constexpr std::uint32_t data=0x60000000, object=data+0x1000, date=data+0x2000,
    awards=data+0x3000, ratings=data+0x4000, trace_at=data+0x10000,
    stack=0x71010000, sentinel=0xfffffff0;
constexpr std::array entries{0x10440620u,0x104406d0u,0x104409c0u};
struct Case {
    std::uint32_t entry;
    unsigned file=0,load=1,failures=0,year=2002,date_match=0,year_match=7;
    bool clobber=false,live_mutation=false,real_date=false;
    unsigned calendar_day=6,calendar_month=4;
    int stored_day=123;
};
Case active{entries[0]};
bool original_mode=false,production_mode=false,ownership_mode=false;
unsigned allocations=0,date_calls=0,calls=0;
void trace(Cpu& c,std::uint32_t target) {
    if(++calls>100)throw std::runtime_error("fixture call bound");
    auto size=c.u32(trace_at);
    const auto add=[&](std::uint32_t value){c.put32(trace_at+4+size++*4,value);};
    add(target);add(c.lr());
    for(unsigned r=3;r<=10;++r)add(c.r(r));
    for(unsigned r=13;r<32;++r)add(c.r(r));
    for(unsigned f=0;f<8;++f)add(c.cr(f));
    for(unsigned offset=0x38;offset<=0x44;offset+=4)add(c.u32(c.sp()+offset));
    c.put32(trace_at,size);
}
bool probe(State& s) {
    Cpu c(s);const auto target=s.pc;
    // Match Cpu::call: lowered external leaves execute before boundary probes.
    if(run_leaf(s))return true;
    if(target>=entries[0] && target<0x10440c70) {
        if(original_mode)return international_awards_reference(s);
        if(production_mode)return false;
        if(international_awards_lists(target)){international_awards_run_rewrite(s,target);return true;}
        return false;
    }
    if(target==0x10781320) {
        const auto descriptor=c.r(12);c.put32(c.sp()+0x14,c.r(2));
        c.set(2,c.u32(descriptor+4));s.pc=c.u32(descriptor);return true;
    }
    if(target==0x107156f0 && active.real_date)return false;
    const bool known=target==0x10444390 || target==0x10446b00 || target==0x10775130 ||
        target==0x10773520 || target==0x1044cad0 || target==0x107156f0 ||
        target==0x10444580 || target==0x10444e80 || target==0x104449c0 ||
        target==0x10739890 || target==0x10768810 || target==0x1077b720 || target==0x10769b60;
    if(!known)return false;
    if(ownership_mode)s.budget=s.executed;
    trace(c,target);
    std::uint32_t answer=c.r(3);
    if(target==0x10444390) {
        // Execute the real neighboring module's constructor, not a second
        // handwritten model of its untouched-field and volatile behavior.
        nation_awards_run_rewrite(s,target);return true;
    }
    if(target==0x10446b00)answer=active.load;
    else if(target==0x10775130 || target==0x10773520) {
        const auto index=allocations++;
        answer=(active.failures&(1u<<index))?0:data+0x5000+index*0x100;
    } else if(target==0x1044cad0) {
        // Constructor contracts: the allocated identity survives as r3.
        c.put32(c.r(3),c.r(5));c.put32(c.r(3)+4,c.r(6));
    } else if(target==0x107156f0) {
        const auto index=date_calls++;
        if(index>=3)throw std::runtime_error("date call bound");
        c.put16(answer,active.date_match&(1u<<index)?c.i16(date):c.i16(date)+1);
        c.put16(answer+2,active.year_match&(1u<<index)?active.year:active.year+1);
        c.put32(answer+4,0x76543210+index);
    } else if(target==0x10739890) {
        c.put32(c.r(6),0x494e5400);c.put32(c.r(7),0x43505000);
    } else if(target==0x10768810)answer=data+0x8000;
    else if(target==0x1077b720)c.put32(c.r(3),c.r(8));
    if(active.live_mutation && (target==0x10444580 || target==0x10444e80 || target==0x104449c0)) {
        // Explicit caller characterization: later calls must reload live award
        // storage; the ratings value captured in r28 must remain unchanged.
        c.put32(object+4,awards+0x100);c.put32(data+0x100,ratings+0x100);
    }
    if(active.clobber) {
        for(unsigned r=4;r<=10;++r)c.set(r,0x41000000+calls*0x100+r);
        c.compare(0,17,29);c.set_f(1,3.5+calls);
    }
    c.set(3,answer);s.pc=c.lr();return true;
}
void initialize(Cpu& c,const Case& t) {
    c.s.memory.map(data,0x30000);c.s.memory.map(0x71000000,0x20000);
    c.s.memory.copy_in(data,std::vector<std::uint8_t>(0x30000,0xa5));
    c.s.memory.copy_in(0x71000000,std::vector<std::uint8_t>(0x20000,0xa5));
    for(unsigned r=0;r<32;++r)c.set(r,0x51000000+r*0x101);
    for(unsigned f=0;f<32;++f)c.set_f(f,1.25+f);
    for(unsigned f=0;f<8;++f)c.set_cr(f,f+1);
    c.set(1,stack);c.set(2,0x108bba90);c.set_lr(sentinel);
    c.put32(object,c.toc(-0x61e0));c.put32(object+4,awards);c.put32(object+24,0x123);
    for(unsigned i=0;i<6;++i){c.put32(awards+i*4,data+0x9000+i*0x100);c.put32(awards+0x100+i*4,data+0xa000+i*0x100);}
    c.put16(date,std::uint32_t(t.stored_day));c.put16(date+2,t.year);
    c.put32(c.r(2)-0x7bb4,data+0x100);c.put32(data+0x100,ratings);c.put32(ratings+16,data+0xb000);c.put32(ratings+0x110,data+0xc000);
    for(auto slot:{-0x6ba0,-0x6ba4,-0x6bb0,-0x6ba8,-0x6bac,-0x70f8,-0x7974,-0x7a70,-0x6b9c,-0x7970}) {
        const auto offset=unsigned(-slot)&0xfff;c.put32(c.r(2)+std::uint32_t(slot),data+0x200+offset);c.put32(data+0x200+offset,0x12340000+offset);
    }
    c.put32(c.r(2)-0x7ba8,data+0xd000);c.put32(c.r(2)-0x7bac,data+0xd100);c.put8(data+0xd100,1);
    if(t.real_date) {
        c.s.budget=c.s.executed+100000;
        c.set(3,date);c.set(4,t.calendar_day);c.set(5,t.calendar_month);c.set(6,t.year);c.set(7,0xffffffff);
        c.call(0x107156f0,sentinel);
    }
    c.put32(trace_at,0);c.set(3,object);c.set(4,t.entry==entries[2]?date:t.file);
    c.s.budget=c.s.executed+100000;
}
void original(State& s,std::uint32_t entry) {
    original_mode=true;allocations=date_calls=calls=0;s.pc=entry;run_until(s,sentinel);
}
void rewrite(State& s,std::uint32_t entry) {
    original_mode=false;allocations=date_calls=calls=0;
    if(production_mode){s.pc=entry;run_until(s,sentinel);}
    else international_awards_run_rewrite(s,entry);
#if defined(CM_INTERNATIONAL_AWARDS_COVERAGE)
    __llvm_profile_write_file();
#endif
}
void corrupt(State& s,std::uint32_t entry){rewrite(s,entry);Cpu c(s);c.put8(object+20,c.u8(object+20)^1);}
struct Observed final:MemoryObserver {
    void mapped(std::uint32_t,std::uint64_t)override{}
    void written(std::uint32_t,const std::uint8_t*,std::size_t)override{}
    void cleared(std::uint32_t,std::uint64_t)override{}
};
}
int main(int argc,char** argv) {
    if(argc!=2)return 2;
    try {
        auto base=std::make_unique<State>();load_program(*base,argv[1]);dispatch_probe=probe;
        std::vector<Case> cases;
        for(bool clobber:{false,true}) {
            for(unsigned failures=0;failures<128;++failures) {
                Case t{entries[1]};t.failures=failures;t.clobber=clobber;cases.push_back(t);
                for(unsigned file:{0u,7u})for(unsigned load:{0u,1u,0x100u,0xffu}){
                    if(file==0 && load!=0)continue;
                    t.entry=entries[0];t.file=file;t.load=load;cases.push_back(t);
                }
            }
            for(unsigned year:{0u,2001u,2002u,65535u})for(unsigned day=0;day<8;++day)for(unsigned matched_year=0;matched_year<8;++matched_year)for(bool mutation:{false,true}) {
                Case t{entries[2]};t.year=year;t.date_match=day;t.year_match=matched_year;t.clobber=clobber;t.live_mutation=mutation;cases.push_back(t);
            }
        }
        for(int day:{-32768,-1,0,32767})for(unsigned matched=0;matched<8;++matched){
            Case t{entries[2]};t.stored_day=day;t.date_match=matched;cases.push_back(t);
        }
        for(unsigned year:{2000u,2001u,2002u,2004u,2100u})
            for(auto calendar:std::array{std::pair{5u,4u},std::pair{6u,4u},std::pair{7u,4u},
                std::pair{5u,5u},std::pair{6u,5u},std::pair{7u,5u},std::pair{15u,5u},std::pair{16u,5u},std::pair{17u,5u}}) {
                Case t{entries[2]};t.real_date=true;t.year=year;t.calendar_day=calendar.first;t.calendar_month=calendar.second;cases.push_back(t);
            }
        unsigned completed=0,failures=0;std::map<std::uint32_t,std::array<unsigned,2>> counts;
        for(bool production:{false,true}) {
            production_mode=production;
            for(const auto& test:cases) {
                active=test;auto s=cm::semantic_io_checks::clone(*base);Cpu c(*s);initialize(c,test);
                Observed observer;if(production)s->memory.observer=&observer;
                const auto differences=compare_routine_call(*s,test.entry,original,rewrite,RoutineChecker::result_r3|RoutineChecker::result_f1);
                if(!differences.empty()) {
                    std::cout<<"DIFFERS "<<hex_address(test.entry)<<" production="<<production<<" alloc="<<test.failures<<" file="<<test.file<<" load="<<test.load<<" year="<<test.year<<" day="<<test.date_match<<" yr="<<test.year_match<<" clobber="<<test.clobber<<" mutation="<<test.live_mutation<<'\n';
                    for(const auto& d:differences)std::cout<<"  "<<d<<'\n';
                    if(++failures>=8)break;
                } else {++completed;++counts[test.entry][production?1:0];}
            }
            if(failures)break;
        }
        production_mode=false;active=cases.front();active.entry=entries[0];auto s=cm::semantic_io_checks::clone(*base);Cpu c(*s);initialize(c,active);
        if(compare_routine_call(*s,active.entry,original,corrupt,RoutineChecker::result_r3).empty()){++failures;std::cout<<"FAIL corruption control\n";}
        else std::cout<<"ok deliberate memory corruption detected\n";
        unsigned ownership=0;
        for(auto entry:entries) {
            active=Case{entry};auto normal=cm::semantic_io_checks::clone(*base);Cpu nc(*normal);initialize(nc,active);
            normal->budget=normal->executed;Observed observer;normal->memory.observer=&observer;production_mode=true;ownership_mode=true;
            try{rewrite(*normal,entry);++ownership;}catch(const std::exception& e){++failures;std::cout<<"FAIL production ownership "<<hex_address(entry)<<' '<<e.what()<<'\n';}
            ownership_mode=false;
            auto ref=cm::semantic_io_checks::clone(*base);Cpu rc(*ref);initialize(rc,active);ref->budget=ref->executed;bool rejected=false;
            try{original(*ref,entry);}catch(const std::exception& e){rejected=std::string(e.what())=="Native instruction budget exhausted" && ref->pc==entry;}
            if(!rejected){++failures;std::cout<<"FAIL original budget control\n";}
        }
        for(const auto& [entry,n]:counts)std::cout<<hex_address(entry)<<" returned "<<n[0]<<" controlled + "<<n[1]<<" production\n";
        std::cout<<ownership<<"/3 observed production controls; "<<completed<<" returned comparisons; "<<failures<<" failures\n";
        return failures?1:0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
