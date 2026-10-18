#include "preferences_actions.hpp"
#include "semantic_io_checks.hpp"
#include <bit>
#include <fstream>
#include <set>

namespace cm::aot {
bool preferences_actions_reference(State&);
std::vector<std::uint32_t> preferences_actions_expected_sites();
void preferences_actions_fixture_tables(State&,std::uint32_t,std::uint32_t);
}
namespace {
using namespace cm::semantic_io_checks;
namespace actions=cm::aot::preferences_actions;
constexpr std::array<std::uint32_t,3> ends{0x1033cf14,0x1033d028,0x1033fc60};
constexpr std::array<unsigned,3> site_counts{41,66,264};
constexpr std::uint32_t tables=fixture+0x10000,context=fixture+0x20000,primary=fixture+0x30000,
    edit=fixture+0x31000,alternate=fixture+0x32000,cells=fixture+0x40000,screen=fixture+0x50000,other_screen=fixture+0x60000;
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{actions::open_with_mode,actions::open_from_game_state,actions::dispatch_action};
unsigned index(std::uint32_t root){const auto i=std::find(actions::entries.begin(),actions::entries.end(),root);require(i!=actions::entries.end(),"Known preference action root");return unsigned(i-actions::entries.begin());}
std::uint32_t event_address(std::uint32_t page,std::uint32_t row){return page+0xba86c+std::uint32_t(std::int32_t(std::int16_t(row))*0x17c);}
struct Options {
    std::uint32_t open_result=1,apply_result=1,setter_result=1;
    unsigned interrupt=0;
    bool allocation=true,own_negative=false,effect_negative=false,redirect_primary_slot=false,
        redirect_primary_value=false,redirect_view=false,redirect_context=false,redirect_panel=false;
};
struct Trace:SemanticGuestObserver {
    std::vector<std::uint32_t> sites;bool effect_negative=false,damaged=false;
    void before_step(State& s,std::uint32_t pc)override {
        sites.push_back(pc);
        if(effect_negative&&!damaged&&s.executed<s.budget){damaged=true;const auto value=s.memory.read<1>(cells+0x3f0);s.memory.write<1>(cells+0x3f0,value^1);s.memory.write<1>(cells+0x3f0,value);}
    }
};
thread_local Trace* original_trace=nullptr;
struct TimedEffects:MemoryObserver {
    State& state;Effects effects;std::vector<std::uint32_t> pcs;
    explicit TimedEffects(State& s):state(s){effects.state=&s;}
    void mapped(std::uint32_t a,std::uint64_t n)override{pcs.push_back(state.pc);effects.mapped(a,n);}
    void written(std::uint32_t a,const std::uint8_t* d,std::size_t n)override{pcs.push_back(state.pc);effects.written(a,d,n);}
    void cleared(std::uint32_t a,std::uint64_t n)override{pcs.push_back(state.pc);effects.cleared(a,n);}
    void read(std::uint32_t a,const std::uint8_t* d,unsigned n)override{pcs.push_back(state.pc);effects.read(a,d,n);}
};
struct Event {std::uint32_t target,stop;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Model:SemanticDependencyDispatch {
    Options options;std::vector<Event> calls;
    explicit Model(Options value):options(value){}
    void reference(State& s,std::uint32_t stop=return_sentinel){const auto n=index(s.pc),root=s.pc;bool initial=true;while(s.pc!=stop){if(root<=s.pc&&s.pc<ends[n]&&(initial||s.pc!=root))require(preferences_actions_reference(s),"Whole original preference action reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));initial=false;}}
    void call(State& s,std::uint32_t target,std::uint32_t stop)override {
        const auto serial=calls.size();calls.push_back({target,stop,s.registers,{}});
        try {
            const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};std::uint32_t result=options.setter_result;
            // Explicit boundary models, including the already reconstructed
            // preference children. This component does not execute native UI,
            // allocation, persistence, or those children's production services.
            s.memory.write<4>(cells+0x300,target);s.memory.write<4>(cells+0x304,s.memory.read<4>(cells+0x304)+1);
            if(target==0x1073e4b0){
                require(arg(3)==context&&arg(4)==tables+0x3000&&arg(5)==tables+0x4000&&arg(6)==1&&!arg(7)&&!arg(8),"Original option-screen callback descriptors and open ABI");result=options.open_result;
            } else if(target==0x10773520){
                require(arg(3)==0x150,"Preference edit allocation size");result=options.allocation?edit:0;
                if(options.redirect_primary_slot)s.memory.write<4>(toc-0x7ac8,cells+4);
                if(options.redirect_primary_value)s.memory.write<4>(cells,alternate);
                if(options.redirect_context)s.memory.write<4>(toc-0x7bb0,0xf0000000);
            } else if(target==0x1013f410){
                require(arg(3)==edit&&(arg(4)==primary||arg(4)==alternate),"Linked preference copy ABI");
                s.memory.write<4>(arg(3),arg(4));s.memory.write<4>(arg(3)+4,s.memory.read<4>(arg(4)+4));result=arg(3);
            } else if(target==0x1073f1e0){
                require(arg(3)==context&&arg(4)<2&&!arg(6),"Original screen user-data write ABI");s.memory.write<4>(cells+0x200+arg(4)*4,arg(5));
            } else if(target==0x1073ef70){
                require(arg(3)==context&&arg(4)<2,"Original screen user-data read ABI");result=s.memory.read<4>(cells+0x200+arg(4)*4);
            } else if(target==0x1013f570){s.memory.write<4>(arg(3)+4,0x21255329);}
            else if(target==0x1013feb0){s.memory.write<4>(cells+0x340,arg(3));result=options.apply_result;}
            else if(target==0x1013f4d0){require(arg(3)==edit&&arg(4)==1,"Destroy edit record with deletion enabled");s.memory.write<4>(cells+0x344,arg(3));}
            else if(target==0x1013fb60){s.memory.write<1>(arg(3)+8,arg(4));if(options.redirect_panel)s.memory.write<4>(toc-0x7b98,cells+0x104);}
            else if(target==0x1013fc50){s.memory.write<1>(arg(3)+9,arg(4));}
            else if(target==0x101406f0){s.memory.write<4>(arg(3)+0x12c,arg(4));}
            else if(target==0x1013f680){
                s.memory.write<4>(cells+0x348,arg(4));
                if(options.redirect_view){s.memory.write<2>(context+0x2c58,1);s.memory.write<4>(event_address(other_screen,0)+0x40,365);}
                if(options.redirect_context)s.memory.write<4>(toc-0x7bb0,0xf0000000);
            } else if(target==0x1013fe00){s.memory.write<2>(arg(3)+10,arg(4));}
            else if(target!=0x10343200)throw std::runtime_error("Unknown preference action dependency "+hex_address(target));
            for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xdd000000+n*0x101);
            for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(1000.25+n));
            s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=stop;
            calls[serial].after=s.registers;
            if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Preference action fixture interruption");
        } catch(...) {calls[serial].after=s.registers;throw;}
    }
};
std::unique_ptr<State> seed(unsigned n,std::uint32_t argument=0,std::uint32_t action=39,std::uint32_t value=7,std::uint32_t mode=0){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);s->memory.map(fixture,0x80000);s->memory.map(fixture+0x108000,0x16000);s->memory.map(fixture+0x278000,0x1000);s->memory.map(stack-0x2000,0x3000);s->memory.map(toc-0x8000,0x9000);
    s->pc=actions::entries[n];s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(12,argument);s->wr<4>(4128,return_sentinel);s->budget=100000;
    for(unsigned f=1280;f<=1311;++f)s->wr<1>(f,0);
    preferences_actions_fixture_tables(*s,toc,tables);s->memory.write<4>(toc-0x7bb0,context);s->memory.write<4>(toc-0x7ac8,cells);s->memory.write<4>(toc-0x7b98,cells+0x100);
    s->memory.write<4>(cells,primary);s->memory.write<4>(cells+4,alternate);s->memory.write<4>(cells+0x100,1);s->memory.write<4>(cells+0x104,1);s->memory.write<4>(cells+0x200,edit);s->memory.write<4>(cells+0x204,mode);
    s->memory.write<4>(context+0x24,screen);s->memory.write<4>(context+0x24+0x2c0,other_screen);s->memory.write<2>(context+0x2c58,0);s->memory.write<2>(context+0x25819e,action);
    s->memory.write<4>(primary+4,0x21255329);s->memory.write<4>(alternate+4,0x12345678);s->memory.write<4>(edit,primary);s->memory.write<4>(edit+4,0xabcdef12);
    const auto row=n!=2||std::int16_t(argument)==-1?0:argument;
    for(auto page:{screen,other_screen}){s->memory.write<4>(event_address(page,row),action);s->memory.write<4>(event_address(page,row)+0x40,value);}
    return s;
}
unsigned cases=0,returns=0,stops=0,negatives=0;
std::array<unsigned,3> root_returns{},root_stops{},root_negatives{};
std::array<std::set<std::uint32_t>,3> root_complete_sites;
std::set<std::uint32_t> complete_sites,all_sites;
struct Result {std::unique_ptr<State> state;std::vector<Effects::Access> accesses;unsigned calls;std::string outcome;std::vector<Event> dependencies;};
Result compare(const State& input,Options options={}) {
    auto a=clone(input),b=clone(input);TimedEffects x(*a),y(*b);Trace ot,nt;nt.effect_negative=options.effect_negative;Model old(options),modern(options);a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto run=[&](State& s,Model& d,bool reference){try{if(reference){original_trace=&ot;d.reference(s);}else methods[index(s.pc)](s,&d,&nt);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="Preference actions root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Preference action fixture interruption"||expected.starts_with("Unmapped data access at "),label+" unexpected reference: "+expected);require(expected==actual,label+" original="+expected+" modern="+actual);
    const auto negative=options.own_negative||options.effect_negative;
    if(negative)require(expected.empty(),label+" negative needs a complete return");
    if(options.own_negative)b->wr<4>(12,b->r<4>(12)^1);
    if(options.effect_negative){require(nt.damaged,label+" effect negative executed");Image(*a).compare(*b,label+" restored effect image");}
    bool rejected=false;
    try {
        if(ot.sites!=nt.sites){const auto delta=std::mismatch(ot.sites.begin(),ot.sites.end(),nt.sites.begin(),nt.sites.end());throw std::runtime_error(label+" source order at="+std::to_string(delta.first-ot.sites.begin())+" old="+(delta.first==ot.sites.end()?"end":hex_address(*delta.first))+" new="+(delta.second==nt.sites.end()?"end":hex_address(*delta.second)));}
        require(old.calls==modern.calls,label+" dependency entry/return ABI");require(x.effects.bytes==y.effects.bytes&&x.pcs==y.pcs,label+" ordered timed effects");require(a->executed==b->executed,label+" instruction accounting");Image(*a).compare(*b,label);
    } catch(const std::runtime_error&) {if(!negative)throw;rejected=true;}
    require(rejected==negative,label+" discriminating negative");const auto n=index(input.pc);++cases;returns+=expected.empty()&&!negative;root_returns[n]+=expected.empty()&&!negative;stops+=!expected.empty();root_stops[n]+=!expected.empty();negatives+=negative;root_negatives[n]+=negative;
    if(!negative)for(auto pc:ot.sites)if(actions::entries[n]<=pc&&pc<ends[n]){all_sites.insert(pc);if(expected.empty()){complete_sites.insert(pc);root_complete_sites[n].insert(pc);}}
    return {std::move(a),x.effects.log,unsigned(old.calls.size()),expected,std::move(old.calls)};
}
void boundaries(const State& input) {
    const auto base=compare(input);require(base.outcome.empty()&&base.state->pc==return_sentinel,"Complete action boundary baseline");const auto steps=base.state->executed-input.executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto s=clone(input);s->budget=s->executed+cut;compare(*s);}
    std::set<std::uint32_t> bytes;for(const auto& access:base.accesses)if(access.size){bytes.insert(access.address);bytes.insert(access.address+std::uint32_t(access.size)-1);}
    std::set<std::uint32_t> holes;for(unsigned n=0;n<5;++n){auto at=bytes.begin();std::advance(at,std::ptrdiff_t(n*(bytes.size()-1)/4));holes.insert(*at);}
    for(auto hole:holes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;s->budget=input.budget;s->executed=input.executed;for(auto [at,size]:input.memory.mapped_ranges())for(auto [address,n]:hole<at||std::uint64_t(hole)>=std::uint64_t(at)+size?std::vector<std::pair<std::uint32_t,std::uint64_t>>{{at,size}}:std::vector<std::pair<std::uint32_t,std::uint64_t>>{{at,hole-at},{hole+1,std::uint64_t(at)+size-hole-1}})if(n){s->memory.map(address,n);s->memory.copy_in(address,input.memory.copy_out(address,n));}require(compare(*s).outcome.starts_with("Unmapped data access at "),"Missing required action byte must fault");}
    for(unsigned cut:std::set<unsigned>{1,base.calls/2,base.calls})if(cut&&cut<=base.calls){Options o;o.interrupt=cut;compare(input,o);}
}
std::vector<const Event*> calls_to(const Result& result,std::uint32_t target){std::vector<const Event*> out;for(const auto& call:result.dependencies)if(call.target==target)out.push_back(&call);return out;}
std::uint32_t argument(const Event& call,unsigned n){return std::uint32_t(read_be<4>(call.before.data()+n*4));}
void corpus(bool probe) {
    for(unsigned n=0;n<3;++n){auto s=seed(n);compare(*s);if(!probe){Options o;o.own_negative=true;compare(*s,o);o.own_negative=false;o.effect_negative=true;compare(*s,o);boundaries(*s);}}
    if(probe)return;
    for(unsigned n=0;n<2;++n)for(unsigned byte=0;byte<256;++byte)for(auto upper:{0u,0x12340000u}){
        auto s=seed(n,upper|byte);const auto out=compare(*s);require(out.state->memory.read<4>(cells+0x200)==edit,"Opening stores the edit pointer");require(out.state->memory.read<4>(cells+0x204)==(n?byte?3u:2u:byte),"Opening low-byte mode semantics");require(out.state->memory.read<4>(edit)==primary,"Opening links its editable copy");
    }
    for(unsigned n=0;n<2;++n)for(auto arg:{0u,1u,256u,0xffffffffu})for(auto opened:{0u,1u,256u,257u,0xffffff80u})for(bool allocated:{false,true}){
        auto s=seed(n,arg);Options o;o.open_result=opened;o.allocation=allocated;const auto out=compare(*s,o);const bool success=(opened&255)!=0;
        require(calls_to(out,0x10773520).size()==unsigned(success),"Only a successful low-byte open allocates");require(calls_to(out,0x1013f410).size()==unsigned(success&&allocated),"Allocation failure skips copy");require(calls_to(out,0x1073f1e0).size()==(success?2u:0u),"Both user-data slots are written after allocation failure too");if(success)require(out.state->memory.read<4>(cells+0x200)==(allocated?edit:0),"Null allocation stays null in user data");
    }
    for(unsigned n=0;n<2;++n)for(auto arg:{0u,1u})for(unsigned redirect=0;redirect<3;++redirect){auto s=seed(n,arg);Options o;o.redirect_primary_slot=redirect==0;o.redirect_primary_value=redirect==1;o.redirect_context=redirect==2;const auto out=compare(*s,o);const auto source=redirect==0?(n?primary:alternate):redirect==1?alternate:primary;require(out.state->memory.read<4>(edit)==source,"Opening preserves late versus cached primary-cell lookup");}
    for(auto action:{0u,39u,40u,41u,42u,43u,44u,45u,61u,62u,0xffffu})for(auto row:{0u,1u,0xfffffffeu,0xffffffffu,0x12340000u})for(auto upper:{0u,0x12340000u}){
        auto s=seed(2,row,action|upper,0x12345687);const auto out=compare(*s);const auto expected=action==40||action==41?0xfffffff7u:action==39||action==42||action==43||action==44||action==45||action==61?0xfffffff5u:0u;require(out.state->r<4>(12)==expected,"Controller action return policy");
    }
    for(auto action:{39u,40u,41u,42u,43u,44u,45u,61u}){auto s=seed(2,0,0x10000,7);s->memory.write<2>(context+0x25819e,action);compare(*s);}
    for(auto action:{42u,43u})for(unsigned byte=0;byte<256;++byte)for(auto upper:{0u,0x12340000u}){
        auto s=seed(2,0,action,upper|byte);const auto out=compare(*s);const auto calls=calls_to(out,action==42?0x1013fb60:0x1013fc50);require(calls.size()==1&&argument(*calls[0],4)==std::uint32_t(std::int32_t(std::int8_t(byte))),"Language and currency payloads use the signed low byte");
    }
    for(auto action:{44u,61u})for(auto value:{0u,1u,255u,256u,0x7fffffffu,0x80000000u,0xffffffffu}){auto s=seed(2,0,action,value);const auto out=compare(*s);const auto calls=calls_to(out,action==44?0x1013f680:0x101406f0);require(calls.size()==1&&argument(*calls[0],4)==value,"Option and video payloads retain all 32 bits");}
    for(auto value:{0u,7u,14u,30u,90u,180u,360u,365u,366u,0x12340007u,0xffff8000u,0xffffffffu}){auto s=seed(2,0,45,value);const auto out=compare(*s);const auto option=calls_to(out,0x1013f680),interval=calls_to(out,0x1013fe00);require(option.size()==1&&argument(*option[0],4)==8192&&interval.size()==1&&argument(*interval[0],4)==std::uint32_t(std::int32_t(std::int16_t(value))),"Auto-save enables its option before passing the signed low-halfword interval");}
    for(auto action:{40u,41u})for(auto applied:{0u,1u}){auto s=seed(2,0,action);Options o;o.apply_result=applied;const auto out=compare(*s,o);const auto applied_calls=calls_to(out,0x1013feb0);require(applied_calls.size()==1&&argument(*applied_calls[0],3)==(action==40?primary:edit),"Discard applies primary; accept applies edited preferences");require(calls_to(out,0x1013f4d0).size()==1&&out.state->r<4>(12)==0xfffffff7,"Both actions destroy the edit and return close even after apply failure");}
    for(auto mode:{0u,1u,2u,3u,255u,256u,257u})for(bool panel:{false,true})for(bool redirect:{false,true}){
        auto s=seed(2,0,42,1,mode);s->memory.write<4>(cells+0x100,panel);Options o;o.setter_result=0;o.redirect_panel=redirect;const auto out=compare(*s,o);require(calls_to(out,0x10343200).size()==unsigned((mode&255)<=1&&(panel||redirect)),"Language refresh uses the current panel cell and low-byte screen mode, even after setter failure");
    }
    {auto s=seed(2,0,45,7);Options o;o.setter_result=0;o.redirect_view=true;o.redirect_context=true;const auto out=compare(*s,o);const auto interval=calls_to(out,0x1013fe00);require(interval.size()==1&&argument(*interval[0],4)==365,"Auto-save rereads the active view after option selection while keeping its original context");}
    for(auto action:{39u,40u,41u,42u,43u,44u,45u,61u,0u})boundaries(*seed(2,0,action));
}
}
namespace cm::aot {void preferences_actions_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try{
    require(argc==1||argc==2,"Optional preference action coverage output or probe");const bool probe=argc==2&&std::string_view(argv[1])=="--probe";corpus(probe);const auto expected=preferences_actions_expected_sites();
    if(argc==2&&!probe){std::ofstream out(argv[1]);for(auto pc:expected)out<<hex_address(pc)<<" "<<all_sites.contains(pc)<<" "<<complete_sites.contains(pc)<<'\n';}
    if(!probe)for(auto pc:expected)if(!complete_sites.contains(pc))std::cerr<<"Uncovered preference action site "<<hex_address(pc)<<std::endl;
    std::cout<<"Preference actions comparisons="<<cases<<" returns="<<returns<<" stops="<<stops<<" negatives="<<negatives<<" completed_sites="<<complete_sites.size()<<"/"<<expected.size()<<std::endl;
    for(unsigned n=0;n<3;++n)std::cout<<"Root "<<hex_address(actions::entries[n])<<" returns="<<root_returns[n]<<" stops="<<root_stops[n]<<" negatives="<<root_negatives[n]<<" completed_sites="<<root_complete_sites[n].size()<<std::endl;
    if(!probe){require(complete_sites.size()==expected.size(),"Every action instruction needs a complete-return witness");for(unsigned n=0;n<3;++n){require(root_complete_sites[n].size()==site_counts[n],"Every action entry needs its own complete-return coverage");require(root_negatives[n]==2,"Every action entry needs result and restored-effect negatives");}}
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
