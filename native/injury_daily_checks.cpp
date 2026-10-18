#include "actor_attributes.hpp"
#include "injury_daily.hpp"
#include "semantic_io_checks.hpp"
#include "application_exit.hpp"
#include <set>

namespace cm::aot {
bool actor_attributes_reference(State&);
bool injury_daily_reference(State&);
void injury_daily_fixture_tables(State&,std::uint32_t,std::uint32_t);
}
namespace {
using namespace cm::semantic_io_checks;
namespace attributes=cm::aot::actor_attributes;
namespace daily=cm::aot::injury_daily;

constexpr std::uint32_t manager=fixture+0x1000,staff=fixture+0x2000,rows=fixture+0x4000,random_words=fixture+0x8000;
constexpr std::uint32_t objects=fixture+0xc000,profile=fixture+0x3000;
constexpr std::uint32_t pending=fixture+0x10000,pending_rows=fixture+0x11000;
constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,3> ranges{{{daily::entry,0x101864a0},{attributes::sampling_entry,0x10186a18},{attributes::record_refresh_entry,0x10188488}}};
struct Control {std::uint32_t pc;unsigned reg;std::uint32_t value;int cr=-1;unsigned occurrence=0;};
struct Options {
    unsigned interrupt=0;std::uint32_t random_seed=0,phase_result=1,row_days=3,injury_days=5,start_distance=2,end_distance=20,schedule_status=0,schedule_day=193,schedule_year=2001,nested_random_result=0;
    bool own_negative=false,child_negative=false,real_random=false,real_fitness=false;
    std::uint32_t allocation_result=pending,injury_result=0xffffffff;
    std::uint32_t weekday=4,weekday_second=4,availability_result=0,injury_gate=0,category_result=3;
    bool history_mismatch=false;
    std::vector<std::pair<std::uint32_t,std::uint32_t>> reseed_values;
    std::uint32_t force_pc=0,force_value=0;unsigned force_register=0;int force_cr=-1;
    std::vector<Control> controls;
};
struct Trace final:SemanticGuestObserver {
    Options options;std::vector<std::uint32_t> sites;std::vector<unsigned> visits;
    explicit Trace(Options value):options(value),visits(options.controls.size()){}
    void before_step(State& s,std::uint32_t pc)override {
        sites.push_back(pc);const auto apply=[&](unsigned reg,std::uint32_t value,int cr){s.wr<4>(reg*4,value);if(cr>=0)s.wr<1>(2304,(s.r<1>(2304)&0xf0)|unsigned(cr)|(s.r<1>(1024)&1));};
        if(pc==options.force_pc)apply(options.force_register,options.force_value,options.force_cr);for(unsigned n=0;n<options.controls.size();++n){const auto& c=options.controls[n];if(pc==c.pc && (++visits[n]==c.occurrence || !c.occurrence))apply(c.reg,c.value,c.cr);}
    }
};
thread_local Trace* original_trace=nullptr;
struct Event {std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
void modern_root(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    daily::process_daily_injuries(s,d,o);
}
struct Model final:SemanticDependencyDispatch {
    bool original;Options options;Trace& trace;std::vector<Event> calls;unsigned weekday_calls=0;
    Model(bool old,Options value,Trace& observer):original(old),options(value),trace(observer){}
    void reference(State& s,std::uint32_t stop=return_sentinel) {
        const auto entry=s.pc;const auto range=std::find_if(ranges.begin(),ranges.end(),[&](auto r){return r.first==entry;});require(range!=ranges.end(),"Known original daily injury root");
        bool first=true;while(s.pc!=stop){if(s.pc>=entry && s.pc<range->second && (first||s.pc!=entry))require(entry==daily::entry?injury_daily_reference(s):actor_attributes_reference(s),"Complete daily injury reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));first=false;}
    }
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override {
        constexpr std::array allowed{0x1077b770u,0x10739890u,0x10768810u,0x1077b720u,0x10769b60u,attributes::record_refresh_entry,attributes::availability_entry,0x10717dd0u,0x10715880u,0x1018b9f0u,0x102ae9c0u,0x102af650u,0x101929b0u,0x10718130u,0x101864a0u,0x10773520u,0x10738c00u,0x10738e00u,0x10738d10u,0x10738e60u,0x10188930u,attributes::capacity_assignment_entry,0x1018fdf0u,0x10188cc0u,0x10715b20u,0x10738c20u,0x10003dc0u,0x10188490u,0x1018b2b0u,0x1018bdd0u,0x1018c180u};
        require(std::find(allowed.begin(),allowed.end(),target)!=allowed.end(),"Known fitness dependency "+hex_address(target));
        Event event{target,continuation,s.registers,{}};
        if(target==attributes::sampling_entry && options.real_random){if(original)reference(s,continuation);else attributes::sample_condition_range(s,this,&trace);event.after=s.registers;calls.push_back(event);return;}
        if(target==attributes::record_refresh_entry && options.real_fitness){if(original)reference(s,continuation);else attributes::refresh_condition_record(s,this,&trace);event.after=s.registers;calls.push_back(event);return;}
        const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};
        s.memory.write<4>(fixture+0x50000+unsigned(calls.size())*4,target^arg(3));std::uint32_t result=0;
        if(target==0x1077b770){result=options.random_seed;for(auto [pc,value]:options.reseed_values)if(continuation==pc)result=value;}
        if(target==attributes::availability_entry)result=options.phase_result;
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
        if(target==0x10715b20)result=weekday_calls++?options.weekday_second:options.weekday;
        if(target==0x10003dc0)result=options.availability_result;
        if(target==0x10188490)result=options.injury_gate;
        if(target==0x1018c180)result=options.category_result+(options.history_mismatch && continuation!=0x10185bbc);
        for(unsigned n=0;n<=12;++n)if(n!=1 && n!=2)s.wr<4>(n*4,0xac000000+n*17);
        for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-20.0-n));
        s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=continuation;
        if(options.child_negative && !original){s.wr<4>(48,s.r<4>(48)^1);const auto marker=fixture+0x50000+unsigned(calls.size())*4;s.memory.write<4>(marker,s.memory.read<4>(marker)^1);}
        event.after=s.registers;calls.push_back(event);
        if(options.interrupt && calls.size()>=options.interrupt)throw std::runtime_error("Fixture daily injury dependency interruption");
    }
};
std::unique_ptr<State> seed() {
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned n=1280;n<1312;++n)s->wr<1>(n,0);s->memory.map(fixture,0x60000);s->memory.map(stack-0x4000,0x5000);s->memory.map(toc-0x8000,0x9000);
    s->pc=daily::entry;s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->budget=100000;
    for(auto [slot,pointer]:{std::pair{-0x7ba8,objects},std::pair{-0x7bac,objects+0x100},std::pair{-0x7be4,objects+0x200},std::pair{-0x7be0,objects+0x218},std::pair{-0x7b90,objects+0x238},std::pair{-0x7b8c,random_words},std::pair{-0x7b88,objects+0x234},std::pair{-0x7b84,objects+0x230},std::pair{-0x7b80,objects+0x500},std::pair{-0x5448,objects+0x600},std::pair{-0x544c,objects+0x610},std::pair{-0x5450,objects+0x620},std::pair{-0x7b9c,objects+0x240},std::pair{-0x7a64,objects+0x22c}})s->memory.write<4>(toc+slot,pointer);
    for(auto [pointer,value]:{std::pair{objects+0x200,2u},std::pair{objects+0x218,staff},std::pair{objects+0x230,random_words},std::pair{objects+0x234,random_words+0x400},std::pair{objects+0x240,objects+0x1000},std::pair{objects+0x22c,objects+0x400}})s->memory.write<4>(pointer,value);
    s->memory.write<4>(manager,rows);s->memory.write<4>(manager+4,rows+0x1000);s->memory.write<4>(manager+8,1);s->memory.write<4>(staff,0);s->memory.write<4>(staff+0x14,profile);s->memory.write<4>(staff+0x20,objects+0x2000);s->memory.write<4>(staff+0x24,objects+0x3000);s->memory.write<1>(staff+0x67,9);s->memory.write<2>(staff+0x5c,25);s->memory.write<1>(profile+0x34,20);
    s->memory.write<2>(objects+0x500,194);s->memory.write<2>(objects+0x502,2001);s->memory.write<1>(objects+0x504,2);s->memory.write<2>(rows,192);s->memory.write<2>(rows+2,2001);s->memory.write<2>(rows+8,8000);s->memory.write<2>(rows+10,7000);s->memory.write<2>(rows+12,500);s->memory.write<4>(rows+0x10,0);s->memory.write<1>(rows+0x22,50);s->memory.write<2>(rows+0x1000+0x30,8);s->memory.write<2>(rows+0x1000+0x32,2);
    injury_daily_fixture_tables(*s,toc,fixture+0x34000);
    s->wr<4>(12,manager);s->wr<4>(16,staff);s->wr<4>(20,0);
    s->memory.write<4>(toc-0x7a9c,objects+0x900);s->memory.write<2>(objects+0x900,193);s->memory.write<2>(objects+0x902,2001);
    s->memory.write<4>(manager+0x28,pending);s->memory.write<4>(pending,2);s->memory.write<4>(pending+4,0);s->memory.write<4>(pending+12,0);
    s->memory.write<4>(toc-0x7b24,objects+0xa00);s->memory.write<4>(objects+0xa00,objects+0xb00);s->memory.write<4>(objects+0x200,1);
    s->memory.write<1>(profile+0x2d,10);s->memory.write<1>(profile+15,15);s->memory.write<4>(rows+0x10,0xffffffff);s->memory.write<1>(rows+0x14,255);
    s->memory.write<2>(rows+0x1000+0x34,10);s->memory.write<2>(rows+0x1000+0x36,8);
    for(unsigned n=0;n<256;++n)s->memory.write<2>(random_words+n*2,3);
    return s;
}
unsigned cases=0,completed=0,interruptions=0,negatives=0,structural=0;std::set<std::uint32_t> sites;
struct Result {std::unique_ptr<State> state;std::vector<Effects::Access> accesses;std::vector<Event> calls;};
Result compare(const State& input,Options options={}) {
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    Trace old_trace(options),new_trace(options);Model old(true,options,old_trace),modern(false,options,new_trace);
    const auto run=[&](State& s,Model& m,bool reference){try{if(reference){original_trace=&old_trace;m.reference(s);}else modern_root(s,&m,&new_trace);original_trace=nullptr;return std::string{};}catch(const ApplicationExit&){original_trace=nullptr;return std::string("Fixture application exit");}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="Daily injury entry="+hex_address(input.pc)+" case="+std::to_string(cases);
    if((expected!=actual || old_trace.sites!=new_trace.sites) && !options.child_negative){for(unsigned n=0;n<std::min(old_trace.sites.size(),new_trace.sites.size());++n)if(old_trace.sites[n]!=new_trace.sites[n]){std::cerr<<"First daily site "<<n<<" "<<hex_address(old_trace.sites[n])<<"/"<<hex_address(new_trace.sites[n])<<std::endl;break;}}
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
    require(base.state->pc==return_sentinel,"Complete normal daily injury boundary fixture");
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
void daily_cases() {
    const auto eligible=[](){auto s=seed();s->memory.write<1>(staff+0x62,11);return s;};
    boundaries(*seed());boundaries(*eligible());
    auto no_pending=seed();no_pending->memory.write<4>(manager+0x28,0);compare(*no_pending);
    auto null_manager=seed();null_manager->wr<4>(12,0);compare(*null_manager);
    for(auto day:{5u,6u,65542u}){Options o;o.weekday=day;o.weekday_second=day;compare(*seed(),o);}
    for(auto [year,day]:{std::pair{2000u,180u},std::pair{2001u,180u},std::pair{2001u,193u},std::pair{2002u,180u}}){auto s=seed();s->memory.write<2>(objects+0x502,year);s->memory.write<2>(objects+0x500,day);compare(*s);}
    auto season=seed();season->memory.write<2>(objects+0x500,180);season->memory.write<2>(objects+0x902,2002);Options shared;shared.real_fitness=true;compare(*season,shared);
    auto zero_staff=seed();zero_staff->memory.write<4>(objects+0x200,0);compare(*zero_staff);
    auto no_profile=seed();no_profile->memory.write<4>(staff+0x14,0);no_profile->budget=500;compare(*no_profile);
    for(auto kind:{11u,13u,15u,12u,128u}){auto s=eligible();s->memory.write<1>(staff+0x62,kind);compare(*s);}
    for(auto availability:{1u,128u,256u}){Options o;o.availability_result=availability;compare(*eligible(),o);}
    auto injured=eligible();injured->memory.write<4>(rows+0x10,0);compare(*injured);
    for(auto condition:{0u,19u,20u,128u}){auto s=eligible();s->memory.write<1>(rows+0x22,condition);compare(*s);}
    for(auto count:{1500u,2100u}){auto s=eligible();s->memory.write<4>(objects+0x200,count);Options o;o.controls={{0x101855b8,16,1}};compare(*s,o);}
    for(auto category:{15u,20u}){auto s=eligible();s->memory.write<1>(profile+15,category);constexpr std::array pattern{3u,0u,0u,3u,3u,3u,3u};if(category==20)for(unsigned n=0;n<256;++n)s->memory.write<2>(random_words+n*2,pattern[(n+6)%7]);compare(*s);}
    auto previous=eligible();previous->memory.write<1>(rows+0x14,1);previous->memory.write<1>(rows+0x15,0);compare(*previous);previous->memory.write<1>(rows+0x15,254);compare(*previous);
    for(auto rare:{0u,1u}){auto s=eligible();s->memory.write<2>(random_words+8,0);s->memory.write<2>(random_words+10,rare);s->memory.write<2>(random_words+12,1);compare(*s);}
    auto wrapped=eligible();wrapped->memory.write<4>(objects+0x234,random_words);compare(*wrapped);
    for(auto [result,attribute]:{std::pair{12u,20u},std::pair{12u,19u},std::pair{28u,20u},std::pair{45u,16u},std::pair{59u,20u},std::pair{59u,16u}}){auto s=clone(*wrapped);s->memory.write<1>(rows+0x14,1);s->memory.write<1>(rows+0x15,0);s->memory.write<1>(profile+0x1b,attribute);s->memory.write<1>(profile+0x36,20);s->memory.write<1>(profile+0x1d,attribute);Options o;o.injury_result=result;compare(*s,o);o.history_mismatch=true;o.reseed_values={{0x1018601c,1},{0x101861f4,1},{0x10186384,1}};compare(*s,o);}
    auto special=clone(*wrapped);special->memory.write<1>(profile+15,20);compare(*special);
    Options rare_wrap;rare_wrap.controls={{0x10185c24,16,0,2,1}};compare(*wrapped,rare_wrap);
    for(auto result:{0u,12u,20u,28u,45u,59u,60u,81u}){auto s=eligible();s->memory.write<1>(profile+0x1b,20);s->memory.write<1>(profile+0x36,20);s->memory.write<1>(profile+0x1d,20);Options o;o.injury_result=result;compare(*s,o);}
    for(auto attr:{15u,16u,18u,19u,21u})for(auto result:{12u,45u,59u}){auto s=eligible();s->memory.write<1>(profile+0x1b,attr);s->memory.write<1>(profile+0x1d,attr);Options o;o.injury_result=result;compare(*s,o);}
    for(auto result:{12u,28u}){auto s=eligible();s->memory.write<1>(rows+0x14,1);s->memory.write<1>(rows+0x15,0);s->memory.write<1>(profile+0x1b,20);s->memory.write<1>(profile+0x36,20);Options o;o.injury_result=result;compare(*s,o);o.history_mismatch=true;compare(*s,o);}
    for(auto result:{12u,59u}){auto s=eligible();s->memory.write<1>(profile+0x1b,20);s->memory.write<1>(profile+0x1d,20);s->memory.write<2>(random_words+12,4);s->memory.write<2>(random_words+14,4);Options o;o.injury_result=result;compare(*s,o);}
    for(auto bound:{65536u,0xffff0000u}) {
        auto s=eligible();Options o;o.real_random=true;o.controls={{0x101855f0,16,bound},{0x10185710,17,0}};compare(*s,o);
        s=eligible();s->memory.write<1>(rows+0x14,1);s->memory.write<1>(rows+0x15,0);o.controls={{0x10185a2c,16,bound}};compare(*s,o);
        o.controls={{0x10185cd4,23,bound}};compare(*eligible(),o);
    }
    Options zero_span;zero_span.controls={{0x10185ca0,23,0,2,1}};compare(*eligible(),zero_span);
    Options underage;underage.controls={{0x10185dfc,16,7}};auto young=eligible();young->memory.write<1>(profile+15,14);young->budget=1000;compare(*young,underage);
    Options gate;gate.injury_gate=1;compare(*eligible(),gate);
    Options own;own.own_negative=true;compare(*seed(),own);own.own_negative=false;own.child_negative=true;compare(*seed(),own);
}
}
namespace cm::aot {
void actor_attributes_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}
void injury_daily_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}
}
int main(){try {
    daily_cases();
    unsigned daily_sites=0;
    for(auto pc=daily::entry;pc<0x101864a0;pc+=4){if(sites.contains(pc))++daily_sites;else std::cerr<<"Uncovered daily injury "<<hex_address(pc)<<std::endl;}
    require(daily_sites==1060,"Complete original daily controller source coverage");
    std::cout<<"Daily comparisons="<<cases<<" completions="<<completed<<" interruptions="<<interruptions<<" negatives="<<negatives<<" structural="<<structural<<" daily_sites="<<daily_sites<<" dependency_sites="<<sites.size()-daily_sites<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
