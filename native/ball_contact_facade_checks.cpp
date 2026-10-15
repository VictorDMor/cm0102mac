#include "ball_contact.hpp"
#include "semantic_io_checks.hpp"
#include <set>

namespace cm::aot {bool ball_contact_facades_reference(State&);}
namespace {
using namespace cm::semantic_io_checks;
namespace contact=cm::aot::ball_contact;
constexpr std::array<std::uint32_t,4> ends{0x101f50b8,0x101f5110,0x101f5180,0x101f51e0};
constexpr std::array<std::uint32_t,4> children{0x1021c260,0x1021b860,0x10221e00,0x1021bec0};
constexpr std::uint32_t contact_stack=0x70008000,match=fixture+0x1000,output=fixture+0x80;
struct Trace final:SemanticGuestObserver {
    std::vector<std::uint32_t> sites;
    void before_step(State&,std::uint32_t pc)override{sites.push_back(pc);}
};
thread_local Trace* original_trace=nullptr;
struct Event {std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Options {bool own_negative=false,child_negative=false,interrupt=false;std::uint32_t write_address=output;};
struct Model final:SemanticDependencyDispatch {
    bool original;unsigned index;Options options;std::vector<Event> calls;
    Model(bool old,unsigned i,Options o):original(old),index(i),options(o){}
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override {
        require(target==children[index],"Unknown contact facade dependency");Event e{target,continuation,s.registers,{}};
        s.memory.write<4>(options.write_address,0x11223344^unsigned(options.child_negative&&!original));
        for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xac000000+n*17);
        for(unsigned n=0;n<=13;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-11.0-n));
        s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,0xbeef0123);s.pc=continuation;e.after=s.registers;calls.push_back(e);
        if(options.interrupt)throw std::runtime_error("Fixture contact facade interruption");
    }
    void reference(State& s) {
        for(;;) {
            if(s.pc>=contact::facade_entries[index]&&s.pc<ends[index])require(ball_contact_facades_reference(s),"Missing independent contact facade reference");
            else if(s.pc==children[index])call(s,s.pc,std::uint32_t(s.r<4>(4128)));
            else return;
        }
    }
};
std::unique_ptr<State> seed(unsigned index) {
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);s->memory.map(fixture,0x12000);s->memory.map(contact_stack-0x200,0x500);
    s->wr<4>(4,contact_stack);s->wr<4>(8,0x108bba90);s->wr<4>(4128,return_sentinel);s->pc=contact::facade_entries[index];
    for(unsigned n=3;n<=10;++n)s->wr<4>(n*4,0x61000000+n*0x101);s->wr<4>(12,match);
    s->memory.write<1>(contact_stack+0x3b,0xed);return s;
}
unsigned cases=0,completions=0,interruptions=0,negatives=0;
std::set<std::uint32_t> sites;
struct Result {std::unique_ptr<State> state;std::vector<Effects::Access> accesses;std::vector<Event> calls;};
Result compare(const State& input,unsigned index,Options options={}) {
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    Trace old_trace,new_trace;Model old(true,index,options),modern(false,index,options);
    const auto run=[&](State& s,Model& m,bool reference){try{
        if(reference){original_trace=&old_trace;m.reference(s);}
        else {constexpr std::array methods{contact::screen,contact::attempt,contact::apply,contact::resolve};methods[index](s,&m,&new_trace);}
        original_trace=nullptr;return std::string{};
    }catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="Contact facade "+hex_address(contact::facade_entries[index])+" case="+std::to_string(cases);
    require(!expected.starts_with("Unknown contact"),expected);require(expected==actual,label+" original="+expected+" modern="+actual);
    if(options.own_negative)b->wr<4>(52,b->r<4>(52)^1);
    bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative&&!options.child_negative)throw;rejected=true;}
    require(rejected==(options.own_negative||options.child_negative),label+" discriminating own/child negative");
    if(!rejected){require(x.bytes==y.bytes,label+" ordered memory effects");require(a->executed==b->executed&&old_trace.sites==new_trace.sites,label+" original-site order/accounting");require(old.calls==modern.calls,label+" complete dependency ABI");}
    ++cases;completions+=expected.empty()&&!rejected;interruptions+=!expected.empty();negatives+=rejected;sites.insert(old_trace.sites.begin(),old_trace.sites.end());
    return {std::move(a),x.log,old.calls};
}
void boundaries(const State& input,unsigned index,Options options={}) {
    const auto baseline=compare(input,index,options);const auto steps=baseline.state->executed-input.executed;
    for(std::uint64_t n=0;n<=steps;++n){auto s=clone(input);s->budget=s->executed+n;compare(*s,index,options);}
    std::set<std::uint32_t> bytes;for(const auto& a:baseline.accesses)for(unsigned n=0;n<a.size;++n)bytes.insert(a.address+n);
    for(auto hole:bytes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;
        for(auto b:bytes)if(b!=hole){s->memory.map(b,1);s->memory.write<1>(b,input.memory.read<1>(b));}compare(*s,index,options);}
    options.interrupt=true;compare(input,index,options);
}
}
namespace cm::aot {void ball_contact_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(){try {
    for(unsigned index=0;index<contact::facade_entries.size();++index) {
        auto initial=seed(index);auto r=compare(*initial,index);require(r.state->pc==return_sentinel&&r.state->r<4>(4)==contact_stack&&r.state->r<4>(12)==0xbeef0123,"Facade returns the complete child result and original frame");
        require(r.calls.size()==1&&r.calls[0].target==children[index]&&read_be<4>(r.calls[0].before.data()+12)==match+0xe030,"Facade selects its exact match-context subobject and dependency");
        for(unsigned n=4;n<=10;++n){auto expected=initial->r<4>(n*4);const bool narrowed=(index==0&&(n==6||n==7))||(index==2&&(n==7||n==8||n==9))||(index==3&&(n==6||n==7||n==8));
            if(narrowed)expected&=255;if(index==1&&n==5)expected=std::uint32_t(signed_value<2>(expected));
            if((index==1&&n>7)||(index==3&&n>9))continue;require(read_be<4>(r.calls[0].before.data()+n*4)==expected,"Independent facade argument narrowing and full-word forwarding");}
        if(index==2)require(r.state->memory.read<4>(contact_stack-0x40+0x38)==0xed,"Ninth byte argument becomes the exact outgoing contact_stack word");boundaries(*initial,index);
        for(unsigned reg=3;reg<=10;++reg)for(auto value:{0u,1u,127u,128u,255u,256u,32767u,32768u,65535u,65536u,0x80000000u,0xffffffffu}){auto s=seed(index);s->wr<4>(reg*4,value);compare(*s,index);}
        for(unsigned byte=0;byte<256;++byte){auto s=seed(index);for(unsigned n=5;n<=10;++n)s->wr<4>(n*4,0xabcd0000|byte);s->memory.write<1>(contact_stack+0x3b,byte);compare(*s,index);}
        for(auto sp:{contact_stack+1,contact_stack+8,output+0x40,match+0x40}){auto s=seed(index);s->wr<4>(4,sp);compare(*s,index);}
        for(auto at:{output,contact_stack+8,contact_stack+0x18,contact_stack-8}){Options o;o.write_address=at;boundaries(*initial,index,o);}
        for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*initial,index,o);}
    }
    require(sites.size()==98,"All 98 original facade instruction sites covered");
    std::cout<<"PASS: "<<cases<<" contact-facade comparisons; "<<completions<<" completions, "<<interruptions<<" interruptions, "<<negatives<<" own/child negatives; all "<<sites.size()<<" original sites; full state, ordered effects, ABI and accounting; production unchanged\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
