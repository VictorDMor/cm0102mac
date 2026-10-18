#include "contract_storage.hpp"
#include "semantic_io_checks.hpp"
#include <bit>
#include <set>

namespace cm::aot {bool contract_storage_reference(State&);void contract_storage_fixture_tables(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace storage=cm::aot::contract_storage;
constexpr std::uint32_t manager=fixture+0x1000,staff=fixture+0x2000,data=fixture+0x3000,rows=fixture+0x8000;
constexpr std::uint32_t relocated=fixture+0x10000,statistics=fixture+0x20000,globals=fixture+0x28000,diagnostics=fixture+0x30000,tables=fixture+0x40000;
constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,2> ranges{{{0x10078e70,0x10078ef8},{0x10078f00,0x10079048}}};
struct Options {
    unsigned interrupt=0;
    bool allocation_fail=false,nested=true,mutate_staff=false,mutate_controller=false,mutate_final_count=false;
    bool own_negative=false,child_negative=false;
    std::uint32_t slot=2,allocation_result=relocated;
};
struct Trace final:SemanticGuestObserver {
    Options options;std::vector<std::uint32_t> sites;
    explicit Trace(Options value):options(value){}
    void before_step(State& s,std::uint32_t pc)override {
        sites.push_back(pc);
        if(options.mutate_final_count && pc==0x1007902c)s.memory.write<4>(manager+8,17);
        if(options.mutate_staff && pc==0x10078ec4)s.memory.write<4>(staff,3);
    }
};
thread_local Trace* original_trace=nullptr;
void modern_root(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    if(s.pc==storage::bind_primary_entry)storage::bind_primary(s,d,o);
    else {require(s.pc==storage::reserve_entry,"Known prepared contract storage root");storage::reserve_record(s,d,o);}
}
struct Event {std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Model final:SemanticDependencyDispatch {
    bool original;Options options;Trace& trace;std::vector<Event> calls;
    Model(bool old,Options o,Trace& t):original(old),options(o),trace(t){}
    void reference(State& s,std::uint32_t stop=return_sentinel) {
        const auto entry=s.pc;const auto range=std::find_if(ranges.begin(),ranges.end(),[&](auto r){return r.first==entry;});
        require(range!=ranges.end(),"Known original prepared contract storage root");
        while(s.pc!=stop) {
            if(s.pc>=entry && s.pc<range->second)require(contract_storage_reference(s),"Complete contract storage reference");
            else call(s,s.pc,std::uint32_t(s.r<4>(4128)));
        }
    }
    static std::vector<std::uint8_t> text(State& s,std::uint32_t pointer) {
        std::vector<std::uint8_t> value;
        for(unsigned n=0;n<4096;++n){const auto c=std::uint8_t(s.memory.read<1>(pointer+n));if(!c)return value;value.push_back(c);}
        throw std::runtime_error("Unterminated contract storage fixture text");
    }
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override {
        constexpr std::array allowed{storage::reserve_entry,storage::initializer_entry,0x107751b0u,0x10775130u,0x10739890u,0x10768810u,0x1077b720u,0x10727890u,0x10769b60u};
        require(std::find(allowed.begin(),allowed.end(),target)!=allowed.end(),"Known contract storage dependency "+hex_address(target));
        const auto serial=calls.size();calls.push_back({target,continuation,s.registers,{}});
        try {
            if(target==storage::reserve_entry && options.nested) {
                if(original)reference(s,continuation);else modern_root(s,this,&trace);
            } else {
                const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};
                const auto first=arg(3),second=arg(4),third=arg(5);std::uint32_t result=0;
                if(target==storage::reserve_entry)result=options.slot;
                if(target==0x107751b0 || target==0x10775130) {
                    result=options.allocation_fail?0:options.allocation_result;
                    // A bounded, stateful allocator fixture retains a known
                    // old prefix and relocates it; requested size/ABI remain
                    // fully compared even for 32-bit capacity-size wrap.
                    if(result && target==0x107751b0)s.memory.copy_in(result,s.memory.copy_out(first,400));
                    if(result && target==0x10775130)s.memory.clear(result,400);
                    if(options.mutate_controller)s.memory.write<4>(manager+8,7);
                }
                if(target==storage::initializer_entry) {
                    // Keep this isolated reservation/binding matrix focused on
                    // its stateful dependency boundary. The separate initializer
                    // matrix executes the complete three-operation composition.
                    s.memory.write<4>(first+4,s.memory.read<4>(second));s.memory.write<4>(first+12,third);
                    s.memory.write<1>(first+0x38,s.memory.read<1>(first+0x38)|1);result=0x12345678;
                }
                if(target==0x10739890){(void)text(s,first);s.memory.write<4>(arg(6),0x12345678);s.memory.write<4>(arg(7),0x87654321);}
                if(target==0x10768810)result=0x10203040;
                if(target==0x1077b720) {
                    const auto format=text(s,second);const auto operation=text(s,arg(9));
                    s.memory.write<4>(first,third);s.memory.write<4>(first+4,arg(8));
                    s.memory.copy_in(first+8,format);s.memory.write<1>(first+8+format.size(),0);
                    s.memory.copy_in(first+0x80,operation);s.memory.write<1>(first+0x80+operation.size(),0);
                }
                if(target==0x10727890){auto value=text(s,second);s.memory.copy_in(first,value);s.memory.write<1>(first+value.size(),0);if(second!=tables+0xb8)s.memory.write<4>(first+0x200,third);}
                if(target==0x10769b60){auto title=text(s,first),message=text(s,second);s.memory.write<4>(globals+0x100,std::uint32_t(title.size()));s.memory.write<4>(globals+0x104,std::uint32_t(message.size()));}
                for(unsigned n=0;n<=12;++n)if(n!=1 && n!=2)s.wr<4>(n*4,0xda000000+n*0x101);
                for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-40.0-n));
                s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=continuation;
            }
            if(options.child_negative && !original)s.memory.write<1>(globals+0x180,1);
            calls[serial].after=s.registers;
            if(options.interrupt && calls.size()>=options.interrupt)throw std::runtime_error("Fixture contract storage dependency interruption");
        }catch(...){calls[serial].after=s.registers;throw;}
    }
};
std::unique_ptr<State> seed(unsigned index) {
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);s->memory.map(fixture,0x50000);
    s->memory.map(stack-0x2000,0x3000);s->memory.map(toc-0x8000,0x9000);s->pc=storage::prepared_entries[index];
    s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->budget=10000;
    s->wr<4>(12,manager);s->wr<4>(16,index==0?staff:1);s->wr<4>(20,data);
    s->memory.write<4>(manager,rows);s->memory.write<4>(manager+4,4);s->memory.write<4>(manager+8,2);s->memory.write<4>(staff,2);
    for(unsigned n=0;n<8;++n)s->memory.write<4>(rows+n*100,n+10);
    s->memory.write<4>(statistics+200,-1);s->memory.write<4>(globals,statistics);
    for(auto [offset,pointer]:{std::pair{-0x7b9c,globals},std::pair{-0x7ba8,diagnostics},std::pair{-0x79dc,diagnostics+0x1000},std::pair{-0x79e0,diagnostics+0x2000}})s->memory.write<4>(toc+offset,pointer);
    contract_storage_fixture_tables(*s,toc,tables);return s;
}
unsigned cases=0,completed=0,interrupted=0,negatives=0;std::set<std::uint32_t> sites;
struct Result {std::unique_ptr<State> state;std::vector<Effects::Access> accesses;std::vector<Event> calls;};
Result compare(const State& input,Options options={}) {
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();
    a->memory.date_observer=&x;b->memory.date_observer=&y;Trace old_trace(options),new_trace(options);Model old(true,options,old_trace),modern(false,options,new_trace);
    const auto run=[&](State& s,Model& m,bool original){try{if(original){original_trace=&old_trace;m.reference(s);}else modern_root(s,&m,&new_trace);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="Contract storage entry="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected.empty() || expected=="Native instruction budget exhausted" || expected=="Fixture contract storage dependency interruption" || expected.starts_with("Unmapped data access at "),label+" unexpected fixture failure: "+expected);
    require(expected==actual,label+" original="+expected+" modern="+actual);
    if(options.own_negative)b->wr<4>(12,b->r<4>(12)^1);
    bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative && !options.child_negative)throw;rejected=true;}
    require(rejected==(options.own_negative||options.child_negative),label+" discriminating negative");
    if(!rejected){require(x.bytes==y.bytes,label+" ordered effects");require(old.calls==modern.calls,label+" dependency ABI");require(a->executed==b->executed,label+" accounting");require(old_trace.sites==new_trace.sites,label+" source order");}
    ++cases;completed+=expected.empty()&&!rejected;interrupted+=!expected.empty();negatives+=rejected;if(expected.empty())sites.insert(old_trace.sites.begin(),old_trace.sites.end());
    return{std::move(a),x.log,old.calls};
}
void boundaries(const State& input,Options options={}) {
    const auto base=compare(input,options);require(base.state->pc==return_sentinel,"Complete selected contract storage boundary fixture");
    const auto steps=base.state->executed-input.executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto s=clone(input);s->budget=s->executed+cut;compare(*s,options);}
    std::set<std::uint32_t> bytes;for(auto access:base.accesses)if(access.size){bytes.insert(access.address);bytes.insert(access.address+std::uint32_t(access.size)-1);}
    std::set<std::uint32_t> holes;
    if(!bytes.empty())for(unsigned n=0;n<6;++n){auto it=bytes.begin();std::advance(it,std::ptrdiff_t(n*(bytes.size()-1)/5));holes.insert(*it);}
    for(auto hole:holes) {
        auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;s->executed=input.executed;s->budget=input.budget;
        for(auto [start,size]:input.memory.mapped_ranges()) {
            if(hole<start || std::uint64_t(hole)>=std::uint64_t(start)+size){s->memory.map(start,size);s->memory.copy_in(start,input.memory.copy_out(start,size));continue;}
            for(auto [address,count]:{std::pair{start,std::uint64_t(hole-start)},std::pair{hole+1,std::uint64_t(start)+size-hole-1}})
                if(count){s->memory.map(address,count);s->memory.copy_in(address,input.memory.copy_out(address,count));}
        }
        compare(*s,options);
    }
    for(unsigned n=1;n<=base.calls.size();++n){auto o=options;o.interrupt=n;compare(input,o);}
}
void reservation_cases() {
    for(auto mode:{0u,1u,128u,255u,256u,257u,0xffffffffu})for(auto count:{0u,1u,3u})for(auto spare:{0u,1u})for(auto hole:{0u,1u,3u,8u}) {
        auto s=seed(1);s->wr<4>(16,mode);s->memory.write<4>(manager+8,count);s->memory.write<4>(manager+4,count+spare);s->memory.write<4>(rows+hole*100,-1);
        auto result=compare(*s);const bool reused=!(mode&255) && hole<count;
        require(result.state->r<4>(12)==(reused?hole:count),"Independent reused/append contract index");
        require(result.state->memory.read<4>(manager+8)==count+!reused,"Independent live used count");
        require(result.state->memory.read<4>(manager+4)==count+spare+(!reused && !spare?200:0),"Independent capacity growth");
    }
    for(auto mode:{0u,1u})for(auto pointer:{0u,rows})for(auto fail:{false,true}) {
        auto s=seed(1);s->wr<4>(16,mode);s->memory.write<4>(manager,pointer);s->memory.write<4>(manager+4,0);s->memory.write<4>(manager+8,0);Options o;o.allocation_fail=fail;
        auto result=compare(*s,o);require(result.state->r<4>(12)==(fail?0xffffffffu:0u),"Independent allocation failure result");
        require(result.state->memory.read<4>(manager)==(fail?0:relocated) && result.state->memory.read<4>(manager+4)==200 && result.state->memory.read<4>(manager+8)==unsigned(!fail),"Allocation failure retains capacity change and pointer loss");boundaries(*s,o);
    }
    boundaries(*seed(1));
    for(auto count:{0x7fffffffu,0x80000000u,0xffffffffu,0xffffff38u})for(auto full:{false,true}) {
        auto s=seed(1);s->memory.write<4>(manager+8,count);s->memory.write<4>(manager+4,full?count:count-1);
        const auto result=compare(*s);require(result.state->r<4>(12)==count && result.state->memory.read<4>(manager+8)==count+1,"Independent wrapped used count");
        if(full){const auto& call=result.calls.front();State abi;abi.registers=call.before;require(call.target==0x107751b0 && abi.r<4>(16)==(count+200u)*100u,"Independent wrapped allocation size and original realloc branch");}
    }
    {auto s=seed(1);s->wr<4>(16,0);s->memory.write<4>(rows,-1);boundaries(*s);}
    {auto s=seed(1);s->memory.write<4>(manager+4,2);Options o;o.mutate_controller=true;boundaries(*s,o);}
    {Options o;o.mutate_final_count=true;auto result=compare(*seed(1),o);require(result.state->r<4>(12)==16,"Original rereads used count before returning");boundaries(*seed(1),o);}
}
void binding_cases() {
    boundaries(*seed(0));
    for(auto slot:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto id:{0u,2u,0x40000000u,0x7fffffffu,0x80000000u,0xffffffffu}) {
        auto s=seed(0);s->memory.write<4>(staff,id);Options o;o.nested=false;o.slot=slot;const auto result=compare(*s,o);
        const bool accepted=std::bit_cast<std::int32_t>(slot)>=0;
        require(result.calls.size()==(accepted?2u:1u),"Independent signed binder slot gate");
        if(accepted){require(result.state->memory.read<4>(statistics+id*100u)==slot && result.state->memory.read<4>(rows+slot*100u)==id,"Independent contract/statistics binding");}
    }
    for(auto fail:{false,true}){auto s=seed(0);s->memory.write<4>(manager+4,2);Options o;o.allocation_fail=fail;boundaries(*s,o);}
    {Options o;o.mutate_staff=true;auto result=compare(*seed(0),o);require(result.state->memory.read<4>(statistics+200)==2 && result.state->memory.read<4>(rows+200)==3,"Original binder rereads live staff ID");boundaries(*seed(0),o);}
    for(auto alias:{manager,rows,statistics}){auto s=seed(0);s->wr<4>(20,alias);boundaries(*s);}
    {auto s=seed(0);s->memory.write<4>(staff,0);s->memory.write<4>(globals,manager);const auto result=compare(*s);require(result.state->memory.read<4>(manager)==2 && result.calls.size()==1,"Statistics alias commits controller mutation before original pointer fault");}
    {auto s=seed(0);s->memory.write<4>(manager,manager-200);const auto result=compare(*s);require(result.state->memory.read<4>(manager)==2 && result.calls.size()==2,"Destination alias reloads changed controller before initializer dependency");}
}
}
namespace cm::aot {void contract_storage_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(){try {
    reservation_cases();binding_cases();
    for(unsigned index=0;index<2;++index) {
        Options o;o.own_negative=true;compare(*seed(index),o);o.own_negative=false;o.child_negative=true;auto s=seed(index);s->memory.write<4>(manager+4,2);compare(*s,o);
        s=seed(index);constexpr auto moved=toc+0x10000;s->memory.map(moved-0x8000,0x9000);s->memory.copy_in(moved-0x8000,s->memory.copy_out(toc-0x8000,0x9000));s->wr<4>(8,moved);boundaries(*s);
    }
    for(auto [start,end]:ranges)for(auto pc=start;pc<end;pc+=4)if(!sites.contains(pc))std::cerr<<"Uncovered contract storage "<<hex_address(pc)<<std::endl;
    require(sites.size()==116,"Complete prepared reservation/binding source coverage");
    std::cout<<"Prepared contract storage comparisons="<<cases<<" completions="<<completed<<" interruptions="<<interrupted<<" negatives="<<negatives<<" sites="<<sites.size()<<" initializerDependencyFixture=1"<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
