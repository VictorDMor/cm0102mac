#include "actor_encounter.hpp"
#include "semantic_io_checks.hpp"
#include <fstream>
#include <set>
namespace cm::aot {bool actor_encounter_reference(State&);}
namespace {
using namespace cm::semantic_io_checks;
namespace selection=cm::aot::actor_encounter;
constexpr auto match=fixture+0x10000,first=match+0x311c,profile=fixture+0x50000,constants=fixture+0x60000,doubles=constants+0x1000;
constexpr auto cursor=fixture+0x20,salt=fixture+0x24,pool_end=fixture+0x28,pool=fixture+0x1000,club=fixture+0x70000;
constexpr std::array<std::uint32_t,4> ends{0x101d68ec,0x10286650,0x102875f0,0x10288228};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{selection::scale_team_scores,selection::scale_actor_score,selection::accept_response,selection::resolve_peer};
struct Trace:SemanticGuestObserver{std::vector<std::uint32_t> order;std::vector<std::pair<std::uint32_t,std::uint8_t>> forced;void before_step(State& s,std::uint32_t pc)override{if(s.executed<s.budget){for(auto [site,cr]:forced)if(site==pc)s.wr<1>(2304,cr);order.push_back(pc);}}};
thread_local Trace* original_trace=nullptr;
struct Event{std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Options{std::vector<std::uint32_t> draws{1};std::array<double,8> power{1,1,1,1,1,1,1,1};bool own_negative=false,child_negative=false;unsigned interrupt=0;std::uint32_t contact=1;std::vector<std::pair<std::uint32_t,std::uint8_t>> forced;};
struct Model:SemanticDependencyDispatch{
    bool original;Options options;Trace* trace;std::vector<Event> calls;unsigned draws=0,child_mutations=0;
    Model(bool old,const Options& o,Trace* t):original(old),options(o),trace(t){}
    void reference(State& s,std::uint32_t until){
        auto it=std::find(selection::entries.begin(),selection::entries.end(),s.pc);require(it!=selection::entries.end(),"Independent encounter starts at complete root");
        auto index=unsigned(it-selection::entries.begin());auto caller_stack=std::uint32_t(s.r<4>(4));bool first_call=true;
        while(s.pc!=until){if((!first_call&&std::find(selection::entries.begin(),selection::entries.end(),s.pc)!=selection::entries.end())||!actor_encounter_reference(s))call(s,s.pc,s.r<4>(4128));
            if(!trace->order.empty()&&trace->order.back()==ends[index]-4&&s.r<4>(4)==caller_stack)return;first_call=false;}
    }
    void call(State& s,std::uint32_t entry,std::uint32_t continuation)override{
        const auto n=calls.size();calls.push_back({entry,continuation,s.registers,{}});const auto own=std::find(selection::entries.begin(),selection::entries.end(),entry);
        if(own!=selection::entries.end()){
            if(original)reference(s,continuation);else methods[unsigned(own-selection::entries.begin())](s,this,trace);
            if(options.child_negative&&!original&&entry==selection::entries[1]){s.wr<4>(12,s.r<4>(12)^1);++child_mutations;}
        }else{
            double power=0;std::uint32_t value=0;
            if(entry==0x10780fc0){power=options.power[s.r<4>(120)&7];s.memory.write<4>(s.r<4>(4)+0x14,s.r<4>(8));}
            else if(entry==0x1077b770||entry==0x10226520)value=options.draws[draws++%options.draws.size()];
            else if(entry==0x101f5050){value=options.contact;for(unsigned r=8;r<=10;++r)s.memory.write<1>(s.r<4>(r*4),r-7);}
            else if(entry==0x102374f0){s.memory.write<1>(s.r<4>(12)+0x105,s.r<4>(16));s.memory.write<1>(s.r<4>(12)+0x106,s.r<4>(20));}
            else if(entry==0x10201180){auto address=fixture+0x800; s.memory.write<4>(address,s.memory.read<4>(address)+1);}
            else throw std::runtime_error("Unknown encounter dependency "+hex_address(entry));
            for(unsigned reg=0;reg<=12;++reg)if(reg!=1&&reg!=2)s.wr<4>(reg*4,0xac000000+reg*71);
            for(unsigned reg=0;reg<=13;++reg)s.wr<8>(0x4000+reg*16,float_bits<8>(-17.0-reg));
            s.wr<1>(2304,5);s.wr<4>(12,value);
            if(entry==0x10780fc0){s.wr<8>(0x4010,float_bits<8>(power));s.wr<4>(8,0xbad00002);}
            s.pc=continuation;
        }calls[n].after=s.registers;
        if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Fixture encounter dependency interrupted");
    }
};
std::unique_ptr<State> seed(unsigned index){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned n=1280;n<1312;++n)s->wr<1>(n,0);
    s->memory.map(fixture,0x80000);s->memory.map(stack-0x4000,0x5000);s->memory.map(toc-0x8000,0x9000);
    s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->pc=selection::entries[index];s->wr<4>(12,index==0?match:first);s->wr<4>(16,index==2?1:0);s->wr<4>(20,0);s->wr<8>(0x4010,float_bits<8>(0.8));
    s->memory.write<4>(toc-0x51e8,doubles);s->memory.write<4>(toc-0x51e0,constants);
    for(unsigned off=0;off<0x400;off+=8)s->memory.write<8>(doubles+off,float_bits<8>(1));
    s->memory.write<8>(doubles+0x80,UINT64_C(0x4330000080000000));s->memory.write<8>(doubles+0x88,float_bits<8>(0));s->memory.write<8>(doubles,float_bits<8>(65535));
    s->memory.write<8>(doubles+0x3a8,float_bits<8>(100));s->memory.write<8>(doubles+0xc8,float_bits<8>(1.5));
    for(unsigned off=0;off<0x200;off+=4)s->memory.write<4>(constants+off,float_bits<4>(0.8));s->memory.write<4>(constants+0x1c,float_bits<4>(100));
    s->memory.write<4>(toc-0x7b90,salt);s->memory.write<4>(toc-0x7b8c,pool);s->memory.write<4>(toc-0x7b88,pool_end);s->memory.write<4>(toc-0x7b84,cursor);
    s->memory.write<4>(cursor,pool);s->memory.write<4>(pool_end,pool+0x100);s->memory.write<4>(salt,0);for(unsigned off=0;off<=0x100;off+=2)s->memory.write<2>(pool+off,1);
    s->memory.write<1>(club+0x119,8);
    for(unsigned side=0;side<2;++side)for(unsigned slot=0;slot<20;++slot){
        const auto actor=first+side*0x2350+slot*0x1c4,record=profile+(side*20+slot)*0x100;
        s->memory.write<1>(actor+0x19,slot==0?0:255);s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x2f,slot);s->memory.write<4>(actor+0x6c,record);s->memory.write<4>(actor+0x70,record);s->memory.write<4>(actor+0x1a4,match);s->memory.write<4>(actor+0x1a8,float_bits<4>(60));s->memory.write<4>(record+0xc,club);s->memory.write<1>(record+0x69,10);
    }
    s->memory.write<1>(match+0xa08+0x59,1);s->memory.write<4>(match+0xa08+0x2c,first+0x2350);
    s->budget=200000;return s;
}
unsigned cases=0,returns=0,interruptions=0,negatives=0;std::array<unsigned,4> root_returns{};std::array<std::set<std::uint32_t>,4> complete_sites,ordinary_sites,checkpoint_sites;unsigned checkpoint_returns=0;
struct Result{std::unique_ptr<State> state;std::vector<Effects::Access> accesses;unsigned calls;};
Result compare(const State& input,unsigned index,const Options& options={}){
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    Trace before,after;before.forced=after.forced=options.forced;Model old(true,options,&before),modern(false,options,&after);
    const auto run=[&](State& s,Model& m){try{if(m.original){original_trace=m.trace;m.reference(s,return_sentinel);}else methods[index](s,&m,m.trace);}
        catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}original_trace=nullptr;return std::string{};};
    auto expected=run(*a,old),actual=run(*b,modern);a->memory.date_observer=b->memory.date_observer=nullptr;
    auto label="Encounter root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected==actual,label+" original="+expected+" modern="+actual);require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Fixture encounter dependency interrupted"||expected=="Signed division by zero in translated code"||expected.starts_with("Unmapped data access at "),label+" unexpected boundary="+expected);
    if(options.own_negative)b->wr<4>(12,b->r<4>(12)^1);
    if(options.child_negative)require(modern.child_mutations>0,label+" RNG child negative not exercised");
    bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative&&!options.child_negative)throw;rejected=true;}
    require(rejected==(options.own_negative||options.child_negative),label+" negative must discriminate");
    if(!rejected){require(x.bytes==y.bytes,label+" ordered memory effects");require(a->executed==b->executed,label+" instruction count");require(old.calls==modern.calls,label+" dependency ABI");if(before.order!=after.order){auto d=std::mismatch(before.order.begin(),before.order.end(),after.order.begin(),after.order.end());auto n=d.first-before.order.begin();throw std::runtime_error(label+" source order offset="+std::to_string(n)+" old="+(d.first==before.order.end()?"end":hex_address(*d.first))+" new="+(d.second==after.order.end()?"end":hex_address(*d.second)));}}
    ++cases;interruptions+=!expected.empty();negatives+=rejected;
    if(expected.empty()&&!rejected){++returns;++root_returns[index];for(auto pc:before.order)if(selection::entries[index]<=pc&&pc<ends[index]){complete_sites[index].insert(pc);(options.forced.empty()?ordinary_sites:checkpoint_sites)[index].insert(pc);}checkpoint_returns+=!options.forced.empty();}
    return{std::move(a),x.log,unsigned(old.calls.size())};
}
void actors(State& s,unsigned offset,unsigned bytes,std::uint64_t value){for(unsigned side=0;side<2;++side)for(unsigned slot=0;slot<20;++slot){auto address=first+side*0x2350+slot*0x1c4+offset;if(bytes==1)s.memory.write<1>(address,value);else if(bytes==2)s.memory.write<2>(address,value);else s.memory.write<4>(address,value);}}
void fill_pool(State& s,std::uint32_t value){for(unsigned off=0;off<=0x100;off+=2)s.memory.write<2>(pool+off,value);}
}
namespace cm::aot{void actor_encounter_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try{
    require(argc==2,"Expected encounter coverage output");
    for(unsigned index=0;index<4;++index){auto s=seed(index);compare(*s,index);Options own;own.own_negative=true;compare(*s,index,own);}
    Options child;child.child_negative=true;compare(*seed(0),0,child);
    for(unsigned index=0;index<2;++index)for(auto level:{0u,10u,127u,128u,255u})for(auto bits:{0u,0x42700000u,0xc2700000u,0x7f800000u,0xff800000u,0x7fc00123u}){
        auto s=seed(index);actors(*s,0x1a8,4,bits);for(unsigned slot=0;slot<40;++slot)s->memory.write<1>(profile+slot*0x100+0x69,level);compare(*s,index);
    }
    for(auto kind:{0u,1u,2u,3u,4u,5u,6u,128u,255u,0x103u})for(auto power:{-100000.0,-2.0,0.0,1.0,2.0,100000.0})for(auto draw:{0u,1u,65535u}){
        auto s=seed(2);s->wr<4>(16,kind);Options o;o.power.fill(power);fill_pool(*s,draw);compare(*s,2,o);
    }
    for(auto score:{-128.0,-1.0,0.0,109.0,110.0,111.0,127.0,128.0})for(auto draw:{0u,1u,65535u}){auto s=seed(2);s->memory.write<4>(first+0x1a8,float_bits<4>(score));fill_pool(*s,draw);compare(*s,2);}
    for(auto control:{0u,1u,2u,9u,10u,128u,205u,206u,215u,216u,255u})for(auto draw:{0u,1u,5u,99u,65535u}){auto s=seed(3);s->wr<4>(16,control);fill_pool(*s,draw);compare(*s,3);}
    // Exercise all five probability policies through their complete entry.
    for(unsigned kind=2;kind<=5;++kind)for(auto numerator:{0.0,1.0,65535.0,65536.0,-65535.0,-65536.0,1e15})for(auto draw:{0u,1u,65535u}){
        auto input=seed(2);input->wr<4>(16,kind);input->memory.write<8>(doubles+0x3b0+(kind-2)*8,float_bits<8>(numerator));fill_pool(*input,draw);Options o;o.draws={0,9,65535};compare(*input,2,o);
    }
    for(unsigned policy=0;policy<4;++policy)for(auto draw:{0u,1u,4u,9u,10u,99u,65535u})for(auto level:{0u,10u,255u})for(auto cap:{-100.0,100.0}){
        auto input=seed(3);fill_pool(*input,draw);input->wr<4>(16,10);input->memory.write<8>(doubles+0x3c0,float_bits<8>(policy==0?1:100));input->memory.write<8>(doubles+0x3b8,float_bits<8>(policy==1?1:100));input->memory.write<8>(doubles+0x3b0,float_bits<8>(policy==2?1:100));input->memory.write<8>(doubles+0x3a8,float_bits<8>(cap));
        for(unsigned slot=0;slot<40;++slot)input->memory.write<1>(profile+slot*0x100+0x69,level);compare(*input,3);
    }
    for(unsigned index=0;index<4;++index){
        for(auto cut:{0u,1u,2u,3u,7u,13u,20u,34u,43u,64u,99u,199u}){auto input=seed(index);input->budget=cut;compare(*input,index);}
        for(auto address:{toc-0x51e8,toc-0x51e0,toc-0x7b84,toc-0x7b88,first+0x6c,first+0x1a4}){auto input=seed(index);input->memory.write<4>(address,0);compare(*input,index);}
    }
    for(unsigned kind=1;kind<=5;++kind)for(auto matching:{0u,1u,2u}){auto input=seed(2);input->wr<4>(16,kind);input->wr<4>(20,first);input->memory.write<4>(first+0x1ac,matching==1?first:matching==2?first+4:0);input->memory.write<4>(cursor,pool+0x100);compare(*input,2);}
    for(unsigned policy=0;policy<3;++policy)for(auto draw:{0u,1u,4u,9u,10u,99u})for(auto rollover:{0u,1u}){
        auto input=seed(3);fill_pool(*input,draw);input->wr<4>(16,3);input->memory.write<1>(match+0x1bc,3);input->memory.write<4>(match+0xb4,first);input->memory.write<4>(match+0xc0,first+0x2350);input->memory.write<2>(first+0x19c,0x5f);
        input->memory.write<8>(doubles+0x3c0,float_bits<8>(policy==0?1:100));input->memory.write<8>(doubles+0x3b8,float_bits<8>(policy==1?1:100));input->memory.write<8>(doubles+0x3b0,float_bits<8>(policy==2?1:100));
        if(rollover)input->memory.write<4>(pool_end,pool);compare(*input,3);
    }
    auto signed_salt=seed(3);signed_salt->memory.write<4>(salt,0xffffffff);fill_pool(*signed_salt,0);signed_salt->wr<4>(16,10);signed_salt->memory.write<8>(doubles+0x3c0,float_bits<8>(100));signed_salt->memory.write<8>(doubles+0x3b8,float_bits<8>(1));compare(*signed_salt,3);
    auto negative_remainder=seed(3);negative_remainder->wr<4>(16,10);negative_remainder->memory.write<4>(salt,0xffffffff);fill_pool(*negative_remainder,300);negative_remainder->memory.write<2>(pool+8,0);negative_remainder->memory.write<8>(doubles+0x3c0,float_bits<8>(100));negative_remainder->memory.write<8>(doubles+0x3b8,float_bits<8>(1));compare(*negative_remainder,3);
    for(unsigned index=0;index<4;++index){auto input=seed(index);input->memory.map(fixture+0x110000,0x9000);input->memory.copy_in(fixture+0x110000,input->memory.copy_out(toc-0x8000,0x9000));input->wr<4>(8,fixture+0x118000);compare(*input,index);}
    auto alias=seed(1);alias->memory.write<4>(first+0x6c,first+0x13f);compare(*alias,1);
    // The compiler-expanded generic RNG arms below are unreachable from their
    // fixed ordinary bounds (110-signed_byte and 300/1000). Force only the
    // named branch condition on both sides; report this evidence separately.
    for(auto draw:{0u,65535u}){auto input=seed(2);fill_pool(*input,draw);Options o;o.forced={{0x1028676c,4}};o.draws={65535};compare(*input,2,o);}
    for(auto branch:{0x102878d8u,0x1028791cu})for(auto draw:{0u,65535u}){
        auto input=seed(3);input->wr<4>(16,10);fill_pool(*input,draw);input->memory.write<8>(doubles+0x3c0,float_bits<8>(100));input->memory.write<8>(doubles+0x3b8,float_bits<8>(1));Options o;o.forced={{branch,std::uint8_t(branch==0x102878d8?2:4)}};o.draws={65535};compare(*input,3,o);
    }
    std::ofstream coverage(argv[1]);unsigned total=0;constexpr std::array<unsigned,4> expected{35,44,1000,730};
    for(unsigned index=0;index<4;++index){for(auto pc:complete_sites[index])coverage<<index<<" "<<hex_address(pc)<<'\n';require(root_returns[index]>0&&complete_sites[index].size()==expected[index],"Complete encounter source sites root="+hex_address(selection::entries[index])+" sites="+std::to_string(complete_sites[index].size()));total+=complete_sites[index].size();std::cout<<"Root "<<hex_address(selection::entries[index])<<" complete_sites="<<complete_sites[index].size()<<"/"<<expected[index]<<" ordinary_sites="<<ordinary_sites[index].size()<<" checkpoint_sites="<<checkpoint_sites[index].size()<<" returns="<<root_returns[index]<<'\n';}
    std::cout<<"Actor encounter comparisons="<<cases<<" returns="<<returns<<" interruptions="<<interruptions<<" negatives="<<negatives<<" checkpoint_returns="<<checkpoint_returns<<" completed_sites="<<total<<"/1809\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
