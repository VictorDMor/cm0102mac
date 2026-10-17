#include "injury_bootstrap.hpp"
#include "semantic_io_checks.hpp"
#include "application_exit.hpp"
#include <set>

namespace cm::aot {bool injury_bootstrap_reference(State&);void injury_bootstrap_fixture_tables(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace bootstrap=cm::aot::injury_bootstrap;
constexpr std::uint32_t manager=fixture+0x1000,staff=fixture+0x2000,rows=fixture+0x4000,random_words=fixture+0x8000;
constexpr std::uint32_t objects=fixture+0xc000,clubs=fixture+0x10000,strings=fixture+0x20000,tables=fixture+0x30000,profiles=fixture+0x3000;
constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,6> ranges{{{0x101840e0,0x10184308},{0x10184310,0x101843c4},{0x101843d0,0x10184528},{0x10187a20,0x10187a38},{0x10188490,0x101885f0},{0x101885f0,0x10188924}}};
struct Options {
    unsigned interrupt=0,probe_failures=0;std::uint32_t allocation=1,load_result=1,date_status=0,date_delta=20,schedule_day=194,schedule_year=2001,random_seed=0;
    bool own_negative=false,child_negative=false,fatal_exit=false,mutate_count=false;
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
    using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
    constexpr Function methods[]{bootstrap::construct_controller,bootstrap::destroy_controller,bootstrap::initialize_club_thresholds,bootstrap::set_staff_injury_flag,bootstrap::staff_availability_gate,bootstrap::populate_staff_records};
    const auto entry=std::find(bootstrap::entries.begin(),bootstrap::entries.end(),s.pc);require(entry!=bootstrap::entries.end(),"Known injury bootstrap root");methods[entry-bootstrap::entries.begin()](s,d,o);
}
struct Model final:SemanticDependencyDispatch {
    bool original;Options options;Trace& trace;std::vector<Event> calls;
    Model(bool old,Options value,Trace& observer):original(old),options(value),trace(observer){}
    void reference(State& s,std::uint32_t stop=return_sentinel) {
        const auto entry=s.pc;const auto range=std::find_if(ranges.begin(),ranges.end(),[&](auto r){return r.first==entry;});require(range!=ranges.end(),"Known original injury bootstrap root");
        while(s.pc!=stop){if(s.pc>=entry && s.pc<range->second)require(injury_bootstrap_reference(s),"Complete injury bootstrap reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));}
    }
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override {
        constexpr std::array<std::uint32_t,19> allowed{0x1077fee0,0x10727890,0x10769b60,0x10773520,0x10746280,0x10748bb0,0x10739890,0x10768810,0x1077b720,0x1076f7d0,0x10184530,0x101885f0,0x10775170,0x10748c80,0x10738c20,0x107735a0,0x10715880,0x101929b0,0x10718130};
        require(std::find(allowed.begin(),allowed.end(),target)!=allowed.end() || target==0x102ae9c0 || target==0x102af650 || target==0x10775130 || target==0x1077b770 || target==0x10188930 || target==0x10188cc0,"Known bootstrap dependency "+hex_address(target));
        Event event{target,continuation,s.registers,{}};const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};
        s.memory.write<4>(fixture+0x50000+unsigned(calls.size())*4,target^arg(3));std::uint32_t result=0;
        if(target==0x1077fee0) {
            const auto attempts=std::count_if(calls.begin(),calls.end(),[](const Event& e){return e.target==0x1077fee0;});
            result=unsigned(attempts)<options.probe_failures?0xffffffff:0x10000;
            s.memory.write<4>(arg(1)+0x14,arg(2));s.memory.write<4>(arg(6),0x12345678);
        }
        if(target==0x10773520)result=options.allocation?objects+0x800:0;
        if(target==0x10775130)result=options.allocation?rows:0;
        if(target==0x10746280)result=objects+0x900;
        if(target==0x10748bb0)s.memory.write<4>(arg(3),arg(4));
        if(target==0x10739890){s.memory.write<4>(arg(6),0x12345678);s.memory.write<4>(arg(7),0x87654321);}
        if(target==0x10727890 || target==0x1077b720){s.memory.write<4>(arg(3),arg(4));s.memory.write<4>(arg(3)+4,arg(5));}
        if(target==0x10184530){result=options.load_result;s.memory.write<4>(arg(3),rows+0x1000);}
        if(target==bootstrap::population_entry)s.memory.write<4>(arg(3),rows);
        if(target==0x10715880){s.memory.write<4>(arg(3),0);s.memory.write<4>(arg(3)+4,0);}
        if(target==0x101929b0){result=options.date_status;for(auto n:{4u,5u}){s.memory.write<2>(arg(n),options.schedule_day);s.memory.write<2>(arg(n)+2,options.schedule_year);s.memory.write<4>(arg(n)+4,0);}}
        if(target==0x10718130)result=options.date_delta;
        if(target==0x102ae9c0)result=3;
        if(target==0x102af650)result=objects+0x600;
        if(target==0x1077b770){result=options.random_seed;if(options.mutate_count)s.memory.write<4>(objects+0x200,1);}
        if(target==0x10188930)result=39;
        for(unsigned n=0;n<=12;++n)if(n!=1 && n!=2)s.wr<4>(n*4,0xac000000+n*17);
        for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-20.0-n));
        s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=continuation;
        if(options.child_negative && !original)s.wr<4>(48,s.r<4>(48)^1);
        event.after=s.registers;calls.push_back(event);
        if(target==0x1076f7d0 && options.fatal_exit)throw ApplicationExit{};
        if(options.interrupt && calls.size()>=options.interrupt)throw std::runtime_error("Fixture injury bootstrap dependency interruption");
    }
};
std::unique_ptr<State> seed(unsigned index) {
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);s->memory.map(fixture,0x60000);s->memory.map(stack-0x4000,0x5000);s->memory.map(toc-0x8000,0x9000);
    s->pc=bootstrap::entries[index];s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);
    for(auto [slot,pointer]:{std::pair{-0x5438,strings},std::pair{-0x7ba8,objects},std::pair{-0x7bb8,objects+0x1000},std::pair{-0x7bac,objects+0x100},std::pair{-0x7be4,objects+0x200},std::pair{-0x7be0,objects+0x218},std::pair{-0x7be8,objects+0x220},std::pair{-0x7b04,objects+0x224},std::pair{-0x7b50,objects+0x228},std::pair{-0x7a64,objects+0x22c},std::pair{-0x7b80,objects+0x500},std::pair{-0x7b84,objects+0x230},std::pair{-0x7b88,objects+0x234},std::pair{-0x7b8c,random_words},std::pair{-0x7b90,objects+0x238}})s->memory.write<4>(toc+slot,pointer);
    for(auto [pointer,value]:{std::pair{objects+0x200,2u},std::pair{objects+0x218,staff},std::pair{objects+0x220,clubs},std::pair{objects+0x228,1u},std::pair{objects+0x22c,objects+0x400},std::pair{objects+0x230,random_words},std::pair{objects+0x234,random_words+0x400}})s->memory.write<4>(pointer,value);
    s->memory.write<4>(manager,rows);s->memory.write<4>(manager+4,rows+0x1000);s->memory.write<4>(manager+12,objects+0x800);s->memory.write<4>(manager+16,rows+0x1800);s->memory.write<4>(manager+28,rows+0x2000);s->memory.write<4>(manager+40,objects+0xa00);
    for(unsigned n=0;n<2;++n){s->memory.write<4>(staff+n*0x70,n);s->memory.write<4>(staff+n*0x70+0x14,profiles+n*0x100);s->memory.write<4>(clubs+0x98+n*4,staff+n*0x70);s->memory.write<1>(profiles+n*0x100+0x34,20);}
    s->memory.write<4>(staff+0x20,clubs);s->memory.write<4>(staff+0x24,clubs+0x1000);s->memory.write<4>(clubs+0x1014,0xffffffff);
    s->memory.write<2>(objects+0x500,194);s->memory.write<2>(objects+0x502,2001);s->memory.write<2>(objects+0x600,194);s->memory.write<2>(objects+0x602,2001);
    injury_bootstrap_fixture_tables(*s,toc,tables);
    s->wr<4>(12,manager);s->wr<4>(16,index==0?0u:index==1?1u:staff);s->wr<4>(20,0x12340180);return s;
}
unsigned cases=0,completed=0,interruptions=0,negatives=0,structural=0;std::set<std::uint32_t> sites;
struct Result {std::unique_ptr<State> state;std::vector<Effects::Access> accesses;std::vector<Event> calls;};
Result compare(const State& input,Options options={}) {
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    Trace old_trace(options),new_trace(options);Model old(true,options,old_trace),modern(false,options,new_trace);
    const auto run=[&](State& s,Model& m,bool reference){try{if(reference){original_trace=&old_trace;m.reference(s);}else modern_root(s,&m,&new_trace);original_trace=nullptr;return std::string{};}catch(const ApplicationExit&){original_trace=nullptr;return std::string("Fixture application exit");}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="Injury reporting entry="+hex_address(input.pc)+" case="+std::to_string(cases);
    if((expected!=actual || old_trace.sites!=new_trace.sites) && !options.child_negative){for(unsigned n=0;n<std::min(old_trace.sites.size(),new_trace.sites.size());++n)if(old_trace.sites[n]!=new_trace.sites[n]){std::cerr<<"First readout site "<<n<<" "<<hex_address(old_trace.sites[n])<<"/"<<hex_address(new_trace.sites[n])<<std::endl;break;}}
    require(expected==actual,label+" original="+expected+" modern="+actual);if(options.own_negative)b->wr<4>(52,b->r<4>(52)^1);
    bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative && !options.child_negative)throw;rejected=true;}
    require(rejected==(options.own_negative||options.child_negative),label+" discriminating negative");
    if(!rejected){require(x.bytes==y.bytes,label+" ordered effects");require(a->executed==b->executed,label+" accounting");require(old.calls==modern.calls,label+" dependency ABI");require(old_trace.sites==new_trace.sites,label+" source order");}
    if(options.force_pc && expected.empty())require(std::find(old_trace.sites.begin(),old_trace.sites.end(),options.force_pc)!=old_trace.sites.end(),label+" structural control reached");
    ++cases;completed+=expected.empty()&&!rejected;interruptions+=!expected.empty();negatives+=rejected;structural+=options.force_pc!=0;sites.insert(old_trace.sites.begin(),old_trace.sites.end());return{std::move(a),x.log,old.calls};
}
void boundaries(const State& input,Options options={},bool full=true) {
    const auto baseline=compare(input,options);const auto steps=baseline.state->executed-input.executed;
    if(full)for(std::uint64_t n=0;n<=steps;++n){auto s=clone(input);s->budget=s->executed+n;compare(*s,options);}
    else for(auto n:{std::uint64_t(0),std::uint64_t(1),steps/2,steps?steps-1:0,steps}){auto s=clone(input);s->budget=s->executed+n;compare(*s,options);}
    std::set<std::uint32_t> bytes;for(const auto& a:baseline.accesses)for(unsigned n=0;n<a.size;++n)bytes.insert(a.address+n);
    for(auto hole:bytes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;for(auto b:bytes)if(b!=hole){s->memory.map(b,1);s->memory.write<1>(b,input.memory.read<1>(b));}compare(*s,options);}
    for(unsigned n=1;n<=baseline.calls.size();++n){auto o=options;o.interrupt=n;compare(input,o);}
}
void constructor_checks() {
    boundaries(*seed(0));
    for(auto load:{0u,1u,128u,255u,256u}){auto s=seed(0);s->wr<4>(16,objects+0x2000);Options o;o.load_result=load;boundaries(*s,o,false);}
    Options retry;retry.probe_failures=1;boundaries(*seed(0),retry);
    Options failed;failed.allocation=0;boundaries(*seed(0),failed);failed.fatal_exit=true;boundaries(*seed(0),failed);
    for(auto slot:{manager,stack-0x380+0xcc,tables,objects+0x800}){auto s=seed(0);s->wr<4>(12,slot);boundaries(*s,{},false);}
}
void destructor_checks() {
    boundaries(*seed(1));
    for(unsigned mask=0;mask<64;++mask)for(auto flag:{0u,1u,0x7fffu,0x8000u,0xffffu,0x10000u}) {
        auto s=seed(1);s->wr<4>(16,flag);unsigned bit=0;for(auto off:{0,4,12,16,40,28}){if(!(mask&(1u<<bit)))s->memory.write<4>(manager+off,0);++bit;}compare(*s);
    }
    auto s=seed(1);s->wr<4>(12,0);boundaries(*s);
}
void threshold_checks() {
    boundaries(*seed(2));
    for(auto status:{0u,1u,128u,256u})for(auto delta:{0u,1u,39u,40u,41u,79u,80u,81u,0xffffffffu})for(auto fitness:{0u,1u,79u,80u,127u,128u,255u}) {
        auto s=seed(2);s->memory.write<1>(profiles+0x34,fitness);Options o;o.date_status=status;o.date_delta=delta;auto r=compare(*s,o);
        auto window=signed_value<1>(status)?0:std::bit_cast<std::int32_t>(delta);if(window>80)window=40;if(window>40)window=std::int8_t(80-window);
        const auto score=std::clamp<std::int64_t>(window-signed_value<1>(fitness)/2,1,40);
        require(r.state->memory.read<2>(rows+8)==unsigned(10000-score*100),"Independent club injury threshold");
    }
    for(auto count:{0u,1u,2u,0x7fffffffu,0x80000000u,0xffffffffu}){auto s=seed(2);s->memory.write<4>(objects+0x200,count);boundaries(*s,{},false);}
    for(auto clubcount:{0u,0x80000000u,0xffffffffu}){auto s=seed(2);s->memory.write<4>(objects+0x228,clubcount);boundaries(*s,{},false);}
}
void flag_checks() {
    boundaries(*seed(3));
    for(auto value:{0u,1u,127u,128u,255u,256u,0xffffffffu})for(auto id:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu}) {
        auto s=seed(3);s->wr<4>(20,value);s->memory.write<4>(staff,id);auto r=compare(*s);if(id<=1)require(r.state->memory.read<1>(rows+id*0x24+0x22)==(value&255),"Independent flag byte narrowing");
    }
    for(auto address:{manager,rows,staff}){auto s=seed(3);s->wr<4>(16,address);boundaries(*s,{},false);}
}
void availability_checks() {
    boundaries(*seed(4));
    for(auto year:{2000u,2001u,2002u})for(auto day:{0u,193u,194u,195u,32767u,32768u,65535u})for(auto status:{0u,1u,128u,256u})for(auto distance:{0u,39u,40u,65536u,0xffffffffu}) {
        auto s=seed(4);s->memory.write<2>(objects+0x600,day);s->memory.write<2>(objects+0x602,year);Options o;o.date_status=status;o.date_delta=distance;auto r=compare(*s,o);
        const bool future=year>2001||(year==2001 && signed_value<2>(day)>194);
        require(r.state->r<4>(12)==unsigned(!future&&!signed_value<1>(status)&&(distance&65535)<40),"Independent availability gate");
    }
    for(auto off:{0x20,0x24}){auto s=seed(4);s->memory.write<4>(staff+off,0);boundaries(*s);}
    for(auto contract:{0u,1u,0x7fffffffu,0xffffffffu}){auto s=seed(4);s->memory.write<4>(clubs+0x1014,contract);boundaries(*s,{},false);}
    auto s=seed(4);s->wr<4>(16,0);boundaries(*s);
}
void population_checks() {
    boundaries(*seed(5));
    for(auto count:{0u,1u,0xffffffffu}){auto s=seed(5);s->memory.write<4>(objects+0x200,count);boundaries(*s,{},false);}
    Options fail;fail.allocation=0;boundaries(*seed(5),fail);
    auto s=seed(5);s->memory.write<4>(objects+0x234,random_words);for(unsigned n=0;n<82;++n)s->memory.write<1>(tables+0x2000+n*12+9,1);boundaries(*s);
    auto r=compare(*s);require(r.state->memory.read<1>(rows+0x14)==0 && r.state->memory.read<4>(rows+0x10)==0xffffffff && r.state->memory.read<1>(rows+0x22)==50,"Independent initial row defaults and risk type");
    for(auto value:{0u,1u,9u,10u,11u,12u,81u,82u,99u,100u,65535u}) {
        auto x=seed(5);for(unsigned n=0;n<256;++n)x->memory.write<2>(random_words+n*2,value);for(unsigned n=0;n<82;++n)x->memory.write<1>(tables+0x2000+n*12+9,1);boundaries(*x,{},false);
    }
    for(auto value:{0u,1u,65535u,65536u,0xffffffffu}){Options o;o.random_seed=value;auto x=seed(5);x->memory.write<4>(objects+0x234,random_words);boundaries(*x,o,false);}
    Options mutation;mutation.mutate_count=true;boundaries(*s,mutation,false);
    for(auto address:{rows,staff,stack-0xb0+0x38}){auto x=seed(5);x->wr<4>(12,address);boundaries(*x,{},false);}
}
void structural_checks() {
    for(unsigned index=0;index<6;++index){auto s=seed(index);constexpr auto moved=toc+0x10000;s->memory.map(moved-0x8000,0x9000);s->memory.copy_in(moved-0x8000,s->memory.copy_out(toc-0x8000,0x9000));s->wr<4>(8,moved);boundaries(*s,{},false);}
}
}
namespace cm::aot {void injury_bootstrap_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try {
    const auto mode=argc==2?std::string_view(argv[1]):std::string_view{};
    if(mode.empty()||mode=="--constructor")constructor_checks();if(mode.empty()||mode=="--destructor")destructor_checks();if(mode.empty()||mode=="--thresholds")threshold_checks();if(mode.empty()||mode=="--flag")flag_checks();if(mode.empty()||mode=="--availability")availability_checks();if(mode.empty()||mode=="--population")population_checks();if(mode.empty())structural_checks();
    if(mode.empty())for(unsigned index=0;index<6;++index){Options o;o.own_negative=true;compare(*seed(index),o);o.own_negative=false;o.child_negative=true;if(index!=3)compare(*seed(index),o);}
    for(auto [start,end]:ranges)for(auto pc=start;pc<end;pc+=4)if(pc!=0x10184164 && pc!=0x1018446c && !sites.contains(pc))std::cerr<<"Uncovered injury bootstrap "<<hex_address(pc)<<std::endl;
    if(mode.empty())require(sites.size()==566,"Complete injury bootstrap source coverage");
    std::cout<<"Bootstrap comparisons="<<cases<<" completions="<<completed<<" interruptions="<<interruptions<<" negatives="<<negatives<<" structural="<<structural<<" sites="<<sites.size()<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
