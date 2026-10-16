#include "ball_contact.hpp"
#include "semantic_io_checks.hpp"
#include "register_context_adapter.hpp"
#include <set>
#include "ball_contact_description_tables.hpp"
namespace cm::aot {bool ball_contact_description_reference(State&);bool ball_contact_description_assessment_reference(State&);bool ball_contact_description_reactions_reference(State&);bool ball_contact_description_preparation_reference(State&);bool ball_contact_description_weight_reference(State&);bool ball_contact_description_exit_reference(State&);bool ball_contact_description_players_reference(State&);bool ball_contact_description_next_reference(State&);bool ball_contact_description_return_reference(State&);}
namespace {
using namespace cm::semantic_io_checks;
namespace contact=cm::aot::ball_contact;
constexpr std::uint32_t actor=fixture+0x1000,receiver=fixture+0x2000,actor_profile=fixture+0x3000,receiver_profile=fixture+0x4000;
constexpr std::uint32_t actor_data=fixture+0x5000,receiver_data=fixture+0x6000,singles=fixture+0x8000,doubles=fixture+0x9000,weights=fixture+0xa000;
constexpr std::uint32_t match=fixture+0xb000,context=fixture+0x600,record=fixture+0x700,weight_table=fixture+0xc000;
struct Options {std::vector<std::uint32_t> random;std::array<std::uint32_t,2> records{record,record+4};std::array<std::uint32_t,4> ratings{2,4,3,5};std::uint32_t descriptor=3;bool own_negative=false,child_negative=false,mutate_context=false,mutate_players=false,mutate_table=false;unsigned interrupt=0;std::int32_t table_slot=-0x52e0;std::uint32_t table_replacement=weight_table+0x100;};
struct Trace final:SemanticGuestObserver {std::vector<std::uint32_t> sites;void before_step(State&,std::uint32_t pc)override{sites.push_back(pc);}};
thread_local Trace* original_trace=nullptr;
struct Event {std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Model final:SemanticDependencyDispatch {
    bool original;Options options;unsigned phase,draws=0,queries=0;std::vector<Event> calls;
    Model(bool old,Options o,unsigned part):original(old),options(std::move(o)),phase(part){}
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override {
        Event e{target,continuation,s.registers,{}};std::uint32_t result=0;
        if(target==0x10773804||target==0x10773854) {
            invoke_register_context(s,target);
            if(options.child_negative&&!original)s.memory.write<8>(stack-0x28,s.memory.read<8>(stack-0x28)^1);
        } else {
            switch(target) {
            case 0x101d3c90:result=draws<options.random.size()?options.random[draws]:2;++draws;break;
            case 0x1018c340:result=options.records[unsigned(s.r<4>(16)!=actor_data)];break;
            case 0x1018c180:result=options.descriptor;break;
            case 0x1018c270:result=options.ratings[std::min(queries++,3u)];break;
            case 0x101f6940:break;
            default:throw std::runtime_error("Unknown contact-description dependency "+hex_address(target));
            }
            s.memory.write<4>(fixture+0x1e000+std::uint32_t(calls.size())*4,target^std::uint32_t(s.r<4>(12))^unsigned(options.child_negative&&!original));
            if(options.mutate_context)s.memory.write<4>(context,context+0x400+std::uint32_t(calls.size())*4);
            if(options.mutate_players&&target==0x1018c340){s.memory.write<4>(actor+0x50,float_bits<4>(-100));s.memory.write<1>(receiver_profile+0x2d,255);}
            for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xac000000+n*17);
            for(unsigned n=0;n<=13;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-11.0-n));
            s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=continuation;
        }
        e.after=s.registers;calls.push_back(e);if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Fixture contact-description interruption");
    }
    void reference(State& s) {
        if(phase==11){while(s.pc!=return_sentinel){if(s.pc>=contact::description_entry&&s.pc<0x101f6940)require(ball_contact_description_reference(s),"Missing complete independent description");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));}return;}
        if(phase==7){while(s.pc>=contact::description_weight_phase&&s.pc<contact::description_assessment_phase)require(ball_contact_description_weight_reference(s),"Missing independent description weight");return;}
        if(phase==9){while(s.pc!=contact::description_reaction_phase&&s.pc!=contact::description_next_phase){if(s.pc>=contact::description_assessment_phase&&s.pc<contact::description_reaction_phase)require(ball_contact_description_assessment_reference(s),"Missing independent description assessment");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));}return;}
        if(phase==10){while(s.pc!=contact::description_next_phase&&s.pc!=contact::description_return_phase){if(s.pc>=contact::description_reaction_phase&&s.pc<contact::description_next_phase)require(ball_contact_description_reactions_reference(s),"Missing independent description reactions");else if(s.pc==0x101d3c90||s.pc==0x101f6940)call(s,s.pc,std::uint32_t(s.r<4>(4128)));else return;}return;}
        const auto done=[&] {
            if(phase==0)return s.pc==contact::description_exit_phase;
            if(phase==1||phase==2)return s.pc==contact::description_players_phase||s.pc==contact::description_return_phase;
            if(phase==3)return s.pc==contact::description_weight_phase||s.pc==contact::description_return_phase;
            if(phase==4)return s.pc==contact::description_weight_phase||s.pc==return_sentinel;
            if(phase==5)return s.pc==return_sentinel;
            if(phase==8)return s.pc==contact::description_assessment_phase||s.pc==contact::description_next_phase||s.pc==return_sentinel;
            return s.pc==contact::description_test_phase;
        };
        while(!done()) {
            if(s.pc>=contact::description_entry&&s.pc<contact::description_exit_phase)require(ball_contact_description_preparation_reference(s),"Missing independent description preparation");
            else if(s.pc>=contact::description_exit_phase&&s.pc<contact::description_players_phase)require(ball_contact_description_exit_reference(s),"Missing independent description exit");
            else if(s.pc>=contact::description_players_phase&&s.pc<contact::description_weight_phase)require(ball_contact_description_players_reference(s),"Missing independent description players");
            else if(s.pc>=contact::description_weight_phase&&s.pc<contact::description_assessment_phase)require(ball_contact_description_weight_reference(s),"Missing independent description weight");
            else if(s.pc>=contact::description_next_phase&&s.pc<contact::description_return_phase)require(ball_contact_description_next_reference(s),"Missing independent description loop");
            else if(s.pc>=contact::description_return_phase&&s.pc<0x101f6940)require(ball_contact_description_return_reference(s),"Missing independent description return");
            else call(s,s.pc,std::uint32_t(s.r<4>(4128)));
        }
    }
};
std::unique_ptr<State> seed() {
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned n=1280;n<1312;++n)s->wr<1>(n,0);s->memory.map(fixture,0x20000);s->memory.map(stack-0x300,0x500);s->memory.map(toc-0x8000,0x9000);
    s->pc=contact::description_entry;s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(12,match);s->wr<4>(16,actor);s->wr<4>(20,5);s->wr<4>(24,receiver);s->wr<4>(28,7);s->wr<4>(32,0);s->wr<4>(4128,return_sentinel);
    s->memory.write<4>(toc-0x7ba0,context);s->memory.write<4>(toc-0x52c8,singles);s->memory.write<4>(toc-0x52c4,doubles);s->memory.write<2>(toc-0x3914,0x0102);s->memory.write<4>(toc-0x52dc,weights);
    s->memory.write<4>(weights,float_bits<4>(8));s->memory.write<4>(weights+4,float_bits<4>(16));s->memory.write<4>(context,context+0x400);s->memory.write<1>(record,11);s->memory.write<1>(record+4,22);
    for(auto [player,profile,data]:std::array<std::array<std::uint32_t,3>,2>{{{actor,actor_profile,actor_data},{receiver,receiver_profile,receiver_data}}}) {
        s->memory.write<4>(player+0x70,profile);s->memory.write<4>(player+0x6c,data);s->memory.write<4>(player+0x50,float_bits<4>(7500));s->memory.write<1>(profile+0x2d,20);
    }
    for(auto [offset,value]:std::array<std::pair<unsigned,double>,8>{{{0x10,1},{0x28,10},{0x54,0.0001},{0xc8,0.25},{0xdc,0.1},{0xcc,1},{0x108,2},{0x11c,0.5}}})s->memory.write<4>(singles+offset,float_bits<4>(value));
    s->memory.write<4>(toc-0x52e0,weight_table);s->memory.write<4>(match+0xe000,actor);s->memory.write<4>(actor+0xdc,float_bits<4>(4));s->memory.write<4>(receiver+0xdc,float_bits<4>(8));
    for(unsigned n=0;n<48;++n){s->memory.write<4>(weight_table+n*4,original_contact_weight_table[n]);s->memory.write<4>(weight_table+0x100+n*4,original_contact_weight_table[47-n]);}
    for(auto [offset,value]:std::array<std::pair<unsigned,double>,11>{{{0x48,2},{0x130,3},{0x2b0,4},{0x2b8,0.5},{0x2c0,5},{0xc0,0.25},{0x2c8,0.75},{0x2d0,6},{0x2d8,7},{0x2e0,8},{0x30,9}}})s->memory.write<8>(doubles+offset,float_bits<8>(value));
    s->memory.write<4>(toc-0x52e4,weight_table+0x400);s->memory.write<4>(toc-0x52e8,weight_table+0x500);
    for(unsigned n=0;n<original_contact_style_table.size();++n)s->memory.write<4>(weight_table+0x400+n*4,original_contact_style_table[n]);
    for(unsigned n=0;n<original_contact_random_table.size();++n)s->memory.write<4>(weight_table+0x500+n*4,original_contact_random_table[n]);
    s->memory.write<8>(doubles+0x98,float_bits<8>(2));s->memory.write<4>(actor+0x7c,float_bits<4>(4));s->memory.write<4>(receiver+0x7c,float_bits<4>(8));
    s->memory.write<8>(doubles+0x60,float_bits<8>(4503601774854144.0));s->memory.write<8>(doubles+0x90,float_bits<8>(0.1));s->memory.write<8>(doubles+0x1b8,float_bits<8>(1));s->memory.write<8>(doubles+0x40,float_bits<8>(2));return s;
}
unsigned cases=0,completed=0,interruptions=0,negatives=0;std::set<std::uint32_t> sites;
struct Result {std::unique_ptr<State> state;std::vector<Effects::Access> accesses;std::vector<Event> calls;std::string error;};
Result compare(const State& input,Options options={},unsigned phase=0) {
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    if(options.mutate_table) {
        auto edit=[&](State& s,Effects& e,std::uint32_t at,unsigned){if(at==toc+options.table_slot&&!e.mutated){e.mutated=true;s.memory.write<4>(at,options.table_replacement);}};x.on_read=edit;y.on_read=edit;
    }
    Trace old_trace,new_trace;Model old(true,options,phase),modern(false,options,phase);
    const auto run=[&](State& s,Model& m,bool reference){try{if(reference){original_trace=&old_trace;m.reference(s);}else {
        if(phase==11)contact::describe(s,&m,&new_trace);
        if(phase==0||phase==2||phase==4||phase==8)contact::prepare_description(s,&m,&new_trace);
        if((phase==1||phase==2||phase==4||phase==8)&&s.pc==contact::description_exit_phase)contact::select_description_exit(s,&m,&new_trace);
        if((phase==6||phase==4||phase==8)&&s.pc==contact::description_players_phase)contact::prepare_description_players(s,&m,&new_trace);
        if((phase==3||phase==4||phase==8)&&(s.pc==contact::description_next_phase||s.pc==contact::description_test_phase))contact::next_description_player(s,&m,&new_trace);
        if(phase==7||(phase==8&&s.pc==contact::description_weight_phase))contact::adjust_description_weight(s,&m,&new_trace);
        if(phase==9)contact::assess_description_weight(s,&m,&new_trace);
        if(phase==10)contact::select_description_reaction(s,&m,&new_trace);
        if((phase==5||phase==4||phase==8)&&s.pc==contact::description_return_phase)contact::return_description_frame(s,&m,&new_trace);
    }original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="Contact description phase="+std::to_string(phase)+" case="+std::to_string(cases);require(!expected.starts_with("Unknown contact"),expected);require(expected==actual,label+" original="+expected+" modern="+actual);
    if(options.own_negative)b->wr<4>(52,b->r<4>(52)^1);bool rejected=false;
    try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative&&!options.child_negative){std::cerr<<"Final "<<hex_address(a->pc)<<" / "<<hex_address(b->pc)<<"; steps "<<old_trace.sites.size()<<" / "<<new_trace.sites.size()<<'\n';throw;}rejected=true;}
    require(rejected==(options.own_negative||options.child_negative),label+" discriminating own/child negative");
    if(!rejected){require(x.bytes==y.bytes,label+" ordered effects");if(a->executed!=b->executed){std::cerr<<label<<" executed "<<a->executed<<" / "<<b->executed<<" sites "<<old_trace.sites.size()<<" / "<<new_trace.sites.size()<<'\n';for(unsigned n=0;n<std::min(old_trace.sites.size(),new_trace.sites.size());++n)if(old_trace.sites[n]!=new_trace.sites[n]){std::cerr<<"First accounting site "<<hex_address(old_trace.sites[n])<<" / "<<hex_address(new_trace.sites[n])<<'\n';break;}}
        require(a->executed==b->executed,label+" accounting");require(old.calls==modern.calls,label+" complete dependency ABI");
        if(old_trace.sites!=new_trace.sites)for(unsigned n=0;n<std::min(old_trace.sites.size(),new_trace.sites.size());++n)if(old_trace.sites[n]!=new_trace.sites[n]){std::cerr<<"First site "<<hex_address(old_trace.sites[n])<<" / "<<hex_address(new_trace.sites[n])<<'\n';break;}
        require(old_trace.sites==new_trace.sites,label+" original-site order");}
    ++cases;completed+=expected.empty()&&!rejected;interruptions+=!expected.empty();negatives+=rejected;sites.insert(old_trace.sites.begin(),old_trace.sites.end());return {std::move(a),x.log,old.calls,expected};
}
void boundaries(const State& input,Options options={},unsigned phase=0) {
    const auto baseline=compare(input,options,phase);const auto steps=baseline.state->executed-input.executed;
    for(std::uint64_t n=0;n<=steps;++n){auto s=clone(input);s->budget=s->executed+n;compare(*s,options,phase);}
    std::set<std::uint32_t> bytes;for(const auto& a:baseline.accesses)for(unsigned n=0;n<a.size;++n)bytes.insert(a.address+n);
    for(auto hole:bytes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;for(auto b:bytes)if(b!=hole){s->memory.map(b,1);s->memory.write<1>(b,input.memory.read<1>(b));}compare(*s,options,phase);}
    for(unsigned n=1;n<=baseline.calls.size();++n){auto o=options;o.interrupt=n;compare(input,o,phase);}
}
std::unique_ptr<State> flow_seed(std::uint32_t pc=contact::description_exit_phase) {
    auto s=seed();s->pc=pc;s->wr<4>(4,stack-0xc0);s->wr<4>(84,stack-0xc0+0x40);s->wr<4>(88,stack-0xc0+0x3a);
    s->wr<4>(100,context);s->wr<4>(104,receiver);s->wr<4>(108,actor);s->wr<4>(112,singles);s->wr<4>(116,match);s->wr<4>(120,doubles);s->wr<4>(124,0);
    s->memory.write<1>(stack-0xc0+0xef,1);s->memory.write<1>(stack-0xc0+0x38,7);s->memory.write<1>(stack-0xc0+0x3a,3);s->memory.write<4>(stack-0xc0+0x40,float_bits<4>(8));
    s->memory.write<2>(stack-0xc0+0xe2,5);s->memory.write<2>(stack-0xc0+0xea,7);s->memory.write<4>(stack+8,return_sentinel);s->memory.write<4>(stack-0xc0,stack);
    for(unsigned n=21;n<=31;++n)s->memory.write<4>(stack-0x5c+(n-21)*4,0x11223300u+n);
    for(unsigned n=27;n<=31;++n)s->memory.write<8>(stack-(32-n)*8,float_bits<8>(double(n)+0.25));return s;
}
std::unique_ptr<State> weight_seed(unsigned index=0,unsigned kind=0) {
    auto s=flow_seed(contact::description_weight_phase);s->wr<4>(124,index);s->memory.write<4>(stack-0xc0+0x48,actor);s->memory.write<4>(stack-0xc0+0x4c,receiver);
    s->memory.write<2>(stack-0xc0+0x3c,kind);s->memory.write<2>(stack-0xc0+0x3e,kind);s->memory.write<4>(stack-0xc0+0x44,float_bits<4>(16));s->memory.write<1>(stack-0xc0+0x39,9);
    s->memory.write<4>(match+0xe000,index==0?actor:receiver);return s;
}
}
namespace cm::aot {void ball_contact_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(){try {
    boundaries(*seed());
    for(auto first:{0u,actor})for(auto second:{0u,receiver}){auto s=seed();s->wr<4>(16,first);s->wr<4>(24,second);auto r=compare(*s);if(!first&&second)require(r.error.starts_with("Unmapped data access")&&r.state->pc==0x101f56ec,"Receiver scaling retains the original initiating-profile null fault");boundaries(*s);}
    for(auto a:{0u,record})for(auto b:{0u,record+4})for(auto gate:{0u,1u,255u}){Options o;o.records={a,b};o.ratings={gate,gate,gate,gate};boundaries(*seed(),o);}
    for(unsigned trait=0;trait<256;++trait){auto s=seed();s->memory.write<1>(actor_profile+0x2d,trait);s->memory.write<1>(receiver_profile+0x2d,255-trait);compare(*s);}
    for(auto a:{0u,9u,10u,11u,127u,128u,255u})for(auto b:{0u,9u,10u,11u,127u,128u,255u}){auto s=seed();s->memory.write<1>(actor_profile+0x2d,a);s->memory.write<1>(receiver_profile+0x2d,b);boundaries(*s);}
    for(auto value:{-10000.0,0.0,1.0,7499.0,7500.0,10000.0,1.0e30})for(auto other:{-10000.0,0.0,7500.0,10000.0}){auto s=seed();s->memory.write<4>(actor+0x50,float_bits<4>(value));s->memory.write<4>(receiver+0x50,float_bits<4>(other));compare(*s);}
    for(auto slot:{0x10u,0x28u,0x54u,0xc8u,0xdcu,0xccu,0x108u,0x11cu})for(auto bits:{0u,0x80000000u,0x3f800000u,0x7f800000u,0xff800000u,0x7fc12345u,0x7f812345u}){auto s=seed();s->memory.write<4>(singles+slot,bits);compare(*s);}
    for(auto value:{0u,1u,49u,50u,51u,75u,76u,127u,128u,255u,256u,0xffffffffu}){Options o;o.random.assign(16,value);boundaries(*seed(),o);}
    {Options o;o.random={102,51,1,76,75,102,51,1,76,74};boundaries(*seed(),o);}
    for(auto rating:{0u,1u,10u,127u,128u,255u,0x100u,0x1ffu}){Options o;o.ratings.fill(rating);boundaries(*seed(),o);}
    {Options o;o.ratings={1,0,1,255};o.mutate_context=true;o.mutate_players=true;boundaries(*seed(),o);}
    for(auto offset:{-0x7ba0,-0x52c8,-0x52c4,-0x52dc})for(auto pointer:{0u,actor+1,stack-0xc0}){auto s=seed();s->memory.write<4>(toc+offset,pointer);compare(*s);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*seed(),o);}
    for(unsigned style=0;style<256;++style)for(auto draw:{0u,1u,2u,3u,5u,6u,8u,9u,10u,11u,127u,128u,255u,0xffffffffu}) {
        auto s=flow_seed();s->memory.write<1>(stack-0xc0+0x3a,style);Options o;o.random={0,draw};compare(*s,o,1);
    }
    for(auto style:{0u,1u,2u,3u,4u,5u,128u,255u})for(auto draw:{0u,3u,6u,8u,9u,10u,11u,0xffffffffu}){auto s=flow_seed();s->memory.write<1>(stack-0xc0+0x3a,style);Options o;o.random={0,draw};boundaries(*s,o,1);}
    for(auto actor_pointer:{0u,actor})for(auto flag:{0u,1u,128u,255u})for(auto value:{-100.0,0.0,1.0,100.0}){auto s=flow_seed();s->wr<4>(108,actor_pointer);s->memory.write<1>(stack-0xc0+0xef,flag);s->memory.write<4>(actor+0x50,float_bits<4>(value));boundaries(*s,{},1);}
    for(auto bits:{0u,0x80000000u,0x3f800000u,0x7f800000u,0xff800000u,0x7fc12345u}){auto s=flow_seed();s->memory.write<4>(singles+0xd8,bits);boundaries(*s,{},1);s->memory.write<4>(stack-0xc0+0x40,bits);compare(*s,{},1);}
    for(auto phase:{2u,4u})for(auto flag:{0u,1u})for(auto actor_pointer:{0u,actor})for(auto receiver_pointer:{0u,receiver}) {
        auto s=seed();s->wr<4>(16,actor_pointer);s->wr<4>(24,receiver_pointer);s->wr<4>(32,flag);boundaries(*s,{},phase);
    }
    for(unsigned raw=0;raw<256;++raw)for(auto pc:{contact::description_next_phase,contact::description_test_phase}){auto s=flow_seed(pc);s->wr<4>(124,0xabcd0000|raw);auto r=compare(*s,{},3);require(r.state->pc==(signed_value<1>(raw+unsigned(pc==contact::description_next_phase))<2?contact::description_weight_phase:contact::description_return_phase),"Loop increments the full word and tests its signed byte");}
    for(auto raw:{0u,1u,2u,127u,128u,255u}){auto s=flow_seed(contact::description_next_phase);s->wr<4>(124,raw);boundaries(*s,{},3);}
    {auto s=flow_seed(contact::description_return_phase);auto r=compare(*s,{},5);require(r.state->pc==return_sentinel&&r.state->r<4>(4)==stack,"Description return restores its actual caller frame");for(unsigned n=21;n<=31;++n)require(r.state->r<4>(n*4)==0x11223300u+n,"Description return restores every nonvolatile GPR");for(unsigned n=27;n<=31;++n)require(r.state->r<8>(0x4000+n*16)==float_bits<8>(double(n)+0.25),"Description return restores all five saved FPRs");boundaries(*s,{},5);}
    for(auto first:{0u,32767u,32768u,65535u})for(auto second:{0u,32767u,32768u,65535u}){auto s=flow_seed(contact::description_players_phase);s->memory.write<2>(stack-0xc0+0xe2,first);s->memory.write<2>(stack-0xc0+0xea,second);compare(*s,{},6);}boundaries(*flow_seed(contact::description_players_phase),{},6);
    for(auto phase:{1u,3u,5u,6u})for(bool child:{false,true}){if(child&&(phase==3||phase==6))continue;auto s=flow_seed(phase==3?contact::description_next_phase:phase==5?contact::description_return_phase:phase==6?contact::description_players_phase:contact::description_exit_phase);Options o;o.own_negative=!child;o.child_negative=child;compare(*s,o,phase);}
    const std::array<std::uint32_t,13> weight_targets{{0x101f5ab8,0x101f5afc,0x101f5b40,0x101f5b9c,0x101f5be0,0x101f5c3c,0x101f5c80,0x101f5d10,0x101f5d54,0x101f5d98,0x101f5ddc,0x101f5e20,0x101f5e64}};
    for(auto index:{0u,1u})for(unsigned kind=0;kind<256;++kind){auto s=weight_seed(index,kind);auto r=compare(*s,{},7);require(r.state->pc==contact::description_assessment_phase,"Live opponent kind reaches weight assessment");}
    for(auto index:{0u,1u})for(unsigned kind=0;kind<48;++kind)for(auto target:weight_targets){auto s=weight_seed(index,kind);s->memory.write<4>(weight_table+kind*4,target);auto r=compare(*s,{},7);
        unsigned offset=0x48;bool multiply=false;switch(target){case 0x101f5afc:offset=0x130;break;case 0x101f5b40:offset=0x2b0;break;case 0x101f5b9c:offset=0x2b8;multiply=true;break;case 0x101f5be0:offset=0x2c0;break;case 0x101f5c80:case 0x101f5d10:offset=0xc0;multiply=true;break;case 0x101f5d54:offset=0x2d0;break;case 0x101f5d98:offset=0x2d8;break;case 0x101f5e20:offset=0x2e0;break;case 0x101f5e64:offset=0x30;break;default:break;}
        const auto constant=float_value<8>(s->memory.read<8>(doubles+offset));const float factor=float(multiply?double(index?4:8)*constant:double(index?4:8)/constant);
        require(r.state->memory.read<4>(stack-0xc0+0x40+index*4)==float_bits<4>(double(float(index?16:8))*double(factor)),"Independent opponent scale preserves separate double factor rounding and single weight product");
        const auto sample=index?9u:7u;const auto expected=target==0x101f5b40||target==0x101f5be0?sample*2:target==0x101f5c80?unsigned(int(double(sample)*0.75)):sample;
        require(r.state->memory.read<1>(stack-0xc0+0x38+index)==expected,"Weight variant retains its independent sample-byte transformation");
    }
    for(auto index:{0u,1u})for(auto target:weight_targets){auto s=weight_seed(index);s->memory.write<4>(weight_table,target);boundaries(*s,{},7);s->pc=target;boundaries(*s,{},7);}
    for(auto index:{0u,1u})for(auto current:{0u,actor})for(auto opponent:{0u,receiver})for(auto owns:{0u,actor,receiver}){auto s=weight_seed(index);s->memory.write<4>(stack-0xc0+0x48+index*4,current);s->memory.write<4>(stack-0xc0+0x48+(1-index)*4,opponent);s->memory.write<4>(match+0xe000,owns);boundaries(*s,{},7);}
    for(auto kind:{48u,32767u,32768u,65535u}){auto s=weight_seed(0,kind);s->memory.write<4>(toc-0x52e0,0);boundaries(*s,{},7);}
    for(auto target:{0u,contact::description_assessment_phase,contact::description_next_phase,contact::description_return_phase,0x12345679u}){auto s=weight_seed();s->memory.write<4>(weight_table,target);auto r=compare(*s,{},7);require(r.state->pc==target,"Unknown weight table target remains a component continuation");boundaries(*s,{},7);}
    for(auto target:{0x101f5b40u,0x101f5be0u,0x101f5c80u})for(unsigned byte=0;byte<256;++byte){auto s=weight_seed();s->memory.write<4>(weight_table,target);s->memory.write<1>(stack-0xc0+0x38,byte);compare(*s,{},7);}
    for(auto target:weight_targets)for(auto bits:{0u,0x80000000u,0x7f800000u,0xff800000u,0x7fc12345u}){auto s=weight_seed();s->memory.write<4>(weight_table,target);s->memory.write<4>(receiver+0xdc,bits);compare(*s,{},7);}
    {Options o;o.mutate_table=true;boundaries(*weight_seed(),o,7);}for(auto pointer:{0u,weight_table+1,weight_table+0x200,stack-0xc0}){auto s=weight_seed();s->memory.write<4>(toc-0x52e0,pointer);compare(*s,{},7);}
    for(auto target:weight_targets)for(auto mode:{0u,1u,0xa5u,0xffu}){auto s=weight_seed();s->memory.write<4>(weight_table,target);for(unsigned n=1280;n<1312;++n)s->wr<1>(n,mode);compare(*s,{},7);}
    for(auto kind:{0u,5u,7u,47u,48u,32768u})for(auto owns:{0u,actor,receiver}){auto s=seed();s->wr<4>(28,kind);s->memory.write<4>(match+0xe000,owns);boundaries(*s,{},8);}
    {Options o;o.own_negative=true;compare(*weight_seed(),o,7);}
    for(auto index:{0u,1u})for(auto kind:{0u,5u,6u,32768u})for(auto value:{-4.0,0.0,1.0,2.0,3.0,8.0}) {
        auto s=weight_seed(index,kind);s->pc=contact::description_assessment_phase;s->memory.write<4>((index?receiver:actor)+0x7c,float_bits<4>(value));boundaries(*s,{},9);
    }
    for(auto draw:{0u,1u,39u,40u,79u,80u,81u,160u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto kind:{0u,5u}) {
        auto s=weight_seed(0,kind);s->pc=contact::description_assessment_phase;Options o;o.random={draw};boundaries(*s,o,9);
    }
    for(auto bits:{0u,0x80000000u,0x7f800000u,0xff800000u,0x7fc12345u,0x7f812345u})for(auto kind:{0u,5u}) {
        auto s=weight_seed(0,kind);s->pc=contact::description_assessment_phase;s->memory.write<4>(actor+0x7c,bits);compare(*s,{},9);s->memory.write<4>(stack-0xc0+0x40,bits);compare(*s,{},9);
    }
    for(auto phase:{9u,10u})for(bool child:{false,true}){auto s=weight_seed();s->pc=phase==9?contact::description_assessment_phase:contact::description_reaction_phase;Options o;o.own_negative=!child;o.child_negative=child;compare(*s,o,phase);}
    for(auto index:{0u,1u})for(auto kind:{0u,5u})for(auto other:{0u,receiver})for(unsigned style=0;style<256;++style)for(auto draw:{0u,3u,4u,6u,7u,8u,9u,10u,11u,0xffffffffu}) {
        auto s=weight_seed(index,kind);s->pc=contact::description_reaction_phase;s->memory.write<1>(stack-0xc0+0x3a+index,style);s->memory.write<4>(stack-0xc0+0x48+(1-index)*4,other);Options o;o.random={draw};compare(*s,o,10);
    }
    for(auto index:{0u,1u})for(auto kind:{0u,5u})for(auto other:{0u,receiver})for(auto style:{0u,1u,2u,3u,4u,5u,6u,7u,8u,9u,10u,128u})for(auto draw:{0u,3u,4u,6u,7u,8u,9u,10u,11u,0xffffffffu}) {
        auto s=weight_seed(index,kind);s->pc=contact::description_reaction_phase;s->memory.write<1>(stack-0xc0+0x3a+index,style);s->memory.write<4>(stack-0xc0+0x48+(1-index)*4,other);Options o;o.random={draw};boundaries(*s,o,10);
    }
    for(auto index:{0u,1u})for(auto mode:{0u,4u,128u,255u})for(auto other:{0u,receiver}) {
        auto s=weight_seed(index);s->pc=contact::description_reaction_phase;s->memory.write<1>(match+0x791f,mode);s->memory.write<4>(stack-0xc0+0x48+(1-index)*4,other);boundaries(*s,{},10);
    }
    // This path has two observable callbacks, separated only by fall-through.
    {auto s=weight_seed();s->pc=contact::description_reaction_phase;s->memory.write<4>(stack-0xc0+0x4c,0);s->memory.write<1>(stack-0xc0+0x3a,0);Options o;o.random={6};auto r=compare(*s,o,10);
        require(r.calls.size()==3&&read_be<4>(r.calls[1].before.data()+16)==2&&read_be<4>(r.calls[2].before.data()+16)==5,"Unpaired fallback preserves both kind-two and kind-five callbacks");}
    for(auto index:{0u,1u})for(unsigned style=0;style<10;++style)for(auto target:original_contact_style_table) {
        auto s=weight_seed(index);s->pc=contact::description_reaction_phase;s->memory.write<1>(stack-0xc0+0x3a+index,style);s->memory.write<4>(weight_table+0x400+style*4,target);compare(*s,{},10);
    }
    for(unsigned draw=0;draw<9;++draw)for(auto target:original_contact_random_table) {
        auto s=weight_seed();s->pc=contact::description_reaction_phase;s->memory.write<1>(stack-0xc0+0x3a,10);s->memory.write<4>(weight_table+0x500+draw*4,target);Options o;o.random={draw};compare(*s,o,10);
    }
    for(auto table:{0u,1u}) {
        auto s=weight_seed();s->pc=contact::description_reaction_phase;s->memory.write<1>(stack-0xc0+0x3a,table?10:0);Options o;o.random={0};
        const auto slot=table?-0x52e8:-0x52e4;const auto address=weight_table+(table?0x500:0x400);
        for(auto target:{0u,contact::description_next_phase,contact::description_return_phase,0x12345679u}){s->memory.write<4>(address,target);auto r=compare(*s,o,10);require(r.state->pc==target,"Live reaction table preserves external component destination");}
        s->memory.write<4>(address,table?original_contact_random_table[0]:original_contact_style_table[0]);
        for(auto pointer:{0u,address+1,stack-0xc0}){s->memory.write<4>(toc+slot,pointer);compare(*s,o,10);}
        s->memory.write<4>(toc+slot,address);s->memory.write<4>(weight_table+0x600,0x101f68d4);o.mutate_table=true;o.table_slot=slot;o.table_replacement=weight_table+0x600;boundaries(*s,o,10);
    }
    for(auto actor_pointer:{0u,actor})for(auto receiver_pointer:{0u,receiver})for(auto flag:{0u,1u})for(auto kind:{0u,5u,7u}) {
        auto s=seed();s->wr<4>(16,actor_pointer);s->wr<4>(24,receiver_pointer);s->wr<4>(20,kind);s->wr<4>(28,kind);s->wr<4>(32,flag);boundaries(*s,{},11);
    }
    for(auto descriptor:{0u,1u,2u,3u,4u,5u,6u,7u,8u,9u,10u,128u,255u})for(auto draw:{0u,3u,6u,8u,9u,10u,11u,80u,0xffffffffu})for(auto owns:{0u,actor,receiver}) {
        auto s=seed();s->wr<4>(20,0);s->wr<4>(28,0);s->memory.write<4>(match+0xe000,owns);Options o;o.descriptor=descriptor;o.random.assign(24,draw);compare(*s,o,11);
    }
    for(auto kind:{0u,5u})for(auto mode:{0u,4u}){auto s=seed();s->wr<4>(20,kind);s->wr<4>(28,kind);s->memory.write<1>(match+0x791f,mode);Options o;o.mutate_context=true;o.mutate_players=true;boundaries(*s,o,11);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*seed(),o,11);}
    for(auto pc=contact::description_entry;pc<contact::description_exit_phase;pc+=4)if(!sites.contains(pc))std::cerr<<"Uncovered "<<hex_address(pc)<<'\n';require(sites.size()==1404,"All 1404 description original sites covered");
    std::cout<<"PASS: "<<cases<<" description component comparisons; "<<completed<<" completions, "<<interruptions<<" interruptions, "<<negatives<<" own/child negatives; all "<<sites.size()<<" original sites; full state/effects/ABI/accounting; independent original references\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
