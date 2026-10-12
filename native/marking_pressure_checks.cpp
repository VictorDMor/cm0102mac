#include "marking_pressure.hpp"
#include "semantic_io_checks.hpp"
#include "spatial_role.hpp"
#include <set>

namespace cm::aot { bool marking_pressure_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
namespace marking=cm::aot::marking_pressure;
constexpr auto match=fixture+0x1000, actor=match+0x311c+8*0x1c4, opposite=match+0x546c;
constexpr auto center=match+0x2eb0, sum=fixture+0x20000, collection=fixture+0x21000, doubles=fixture+0x23000;
struct Options{std::uint32_t tactical=0,role=4;unsigned fault_at=0;bool negative=false,child_negative=false;};
struct Call{std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Call&)const=default;};
struct Append{std::uint32_t collection,actor,length;std::vector<std::uint8_t> payload;bool operator==(const Append&)const=default;};
struct Model final:marking::DependencyDispatch{
    Options options;bool old;std::vector<Call> calls;std::vector<Append> appended;
    Model(Options o,bool original):options(o),old(original){}
    void reference(State& s,std::uint32_t end){bool first=true;while(s.pc!=end){
        const bool child=!first && std::find(marking::entries.begin(),marking::entries.end(),s.pc)!=marking::entries.end();
        if(child || !marking_pressure_reference(s))call(s,s.pc,std::uint32_t(s.r<4>(4128)));first=false;
    }}
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override{
        const auto number=calls.size();calls.push_back({target,continuation,s.registers,{}});
        const auto found=std::find(marking::entries.begin(),marking::entries.end(),target);
        if(found!=marking::entries.end()){
            const auto entry=marking::Entry(found-marking::entries.begin());if(old)reference(s,continuation);else marking::invoke(s,entry,this);
            if(!old && options.child_negative && entry==marking::Entry::trait_10d)s.wr<4>(12,s.r<4>(12)^1);
        }else if(target==0x101d5680 || target==0x101d5690){cm::aot::spatial_role::invoke(s,target==0x101d5680?cm::aot::spatial_role::Entry::column:cm::aot::spatial_role::Entry::row);}
        else{
            require(target==0x101149f0 || target==0x10289560 || target==0x10738d10,"Unknown marking fixture dependency "+hex_address(target));
            if(target==0x10738d10){
                const auto dest=std::uint32_t(s.r<4>(12)),source=std::uint32_t(s.r<4>(16)),length=std::uint32_t(s.r<4>(20));
                appended.push_back({dest,source,length,s.memory.copy_out(source,length)});const auto count=s.memory.read<2>(dest);
                s.memory.write<2>(dest,count+1);s.memory.write<4>(dest+0x10+count*4,source);
            }
            for(unsigned n=3;n<=12;++n)s.wr<4>(n*4,0x0bad0000+n*37);
            s.wr<4>(0,0xbeef0055);s.wr<4>(12,target==0x101149f0?options.tactical:target==0x10289560?options.role:0x12345678);s.pc=continuation;
        }
        calls[number].after=s.registers;if(options.fault_at && calls.size()>=options.fault_at){s.pc=target;throw std::runtime_error("Fixture marking dependency interruption");}
    }
};
std::unique_ptr<State> seed(marking::Entry entry){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned o=1280;o<1312;++o)s->wr<1>(o,0);
    s->memory.map(fixture,0x50000);s->memory.map(stack-0x300,0x400);s->memory.map(toc-0x8000,0x4000);
    s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->pc=marking::entries[unsigned(entry)];s->wr<4>(12,actor);s->wr<4>(16,opposite);
    if(entry==marking::Entry::set_pressure_adjustment)s->wr<4>(16,20);
    if(entry==marking::Entry::assess_cell){s->wr<4>(12,match);s->wr<4>(16,actor);s->wr<4>(20,sum);s->wr<4>(24,collection);}
    if(entry==marking::Entry::eligible_marker){s->wr<4>(12,match);s->wr<4>(16,actor);s->wr<4>(20,7);s->wr<4>(24,6);}
    s->memory.write<4>(actor+0x1a4,match);s->memory.write<1>(actor+0x19,8);s->memory.write<1>(actor+0x27,0);s->memory.write<1>(actor+0x105,8);s->memory.write<1>(actor+0x106,6);
    s->memory.write<1>(actor+0x104,1);s->memory.write<4>(actor+0x1b4,opposite);s->memory.write<1>(actor+0x10a,3);s->memory.write<2>(actor+0x1a0,20);
    s->memory.write<1>(actor+0x10b,10);s->memory.write<1>(actor+0x10d,8);s->memory.write<2>(actor+0x19c,0x100);
    for(unsigned index=0;index<4;++index){const auto p=opposite+index*0x1c4;
        s->memory.write<1>(p+0x19,index+2);s->memory.write<1>(p+0x27,1);s->memory.write<4>(p+0x1a4,match);
        s->memory.write<1>(p+0x10d,std::array<unsigned,4>{12,25,30,5}[index]);s->memory.write<1>(p+0x10b,std::array<unsigned,4>{8,15,25,1}[index]);
        s->memory.write<1>(p+0x2c,index==1?1:0);s->memory.write<2>(p+0x19c,0x100);s->memory.write<4>(center+0x2c+index*4,p);
    }
    s->memory.write<1>(center+0x59,3);s->memory.write<4>(toc-0x52c4,doubles);s->memory.write<8>(doubles+0x60,float_bits<8>(4503601774854144.0));s->memory.write<8>(doubles+0xb8,float_bits<8>(1.25));
    return s;
}
unsigned comparisons=0,normal=0,interruptions=0,negatives=0;std::array<unsigned,13> normal_entries{};
std::vector<Effects::Access> compare(const State& input,marking::Entry entry,const Options& options={}){
    auto original=clone(input),modern=clone(input);Model old(options,true),next(options,false);Effects x,y;x.state=original.get();y.state=modern.get();original->memory.date_observer=&x;modern->memory.date_observer=&y;
    auto run=[&](State& s,Model& model,bool previous){try{if(previous)model.reference(s,return_sentinel);else marking::invoke(s,entry,&model);}catch(const std::exception& e){return std::string(e.what());}return std::string{};};
    const auto expected=run(*original,old,true),actual=run(*modern,next,false);original->memory.date_observer=modern->memory.date_observer=nullptr;
    const auto label="marking-pressure component "+std::to_string(comparisons)+" root="+hex_address(marking::entries[unsigned(entry)]);
    require(!expected.starts_with("Unknown marking fixture"),label+" incomplete model "+expected);require(expected==actual,label+" original="+expected+" modern="+actual);
    if(options.negative)modern->wr<4>(12,modern->r<4>(12)^1);
    bool rejected=false;try{Image(*original).compare(*modern,label);}catch(const std::runtime_error&){if(!options.negative&&!options.child_negative)throw;rejected=true;}
    require(rejected==(options.negative||options.child_negative),label+" observable own/child negative");
    if(!rejected){require(x.bytes==y.bytes,label+" ordered effects");require(old.appended==next.appended,label+" append payloads and selection order");
        if(old.calls!=next.calls){for(unsigned n=0;n<std::min(old.calls.size(),next.calls.size());++n)if(old.calls[n]!=next.calls[n]){const auto&a=old.calls[n];const auto&b=next.calls[n];const auto p=std::mismatch(a.before.begin(),a.before.end(),b.before.begin());throw std::runtime_error(label+" dependency "+std::to_string(n)+" "+hex_address(a.target)+"/"+hex_address(b.target)+" continue="+hex_address(a.continuation)+"/"+hex_address(b.continuation)+" register-byte="+std::to_string(p.first-a.before.begin()));}throw std::runtime_error(label+" dependency count");}
        require(original->executed==modern->executed,label+" accounting "+std::to_string(original->executed)+"/"+std::to_string(modern->executed));}
    ++comparisons;negatives+=rejected;interruptions+=!expected.empty();if(expected.empty()&&!rejected){++normal;++normal_entries[unsigned(entry)];}return x.log;
}
}
int main(){try{
    {
        auto input=seed(marking::Entry::assess_cell);Model dependencies({},false);
        marking::invoke(*input,marking::Entry::assess_cell,&dependencies);
        require(input->r<4>(12)==4 && input->memory.read<2>(sum)==65,"Independent marked/ordinary count and pressure expectation");
        require(dependencies.appended.size()==2 && dependencies.appended[0].actor==opposite &&
            dependencies.appended[1].actor==opposite+2*0x1c4 && dependencies.appended[0].length==0x1c4,
            "Independent top-two candidate order and record-copy length");
        auto unusual=seed(marking::Entry::append_marker);unusual->memory.write<1>(actor+0x104,255);
        marking::invoke(*unusual,marking::Entry::append_marker);
        require(unusual->memory.read<1>(actor+0x104)==0 && unusual->memory.read<4>(actor+0x1b0)==opposite,
            "Original signed-negative append aliases the target slot");
    }
    for(unsigned index=0;index<marking::entries.size();++index){auto entry=marking::Entry(index);auto input=seed(entry);const auto accesses=compare(*input,entry);require(normal_entries[index]>0,"Entry lacks normal component execution");
        for(unsigned budget=0;budget<=90;++budget){auto limited=clone(*input);limited->budget=budget;compare(*limited,entry);}
        std::set<std::uint32_t> bytes;for(auto access:accesses)for(unsigned n=0;n<access.size;++n)bytes.insert(access.address+n);
        for(auto hole:bytes){auto sparse=std::make_unique<State>();sparse->registers=input->registers;sparse->unique=input->unique;sparse->pc=input->pc;
            for(auto byte:bytes)if(byte!=hole){sparse->memory.map(byte,1);sparse->memory.write<1>(byte,input->memory.read<1>(byte));}compare(*sparse,entry);}
        Options negative;negative.negative=true;compare(*input,entry,negative);
    }
    for(auto entry:{marking::Entry::append_marker,marking::Entry::contains_marker,marking::Entry::remove_marker})for(auto count:{0u,1u,2u,3u,4u,127u,128u,255u})for(auto sought:{opposite,opposite+0x1c4,0u}){
        auto input=seed(entry);input->memory.write<1>(actor+0x104,count);input->wr<4>(16,sought);compare(*input,entry);
    }
    for(auto value:{0u,1u,199u,200u,201u,0x7fffu,0x8000u,0xff37u,0xff38u,0xff39u,0xffffu,0x123400c9u}){auto input=seed(marking::Entry::set_pressure_adjustment);input->wr<4>(16,value);compare(*input,marking::Entry::set_pressure_adjustment);}
    for(auto tactical:{0u,1u,2u,255u}){Options o;o.tactical=tactical;compare(*seed(marking::Entry::tactical_marking_trait),marking::Entry::tactical_marking_trait,o);compare(*seed(marking::Entry::assess_cell),marking::Entry::assess_cell,o);}
    for(auto count:{0u,1u,2u,3u,128u,255u}){auto input=seed(marking::Entry::assess_cell);input->memory.write<1>(center+0x59,count);compare(*input,marking::Entry::assess_cell);}
    for(auto phase:{0u,1u,2u,127u,128u,255u}){auto input=seed(marking::Entry::assess_cell);input->memory.write<1>(opposite+0x1c4+0x2c,phase);compare(*input,marking::Entry::assess_cell);}
    for(auto flags:{0u,1u,4u,0xffffffffu}){Options o;o.role=flags;compare(*seed(marking::Entry::assess_cell),marking::Entry::assess_cell,o);}
    for(double factor:{0.0,0.5,1.0,1.25,100.0,std::numeric_limits<double>::quiet_NaN()}){auto input=seed(marking::Entry::assess_cell);input->memory.write<8>(doubles+0xb8,float_bits<8>(factor));compare(*input,marking::Entry::assess_cell);}
    for(auto value:{0u,1u,128u,255u})for(auto coordinates:{0u,7u,8u,128u,255u}){auto input=seed(marking::Entry::eligible_marker);input->memory.write<1>(actor+0x2c,value);input->wr<4>(20,coordinates);compare(*input,marking::Entry::eligible_marker);}
    for(auto field:{0xe000u,0xdff8u}){auto input=seed(marking::Entry::eligible_marker);input->memory.write<4>(match+field,actor);compare(*input,marking::Entry::eligible_marker);}
    auto interrupted=seed(marking::Entry::eligible_marker);interrupted->memory.write<1>(match+0x7928,1);compare(*interrupted,marking::Entry::eligible_marker);
    auto null_actor=seed(marking::Entry::eligible_marker);null_actor->wr<4>(16,0);compare(*null_actor,marking::Entry::eligible_marker);
    for(unsigned fault=1;fault<=28;++fault){Options o;o.fault_at=fault;compare(*seed(marking::Entry::assess_cell),marking::Entry::assess_cell,o);}
    Options child;child.child_negative=true;compare(*seed(marking::Entry::assess_cell),marking::Entry::assess_cell,child);
    std::cout<<"PASS: marking-pressure comparisons="<<comparisons<<" normal="<<normal<<" interrupted="<<interruptions<<" own/child-negatives="<<negatives<<"; 13 complete entries, full ABI/memory/effects/calls/accounting, list bounds/aliases, ranking and weighted pressure\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
