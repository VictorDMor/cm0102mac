// Independent generated reference, scripted Carbon boundaries and actual guest
// string length. No printer, game window or user's career is opened.
#include "siprint_adapter.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include "guest_values.hpp"
#include <array>
#include <iostream>
#include <limits>
#include <unistd.h>

#if defined(CM_SIPRINT_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
extern "C" void __llvm_profile_set_filename(const char*);
#endif
using namespace cm::aot;
using namespace cm::aot::guest;
namespace cm::aot { bool siprint_reference(State&); }
namespace {
constexpr std::uint32_t data=0x60000000, object=data+0x1000, lines=data+0x2000,
    buffer=data+0x10000, trace_at=data+0x30000, stack=0x71010000, sentinel=0xfffffff0;
constexpr std::array entries{0x1074c960u,0x1074c9a0u,0x1074ca50u,0x1074ca60u,
    0x1074cb50u,0x1074cc20u,0x1074cc30u,0x1074cd90u,0x1074d340u};
struct Case {
    std::uint32_t entry=entries[7], failure=0, columns=17;
    int count=5, deleting=1, width=500, height=40, spacing=10;
    unsigned enabled=1, resource=1, accepted=1, nth=1, width_mode=0, fp_edge=0;
    bool clobber=false, null_session=false, null_format=false, null_settings=false,
         null_ruler=false, null_object=false, empty_lines=false, fractional=false,
         write_on_failure=false;
};
Case active;
bool original_mode=false, production_mode=false, ownership_mode=false;
unsigned calls=0, selected_calls=0;
int font_size=0;
void trace(Cpu& c,std::uint32_t target) {
    if(++calls>500)throw std::runtime_error("fixture call bound");
    auto size=c.u32(trace_at);
    const auto add=[&](std::uint32_t value){c.put32(trace_at+4+size++*4,value);};
    add(target);add(c.lr());
    for(unsigned r=3;r<=10;++r)add(c.r(r));
    for(unsigned r=13;r<32;++r)add(c.r(r));
    for(unsigned f=0;f<8;++f)add(c.cr(f));
    for(unsigned bit=1280;bit<=1309;++bit)add(c.s.r<1>(bit));
    c.put32(trace_at,size);
}
bool probe(State& s) {
    Cpu c(s);const auto target=s.pc;
    if (ownership_mode && target==0x1077dbd0) {
        // Dependency instructions may run; the owned module must still return
        // with no instruction budget of its own, before and after this call.
        const auto saved_probe=dispatch_probe;dispatch_probe=nullptr;
        s.budget=s.executed+100000;
        try { c.call(target,c.lr()); }
        catch (...) { dispatch_probe=saved_probe;throw; }
        dispatch_probe=saved_probe;s.budget=s.executed;return true;
    }
    if(target>=entries.front() && target<0x1074d37c) {
        if(original_mode)return siprint_reference(s);
        if(production_mode)return false;
        if(siprint_lists(target)){siprint_run_rewrite(s,target);return true;}
        return false;
    }
    // Cpu::call lowers these leaves before consulting the probe. The original
    // must execute the same dependency instead of a conflicting fixture stub.
    if(run_leaf(s))return true;
    const bool carbon=target==0x10780150 || target==0x10780378 || target==0x107803c0 ||
        target==0x107803f0 || (target>=0x10780408 && target<=0x107804f8 && (target-0x10780408)%24==0) ||
        target==0x107806f0 || (target>=0x10780a68 && target<=0x10780bd0 && (target-0x10780a68)%24==0);
    const bool program=target==0x107735a0 || target==0x10775130 || target==0x10775170 ||
        target==0x1076fe80 || target==0x10772df0 || target==0x10739890 ||
        target==0x10768810 || target==0x1077b720 || target==0x10769b60;
    if(!carbon && !program)return false;
    if(ownership_mode)s.budget=s.executed;
    trace(c,target);
    const auto arg=c.r(3), arg4=c.r(4), arg5=c.r(5), arg6=c.r(6),arg7=c.r(7);
    const bool fail=active.failure==target && ++selected_calls==active.nth;
    auto answer=0u;
    if(carbon)c.put32(c.sp()+20,c.r(2));
    if(fail && !active.write_on_failure) { /* Failed services may leave outputs untouched. */ }
    else if(target==0x10780378)answer=0xfffffffe;
    else if(target==0x107803f0)c.put32(arg,data+0x4000);
    else if(target==0x10780408)c.put32(arg,active.null_session?0:data+0x4100);
    else if(target==0x10780150)answer=active.resource?data+0x4200:0;
    else if(target==0x10780420 || target==0x10780450)c.put32(target==0x10780420?arg4:arg,active.null_format?0:data+0x4300);
    else if(target==0x10780438)c.put8(arg5,1);
    else if(target==0x10780498) {
        c.put32(arg4,data+0x4400);
        // Opaque page-format bytes are supplied by the OS. The caller must
        // publish exactly this handle and must not change its contents.
        for(unsigned i=0;i<23;++i)c.put8(data+0x4500+i,i*19+7);
        c.put32(data+0x4400,data+0x4500);
    } else if(target==0x107804b0) {
        c.put32(data+0x4600,arg);c.put32(data+0x4604,arg4);c.put32(data+0x4608,arg5);
        for(unsigned i=0;i<23;++i)c.put8(data+0x4610+i,c.u8(c.u32(arg)+i));
    } else if(target==0x107806f0 || target==0x10780a98)c.put8(target==0x107806f0?arg5:arg6,active.accepted);
    else if(target==0x10780a68)c.put32(arg,active.null_settings?0:data+0x4700);
    else if(target==0x10780ac8)answer=7;
    else if(target==0x10780b10) {
        const double fraction=active.fractional?0.75:0;
        c.put_f64(arg4,11.25);c.put_f64(arg4+8,17.75);
        double bottom=11.25+active.height+fraction, right=17.75+active.width+fraction;
        if(active.fp_edge==1)right=std::numeric_limits<double>::infinity();
        if(active.fp_edge==2)right=-std::numeric_limits<double>::infinity();
        if(active.fp_edge==3)right=std::numeric_limits<double>::quiet_NaN();
        if(active.fp_edge==4)right=17.75+2147483648.0;
        if(active.fp_edge==5)bottom=std::numeric_limits<double>::infinity();
        if(active.fp_edge==6)bottom=std::numeric_limits<double>::quiet_NaN();
        c.put_f64(arg4+16,bottom);c.put_f64(arg4+24,right);
    } else if(target==0x10775130)answer=active.null_ruler?0:buffer;
    else if(target==0x10780b28)font_size=std::int16_t(arg);
    else if(target==0x10780b40) {
        if(active.width_mode==1)answer=0;
        else if(active.width_mode==2)answer=0x7fff;
        else answer=std::uint16_t(font_size*std::int16_t(arg5)/2);
    } else if(target==0x10780b58) {
        c.put16(arg,3);c.put16(arg+2,4);c.put16(arg+4,99);c.put16(arg+6,std::uint32_t(active.spacing-7));
    } else if(target==0x10739890) {
        c.put32(arg6,0x53495000);c.put32(arg7,0x43505000);
    } else if(target==0x10768810)answer=data+0x4800;
    else if(target==0x1077b720)c.put32(arg,c.r(8));
    if(fail)answer=0xffffff80;
    if(active.clobber) {
        for(unsigned r=4;r<=10;++r)c.set(r,0x41000000+calls*0x100+r);
        c.compare(0,17,29);c.set_f(1,3.5+calls);
    }
    c.set(3,answer);s.pc=c.lr();return true;
}
void initialize(Cpu& c) {
    c.s.memory.map(data,0x50000);c.s.memory.map(0x71000000,0x20000);
    c.s.memory.copy_in(data,std::vector<std::uint8_t>(0x50000,0xa5));
    c.s.memory.copy_in(0x71000000,std::vector<std::uint8_t>(0x20000,0xa5));
    for(unsigned r=0;r<32;++r)c.set(r,0x51000000+r*0x101);
    for(unsigned f=0;f<32;++f)c.set_f(f,1.25+f);
    for(unsigned f=0;f<8;++f)c.set_cr(f,f+1);
    c.set(1,stack);c.set(2,0x108bba90);c.set_lr(sentinel);
    if(active.fp_edge) { c.s.wr<1>(1280,1);c.s.wr<1>(1294,1); }
    c.put8(object,active.enabled);c.put16(object+2,0xfffd);c.put32(object+4,data+0x4000);
    c.put32(object+8,active.null_session?0:data+0x4100);c.put32(object+12,active.null_format?0:data+0x4300);
    for(unsigned i=0;i<40;++i) {
        const auto text=data+0x5000+i*0x100;
        c.put32(lines+i*4,active.empty_lines || i%3==1?0:text);
        c.put8(text,'A'+i%26);c.put8(text+1,'B');c.put8(text+2,0);
    }
    c.put32(c.r(2)-0x7ba8,data+0x8000);c.put32(c.r(2)-0x7bac,data+0x8100);c.put8(data+0x8100,1);
    c.put32(c.r(2)-0x7408,object);
    c.put32(trace_at,0);c.set(3,active.null_object?0:object);
    c.set(4,active.entry==entries[0]?std::uint32_t(active.deleting):lines);
    c.set(5,std::uint32_t(active.count));c.set(6,active.columns);
    c.s.budget=c.s.executed+1000000;
}
void original(State& s,std::uint32_t entry) {
    original_mode=true;calls=selected_calls=0;font_size=0;s.pc=entry;run_until(s,sentinel);
}
void rewrite(State& s,std::uint32_t entry) {
    original_mode=false;calls=selected_calls=0;font_size=0;
    if(production_mode){s.pc=entry;run_until(s,sentinel);}
    else siprint_run_rewrite(s,entry);
#if defined(CM_SIPRINT_COVERAGE)
    // LLVM expands %p before fork. Select the actual child PID explicitly so
    // returned comparisons retain independent profiles, without overwrites.
    const auto profile=std::string(CM_SIPRINT_PROFILE_DIR)+"/check-"+std::to_string(getpid())+".profraw";
    __llvm_profile_set_filename(profile.c_str());
    if(__llvm_profile_write_file()!=0)throw std::runtime_error("Cannot flush SIPRINT child coverage");
#endif
}
void corrupt(State& s,std::uint32_t entry){rewrite(s,entry);Cpu c(s);c.put8(object,c.u8(object)^1);}
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
        const auto add=[&](Case test){cases.push_back(test);};
        for(auto entry:entries)for(unsigned enabled:{0u,1u,128u,255u}) {
            Case t;t.entry=entry;t.enabled=enabled;add(t);
        }
        for(int deleting:{-32768,-1,0,1,32767,65536,65537})for(bool null:{false,true}) {
            Case t;t.entry=entries[0];t.deleting=deleting;t.null_object=null;add(t);
        }
        for(auto entry:{entries[1],entries[3],entries[4],entries[6]})
            for(unsigned resource:{0u,1u})for(unsigned accepted:{0u,1u,255u})
                for(bool null_session:{false,true})for(bool null_format:{false,true}) {
                    Case t;t.entry=entry;t.resource=resource;t.accepted=accepted;
                    t.null_session=null_session;t.null_format=null_format;add(t);
                    for(auto failure:{0x10780408u,0x10780420u,0x10780438u,0x10780450u,0x10780468u,0x10780498u,0x107806f0u}){
                        t.failure=failure;add(t);
                    }
                }
        for(unsigned columns:{0u,1u,8u,9u,16u,17u,80u,32767u,0xffffu,0x8000u,0x10000u,0x12340011u})
            for(int count:{-1,0,1,5,17}) {
                Case t;t.columns=columns;t.count=count;add(t);
            }
        for(auto failure:{0x10780408u,0x10780a68u,0x10780a80u,0x10780a98u,0x10780ab0u,
                          0x10780b10u,0x10780b70u,0x10780b88u,0x10780bd0u})
            for(unsigned nth:{1u,2u,3u})for(bool writes:{false,true}) {
                Case t;t.failure=failure;t.nth=nth;t.write_on_failure=writes;add(t);
            }
        for(unsigned mode:{0u,1u,2u})for(int width:{-32768,-1,0,1,500,32767})
            for(int height:{-1,1,40,32767}) {
                Case t;t.width_mode=mode;t.width=width;t.height=height;t.fractional=true;add(t);
            }
        for(unsigned accepted:{0u,1u,255u})for(unsigned mask=0;mask<32;++mask) {
            Case t;t.accepted=accepted;t.null_session=mask&1;t.null_settings=mask&2;
            t.null_ruler=mask&4;t.empty_lines=mask&8;t.null_format=mask&16;add(t);
        }
        for(unsigned edge=1;edge<=6;++edge){Case t;t.fp_edge=edge;add(t);}
        unsigned completed=0,failures=0;std::map<std::uint32_t,std::array<unsigned,2>> counts;
        for(bool production:{false,true}) {
            production_mode=production;
            for(const auto& test:cases)for(bool clobber:{false,true}) {
                if(failures>=6)break;
                active=test;active.clobber=clobber;
                auto s=cm::semantic_io_checks::clone(*base);Cpu c(*s);initialize(c);
                Observed observer;if(production)s->memory.observer=&observer;
                auto differences=compare_routine_call(*s,test.entry,original,rewrite,RoutineChecker::result_r3|RoutineChecker::result_f1);
                if(!differences.empty()) {
                    std::cout<<"DIFFERS "<<hex_address(test.entry)<<" production="<<production<<" fail="<<hex_address(test.failure)<<" nth="<<test.nth<<" columns="<<test.columns<<" count="<<test.count<<" resource="<<test.resource<<" accepted="<<test.accepted<<" clobber="<<clobber<<'\n';
                    for(const auto& d:differences)std::cout<<"  "<<d<<'\n';
                    if(++failures>=6)break;
                } else {++completed;++counts[test.entry][production?1:0];}
            }
            if(failures)break;
        }
        production_mode=false;active=Case{};auto s=cm::semantic_io_checks::clone(*base);Cpu c(*s);initialize(c);
        if(compare_routine_call(*s,active.entry,original,corrupt,RoutineChecker::result_r3).empty()){++failures;std::cout<<"FAIL corruption control\n";}
        else std::cout<<"ok deliberate memory corruption detected\n";
        unsigned ownership=0;
        for(auto entry:entries) {
            active=Case{};active.entry=entry;auto normal=cm::semantic_io_checks::clone(*base);Cpu nc(*normal);initialize(nc);
            normal->budget=normal->executed;Observed observer;normal->memory.observer=&observer;production_mode=true;ownership_mode=true;
            try{rewrite(*normal,entry);++ownership;}catch(const std::exception& e){++failures;std::cout<<"FAIL production ownership "<<hex_address(entry)<<' '<<e.what()<<'\n';}
            ownership_mode=false;
            auto ref=cm::semantic_io_checks::clone(*base);Cpu rc(*ref);initialize(rc);ref->budget=ref->executed;bool rejected=false;
            try{original(*ref,entry);}catch(const std::exception& e){rejected=std::string(e.what())=="Native instruction budget exhausted" && ref->pc==entry;}
            if(!rejected){++failures;std::cout<<"FAIL original budget control\n";}
        }
        for(const auto& [entry,n]:counts)std::cout<<hex_address(entry)<<" returned "<<n[0]<<" controlled + "<<n[1]<<" production\n";
        std::cout<<ownership<<"/9 observed production controls; "<<completed<<" returned comparisons; "<<failures<<" failures\n";
        return failures?1:0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
