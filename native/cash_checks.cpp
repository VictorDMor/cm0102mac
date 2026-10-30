// CASH original/C++ returned comparisons use the actual arithmetic, string
// and printf dependencies. Error dialogs and the Toolbox BlockMoveData import
// reached inside printf are explicit external fixtures.
#include "cash_adapter.hpp"
#include "cash_detail.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#if defined(CM_CASH_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
#endif
using namespace cm::aot;
using namespace cm::aot::cash::detail;
namespace cm::aot {bool cash_reference(State&);}
namespace {
constexpr unsigned data=0x60000000,amount_at=data+0x1000,output=data+0x2000,
    table=data+0x3000,trace_at=data+0x10000,stack=0x71010000,sentinel=0xfffffff0;
constexpr std::array entries{0x10009ef0u,0x10009f20u,0x1000a010u,0x1000a060u,0x1000a170u,
    0x1000a270u,0x1000a330u,0x1000a440u,0x1000b150u,0x1000b160u,0x1000b410u,0x1000c770u};
bool original_mode=false,production_mode=false,tracing=false,ownership_mode=false;
struct Case {
    unsigned entry;
    std::string name="default";
    std::uint64_t amount=1234567;
    unsigned flags=8,currency=0,index=3,capacity=200,count=7,table_null=0,output_null=0;
    unsigned alias=0,symbol_size=0,override_scale=0;
    std::uint64_t scale_bits=0;
};
Case active{entries[0]};
void trace(Cpu& c,unsigned target) {
    auto size=c.u32(trace_at);
    const auto add=[&](unsigned value){if(size>=10000)throw std::runtime_error("CASH trace bound");c.put32(trace_at+4+size++*4,value);};
    add(target);add(c.lr());
    for(unsigned r=3;r<=10;++r)add(c.r(r));
    for(unsigned r=13;r<32;++r)add(c.r(r));
    for(unsigned f=0;f<8;++f)add(c.cr(f));
    const auto bits=c.s.r<8>(guest::reg::fpr(1));add(unsigned(bits>>32));add(unsigned(bits));
    for(unsigned f=14;f<32;++f){const auto value=c.s.r<8>(guest::reg::fpr(f));add(unsigned(value>>32));add(unsigned(value));}
    c.put32(trace_at,size);
}
bool probe(State& s) {
    Cpu c(s);const auto target=s.pc;
    if(entries.front()<=target && target<0x1000cbc0) {
        if(original_mode)return cash_reference(s);
        if(production_mode)return false;
        if(cash_lists(target)){cash_run_rewrite(s,target);return true;}
        return false;
    }
    if(ownership_mode && entries.front()<=c.lr() && c.lr()<0x1000cbc0) {
        // The owned body has no instruction budget. Only its real external
        // dependency may spend instructions; restore the tripwire at return.
        const auto stop=c.lr();s.budget=s.executed+2000000;ownership_mode=false;
        try{run_until(s,stop);}catch(...){ownership_mode=true;throw;}
        ownership_mode=true;s.budget=s.executed;return true;
    }
    // Cpu::call resolves native leaves before the dispatch probe. Apply the
    // same leaf boundary to the reference; its result and memory still enter
    // the full-state comparison and the next non-leaf call's argument trace.
    if(run_leaf(s))return true;
    const auto caller=c.lr();
    if(tracing && entries.front()<=caller && caller<0x1000cbc0 &&
       target!=0x10773808 && target!=0x10773858)trace(c,target);
    if(target==0x10769b60){c.set(3,1);s.pc=c.lr();return true;}
    if(target==0xfff10000) {
        const auto bytes=s.memory.copy_out(c.r(3),c.r(5));s.memory.copy_in(c.r(4),bytes);s.pc=c.lr();return true;
    }
    return false;
}
void seed(Cpu& c) {
    for(unsigned r=0;r<32;++r)c.set(r,0x51000000+r*0x101);
    c.set(1,stack);c.set(2,0x108bba90);c.set_lr(sentinel);
    for(unsigned f=0;f<8;++f)c.set_cr(f,f+1);
    for(unsigned f=0;f<32;++f)c.set_f(f,1.25+f);
    for(unsigned bit=1280;bit<1312;++bit)c.s.wr<1>(bit,0);
    c.s.budget=c.s.executed+2000000;
}
void setup(Cpu& c,const Case& t) {
    seed(c);c.s.memory.map(data,0x40000);c.s.memory.map(stack-0x10000,0x20000);
    c.s.memory.copy_in(data,std::vector<std::uint8_t>(0x40000,0xa5));
    c.s.memory.copy_in(stack-0x10000,std::vector<std::uint8_t>(0x20000,0xa5));
    unsigned offset=0x4000;
    for(auto slot:{0x59d0u,0x59e0u,0x59e4u,0x59e8u,0x59ecu,0x59f0u}){c.put32(c.r(2)-slot,data+offset);offset+=0x100;}
    c.put32(c.r(2)-0x7ba8,data+0x5000);c.put32(c.r(2)-0x7bac,data+0x6000);
    c.put32(c.r(2)-0x7e64,data+0x38000);c.put32(data+0x38000,0xfff10000);c.put32(data+0x38004,c.r(2));
    c.put8(data+0x6000,1);c.put32(trace_at,0);
    // Establish scales and suffix text through the independent original path.
    original_mode=true;tracing=false;c.set(3,t.currency<25?t.currency:0);c.s.pc=0x1000a440;
    run_until(c.s,sentinel);seed(c);
    if(t.override_scale==1)for(auto slot:{0x3aecu,0x3af4u,0x3afcu})c.put64(c.r(2)-slot,t.scale_bits);
    if(t.override_scale==2)for(unsigned offset=0;offset<328;offset+=8)c.put64(c.toc(-0x59d4)+offset,t.scale_bits);
    if(t.symbol_size){const auto length=t.symbol_size==~0u?0:t.symbol_size;for(unsigned i=0;i<length;++i)c.put8(c.toc(-0x59d0)+i,'X');c.put8(c.toc(-0x59d0)+length,0);}
    c.put32(amount_at,unsigned(t.amount));
    for(unsigned i=0;i<8;++i)c.put32(table+i*4,i*10000+123);
    auto out=t.output_null?0:output,amount_address=amount_at;
    if(t.alias==1)for(auto slot:{0x59d0u,0x59e0u,0x59e4u,0x59e8u,0x59ecu,0x59f0u})c.put32(c.r(2)-slot,data+0x4700);
    if(t.alias==2)c.put32(c.r(2)-0x59d0,c.r(2)-0x3af4);
    if(t.alias==3)c.put32(c.r(2)-0x59d0,c.toc(-0x59d4));
    if(t.alias==4)c.put32(c.r(2)-0x59d0,c.r(2)-0x59e0);
    if(t.alias==5)out=stack-0x70;
    if(t.alias==6)out=stack-0x4c;
    if(t.alias==7){amount_address=stack-0x18;c.put32(amount_address,unsigned(t.amount));}
    c.set(3,amount_address);c.set(4,t.flags);
    if(t.entry==entries[0] || t.entry==entries[8]){}
    if(t.entry==entries[2])c.set({{4,unsigned(t.amount>>32)},{5,unsigned(t.amount)}});
    if(t.entry==entries[5])c.set(4,t.index);
    if(t.entry==entries[6])c.set({{4,t.index},{5,t.table_null?0:table},{6,t.count}});
    if(t.entry==entries[7])c.set(3,t.index);
    if(t.entry==entries[9])c.set({{3,out},{4,t.capacity},{5,t.index}});
    if(t.entry==entries[10])c.set({{3,out},{4,t.capacity},{5,unsigned(t.amount>>32)},{6,unsigned(t.amount)},{7,t.flags}});
    if(t.entry==entries[11])c.set({{3,unsigned(t.amount>>32)},{4,unsigned(t.amount)},{5,t.flags}});
    c.put32(trace_at,0);c.s.pc=t.entry;tracing=true;
}
void returned(State& s) {
    Cpu c(s);if(c.sp()!=stack || c.lr()!=sentinel || s.pc!=sentinel)throw std::runtime_error("CASH fixture did not return");
}
void original(State& s,unsigned entry){original_mode=true;s.pc=entry;run_until(s,sentinel);returned(s);}
void rewrite(State& s,unsigned entry) {
    original_mode=false;
    if(production_mode){s.pc=entry;run_until(s,sentinel);}else cash_run_rewrite(s,entry);
    returned(s);
#if defined(CM_CASH_COVERAGE)
    // Checker children use _exit. Flush into LLVM's locked %m merge pool;
    // parent process ids in filenames do not isolate forked children.
    if(__llvm_profile_write_file()!=0)throw std::runtime_error("Cannot flush CASH coverage");
#endif
}
void corrupt(State& s,unsigned entry){rewrite(s,entry);Cpu c(s);c.set(3,c.r(3)^1);}
void corrupt_output(State& s,unsigned entry){rewrite(s,entry);Cpu c(s);c.put8(output,c.u8(output)^1);}
void corrupt_amount(State& s,unsigned entry){rewrite(s,entry);Cpu c(s);c.put8(amount_at,c.u8(amount_at)^1);}
struct Observed final:MemoryObserver {
    void mapped(unsigned,std::uint64_t)override{}
    void written(unsigned,const std::uint8_t*,std::size_t)override{}
    void cleared(unsigned,std::uint64_t)override{}
};
}
int main(int argc,char** argv) {
    try {
        if(argc!=2)throw std::runtime_error("Expected private original export");
        auto base=std::make_unique<State>();load_program(*base,argv[1]);dispatch_probe=probe;
        std::vector<Case> cases;for(auto entry:entries)cases.push_back({entry});
        const bool quick=std::getenv("CM_CASH_QUICK");
        if(!quick) {
            for(auto entry:{entries[7],entries[9]})for(unsigned index=0;index<256;++index){Case t{entry};t.index=index;t.name="currency-"+std::to_string(index);cases.push_back(t);}
            for(auto entry:{entries[5],entries[6]})for(auto index:{0u,6u,7u,127u,128u,255u,0x12340003u}){Case t{entry};t.index=index;t.name="preset-"+std::to_string(index);cases.push_back(t);}
            for(auto entry:{entries[9],entries[10]})for(auto capacity:{0u,24u,25u,49u,50u,0x8000u,0xffffu,0x123400c8u}){Case t{entry};t.capacity=capacity;t.name="capacity-"+std::to_string(capacity);cases.push_back(t);}
            for(auto entry:{entries[9],entries[10]}){Case t{entry};t.output_null=1;t.name="null-output";cases.push_back(t);}
            {Case t{entries[6]};t.table_null=1;t.name="null-preset-table";cases.push_back(t);}
            for(auto value:{UINT64_C(0),UINT64_C(1),UINT64_C(1234),UINT64_C(123456),UINT64_C(1234567),UINT64_C(1000000),UINT64_C(1234567890),UINT64_C(123456789012345),UINT64_C(1000000000000000),UINT64_C(0x7fffffffffffffff),UINT64_C(0x8000000000000000),UINT64_MAX})
                for(auto flags:{0u,2u,4u,8u,12u,32u}){Case t{entries[10]};t.amount=value;t.flags=flags;t.name="format-"+std::to_string(value)+"-"+std::to_string(flags);cases.push_back(t);}
            for(auto entry:{entries[1],entries[2],entries[3],entries[4],entries[11]})
                for(auto value:{UINT64_C(0),UINT64_C(1),UINT64_C(1000),UINT64_C(50001),UINT64_C(1000001),UINT64_C(0x7fffffff),UINT64_C(0x80000000),UINT64_C(0xffffffff)})
                    for(auto flags:{0u,4u,8u,12u,32u}){Case t{entry};t.amount=value;t.flags=flags;t.name="value-"+std::to_string(value)+"-"+std::to_string(flags);cases.push_back(t);}
            for(unsigned currency=0;currency<25;++currency) {
                for(auto entry:{entries[1],entries[2],entries[3],entries[4],entries[5],entries[6]}){Case t{entry};t.currency=currency;t.name="configured-value-"+std::to_string(currency);cases.push_back(t);}
                for(unsigned digits=0;digits<=15;++digits)for(auto flags:{0u,2u})for(auto tail:{0u,1u,10u,11u}) {
                    Case t{entries[10]};t.currency=currency;t.flags=flags;
                    std::uint64_t power=1;for(unsigned i=1;i<digits;++i)power*=10;
                    std::uint64_t fraction_scale=1;
                    if(digits>3)for(unsigned n=0;n<((digits-1)/3)*3-2;++n)fraction_scale*=10;
                    t.amount=digits==0?0:power+std::min(std::uint64_t(tail)*fraction_scale,power-1);
                    t.name="layout-"+std::to_string(currency)+"-"+std::to_string(digits)+"-"+std::to_string(flags)+"-"+std::to_string(tail);cases.push_back(t);
                }
                for(unsigned alias=1;alias<=4;++alias){Case t{entries[7]};t.index=currency;t.alias=alias;t.name="mapped-currency-alias-"+std::to_string(currency)+"-"+std::to_string(alias);cases.push_back(t);}
            }
            for(auto entry:{entries[1],entries[2],entries[3],entries[4],entries[11]})for(auto bits:{
                UINT64_C(0),UINT64_C(0x8000000000000000),UINT64_C(0x3ff8000000000000),UINT64_C(0xbff8000000000000),
                UINT64_C(0x7ff0000000000000),UINT64_C(0xfff0000000000000),UINT64_C(0x7ff8000012345678),UINT64_C(0x7ff0000000000001)}) {
                Case t{entry};t.override_scale=1;t.scale_bits=bits;t.flags=entry==entries[11]?8:32;t.name="noncanonical-scale-"+std::to_string(bits);cases.push_back(t);
            }
            for(unsigned index=0;index<25;++index)for(auto bits:{UINT64_C(0),UINT64_C(0x8000000000000000),UINT64_C(0x7ff8000012345678),UINT64_C(0x7ff0000000000001)}) {
                Case t{entries[7]};t.index=index;t.override_scale=2;t.scale_bits=bits;t.name="noncanonical-currency-coefficients-"+std::to_string(index)+"-"+std::to_string(bits);cases.push_back(t);
            }
            for(auto size:{~0u,1u,127u,128u,255u,256u}){Case t{entries[0]};t.symbol_size=size;t.name="mapped-symbol-length-"+std::to_string(size);cases.push_back(t);}
            for(auto entry:{entries[1],entries[2],entries[3],entries[4]}){Case t{entry};t.alias=7;t.name="mapped-amount-frame-alias";cases.push_back(t);}
            for(auto alias:{5u,6u})for(auto flags:{0u,2u}){Case t{entries[10]};t.alias=alias;t.flags=flags;t.name="mapped-output-frame-alias-"+std::to_string(alias)+"-"+std::to_string(flags);cases.push_back(t);}
            for(auto count:{0u,1u,3u,4u,0x8000u,0xffffu,0x12340007u}){Case t{entries[6]};t.count=count;t.name="signed-preset-count-"+std::to_string(count);cases.push_back(t);}
            Cpu source(*base);const auto numbers=source.toc(-0x59d4);
            for(auto offset:{0x60u,0x40u,0x118u,0xc0u,0x168u,0x1a8u,0x1b0u,0x160u,0x170u,0x138u,0x180u,0x188u,0x190u,0x148u,0x150u})
                for(auto flags:{4u,8u,12u,0x12340004u,0xffffff08u})for(int delta:{-1,0,1}){
                    Case t{entries[11]};t.amount=std::uint64_t(std::int64_t(source.f64(numbers+offset))+delta);t.flags=flags;
                    t.name="quantum-boundary-"+std::to_string(offset)+"-"+std::to_string(flags)+"-"+std::to_string(delta);cases.push_back(t);
                }
            for(auto value:{UINT64_C(0x7fffffffffffffff),UINT64_C(0x8000000000000000),UINT64_MAX})
                for(auto flags:{0u,4u,8u,12u}){Case t{entries[11]};t.amount=value;t.flags=flags;t.name="signed64-quantum-"+std::to_string(value)+"-"+std::to_string(flags);cases.push_back(t);}
        }
        if(const auto path=std::getenv("CM_CASH_MANIFEST")) {
            std::ofstream manifest(path);
            for(const auto& t:cases)manifest<<hex_address(t.entry)<<' '<<t.name<<" amount="<<t.amount<<" flags="<<t.flags<<" currency="<<t.currency<<" index="<<t.index<<" capacity="<<t.capacity<<" count="<<t.count<<" table_null="<<t.table_null<<" output_null="<<t.output_null<<" alias="<<t.alias<<" symbol_size="<<t.symbol_size<<" override_scale="<<t.override_scale<<" scale_bits="<<t.scale_bits<<'\n';
            if(!manifest)throw std::runtime_error("Cannot write CASH case manifest");
        }
        const auto only=std::getenv("CM_CASH_ONLY");const auto filter=only?std::strtoul(only,nullptr,0):0;
        unsigned completed=0,attempted=0,failures=0;std::map<unsigned,std::array<unsigned,2>> counts;
        for(bool production:{false,true}) {
        if(production && std::getenv("CM_CASH_CONTROLLED_ONLY"))break;
        production_mode=production;
        for(const auto& test:cases) {
            if(failures>=8)break;if(filter && test.entry!=filter)continue;active=test;
            auto s=cm::semantic_io_checks::clone(*base);Cpu c(*s);setup(c,test);Observed observer;if(production)s->memory.observer=&observer;
            const auto differences=compare_routine_call(*s,test.entry,original,rewrite,RoutineChecker::result_r3|RoutineChecker::result_f1);++attempted;
            if(differences.empty()){++completed;++counts[test.entry][production?1:0];}
            else{++failures;std::cout<<"DIFFERS "<<(production?"production ":"controlled ")<<hex_address(test.entry)<<' '<<test.name<<'\n';for(const auto& d:differences)std::cout<<"  "<<d<<'\n';}
        }
        }
        production_mode=false;
        if(!failures){active=Case{entries[8]};auto s=cm::semantic_io_checks::clone(*base);Cpu c(*s);setup(c,active);
            if(compare_routine_call(*s,active.entry,original,corrupt,RoutineChecker::result_r3).empty()){++failures;std::cout<<"FAIL corruption control\n";}
            else std::cout<<"ok deliberate corruption detected\n";
            for(auto entry:{entries[3],entries[10]}) {
                active=Case{entry};auto state=cm::semantic_io_checks::clone(*base);Cpu cpu(*state);setup(cpu,active);
                if(compare_routine_call(*state,entry,original,entry==entries[3]?corrupt_amount:corrupt_output,RoutineChecker::result_r3|RoutineChecker::result_f1).empty()){
                    ++failures;std::cout<<"FAIL amount/text corruption control\n";
                }else std::cout<<"ok amount/text corruption detected "<<hex_address(entry)<<'\n';
            }
        }
        unsigned ownership=0;
        if(!std::getenv("CM_CASH_CONTROLLED_ONLY"))for(auto entry:entries) {
            active=Case{entry};auto state=cm::semantic_io_checks::clone(*base);Cpu cpu(*state);setup(cpu,active);
            Observed observer;state->memory.observer=&observer;state->budget=state->executed;
            production_mode=true;ownership_mode=true;
            try{rewrite(*state,entry);++ownership;}catch(const std::exception& e){++failures;std::cout<<"FAIL ownership "<<hex_address(entry)<<' '<<e.what()<<'\n';}
            ownership_mode=false;production_mode=false;
            auto reference=cm::semantic_io_checks::clone(*base);Cpu rc(*reference);setup(rc,active);reference->budget=reference->executed;bool rejected=false;
            try{original(*reference,entry);}catch(const std::exception& e){rejected=std::string(e.what())=="Native instruction budget exhausted" && reference->pc==entry;}
            if(!rejected){++failures;std::cout<<"FAIL original budget control "<<hex_address(entry)<<'\n';}
        }
        std::cout<<ownership<<"/12 observed normal-production ownership controls\n";
        for(const auto& [entry,n]:counts)std::cout<<hex_address(entry)<<" returned "<<n[0]<<" controlled + "<<n[1]<<" production\n";
        std::cout<<completed<<" returned comparisons / "<<attempted<<" attempts; "<<failures<<" failures\n";return failures?1:0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
