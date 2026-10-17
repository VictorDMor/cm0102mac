#include "injury_reporting.hpp"
#include "injury_readouts.hpp"
#include "semantic_io_checks.hpp"
#include "application_exit.hpp"
#include <set>

namespace cm::aot {bool injury_reporting_reference(State&);void injury_reporting_fixture_details(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace reporting=cm::aot::injury_reporting;
constexpr std::uint32_t manager=fixture+0x1000,staff=fixture+0x2000,rows=fixture+0x4000,injuries=fixture+0x8000;
constexpr std::uint32_t objects=fixture+0xc000,output=fixture+0xe000,strings=fixture+0x10000,details=fixture+0x20000;
constexpr std::uint32_t message=fixture+0x22000,clubs=fixture+0x24000,competitions=fixture+0x26000;
constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,4> ranges{{{0x1018f340,0x1018f468},{0x1018f470,0x1018fbd8},{0x1018fbe0,0x1018fdf0},{0x1018fdf0,0x1018ff04}}};
struct Options {
    unsigned interrupt=0;std::uint32_t link=1,competition=1,match=1,form=2;
    bool own_negative=false,child_negative=false,refresh_mutate=false,argument_mutate=false,fatal_exit=false;
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
    constexpr Function methods[]{reporting::has_special_injury_type,reporting::compose_injury_notice,reporting::describe_injury_match,reporting::clear_injury_history};
    const auto entry=std::find(reporting::entries.begin(),reporting::entries.end(),s.pc);require(entry!=reporting::entries.end(),"Known injury reporting root");methods[entry-reporting::entries.begin()](s,d,o);
}
struct Model final:SemanticDependencyDispatch {
    bool original;Options options;Trace& trace;std::vector<Event> calls;
    Model(bool old,Options value,Trace& observer):original(old),options(value),trace(observer){}
    void reference(State& s,std::uint32_t stop=return_sentinel) {
        const auto entry=s.pc;const auto range=std::find_if(ranges.begin(),ranges.end(),[&](auto r){return r.first==entry;});require(range!=ranges.end(),"Known original injury reporting root");
        while(s.pc!=stop){if(s.pc>=entry && s.pc<range->second)require(injury_reporting_reference(s),"Complete injury reporting reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));}
    }
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override {
        constexpr std::array<std::uint32_t,18> allowed{0x10187a40,0x10739890,0x10768810,0x1077b720,0x10769b60,0x1076f7d0,0x10749fc0,0x102cb570,0x107156b0,0x100cb590,0x100cbde0,0x10727890,0x1074b830,0x1074b960,0x1074bab0,0x1030e880,0x104abf40,0x1029eb20};
        require(std::find(allowed.begin(),allowed.end(),target)!=allowed.end() || target==injury_readouts::category_text_entry || target==injury_readouts::type_text_entry || target==injury_readouts::type_form_entry,"Known reporting dependency "+hex_address(target));
        Event event{target,continuation,s.registers,{}};const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};
        s.memory.write<4>(fixture+0x50000+unsigned(calls.size())*4,target^arg(3));std::uint32_t result=0;
        if(target==0x102cb570){result=std::uint32_t(s.memory.read<4>(arg(3)+8+arg(4)*4));if(options.argument_mutate)s.memory.write<4>(arg(3)+8+2*4,3);}
        if(target==0x10739890){s.memory.write<4>(arg(6),0x12345678);s.memory.write<4>(arg(7),0x87654321);}
        if(target==0x107156b0){s.memory.write<2>(arg(3),arg(4));s.memory.write<2>(arg(3)+2,arg(5));s.memory.write<1>(arg(3)+4,0);}
        if(target==0x100cb590){s.memory.write<4>(arg(6),arg(3)^arg(4));s.memory.write<4>(arg(6)+4,arg(8));}
        if(target==0x100cbde0)result=options.link;
        if(target==0x104abf40)result=options.competition;
        if(target==0x1029eb20)result=options.match;
        if(target==0x10727890 || target==0x1077b720){s.memory.write<4>(arg(3),arg(4));s.memory.write<4>(arg(3)+4,arg(5));}
        if(target==0x1074b830 || target==0x1074b960 || target==0x1074bab0){s.memory.write<4>(arg(3)+unsigned(calls.size())*4,arg(4));}
        if(target==0x1030e880){s.memory.write<4>(arg(4),arg(3));s.memory.write<4>(arg(4)+4,arg(6));}
        if(target==injury_readouts::category_text_entry || target==injury_readouts::type_text_entry){s.memory.write<4>(arg(5),arg(4));s.memory.write<4>(arg(5)+4,arg(6));result=options.form;}
        if(target==injury_readouts::type_form_entry)result=options.form;
        if(target==0x10749fc0)s.memory.write<4>(arg(5),0xffffffff);
        if(target==0x10187a40 && options.refresh_mutate){s.memory.write<4>(manager,rows+0x1000);s.memory.write<4>(rows+0x1010,1);s.memory.write<1>(injuries+0x38,0);}
        for(unsigned n=0;n<=12;++n)if(n!=1 && n!=2)s.wr<4>(n*4,0xac000000+n*17);
        for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-20.0-n));
        s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=continuation;
        if(options.child_negative && !original)s.wr<4>(48,s.r<4>(48)^1);
        event.after=s.registers;calls.push_back(event);
        if(target==0x1076f7d0 && options.fatal_exit)throw ApplicationExit{};
        if(options.interrupt && calls.size()>=options.interrupt)throw std::runtime_error("Fixture injury reporting dependency interruption");
    }
};
std::unique_ptr<State> seed(unsigned index) {
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);s->memory.map(fixture,0x60000);s->memory.map(stack-0x4000,0x5000);s->memory.map(toc-0x8000,0x9000);
    s->pc=reporting::entries[index];s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->memory.write<4>(stack+0x14,toc);
    for(auto [slot,pointer]:{std::pair{-0x5438,strings},std::pair{-0x7ba8,objects},std::pair{-0x7bac,objects+0x100},std::pair{-0x7be4,objects+0x200},std::pair{-0x7ba0,objects+0x204},std::pair{-0x7be8,objects+0x208},std::pair{-0x7bbc,objects+0x20c},std::pair{-0x7b34,objects+0x210},std::pair{-0x7b4c,objects+0x214},std::pair{-0x7be0,objects+0x218},std::pair{-0x7bb8,objects+0x1000}})s->memory.write<4>(toc+slot,pointer);
    for(auto [pointer,value]:{std::pair{objects+0x200,16u},std::pair{objects+0x204,manager},std::pair{objects+0x208,clubs},std::pair{objects+0x20c,competitions},std::pair{objects+0x210,objects+0x300},std::pair{objects+0x214,objects+0x400},std::pair{objects+0x218,staff}})s->memory.write<4>(pointer,value);
    s->memory.write<4>(manager,rows);s->memory.write<4>(manager+4,injuries);s->memory.write<4>(manager+12,objects+0x500);s->memory.write<4>(staff,0);s->memory.write<4>(staff+0x14,objects+0x600);s->memory.write<4>(rows+0x10,0);s->memory.write<4>(rows+0x18,0x12345678);s->memory.write<4>(rows+0x1c,0x87654321);
    s->memory.write<1>(injuries,81);s->memory.write<4>(injuries+12,0);s->memory.write<4>(injuries+0x18,0);s->memory.write<4>(injuries+0x1c,1);s->memory.write<1>(injuries+0x20,2);s->memory.write<1>(injuries+0x21,1);s->memory.write<4>(injuries+0x24,0);s->memory.write<2>(injuries+4,194);s->memory.write<2>(injuries+6,2001);
    injury_reporting_fixture_details(*s,toc,details);
    for(auto [n,value]:{std::pair{0u,0u},std::pair{1u,11u},std::pair{2u,0u},std::pair{3u,objects+0x700},std::pair{4u,objects+0x800},std::pair{5u,objects+0x900},std::pair{6u,194u},std::pair{7u,2001u},std::pair{8u,12u},std::pair{9u,0u},std::pair{10u,0u}})s->memory.write<4>(message+8+n*4,value);
    s->wr<4>(12,manager);s->wr<4>(16,index==1?message:index==2?injuries:staff);s->wr<4>(20,index==1?1u:output);s->wr<4>(24,output);return s;
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
void status_checks() {
    boundaries(*seed(0));
    for(auto pointer:{0u,staff})for(auto id:{0u,15u,16u,0x7fffffffu,0x80000000u,0xffffffffu}) {
        auto s=seed(0);s->wr<4>(16,pointer);s->memory.write<4>(staff,id);boundaries(*s,{},false);
    }
    for(auto record:{0u,1u,0xffffffffu})for(auto type:{0u,80u,81u,127u,128u,255u}) {
        auto s=seed(0);s->memory.write<4>(rows+0x10,record);s->memory.write<1>(injuries+0x38,type);s->memory.write<1>(injuries,type);
        auto r=compare(*s);require(r.state->r<4>(12)==unsigned(record!=0xffffffff && type==81),"Independent type-81 predicate");boundaries(*s,{},false);
    }
    Options mutation;mutation.refresh_mutate=true;auto s=seed(0);auto r=compare(*s,mutation);require(r.state->r<4>(12)==0,"Special predicate reloads callback-mutated current injury");boundaries(*s,mutation);
    Options structural_control;structural_control.force_pc=0x1018f3cc;structural_control.force_cr=2;boundaries(*seed(0),structural_control);
}
void notice_checks() {
    boundaries(*seed(1));
    for(auto phase:{0u,1u,255u,256u})for(auto style:{0u,1u,128u,256u})for(auto type:{0u,1u,2u,3u,11u,48u,77u,78u,79u,80u,81u,127u,128u,255u,256u})for(auto action:{0u,2u})for(auto reaction:{0u,1u,2u,3u,0xffffffffu}) {
        auto s=seed(1);s->memory.write<4>(message+8,phase);s->memory.write<4>(message+12,type);s->wr<4>(20,style);s->memory.write<4>(message+8+9*4,action);s->memory.write<4>(message+8+10*4,reaction);compare(*s);
    }
    for(auto phase:{0u,1u})for(auto category:{0u,1u,6u,12u})for(auto type:{0u,1u,2u,11u,77u,78u,79u,80u,81u})for(auto action:{0u,2u})for(auto reaction:{0u,1u,2u,3u,0xffffffffu}) {
        auto s=seed(1);s->memory.write<4>(message+8,phase);s->memory.write<4>(message+12,type);s->memory.write<1>(details+type*12+1,category);s->memory.write<4>(message+8+9*4,action);s->memory.write<4>(message+8+10*4,reaction);compare(*s);
    }
    for(auto phase:{0u,1u})for(auto type:{0u,11u,77u,78u,79u,80u,81u})for(auto style:{0u,1u}) {
        auto s=seed(1);s->memory.write<4>(message+8,phase);s->memory.write<4>(message+12,type);s->wr<4>(20,style);boundaries(*s,{},false);
    }
    for(auto link:{0u,1u,128u,255u,256u}){Options o;o.link=link;boundaries(*seed(1),o,false);}
    for(auto day:{0u,32767u,32768u,65535u,65536u})for(auto year:{0u,65535u,65536u}) {
        auto s=seed(1);s->memory.write<4>(message+8+6*4,day);s->memory.write<4>(message+8+7*4,year);auto r=compare(*s);
        const auto call=std::find_if(r.calls.begin(),r.calls.end(),[](const Event& e){return e.target==0x107156b0;});require(call!=r.calls.end(),"Original date constructor receives notice date");
        const auto expected=std::uint32_t(signed_value<2>(day));require(read_be<4>(call->before.data()+16)==expected && read_be<4>(call->before.data()+20)==(year&0xffff),"Independent signed-day/unsigned-year ABI");
    }
    for(auto [phase,style,type,reaction,token]:{std::array<std::uint32_t,5>{0,0,11,0,0x9e0},{0,1,0,0,0xd91},{1,0,81,3,0xfd2},{1,1,81,2,0x1057}}) {
        auto s=seed(1);s->memory.write<4>(message+8,phase);s->memory.write<4>(message+12,type);s->memory.write<4>(message+8+10*4,reaction);s->wr<4>(20,style);auto r=compare(*s);
        require(std::any_of(r.calls.begin(),r.calls.end(),[&](const Event& e){return e.target==0x10727890 && read_be<4>(e.before.data()+16)==strings+token;}),"Independent notice token choice");
    }
    Options mutation;mutation.argument_mutate=true;boundaries(*seed(1),mutation);
    for(auto address:{output,stack-32,message,details}){auto s=seed(1);s->wr<4>(24,address);boundaries(*s,{},false);}
}
void context_checks() {
    boundaries(*seed(2));
    for(auto first:{0u,1u,127u,128u,255u})for(auto second:{0u,1u,127u,128u,255u})for(auto competition:{0u,1u,128u,255u,256u})for(auto match:{0u,1u,128u,256u}) {
        auto s=seed(2);s->memory.write<1>(injuries+0x20,first);s->memory.write<1>(injuries+0x21,second);Options o;o.competition=competition;o.match=match;auto r=compare(*s,o);
        const auto a=signed_value<1>(first),b=signed_value<1>(second);const auto token=a>b?0x1c17:a<b?0x1c87:0x1cfe;
        require(std::any_of(r.calls.begin(),r.calls.end(),[&](const Event& e){return e.target==0x10727890 && read_be<4>(e.before.data()+16)==strings+token;}),"Independent signed-score text choice");
    }
    for(auto [first,second]:{std::pair{1u,2u},std::pair{1u,1u},std::pair{255u,128u}}){auto s=seed(2);s->memory.write<1>(injuries+0x20,first);s->memory.write<1>(injuries+0x21,second);boundaries(*s);}
    for(auto address:{output,stack-32,injuries,clubs}){auto s=seed(2);s->wr<4>(20,address);boundaries(*s,{},false);}
}
void history_checks() {
    boundaries(*seed(3));auto r=compare(*seed(3));require(r.state->memory.read<4>(rows+0x18)==0x12345678 && r.state->memory.read<4>(rows+0x1c)==0xffffffff,"Independent invalidated history tail with preserved start offset");
    for(auto id:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu}){auto s=seed(3);s->memory.write<4>(staff,id);boundaries(*s,{},false);}
    auto s=seed(3);s->wr<4>(16,0);boundaries(*s);Options o;o.fatal_exit=true;boundaries(*s,o);
}
void structural_checks() {
    for(unsigned index=0;index<4;++index){auto s=seed(index);constexpr auto moved=toc+0x10000;s->memory.map(moved-0x8000,0x9000);s->memory.copy_in(moved-0x8000,s->memory.copy_out(toc-0x8000,0x9000));s->wr<4>(8,moved);boundaries(*s,{},false);}
}
}
namespace cm::aot {void injury_reporting_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try {
    const auto mode=argc==2?std::string_view(argv[1]):std::string_view{};
    if(mode.empty()||mode=="--status")status_checks();if(mode.empty()||mode=="--notice")notice_checks();if(mode.empty()||mode=="--context")context_checks();if(mode.empty()||mode=="--history")history_checks();if(mode.empty())structural_checks();
    if(mode.empty())for(unsigned index=0;index<4;++index){Options o;o.own_negative=true;compare(*seed(index),o);o.own_negative=false;o.child_negative=true;compare(*seed(index),o);}
    for(auto [start,end]:ranges)for(auto pc=start;pc<end;pc+=4)if(!sites.contains(pc))std::cerr<<"Uncovered injury reporting "<<hex_address(pc)<<std::endl;
    if(mode.empty())require(sites.size()==749,"Complete injury reporting source coverage");
    std::cout<<"Reporting comparisons="<<cases<<" completions="<<completed<<" interruptions="<<interruptions<<" negatives="<<negatives<<" structural="<<structural<<" sites="<<sites.size()<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
