#include "injury_fitness.hpp"
#include "injury_random.hpp"
#include "injury_maintenance.hpp"
#include "semantic_io_checks.hpp"
#include "application_exit.hpp"
#include <set>

namespace cm::aot {bool injury_fitness_reference(State&);bool injury_random_reference(State&);bool injury_maintenance_reference(State&);void injury_fitness_fixture_tables(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace fitness=cm::aot::injury_fitness;
namespace random_draw=cm::aot::injury_random;
namespace maintenance=cm::aot::injury_maintenance;
constexpr std::array entries{fitness::entries[0],fitness::entries[1],fitness::entries[2],fitness::entries[3],fitness::entries[4],fitness::entries[5],random_draw::entry,maintenance::apply_fitness_entry,maintenance::merge_pending_entry,maintenance::find_pending_entry,maintenance::reset_staff_entry,maintenance::apply_match_entry};
constexpr std::uint32_t manager=fixture+0x1000,staff=fixture+0x2000,rows=fixture+0x4000,random_words=fixture+0x8000;
constexpr std::uint32_t objects=fixture+0xc000,tables=fixture+0x30000,profile=fixture+0x3000;
constexpr std::uint32_t pending=fixture+0x10000,pending_rows=fixture+0x11000,values=fixture+0x12000,match=fixture+0x17000,roster=fixture+0x18000;
constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,12> ranges{{{0x10187450,0x10187644},{0x10187650,0x10187680},{0x10187680,0x101877dc},{0x101877e0,0x101878d0},{0x101878d0,0x10187a1c},{0x10187a40,0x10188488},{0x101864a0,0x10186a18},{0x10186a20,0x10186cd0},{0x10186cd0,0x10186e0c},{0x10186e10,0x10186e94},{0x10186ea0,0x10186fb4},{0x10186fc0,0x10187450}}};
struct Control {std::uint32_t pc;unsigned reg;std::uint32_t value;int cr=-1;};
struct Options {
    unsigned interrupt=0;std::uint32_t random_seed=0,phase_result=1,row_days=3,injury_days=5,start_distance=2,end_distance=20,schedule_status=0,schedule_day=193,schedule_year=2001,nested_random_result=0;
    bool own_negative=false,child_negative=false,real_random=false,real_maintenance=false;
    std::uint32_t allocation_result=pending,injury_result=0xffffffff;
    std::uint32_t force_pc=0,force_value=0;unsigned force_register=0;int force_cr=-1;
    std::vector<Control> controls;
};
struct Trace final:SemanticGuestObserver {
    Options options;std::vector<std::uint32_t> sites;
    explicit Trace(Options value):options(value){}
    void before_step(State& s,std::uint32_t pc)override {
        sites.push_back(pc);const auto apply=[&](unsigned reg,std::uint32_t value,int cr){s.wr<4>(reg*4,value);if(cr>=0)s.wr<1>(2304,(s.r<1>(2304)&0xf0)|unsigned(cr)|(s.r<1>(1024)&1));};
        if(pc==options.force_pc)apply(options.force_register,options.force_value,options.force_cr);for(const auto& c:options.controls)if(pc==c.pc)apply(c.reg,c.value,c.cr);
    }
};
thread_local Trace* original_trace=nullptr;
struct Event {std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
void modern_root(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
    constexpr Function methods[]{fitness::update_staff_condition,fitness::read_staff_condition,fitness::classify_injury_recovery,fitness::read_staff_fitness,fitness::read_effective_fitness,fitness::refresh_staff_fitness,random_draw::draw_signed_bounded,maintenance::apply_staff_fitness,maintenance::merge_pending_injury,maintenance::find_pending_injury,maintenance::reset_staff_physical_state,maintenance::apply_match_physical_state};
    const auto entry=std::find(entries.begin(),entries.end(),s.pc);require(entry!=entries.end(),"Known injury fitness root");methods[entry-entries.begin()](s,d,o);
}
struct Model final:SemanticDependencyDispatch {
    bool original;Options options;Trace& trace;std::vector<Event> calls;
    Model(bool old,Options value,Trace& observer):original(old),options(value),trace(observer){}
    void reference(State& s,std::uint32_t stop=return_sentinel) {
        const auto entry=s.pc;const auto range=std::find_if(ranges.begin(),ranges.end(),[&](auto r){return r.first==entry;});require(range!=ranges.end(),"Known original injury fitness root");
        bool first=true;while(s.pc!=stop){if(s.pc>=entry && s.pc<range->second && (first||s.pc!=entry))require(entry==random_draw::entry?injury_random_reference(s):entry>=maintenance::apply_fitness_entry && entry<0x10187450?injury_maintenance_reference(s):injury_fitness_reference(s),"Complete injury fitness reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));first=false;}
    }
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override {
        constexpr std::array allowed{0x1077b770u,0x10739890u,0x10768810u,0x1077b720u,0x10769b60u,fitness::refresh_entry,fitness::recovery_phase_entry,0x10717dd0u,0x10715880u,0x1018b9f0u,0x102ae9c0u,0x102af650u,0x101929b0u,0x10718130u,0x101864a0u,0x10773520u,0x10738c00u,0x10738e00u,0x10738d10u,0x10738e60u,0x10188930u,maintenance::apply_fitness_entry,0x1018fdf0u,0x10188cc0u};
        require(std::find(allowed.begin(),allowed.end(),target)!=allowed.end(),"Known fitness dependency "+hex_address(target));
        Event event{target,continuation,s.registers,{}};
        if(target==random_draw::entry && options.real_random){if(original)reference(s,continuation);else random_draw::draw_signed_bounded(s,this,&trace);event.after=s.registers;calls.push_back(event);return;}
        if(target==maintenance::apply_fitness_entry && options.real_maintenance){if(original)reference(s,continuation);else maintenance::apply_staff_fitness(s,this,&trace);event.after=s.registers;calls.push_back(event);return;}
        const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};
        s.memory.write<4>(fixture+0x50000+unsigned(calls.size())*4,target^arg(3));std::uint32_t result=0;
        if(target==0x1077b770)result=options.random_seed;
        if(target==fitness::recovery_phase_entry)result=options.phase_result;
        if(target==0x10739890){s.memory.write<4>(arg(6),0x12345678);s.memory.write<4>(arg(7),0x87654321);}
        if(target==0x1077b720){s.memory.write<4>(arg(3),arg(4));s.memory.write<4>(arg(3)+4,arg(5));}
        if(target==0x10715880){s.memory.write<4>(arg(3),0);s.memory.write<4>(arg(3)+4,0);}
        if(target==0x10717dd0){s.memory.write<2>(arg(3),continuation==0x10187bb8?options.row_days:options.injury_days);s.memory.write<2>(arg(3)+2,0);s.memory.write<4>(arg(3)+4,0);}
        if(target==0x1018b9f0)s.memory.write<4>(rows+0x10,0xffffffff);
        if(target==0x102ae9c0)result=3;
        if(target==0x102af650){result=objects+0x700;s.memory.write<2>(result,options.schedule_day);s.memory.write<2>(result+2,options.schedule_year);}
        if(target==0x101929b0){result=options.schedule_status;for(auto n:{4u,5u}){s.memory.write<2>(arg(n),193);s.memory.write<2>(arg(n)+2,2001);s.memory.write<4>(arg(n)+4,0);}}
        if(target==0x10718130)result=continuation==0x10187e08?options.start_distance:options.end_distance;
        if(target==0x101864a0)result=options.nested_random_result;
        if(target==0x10773520)result=options.allocation_result;
        if(target==0x10738c00){s.memory.write<4>(arg(3),0);s.memory.write<4>(arg(3)+4,0);s.memory.write<4>(arg(3)+12,0);}
        if(target==0x10738e00){const std::uint32_t object=arg(3),cursor=std::uint32_t(s.memory.read<4>(object+12)),count=std::uint32_t(s.memory.read<4>(object));if(cursor<count){result=pending_rows+cursor*8;s.memory.write<4>(object+12,cursor+1);}}
        if(target==0x10738d10){const std::uint32_t object=arg(3),count=std::uint32_t(s.memory.read<4>(object));s.memory.copy_in(pending_rows+count*8,s.memory.copy_out(arg(4),arg(5)));s.memory.write<4>(object,count+1);}
        if(target==0x10738e60){const std::uint32_t object=arg(3),cursor=std::uint32_t(s.memory.read<4>(object+12)),count=std::uint32_t(s.memory.read<4>(object));require(cursor>0 && cursor<=count,"Valid pending fixture removal");for(auto n=cursor;n<count;++n)s.memory.copy_in(pending_rows+(n-1)*8,s.memory.copy_out(pending_rows+n*8,8));s.memory.write<4>(object,count-1);s.memory.write<4>(object+12,cursor-1);}
        if(target==0x10188930)result=options.injury_result;
        for(unsigned n=0;n<=12;++n)if(n!=1 && n!=2)s.wr<4>(n*4,0xac000000+n*17);
        for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-20.0-n));
        s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=continuation;
        if(options.child_negative && !original){s.wr<4>(48,s.r<4>(48)^1);const auto marker=fixture+0x50000+unsigned(calls.size())*4;s.memory.write<4>(marker,s.memory.read<4>(marker)^1);}
        event.after=s.registers;calls.push_back(event);
        if(options.interrupt && calls.size()>=options.interrupt)throw std::runtime_error("Fixture injury fitness dependency interruption");
    }
};
std::unique_ptr<State> seed(unsigned index) {
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned n=1280;n<1312;++n)s->wr<1>(n,0);s->memory.map(fixture,0x60000);s->memory.map(stack-0x4000,0x5000);s->memory.map(toc-0x8000,0x9000);
    s->pc=entries[index];s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->budget=100000;
    for(auto [slot,pointer]:{std::pair{-0x7ba8,objects},std::pair{-0x7bac,objects+0x100},std::pair{-0x7be4,objects+0x200},std::pair{-0x7be0,objects+0x218},std::pair{-0x7b90,objects+0x238},std::pair{-0x7b8c,random_words},std::pair{-0x7b88,objects+0x234},std::pair{-0x7b84,objects+0x230},std::pair{-0x7b80,objects+0x500},std::pair{-0x5448,objects+0x600},std::pair{-0x544c,objects+0x610},std::pair{-0x5450,objects+0x620},std::pair{-0x7b9c,objects+0x240},std::pair{-0x7a64,objects+0x22c}})s->memory.write<4>(toc+slot,pointer);
    for(auto [pointer,value]:{std::pair{objects+0x200,2u},std::pair{objects+0x218,staff},std::pair{objects+0x230,random_words},std::pair{objects+0x234,random_words+0x400},std::pair{objects+0x240,objects+0x1000},std::pair{objects+0x22c,objects+0x400}})s->memory.write<4>(pointer,value);
    s->memory.write<4>(manager,rows);s->memory.write<4>(manager+4,rows+0x1000);s->memory.write<4>(manager+8,1);s->memory.write<4>(staff,0);s->memory.write<4>(staff+0x14,profile);s->memory.write<4>(staff+0x20,objects+0x2000);s->memory.write<4>(staff+0x24,objects+0x3000);s->memory.write<1>(staff+0x67,9);s->memory.write<2>(staff+0x5c,25);s->memory.write<1>(profile+0x34,20);
    s->memory.write<2>(objects+0x500,194);s->memory.write<2>(objects+0x502,2001);s->memory.write<1>(objects+0x504,2);s->memory.write<2>(rows,192);s->memory.write<2>(rows+2,2001);s->memory.write<2>(rows+8,8000);s->memory.write<2>(rows+10,7000);s->memory.write<2>(rows+12,500);s->memory.write<4>(rows+0x10,0);s->memory.write<1>(rows+0x22,50);s->memory.write<2>(rows+0x1000+0x30,8);s->memory.write<2>(rows+0x1000+0x32,2);
    injury_fitness_fixture_tables(*s,toc,tables);
    s->wr<4>(12,index==6?100u:manager);s->wr<4>(16,index<2?0u:staff);s->wr<4>(20,0);
    if(index>=7) {
        s->memory.write<4>(toc-0x7a9c,objects+0x900);s->memory.write<2>(objects+0x900,193);s->memory.write<2>(objects+0x902,2001);
        s->memory.write<4>(manager+0x28,pending);s->memory.write<4>(pending,2);s->memory.write<4>(pending+4,0);s->memory.write<4>(pending+12,0);
        for(unsigned n=0;n<2;++n){s->memory.write<4>(pending_rows+n*8,n);s->memory.write<1>(pending_rows+n*8+4,n+1);s->memory.write<1>(pending_rows+n*8+5,50+n*10);}
        s->memory.write<4>(values,0);s->memory.write<1>(values+4,1);s->memory.write<1>(values+5,40);s->memory.write<1>(profile+0x44,30);s->memory.write<1>(profile+0x3c,30);s->memory.write<1>(objects+0x1000+13,255);
        if(index==7){s->wr<4>(20,9000);s->wr<4>(24,20);}
        if(index==8){s->wr<4>(16,values);s->wr<4>(20,values+4);s->wr<4>(24,values+5);}
        if(index==9)s->wr<4>(16,values);
        if(index==10)s->wr<4>(16,0);
        if(index==11){s->wr<4>(16,match);s->wr<4>(20,roster);s->memory.write<4>(match,0xfffffffe);s->memory.write<1>(match+0x40,1);s->memory.write<2>(roster+0x4e,90);for(unsigned side=0;side<2;++side)for(unsigned player=0;player<20;++player){auto slot=roster+side*0x664+player*0x4c;s->memory.write<4>(slot+0x194,0xffffffff);s->memory.write<1>(slot+0x1c8,255);s->memory.write<1>(slot+0x1c4,255);s->memory.write<1>(slot+0x1c6,255);s->memory.write<2>(slot+0x1ca,9000);}s->memory.write<4>(roster+0x194,0);}
    }
    return s;
}
unsigned cases=0,completed=0,interruptions=0,negatives=0,structural=0;std::set<std::uint32_t> sites;
struct Result {std::unique_ptr<State> state;std::vector<Effects::Access> accesses;std::vector<Event> calls;};
Result compare(const State& input,Options options={}) {
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    Trace old_trace(options),new_trace(options);Model old(true,options,old_trace),modern(false,options,new_trace);
    const auto run=[&](State& s,Model& m,bool reference){try{if(reference){original_trace=&old_trace;m.reference(s);}else modern_root(s,&m,&new_trace);original_trace=nullptr;return std::string{};}catch(const ApplicationExit&){original_trace=nullptr;return std::string("Fixture application exit");}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="Injury fitness entry="+hex_address(input.pc)+" case="+std::to_string(cases);
    if((expected!=actual || old_trace.sites!=new_trace.sites) && !options.child_negative){for(unsigned n=0;n<std::min(old_trace.sites.size(),new_trace.sites.size());++n)if(old_trace.sites[n]!=new_trace.sites[n]){std::cerr<<"First fitness site "<<n<<" "<<hex_address(old_trace.sites[n])<<"/"<<hex_address(new_trace.sites[n])<<std::endl;break;}}
    require(expected==actual,label+" original="+expected+" modern="+actual);if(options.own_negative)b->wr<4>(52,b->r<4>(52)^1);
    bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative && !options.child_negative)throw;rejected=true;}
    require(rejected==(options.own_negative||options.child_negative),label+" discriminating negative");
    if(!rejected){require(x.bytes==y.bytes,label+" ordered effects");require(a->executed==b->executed,label+" accounting");require(old.calls==modern.calls,label+" dependency ABI");require(old_trace.sites==new_trace.sites,label+" source order");}
    if(options.force_pc && expected.empty())require(std::find(old_trace.sites.begin(),old_trace.sites.end(),options.force_pc)!=old_trace.sites.end(),label+" structural control reached");
    if(expected.empty())for(const auto& c:options.controls)require(std::find(old_trace.sites.begin(),old_trace.sites.end(),c.pc)!=old_trace.sites.end(),label+" recursive structural control reached");
    ++cases;completed+=expected.empty()&&!rejected;interruptions+=!expected.empty();negatives+=rejected;structural+=options.force_pc!=0||!options.controls.empty();sites.insert(old_trace.sites.begin(),old_trace.sites.end());return{std::move(a),x.log,old.calls};
}
void boundaries(const State& input,Options options={}) {
    const auto base=compare(input,options);const auto steps=base.state->executed-input.executed;
    require(base.state->pc==return_sentinel,"Complete normal injury fitness boundary fixture");
    std::set<std::uint64_t> cuts{0,1,2,3,steps/2,steps?steps-1:0,steps};
    for(auto n:cuts){auto s=clone(input);s->budget=s->executed+n;compare(*s,options);}
    std::set<std::uint32_t> holes;
    for(const auto& access:base.accesses)if(access.size){holes.insert(access.address);holes.insert(access.address+std::uint32_t(access.size)-1);}
    if(holes.size()>12){std::set<std::uint32_t> selected;auto begin=holes.begin();for(unsigned n=0;n<12;++n){auto it=begin;std::advance(it,std::ptrdiff_t(n*(holes.size()-1)/11));selected.insert(*it);}holes=std::move(selected);}
    for(auto hole:holes){
        auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;s->executed=input.executed;s->budget=input.budget;s->native_calls=input.native_calls;s->heap_end=input.heap_end;s->memory_error=input.memory_error;s->allocated=input.allocated;s->available=input.available;
        for(auto [start,size]:input.memory.mapped_ranges()) {
            if(hole<start || std::uint64_t(hole)>=std::uint64_t(start)+size){s->memory.map(start,size);s->memory.copy_in(start,input.memory.copy_out(start,size));continue;}
            for(auto [address,count]:{std::pair{start,std::uint64_t(hole-start)},std::pair{hole+1,std::uint64_t(start)+size-hole-1}})if(count){s->memory.map(address,count);s->memory.copy_in(address,input.memory.copy_out(address,count));}
        }
        compare(*s,options);
    }
    for(unsigned n=1;n<=base.calls.size();++n){auto o=options;o.interrupt=n;compare(input,o);}
}
void core_boundaries() {
    for(unsigned index=0;index<6;++index)boundaries(*seed(index));
    for(unsigned index=2;index<6;++index){auto s=seed(index);s->wr<4>(16,0);boundaries(*s);}
    auto fresh=seed(5);fresh->memory.write<4>(rows+0x10,0xffffffff);boundaries(*fresh);
    Options wide;wide.force_pc=0x10187e98;wide.force_register=21;wide.force_value=65536;boundaries(*fresh,wide);
}
void guard_and_readout_cases() {
    for(unsigned index=0;index<6;++index) {
        for(auto id:{1u,2u,0x7fffffffu,0x80000000u,0xffffffffu}){auto s=seed(index);if(index<2)s->wr<4>(16,id);else s->memory.write<4>(staff,id);compare(*s);}
        auto bad=seed(index);bad->wr<4>(12,0);compare(*bad);
        auto moved=seed(index);constexpr auto relocated=toc+0x10000;moved->memory.map(relocated-0x8000,0x9000);moved->memory.copy_in(relocated-0x8000,moved->memory.copy_out(toc-0x8000,0x9000));moved->wr<4>(8,relocated);compare(*moved);
    }
    for(unsigned index:{3u,4u})for(auto flag:{0u,1u,127u,128u,255u,256u,0xffffffffu}) {
        auto s=seed(index);s->wr<4>(20,flag);auto r=compare(*s);const auto raw=index==3?7000u:5600u;require(r.state->r<4>(12)==(signed_value<1>(flag)?raw/100:raw),"Independent fitness/raw percentage readout index="+std::to_string(index)+" flag="+std::to_string(flag)+" actual="+std::to_string(r.state->r<4>(12)));
        s->memory.write<4>(staff,2);r=compare(*s);require(r.state->r<4>(12)==(signed_value<1>(flag)?100u:10000u),"Original invalid-ID fitness fallback and flag narrowing");
    }
    for(auto phase:{0u,1u,2u,128u,255u,256u}){auto s=seed(4);Options o;o.phase_result=phase;compare(*s,o);}
    auto capped=seed(4);capped->memory.write<2>(rows+8,11000);capped->memory.write<2>(rows+10,10000);capped->wr<4>(20,1);Options uncapped_phase;uncapped_phase.phase_result=0;require(compare(*capped,uncapped_phase).state->r<4>(12)==100,"Independent percentage upper cap");
    for(auto elapsed:{0u,1u,2u,7u,8u,9u,0x7fffu,0x8000u,0xffffu}){Options o;o.injury_days=elapsed;auto r=compare(*seed(2),o);const auto day=signed_value<2>(elapsed);require(r.state->r<4>(12)==unsigned(day<2?2:day<8?1:0),"Independent recovery phase boundaries");}
    auto no_injury=seed(2);no_injury->memory.write<4>(rows+0x10,0xffffffff);compare(*no_injury);
}
void condition_cases() {
    for(auto value:{-32768,-1001,-1000,-501,-500,-1,0,1000,1001,1050,2000,32767}){auto s=seed(0);s->memory.write<2>(rows+12,std::uint16_t(value));compare(*s);}
    for(auto age:{0u,18u,19u,24u,25u,127u,128u,255u})for(auto value:{-501,1000}){auto s=seed(0);s->memory.write<2>(staff+0x5c,age);s->memory.write<2>(rows+12,std::uint16_t(value));compare(*s);}
    for(auto value:{-501,1000,2000}){auto s=seed(0);s->memory.write<1>(profile+15,20);s->memory.write<2>(rows+12,std::uint16_t(value));auto r=compare(*s);require(signed_value<2>(r.state->memory.read<2>(rows+12))<=1050,"Independent staff-category condition cap");}
    auto draw=seed(0);draw->memory.write<2>(staff+0x5c,19);for(unsigned n=0;n<256;++n)draw->memory.write<2>(random_words+n*2,24);boundaries(*draw);
    for(auto seed_value:{0u,1u,65535u,65536u,0xffffffffu}){auto s=seed(0);s->memory.write<4>(objects+0x234,random_words);s->memory.write<2>(rows+12,std::uint16_t(-501));Options o;o.random_seed=seed_value;compare(*s,o);}
    auto check=seed(1);check->memory.write<2>(rows+12,std::uint16_t(-32768));require(compare(*check).state->r<4>(12)==0xffff8000u,"Independent signed condition readout");
}
void refresh_cases() {
    for(auto [days,elapsed]:{std::pair{3,0},std::pair{3,5},std::pair{20,5},std::pair{20,10},std::pair{-2,10},std::pair{-20,10},std::pair{32767,-32768},std::pair{-32768,32767}}){Options o;o.row_days=std::uint16_t(days);o.injury_days=std::uint16_t(elapsed);compare(*seed(5),o);}
    for(auto id:{0u,1u,2u,0xffffffffu,0xfffffffeu}){auto s=seed(5);s->memory.write<4>(rows+0x10,id);compare(*s);}
    auto current=seed(5);current->memory.write<2>(rows,194);compare(*current);
    auto no_profile=seed(5);no_profile->memory.write<4>(staff+0x14,0);compare(*no_profile);
    auto training=[](){auto s=seed(5);s->memory.write<4>(rows+0x10,0xffffffff);return s;};
    for(auto off:{0x20,0x24}){auto s=training();s->memory.write<4>(staff+off,0);compare(*s);}
    for(auto [year,day]:{std::pair{2000u,194u},std::pair{2001u,193u},std::pair{2001u,194u},std::pair{2001u,195u},std::pair{2002u,194u}}){Options o;o.schedule_day=day;o.schedule_year=year;compare(*training(),o);}
    for(auto status:{0u,1u,128u,256u}){Options o;o.schedule_status=status;compare(*training(),o);}
    for(auto [start,end]:{std::pair{2u,20u},std::pair{20u,2u},std::pair{2u,14u},std::pair{2u,15u},std::pair{0u,0u},std::pair{65536u,65535u}}){Options o;o.start_distance=start;o.end_distance=end;compare(*training(),o);}
    for(auto days:{1u,3u,20u}){Options o;o.row_days=days;o.end_distance=14;compare(*training(),o);}
    for(auto chance:{0u,9u,127u,128u,250u,255u}){auto s=training();s->memory.write<1>(staff+0x67,chance);compare(*s);}
    auto capped=training();capped->memory.write<1>(objects+0x1000+10,50);compare(*capped);
    auto wrap=training();wrap->memory.write<4>(objects+0x234,random_words);compare(*wrap);Options late_draw;late_draw.start_distance=20;late_draw.end_distance=15;compare(*wrap,late_draw);
    for(auto days:{-1000,-32768}){Options o;o.row_days=std::uint16_t(days);compare(*training(),o);}
    auto cached=training();for(auto off:{-0x3947,-0x3948,-0x3949,-0x394a,-0x394c})cached->memory.write<1>(toc+off,1);cached->memory.write<1>(toc-0x394b,1);cached->memory.write<4>(toc-0x3944,objects+0x3000);cached->memory.write<2>(objects+0x600,194);cached->memory.write<2>(objects+0x602,2001);compare(*cached);cached->memory.write<2>(objects+0x602,2000);compare(*cached);cached->memory.write<2>(objects+0x602,2001);cached->memory.write<2>(objects+0x600,193);compare(*cached);
    for(auto [condition,debt]:{std::pair{10000,500},std::pair{9851,500},std::pair{9850,1001},std::pair{9500,1200},std::pair{9000,500},std::pair{8000,500},std::pair{7000,32767},std::pair{6999,-32768},std::pair{5000,500},std::pair{-1,500}}){auto s=training();s->memory.write<4>(staff+0x24,0);s->memory.write<2>(rows+8,std::uint16_t(condition));s->memory.write<2>(rows+12,std::uint16_t(debt));compare(*s);}
    for(auto profile_value:{0u,20u,127u,128u,255u}){auto s=training();s->memory.write<1>(profile+0x34,profile_value);compare(*s);}
    for(auto wide:{65536u,0xffff0000u}){Options o;o.force_pc=0x10187e98;o.force_register=21;o.force_value=wide;compare(*training(),o);o.nested_random_result=1000000;compare(*training(),o);}
    Options branch;branch.row_days=std::uint16_t(-20);branch.injury_days=10;branch.force_pc=0x10187c88;branch.force_cr=8;compare(*seed(5),branch);
    Options midpoint;midpoint.start_distance=20;midpoint.end_distance=2;midpoint.force_pc=0x10188018;midpoint.force_register=0;midpoint.force_value=100;compare(*training(),midpoint);
}
void random_cases() {
    Options actual;actual.real_random=true;
    for(auto bound:{0u,1u,100u,65535u,65536u,65537u,0x7fffffffu,0x80000000u,0xffff0000u,0xffff0001u,0xffffffffu}) {
        auto s=seed(6);s->wr<4>(12,bound);for(unsigned n=0;n<256;++n)s->memory.write<2>(random_words+n*2,12345);auto r=compare(*s,actual);require(r.state->pc==return_sentinel,"Complete signed bounded random fixture");
        if(bound<=65535)require(r.state->r<4>(12)==(bound?12345u%bound:0u),"Independent narrow positive random result");
    }
    for(auto bound:{100u,65536u,0xffff0000u})for(auto seed_value:{0u,65535u,65536u,0xffffffffu}) {
        auto s=seed(6);s->wr<4>(12,bound);s->memory.write<4>(objects+0x234,random_words);auto o=actual;o.random_seed=seed_value;compare(*s,o);
    }
    boundaries(*seed(6),actual);auto wide=seed(6);wide->wr<4>(12,65536);boundaries(*wide,actual);
    auto wrapped=seed(6);wrapped->wr<4>(12,65536);wrapped->memory.write<4>(objects+0x234,random_words);boundaries(*wrapped,actual);
    for(unsigned depth=1;depth<=3;++depth)for(auto value:{65536u,0xffff0000u}) {
        auto s=seed(6);s->wr<4>(12,65536);auto o=actual;
        for(unsigned n=0;n<depth;++n)o.controls.push_back({std::array{0x101865b8u,0x101866a4u,0x10186790u}[n],22-n,value,4});
        compare(*s,o);
    }
    for(unsigned depth=1;depth<=2;++depth){auto s=seed(6);s->wr<4>(12,65536);auto o=actual;o.controls.push_back({0x101865b8,22,depth==1?0x7fffffffu:65536u,4});if(depth==2)o.controls.push_back({0x101866a4,21,0x7fffffffu,4});compare(*s,o);}
    auto wrapped_nested=actual;wrapped_nested.controls={{0x101865b8,22,65536,4},{0x101866a4,21,65536,4},{0x10186790,20,65536,4}};compare(*wrapped,wrapped_nested);
    auto own=actual;own.own_negative=true;compare(*seed(6),own);auto child=actual;child.child_negative=true;compare(*wrapped,child);
    auto fitness_dependency=seed(5);fitness_dependency->memory.write<4>(rows+0x10,0xffffffff);auto o=actual;o.force_pc=0x10187e98;o.force_register=21;o.force_value=65536;compare(*fitness_dependency,o);
}
void maintenance_cases() {
    for(unsigned index=7;index<12;++index)boundaries(*seed(index));
    for(unsigned index=7;index<12;++index){auto s=seed(index);s->wr<4>(12,0);compare(*s);Options own;own.own_negative=true;compare(*seed(index),own);}
    for(auto id:{1u,2u,0x7fffffffu,0xffffffffu}){auto s=seed(7);s->memory.write<4>(staff,id);compare(*s);}
    auto absent=seed(7);absent->wr<4>(16,0);compare(*absent);
    for(auto debt:{-32768,-1001,-1000,500,1130,1131,1200,1249,1250,32767})for(auto delta:{-32768,-1,0,20,32767,65536}){auto s=seed(7);s->memory.write<2>(rows+12,std::uint16_t(debt));s->wr<4>(24,std::uint32_t(delta));compare(*s);}
    auto wrapped=seed(7);wrapped->memory.write<2>(rows+12,1200);wrapped->memory.write<4>(objects+0x234,random_words);boundaries(*wrapped);
    for(auto bound:{65536u,0xffff0000u}){Options o;o.force_pc=0x10186b78;o.force_register=28;o.force_value=bound;o.real_random=true;compare(*wrapped,o);}
    for(unsigned index:{8u,9u}) {
        auto missing=seed(index);missing->memory.write<4>(manager+0x28,0);boundaries(*missing);
        for(auto id:{0u,1u,2u}){auto s=seed(index);s->memory.write<4>(values,id);compare(*s);}
    }
    auto allocation_fail=seed(8);allocation_fail->memory.write<4>(manager+0x28,0);Options failed;failed.allocation_result=0;compare(*allocation_fail,failed);
    for(auto [type,weight]:{std::pair{1u,40u},std::pair{1u,60u},std::pair{1u,128u},std::pair{2u,40u},std::pair{2u,60u},std::pair{2u,128u}}){auto s=seed(8);s->memory.write<1>(values+4,type);s->memory.write<1>(values+5,weight);compare(*s);}
    auto no_queue=seed(10);no_queue->memory.write<4>(manager+0x28,0);no_queue->memory.write<4>(rows+0x10,0xffffffff);compare(*no_queue);
    auto repeated=seed(10);repeated->memory.write<4>(pending_rows+8,0);compare(*repeated);
    for(auto age:{18u,25u,40u,128u,255u}){auto s=seed(11);s->memory.write<2>(staff+0x5c,age);compare(*s);}
    auto friendly=seed(11);friendly->memory.write<1>(match+0x40,0);compare(*friendly);
    auto fixed_minutes=seed(11);fixed_minutes->memory.write<1>(roster+0x4c,1);compare(*fixed_minutes);
    for(auto offset:{0x1c8u,0x1c4u}){auto s=seed(11);s->memory.write<1>(roster+offset,60);compare(*s);}
    for(auto incoming:{255u,20u})for(auto outgoing:{255u,60u}){auto s=seed(11);s->memory.write<4>(roster+0x194,0xffffffff);auto slot=roster+11*0x4c;s->memory.write<4>(slot+0x194,0);s->memory.write<1>(slot+0x1c6,incoming);s->memory.write<1>(slot+0x1c8,outgoing);compare(*s);}
    auto substitute=seed(11);substitute->memory.write<4>(roster+0x194,0xffffffff);auto substitute_slot=roster+11*0x4c;substitute->memory.write<4>(substitute_slot+0x194,0);substitute->memory.write<1>(substitute_slot+0x1c6,20);substitute->memory.write<1>(substitute_slot+0x1c4,60);compare(*substitute);
    for(auto [year,day]:{std::pair{2000u,194u},std::pair{2001u,192u},std::pair{2001u,194u},std::pair{2002u,194u}}){auto s=seed(11);s->memory.write<2>(objects+0x502,year);s->memory.write<2>(objects+0x500,day);compare(*s);}
    auto wrapped_match=seed(11);wrapped_match->memory.write<4>(objects+0x234,random_words);compare(*wrapped_match);
    for(auto weight:{50u,51u,65u,66u,95u,96u,128u})for(auto type:{1u,7u}){auto s=seed(11);s->memory.write<1>(roster+0x1b3,1);s->memory.write<1>(pending_rows+4,type);s->memory.write<1>(pending_rows+5,weight);Options o;o.injury_result=1;compare(*s,o);}
    auto competitive=seed(11);competitive->memory.write<4>(match,1);competitive->memory.write<1>(roster+0x1b3,1);Options injury;injury.injury_result=1;compare(*competitive,injury);
    auto real=seed(11);Options linked;linked.real_maintenance=true;auto r=compare(*real,linked);require(r.state->memory.read<2>(rows+10)==9000 && r.state->memory.read<2>(rows+12)==600,"Independent match-to-staff fitness effects");
    for(unsigned index=7;index<12;++index){auto s=seed(index);if(index==7){s->memory.write<2>(rows+12,1200);s->memory.write<4>(objects+0x234,random_words);}Options o;o.child_negative=true;compare(*s,o);}
}
}
namespace cm::aot {void injury_fitness_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}void injury_random_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}void injury_maintenance_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try {
    const auto mode=argc==2?std::string_view(argv[1]):std::string_view{};
    if(mode.empty()||mode=="--boundaries")core_boundaries();if(mode.empty()||mode=="--readouts")guard_and_readout_cases();if(mode.empty()||mode=="--condition")condition_cases();if(mode.empty()||mode=="--refresh")refresh_cases();
    if(mode.empty()||mode=="--random")random_cases();
    if(mode.empty()||mode=="--maintenance")maintenance_cases();
    if(mode.empty())for(unsigned index=0;index<6;++index){Options o;o.own_negative=true;compare(*seed(index),o);o.own_negative=false;o.child_negative=true;if(index!=1){auto s=seed(index);if(index==0)s->memory.write<4>(objects+0x234,random_words);compare(*s,o);}}
    for(auto [start,end]:ranges)for(auto pc=start;pc<end;pc+=4)if(pc!=0x10186d24 && pc!=0x10186e4c && !sites.contains(pc))std::cerr<<"Uncovered injury fitness "<<hex_address(pc)<<std::endl;
    if(mode.empty())require(sites.size()==2018,"Complete fitness, maintenance and supporting random source coverage");
    std::cout<<"Fitness comparisons="<<cases<<" completions="<<completed<<" interruptions="<<interruptions<<" negatives="<<negatives<<" structural="<<structural<<" sites="<<sites.size()<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
