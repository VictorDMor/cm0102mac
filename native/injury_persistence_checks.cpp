#include "injury_persistence.hpp"
#include "semantic_io_checks.hpp"
#include <set>

namespace cm::aot {bool injury_persistence_reference(State&);void injury_persistence_fixture_tables(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace persistence=cm::aot::injury_persistence;
constexpr std::uint32_t manager=fixture+0x1000,objects=fixture+0x2000,rows=fixture+0x10000,strings=fixture+0x4000,tables=fixture+0x5000;
constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,2> ranges{{{0x10184530,0x10184e58},{0x10184e60,0x10185404}}};
struct Options {
    unsigned interrupt=0,io_failure=0,allocation_failure=0;
    std::uint32_t version=0x3e024,record_count=2,first_count=1,second_count=1,list_count=2;
    bool open_failure=false,own_negative=false,child_negative=false,mutate_count=false;
    std::uint32_t force_pc=0,force_value=0;unsigned force_register=0;int force_cr=-1;
};
struct Trace final:SemanticGuestObserver {
    Options options;std::vector<std::uint32_t> sites;
    explicit Trace(Options value):options(value){}
    void before_step(State& s,std::uint32_t pc)override {
        sites.push_back(pc);if(pc!=options.force_pc)return;s.wr<4>(options.force_register*4,options.force_value);
        if(options.force_cr>=0)s.wr<1>(2304,(s.r<1>(2304)&0xf0)|unsigned(options.force_cr)|(s.r<1>(1024)&1));
    }
};
thread_local Trace* original_trace=nullptr;
struct Event {std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
void modern_root(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    if(s.pc==persistence::load_entry)persistence::load_controller(s,d,o);
    else {require(s.pc==persistence::save_entry,"Known injury persistence root");persistence::save_controller(s,d,o);}
}
struct Model final:SemanticDependencyDispatch {
    bool original;Options options;Trace& trace;std::vector<Event> calls;std::vector<std::uint8_t> output;
    unsigned position=0,io_calls=0,allocations=0,iterations=0;
    Model(bool old,Options value,Trace& observer):original(old),options(value),trace(observer){}
    void reference(State& s,std::uint32_t stop=return_sentinel) {
        const auto entry=s.pc;const auto range=std::find_if(ranges.begin(),ranges.end(),[&](auto r){return r.first==entry;});require(range!=ranges.end(),"Known original injury persistence root");
        while(s.pc!=stop){if(s.pc>=entry && s.pc<range->second)require(injury_persistence_reference(s),"Complete injury persistence reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));}
    }
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override {
        constexpr std::array<std::uint32_t,16> allowed{0x10739890,0x10768810,0x1077b720,0x10769b60,0x107475b0,0x10775130,0x10747900,0x10747a80,0x10775550,0x10717890,0x10718130,0x10738c20,0x10773520,0x10738c00,0x10738d10,0x10738e00};
        require(std::find(allowed.begin(),allowed.end(),target)!=allowed.end() || target==0x10747cc0,"Known persistence dependency "+hex_address(target));
        Event event{target,continuation,s.registers,{}};const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};
        s.memory.write<4>(fixture+0x30000+unsigned(calls.size())*4,target^arg(3));std::uint32_t result=0;
        if(target==0x107475b0){require(arg(4)==tables+0x71 && arg(5)<=1 && arg(6)==0x16 && arg(7)==4,"Original archive open ABI");result=options.open_failure?0:objects+0x800;}
        if(target==0x10775130 || target==0x10773520){++allocations;result=options.allocation_failure==allocations?0:rows+allocations*0x1000;}
        if(target==0x10747900){s.memory.write<4>(arg(3),0);}
        if(target==0x10775550)result=options.version;
        if(target==0x10739890){s.memory.write<4>(arg(6),0x12345678);s.memory.write<4>(arg(7),0x87654321);}
        if(target==0x1077b720){s.memory.write<4>(arg(3),arg(4));s.memory.write<4>(arg(3)+4,arg(5));}
        if(target==0x10717890){s.memory.write<4>(arg(3),arg(4));s.memory.write<4>(arg(3)+4,arg(5));}
        if(target==0x10718130)result=0x12340000+unsigned(calls.size());
        if(target==0x10738c00){result=arg(3);s.memory.write<2>(arg(3),0);}
        if(target==0x10738d10){s.memory.write<2>(arg(3),s.memory.read<2>(arg(3))+1);s.memory.write<4>(arg(3)+4,s.memory.read<4>(arg(4)));}
        if(target==0x10738e00){result=iterations++<2?objects+0x1000+(iterations-1)*8:0;s.memory.write<4>(arg(3)+12,result);}
        if(target==0x10747a80 || target==0x10747cc0) {
            ++io_calls;require(arg(4)<=0x100 && arg(5)<=100,"Bounded fixture archive request");result=arg(5);if(io_calls==options.io_failure)result=result?result-1:1;
            const auto bytes=std::uint32_t(arg(4)*arg(5));const auto partial=io_calls==options.io_failure&&bytes?bytes-1:bytes;
            if(target==0x10747a80) {
                for(unsigned n=0;n<partial;++n)s.memory.write<1>(arg(3)+n,(position+n)*37+11);
                if(partial==bytes) {
                    if(continuation==0x10184740)s.memory.write<4>(arg(3),options.record_count);
                    if(continuation==0x101849e4)s.memory.write<4>(arg(3),options.first_count);
                    if(continuation==0x10184b68)s.memory.write<4>(arg(3),options.second_count);
                    if(continuation==0x10184cec)s.memory.write<2>(arg(3),options.list_count);
                }
            } else for(unsigned n=0;n<partial;++n)output.push_back(std::uint8_t(s.memory.read<1>(arg(3)+n)));
            position+=partial;
            if(options.mutate_count && io_calls==1)s.memory.write<4>(objects+0x200,3);
        }
        for(unsigned n=0;n<=12;++n)if(n!=1 && n!=2)s.wr<4>(n*4,0xac000000+n*17);
        for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-20.0-n));
        s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=continuation;
        if(options.child_negative && !original)s.wr<4>(48,s.r<4>(48)^1);
        event.after=s.registers;calls.push_back(event);
        if(options.interrupt && calls.size()>=options.interrupt)throw std::runtime_error("Fixture injury persistence dependency interruption");
    }
};
std::unique_ptr<State> seed(unsigned index) {
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);s->memory.map(fixture,0x40000);s->memory.map(stack-0x4000,0x5000);s->memory.map(toc-0x8000,0x9000);
    s->pc=persistence::entries[index];s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->budget=100000;
    for(auto [slot,pointer]:{std::pair{-0x5438,strings},std::pair{-0x7ba8,objects},std::pair{-0x7bac,objects+0x100},std::pair{-0x7be4,objects+0x200},std::pair{-0x7b80,objects+0x500},std::pair{-0x7b74,objects+0x600}})s->memory.write<4>(toc+slot,pointer);
    s->memory.write<4>(objects+0x200,2);s->memory.write<4>(objects+0x600,objects+0x700);s->memory.write<4>(objects+0x500,0x12345678);s->memory.write<4>(objects+0x504,0x87654321);
    for(unsigned n=0;n<0x7000;++n)s->memory.write<1>(rows+n,n*19+7);
    for(auto [off,value]:{std::pair{0,rows},std::pair{4,rows+0x1000},std::pair{8,2u},std::pair{0x10,rows+0x2000},std::pair{0x14,1u},std::pair{0x18,1u},std::pair{0x1c,rows+0x3000},std::pair{0x20,1u},std::pair{0x24,1u},std::pair{0x28,objects+0x900}})s->memory.write<4>(manager+off,value);
    s->memory.write<2>(objects+0x900,2);s->memory.write<4>(objects+0x904,objects+0x1000);
    for(unsigned n=0;n<16;++n)s->memory.write<1>(objects+0x1000+n,n*13+5);
    injury_persistence_fixture_tables(*s,toc,tables);
    s->wr<4>(12,manager);s->wr<4>(16,objects+0x1800);return s;
}
unsigned cases=0,completed=0,interruptions=0,negatives=0,structural=0;std::set<std::uint32_t> sites;
struct Result {std::unique_ptr<State> state;std::vector<Effects::Access> accesses;std::vector<Event> calls;std::vector<std::uint8_t> output;};
Result compare(const State& input,Options options={}) {
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    Trace old_trace(options),new_trace(options);Model old(true,options,old_trace),modern(false,options,new_trace);
    const auto run=[&](State& s,Model& m,bool reference){try{if(reference){original_trace=&old_trace;m.reference(s);}else modern_root(s,&m,&new_trace);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="Injury persistence entry="+hex_address(input.pc)+" case="+std::to_string(cases);
    if((expected!=actual || old_trace.sites!=new_trace.sites) && !options.child_negative){for(unsigned n=0;n<std::min(old_trace.sites.size(),new_trace.sites.size());++n)if(old_trace.sites[n]!=new_trace.sites[n]){std::cerr<<"First persistence site "<<n<<" "<<hex_address(old_trace.sites[n])<<"/"<<hex_address(new_trace.sites[n])<<std::endl;break;}}
    require(expected==actual,label+" original="+expected+" modern="+actual);if(options.own_negative)b->wr<4>(52,b->r<4>(52)^1);
    bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative && !options.child_negative)throw;rejected=true;}
    require(rejected==(options.own_negative||options.child_negative),label+" discriminating negative");
    if(!rejected){require(x.bytes==y.bytes,label+" ordered effects");require(a->executed==b->executed,label+" accounting");require(old.calls==modern.calls,label+" dependency ABI");require(old_trace.sites==new_trace.sites,label+" source order");}
    if(options.force_pc && expected.empty())require(std::find(old_trace.sites.begin(),old_trace.sites.end(),options.force_pc)!=old_trace.sites.end(),label+" structural control reached");
    ++cases;completed+=expected.empty()&&!rejected;interruptions+=!expected.empty();negatives+=rejected;structural+=options.force_pc!=0;sites.insert(old_trace.sites.begin(),old_trace.sites.end());require(old.output==modern.output && old.position==modern.position,label+" ordered archive stream");return{std::move(a),x.log,old.calls,old.output};
}
void boundaries(const State& input,Options options={},bool full=true) {
    const auto baseline=compare(input,options);const auto steps=baseline.state->executed-input.executed;
    if(full)for(std::uint64_t n=0;n<=steps;++n){auto s=clone(input);s->budget=s->executed+n;compare(*s,options);}
    else for(auto n:{std::uint64_t(0),std::uint64_t(1),steps/2,steps?steps-1:0,steps}){auto s=clone(input);s->budget=s->executed+n;compare(*s,options);}
    std::set<std::uint32_t> bytes;for(const auto& a:baseline.accesses)for(unsigned n=0;n<a.size;++n)bytes.insert(a.address+n);
    for(auto hole:bytes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;for(auto b:bytes)if(b!=hole){s->memory.map(b,1);s->memory.write<1>(b,input.memory.read<1>(b));}compare(*s,options);}
    for(unsigned n=1;n<=baseline.calls.size();++n){auto o=options;o.interrupt=n;compare(input,o);}
}
void load_checks() {
    boundaries(*seed(0));Options legacy;legacy.version=0x3e023;boundaries(*seed(0),legacy);
    auto null=seed(0);null->wr<4>(16,0);boundaries(*null);
    Options failed;failed.open_failure=true;boundaries(*seed(0),failed);
    for(unsigned n=1;n<=5;++n){Options o;o.allocation_failure=n;boundaries(*seed(0),o);}
    for(bool old:{false,true})for(unsigned n=1;n<=(old?11u:10u);++n){Options o;o.version=old?0x3e023:0x3e024;o.io_failure=n;boundaries(*seed(0),o);}
    for(auto version:{0u,0x3e023u,0x3e024u,0x3e025u,0x7fffffffu,0x80000000u,0xffffffffu}){Options o;o.version=version;compare(*seed(0),o);}
    for(auto count:{0u,1u,2u,0x7fffu,0x8000u,0xffffu}){Options o;o.list_count=count;if(count<3 || count>=0x8000)boundaries(*seed(0),o,false);}
    for(auto mask:{0u,1u,2u,3u}){Options o;o.first_count=mask&1;o.second_count=(mask>>1)&1;boundaries(*seed(0),o,false);}
    for(auto count:{0u,1u,2u,0xffffffffu}){Options o;o.record_count=count;boundaries(*seed(0),o,false);}
    auto no_old_list=seed(0);no_old_list->memory.write<4>(manager+0x28,0);boundaries(*no_old_list);
    Options negative_list;negative_list.list_count=0xffff;negative_list.allocation_failure=5;boundaries(*seed(0),negative_list);
    Options mutation;mutation.mutate_count=true;boundaries(*seed(0),mutation);
    const auto complete=compare(*seed(0));require(complete.state->r<4>(12)==1 && complete.state->memory.read<4>(manager+0x18)==1 && complete.state->memory.read<4>(manager+0x24)==1,"Independent loaded success and capacity mirrors");
}
void save_checks() {
    boundaries(*seed(1));auto null=seed(1);null->wr<4>(16,0);boundaries(*null);
    Options failed;failed.open_failure=true;boundaries(*seed(1),failed);
    for(unsigned n=1;n<=10;++n){Options o;o.io_failure=n;auto result=compare(*seed(1),o);boundaries(*seed(1),o);if(n==9 || n==10)require(std::none_of(result.calls.begin(),result.calls.end(),[](const Event& e){return e.target==0x10747900;}),"Original late list-write failure leaves file open");}
    for(auto mask:{0u,1u,2u,3u}){auto s=seed(1);s->memory.write<4>(manager+0x18,mask&1);s->memory.write<4>(manager+0x24,(mask>>1)&1);boundaries(*s,{},false);}
    for(auto count:{0u,1u,0x7fffu,0x8000u,0xffffu}){auto s=seed(1);s->memory.write<2>(objects+0x900,count);boundaries(*s,{},false);}
    auto no_list=seed(1);no_list->memory.write<4>(manager+0x28,0);boundaries(*no_list);
    Options mutation;mutation.mutate_count=true;boundaries(*seed(1),mutation);
    auto s=seed(1);auto result=compare(*s);std::vector<std::uint8_t> expected;
    const auto append=[&](std::uint32_t addr,unsigned bytes){for(unsigned n=0;n<bytes;++n)expected.push_back(std::uint8_t(s->memory.read<1>(addr+n)));};
    append(rows,72);append(manager+8,4);append(rows+0x1000,112);append(manager+0x18,4);append(rows+0x2000,24);append(manager+0x24,4);append(rows+0x3000,20);append(objects+0x900,2);append(objects+0x1000,16);
    require(result.output==expected,"Independent serialized field order, sizes and list records");
}
void alias_and_relocation_checks() {
    for(unsigned index=0;index<2;++index){
        auto s=seed(index);constexpr auto moved=toc+0x10000;s->memory.map(moved-0x8000,0x9000);s->memory.copy_in(moved-0x8000,s->memory.copy_out(toc-0x8000,0x9000));s->wr<4>(8,moved);boundaries(*s,{},false);
        for(auto pointer:{manager,rows,stack-(index?528u:768u)+0x38}){auto x=seed(index);x->wr<4>(12,pointer);boundaries(*x,{},false);}
        auto bad=seed(index);bad->wr<4>(12,0);boundaries(*bad,{},false);
    }
}
}
namespace cm::aot {void injury_persistence_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try {
    const auto mode=argc==2?std::string_view(argv[1]):std::string_view{};
    if(mode.empty()||mode=="--load")load_checks();if(mode.empty()||mode=="--save")save_checks();if(mode.empty())alias_and_relocation_checks();
    if(mode.empty())for(unsigned index=0;index<2;++index){Options o;o.own_negative=true;compare(*seed(index),o);o.own_negative=false;o.child_negative=true;compare(*seed(index),o);}
    for(auto [start,end]:ranges)for(auto pc=start;pc<end;pc+=4)if(pc!=0x1018484c && pc!=0x10184e10 && !sites.contains(pc))std::cerr<<"Uncovered injury persistence "<<hex_address(pc)<<std::endl;
    if(mode.empty())require(sites.size()==945,"Complete injury persistence source coverage");
    std::cout<<"Persistence comparisons="<<cases<<" completions="<<completed<<" interruptions="<<interruptions<<" negatives="<<negatives<<" structural="<<structural<<" sites="<<sites.size()<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
