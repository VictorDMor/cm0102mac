#include "injury_club_events.hpp"
#include "semantic_io_checks.hpp"
#include <set>

namespace cm::aot {bool injury_club_events_reference(State&);}
namespace {
using namespace cm::semantic_io_checks;
namespace club_events=cm::aot::injury_club_events;
constexpr std::uint32_t manager=fixture+0x1000,events=fixture+0x3000,reallocated=fixture+0x6000;
constexpr std::uint32_t club=fixture+0x10000,staff=fixture+0x14000,rows=fixture+0x18000;
constexpr std::uint32_t globals=fixture+0x20000,pool=fixture+0x21000,date=fixture+0x22000;
constexpr std::uint32_t objects=fixture+0x24000,descriptor=fixture+0x25000,context=fixture+0x26000,strings=fixture+0x28000;
constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,3> ranges{{{club_events::enqueue_entry,0x1018bdd0},{club_events::process_entry,0x1018c180},{club_events::compare_entry,0x10190180}}};
struct Options {
    unsigned interrupt=0;std::uint32_t allocation=reallocated,blocked=0,random=0,cutoff_day=15,cutoff_year=2001;
    std::uint32_t force_pc=0;int force_cr=-1;
    bool mutate_club=false,mutate_sort=false,register_mark=true,own_negative=false,child_negative=false;
};
struct Trace final:SemanticGuestObserver {
    Options options;std::vector<std::uint32_t> sites;
    explicit Trace(Options value):options(value){}
    void before_step(State& s,std::uint32_t pc)override{sites.push_back(pc);if(options.force_pc==pc && options.force_cr>=0)s.wr<1>(2304,(s.r<1>(2304)&0xf0)|unsigned(options.force_cr)|(s.r<1>(1024)&1));}
};
thread_local Trace* original_trace=nullptr;
struct Event {std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
void modern_root(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    if(s.pc==club_events::enqueue_entry)club_events::enqueue_club_event(s,d,o);
    else if(s.pc==club_events::process_entry)club_events::process_club_events(s,d,o);
    else {require(s.pc==club_events::compare_entry,"Known club event component root");club_events::compare_event_date(s,d,o);}
}
struct Model final:SemanticDependencyDispatch {
    bool original;Options options;Trace& trace;std::vector<Event> calls;
    Model(bool old,Options o,Trace& t):original(old),options(o),trace(t){}
    void reference(State& s,std::uint32_t stop=return_sentinel) {
        const auto entry=s.pc;auto range=std::find_if(ranges.begin(),ranges.end(),[&](auto r){return r.first==entry;});require(range!=ranges.end(),"Known club event reference root");
        while(s.pc!=stop){if(s.pc>=entry && s.pc<range->second)require(injury_club_events_reference(s),"Complete club event reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));}
    }
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override {
        Event event{target,continuation,s.registers,{}};const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};
        constexpr std::array<std::uint32_t,11> allowed{0x1077b770,0x100c7d00,0x107751b0,0x10775130,0x10739890,0x10768810,0x1077b720,0x10769b60,0x10768300,0x10717b20,0x1077fc28};
        require(target==0x10188cc0 || std::find(allowed.begin(),allowed.end(),target)!=allowed.end(),"Known club event dependency "+hex_address(target));
        s.memory.write<4>(fixture+0x40000+unsigned(calls.size())*4,target^arg(3));std::uint32_t result=0;
        if(target==0x1077b770)result=options.random;
        if(target==0x100c7d00){result=options.blocked;if(options.mutate_club){s.memory.write<4>(manager+0x1c,events+0x1000);s.memory.write<4>(manager+0x24,0);}}
        if(target==0x107751b0 || target==0x10775130)result=options.allocation;
        if(target==0x10768810)result=0x71;
        if(target==0x10717b20){s.memory.write<2>(arg(3),options.cutoff_day);s.memory.write<2>(arg(3)+2,options.cutoff_year);s.memory.write<1>(arg(3)+4,1);}
        if(target==0x1077fc28)s.memory.copy_in(arg(4),s.memory.copy_out(arg(3),arg(5)));
        if(target==0x10188cc0 && options.register_mark){const auto id=s.memory.read<4>(arg(6));s.memory.write<1>(rows+id*0x24+0x21,arg(4));}
        if(target==0x10768300 && arg(4)>1 && arg(4)<5) {
            const auto base=arg(3);s.wr<4>(12,base);s.wr<4>(16,base+20);s.pc=club_events::compare_entry;s.wr<4>(4128,continuation);
            if(original)reference(s,continuation);else modern_root(s,this,&trace);
            if(options.mutate_sort)s.memory.write<4>(manager+0x24,1);
        }
        for(unsigned n=0;n<=12;++n)if(n!=1 && n!=2)s.wr<4>(n*4,0xac000000+n*17);
        s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);
        if(options.child_negative && !original && target==0x100c7d00)s.wr<4>(12,result^1);
        s.pc=continuation;event.after=s.registers;calls.push_back(event);
        if(options.interrupt && calls.size()>=options.interrupt)throw std::runtime_error("Fixture club event dependency interruption");
    }
};
std::unique_ptr<State> seed(std::uint32_t entry=club_events::enqueue_entry) {
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);s->memory.map(fixture,0x60000);s->memory.map(stack-0x4000,0x5000);s->memory.map(toc-0x8000,0x9000);
    s->pc=entry;s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->memory.write<4>(stack+0x14,toc);
    for(auto [slot,pointer]:{std::pair{-0x7b90,globals},std::pair{-0x7b8c,pool},std::pair{-0x7b88,globals+4},std::pair{-0x7b84,globals+8},std::pair{-0x7ba8,objects+0x200},std::pair{-0x5438,strings},std::pair{-0x7be8,objects},std::pair{-0x7be4,objects+4},std::pair{-0x7bac,objects+8},std::pair{-0x7b80,date},std::pair{-0x5460,descriptor}})s->memory.write<4>(toc+slot,pointer);
    s->memory.write<4>(objects,club);s->memory.write<4>(objects+4,16);s->memory.write<4>(globals+4,pool+0x1fe);s->memory.write<4>(globals+8,pool);
    s->memory.write<4>(manager,rows);s->memory.write<4>(manager+0x1c,events);s->memory.write<4>(manager+0x20,30);s->memory.write<4>(manager+0x24,1);
    s->memory.write<4>(staff,0);s->memory.write<4>(staff+0x14,objects+0x400);s->memory.write<1>(rows+0x21,255);s->memory.write<4>(context+0x68,123);
    for(unsigned n=0;n<50;++n)s->memory.write<4>(club+0x98+n*4,staff);
    for(unsigned n=0;n<4;++n){s->memory.write<2>(events+n*20,20);s->memory.write<2>(events+n*20+2,2001);s->memory.write<1>(events+n*20+4,1);s->memory.write<1>(events+n*20+8,3);s->memory.write<4>(events+n*20+12,0);s->memory.write<4>(events+n*20+16,0xffffffff);}
    s->memory.write<2>(date,20);s->memory.write<2>(date+2,2001);s->memory.write<1>(date+4,1);s->memory.write<4>(descriptor,club_events::compare_entry);s->memory.write<4>(descriptor+4,toc);
    s->wr<4>(12,manager);s->wr<4>(16,date);s->wr<4>(20,3);s->wr<4>(24,0);s->wr<4>(28,0);
    if(entry==club_events::compare_entry){s->wr<4>(12,date);s->wr<4>(16,events);}
    return s;
}
unsigned cases=0,completed=0,interruptions=0,negatives=0,structural=0;std::set<std::uint32_t> sites;
struct Result {std::unique_ptr<State> state;std::vector<Effects::Access> accesses;std::vector<Event> calls;};
Result compare(const State& input,Options options={}) {
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    Trace old_trace(options),new_trace(options);Model old(true,options,old_trace),modern(false,options,new_trace);
    const auto run=[&](State& s,Model& m,bool reference){try{if(reference){original_trace=&old_trace;m.reference(s);}else modern_root(s,&m,&new_trace);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="Club event entry="+hex_address(input.pc)+" case="+std::to_string(cases);
    if((expected!=actual || old_trace.sites!=new_trace.sites) && !options.child_negative){for(unsigned n=0;n<std::min(old_trace.sites.size(),new_trace.sites.size());++n)if(old_trace.sites[n]!=new_trace.sites[n]){std::cerr<<"First site "<<n<<" "<<hex_address(old_trace.sites[n])<<"/"<<hex_address(new_trace.sites[n])<<std::endl;break;}}
    require(expected==actual,label+" original="+expected+" modern="+actual);if(options.own_negative)b->wr<4>(52,b->r<4>(52)^1);
    bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative && !options.child_negative)throw;rejected=true;}
    require(rejected==(options.own_negative||options.child_negative),label+" discriminating negative");
    if(!rejected){require(x.bytes==y.bytes,label+" ordered effects");require(a->executed==b->executed,label+" accounting");require(old.calls==modern.calls,label+" dependency ABI");require(old_trace.sites==new_trace.sites,label+" source order");}
    if(options.force_pc && expected.empty())require(std::find(old_trace.sites.begin(),old_trace.sites.end(),options.force_pc)!=old_trace.sites.end(),label+" structural control reached");
    ++cases;completed+=expected.empty()&&!rejected;interruptions+=!expected.empty();negatives+=rejected;structural+=options.force_pc!=0;sites.insert(old_trace.sites.begin(),old_trace.sites.end());return{std::move(a),x.log,old.calls};
}
void boundaries(const State& input,Options options={},bool all_budgets=true) {
    const auto baseline=compare(input,options);const auto steps=baseline.state->executed-input.executed;
    if(all_budgets)for(std::uint64_t n=0;n<=steps;++n){auto s=clone(input);s->budget=s->executed+n;compare(*s,options);}
    else for(auto n:{std::uint64_t(0),std::uint64_t(1),steps/2,steps?steps-1:0,steps}){auto s=clone(input);s->budget=s->executed+n;compare(*s,options);}
    std::set<std::uint32_t> bytes;for(const auto& a:baseline.accesses)for(unsigned n=0;n<a.size;++n)bytes.insert(a.address+n);
    for(auto hole:bytes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;for(auto b:bytes)if(b!=hole){s->memory.map(b,1);s->memory.write<1>(b,input.memory.read<1>(b));}compare(*s,options);}
    for(unsigned n=1;n<=baseline.calls.size();++n){auto o=options;o.interrupt=n;compare(input,o);}
}
void enqueue_checks() {
    boundaries(*seed());
    for(auto type:{0u,1u,2u,3u,4u,127u,128u,255u,256u})for(auto random:{0u,1u,4u,15u,65535u}){
        auto s=seed();s->wr<4>(20,type);for(unsigned n=0;n<0x200;n+=2)s->memory.write<2>(pool+n,random);boundaries(*s,{},false);
    }
    for(auto count:{0u,1u,30u,31u})for(auto pointer:{0u,events})for(auto allocation:{0u,reallocated}) {
        auto s=seed();s->memory.write<4>(manager+0x24,count);s->memory.write<4>(manager+0x1c,pointer);Options o;o.allocation=allocation;boundaries(*s,o);
    }
    for(auto pointer:{0u,context}){auto s=seed();s->wr<4>(28,pointer);auto r=compare(*s);const auto base=r.state->memory.read<4>(manager+0x1c);require(r.state->memory.read<4>(manager+0x24)==2 && r.state->memory.read<4>(base+36)==(pointer?123u:0xffffffffu),"Independent queued event binding");}
    for(auto salt:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto cursor:{pool,pool+0x1fe,0xfffffffeu}){auto s=seed();s->memory.write<4>(globals,salt);s->memory.write<4>(globals+8,cursor);Options o;o.random=65535;boundaries(*s,o,false);}
    {auto s=seed();s->wr<4>(20,0);s->memory.write<4>(globals+8,pool+0x1fe);Options o;o.random=1;boundaries(*s,o);}
    for(auto blocked:{0u,1u,128u,256u}){Options o;o.blocked=blocked;o.mutate_club=true;boundaries(*seed(),o,false);}
    {Options o;o.mutate_sort=true;boundaries(*seed(),o,false);}
    {Options o;o.child_negative=true;compare(*seed(),o);}
}
void process_checks() {
    boundaries(*seed(club_events::process_entry));
    for(auto count:{0u,1u,2u,4u})for(auto day:{0u,14u,15u,19u,20u,21u,32768u,65535u})for(auto year:{2000u,2001u,2002u}) {
        auto s=seed(club_events::process_entry);s->memory.write<4>(manager+0x24,count);
        for(unsigned n=0;n<count;++n){s->memory.write<2>(events+n*20,day);s->memory.write<2>(events+n*20+2,year);}
        boundaries(*s,{},false);
    }
    for(auto type:{0u,1u,2u,3u,4u,128u,255u})for(auto random:{0u,1u,2u,3u,49u,50u,65535u}) {
        auto s=seed(club_events::process_entry);s->memory.write<1>(events+8,type);for(unsigned n=0;n<0x200;n+=2)s->memory.write<2>(pool+n,random);boundaries(*s,{},false);
    }
    for(auto expired:{1u,2u,4u}){auto s=seed(club_events::process_entry);s->memory.write<4>(manager+0x24,4);for(unsigned n=0;n<expired;++n)s->memory.write<2>(events+n*20,14);boundaries(*s);}
    for(auto pointer:{0u,staff})for(auto id:{0u,15u,16u,0x7fffffffu,0x80000000u,0xffffffffu}){auto s=seed(club_events::process_entry);s->memory.write<4>(club+0x98,pointer);s->memory.write<4>(staff,id);boundaries(*s,{},false);}
    for(auto blocked:{0u,1u,128u,256u}){Options o;o.blocked=blocked;boundaries(*seed(club_events::process_entry),o);}
    for(auto cursor:{pool,pool+0x1fc,pool+0x1fe,0xfffffffeu}){auto s=seed(club_events::process_entry);s->memory.write<4>(globals+8,cursor);boundaries(*s);}
    {auto s=seed(club_events::process_entry);s->memory.write<1>(events+8,0);s->memory.write<4>(globals+8,pool+0x1fe);Options o;o.random=1;boundaries(*s,o);}
    for(auto cutoff_year:{0u,2000u,2001u,2002u,65535u}){Options o;o.cutoff_year=cutoff_year;boundaries(*seed(club_events::process_entry),o,false);}
    {Options o;o.register_mark=false;boundaries(*seed(club_events::process_entry),o);}
    {Options o;o.child_negative=true;compare(*seed(club_events::process_entry),o);}
}
void date_checks() {
    for(auto year:{0u,1u,2000u,2001u,65535u})for(auto day:{0u,1u,19u,20u,21u,32767u,32768u,65535u}){
        auto s=seed(club_events::compare_entry);s->memory.write<2>(events,day);s->memory.write<2>(events+2,year);auto r=compare(*s);
        const auto signed_day=day<32768?int(day):int(day)-65536;const auto expected=year>2001 || (year==2001 && signed_day>20)?0xffffffffu:year<2001 || (year==2001 && signed_day<20)?1u:0u;
        require(r.state->r<4>(12)==expected,"Independent club event date order");if(year==2001)boundaries(*s);
    }
    for(auto offset:{0u,4u,8u,0xfffffff8u}){auto s=seed(club_events::compare_entry);s->wr<4>(16,stack+offset);boundaries(*s);}
    {Options o;o.force_pc=club_events::compare_entry+0x84;o.force_cr=4;boundaries(*seed(club_events::compare_entry),o);}
}
}
namespace cm::aot {void injury_club_events_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try {
    const auto mode=argc==2?std::string_view(argv[1]):std::string_view{};
    if(mode.empty()||mode=="--enqueue")enqueue_checks();if(mode.empty()||mode=="--process")process_checks();if(mode.empty()||mode=="--dates")date_checks();
    if(mode.empty())for(auto entry:club_events::entries){Options o;o.own_negative=true;compare(*seed(entry),o);}
    for(auto [start,end]:ranges)for(auto pc=start;pc<end;pc+=4)if(!sites.contains(pc))std::cerr<<"Uncovered club event "<<hex_address(pc)<<std::endl;
    if(mode.empty())require(sites.size()==448,"Complete club injury event source coverage");
    std::cout<<"Club event comparisons="<<cases<<" completions="<<completed<<" interruptions="<<interruptions<<" negatives="<<negatives<<" structural="<<structural<<" sites="<<sites.size()<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
