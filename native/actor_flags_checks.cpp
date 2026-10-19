#include "actor_flags.hpp"
#include "match_target.hpp"
#include "semantic_io_checks.hpp"
#include <fstream>
#include <set>
namespace cm::aot {bool actor_flags_reference(State&);}
namespace {
using namespace cm::semantic_io_checks;
namespace flags=cm::aot::actor_flags;
constexpr auto actor=fixture+0x1000,match=fixture+0x1000000;
constexpr std::array<std::uint32_t,7> ends{0x10286320,0x10286390,0x10286400,0x10286470,0x102864ac,0x102864ec,0x1028659c};
constexpr std::array<unsigned,4> bits{21,25,29,27};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{flags::test_bit21,flags::test_bit25,flags::test_bit29,flags::test_bit27,flags::read_flags,flags::read_flags_alternate,flags::team_or_slot_status};
struct Trace:SemanticGuestObserver{std::vector<std::uint32_t> sites;void before_step(State& s,std::uint32_t pc)override{if(s.executed<s.budget)sites.push_back(pc);}};
thread_local Trace* reference_trace=nullptr;
struct Event {std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Calls:SemanticDependencyDispatch {
 bool original,negative;Trace& trace;std::vector<Event> events;unsigned mutations=0;
 Calls(bool old,bool bad,Trace& t):original(old),negative(bad),trace(t){}
 void run_reference(State& s,std::uint32_t until){bool first=true;while(s.pc!=until){
  if(!first&&s.pc==0x10113e10)call(s,s.pc,s.r<4>(4128));
  else require(actor_flags_reference(s),"Uncompiled independent flag reference "+hex_address(s.pc));first=false;
 }}
 void call(State& s,std::uint32_t target,std::uint32_t continuation)override{
  require(target==0x10113e10,"Unexpected flag-query dependency");auto n=events.size();events.push_back({target,continuation,s.registers,{}});
  if(original)run_reference(s,continuation);else match_target::strategy_flags(s,this,&trace);
  if(negative&&!original){s.wr<4>(12,s.r<4>(12)^1);++mutations;}events[n].after=s.registers;
 }
};
std::uint32_t team_address(unsigned side){return match+0x7c6c+std::uint32_t(std::int8_t(side))*0x18e0;}
std::uint32_t word_address(unsigned side,unsigned slot){return team_address(side)+0x1888+std::uint32_t(std::int8_t(slot))*8;}
std::unique_ptr<State> seed(unsigned index,unsigned side=0,unsigned slot=0,std::uint32_t low=0,std::uint32_t high=0xa55a1234){
 auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned n=1280;n<1312;++n)s->wr<1>(n,0);
 s->memory.map(actor,0x1000);s->memory.map(match-0x1000,0x3000);s->memory.map(team_address(side)-0x1000,0x6000);s->memory.map(stack-0x1000,0x2000);s->memory.map(toc-0x8000,0x9000);
 s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(12,actor);s->wr<4>(4128,return_sentinel);s->pc=flags::entries[index];s->budget=1000;
 s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x19,slot);s->memory.write<4>(actor+0x1a4,match);
 s->memory.write<4>(word_address(side,slot),high);s->memory.write<4>(word_address(side,slot)+4,low);
 return s;
}
struct Options {bool negative=false,child_negative=false,mutate_low=false,mutate_side=false;};
unsigned cases=0,returns=0,budgets=0,faults=0,negatives=0;std::array<std::set<std::uint32_t>,7> sites;std::array<unsigned,7> root_returns{};
std::unique_ptr<State> compare(const State& input,unsigned index,Options options={}){
 auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();Trace before,after;Calls old(true,false,before),modern(false,options.child_negative,after);
 const auto hook=[&](State& s,Effects& e,std::uint32_t address,unsigned){
  const auto side=input.memory.read<1>(actor+0x27),slot=input.memory.read<1>(actor+0x19);
  if(options.mutate_low&&!e.mutated&&address==word_address(side,slot)){e.mutated=true;s.memory.write<4>(address+4,0x2468ace0);}
  if(options.mutate_side&&!e.mutated&&address==actor+0x27){e.mutated=true;s.memory.write<1>(actor+0x27,side+1);}
 };
 x.on_read=y.on_read=hook;a->memory.date_observer=&x;b->memory.date_observer=&y;
 auto run=[&](State& s,Calls& calls){try{reference_trace=&calls.trace;if(calls.original)calls.run_reference(s,return_sentinel);else methods[index](s,&calls,&calls.trace);reference_trace=nullptr;return std::string{};}catch(const std::runtime_error& e){reference_trace=nullptr;return std::string(e.what());}};
 const auto expected=run(*a,old),actual=run(*b,modern);a->memory.date_observer=b->memory.date_observer=nullptr;
 auto label="Actor flags root="+hex_address(input.pc)+" case="+std::to_string(cases);
 require(expected==actual,label+" original="+expected+" candidate="+actual);require(expected.empty()||expected=="Native instruction budget exhausted"||expected.starts_with("Unmapped data access at "),label+" unexpected result");
 if(options.negative)b->wr<4>(12,b->r<4>(12)^1);if(options.child_negative)require(modern.mutations>0,label+" child negative not exercised");
 bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.negative&&!options.child_negative)throw;rejected=true;}
 require(rejected==(options.negative||options.child_negative),label+" discriminating control");
 if(!rejected){require(x.bytes==y.bytes,label+" memory order");require(old.events==modern.events,label+" complete shared-reader ABI");require(before.sites==after.sites,label+" source-site order");require(a->executed==b->executed,label+" instruction accounting");}
 ++cases;budgets+=expected=="Native instruction budget exhausted";faults+=expected.starts_with("Unmapped data access at ");negatives+=rejected;
 if(expected.empty()&&!rejected){++returns;++root_returns[index];for(auto pc:before.sites)if(flags::entries[index]<=pc&&pc<ends[index])sites[index].insert(pc);}
 return a;
}
}
namespace cm::aot {void actor_flags_step_seen(State& s,std::uint32_t pc){if(reference_trace)reference_trace->before_step(s,pc);}}
int main(int argc,char**argv){try{
 require(argc==2,"Expected source-site output");
 std::vector<std::uint32_t> words{0,0xffffffff,0xa55a1234};for(unsigned bit=0;bit<32;++bit)words.push_back(1u<<bit);
 for(unsigned index=0;index<7;++index){
  for(auto side:{0u,1u,127u,128u,255u,0x100u,0xabcdef01u})for(auto slot:{0u,1u,19u,127u,128u,255u,0xabcdef01u})for(auto low:words){
   auto input=seed(index,side,slot,low);const auto high=input->memory.read<4>(word_address(side,slot));auto result=compare(*input,index);
   if(index<4)require(result->r<4>(12)==unsigned((low&(1u<<bits[index]))!=0),"Independent mask oracle");
   else if(index<6)require(result->r<4>(12)==high&&result->r<4>(16)==low,"Independent ordered-word oracle");
   else require(result->r<4>(12)==unsigned((low&(1u<<23))!=0),"Independent slot flag fallback oracle");
  }
  for(unsigned byte=0;byte<256;++byte){compare(*seed(index,byte,19,0xffffffff),index);compare(*seed(index,1,byte,0xffffffff),index);}
  for(auto cr:{0u,1u,0xa5u,255u})for(auto carry:{0u,1u})for(auto so:{0u,1u})for(auto low:{0u,0xffffffffu}){
   auto input=seed(index,1,19,low);input->wr<1>(2304,cr);input->wr<1>(1027,carry);input->wr<1>(1024,so);compare(*input,index);
  }
  for(auto low:{0u,0xffffffffu}){
   auto input=seed(index,0,0,low);auto result=compare(*input,index);for(unsigned budget=0;budget<=result->executed+1;++budget){auto limited=clone(*input);limited->budget=budget;compare(*limited,index);}
  }
  Options negative;negative.negative=true;compare(*seed(index),index,negative);
  auto null_actor=seed(index);null_actor->wr<4>(12,0);compare(*null_actor,index);
  auto null_match=seed(index);null_match->memory.write<4>(actor+0x1a4,0);compare(*null_match,index);
  auto aliased_frame=seed(index);aliased_frame->wr<4>(12,stack+8-0x1a4);compare(*aliased_frame,index);
  Options mutation;mutation.mutate_low=true;compare(*seed(index),index,mutation);
  auto relocated=seed(index);relocated->wr<4>(8,toc+0x100);compare(*relocated,index);
 }
 for(auto side:{0u,1u,127u,128u,255u})for(auto value:{0u,1u,127u,128u,255u}){
  auto input=seed(6,side,0);input->memory.write<4>(team_address(side)+0x588,0x1000);input->memory.write<1>(match+6+std::uint32_t(std::int8_t(side)),value);auto result=compare(*input,6);require(result->r<4>(12)==value,"Independent raw team-byte override oracle");
 }
 for(auto value:{0u,1u,0xffffffffu}){auto input=seed(6,0,0,value);Options mutation;mutation.mutate_side=true;compare(*input,6,mutation);}
 Options composed;composed.child_negative=true;compare(*seed(4),4,composed);
 std::ofstream coverage(argv[1]);unsigned total=0;
 for(unsigned i=0;i<7;++i){const auto expected=(ends[i]-flags::entries[i])/4;require(sites[i].size()==expected&&root_returns[i]>0,"Complete productive source coverage root="+hex_address(flags::entries[i]));total+=sites[i].size();for(auto pc:sites[i])coverage<<i<<" "<<std::hex<<pc<<std::dec<<"\n";}
 std::cout<<"Actor flags comparisons="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" faults="<<faults<<" negatives="<<negatives<<" complete_sites="<<total<<"/185\n";
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
