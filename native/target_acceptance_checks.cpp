#include "target_acceptance.hpp"
#include "semantic_io_checks.hpp"
#include <fstream>
#include <set>
namespace cm::aot { bool target_acceptance_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
namespace target=cm::aot::target_acceptance;
constexpr auto actor=fixture+0x1000,match=fixture+0x10000,doubles=fixture+0x40000,singles=fixture+0x41000;
constexpr auto pool=fixture+0x42000,salt=fixture+0x40,cursor=fixture+0x44,pool_end=fixture+0x48;
constexpr auto angles=fixture+0x80000,distances=fixture+0x90000,position_constants=fixture+0xa0000,output=fixture+0x100;
constexpr std::array<std::uint32_t,4> ends{0x10245050,0x101eeca4,0x101ef3c0,0x101f0ac4};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{target::can_face_target,target::target_position_allowed,target::route_obstruction,target::cell_occupants};
std::vector<std::uint8_t> original_doubles,original_singles,original_position;
struct Mutation {std::uint32_t pc;unsigned reg;std::uint32_t value;bool operator==(const Mutation&)const=default;};
struct Trace:SemanticGuestObserver {
 std::vector<std::uint32_t> order;std::vector<Mutation> mutations,applied;
 void before_step(State& s,std::uint32_t pc)override{if(s.executed<s.budget){for(const auto& m:mutations)if(pc==m.pc){applied.push_back(m);s.wr<4>(m.reg*4,m.value);}order.push_back(pc);}}
};
thread_local Trace* reference_trace=nullptr;
struct Event {std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Options {bool position=true,response=false,negative=false,child_negative=false,mutate_heading=false;unsigned interrupt=0,draw=0;double distance=2;std::vector<Mutation> mutations;};
struct Services:SemanticDependencyDispatch {
 bool original;Options options;Trace& trace;std::vector<Event> calls;unsigned child_mutations=0;
 Services(bool old,const Options& o,Trace& t):original(old),options(o),trace(t){}
 void run_reference(State& s,std::uint32_t stop){bool first=true;while(s.pc!=stop){
  const auto own=std::find(target::entries.begin(),target::entries.end(),s.pc);
  if(!first&&own!=target::entries.end())call(s,s.pc,s.r<4>(4128));
  else if(!target_acceptance_reference(s))call(s,s.pc,s.r<4>(4128));first=false;
 }}
 void call(State& s,std::uint32_t entry,std::uint32_t continuation)override{
  const auto n=calls.size();calls.push_back({entry,continuation,s.registers,{}});
  const auto own=std::find(target::entries.begin(),target::entries.end(),entry);
  if(own!=target::entries.end()){
   if(original)run_reference(s,continuation);else methods[unsigned(own-target::entries.begin())](s,this,&trace);
  }else if(entry==0x10773808||entry==0x10773858){
   // Explicit FP-save/restore boundary model preserves all live argument GPRs.
   for(unsigned fp=28;fp<32;++fp){const auto address=std::uint32_t(s.r<4>(44))-8*(32-fp);
    if(entry==0x10773808)s.memory.write<8>(address,s.r<8>(0x4000+fp*16));
    else s.wr<8>(0x4000+fp*16,s.memory.read<8>(address));
   }s.pc=continuation;
  }else{
   auto value=options.draw;double floating=options.distance;bool fp_result=false;
   if(entry==0x10775540){const auto input=std::uint32_t(s.r<4>(12));value=std::int32_t(input)<0?0u-input:input;}
   else if(entry==0x10283690)value=s.memory.read<1>(std::uint32_t(s.r<4>(12))+0x27);
   else if(entry==0x101d5680)value=s.memory.read<1>(std::uint32_t(s.r<4>(12))+0x106);
   else if(entry==0x101d5690)value=s.memory.read<1>(std::uint32_t(s.r<4>(12))+0x105);
   else if(entry==0x101eea30)value=options.position;
   else if(entry==0x101ef8d0)fp_result=true;
   else if(entry==0x10239f10)value=options.response;
   else require(entry==0x1077b770||entry==0x10226520,"Unexpected target service "+hex_address(entry));
   // Boundary models deliberately poison caller-saved registers. They establish
   // caller contracts only; actual services require separate integration proof.
   for(unsigned reg=0;reg<=12;++reg)if(reg!=1&&reg!=2)s.wr<4>(reg*4,0xa7000000+reg*79);
   for(unsigned fp=0;fp<=13;++fp)s.wr<8>(0x4000+fp*16,float_bits<8>(-13.5-fp));
   s.wr<1>(2304,5);s.wr<4>(12,value);if(fp_result)s.wr<8>(0x4010,float_bits<8>(floating));s.pc=continuation;
   if(options.child_negative&&!original&&entry==0x10283690){s.memory.write<1>(fixture+0x10,1);++child_mutations;}
  }
  calls[n].after=s.registers;
  if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Fixture target service interrupted");
 }
};
std::uint32_t candidate(unsigned n){return fixture+0x4000+n*0x200;}
std::uint32_t cell(unsigned x,unsigned y){return match+0xa08+x*0x450+y*0x5c;}
std::unique_ptr<State> seed(unsigned index){
 auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned n=1280;n<1312;++n)s->wr<1>(n,0);
 s->memory.map(fixture,0xb0000);s->memory.map(stack-0x4000,0x5000);s->memory.map(toc-0x8000,0x9000);
 s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->pc=target::entries[index];s->budget=100000;
 s->memory.copy_in(doubles,original_doubles);s->memory.copy_in(singles,original_singles);s->memory.copy_in(position_constants,original_position);
 for(auto [offset,address]:std::array<std::pair<int,std::uint32_t>,10>{{{-0x51e8,doubles},{-0x51e0,singles},{-0x52c4,position_constants},{-0x641c,angles},{-0x6424,distances},{-0x7b90,salt},{-0x7b84,cursor},{-0x7b88,pool_end},{-0x7b8c,pool},{-0x6418,angles}}})s->memory.write<4>(toc+std::uint32_t(offset),address);
 s->memory.write<4>(cursor,pool);s->memory.write<4>(pool_end,pool+0x7fe);
 for(unsigned off=0;off<0x800;off+=2)s->memory.write<2>(pool+off,off/2);
 for(unsigned off=0;off<0x8000;off+=2)s->memory.write<2>(angles-0x4000+off,45);
 for(unsigned off=0;off<0x4000;off+=4)s->memory.write<4>(distances+off,float_bits<4>(5.0));
 s->memory.write<4>(actor+0x1a4,match);s->memory.write<1>(actor+0x105,4);s->memory.write<1>(actor+0x106,6);s->memory.write<1>(actor+0x10e,10);s->memory.write<2>(actor+0x19e,0);
 for(unsigned x=0;x<=8;++x)for(unsigned y=0;y<=11;++y){const auto c=cell(x,y);
  for(unsigned side=0;side<2;++side){for(unsigned n=0;n<11;++n)s->memory.write<4>(c+side*0x2c+n*4,candidate(n));s->memory.write<1>(c+0x58+side,2);}
 }
 for(unsigned n=0;n<11;++n){s->memory.write<1>(candidate(n)+0x19,n);s->memory.write<1>(candidate(n)+0x2c,0);}
 s->wr<8>(0x4010,float_bits<8>(10.0));s->wr<8>(0x4020,float_bits<8>(5.0));
 if(index==0){s->wr<4>(12,actor);s->wr<4>(16,5);s->wr<4>(20,7);s->wr<4>(24,1);s->wr<4>(36,output);}
 if(index==1){s->wr<4>(12,match);s->wr<4>(16,actor);s->wr<4>(20,5);s->wr<4>(24,7);}
 if(index==2){s->wr<4>(12,match);s->wr<4>(16,4);s->wr<4>(20,6);s->wr<4>(24,5);s->wr<4>(28,7);}
 if(index==3){s->wr<4>(12,match);s->wr<4>(16,0);s->wr<4>(20,cell(4,6));s->wr<4>(28,0xffffffff);s->wr<4>(32,0xffffffff);s->wr<4>(36,0xffffffff);s->wr<4>(40,2);}
 return s;
}
unsigned cases=0,returns=0,interruptions=0,negatives=0,checkpoint_returns=0,budget_stops=0,memory_faults=0,service_stops=0,fp_stops=0;
std::array<std::set<std::uint32_t>,4> complete_sites,ordinary_sites;std::array<unsigned,4> root_returns{};
struct Result {std::unique_ptr<State> state;unsigned call_count;};
Result compare(const State& input,unsigned index,const Options& options={}){
 auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();Trace before,after;before.mutations=after.mutations=options.mutations;Services old(true,options,before),modern(false,options,after);
 const auto mutate=[&](State& s,Effects& e,std::uint32_t address,unsigned){if(options.mutate_heading&&!e.mutated&&address==actor+0x19e){e.mutated=true;s.memory.write<2>(address,180);}};x.on_read=y.on_read=mutate;
 a->memory.date_observer=&x;b->memory.date_observer=&y;
 auto run=[&](State& s,Services& services,Trace& trace){try{reference_trace=&trace;if(services.original)services.run_reference(s,return_sentinel);else methods[index](s,&services,&trace);reference_trace=nullptr;return std::string{};}catch(const std::runtime_error& e){reference_trace=nullptr;return std::string(e.what());}};
 const auto expected=run(*a,old,before),actual=run(*b,modern,after);a->memory.date_observer=b->memory.date_observer=nullptr;
 const auto label="Target acceptance root="+hex_address(input.pc)+" case="+std::to_string(cases);
 require(expected==actual,label+" original="+expected+" candidate="+actual);require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Fixture target service interrupted"||expected=="Signed division by zero in translated code"||expected=="Original enabled floating invalid-conversion exception requires a handler"||expected=="Original enabled floating inexact exception requires a handler"||expected.starts_with("Unmapped data access at "),label+" unexpected outcome="+expected);
 if(options.negative)b->wr<4>(12,b->r<4>(12)^1);if(options.child_negative)require(modern.child_mutations>0,label+" child negative not exercised");
 bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.negative&&!options.child_negative)throw;rejected=true;}
 require(rejected==(options.negative||options.child_negative),label+" discriminating control");
 if(!rejected){require(x.bytes==y.bytes,label+" ordered memory effects");require(old.calls==modern.calls,label+" nested/service ABI");require(a->executed==b->executed,label+" instruction accounting");
  if(before.order!=after.order){auto mismatch=std::mismatch(before.order.begin(),before.order.end(),after.order.begin(),after.order.end());throw std::runtime_error(label+" source order offset="+std::to_string(mismatch.first-before.order.begin())+" old="+(mismatch.first==before.order.end()?"end":hex_address(*mismatch.first))+" new="+(mismatch.second==after.order.end()?"end":hex_address(*mismatch.second)));}}
 require(before.applied==after.applied,label+" applied source checkpoints");++cases;interruptions+=!expected.empty();negatives+=rejected;budget_stops+=expected=="Native instruction budget exhausted";memory_faults+=expected.starts_with("Unmapped data access at ");service_stops+=expected=="Fixture target service interrupted";fp_stops+=expected.starts_with("Original enabled floating ");
 if(expected.empty()&&!rejected){++returns;++root_returns[index];checkpoint_returns+=!before.applied.empty();for(auto pc:before.order)for(unsigned i=0;i<4;++i)if(target::entries[i]<=pc&&pc<ends[i]){complete_sites[i].insert(pc);if(before.applied.empty())ordinary_sites[i].insert(pc);}}
 return{std::move(a),unsigned(old.calls.size())};
}
}
namespace cm::aot {void target_acceptance_step_seen(State& s,std::uint32_t pc){if(reference_trace)reference_trace->before_step(s,pc);}}
int main(int argc,char**argv){try{
 require(argc==3,"Expected private original program and coverage output");State program;load_program(program,argv[1]);original_doubles=program.memory.copy_out(program.memory.read<4>(toc-0x51e8),0x400);original_singles=program.memory.copy_out(program.memory.read<4>(toc-0x51e0),0x200);original_position=program.memory.copy_out(program.memory.read<4>(toc-0x52c4),0x400);
 for(unsigned index=0;index<4;++index){auto input=seed(index);auto result=compare(*input,index);Options o;o.negative=true;compare(*input,index,o);
  auto bad=seed(index);bad->wr<4>(index==3?20:12,0);compare(*bad,index);
  for(unsigned stop=0;stop<=result.state->executed+1;++stop){auto limited=clone(*input);limited->budget=stop;compare(*limited,index);}
  for(unsigned stop=1;stop<=result.call_count;++stop){o={};o.interrupt=stop;compare(*input,index,o);}
 }
 // Full signed-coordinate, side, goalkeeper and distance decisions with actual
 // own-child composition and explicitly modeled external query boundaries.
 for(auto side:{0u,1u,127u,128u,255u})for(auto x:{0u,1u,2u,3u,4u,5u,6u,7u,8u,9u,127u,128u,255u})for(auto y:{0u,1u,2u,5u,9u,10u,11u,127u,128u,255u})for(auto position:{false,true}){
  auto input=seed(1);input->memory.write<1>(actor+0x27,side);input->memory.write<1>(actor+0x105,x);input->memory.write<1>(actor+0x106,y);Options o;o.position=position;compare(*input,1,o);
 }
 for(auto x:{0u,2u,3u,4u,5u,6u,8u,127u,128u,255u})for(auto y:{0u,1u,2u,5u,6u,9u,11u,127u,128u,255u})for(auto distance:{0.0,1.0,2.0,10.0,100.0}){
  auto input=seed(1);input->wr<4>(20,x);input->wr<4>(24,y);Options o;o.distance=distance;compare(*input,1,o);
 }
 // All four complete corridor scan forms, reversal, empty/invalid borders and
 // signed-byte truncation. Counts have a simple independent occupancy oracle.
 for(auto sx:{0u,4u,8u,9u,255u})for(auto sy:{0u,6u,11u,12u,255u})for(auto tx:{0u,4u,8u,9u,255u})for(auto ty:{0u,6u,11u,12u,255u})for(auto side:{0u,1u}){
  auto input=seed(2);input->wr<4>(16,sx);input->wr<4>(20,sy);input->wr<4>(24,tx);input->wr<4>(28,ty);input->memory.write<1>(match+0x7924,side);auto result=compare(*input,2);
  if(sx<=8&&tx<=8&&sy<=11&&ty<=11){const auto dx=sx>tx?sx-tx:tx-sx,dy=sy>ty?sy-ty:ty-sy;const auto traversed=!dx?dy:!dy?dx:std::max(dx,dy)+1;require(result.state->r<4>(12)==traversed*2,"Independent complete corridor occupancy oracle");}
 }
 for(auto mode:{0u,1u,2u,3u,255u})for(auto side:{0u,1u})for(auto count:{0u,1u,2u,10u,11u,127u,128u,255u})for(auto counter:{0u,1u,2u,127u,128u,255u})for(auto slot:{0u,1u,127u,128u,255u})for(auto response:{false,true}){
  auto input=seed(3);input->wr<4>(16,side);input->wr<4>(40,mode);input->memory.write<1>(cell(4,6)+0x58+side,count);
  for(unsigned n=0;n<11;++n){input->memory.write<1>(candidate(n)+0x19,slot);input->memory.write<1>(candidate(n)+0x2c,counter);}
  Options o;o.response=response;auto result=compare(*input,3,o);
  if(count<=11){const bool eligible=std::int8_t(counter)<=1&&std::int8_t(slot)>=0;const auto expected=eligible?(mode==2?count:unsigned(response&&count>0)):0;require(result.state->r<4>(12)==expected,"Independent occupant count/early-accept oracle");}
 }
 for(auto heading:{0u,1u,15u,30u,44u,45u,46u,90u,179u,180u,225u,315u,359u,360u,32767u,32768u,65535u})for(auto trait:{0u,1u,5u,20u,127u,128u,255u})for(auto qualify:{0u,1u})for(auto out:{0u,output})for(auto sample:{0u,5u,65535u}){
  auto input=seed(0);input->memory.write<2>(actor+0x19e,heading);input->memory.write<1>(actor+0x10e,trait);input->wr<4>(24,qualify);input->wr<4>(36,out);for(unsigned off=0;off<0x800;off+=2)input->memory.write<2>(pool+off,sample);auto result=compare(*input,0);
  if(!qualify&&!out&&heading<=360){auto difference=heading>45?heading-45:45-heading;if(difference>180)difference=360-difference;require(result.state->r<4>(12)==unsigned(difference<75),"Independent disabled-qualification facing oracle");}
 }
 for(auto f1:{-100.0,0.0,1.0,20.0,100.0})for(auto f2:{-100.0,0.0,1.0,20.0,100.0})for(auto heading:{0u,45u,90u,180u})for(auto trait:{0u,20u,127u,128u,255u})for(auto position:{false,true}){
  auto input=seed(0);input->wr<8>(0x4010,float_bits<8>(f1));input->wr<8>(0x4020,float_bits<8>(f2));input->memory.write<2>(actor+0x19e,heading);input->memory.write<1>(actor+0x10e,trait);Options o;o.position=position;compare(*input,0,o);
 }
 // Returning eligibility-true branch, empty/nonempty corridors and actual pool
 // wrap use ordinary inputs, not altered source registers or conditions.
 for(auto count:{0u,1u,2u})for(auto wrap:{false,true})for(auto heading:{0u,30u,44u,46u,90u})for(auto trait:{0u,5u,127u,128u,255u}){
  auto input=seed(0);input->memory.write<1>(actor+0x105,1);input->memory.write<1>(actor+0x106,9);input->memory.write<2>(actor+0x19e,heading);input->memory.write<1>(actor+0x10e,trait);input->memory.write<4>(pool_end,wrap?pool:pool+0x7fe);
  for(unsigned x=0;x<=8;++x)for(unsigned y=0;y<=11;++y)for(unsigned side=0;side<2;++side)input->memory.write<1>(cell(x,y)+0x58+side,count);
  Options o;o.distance=100;compare(*input,0,o);
 }
 for(auto first:{UINT64_C(0),UINT64_C(0x8000000000000000),UINT64_C(1),UINT64_C(0x7fefffffffffffff),UINT64_C(0x7ff0000000000000),UINT64_C(0xfff0000000000000),UINT64_C(0x7ff8000000000001)})for(auto second:{UINT64_C(0),UINT64_C(0x8000000000000000),UINT64_C(0x7ff0000000000000),UINT64_C(0x7ff8000000000011)})for(auto enabled:{false,true}){
  auto input=seed(0);input->wr<8>(0x4010,first);input->wr<8>(0x4020,second);for(unsigned flag=1280;flag<1312;++flag)input->wr<1>(flag,flag<1304||enabled);compare(*input,0);
 }
 {auto input=seed(0);input->memory.write<1>(actor+0x105,1);input->memory.write<1>(actor+0x106,9);Options o;o.distance=100;auto result=compare(*input,0,o);
  for(unsigned budget=0;budget<=result.state->executed+1;++budget){auto limited=clone(*input);limited->budget=budget;compare(*limited,0,o);}
 }
 Options child;child.child_negative=true;compare(*seed(1),1,child);Options mutation;mutation.mutate_heading=true;compare(*seed(0),0,mutation);
 for(auto out:{actor+0x10e,actor+0x19e,stack+0x44}){auto input=seed(0);input->wr<4>(36,out);compare(*input,0);}
 // The generic sampler arm is structurally wider than the square of a stable
 // signed byte. These explicit checkpoints are never labeled natural input.
 for(auto value:{65536u,0xffff0000u})for(auto draw:{0u,1000000u}){auto input=seed(0);Options o;o.draw=draw;o.mutations={{0x10244ee0,26,value}};compare(*input,0,o);}
 std::ofstream out(argv[2]);unsigned total=0,ordinary=0;
 for(unsigned i=0;i<4;++i){require(root_returns[i]>0,"Productive complete return per target root");total+=complete_sites[i].size();ordinary+=ordinary_sites[i].size();for(auto pc:complete_sites[i])out<<i<<" "<<std::hex<<pc<<std::dec<<" "<<ordinary_sites[i].contains(pc)<<"\n";std::cout<<"Root "<<hex_address(target::entries[i])<<" complete_sites="<<complete_sites[i].size()<<" ordinary_sites="<<ordinary_sites[i].size()<<" productive_returns="<<root_returns[i]<<"\n";}
 std::cout<<"Target acceptance comparisons="<<cases<<" returns="<<returns<<" interruptions="<<interruptions<<" budgets="<<budget_stops<<" faults="<<memory_faults<<" service_stops="<<service_stops<<" fp_stops="<<fp_stops<<" negatives="<<negatives<<" checkpoint_returns="<<checkpoint_returns<<" completed_sites="<<total<<"/758 ordinary_sites="<<ordinary<<"\n";
 require(total==758,"Complete own source coverage required before acceptance");
}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
