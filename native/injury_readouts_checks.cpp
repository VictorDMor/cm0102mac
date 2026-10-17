#include "injury_readouts.hpp"
#include "semantic_io_checks.hpp"
#include <set>

namespace cm::aot {bool injury_readouts_reference(State&);void injury_readouts_fixture_tables(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace readouts=cm::aot::injury_readouts;
constexpr std::uint32_t manager=fixture+0x1000,staff=fixture+0x2000,rows=fixture+0x4000,injuries=fixture+0x8000;
constexpr std::uint32_t objects=fixture+0xc000,output=fixture+0xe000,strings=fixture+0x10000,tables=fixture+0x20000,date=fixture+0x24000;
constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,8> ranges{{{0x1018c1a0,0x1018c26c},{0x1018c270,0x1018c2e4},{0x1018c2f0,0x1018c33c},{0x1018c450,0x1018c560},{0x1018c560,0x1018d140},{0x1018d140,0x1018d94c},{0x1018d950,0x1018daa8},{0x1018dab0,0x1018dcb0}}};
struct Options {
    unsigned interrupt=0;std::vector<unsigned> modes{0};std::uint32_t history=objects+0x500,day=7,year=0;
    bool own_negative=false,child_negative=false,refresh_mutate=false,mode_mutate=false,nested=true;
    std::uint32_t force_pc=0,force_value=0;unsigned force_register=0;int force_cr=-1;
};
struct Trace final:SemanticGuestObserver {
    Options options;std::vector<std::uint32_t> sites;
    explicit Trace(Options value):options(std::move(value)){}
    void before_step(State& s,std::uint32_t pc)override {
        sites.push_back(pc);if(pc!=options.force_pc)return;s.wr<4>(options.force_register*4,options.force_value);
        if(options.force_cr>=0)s.wr<1>(2304,(s.r<1>(2304)&0xf0)|unsigned(options.force_cr)|(s.r<1>(1024)&1));
    }
};
thread_local Trace* original_trace=nullptr;
struct Event {std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
void modern_root(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
    constexpr Function methods[]{readouts::find_history_record,readouts::risk_multiplier,readouts::risk_type,readouts::describe_current_injury,readouts::describe_injury_type,readouts::injury_type_form,readouts::describe_remaining_time,readouts::describe_injury_category};
    const auto entry=std::find(readouts::entries.begin(),readouts::entries.end(),s.pc);require(entry!=readouts::entries.end(),"Known injury readout root");methods[entry-readouts::entries.begin()](s,d,o);
}
struct Model final:SemanticDependencyDispatch {
    bool original;Options options;Trace& trace;std::vector<Event> calls;unsigned mode_queries=0;
    Model(bool old,Options value,Trace& observer):original(old),options(std::move(value)),trace(observer){}
    void reference(State& s,std::uint32_t stop=return_sentinel) {
        const auto entry=s.pc;const auto range=std::find_if(ranges.begin(),ranges.end(),[&](auto r){return r.first==entry;});require(range!=ranges.end(),"Known original injury readout");
        while(s.pc!=stop){if(s.pc>=entry && s.pc<range->second)require(injury_readouts_reference(s),"Complete injury readout reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));}
    }
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override {
        Event event{target,continuation,s.registers,{}};const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};
        if(target==readouts::type_text_entry && options.nested){if(original)reference(s,continuation);else modern_root(s,this,&trace);}
        else {
            constexpr std::array<std::uint32_t,9> allowed{0x10749fd0,0x10739890,0x10768810,0x1077b720,0x10769b60,0x10187a40,0x10727890,0x10727880,0x10717dd0};
            require(target==0x1030e880 || target==readouts::type_text_entry || std::find(allowed.begin(),allowed.end(),target)!=allowed.end(),"Known injury readout dependency "+hex_address(target));
            s.memory.write<4>(fixture+0x50000+unsigned(calls.size())*4,target^arg(3));std::uint32_t result=0;
            if(target==0x10749fd0)result=options.history;
            if(target==0x10727890){s.memory.write<4>(arg(3),arg(4));s.memory.write<4>(arg(3)+4,arg(5));result=9;}
            if(target==0x1030e880){s.memory.write<4>(arg(4),arg(3));s.memory.write<4>(arg(4)+4,arg(5));result=8;}
            if(target==0x10717dd0){s.memory.write<2>(arg(3),options.day);s.memory.write<2>(arg(3)+2,options.year);s.memory.write<1>(arg(3)+4,0);}
            if(target==0x10187a40 && options.refresh_mutate){s.memory.write<4>(manager,rows+0x1000);s.memory.write<4>(rows+0x1010,1);s.memory.write<4>(manager+4,injuries+0x1000);s.memory.write<1>(injuries+0x1038,2);s.memory.write<2>(injuries+0x1038+0x32,21);}
            if(target==0x10727880){result=options.modes[std::min<std::size_t>(mode_queries++,options.modes.size()-1)];if(options.mode_mutate){const auto base=s.memory.read<4>(s.r<4>(8)-0x5464);s.memory.write<4>(base,s.memory.read<4>(base+4));}}
            for(unsigned n=0;n<=12;++n)if(n!=1 && n!=2)s.wr<4>(n*4,0xac000000+n*17);
            for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-20.0-n));
            s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=continuation;
        }
        if(options.child_negative && !original && target!=readouts::type_text_entry)s.wr<4>(12,s.r<4>(12)^1);
        event.after=s.registers;calls.push_back(event);if(options.interrupt && calls.size()>=options.interrupt)throw std::runtime_error("Fixture injury readout dependency interruption");
    }
};
std::unique_ptr<State> seed(unsigned index) {
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);s->memory.map(fixture,0x60000);s->memory.map(stack-0x4000,0x5000);s->memory.map(toc-0x8000,0x9000);
    s->pc=readouts::entries[index];s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->memory.write<4>(stack+0x14,toc);
    for(auto [slot,pointer]:{std::pair{-0x5438,strings},std::pair{-0x7ba8,objects},std::pair{-0x7bac,objects+0x100},std::pair{-0x7be4,objects+0x200},std::pair{-0x7b80,date}})s->memory.write<4>(toc+slot,pointer);
    s->memory.write<4>(objects+0x200,16);s->memory.write<4>(manager,rows);s->memory.write<4>(manager+4,injuries);s->memory.write<4>(manager+12,objects+0x300);s->memory.write<4>(staff,0);s->memory.write<4>(staff+0x14,objects+0x400);
    s->memory.write<4>(rows+0x10,0);s->memory.write<4>(rows+0x18,0x12345678);s->memory.write<4>(rows+0x1c,0x87654321);s->memory.write<1>(rows+0x14,77);s->memory.write<1>(rows+0x15,3);
    s->memory.write<1>(injuries,1);s->memory.write<2>(injuries+0x30,12);s->memory.write<2>(injuries+0x32,20);s->memory.write<2>(date,20);s->memory.write<2>(date+2,2001);
    injury_readouts_fixture_tables(*s,toc,tables);
    s->wr<4>(12,manager);s->wr<4>(16,staff);s->wr<4>(20,output);s->wr<4>(24,80);s->wr<4>(28,0);s->wr<4>(32,0);
    if(index==4 || index==5 || index==7)s->wr<4>(16,0);
    return s;
}
unsigned cases=0,completed=0,interruptions=0,negatives=0,structural=0;std::set<std::uint32_t> sites;
struct Result {std::unique_ptr<State> state;std::vector<Effects::Access> accesses;std::vector<Event> calls;};
Result compare(const State& input,Options options={}) {
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    Trace old_trace(options),new_trace(options);Model old(true,options,old_trace),modern(false,options,new_trace);
    const auto run=[&](State& s,Model& m,bool reference){try{if(reference){original_trace=&old_trace;m.reference(s);}else modern_root(s,&m,&new_trace);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="Injury readout entry="+hex_address(input.pc)+" case="+std::to_string(cases);
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
void query_checks() {
    for(unsigned index:{0u,1u,2u,3u,6u}) {
        boundaries(*seed(index));
        for(auto pointer:{0u,staff})for(auto id:{0u,15u,16u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto profile:{0u,objects+0x400}) {
            auto s=seed(index);s->wr<4>(16,pointer);s->memory.write<4>(staff,id);s->memory.write<4>(staff+0x14,profile);boundaries(*s,{},false);
        }
    }
    for(auto type:{0u,1u,77u,127u,128u,255u})for(auto count:{0u,1u,2u,127u,128u,255u}) {
        for(unsigned index:{1u,2u}){auto s=seed(index);s->memory.write<1>(rows+0x14,type);s->memory.write<1>(rows+0x15,count);auto r=compare(*s);const auto expected=index==2?type:type>=128?0u:count>1 && count<128?count:1u;require(r.state->r<4>(12)==expected,"Independent risk type/multiplier");}
    }
    for(auto pointer:{0u,objects+0x300})for(auto value:{0u,1u,0x81234567u,0xffffffffu}){auto s=seed(0);s->memory.write<4>(manager+12,pointer);Options o;o.history=value;auto r=compare(*s,o);require(r.state->r<4>(12)==(pointer?value:0),"Independent history handle");boundaries(*s,o,false);}
    for(unsigned index:{3u,6u})for(auto record:{0u,1u,0xffffffffu}){auto s=seed(index);s->memory.write<4>(rows+0x10,record);boundaries(*s);Options o;o.refresh_mutate=true;boundaries(*s,o,false);}
    for(auto selector:{0u,1u,128u,256u})for(auto day:{0u,7u,32767u,32768u,65535u})for(auto year:{0u,1u,65535u}) {
        auto s=seed(6);s->wr<4>(28,selector);Options o;o.day=day;o.year=year;auto r=compare(*s,o);const auto signed_day=day<32768?int(day):int(day)-65536;const auto expected=std::uint32_t((selector&255)?12:20)-std::uint32_t(signed_day)-year*365u;require(r.state->memory.read<4>(output)==expected,"Independent remaining-day arithmetic");
    }
}
void label_checks() {
    for(auto mode:{0u,1u,3u,5u,9u,255u})for(auto style:{0u,1u,2u,128u,256u})for(unsigned type=0;type<=82;++type){auto s=seed(4);s->wr<4>(16,type);s->wr<4>(28,style);Options o;o.modes={mode};compare(*s,o);}
    for(unsigned index:{4u,7u})for(auto length:{0u,19u,20u,29u,30u,31u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto type:{0u,12u,13u,76u,77u,81u,82u,127u,128u,255u,256u}) {
        auto s=seed(index);s->wr<4>(16,type);s->wr<4>(24,length);boundaries(*s,{},false);
    }
    for(auto mode:{0u,3u,128u,255u})for(unsigned type=0;type<13;++type){auto s=seed(7);s->wr<4>(16,type);Options o;o.modes={mode};auto r=compare(*s,o);require(r.state->r<4>(12)==(mode==3?2:0),"Independent category form");}
    {auto s=seed(4);auto r=compare(*s);require(r.state->memory.read<4>(output)==strings+0xf7,"Independent first injury label token");Options o;o.mode_mutate=true;o.modes={3};s->wr<4>(28,1);boundaries(*s,o);}
    for(unsigned index:{4u,7u})for(auto address:{output,stack-32,strings}){auto s=seed(index);s->wr<4>(20,address);boundaries(*s);}
}
void form_checks() {
    for(auto mode:{0u,3u,5u,9u,128u,255u})for(unsigned type=0;type<256;++type){auto s=seed(5);s->wr<4>(16,type);Options o;o.modes={mode};compare(*s,o);}
    for(auto modes:{std::vector<unsigned>{0,3},std::vector<unsigned>{0,0,5},std::vector<unsigned>{0,0,0},std::vector<unsigned>{9}}){Options o;o.modes=modes;boundaries(*seed(5),o);}
    for(unsigned index:{4u,5u,7u}){
        auto s=seed(index);const std::uint32_t slot=index==4?0xffffab9cu:index==5?0xffffab94u:0xffffab88u;const auto base=s->memory.read<4>(toc+slot);
        const auto first=s->memory.read<4>(base);s->memory.write<4>(base,s->memory.read<4>(base+4));Options o;if(index==5)o.modes={9};boundaries(*s,o);
        for(unsigned displacement:{4u,8u,12u}){if(index==5 && displacement>4)continue;auto x=seed(index);const auto b=x->memory.read<4>(toc+slot);x->memory.write<4>(b,first+displacement);boundaries(*x,o);}
    }
}
void structural_checks() {
    for(auto [index,pc]:{std::pair{1u,0x1018c278u},std::pair{2u,0x1018c2f8u},std::pair{3u,0x1018c4d8u},std::pair{6u,0x1018d9e0u}}){Options o;o.force_pc=pc;o.force_cr=2;boundaries(*seed(index),o);}
    for(unsigned index=0;index<8;++index){auto s=seed(index);constexpr auto moved=toc+0x10000;s->memory.map(moved-0x8000,0x9000);s->memory.copy_in(moved-0x8000,s->memory.copy_out(toc-0x8000,0x9000));s->wr<4>(8,moved);boundaries(*s,{},false);}
}
}
namespace cm::aot {void injury_readouts_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try {
    const auto mode=argc==2?std::string_view(argv[1]):std::string_view{};
    if(mode.empty()||mode=="--queries")query_checks();if(mode.empty()||mode=="--labels")label_checks();if(mode.empty()||mode=="--forms")form_checks();if(mode.empty()||mode=="--structural")structural_checks();
    if(mode.empty())for(unsigned index=0;index<8;++index){Options o;o.own_negative=true;compare(*seed(index),o);if(index!=1 && index!=2){o.own_negative=false;o.child_negative=true;if(index==5)o.modes={9};compare(*seed(index),o);}}
    for(auto [start,end]:ranges)for(auto pc=start;pc<end;pc+=4)if(!sites.contains(pc))std::cerr<<"Uncovered injury readout "<<hex_address(pc)<<std::endl;
    if(mode.empty())require(sites.size()==1656,"Complete injury readout source coverage");
    std::cout<<"Readout comparisons="<<cases<<" completions="<<completed<<" interruptions="<<interruptions<<" negatives="<<negatives<<" structural="<<structural<<" sites="<<sites.size()<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
