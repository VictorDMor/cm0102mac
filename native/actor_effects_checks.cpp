#include "actor_effects.hpp"
#include "semantic_io_checks.hpp"
#include <fstream>
#include <set>
#include <map>
namespace cm::aot {bool actor_effects_reference(State&);}
namespace {
using namespace cm::semantic_io_checks;
namespace effects=cm::aot::actor_effects;
constexpr auto actor=fixture+0x1000,match=fixture+0x10000,doubles=fixture+0x40000,singles=fixture+0x41000,pool=fixture+0x42000;
constexpr auto salt=fixture+0x40,cursor=fixture+0x44,pool_end=fixture+0x48;
constexpr std::array<std::uint32_t,3> ends{0x102393bc,0x10285ee0,0x10285ee8};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{effects::update_balance,effects::add_counter,effects::assign_counter};
std::vector<std::uint8_t> original_doubles,original_singles;
struct Mutation {std::uint32_t pc;unsigned reg;std::uint32_t value;bool operator==(const Mutation&)const=default;};
struct Trace:SemanticGuestObserver {
 std::vector<std::uint32_t> order;std::vector<Mutation> mutations,applied;
 void before_step(State& s,std::uint32_t pc)override{if(s.executed<s.budget){for(const auto& m:mutations)if(pc==m.pc){applied.push_back(m);if(m.reg==32)s.wr<1>(2304,(s.r<1>(2304)&0xf1)|m.value);else s.wr<4>(m.reg*4,m.value);}order.push_back(pc);}}
};
thread_local Trace* reference_trace=nullptr;
struct Event {std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Options {std::uint32_t role=0,draw=0;bool negative=false,child_negative=false,mutate_balance=false;unsigned interrupt=0;std::vector<Mutation> mutations;};
struct Fixture {std::unique_ptr<State> input;Options options;};
std::map<std::uint32_t,Fixture> captures;
constexpr std::array capture_points{0x1023847cu};
struct Services:SemanticDependencyDispatch {
 bool original;Options options;std::vector<Event> calls;unsigned child_mutations=0;
 Services(bool old,const Options& o):original(old),options(o){}
 void call(State& s,std::uint32_t entry,std::uint32_t continuation)override{
  const auto index=calls.size();calls.push_back({entry,continuation,s.registers,{}});auto value=options.draw;
  if(entry==0x10289560)value=options.role;
  else if(entry==0x102865a0){const auto address=std::uint32_t(s.r<4>(12))+0x1a8;s.memory.write<4>(address,0x3f800000);if(options.child_negative&&!original){s.memory.write<4>(address,0x3f800001);++child_mutations;}}
  else if(entry!=0x1077b770&&entry!=0x10226520)throw std::runtime_error("Unexpected actor-effects service "+hex_address(entry));
  // Models supply boundary behavior only; actual native services are checked
  // separately before installation. Poison every volatile integer/FP register.
  for(unsigned reg=0;reg<=12;++reg)if(reg!=1&&reg!=2)s.wr<4>(reg*4,0xac000000+reg*71);
  for(unsigned reg=0;reg<=13;++reg)s.wr<8>(0x4000+reg*16,float_bits<8>(-17.0-reg));
  s.wr<1>(2304,5);s.wr<4>(12,value);s.pc=continuation;calls[index].after=s.registers;
  if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Fixture actor-effects service interrupted");
 }
};
std::unique_ptr<State> seed(unsigned index){
 auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned n=1280;n<1312;++n)s->wr<1>(n,0);
 s->memory.map(fixture,0x50000);s->memory.map(stack-0x4000,0x5000);s->memory.map(toc-0x8000,0x9000);
 s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(12,actor);s->wr<4>(16,1);s->wr<4>(4128,return_sentinel);s->pc=effects::entries[index];s->budget=100000;
 s->memory.copy_in(doubles,original_doubles);s->memory.copy_in(singles,original_singles);
 s->memory.write<4>(toc-0x51e8,doubles);s->memory.write<4>(toc-0x51e0,singles);
 s->memory.write<4>(toc-0x7b90,salt);s->memory.write<4>(toc-0x7b84,cursor);s->memory.write<4>(toc-0x7b88,pool_end);s->memory.write<4>(toc-0x7b8c,pool);
 s->memory.write<4>(cursor,pool);s->memory.write<4>(pool_end,pool+0x7fe);for(unsigned off=0;off<0x800;off+=2)s->memory.write<2>(pool+off,0);
 s->memory.write<4>(actor+0x1a4,match);s->memory.write<2>(actor+0x36,7000);s->memory.write<1>(actor+0x27,0);s->memory.write<1>(actor+0x19,0);
 return s;
}
unsigned cases=0,returns=0,interruptions=0,negatives=0,checkpoint_returns=0;
std::array<std::set<std::uint32_t>,3> complete_sites,ordinary_sites;std::array<unsigned,3> root_returns{};
struct Result {std::unique_ptr<State> state;unsigned call_count;};
Result compare(const State& input,unsigned index,const Options& options={}){
 auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();Trace before,after;before.mutations=after.mutations=options.mutations;Services old(true,options),modern(false,options);
 const auto mutation=[&](State& s,Effects& e,std::uint32_t address,unsigned){if(options.mutate_balance&&!e.mutated&&address==actor+0x36){e.mutated=true;s.memory.write<2>(actor+0x36,65535);}};x.on_read=y.on_read=mutation;
 a->memory.date_observer=&x;b->memory.date_observer=&y;
 auto run=[&](State& s,Services& services,Trace& trace){try{reference_trace=&trace;if(services.original){while(s.pc!=return_sentinel)if(!actor_effects_reference(s))services.call(s,s.pc,s.r<4>(4128));}else methods[index](s,&services,&trace);reference_trace=nullptr;return std::string{};}catch(const std::runtime_error& e){reference_trace=nullptr;return std::string(e.what());}};
 const auto expected=run(*a,old,before),actual=run(*b,modern,after);a->memory.date_observer=b->memory.date_observer=nullptr;auto label="Actor effects root="+hex_address(input.pc)+" case="+std::to_string(cases);
 require(expected==actual,label+" original="+expected+" candidate="+actual);require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Fixture actor-effects service interrupted"||expected=="Signed division by zero in translated code"||expected.starts_with("Unmapped data access at "),label+" unexpected outcome="+expected);
 if(options.negative)b->wr<4>(12,b->r<4>(12)^1);if(options.child_negative)require(modern.child_mutations>0,label+" child negative not exercised");
 bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.negative&&!options.child_negative)throw;rejected=true;}
 require(rejected==(options.negative||options.child_negative),label+" discriminating control");
 if(!rejected){require(x.bytes==y.bytes,label+" ordered memory effects");require(old.calls==modern.calls,label+" service register ABI");require(a->executed==b->executed,label+" instruction accounting");
  if(before.order!=after.order){const auto mismatch=std::mismatch(before.order.begin(),before.order.end(),after.order.begin(),after.order.end());throw std::runtime_error(label+" source order offset="+std::to_string(mismatch.first-before.order.begin())+" old="+(mismatch.first==before.order.end()?"end":hex_address(*mismatch.first))+" new="+(mismatch.second==after.order.end()?"end":hex_address(*mismatch.second)));}}
 require(before.applied==after.applied,label+" applied source checkpoints");
 if(expected.empty()&&!rejected&&before.applied.empty())for(auto point:capture_points)if(!captures.contains(point)&&std::find(before.order.begin(),before.order.end(),point)!=before.order.end())captures.emplace(point,Fixture{clone(input),options});
 ++cases;interruptions+=!expected.empty();negatives+=rejected;
 if(expected.empty()&&!rejected){++returns;++root_returns[index];checkpoint_returns+=!before.applied.empty();for(auto pc:before.order)if(effects::entries[index]<=pc&&pc<ends[index]){complete_sites[index].insert(pc);if(before.applied.empty())ordinary_sites[index].insert(pc);}}
 return{std::move(a),unsigned(old.calls.size())};
}
}
namespace cm::aot {void actor_effects_step_seen(State& s,std::uint32_t pc){if(reference_trace)reference_trace->before_step(s,pc);}}
int main(int argc,char**argv){try{
 require(argc==3,"Expected private original program and coverage output");State program;load_program(program,argv[1]);original_doubles=program.memory.copy_out(program.memory.read<4>(toc-0x51e8),0x400);original_singles=program.memory.copy_out(program.memory.read<4>(toc-0x51e0),0x200);
 for(unsigned index=0;index<3;++index){compare(*seed(index),index);Options o;o.negative=true;compare(*seed(index),index,o);auto bad=seed(index);bad->wr<4>(12,0);compare(*bad,index);}
 for(unsigned value=0;value<256;++value)for(auto increment:{0u,1u,127u,128u,255u,256u,0x80000000u,0xffffffffu}){
  auto input=seed(1);input->memory.write<1>(actor+0x2c,value);input->wr<4>(16,increment);auto result=compare(*input,1);require(result.state->memory.read<1>(actor+0x2c)==((value+increment)&255),"Independent byte-add oracle");
 }
 for(auto old:{0u,127u,128u,255u})for(unsigned value=0;value<256;++value){auto input=seed(2);input->memory.write<1>(actor+0x2c,old);input->wr<4>(16,0x12340000+value);auto result=compare(*input,2);require(result.state->memory.read<1>(actor+0x2c)==value,"Independent byte-assignment oracle");}
 constexpr unsigned roles[]{0,1,2,4,8,16,32,64,128,0xffff};
 constexpr unsigned balances[]{0,1,6000,6700,7250,7750,8000,8250,8500,9999,32767,32768,65535};
 for(auto role:roles)for(auto balance:balances)for(auto draw:{0u,1u,5u,32767u,65535u,0xffffffffu}){auto input=seed(0);input->memory.write<2>(actor+0x36,balance);Options o;o.role=role;o.draw=draw;compare(*input,0,o);}
 for(auto role:roles)for(auto value:{0u,1u,2u,3u,4u,5u,6u,7u,10u,20u,127u,128u,255u})for(auto balance:{0u,7000u,8250u,9999u}){
  auto input=seed(0);for(unsigned off=1;off<0x20;++off)input->memory.write<1>(actor+off,value);input->memory.write<2>(actor+0x36,balance);Options o;o.role=role;compare(*input,0,o);
 }
 for(auto role:roles)for(auto field:{1u,2u,3u,4u,5u,6u,7u,8u,9u,11u,12u,15u,16u,17u,31u})for(auto value:{0u,1u,2u,5u,10u,20u,127u,128u,255u})for(auto balance:{0u,7250u,8500u,9999u}){
  auto input=seed(0);input->memory.write<1>(actor+field,value);input->memory.write<2>(actor+0x36,balance);Options o;o.role=role;compare(*input,0,o);
 }
 for(unsigned value=0;value<256;++value){auto input=seed(0);input->memory.write<1>(actor+0x2c,value);auto result=compare(*input,0);const auto expected=value<128?(value?value-1:0):0;require(result.state->memory.read<1>(actor+0x2c)==expected,"Independent signed counter normalization oracle");}
 for(auto role:roles){Options o;o.role=role;auto input=seed(0);auto result=compare(*input,0,o);for(unsigned stop=0;stop<=result.state->executed+1;++stop){auto limited=clone(*input);limited->budget=stop;compare(*limited,0,o);}for(unsigned stop=1;stop<=result.call_count;++stop){o.interrupt=stop;compare(*input,0,o);}}
 for(unsigned index=1;index<3;++index)for(unsigned budget=0;budget<=5;++budget){auto input=seed(index);input->budget=budget;compare(*input,index);}
 Options child;child.child_negative=true;compare(*seed(0),0,child);Options mutation;mutation.mutate_balance=true;compare(*seed(0),0,mutation);
 // Ordered pool samples, independent team counters and paired statistic ratios
 // supply ordinary prepared inputs; they do not change source checkpoints.
 for(auto role:{0u,1u,2u,4u,8u,16u,32u,64u,0x84u,0x804u,0x884u})for(auto value:{0u,1u,3u,5u,20u,255u})for(auto balance:{0u,6000u,7250u,7750u,9000u})for(auto wrap:{false,true}){
  auto input=seed(0);input->memory.write<1>(actor+15,value);input->memory.write<2>(actor+0x36,balance);input->memory.write<4>(pool_end,wrap?pool:pool+0x7fe);
  for(unsigned off=0;off<0x800;off+=2)input->memory.write<2>(pool+off,off/2);
  Options o;o.role=role;o.draw=1;compare(*input,0,o);
 }
 for(auto role:{1u,2u,4u,8u,16u,32u,64u,0x84u,0x804u,0x884u})for(auto side:{0u,1u,255u})for(auto team:{0u,1u,2u,3u,5u,10u,127u,128u,255u})for(auto balance:{0u,6000u,7250u,7750u,9000u}){
  auto input=seed(0);input->memory.write<1>(actor+0x27,side);input->memory.write<1>(match+0xe03d,team);input->memory.write<1>(match+0xe073,team);input->memory.write<2>(actor+0x36,balance);Options o;o.role=role;compare(*input,0,o);
 }
 for(auto role:{1u,4u,0x84u,0x804u,0x884u})for(auto pair:std::array<std::pair<unsigned,unsigned>,3>{{{1,2},{5,6},{8,7}}})for(auto numerator:{0u,1u,3u,20u,50u,100u,255u})for(auto denominator:{5u,21u,50u})for(auto balance:{0u,5700u,6500u,7750u,9000u}){
  auto input=seed(0);input->memory.write<1>(actor+pair.first,numerator);input->memory.write<1>(actor+pair.second,denominator);input->memory.write<2>(actor+0x36,balance);Options o;o.role=role;compare(*input,0,o);
 }
 for(auto time:{0u,989u,990u,1001u,32767u,32768u,65535u})for(auto field:{0u,2u,3u})for(auto leading:{0u,1u,2u,3u,4u,5u})for(auto draw:{1u,2u,32767u,65535u})for(auto balance:{4251u,7250u,7750u,9000u}){
  auto input=seed(0);input->memory.write<2>(match+0x7948,time);input->memory.write<1>(actor+15,field);input->memory.write<2>(actor+0x36,balance);
  for(unsigned off=0;off<0x800;off+=2)input->memory.write<2>(pool+off,off/2<=leading?0:draw);Options o;o.role=1;compare(*input,0,o);
 }
 // Explicit source-checkpoint fixtures cover expanded generic RNG arms whose
 // byte-derived bounds cannot naturally exceed the original generic threshold.
 for(auto value:{65536u,0xffff0000u})for(auto draw:{0u,1000000u})for(auto sample:{0u,65535u}){
  auto input=seed(0);input->memory.write<1>(actor+15,3);input->memory.write<2>(actor+0x36,7750);
  for(unsigned off=0;off<0x800;off+=2)input->memory.write<2>(pool+off,sample);
  Options o;o.role=1;o.draw=draw;o.mutations={{0x10237e84,24,value}};compare(*input,0,o);
  o.mutations={{0x1023820c,25,value}};compare(*input,0,o);
 }
 for(auto draw:{0u,1000000u})for(auto time:{0u,990u,65535u}){
  auto input=seed(0);input->memory.write<2>(actor+0x36,9000);input->memory.write<2>(match+0x7948,time);Options o;o.role=1;o.draw=draw;o.mutations={{0x1023847c,32,4}};compare(*input,0,o);
 }
 {auto input=seed(0);input->memory.write<1>(actor+15,3);input->memory.write<2>(actor+0x36,7750);Options o;o.role=1;o.mutations={{0x102381cc,25,0}};compare(*input,0,o);}
 // Combine statistic gates with nonzero ordered samples, both team sides,
 // capped opponent counts and late-role modifiers using ordinary input data.
 for(auto field:{0u,2u,3u})for(auto gate:{0u,5u})for(auto goals:{2u,255u})for(auto sample:{0u,1u})for(auto balance:{0u,7000u,7751u,8751u,9000u}){
  auto input=seed(0);input->memory.write<1>(actor+15,field);input->memory.write<1>(actor+3,gate);input->memory.write<2>(actor+0x36,balance);input->memory.write<1>(match+0xe03f,goals);input->memory.write<1>(match+0xe075,goals);
  for(unsigned off=0;off<0x800;off+=2)input->memory.write<2>(pool+off,sample);Options o;o.role=1;compare(*input,0,o);
  input->memory.write<2>(pool+2,0);input->memory.write<2>(pool+4,1);compare(*input,0,o);
 }
 for(auto role:{4u,0x84u,0x804u,0x884u})for(auto side:{0u,1u})for(auto team:{0u,1u,3u,10u})for(auto active:{0u,1u,5u})for(auto balance:{5500u,6000u,7000u,7750u,9000u}){
  auto input=seed(0);input->memory.write<1>(actor+0x27,side);input->memory.write<1>(actor+3,active);input->memory.write<1>(match+0xe03d,team);input->memory.write<1>(match+0xe073,team);input->memory.write<2>(actor+0x36,balance);Options o;o.role=role;compare(*input,0,o);
  input->memory.write<1>(actor+4,20);input->memory.write<1>(actor+5,0);compare(*input,0,o);
 }
 // Replay a complete ordinary fixture known to reach the time-bound guard;
 // changing its branch condition is explicitly structural checkpoint evidence.
 require(captures.contains(0x1023847c),"Ordinary time-bound guard fixture available");
 for(auto draw:{0u,1000000u}){const auto& f=captures.at(0x1023847c);auto o=f.options;o.draw=draw;o.mutations={{0x1023847c,32,4}};compare(*f.input,0,o);}
 std::ofstream out(argv[2]);unsigned covered=0,ordinary=0;
 for(unsigned i=0;i<3;++i){covered+=complete_sites[i].size();ordinary+=ordinary_sites[i].size();for(auto pc:complete_sites[i])out<<i<<" "<<std::hex<<pc<<std::dec<<" "<<ordinary_sites[i].contains(pc)<<"\n";std::cout<<"Root "<<hex_address(effects::entries[i])<<" complete_sites="<<complete_sites[i].size()<<" ordinary_sites="<<ordinary_sites[i].size()<<"\n";}
 std::cout<<"Actor effects comparisons="<<cases<<" returns="<<returns<<" interruptions="<<interruptions<<" negatives="<<negatives<<" checkpoint_returns="<<checkpoint_returns<<" completed_sites="<<covered<<"/1569 ordinary_sites="<<ordinary<<"\n";
 require(covered==1569,"Complete own source coverage required before acceptance");
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
