#include "cup_stage_adapter.hpp"
#include "routine_check.hpp"
#include "application_exit.hpp"
#include "semantic_io_checks.hpp"
#include "guest_values.hpp"
#include <iostream>
#include <random>
#include <unistd.h>
#if defined(CM_CUP_STAGE_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
extern "C" void __llvm_profile_set_filename(const char*);
#endif
using namespace cm::aot;
using namespace cm::aot::guest;
namespace cm::aot { bool cup_stage_reference(State&); }
namespace {
constexpr std::uint32_t data=0x60000000, object=data+0x1000, clubs=data+0x2000,
    seeds=data+0x3000, entrants=data+0x4000, rounds=data+0x5000, trace_at=data+0x10000,
    stack=0x71010000, sentinel=0xfffffff0;
bool original_mode=false, production_mode=false, ownership_mode=false, budget_controls=true;
unsigned corrupt_trace=0;
unsigned scenario=0;
unsigned excluded_dispatches=0;
void flush_coverage() {
#if defined(CM_CUP_STAGE_COVERAGE)
    const auto path=std::string(CM_CUP_STAGE_PROFILE_DIR)+"/check-"+std::to_string(getpid())+".profraw";
    __llvm_profile_set_filename(path.c_str());
    if(__llvm_profile_write_file()!=0)throw std::runtime_error("coverage flush failed");
#endif
}
unsigned calls=0, allocation_count=0, failure=0, io_count=0, io_failure=0;
std::uint32_t next_allocation=data+0x20000;
std::vector<std::uint8_t> file_bytes;
std::size_t file_position=0;
void trace(Cpu& c,std::uint32_t target) {
    if(++calls>500)throw std::runtime_error("fixture call bound");
    auto size=c.u32(trace_at);
    auto add=[&](std::uint32_t v){c.put32(trace_at+4+size++*4,v);};
    add(target^(corrupt_trace==2 && !original_mode?4u:0u));add(c.lr());
    for(unsigned r=3;r<=10;++r)add(c.r(r)^(corrupt_trace==1 && !original_mode && r==3 ? 1u:0u));
    for(unsigned r=13;r<32;++r)add(c.r(r));
    for(unsigned f=0;f<8;++f)add(c.cr(f));
    add(c.sp());add(c.r(2));
    for(unsigned bit=1280;bit<1312;++bit)add(c.s.r<1>(bit));
    c.put32(trace_at,size);
}
bool probe(State& s) {
    Cpu c(s);auto target=s.pc;
    if(target>=0x104bc3c0 && target<0x104bed58) {
        if(original_mode)return cup_stage_reference(s);
        if(production_mode){
            if(ownership_mode)s.budget=s.executed;
            if(routine_checker && !routine_check_child() && !routine_checker->wants(target))++excluded_dispatches;
            return false;
        }
        if(cup_stage_lists(target)){cup_stage_run_rewrite(s,target);return true;}
        throw std::runtime_error("unimplemented owned call");
    }
    if(target==0x10478f20) {
        trace(c,target);
        if(ownership_mode) {
            const auto previous=dispatch_probe;dispatch_probe=nullptr;s.budget=s.executed+1000;
            try{c.call(target,c.lr());}catch(...){dispatch_probe=previous;throw;}
            dispatch_probe=previous;s.budget=s.executed;return true;
        }
        return false;
    }
    if(ownership_mode && target==0x10781320)s.budget=s.executed+100;
    if(run_leaf(s))return true;
    const bool supported=target==0x10775170 || target==0x107735a0 || target==0x10478f70 ||
        target==0x10478f20 || target==0x10479100 || target==0x10775130 || target==0x10775200 ||
        target==0x10739890 || target==0x1077b720 || target==0x10769b60 ||
        target==0x1077fc28 || target==0x1076f7d0 || target==0x10775fe0 || target==0x10776360 || target==0x1077b770;
    if(!supported)return false;
    if(ownership_mode)s.budget=s.executed;
    trace(c,target);
    if(target==0x1076f7d0) {if(c.r(3)!=0xffffffff)throw std::runtime_error("unexpected fatal exit status");flush_coverage();throw ApplicationExit{};}
    const auto a=c.r(3),b=c.r(4),count=c.r(5);
    std::uint32_t result=0;
    if(target==0x1077fc28) {
        auto bytes=c.s.memory.copy_out(a,count);c.s.memory.copy_in(b,bytes);c.put32(c.sp()+20,c.r(2));
    } else if(target==0x10775130 || target==0x10775200) {
        const auto bytes=target==0x10775200?a*b:a;

        ++allocation_count;
        if(allocation_count!=failure && bytes<=0x8000) {
            result=next_allocation;next_allocation+=std::max(16u,(bytes+15u)&~15u);
            if(next_allocation>=data+0x30000)throw std::runtime_error("allocation arena exhausted");
            if(target==0x10775200)for(unsigned i=0;i<bytes;++i)c.put8(result+i,0);
        }
    } else if(target==0x10478f70 || target==0x10479100)result=(scenario&8)?0xffffffff:1;
    else if(target==0x10739890){
        c.put32(c.r(6),0x43555000);c.put32(c.r(7),0x43505000);
        // Cpu::call executes the following version-text leaf before the probe.
        // Its single charged block is external; the next boundary resets this.
        if(ownership_mode)s.budget=s.executed+1;
    }
    else if(target==0x1077b720)c.put32(a,c.r(8));
    else if(target==0x1077b770)result=0x9876;
    else if(target==0x10775fe0 || target==0x10776360) {
        if(b>0x10000 || count>32)throw std::runtime_error("invalid IO fixture size");
        const auto bytes=b*count;result=(++io_count==io_failure && count)?count-1:count;
        if(target==0x10775fe0) {
            if(file_position+bytes>file_bytes.size())throw std::runtime_error("short fixture file");
            for(unsigned i=0;i<b*result;++i)c.put8(a+i,file_bytes[file_position+i]);
        } else {
            auto length=c.u32(data+0xe000);
            for(unsigned i=0;i<b*result;++i)c.put8(data+0xe004+length+i,c.u8(a+i));
            c.put32(data+0xe000,length+b*result);
        }
        file_position+=bytes;
    }
    for(unsigned r=4;r<=10;++r)c.set(r,0x41000000+calls*0x100+r);
    c.set(3,result);c.compare(0,17,29);s.pc=c.lr();return true;
}
void initialize(Cpu& c,unsigned seed,int count,unsigned mode) {
    c.s.memory.map(data,0x50000);c.s.memory.map(0x71000000,0x20000);
    c.s.memory.copy_in(data,std::vector<std::uint8_t>(0x50000,0xa5));
    c.s.memory.copy_in(0x71000000,std::vector<std::uint8_t>(0x20000,0xa5));
    std::mt19937 random(seed);
    for(unsigned r=0;r<32;++r)c.set(r,random());
    for(unsigned f=0;f<32;++f)c.set_f(f,1.25+f);
    for(unsigned f=0;f<8;++f)c.set_cr(f,f+1);
    c.set(1,stack);c.set(2,0x108bba90);c.set_lr(sentinel);
    c.put32(object+0xa4,mode&1?0:entrants);c.put32(object+0xa0,mode&2?0:rounds);
    c.put32(object+0xac,mode&4?0:data+0x6000);c.put16(object+0x3c,count);
    for(unsigned i=0;i<32;++i) {
        c.put32(clubs+i*4,random());c.put8(seeds+i,random());
        c.put32(data+0x6000+i*4,i%2?0:data+0x7000+i*0x100);
    }
    c.put32(c.toc(-0x7b90),seed*17);c.put32(c.toc(-0x7b84),c.toc(-0x7b8c));
    c.put32(c.toc(-0x7b88),c.toc(-0x7b8c)+100);
    c.put32(c.r(2)-0x7bac,data+0x9000);c.put32(c.r(2)-0x7ba8,data+0x9100);
    c.put32(c.r(2)-0x7be8,data+0x9800);c.put32(data+0x9800,data+0x30000);
    c.put32(c.r(2)-0x7bbc,data+0x9804);c.put32(data+0x9804,data+0xc000);
    c.put32(data+0xe000,0);
    c.put32(trace_at,0);c.s.budget=c.s.executed+1000000;
}
void original(State& s,std::uint32_t entry){original_mode=true;ownership_mode=false;calls=allocation_count=io_count=0;file_position=0;next_allocation=data+0x20000;s.pc=entry;run_until(s,sentinel);}
void rewrite(State& s,std::uint32_t entry){original_mode=false;ownership_mode=production_mode && budget_controls;if(ownership_mode)s.budget=s.executed;calls=allocation_count=io_count=0;file_position=0;next_allocation=data+0x20000;if(production_mode){s.pc=entry;run_until(s,sentinel);}else cup_stage_run_rewrite(s,entry);flush_coverage();}
struct Observed final:MemoryObserver {
    void mapped(std::uint32_t,std::uint64_t)override{}
    void written(std::uint32_t,const std::uint8_t*,std::size_t)override{}
    void cleared(std::uint32_t,std::uint64_t)override{}
};
void corrupt(State& s,std::uint32_t entry){rewrite(s,entry);Cpu c(s);c.set(3,c.r(3)^1);}
void check_nested_selection(State& initial) {
    const bool previous_production=production_mode;
    budget_controls=false; // An explicitly excluded child must execute its original.
    for(bool exclude_child:{false,true}) {
        auto s=cm::semantic_io_checks::clone(initial);
        // Sampling is off only for this parent-routing control: default generated
        // originals call external native allocators directly, outside this fixture.
        // The independent comparison below still checks the entire owned graph.
        RoutineChecker checker([=](auto entry){return cup_stage_lists(entry) && (!exclude_child || entry!=0x104bca90);},0,0);
        checker.compare_f1=true;routine_checker=&checker;
        production_mode=true;
        const auto differences=compare_routine_call(initial,0x104bc3c0,original,rewrite,RoutineChecker::result_r3|RoutineChecker::result_f1);
        if(!differences.empty())throw std::runtime_error("nested selection comparison failed: "+differences.front());
        original_mode=ownership_mode=false;production_mode=true;excluded_dispatches=0;
        calls=allocation_count=io_count=0;file_position=0;next_allocation=data+0x20000;
        s->pc=0x104bc3c0;run_until(*s,sentinel);routine_checker=nullptr;
        const auto& totals=checker.totals();
        if(checker.differences() || totals.at(0x104bc3c0).checked || totals.at(0x104bc3c0).calls!=1 || totals.at(0x104bd8c0).calls!=1 ||
           (exclude_child ? totals.contains(0x104bca90) || excluded_dispatches!=1 : totals.at(0x104bca90).calls!=1))
            throw std::runtime_error("nested checker selection failed");
    }
    production_mode=previous_production;budget_controls=true;flush_coverage();
    std::cout<<"parent, selected child and excluded child routing passed\n";
}
void check_adapter(State& base) {
    for(unsigned mode=0;mode<4;++mode) {
        auto s=cm::semantic_io_checks::clone(base);Cpu c(*s);initialize(c,1,0,0);
        c.set(3,object);c.set(4,0);s->pc=0x104bca90;
        RoutineChecker selection([=](auto){return mode!=2;},0,0);
        routine_checker=mode?&selection:nullptr;
        if(mode==3)cup_stage_run_original(*s,s->pc);
        else if(cup_stage_dispatch_probe(*s)!=(mode!=2))throw std::runtime_error("adapter selection failed");
        routine_checker=nullptr;
        if(s->pc!=(mode==2?0x104bca90:sentinel))throw std::runtime_error("adapter return failed");
    }
    auto s=cm::semantic_io_checks::clone(base);
    for(auto entry:{0x104bc3c4u,0xffffffffu}) {
        s->pc=entry;
        if(cup_stage_lists(entry) || cup_stage_dispatch_probe(*s))throw std::runtime_error("unknown entry accepted");
        try{try_modern_cup_stage(*s,entry);throw std::runtime_error("unknown entry did not throw");}
        catch(const std::runtime_error& e){if(!std::string_view(e.what()).starts_with("Unknown CUP_STAGE.CPP routine"))throw;}
    }
    flush_coverage();
    std::cout<<"adapter probe, original and unknown-entry controls passed\n";
}
}
int main(int argc,char** argv) {
    if(argc<2 || argc>3)return 2;
    production_mode=argc==3 && std::string_view(argv[2])=="--production";
    try {
        auto base=std::make_unique<State>();load_program(*base,argv[1]);dispatch_probe=probe;
        if(argc==3 && std::string_view(argv[2])=="--original-control") {
            dispatch_probe=nullptr;unsigned stopped=0;
            for(auto entry:{0x104bc3c0u,0x104bc750u,0x104bc9b0u,0x104bca90u,0x104bcb20u,0x104bd550u,0x104bd8c0u,0x104be7e0u}) {
                auto s=cm::semantic_io_checks::clone(*base);Cpu c(*s);initialize(c,1,0,0);s->budget=s->executed;s->pc=entry;
                if(cup_stage_dispatch_probe(*s))throw std::runtime_error("forced original probe accepted entry");
                try{run_until(*s,sentinel);}catch(const std::exception& e){
                    if(std::string_view(e.what())=="Native instruction budget exhausted" && s->pc==entry)++stopped;else throw;
                }
            }
            std::cout<<stopped<<" forced-original production controls stopped at their entries\n";
            return stopped==8?0:1;
        }
        check_adapter(*base);
        unsigned completed=0,terminated=0,failures=0;
        for(auto entry:{0x104bc9b0u,0x104bca90u})for(int count:{-1,0,1,3,16})
            for(unsigned mode=0;mode<8;++mode)for(unsigned seed=1;seed<=4;++seed) {
                auto s=cm::semantic_io_checks::clone(*base);Cpu c(*s);initialize(c,seed,count,entry==0x104bca90?0:mode);
                c.set(3,entry==0x104bc9b0 && mode==7?0:object);
                c.set(4,entry==0x104bc9b0?std::uint32_t(count):std::uint32_t(count));
                c.set(5,clubs);c.set(6,mode&2?0xffu:0);c.set(7,mode&1?seeds:0);
                Observed observed;if(production_mode)s->memory.observer=&observed;
                bool exits=false;
                {auto oracle=cm::semantic_io_checks::clone(*s);try{original(*oracle,entry);}catch(const ApplicationExit&){exits=true;}catch(const std::exception& e){throw std::runtime_error(hex_address(entry)+" mode="+std::to_string(mode)+" seed="+std::to_string(seed)+" original: "+e.what());}}
                auto differences=compare_routine_call(*s,entry,original,rewrite,RoutineChecker::result_r3|RoutineChecker::result_f1);
                if(!differences.empty()) {
                    std::cout<<"DIFFERS "<<hex_address(entry)<<" count="<<count<<" mode="<<mode<<" seed="<<seed<<'\n';
                    for(const auto& d:differences)std::cout<<d<<'\n';
                    if(++failures==4)return 1;
                } else {if(exits)++terminated;else ++completed;}
            }
        for(auto entry:{0x104bc3c0u,0x104bc750u,0x104bcb20u,0x104bd550u,0x104bd8c0u,0x104be7e0u})
            for(unsigned mode=0;mode<72;++mode)for(unsigned seed=1;seed<=3;++seed) {
                auto s=cm::semantic_io_checks::clone(*base);Cpu c(*s);initialize(c,seed,2,0);
                scenario=mode;failure=mode<6?mode:0;io_failure=mode<24?mode:0;file_bytes.clear();
                c.set(3,object);c.set(4,1);
                c.put16(object+0x3e,3);c.put32(object+4,data+0xc000);c.put32(data+0xc000,mode==25?0xffffffff:0);
                for(unsigned i=0;i<32;++i){auto club=data+0x30000+i*0x248;c.put32(entrants+i*8,i==1?0:club);c.put32(club,i);c.put32(club+0x3c,data+0x45000);c.put32(data+0x45000,7);}
                c.put16(rounds+0x1c,2);c.put16(rounds+0x78+0x1c,1);
                for(unsigned i=0;i<2;++i){auto r=rounds+i*0x78;c.put16(r+8,0);c.put16(r+14,0x200);c.put16(r+0x1e,i?0:4);c.put16(r+0x20,0);c.put16(r+0x22,1);}
                if(entry==0x104bc3c0) {
                    c.set(4,object);c.set(5,3);c.set(6,clubs);c.set(7,2);c.set(8,data+0xc000);c.set(9,rounds);c.set(10,2001);
                    c.put32(stack+0x48,mode>=12?1:0);c.put32(stack+0x50,7);c.put32(stack+0x54,seeds);
                }
                if(entry==0x104bc750){c.set(4,3);c.set(5,2);c.set(6,data+0x8000);c.put16(data+0x8000,2);c.put16(data+0x8002,1);}
                if(entry==0x104bcb20) {
                    const auto append=[&](std::uint32_t at,unsigned bytes){for(unsigned i=0;i<bytes;++i)file_bytes.push_back(c.u8(at+i));};
                    for(auto [off,size]:{std::pair{0x28u,4u},{0x4bu,1u},{0x46u,1u},{0x47u,1u},{0xb0u,1u},
                        {0x3cu,2u},{0x3eu,2u},{0x40u,2u},{0xa8u,1u},{0xa9u,1u},{0x44u,1u},{0x42u,1u},
                        {0x3au,2u},{0x1cu,4u},{0x20u,4u}})append(object+off,size);
                    append(data+0xc000,4);append(rounds,0xf0);
                    for(unsigned i=0;i<3;++i)c.put32(entrants+i*8,i==1?0xffffffff:i);
                    append(entrants,24);append(data+0x7000,108);append(data+0x7100,54);
                    // Keep the destination initialized with valid serialized IDs even on a partial read.
                }
                if(entry==0x104bd550 || entry==0x104bcb20)c.put32(data+0x6004,data+0x7100);
                if(entry==0x104bd550 && mode>=24 && mode<28)c.put32(object+(mode==24?4:mode==25?0xa0:mode==26?0xa4:0xac),0);
                if(entry==0x104bd8c0) {
                    c.put16(object+0x3c,3);c.put8(object+0x45,mode<6?mode%2:1);c.put32(object+0x24,0xffffffff);
                    for(unsigned i=0;i<3;++i) {
                        auto round=rounds+i*0x78;c.put16(round+8,0);c.put16(round+14,mode>=6?0:0x200);
                        c.put16(round+0x1c,4u>>i);c.put16(round+0x1e,i?0:8);c.put16(round+0x20,0);c.put16(round+0x22,1);
                    }
                    if(mode>=12 && mode<20) {c.put16(object+0x3c,1);c.put16(rounds+0x1c,mode==12?0:4);}
                    if(mode>=20 && mode<28)c.put16(rounds+2*0x78+8,0x8c);
                    if(mode>=28 && mode<40) {c.put16(object+0x3c,7);for(unsigned i=0;i<7;++i){auto r=rounds+i*0x78;c.put16(r+8,mode&1?0x8c:0);c.put16(r+14,0x200);c.put16(r+0x1c,1);c.put16(r+0x1e,i?0:2);c.put16(r+0x20,0);c.put16(r+0x22,1);}}
                }
                if(entry==0x104bd8c0 && mode>=40) {
                    c.put16(object+0x3c,3);c.put8(object+0x45,1);
                    for(unsigned i=0;i<3;++i){auto r=rounds+i*0x78;c.put16(r+8,0);c.put16(r+14,0);c.put16(r+0x1c,i==0?4:1);c.put16(r+0x1e,i?0:8);c.put16(r+0x20,0);c.put16(r+0x22,1);}
                    if(mode==42 || mode==43){c.put16(rounds+0x1c,3);c.put16(rounds+0x1e,6);c.put16(rounds+2*0x78+0x1c,2);}
                    if(mode>=44 && mode<48)c.put16(rounds+0x78+0x1c,0);
                    if(mode>=48 && mode<64) {
                        c.put16(object+0x3c,3);c.put16(rounds+0x1c,2);c.put16(rounds+0x1e,4);
                        c.put16(rounds+0x78+14,0x200);c.put16(rounds+0x78+0x1c,2);c.put16(rounds+0x78+0x1e,3);
                        c.put32(object+0x24,mode%8==0?0xffffffff:mode%8==1?8:7);
                        c.put32(entrants+8,mode%8==2?0:data+0x30248);
                        c.put32(data+0x30248+0x3c,mode%8==3?0:data+0x45000);
                    }
                    if(mode==64)c.put32(object+0xa4,0);
                    if(mode==65){c.put16(object+0x3c,0);c.put8(object+0x45,0);}
                    if(mode==67){c.put16(object+0x3c,0);c.put16(rounds-0x70,0);c.put16(rounds-0x5e,1);}
                    if(mode>=66 && mode!=67){c.put16(object+0x3c,1);c.put16(rounds+8,0x8c);c.put16(rounds+0x1c,1);c.put16(rounds+0x1e,0);}
                }
                if(entry==0x104bc3c0 && mode>=40)failure=3;
                if(entry==0x104be7e0) {
                    constexpr std::uint32_t bounds[]={0,1,0xffffffff,65536,0x80000000,0x7fffffff};c.set(3,bounds[mode%6]);
                    if(mode>=6)c.put32(c.toc(-0x7b84),c.u32(c.toc(-0x7b88)));
                    if(mode>=12)c.set(3,std::uint32_t(0x9e3779b9u*mode+seed));
                }
                Observed observed;if(production_mode)s->memory.observer=&observed;
                bool exits=false;
                {auto oracle=cm::semantic_io_checks::clone(*s);try{original(*oracle,entry);}catch(const ApplicationExit&){exits=true;}catch(const std::exception& e){throw std::runtime_error(hex_address(entry)+" mode="+std::to_string(mode)+" seed="+std::to_string(seed)+" original: "+e.what());}}
                auto differences=compare_routine_call(*s,entry,original,rewrite,RoutineChecker::result_r3|RoutineChecker::result_f1);
                if(!differences.empty()) {
                    std::cout<<"DIFFERS "<<hex_address(entry)<<" mode="<<mode<<" seed="<<seed<<'\n';
                    for(const auto& d:differences)std::cout<<d<<'\n';
                    if(++failures==4)return 1;
                }else {if(exits)++terminated;else ++completed;}
                if(entry==0x104bc3c0 && mode==12 && seed==1)check_nested_selection(*s);
            }
        unsigned original_controls=0;
        for(auto entry:{0x104bc3c0u,0x104bc750u,0x104bc9b0u,0x104bca90u,0x104bcb20u,0x104bd550u,0x104bd8c0u,0x104be7e0u}) {
            auto s=cm::semantic_io_checks::clone(*base);Cpu c(*s);initialize(c,1,0,0);s->budget=s->executed;
            try{original(*s,entry);}catch(const std::exception& e){if(std::string_view(e.what())=="Native instruction budget exhausted" && s->pc==entry)++original_controls;else throw;}
        }
        if(original_controls!=8)throw std::runtime_error("original zero-budget controls failed");
        std::cout<<original_controls<<" original zero-budget controls passed\n";
        failure=io_failure=0;
        auto s=cm::semantic_io_checks::clone(*base);Cpu c(*s);initialize(c,1,3,0);c.set(3,object);c.set(4,1);
        const auto detected=[&](RoutineRunner runner,std::string_view prefix) {
            const auto differences=compare_routine_call(*s,0x104bc9b0,original,runner,RoutineChecker::result_r3);
            return !differences.empty() && std::ranges::all_of(differences,[&](const auto& difference){return difference.starts_with(prefix);});
        };
        if(!detected(corrupt,"r3 "))++failures;
        for(unsigned defect:{1u,2u}) {corrupt_trace=defect;if(!detected(rewrite,"byte 60010"))++failures;}
        std::cout<<completed<<" returned comparisons; "<<terminated<<" expected fatal exits; "<<failures<<" failures; result, argument and target trace controls tested\n";
        return failures?1:0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
