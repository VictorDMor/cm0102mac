#include "match_event.hpp"
#include "semantic_io_checks.hpp"
#include <fstream>
#include <set>
namespace cm::aot {bool match_event_reference(State&);}
namespace {
using namespace cm::semantic_io_checks;
namespace selection=cm::aot::match_event;
constexpr auto queue=fixture+0x10000,match=fixture+0x20000,packet=fixture+0x30000,other=packet+0x100,catalog=fixture+0x40000,weights=fixture+0x48000,doubles=fixture+0x60000;
constexpr auto cursor=fixture+0x20,salt=fixture+0x24,pool_end=fixture+0x28,pool=fixture+0x1000,logging=fixture+0x62000;
constexpr std::array<std::uint32_t,9> ends{0x101e31a0,0x101fea8c,0x101ff5ec,0x101ffb68,0x10201078,0x102010c0,0x10201180,0x10201e30,0x10201ee4};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{selection::actor_address,selection::ball_cell_side,selection::qualify_event,selection::sample_event_jitter,selection::select_transition,selection::same_actor_pair,selection::compatible_actor_context,selection::emit_event,selection::classify_event};
struct Trace:SemanticGuestObserver{std::vector<std::uint32_t> order;std::vector<std::pair<std::uint32_t,std::uint8_t>> forced;std::vector<std::array<std::uint32_t,3>> registers;void before_step(State& s,std::uint32_t pc)override{if(s.executed<s.budget){for(auto [site,cr]:forced)if(site==pc)s.wr<1>(2304,cr);for(auto m:registers)if(m[0]==pc)s.wr<4>(m[1]*4,m[2]);order.push_back(pc);}}};
thread_local Trace* original_trace=nullptr;
struct Event{std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Options{std::vector<std::uint32_t> draws{0};std::array<std::uint32_t,2> sides{1,0};bool own_negative=false,child_negative=false;unsigned interrupt=0,child_entry=5;std::uint32_t reaction=0;std::vector<unsigned> reactions;std::vector<std::array<std::uint32_t,3>> registers;std::vector<std::pair<std::uint32_t,std::uint8_t>> forced;};
struct Model:SemanticDependencyDispatch{
    bool original;Options options;Trace* trace;std::vector<Event> calls;unsigned draws=0,reactions=0,child_mutations=0;
    Model(bool old,const Options& o,Trace* t):original(old),options(o),trace(t){}
    void reference(State& s,std::uint32_t until){
        auto it=std::find(selection::entries.begin(),selection::entries.end(),s.pc);require(it!=selection::entries.end(),"Independent event starts at complete root");
        auto index=unsigned(it-selection::entries.begin());auto caller_stack=std::uint32_t(s.r<4>(4));bool first_call=true;
        while(s.pc!=until){if((!first_call&&std::find(selection::entries.begin(),selection::entries.end(),s.pc)!=selection::entries.end())||!match_event_reference(s))call(s,s.pc,s.r<4>(4128));
            if(!trace->order.empty()&&trace->order.back()==ends[index]-4&&s.r<4>(4)==caller_stack)return;first_call=false;}
    }
    void call(State& s,std::uint32_t entry,std::uint32_t continuation)override{
        const auto n=calls.size();calls.push_back({entry,continuation,s.registers,{}});const auto own=std::find(selection::entries.begin(),selection::entries.end(),entry);
        if(own!=selection::entries.end()){
            if(original)reference(s,continuation);else methods[unsigned(own-selection::entries.begin())](s,this,trace);
            if(options.child_negative&&!original&&entry==selection::entries[options.child_entry]){s.wr<4>(12,s.r<4>(12)^1);++child_mutations;}
        }else{
            std::uint32_t value=0;
            if(entry==0x1077b770)value=options.draws[draws++%options.draws.size()];
            else if(entry==0x101eea30)value=options.sides[s.r<4>(16)&1];
            else if(entry==0x10739890){s.memory.write<4>(s.r<4>(24),0x12345678);s.memory.write<4>(s.r<4>(28),0x87654321);}
            else if(entry==0x10768810)value=logging+0x100;
            else if(entry==0x1077b720){s.memory.write<1>(s.r<4>(12),42);value=1;}
            else if(entry==0x10769b60){s.memory.write<1>(fixture+0x80,1);}
            else if(entry==0x101f7c10){s.memory.write<1>(s.r<4>(16),7);}
            else if(entry==0x10289d50){s.memory.write<1>(s.r<4>(12)+0x120,4);value=options.reactions.empty()?options.reaction:options.reactions[reactions++%options.reactions.size()];}
            else throw std::runtime_error("Unknown event dependency "+hex_address(entry));
            for(unsigned reg=0;reg<=12;++reg)if(reg!=1&&reg!=2)s.wr<4>(reg*4,0xac000000+reg*71);
            for(unsigned reg=0;reg<=13;++reg)s.wr<8>(0x4000+reg*16,float_bits<8>(-17.0-reg));
            s.wr<1>(2304,5);s.wr<4>(12,value);s.pc=continuation;
        }calls[n].after=s.registers;
        if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Fixture event dependency interrupted");
    }
};
void fill_pool(State& s,std::uint32_t value){for(unsigned off=0;off<=0x100;off+=2)s.memory.write<2>(pool+off,value);}
std::unique_ptr<State> seed(unsigned index){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned n=1280;n<1312;++n)s->wr<1>(n,0);
    s->memory.map(fixture,0x90000);s->memory.map(stack-0x4000,0x5000);s->memory.map(toc-0x8000,0x9000);
    s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->pc=selection::entries[index];s->wr<4>(12,queue);s->wr<4>(16,packet);s->wr<4>(20,match);s->wr<4>(24,0);
    s->memory.write<4>(toc-0x52a4,doubles);s->memory.write<8>(doubles,float_bits<8>(65535));s->memory.write<8>(doubles+8,UINT64_C(0x4330000080000000));
    s->memory.write<4>(toc-0x7b90,salt);s->memory.write<4>(toc-0x7b8c,pool);s->memory.write<4>(toc-0x7b88,pool_end);s->memory.write<4>(toc-0x7b84,cursor);
    s->memory.write<4>(cursor,pool);s->memory.write<4>(pool_end,pool+0x100);fill_pool(*s,0);
    s->memory.write<4>(toc-0x5298,catalog);s->memory.write<4>(toc-0x7ba8,logging);s->memory.write<4>(toc-0x52a0,logging+0x1000);s->memory.write<4>(toc-0x7bac,fixture+0x80);
    for(unsigned n=0;n<0x2a5;++n){s->memory.write<1>(catalog+n*20,0);s->memory.write<1>(catalog+n*20+4,1);s->memory.write<4>(catalog+n*20+8,weights);s->memory.write<2>(catalog+n*20+12,2);s->memory.write<4>(catalog+n*20+16,100);}
    s->memory.write<2>(weights,50);s->memory.write<2>(weights+0x10c,50);
    for(unsigned slot=0;slot<12;++slot){s->memory.write<2>(queue+0x30+slot*20,0x1f40);s->memory.write<1>(queue+0x40+slot*20,255);}
    s->memory.write<2>(packet,0x1f40);s->memory.write<2>(other,0x1f40);s->memory.write<1>(packet+7,255);s->memory.write<1>(other+7,255);s->memory.write<2>(match+0x7948,100);
    for(unsigned side=0;side<2;++side)for(unsigned slot=0;slot<20;++slot){auto actor=match+0x311c+side*0x2350+slot*0x1c4;auto record=fixture+0x70000+(side*20+slot)*0x100;s->memory.write<4>(actor+0x1a4,match);s->memory.write<4>(actor+0x6c,record);s->memory.write<4>(actor+0x70,record);s->memory.write<1>(actor+0x27,side);}
    if(index==0||index==1){s->wr<4>(12,match);s->wr<4>(16,0);s->wr<4>(20,0);}
    if(index==2){s->wr<4>(20,0);s->wr<4>(24,0);}
    if(index==3)s->wr<4>(12,10);
    if(index==4)s->memory.write<2>(queue+8,1);
    if(index==5||index==6){s->wr<4>(16,packet);s->wr<4>(20,other);}
    if(index==7){s->wr<4>(16,0x1f40);s->wr<4>(24,0);s->wr<4>(28,0);s->wr<4>(32,255);s->wr<4>(36,255);s->wr<4>(40,0);}
    if(index==8)s->wr<4>(16,0x1f40);
    s->budget=100000;return s;
}
unsigned cases=0,returns=0,interruptions=0,negatives=0;std::array<unsigned,9> root_returns{};std::array<std::set<std::uint32_t>,9> complete_sites,ordinary_sites,checkpoint_sites;unsigned checkpoint_returns=0;
struct Result{std::unique_ptr<State> state;std::vector<Effects::Access> accesses;unsigned calls;};
Result compare(const State& input,unsigned index,const Options& options={}){
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    Trace before,after;before.forced=after.forced=options.forced;before.registers=after.registers=options.registers;Model old(true,options,&before),modern(false,options,&after);
    const auto run=[&](State& s,Model& m){try{if(m.original){original_trace=m.trace;m.reference(s,return_sentinel);}else methods[index](s,&m,m.trace);}
        catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}original_trace=nullptr;return std::string{};};
    auto expected=run(*a,old),actual=run(*b,modern);a->memory.date_observer=b->memory.date_observer=nullptr;
    auto label="Event root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected==actual,label+" original="+expected+" modern="+actual);require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Fixture event dependency interrupted"||expected=="Signed division by zero in translated code"||expected.starts_with("Unmapped data access at "),label+" unexpected boundary="+expected);
    if(options.own_negative)b->wr<4>(12,b->r<4>(12)^1);
    if(options.child_negative)require(modern.child_mutations>0,label+" RNG child negative not exercised");
    bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative&&!options.child_negative)throw;rejected=true;}
    require(rejected==(options.own_negative||options.child_negative),label+" negative must discriminate");
    if(!rejected){require(x.bytes==y.bytes,label+" ordered memory effects");require(a->executed==b->executed,label+" instruction count");require(old.calls==modern.calls,label+" dependency ABI");if(before.order!=after.order){auto d=std::mismatch(before.order.begin(),before.order.end(),after.order.begin(),after.order.end());auto n=d.first-before.order.begin();throw std::runtime_error(label+" source order offset="+std::to_string(n)+" old="+(d.first==before.order.end()?"end":hex_address(*d.first))+" new="+(d.second==after.order.end()?"end":hex_address(*d.second)));}}
    ++cases;interruptions+=!expected.empty();negatives+=rejected;
    if(expected.empty()&&!rejected){++returns;++root_returns[index];for(auto pc:before.order)if(selection::entries[index]<=pc&&pc<ends[index]){complete_sites[index].insert(pc);((options.forced.empty()&&options.registers.empty())?ordinary_sites:checkpoint_sites)[index].insert(pc);}checkpoint_returns+=!options.forced.empty()||!options.registers.empty();}
    return{std::move(a),x.log,unsigned(old.calls.size())};
}
}
namespace cm::aot{void match_event_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try{
    require(argc==2,"Expected event coverage output");
    for(unsigned index=0;index<9;++index){auto input=seed(index);compare(*input,index);Options o;o.own_negative=true;compare(*input,index,o);}
    for(auto side:{0u,1u,127u,128u,255u,0x100u,0xabcdef01u})for(auto slot:{0u,1u,19u,127u,128u,255u}){auto input=seed(0);input->wr<4>(16,side);input->wr<4>(20,slot);auto result=compare(*input,0);auto expected=match+0x311c+std::uint32_t(std::int8_t(side))*0x2350+std::uint32_t(std::int8_t(slot))*0x1c4;require(result.state->r<4>(12)==expected,"Independent actor address formula");}
    for(auto left:{0u,1u})for(auto right:{0u,1u}){Options o;o.sides={left,right};compare(*seed(1),1,o);}
    for(unsigned index=5;index<=6;++index)for(unsigned byte=5;byte<=8;++byte)for(auto value:{0u,1u,127u,128u,255u}){auto input=seed(index);input->memory.write<1>(other+byte,value);compare(*input,index);}
    for(unsigned code=0x1f30;code<=0x21f0;++code){auto input=seed(8);input->wr<4>(16,code);auto result=compare(*input,8);int expected=0;
        if((0x2021<code&&code<0x20d0)||code==0x1f46||code==0x1f47)expected=1;
        if(code==0x1f75||code==0x1f79||code==0x2130||code==0x2131)expected=-1;
        if(code==0x213e||code==0x2147)expected=-2;
        require(std::int32_t(result.state->r<4>(12))==expected,"Independent event classification");}
    for(auto count:{0u,1u,2u,10u,65535u,65536u,1000000u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto draw:{0u,1u,65535u}){auto input=seed(3);input->wr<4>(12,count);fill_pool(*input,draw);compare(*input,3);}
    for(auto code:std::vector<unsigned>{0x1f41,0x1f42,0x1f44,0x1f48,0x1f49,0x1f4a,0x1f4b,0x1f86,0x1f8b,0x1fa2,0x1fa6,0x1fbf,0x1fe6,0x1fee,0x205a,0x205b,0x2079,0x2112,0x211f,0x2150,0x216f})for(auto mode:{0u,1u,2u,8u,64u})for(auto stage:{0u,2u,3u,5u,255u})for(auto phase:{0u,3u,5u})for(auto draw:{0u,1u}){
        auto input=seed(2);input->memory.write<2>(packet,code);input->wr<4>(20,(stage<<24)|(phase<<16));input->wr<4>(24,mode);fill_pool(*input,draw);compare(*input,2);
    }
    const std::vector<unsigned> codes{0x1f40,0x1f41,0x1f43,0x1f44,0x1f45,0x1f46,0x1f47,0x1f4a,0x1f4d,0x1f53,0x1f55,0x1f5d,0x1f5e,0x1f5f,0x1f60,0x1f62,0x1f69,0x1f6c,0x1f6e,0x1f6f,0x1f70,0x1f71,0x1f72,0x1f73,0x1f75,0x1f76,0x1f7d,0x1f7e,0x1f80,0x1f84,0x1f85,0x1f86,0x1f92,0x1f9d,0x1fbe,0x1fe6,0x1fe7,0x2021,0x2076,0x20b9,0x20c4,0x20c5,0x20d0,0x20de,0x210f,0x2111,0x2113,0x2130,0x2131,0x213e,0x2148,0x214a,0x214c,0x214f,0x2150,0x215d,0x216e};
    for(auto current:codes)for(auto previous:codes)for(unsigned pattern=0;pattern<3;++pattern){
        auto input=seed(4);input->memory.write<2>(packet,current);input->memory.write<2>(queue+0x30,previous);
        for(unsigned byte=5;byte<=8;++byte){input->memory.write<1>(packet+byte,pattern==2?255:0);input->memory.write<1>(queue+0x30+byte,pattern==1?1:0);}
        fill_pool(*input,pattern==2?65535:pattern);compare(*input,4);
    }
    auto wildcard=seed(6);wildcard->memory.write<1>(other+8,1);compare(*wildcard,6);
    for(unsigned index=0;index<9;++index)for(auto cut:{0u,1u,2u,3u,7u,15u,31u,63u,127u,255u}){auto input=seed(index);input->budget=cut;compare(*input,index);}
    for(auto code:std::vector<unsigned>{0x1f40,0x216f,0x1fee,0x1f42,0x1f4b,0x1f48})for(auto mode:{16u,32u})for(auto draw:{0u,1u})for(auto rollover:{0u,1u}){auto input=seed(2);input->memory.write<2>(packet,code);input->wr<4>(24,mode);fill_pool(*input,draw);if(rollover)input->memory.write<4>(pool_end,pool);compare(*input,2);}
    for(auto current:codes)for(auto previous:codes){auto input=seed(4);input->memory.write<2>(packet,current);input->memory.write<2>(queue+0x30,previous);input->memory.write<1>(packet+5,0);input->memory.write<1>(packet+6,0);input->memory.write<1>(packet+7,1);input->memory.write<1>(packet+8,0);input->memory.write<1>(queue+0x35,1);input->memory.write<1>(queue+0x36,0);input->memory.write<1>(queue+0x37,0);input->memory.write<1>(queue+0x38,0);input->memory.write<4>(queue+0x3c,(current+previous)%2?1:20);input->memory.write<4>(pool_end,pool);compare(*input,4);}
    auto absent=seed(4);absent->wr<4>(20,0);compare(*absent,4);auto empty=seed(4);empty->memory.write<2>(queue+8,0);compare(*empty,4);
    Options reverse;reverse.sides={0,1};for(auto current:codes){auto input=seed(4);input->memory.write<2>(packet,current);input->memory.write<2>(queue+0x30,0x2021);compare(*input,4,reverse);}
    for(auto right:{0u,1u,255u}){auto input=seed(6);input->memory.write<1>(packet+7,1);input->memory.write<1>(other+7,right);compare(*input,6);}
    for(auto code:std::vector<unsigned>{0x1f40,0x1f47,0x1f75,0x213e,0x2147,0x2153,0x21bf,0x21c0,0x21e4,0x21e5})for(auto type:{0u,1u,2u,3u})for(auto history:{0u,1u,3u})for(auto slot:{0u,11u}){
        auto input=seed(7);input->wr<4>(16,code);input->memory.write<1>(match+0x7929,2);input->memory.write<2>(queue+8,slot);input->memory.write<2>(queue+0xc,slot);input->memory.write<2>(queue+0xe,slot==0?10:9);input->memory.write<1>(queue+2,1);input->memory.write<1>(queue+3,1);
        if(code<=0x21e4){auto record=catalog+(code-0x1f40)*20;input->memory.write<1>(record,type);input->memory.write<1>(record+4,history);input->memory.write<1>(record+7,1);input->memory.write<4>(record+16,100000);}
        fill_pool(*input,99);Options o;o.reaction=type%2;compare(*input,7,o);
    }
    for(auto stamp:{0u,1u,0x1efu,0x3deu,0x483u,0x528u,0xffffu}){auto input=seed(7);input->memory.write<2>(match+0x794c,stamp);compare(*input,7);input->memory.write<1>(match,1);compare(*input,7);}
    for(auto side:{2u,128u,255u}){auto input=seed(7);input->wr<4>(28,side);compare(*input,7);}
    for(auto value:{0u,1u}){auto input=seed(7);auto club=fixture+0x82000;input->memory.write<4>(match+0x3118,club);input->memory.write<4>(club+0x18,club+0x100);input->memory.write<1>(club+0x14c,value);compare(*input,7);}
    for(auto side:{0u,1u})for(auto code:{0x1f46u,0x1f75u,0x213eu,0x2147u,0x2153u}){auto input=seed(7);input->wr<4>(16,code);input->wr<4>(28,side);input->memory.write<1>(match+0x7929,2);input->memory.write<1>(queue+1,!side);input->memory.write<1>(queue+4,0);Options o;o.reaction=1;compare(*input,7,o);}
    auto mapped_null=seed(7);mapped_null->memory.map(0,0x10000);mapped_null->memory.copy_in(0,mapped_null->memory.copy_out(match,0x10000));mapped_null->wr<4>(20,0);compare(*mapped_null,7);
    Options child;child.child_negative=true;child.child_entry=4;compare(*seed(7),7,child);
    for(auto current:codes)for(auto previous:codes){auto input=seed(4);input->memory.write<2>(packet,current);input->memory.write<2>(queue+0x30,previous);input->memory.write<4>(packet+0xc,10);input->memory.write<4>(queue+0x3c,10);input->memory.write<1>(packet+5,0);input->memory.write<1>(packet+6,0);input->memory.write<1>(packet+7,1);input->memory.write<1>(packet+8,0);input->memory.write<1>(queue+0x35,1);input->memory.write<1>(queue+0x36,0);input->memory.write<1>(queue+0x37,0);input->memory.write<1>(queue+0x38,0);fill_pool(*input,1);compare(*input,4);}
    for(auto pair:std::vector<std::pair<unsigned,unsigned>>{{0x1f4a,0x1f4d},{0x1f75,0x213e},{0x1f75,0x1f71},{0x1f75,0x1f72}}){auto input=seed(4);input->memory.write<2>(packet,pair.first);input->memory.write<2>(queue+0x30,pair.second);input->memory.write<1>(packet+7,1);input->memory.write<1>(queue+0x37,255);fill_pool(*input,1);compare(*input,4);}
    auto direction=seed(4);direction->memory.write<2>(packet,0x1f46);direction->memory.write<2>(queue+0x30,0x2021);direction->memory.write<1>(queue+0x36,1);Options sides;sides.sides={0,1};compare(*direction,4,sides);
    for(auto count:{0u,1u,11u}){auto input=seed(7);input->wr<4>(16,0x1f6f);input->memory.write<2>(queue+8,count==0?1:0);input->memory.write<2>(queue+6,count);input->memory.write<2>(queue+0x978,0x1f40);compare(*input,7);}
    auto remap=seed(7);remap->wr<4>(16,0x1f62);remap->memory.write<2>(queue+8,1);remap->memory.write<2>(queue+0x30,0x2113);compare(*remap,7);
    for(auto flag:{1u,3u})for(auto type:{0u,1u,2u})for(auto follow:{0u,1u}){auto input=seed(7);input->memory.write<1>(match+0x7929,2);input->memory.write<1>(queue,flag);input->memory.write<1>(queue+2,follow);input->memory.write<2>(queue+8,1);input->memory.write<2>(queue+0xc,10);input->memory.write<2>(queue+0xe,11);input->memory.write<1>(catalog,type);input->memory.write<1>(catalog+7,1);input->memory.write<1>(catalog+2,follow);compare(*input,7);}
    for(auto old_code:{0u,1u,0xffffu}){auto input=seed(7);input->memory.write<1>(match+0x7929,2);input->memory.write<1>(queue,1);input->memory.write<1>(queue+2,1);input->memory.write<2>(queue+8,1);input->memory.write<2>(queue+0xc,10);input->memory.write<1>(catalog+7,1);input->memory.write<2>(queue+0x30,old_code);compare(*input,7);}
    for(auto old_code:{0u,1u}){auto input=seed(7);input->memory.write<1>(match+0x7929,2);input->memory.write<2>(queue+0xe,10);input->memory.write<1>(catalog+4,3);input->memory.write<2>(queue+0x30+11*20,old_code);compare(*input,7);}
    auto history_wrap=seed(7);history_wrap->memory.write<2>(queue+0xe,11);history_wrap->memory.write<1>(catalog+4,2);compare(*history_wrap,7);
    auto cursor_wrap=seed(7);cursor_wrap->memory.write<2>(queue+8,10);cursor_wrap->memory.write<2>(queue+0xe,11);cursor_wrap->memory.write<1>(catalog+4,0);compare(*cursor_wrap,7);
    auto stamp=seed(7);stamp->memory.write<2>(match+0x794c,1000);compare(*stamp,7);
    auto zero_weight=seed(7);zero_weight->memory.write<4>(catalog+16,0);compare(*zero_weight,7);
    auto cap=seed(7);cap->memory.write<4>(catalog+16,100000);cap->memory.write<8>(doubles,float_bits<8>(1));cap->memory.write<4>(pool_end,pool);fill_pool(*cap,1);compare(*cap,7);
    auto second_reaction=seed(7);second_reaction->wr<4>(16,0x2153);second_reaction->memory.write<1>(match+0x7929,2);Options reaction;reaction.reactions={0,1};compare(*second_reaction,7,reaction);
    for(auto code:{0x1f46u,0x213eu}){auto input=seed(7);input->wr<4>(16,code);input->memory.write<1>(queue+1,1);input->memory.write<2>(match+0x77c2,9);input->memory.write<1>(match+0x77c4,1);compare(*input,7);}
    auto null_follow=seed(7);null_follow->memory.map(0,0x10000);null_follow->memory.copy_in(0,null_follow->memory.copy_out(match,0x10000));null_follow->wr<4>(20,0);null_follow->wr<4>(16,0x213e);null_follow->memory.write<1>(queue+1,1);compare(*null_follow,7);
    for(auto draw:{0u,1u,65534u}){auto input=seed(3);input->wr<4>(12,65536);fill_pool(*input,draw);input->memory.write<4>(pool_end,pool);input->memory.write<8>(doubles,float_bits<8>(1));compare(*input,3);}
    // Explicit register checkpoints exercise compiler-expanded RNG arms whose
    // nested derived bounds cannot naturally exceed the original thresholds.
    for(auto draw:{0u,1u,65534u}){auto input=seed(3);input->wr<4>(12,65536);fill_pool(*input,draw);input->memory.write<4>(pool_end,pool);Options o;o.registers={{0x101ff738,22,65536},{0x101ff824,21,65536},{0x101ff910,20,65536}};compare(*input,3,o);input->memory.write<8>(doubles,float_bits<8>(1));compare(*input,3,o);}
    for(auto condition:{2u,4u}){auto input=seed(4);input->memory.write<2>(packet,0x1f47);input->memory.write<2>(queue+0x30,0x2021);input->memory.write<4>(queue+0x3c,10);fill_pool(*input,1);Options o;o.forced={{condition==2?0x10200a7cu:0x10200abcu,std::uint8_t(condition)}};compare(*input,4,o);}
    auto classifier_guard=seed(7);classifier_guard->wr<4>(16,0x1f75);classifier_guard->memory.write<1>(queue+1,1);Options guard;guard.forced={{0x10201cc4,8}};compare(*classifier_guard,7,guard);
    auto catalog_guard=seed(7);Options catalog_case;catalog_case.registers={{0x10201364,24,0x1f3f}};compare(*catalog_guard,7,catalog_case);
    for(unsigned variant=0;variant<4;++variant)for(auto denominator:{1.0,65535.0}){auto input=seed(3);input->wr<4>(12,65536);fill_pool(*input,1);input->memory.write<4>(pool_end,pool);input->memory.write<8>(doubles,float_bits<8>(denominator));Options o;
        if(variant==0)o.forced={{0x101ff744,4}};
        if(variant==1){o.registers={{0x101ff738,22,65536}};o.forced={{0x101ff830,4}};}
        if(variant==2)o.registers={{0x101ff738,22,65536},{0x101ff824,21,65536}};
        if(variant==3)o.registers={{0x101ff738,22,65536},{0x101ff824,21,65536},{0x101ff910,20,65536}};
        compare(*input,3,o);
    }
    auto long_cap=seed(4);long_cap->memory.write<2>(packet,0x1f47);long_cap->memory.write<2>(queue+0x30,0x2021);long_cap->memory.write<4>(queue+0x3c,10);long_cap->memory.write<8>(doubles,float_bits<8>(1));fill_pool(*long_cap,1);Options long_arm;long_arm.forced={{0x10200abc,4}};compare(*long_cap,4,long_arm);
    // This older rule is shadowed by the earlier 1f86..1fbe rule for the
    // same previous event. Bypass only that earlier condition on both sides.
    auto shadowed=seed(4);shadowed->memory.write<2>(packet,0x1f9d);shadowed->memory.write<2>(queue+0x30,0x1f40);Options shadow;shadow.forced={{0x101ffd28,4}};compare(*shadowed,4,shadow);
    auto skip_scan=seed(7);skip_scan->memory.write<1>(queue,1);skip_scan->memory.write<1>(catalog+7,1);compare(*skip_scan,7);
    for(auto slot:{0u,10u}){auto input=seed(7);input->memory.write<1>(queue+1,255);input->memory.write<2>(queue+8,slot);input->memory.write<2>(queue+0xe,slot+1);input->memory.write<1>(catalog+4,0);compare(*input,7);}
    for(unsigned index=0;index<9;++index){auto input=seed(index);input->memory.map(fixture+0x99000,0x9000);input->memory.copy_in(fixture+0x99000,input->memory.copy_out(toc-0x8000,0x9000));input->wr<4>(8,fixture+0xa1000);compare(*input,index);}
    auto alias=seed(2);alias->memory.copy_in(stack+0x20,alias->memory.copy_out(packet,20));alias->wr<4>(16,stack+0x20);compare(*alias,2);
    for(unsigned index=1;index<=7;++index){auto input=seed(index);if(index==3)input->memory.write<4>(toc-0x7b84,0);else input->wr<4>((index==1||index==7)?12:16,0);auto before=interruptions;compare(*input,index);require(interruptions==before+1,"Required event pointer must fault");}
    for(auto draw:{0u,1u}){auto input=seed(3);input->wr<4>(12,65536);fill_pool(*input,draw);input->memory.write<4>(pool_end,pool);Options o;o.registers={{0x101ff738,22,65536}};compare(*input,3,o);}
    std::ofstream coverage(argv[1]);unsigned total=0;constexpr std::array<unsigned,9> expected{8,31,263,350,1346,16,48,810,45};
    for(unsigned index=0;index<9;++index){for(auto pc:complete_sites[index])coverage<<index<<" "<<hex_address(pc)<<'\n';require(root_returns[index]>0&&complete_sites[index].size()==expected[index],"Complete event source sites root="+hex_address(selection::entries[index])+" sites="+std::to_string(complete_sites[index].size()));total+=complete_sites[index].size();std::cout<<"Root "<<hex_address(selection::entries[index])<<" complete_sites="<<complete_sites[index].size()<<"/"<<expected[index]<<" ordinary_sites="<<ordinary_sites[index].size()<<" checkpoint_sites="<<checkpoint_sites[index].size()<<" returns="<<root_returns[index]<<'\n';}
    std::cout<<"Match event comparisons="<<cases<<" returns="<<returns<<" interruptions="<<interruptions<<" negatives="<<negatives<<" checkpoint_returns="<<checkpoint_returns<<" completed_sites="<<total<<"/2917\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
