#include "actor_policy.hpp"
#include "semantic_io_checks.hpp"
#include "semantic_dependency_dispatch.hpp"
#include <fstream>
#include <map>
#include <set>
namespace cm::aot { bool actor_policy_reference(State&); }
namespace policy_fixture {
using namespace cm::semantic_io_checks;
inline constexpr std::array<std::uint32_t,5> entries{0x101f5280,0x10285750,0x10285790,0x102857c0,0x10285ef0};
inline constexpr std::array<std::uint32_t,5> ends{0x101f5348,0x10285790,0x102857b4,0x10285ec4,0x102860c0};
inline constexpr auto actor=fixture+0x1000,stats=fixture+0x5000,traits=fixture+0x6000,match=fixture+0x10000;
inline constexpr auto doubles=fixture+0x40000,pool=fixture+0x42000,salt=fixture+0x40,cursor=fixture+0x44,pool_end=fixture+0x48;
inline std::vector<std::uint8_t> original_doubles;
struct Mutation {std::uint32_t pc;unsigned reg;std::uint32_t value;bool operator==(const Mutation&)const=default;};
struct Trace:SemanticGuestObserver {
 std::vector<std::uint32_t> order;std::vector<Mutation> mutations,applied;std::set<std::pair<std::uint32_t,unsigned>> changed;
 void before_step(State& s,std::uint32_t pc)override {
  if(s.executed>=s.budget)return;
  for(const auto& m:mutations)if(m.pc==pc&&changed.insert({pc,m.reg}).second){applied.push_back(m);if(m.reg==32)s.wr<1>(2304,(s.r<1>(2304)&0xf0)|m.value);else s.wr<4>(m.reg*4,m.value);}
  order.push_back(pc);
 }
};
inline thread_local Trace* reference_trace=nullptr;
struct Event {std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Options {
 std::uint32_t role=0,other_role=0,side=0,excess=0,strategy_first=4,strategy_second=0,link=traits,draw=7;
 unsigned interrupt=0;bool negative=false;std::vector<Mutation> mutations;
};
struct Services:SemanticDependencyDispatch {
 Options options;std::vector<Event> calls;unsigned strategy_calls=0;
 explicit Services(const Options& o):options(o){}
 void call(State& s,std::uint32_t entry,std::uint32_t continuation)override {
  const auto index=calls.size();calls.push_back({entry,continuation,s.registers,{}});std::uint32_t result=0;
  if(entry==0x10289560)result=options.role;
  else if(entry==0x102895e0)result=options.other_role;
  else if(entry==0x10283690)result=options.side;
  else if(entry==0x101e4e10)result=options.excess;
  else if(entry==0x101149f0)result=strategy_calls++?options.strategy_second:options.strategy_first;
  else if(entry==0x10114b10)result=options.link;
  else if(entry==0x1077b770||entry==0x10226520)result=options.draw;
  else throw std::runtime_error("Unexpected actor-policy dependency "+hex_address(entry));
  for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xa6000000+n*73);
  for(unsigned n=0;n<=13;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-19.5-n));
  s.wr<1>(2304,5);s.wr<4>(12,result);s.pc=continuation;calls[index].after=s.registers;
  if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Fixture actor-policy service interrupted");
 }
 void original(State& s) {
  while(s.pc!=return_sentinel)if(!actor_policy_reference(s))call(s,s.pc,s.r<4>(4128));
 }
};
inline std::uint32_t opposite(unsigned side,unsigned index){return match+0x311c+(side==0?1:0)*0x2350+index*0x1c4;}
inline std::unique_ptr<State> seed(unsigned index) {
 auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);
 for(unsigned n=1280;n<1312;++n)s->wr<1>(n,0);
 s->memory.map(fixture,0x60000);s->memory.map(stack-0x8000,0x9000);s->memory.map(toc-0x8000,0x9000);
 s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(12,index==0?match:actor);s->wr<4>(16,index==0?actor:3);s->wr<4>(4128,return_sentinel);s->pc=entries[index];s->budget=20000;
 s->memory.copy_in(doubles,original_doubles);
 for(auto [offset,address]:std::array<std::pair<int,std::uint32_t>,5>{{{-0x51e8,doubles},{-0x7b90,salt},{-0x7b84,cursor},{-0x7b88,pool_end},{-0x7b8c,pool}}})s->memory.write<4>(toc+std::uint32_t(offset),address);
 s->memory.write<4>(cursor,pool);s->memory.write<4>(pool_end,pool+0x3fe);
 for(unsigned n=0;n<0x400;n+=2)s->memory.write<2>(pool+n,7);
 s->memory.write<4>(actor+0x1a4,match);s->memory.write<4>(actor+0x70,stats);s->memory.write<1>(actor+0x27,0);s->memory.write<1>(actor+0x19,2);
 s->memory.write<1>(stats+0x17,5);s->memory.write<1>(stats+0x18,5);s->memory.write<1>(stats+0x19,5);
 s->memory.write<1>(match+0x791f,7);s->memory.write<1>(match+0x7922,128);s->memory.write<1>(match+0x7923,255);s->memory.write<4>(match+0xe004,actor);
 for(unsigned side=0;side<2;++side)for(unsigned n=0;n<21;++n){s->memory.write<1>(opposite(side,n)+0x19,255);s->memory.write<4>(opposite(side,n)+0x6c,0);}
 s->memory.write<1>(opposite(0,0)+0x19,0);s->memory.write<4>(opposite(0,0)+0x6c,traits);
 return s;
}
inline void load_constants(const char* path) {
 State original;load_program(original,path);original_doubles=original.memory.copy_out(original.memory.read<4>(toc-0x51e8),0x1000);
}
template<class Compare> void ordinary_campaign(Compare compare) {
 for(unsigned index=0;index<5;++index)compare(*seed(index),index,Options{});
 for(auto role:{0u,1u,2u,127u,128u,0x80u,0x100u,0x200u,0x400u,0x800u,0xffffu})for(auto side:{0u,1u,2u,127u,128u,255u}){
  auto s=seed(0);Options o;o.role=role;o.side=side;compare(*s,0,o);
 }
 for(auto index:{1u,2u})for(auto present:{false,true})for(auto value:{0u,1u,127u,128u,255u,256u,0xffffffffu}){
  auto s=seed(index);s->memory.write<4>(actor+0x1a4,present?match:0);s->memory.write<1>(match+0x791f,value);Options o;o.excess=value;compare(*s,index,o);
 }
 for(auto role:{0u,1u,0x80u,0x100u,0x200u,0x400u,0x800u,0x780u})for(auto side:{0u,1u})for(auto trait:{0u,1u,5u,20u,127u,128u,255u})for(auto sample:{0u,1u,7u,40u,65535u}){
  auto s=seed(3);s->memory.write<1>(actor+0x27,side);for(unsigned off=0x17;off<=0x19;++off)s->memory.write<1>(stats+off,trait);for(unsigned n=0;n<0x400;n+=2)s->memory.write<2>(pool+n,sample);Options o;o.role=role;compare(*s,3,o);
 }
 for(auto side:{0u,1u})for(auto first:{0u,4u,20u})for(auto second:{0u,16u})for(auto match_index:{0u,1u,10u,20u,21u})for(auto inhibited:{0u,1u})for(auto role:{0u,1u,64u}){
  auto s=seed(4);s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x109,inhibited);for(unsigned n=0;n<21;++n)s->memory.write<1>(opposite(side,n)+0x19,255);
  if(match_index<21){s->memory.write<1>(opposite(side,match_index)+0x19,0);s->memory.write<4>(opposite(side,match_index)+0x6c,traits);}Options o;o.strategy_first=first;o.strategy_second=second;o.role=role;o.other_role=64;compare(*s,4,o);
 }
}
}

namespace {
using namespace policy_fixture;
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{cm::aot::actor_policy::context_control,cm::aot::actor_policy::column_excess,cm::aot::actor_policy::phase_marker,cm::aot::actor_policy::adjust_action_bias,cm::aot::actor_policy::find_linked_opponent};
unsigned comparisons=0,returns=0,interruptions=0,negatives=0,checkpoint_returns=0;
std::array<std::set<std::uint32_t>,5> complete_sites,ordinary_sites;
struct Result {std::unique_ptr<State> state;unsigned calls;std::string outcome;};
Result compare(const State& input,unsigned index,const Options& options={}) {
 auto a=clone(input),b=clone(input);Trace before,after;before.mutations=after.mutations=options.mutations;Services old(options),modern(options);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
 auto run=[&](State& s,Services& services,Trace& trace,bool original){try {if(original){reference_trace=&trace;services.original(s);reference_trace=nullptr;}else methods[index](s,&services,&trace);return std::string{};}catch(const std::runtime_error& e){reference_trace=nullptr;return std::string(e.what());}};
 const auto expected=run(*a,old,before,true),actual=run(*b,modern,after,false);a->memory.date_observer=b->memory.date_observer=nullptr;
 const auto label="Component actor-policy root="+hex_address(entries[index])+" case="+std::to_string(comparisons);
 require(expected==actual,label+" original="+expected+" candidate="+actual);
 require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Fixture actor-policy service interrupted"||expected=="Signed division by zero in translated code"||expected.starts_with("Original enabled floating ")||expected.starts_with("Unmapped data access at "),label+" unexpected outcome "+expected);
 if(options.negative){require(expected.empty(),label+" negative needs a complete return");b->wr<4>(12,b->r<4>(12)^1);}
 bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.negative)throw;rejected=true;}
 require(rejected==options.negative,label+" discriminating negative");
 if(!rejected){require(x.bytes==y.bytes,label+" memory order");require(old.calls==modern.calls,label+" dependency ABI");require(a->executed==b->executed,label+" instruction accounting");require(before.order==after.order,label+" source order");}
 require(before.applied==after.applied,label+" diagnostic checkpoints");++comparisons;interruptions+=!expected.empty();negatives+=rejected;
 if(expected.empty()&&!rejected){++returns;checkpoint_returns+=!before.applied.empty();for(auto pc:before.order)if(entries[index]<=pc&&pc<ends[index]){complete_sites[index].insert(pc);if(before.applied.empty())ordinary_sites[index].insert(pc);}}
 return{std::move(a),unsigned(old.calls.size()),expected};
}
}
namespace cm::aot {void actor_policy_step_seen(State& s,std::uint32_t pc){if(policy_fixture::reference_trace)policy_fixture::reference_trace->before_step(s,pc);}}
int main(int argc,char**argv){try {
 using namespace policy_fixture;require(argc==3,"Expected original source and coverage output");load_constants(argv[1]);
 ordinary_campaign([](const State& s,unsigned index,const Options& o){compare(s,index,o);});
 for(unsigned index=0;index<5;++index){
  auto s=seed(index);auto normal=compare(*s,index);Options bad;bad.negative=true;compare(*s,index,bad);
  for(unsigned budget=0;budget<=normal.state->executed+1;++budget){auto limited=clone(*s);limited->budget=budget;compare(*limited,index);}
  for(unsigned stop=1;stop<=normal.calls;++stop){Options o;o.interrupt=stop;compare(*s,index,o);}
  auto missing=seed(index);missing->wr<4>(12,0);compare(*missing,index);
 }
 for(auto index:{1u,2u}){auto s=seed(index);s->memory.write<4>(actor+0x1a4,0);auto result=compare(*s,index);require(result.state->r<4>(12)==(index==1?UINT32_MAX:0),"Absent-match query oracle");}
 {auto s=seed(0);Options o;o.role=1;o.side=1;require(compare(*s,0,o).state->r<4>(12)==255,"Side1 unsigned control byte");o.side=0;require(compare(*s,0,o).state->r<4>(12)==128,"Side0 unsigned control byte");}
 {auto s=seed(3);s->wr<4>(16,0x89abcdef);Options o;o.role=1;require(compare(*s,3,o).state->r<4>(12)==0x89abcdef,"Exempt-role full-width bypass");}
 // Independent trait channels, signed input bytes and each stream-replenishment position.
 for(auto channel:{0x17u,0x18u,0x19u})for(auto trait:{0u,1u,2u,20u,127u,128u,255u})for(auto threshold:{0u,1u,10u,127u,128u,255u})for(auto sample:{0u,1u,7u,40u,65535u}){
  auto s=seed(3);s->memory.write<1>(stats+channel,trait);s->memory.write<1>(actor+0x10d,threshold);for(unsigned n=0;n<0x400;n+=2)s->memory.write<2>(pool+n,sample);compare(*s,3);
 }
 for(unsigned stop=0;stop<16;stop+=2)for(auto sample:{0u,1u,65535u})for(auto value:{0u,1u,127u,128u,255u,256u,0xffffffffu}){
  auto s=seed(3);s->memory.write<4>(pool_end,pool+stop);s->wr<4>(16,value);Options o;o.draw=sample;compare(*s,3,o);
 }
 // Distinct consecutive stream values separate trait magnitude draws from
 // their following comparison draws, including each side-specific result.
 for(auto side:{0u,1u})for(auto trait:{1u,5u,20u,127u,128u,255u})for(auto magnitude:{0u,1u,19u,126u,65535u}){
  auto s=seed(3);s->memory.write<1>(actor+0x27,side);for(unsigned off=0x17;off<=0x19;++off)s->memory.write<1>(stats+off,trait);
  for(unsigned n=0;n<0x400;n+=2)s->memory.write<2>(pool+n,n%4==0?magnitude:0);s->memory.write<2>(pool+2,7);compare(*s,3);
 }
 // A signed-byte trait cannot naturally exceed the signed16 sampling premise.
 // These explicit first-visit scalar checkpoints characterize its retained
 // wide-number branches separately from ordinary prepared-input coverage.
 for(auto [pc,reg]:std::array<std::pair<std::uint32_t,unsigned>,3>{{{0x102858bc,23},{0x10285ae0,22},{0x10285cd4,23}}})for(auto wide:{65536u,0x7fffffffu,0x80000000u,0xffff0000u})for(auto sample:{0u,1u,7u,65535u}){
  auto s=seed(3);for(unsigned n=0;n<0x400;n+=2)s->memory.write<2>(pool+n,sample);Options o;o.mutations={{pc,reg,wide}};compare(*s,3,o);
 }
 // Opponent inhibition examines both the opposite team strategy and live role.
 for(auto side:{0u,1u})for(auto team_flag:{0u,0x10000000u})for(auto status:{0u,1u,2u,127u,128u,255u})for(auto inhibited:{0u,1u})for(auto role:{0u,1u,64u})for(auto other:{0u,1u,64u}){
  auto s=seed(4);s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x109,inhibited);s->memory.write<1>(opposite(side,0)+0x19,0);s->memory.write<4>(opposite(side,0)+0x6c,traits);
  s->memory.write<4>(match+(side==0?1:0)*0x18e0+0x7c6c+0x1880,team_flag);s->memory.write<1>(match+0xaf+(side==0?1:0),status);Options o;o.strategy_second=16;o.role=role;o.other_role=other;compare(*s,4,o);
 }
 std::ofstream coverage(argv[2]);unsigned total=0,ordinary=0;
 for(unsigned i=0;i<5;++i){total+=complete_sites[i].size();ordinary+=ordinary_sites[i].size();std::cout<<"Root "<<hex_address(entries[i])<<" complete_sites="<<complete_sites[i].size()<<" ordinary_sites="<<ordinary_sites[i].size()<<std::endl;for(auto pc:complete_sites[i])coverage<<i<<" "<<hex_address(pc)<<" "<<ordinary_sites[i].contains(pc)<<'\n';}
 require(total==640,"Incomplete returning actor-policy source coverage");
 std::cout<<"PASS: actor-policy comparisons="<<comparisons<<" returns="<<returns<<" interruptions="<<interruptions<<" negatives="<<negatives<<" checkpoint_returns="<<checkpoint_returns<<" complete_sites="<<total<<"/640 ordinary_sites="<<ordinary<<"; explicit modeled services, actual composition checked separately\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
