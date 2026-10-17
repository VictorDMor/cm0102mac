#include "injury_fitness.hpp"
#include "injury_random.hpp"
#include "semantic_io_checks.hpp"
#include "application_exit.hpp"
#include <set>

namespace cm::aot {bool injury_fitness_reference(State&);bool injury_random_reference(State&);void injury_fitness_fixture_tables(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace fitness=cm::aot::injury_fitness;
namespace random_draw=cm::aot::injury_random;
constexpr std::array entries{fitness::entries[0],fitness::entries[1],fitness::entries[2],fitness::entries[3],fitness::entries[4],fitness::entries[5],random_draw::entry};
constexpr std::uint32_t manager=fixture+0x1000,staff=fixture+0x2000,rows=fixture+0x4000,random_words=fixture+0x8000;
constexpr std::uint32_t objects=fixture+0xc000,tables=fixture+0x30000,profile=fixture+0x3000;
constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,7> ranges{{{0x10187450,0x10187644},{0x10187650,0x10187680},{0x10187680,0x101877dc},{0x101877e0,0x101878d0},{0x101878d0,0x10187a1c},{0x10187a40,0x10188488},{0x101864a0,0x10186a18}}};
struct Control {std::uint32_t pc;unsigned reg;std::uint32_t value;int cr=-1;};
struct Options {
    unsigned interrupt=0;std::uint32_t random_seed=0,phase_result=1,row_days=3,injury_days=5,start_distance=2,end_distance=20,schedule_status=0,schedule_day=193,schedule_year=2001,nested_random_result=0;
    bool own_negative=false,child_negative=false,real_random=false;
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
    constexpr Function methods[]{fitness::update_staff_condition,fitness::read_staff_condition,fitness::classify_injury_recovery,fitness::read_staff_fitness,fitness::read_effective_fitness,fitness::refresh_staff_fitness,random_draw::draw_signed_bounded};
    const auto entry=std::find(entries.begin(),entries.end(),s.pc);require(entry!=entries.end(),"Known injury fitness root");methods[entry-entries.begin()](s,d,o);
}
struct Model final:SemanticDependencyDispatch {
    bool original;Options options;Trace& trace;std::vector<Event> calls;
    Model(bool old,Options value,Trace& observer):original(old),options(value),trace(observer){}
    void reference(State& s,std::uint32_t stop=return_sentinel) {
        const auto entry=s.pc;const auto range=std::find_if(ranges.begin(),ranges.end(),[&](auto r){return r.first==entry;});require(range!=ranges.end(),"Known original injury fitness root");
        bool first=true;while(s.pc!=stop){if(s.pc>=entry && s.pc<range->second && (first||s.pc!=entry))require(entry==random_draw::entry?injury_random_reference(s):injury_fitness_reference(s),"Complete injury fitness reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));first=false;}
    }
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override {
        constexpr std::array<std::uint32_t,15> allowed{0x1077b770,0x10739890,0x10768810,0x1077b720,0x10769b60,fitness::refresh_entry,fitness::recovery_phase_entry,0x10717dd0,0x10715880,0x1018b9f0,0x102ae9c0,0x102af650,0x101929b0,0x10718130,0x101864a0};
        require(std::find(allowed.begin(),allowed.end(),target)!=allowed.end(),"Known fitness dependency "+hex_address(target));
        Event event{target,continuation,s.registers,{}};
        if(target==random_draw::entry && options.real_random){if(original)reference(s,continuation);else random_draw::draw_signed_bounded(s,this,&trace);event.after=s.registers;calls.push_back(event);return;}
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
    s->wr<4>(12,index==6?100u:manager);s->wr<4>(16,index<2?0u:staff);s->wr<4>(20,0);return s;
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
}
namespace cm::aot {void injury_fitness_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}void injury_random_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try {
    const auto mode=argc==2?std::string_view(argv[1]):std::string_view{};
    if(mode.empty()||mode=="--boundaries")core_boundaries();if(mode.empty()||mode=="--readouts")guard_and_readout_cases();if(mode.empty()||mode=="--condition")condition_cases();if(mode.empty()||mode=="--refresh")refresh_cases();
    if(mode.empty()||mode=="--random")random_cases();
    if(mode.empty())for(unsigned index=0;index<6;++index){Options o;o.own_negative=true;compare(*seed(index),o);o.own_negative=false;o.child_negative=true;if(index!=1){auto s=seed(index);if(index==0)s->memory.write<4>(objects+0x234,random_words);compare(*s,o);}}
    for(auto [start,end]:ranges)for(auto pc=start;pc<end;pc+=4)if(!sites.contains(pc))std::cerr<<"Uncovered injury fitness "<<hex_address(pc)<<std::endl;
    if(mode.empty())require(sites.size()==1375,"Complete fitness and supporting random source coverage");
    std::cout<<"Fitness comparisons="<<cases<<" completions="<<completed<<" interruptions="<<interruptions<<" negatives="<<negatives<<" structural="<<structural<<" sites="<<sites.size()<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
