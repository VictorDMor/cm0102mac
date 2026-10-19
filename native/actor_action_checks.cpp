#include "actor_action.hpp"
#include "semantic_io_checks.hpp"
#include <fstream>
#include <map>
#include <set>
namespace cm::aot {bool actor_action_reference(State&);}
namespace {
using namespace cm::semantic_io_checks;
namespace action=cm::aot::actor_action;
constexpr auto actor=fixture+0x1000,peer=fixture+0x3000,match=fixture+0x10000,stats=fixture+0x30000,traits=fixture+0x31000;
constexpr auto doubles=fixture+0x40000,singles=fixture+0x41000,pool=fixture+0x42000,distances=fixture+0x50000,strings=fixture+0x60000,output=fixture+0x100;
constexpr auto salt=fixture+0x40,cursor=fixture+0x44,pool_end=fixture+0x48;
constexpr std::array<std::uint32_t,5> ends{0x1027821c,0x10281778,0x102818f0,0x10281aec,0x1028353c};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{action::route_actor_action,action::select_primary_action,action::can_reach_destination,action::choose_turn_side,action::select_secondary_action};
std::vector<std::uint8_t> original_doubles,original_singles;
struct Mutation{std::uint32_t pc;unsigned reg;std::uint32_t value;bool operator==(const Mutation&)const=default;};
struct Trace:SemanticGuestObserver{
 std::vector<std::uint32_t> order;std::vector<Mutation> mutations,applied;std::set<std::pair<std::uint32_t,unsigned>> applied_checkpoints;
 void before_step(State& s,std::uint32_t pc)override{if(s.executed<s.budget){
  // Checkpoints change the first dynamic visit only. Later loop iterations
  // resume their original conditions rather than forcing an endless loop.
  for(const auto& m:mutations)if(m.pc==pc&&applied_checkpoints.insert({pc,m.reg}).second){applied.push_back(m);if(m.reg==32)s.wr<1>(2304,(s.r<1>(2304)&0xf0)|m.value);else s.wr<4>(m.reg*4,m.value);}
  order.push_back(pc);
 }}
};
thread_local Trace* reference_trace=nullptr;
struct Event{std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Options{std::map<std::uint32_t,std::uint32_t> values;std::vector<Mutation> mutations;std::uint32_t role=1,query=0,heading=45,draw=0,high_flags=0,low_flags=0;double distance=2;bool negative=false,child_negative=false,release_holder=false,stop_retry=false;unsigned interrupt=0;};
#include "actor_action_check_fixtures.hpp"
struct Services:SemanticDependencyDispatch{
 bool original;Options options;Trace& trace;std::vector<Event> calls;unsigned child_mutations=0;
 Services(bool old,const Options& o,Trace& t):original(old),options(o),trace(t){}
 void run_reference(State& s,std::uint32_t stop){bool first=true;while(s.pc!=stop){auto own=std::find(action::entries.begin(),action::entries.end(),s.pc);if(!first&&own!=action::entries.end())call(s,s.pc,s.r<4>(4128));else if(!actor_action_reference(s))call(s,s.pc,s.r<4>(4128));first=false;}}
 void call(State& s,std::uint32_t entry,std::uint32_t continuation)override{
  auto n=calls.size();calls.push_back({entry,continuation,s.registers,{}});auto own=std::find(action::entries.begin(),action::entries.end(),entry);
  if(own!=action::entries.end()){
   if(entry==action::entries[0]&&options.stop_retry){s.memory.write<1>(actor+0x2c,1);s.memory.write<4>(match+0xe000,0);}
   if(original)run_reference(s,continuation);else methods[unsigned(own-action::entries.begin())](s,this,&trace);
  }else if(entry==0x10773808||entry==0x10773858){
   for(unsigned fp=28;fp<32;++fp){const auto address=std::uint32_t(s.r<4>(44))-8*(32-fp);if(entry==0x10773808)s.memory.write<8>(address,s.r<8>(0x4000+fp*16));else s.wr<8>(0x4000+fp*16,s.memory.read<8>(address));}s.pc=continuation;
  }else{
   const auto arg3=std::uint32_t(s.r<4>(12)),arg4=std::uint32_t(s.r<4>(16));auto value=options.query;bool fp_result=false;double fp_value=options.distance;
   if(entry==0x10775540)value=std::int32_t(arg3)<0?0u-arg3:arg3;
   else if(entry==0x10289560)value=options.role;
   else if(entry==0x101f1e10)value=peer;
   else if(entry==0x1077b770||entry==0x10226520)value=options.draw;
   else if(entry==0x101ee610)value=options.heading;
   else if(entry==0x101ef8d0)fp_result=true;
   else if(entry==0x10780fc0){
    // The import tail saves caller TOC in its CFM linkage slot. The original
    // caller reloads it explicitly after the floating math function returns.
    fp_value=std::pow(float_value<8>(s.r<8>(0x4010)),float_value<8>(s.r<8>(0x4020)));fp_result=true;s.memory.write<4>(std::uint32_t(s.r<4>(4))+20,s.r<4>(8));s.wr<4>(8,0);
   }
   else if(entry==0x102857c0)value=arg4&255;
   else if(entry==0x102864b0)value=options.high_flags;
   else if(entry==0x10252080){s.memory.write<1>(s.r<4>(16),0);s.memory.write<2>(s.r<4>(20),100);}
   else if(entry==0x10738c00){s.memory.write<2>(arg3,0);}
   else if(entry==0x101d8260||entry==0x101e25a0||entry==0x101e28b0){s.memory.write<1>(s.r<4>(20),5);s.memory.write<1>(s.r<4>(24),7);}
   else if(entry==0x102374f0||entry==0x1022e7e0){s.memory.write<1>(arg3+0x105,s.r<4>(16));s.memory.write<1>(arg3+0x106,s.r<4>(20));if(options.release_holder){s.memory.write<4>(match+0xe000,0);s.memory.write<1>(match+0x7928,0);}}
   else if(entry==0x10245220){if(options.release_holder){s.memory.write<4>(match+0xe000,0);s.memory.write<1>(match+0x7928,0);}}
   else if(entry==0x102836a0){s.memory.write<2>(arg3+0x19c,103);}
   else if(entry==0x10235db0){s.memory.write<4>(arg3+0x80,float_bits<4>(1.0));}
   else require(std::find(std::begin(known),std::end(known),entry)!=std::end(known),"Unexpected action dependency "+hex_address(entry));
   if(auto found=options.values.find(entry);found!=options.values.end())value=found->second;
   for(unsigned reg=0;reg<=12;++reg)if(reg!=1&&reg!=2)s.wr<4>(reg*4,0xa6000000+reg*73);
   for(unsigned fp=0;fp<=13;++fp)s.wr<8>(0x4000+fp*16,float_bits<8>(-19.5-fp));
   s.wr<1>(2304,5);s.wr<4>(12,value);if(fp_result)s.wr<8>(0x4010,float_bits<8>(fp_value));if(entry==0x102864b0)s.wr<4>(16,options.low_flags);s.pc=continuation;
   if(options.child_negative&&!original&&entry==0x10289560){s.memory.write<1>(fixture+0x10,1);++child_mutations;}
  }
  calls[n].after=s.registers;if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Fixture action service interrupted");
 }
 static constexpr std::uint32_t known[]{0x101149f0,0x101d69c0,0x101d7dc0,0x101d8120,0x101d8260,0x101d8d70,0x101e25a0,0x101e28b0,0x101e3d50,0x101e3f10,0x101e4020,0x101e4e10,0x101e4ea0,0x101e8950,0x101ee610,0x101ee680,0x101ee770,0x101ee9c0,0x101ee9f0,0x101eea30,0x101eeac0,0x101ef3c0,0x101ef8d0,0x101f0dd0,0x101f1e10,0x101f21d0,0x101f5280,0x101fd000,0x10201180,0x10226520,0x1022e7e0,0x10235db0,0x102374f0,0x10237830,0x10239e50,0x10244c60,0x10245220,0x10245870,0x10252080,0x102576e0,0x10274170,0x102775e0,0x10278220,0x10281780,0x102818f0,0x10281af0,0x102836a0,0x10285790,0x102857c0,0x102862b0,0x10286320,0x10286390,0x10286400,0x102864b0,0x102864f0,0x10289560,0x1028abc0,0x10738c00,0x10738c20,0x10739890,0x10768810,0x10769b60,0x1076f7d0,0x10773808,0x10773858,0x10775540,0x1077b720,0x1077b770,0x10780fc0};
};
std::unique_ptr<State> seed(unsigned index){
 auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned n=1280;n<1312;++n)s->wr<1>(n,0);
 s->memory.map(fixture,0x90000);s->memory.map(stack-0x8000,0x9000);s->memory.map(toc-0x8000,0x9000);
 s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(12,actor);s->wr<4>(4128,return_sentinel);s->pc=action::entries[index];s->budget=20000;
 s->memory.copy_in(doubles,original_doubles);s->memory.copy_in(singles,original_singles);
 for(auto [off,address]:std::array<std::pair<int,std::uint32_t>,10>{{{-0x51e8,doubles},{-0x51e0,singles},{-0x51e4,strings},{-0x6424,distances},{-0x7b90,salt},{-0x7b84,cursor},{-0x7b88,pool_end},{-0x7b8c,pool},{-0x7ba8,output},{-0x7bac,strings+0x1000}}})s->memory.write<4>(toc+std::uint32_t(off),address);
 s->memory.write<4>(toc-0x525c,fixture+0x70000);s->memory.write<4>(fixture+0x70000+0,0x10278cfc);s->memory.write<4>(fixture+0x70000+4,0x10278cfc);s->memory.write<4>(fixture+0x70000+8,0x1027846c);s->memory.write<4>(fixture+0x70000+12,0x10278360);s->memory.write<4>(fixture+0x70000+16,0x102785f4);s->memory.write<4>(fixture+0x70000+20,0x102787b0);s->memory.write<4>(fixture+0x70000+24,0x1027895c);s->memory.write<4>(fixture+0x70000+28,0x10278890);s->memory.write<4>(fixture+0x70000+32,0x10278350);
 s->memory.write<4>(cursor,pool);s->memory.write<4>(pool_end,pool+0x7fe);for(unsigned n=0;n<0x800;n+=2)s->memory.write<2>(pool+n,n/2);
 for(unsigned n=0;n<0x4000;n+=4)s->memory.write<4>(distances+n,float_bits<4>(5.0));
 for(auto a:{actor,peer}){s->memory.write<4>(a+0x1a4,match);s->memory.write<4>(a+0x70,stats);s->memory.write<4>(a+0x6c,traits);s->memory.write<1>(a+0x105,4);s->memory.write<1>(a+0x106,6);s->memory.write<2>(a+0x36,7000);s->memory.write<1>(a+0x19,0);s->memory.write<4>(a+0x80,float_bits<4>(5.0));}
 for(unsigned x=0;x<9;++x)for(unsigned y=0;y<12;++y){const auto cell=match+0xa08+x*0x450+y*0x5c;for(unsigned side=0;side<2;++side){s->memory.write<4>(cell+side*0x2c,peer);s->memory.write<1>(cell+0x58+side,1);}}
 s->memory.write<1>(peer+0x24,1);
 s->memory.write<1>(actor+0x104,1);s->memory.write<4>(actor+0x1b4,peer);s->memory.write<4>(match+0xe000,0);s->memory.write<4>(match+0xdff8,0);s->memory.write<1>(match+0x791f,0);s->memory.write<1>(match+0x7924,0);
 s->wr<4>(16,5);s->wr<4>(20,7);s->wr<4>(24,4);s->wr<4>(28,11);s->wr<4>(32,0);s->wr<4>(40,output);s->wr<8>(0x4010,float_bits<8>(5.0));
 return s;
}
unsigned cases=0,returns=0,interruptions=0,negatives=0,checkpoint_returns=0;
std::map<std::tuple<unsigned,std::string,std::uint32_t>,unsigned> outcomes;
std::array<std::set<std::uint32_t>,5> complete_sites,ordinary_sites;std::array<unsigned,5> root_returns{};
struct Result{std::unique_ptr<State> state;unsigned calls;std::string outcome;std::vector<std::uint32_t> order;bool rejected;};
Result compare(const State& input,unsigned index,const Options& options={}){
 auto a=clone(input),b=clone(input);Trace before,after;before.mutations=after.mutations=options.mutations;Services old(true,options,before),modern(false,options,after);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
 auto run=[&](State& s,Services& services,Trace& trace){try{reference_trace=&trace;if(services.original)services.run_reference(s,return_sentinel);else methods[index](s,&services,&trace);reference_trace=nullptr;return std::string{};}catch(const std::runtime_error&e){reference_trace=nullptr;return std::string(e.what());}};
 const auto expected=run(*a,old,before),actual=run(*b,modern,after);a->memory.date_observer=b->memory.date_observer=nullptr;auto label="Component actor-action root="+hex_address(input.pc)+" case="+std::to_string(cases);
 require(expected==actual,label+" original="+expected+" candidate="+actual);require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Fixture action service interrupted"||expected=="Signed division by zero in translated code"||expected.starts_with("Original enabled floating ")||expected.starts_with("Unmapped data access at "),label+" unexpected outcome "+expected);
 if(options.negative)b->wr<4>(12,b->r<4>(12)^1);if(options.child_negative)require(modern.child_mutations>0,label+" child negative did not execute");bool rejected=false;
 try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.negative&&!options.child_negative)throw;rejected=true;}
 require(rejected==(options.negative||options.child_negative),label+" discriminating control");
 if(!rejected){require(x.bytes==y.bytes,label+" memory ordering");require(old.calls==modern.calls,label+" dependency ABI");require(a->executed==b->executed,label+" source accounting");if(before.order!=after.order){auto m=std::mismatch(before.order.begin(),before.order.end(),after.order.begin(),after.order.end());throw std::runtime_error(label+" source order offset="+std::to_string(m.first-before.order.begin())+" old="+(m.first==before.order.end()?"end":hex_address(*m.first))+" new="+(m.second==after.order.end()?"end":hex_address(*m.second)));}}
 if(!expected.empty()&&expected!="Native instruction budget exhausted"&&!options.interrupt&&input.budget==20000)++outcomes[{index,expected,a->pc}];
 require(before.applied==after.applied,label+" source checkpoints");++cases;interruptions+=!expected.empty();negatives+=rejected;
 if(expected.empty()&&!rejected){
  ++returns;++root_returns[index];checkpoint_returns+=!before.applied.empty();for(auto pc:before.order)for(unsigned i=0;i<5;++i)if(action::entries[i]<=pc&&pc<ends[i]){complete_sites[i].insert(pc);if(before.applied.empty())ordinary_sites[i].insert(pc);}

 }
 return{std::move(a),unsigned(old.calls.size()),expected,std::move(before.order),rejected};
}
}
namespace cm::aot{void actor_action_step_seen(State& s,std::uint32_t pc){if(reference_trace)reference_trace->before_step(s,pc);}}
int main(int argc,char**argv){try{
 require(argc==3,"Expected private original source and coverage output");State original;load_program(original,argv[1]);original_doubles=original.memory.copy_out(original.memory.read<4>(toc-0x51e8),0x1000);original_singles=original.memory.copy_out(original.memory.read<4>(toc-0x51e0),0x1000);
 for(auto index:{0u,1u,2u,3u,4u}){
  auto input=seed(index);auto result=compare(*input,index);Options bad;bad.negative=true;compare(*input,index,bad);auto null_actor=seed(index);null_actor->wr<4>(12,0);compare(*null_actor,index);
  for(unsigned budget=0;budget<=result.state->executed+1;++budget){auto limited=clone(*input);limited->budget=budget;compare(*limited,index);}
  for(unsigned stop=1;stop<=result.calls;++stop){Options o;o.interrupt=stop;compare(*input,index,o);}
 }
 // Destination and turn support: signed narrowing, equal cells, raw flags and
 // strict thresholds use independent component inputs, not real service proof.
 for(auto index:{2u,3u})for(auto side:{0u,1u,127u,128u,255u})for(auto x:{0u,1u,4u,8u,127u,128u,255u})for(auto y:{0u,1u,6u,11u,127u,128u,255u})for(auto value:{0u,1u,20u,127u,128u,255u}){
  auto input=seed(index);input->memory.write<1>(actor+0x27,side);input->memory.write<1>(actor+0x105,x);input->memory.write<1>(actor+0x106,y);input->memory.write<1>(actor+0x10f,value);input->memory.write<1>(stats+0x30,value);input->memory.write<1>(stats+0x3b,value);Options o;o.heading=value;compare(*input,index,o);
 }
 constexpr unsigned codes[]{0,1,2,3,4,0x33,0x34,0x35,0x36,0x37,0x39,0x3d,0x5a,0x5b,0x5c,0xffff};
 for(auto code:codes)for(auto heading:{0u,1u,45u,90u,179u,180u,181u,359u,360u,32767u,32768u,65535u})for(auto speed:{0.0,1.0,5.0,100.0}){
  auto input=seed(3);input->wr<4>(16,code);input->memory.write<2>(actor+0x19e,heading);input->wr<8>(0x4010,float_bits<8>(speed));Options o;o.heading=45;compare(*input,3,o);
 }
 // Every original classification-table index, including the two aliases, uses
 // the complete9558-site selector body. No missing-primary model is used.
 for(unsigned mode=0;mode<10;++mode)for(auto kind:{0u,1u,2u,4u}){auto input=seed(1);input->wr<4>(16,kind);input->memory.write<1>(match+0x7928,mode);compare(*input,1);}
 // Router/secondary policy combinations preserve live possession links and
 // each complete helper ABI; primary and secondary selectors now execute complete bodies.
 for(auto index:{0u,4u})for(auto side:{0u,1u})for(auto holder:{0u,actor,peer})for(auto secondary:{0u,actor,peer})for(auto role:{0u,1u,2u,4u,0x80u,0x800u,0xffffffffu})for(auto query:{0u,1u,5u,127u,255u}){
  auto input=seed(index);input->memory.write<1>(actor+0x27,side);input->memory.write<4>(match+0xe000,holder);input->memory.write<4>(match+0xdff8,secondary);Options o;o.role=role;o.query=query;compare(*input,index,o);
 }
 for(auto index:{0u,4u})for(auto off:{0x27u,0x104u,0x107u,0x10au,0x10du,0x110u,0x2cu})for(auto value:{0u,1u,2u,5u,20u,127u,128u,255u})for(auto sample:{0u,1u,19u,900u,65535u}){
  auto input=seed(index);input->memory.write<1>(actor+off,value);if(off==0x104){input->memory.write<1>(actor+off,value>1?1:value);}for(unsigned off=0;off<0x800;off+=2)input->memory.write<2>(pool+off,sample);Options o;o.role=0;o.query=1;compare(*input,index,o);
 }
 // Broader ordinary actor/possession/strategy fixtures. These do not alter
 // source checkpoints and are retained separately from later structural cases.
 for(auto index:{0u,1u,4u})for(auto side:{0u,1u})for(unsigned x=0;x<9;++x)for(unsigned y=0;y<12;++y)for(auto query:{0u,1u}){
  auto input=seed(index);input->memory.write<1>(actor+0x27,side);input->memory.write<1>(actor+0x105,x);input->memory.write<1>(actor+0x106,y);Options o;o.role=0;o.query=query;compare(*input,index,o);
 }
 for(auto index:{0u,1u,4u})for(auto field:{0x791fu,0x7924u,0x7928u,0x1c0u,0x1c4u,0x1c5u,0x1deu})for(auto value:{0u,1u,2u,3u,4u,5u,8u,10u,11u,127u,128u,255u})for(auto query:{0u,1u}){
  auto input=seed(index);input->memory.write<1>(match+field,value);Options o;o.query=query;compare(*input,index,o);
 }
 for(auto index:{0u,1u,4u})for(auto field:{0x11u,0x17u,0x19u,0x27u,0x2cu,0x2du,0x2eu,0x2fu,0x103u,0x107u,0x109u,0x10au,0x10cu,0x10du,0x10eu,0x110u})for(auto value:{0u,1u,2u,4u,5u,10u,20u,127u,128u,255u})for(auto query:{0u,1u}){
  auto input=seed(index);input->memory.write<1>(actor+field,value);Options o;o.query=query;compare(*input,index,o);
 }
 for(auto index:{0u,1u,4u})for(auto field:{0x50u,0x80u,0x98u,0x9cu,0xa0u,0xa4u,0xacu,0xb4u,0xe4u})for(auto value:{-65536.0,-1.0,0.0,1.0,5.0,20.0,100.0,65536.0})for(auto query:{0u,1u}){
  auto input=seed(index);input->memory.write<4>(actor+field,float_bits<4>(value));Options o;o.query=query;o.role=0;compare(*input,index,o);
 }
 for(auto index:{1u,4u})for(auto service:{0x101149f0u,0x101e3d50u,0x102862b0u,0x102864f0u,0x10286320u,0x10286390u,0x10286400u})for(unsigned bit=0;bit<32;++bit){
  auto input=seed(index);Options o;o.values[service]=UINT32_C(1)<<bit;o.high_flags=UINT32_C(1)<<bit;o.low_flags=UINT32_C(1)<<bit;compare(*input,index,o);
 }
 for(auto index:{1u,4u})for(auto service:{0x101e8950u,0x101fd000u,0x101f21d0u,0x101e3f10u,0x101e4020u})for(auto who:{actor,peer}){
  auto input=seed(index);Options o;o.values[service]=who;compare(*input,index,o);
 }
 // Model opaque action effects at their ABI boundary so the complete
 // routing caller observes a released holder after the complete primary call.
 for(auto mode:{0u,2u,3u,4u,5u,6u,7u,8u})for(auto speed:{0.0,1.0,5.0,100.0,65536.0,-65536.0})for(auto sample:{0u,1u,5u,100u,65535u}){
  auto input=seed(0);input->memory.write<4>(match+0xe000,actor);input->memory.write<1>(match+0x7928,mode);input->memory.write<1>(actor+0x10a,1);input->memory.write<4>(actor+0x80,float_bits<4>(speed));for(unsigned off=0;off<0x800;off+=2)input->memory.write<2>(pool+off,sample);Options o;o.release_holder=true;compare(*input,0,o);
 }
 for(auto index:{1u,4u})for(auto service:{0x101149f0u,0x101e8950u,0x101e3d50u,0x101ee680u,0x101ee770u,0x101ee9f0u,0x101f0dd0u,0x101f5280u,0x101d7dc0u,0x10285790u,0x1028abc0u,0x102862b0u,0x102864f0u,0x10286320u,0x10286390u,0x10286400u})for(auto side:{0u,1u})for(auto possession:{0u,1u,255u})for(auto role:{0u,1u,2u,4u,0x80u}){
  auto input=seed(index);input->memory.write<1>(actor+0x27,side);input->memory.write<1>(match+0x7924,possession);input->memory.write<4>(match+0xdff8,peer);Options o;o.values[service]=service==0x101e8950?peer:1;o.role=role;compare(*input,index,o);
 }
 // Retained full-entry regression for divw. OV/SO normalization and the
 // zero-divisor fault boundary. Only explicitly listed source checkpoints
 // enter the rare diagnostic path; these cases are not ordinary coverage.
 {
  auto regression=seed(1);auto& input=*regression;
  // Exact synthetic input retained from the pre-fix whole-entry failure.
  for(auto who:{actor,peer})input.memory.write<4>(who+0x6c,0);
  input.memory.write<1>(peer+0x24,0);input.memory.write<4>(toc-0x7bac,0);input.memory.write<1>(actor+0x10c,2);
  for(unsigned x=0;x<9;++x)for(unsigned y=0;y<12;++y){const auto cell=match+0xa08+x*0x450+y*0x5c;for(unsigned side=0;side<2;++side){input.memory.write<4>(cell+side*0x2c,0);input.memory.write<1>(cell+0x58+side,0);}}
  for(auto so:{0u,1u,0xa5u,0xffu})for(auto ov:{0u,1u,0xa5u}){
   input.wr<1>(1024,so);input.wr<1>(1025,ov);Options o;o.mutations={{0x1027ec34,32,8}};compare(input,1,o);
  }
  for(auto numerator:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto denominator:{0u,1u,2u,0xffffffffu}){
   Options o;o.mutations={{0x1027ec34,32,8},{0x1027eedc,3,numerator},{0x1027eedc,0,denominator}};compare(input,1,o);
  }
 }
 // Small independent domain oracles supplement the differential proof.
 {auto input=seed(2);input->wr<4>(16,4);input->wr<4>(20,6);auto result=compare(*input,2);require(result.state->r<4>(12)==1,"Equal destination is reachable");}
 for(auto code:{0x33u,0x34u,0x35u,0x36u,0x37u,0x39u,0x3du,0x5au,0x5bu,0x5cu,3u}){auto input=seed(3);input->wr<4>(16,code);auto result=compare(*input,3);require(result.state->r<4>(12)==2&&result.state->memory.read<2>(output)==0,"Turn-exempt action clears adjustment and returns neutral");}
 {auto input=seed(0);input->memory.write<1>(actor+0x2c,1);auto result=compare(*input,0);require(result.state->memory.read<2>(actor+0x19c)==103,"Temporary contact maintenance action tag");}
 {auto input=seed(3);input->wr<8>(0x4010,float_bits<8>(0.0));Options o;o.mutations={{0x10281a4c,32,4}};compare(*input,3,o);}
 Options negative;negative.child_negative=true;compare(*seed(0),0,negative);
 // Frozen prepared inputs and complete diagnostic paths replace exploration.
 // Numeric/CR changes apply to the first dynamic visit only; pointer changes
 // belong exclusively to the prepared inputs. The table bounds are intact.
 const auto fixed_input=[&](unsigned id){
  require(id<fixed_prepared_seeds.size(),"Unknown fixed actor-action seed");const auto& fixture=fixed_prepared_seeds[id];auto input=seed(fixture.index);
  input->memory.write<4>(match+0xdff8,peer);input->memory.write<4>(match+0xe024,peer);input->memory.write<4>(match+0xe028,actor);
  if(fixture.pool_fill>=0)for(unsigned offset=0;offset<0x800;offset+=2)input->memory.write<2>(pool+offset,unsigned(fixture.pool_fill));
  for(const auto& change:fixture.registers)input->wr<1>(change.address,change.value);
  for(const auto& change:fixture.memory)input->memory.write<1>(change.address,change.value);return input;
 };
 for(unsigned id=0;id<fixed_prepared_seeds.size();++id){const auto& fixture=fixed_prepared_seeds[id];auto result=compare(*fixed_input(id),fixture.index,fixture.options);require(result.outcome.empty()&&!result.rejected,"Fixed prepared seed did not return");}
 for(const auto& path:fixed_checkpoint_cases){const auto& fixture=fixed_prepared_seeds[path.seed];auto options=fixture.options;options.mutations=path.mutations;auto result=compare(*fixed_input(path.seed),fixture.index,options);require(result.outcome.empty()&&!result.rejected,"Frozen diagnostic path did not return");}

 std::ofstream failures(std::string(argv[2])+".faults");for(const auto& [key,count]:outcomes){const auto& [root,message,pc]=key;failures<<root<<" "<<hex_address(pc)<<" "<<count<<" "<<message<<"\n";}
 std::ofstream out(argv[2]);unsigned total=0,ordinary=0;
 for(unsigned i=0;i<5;++i){total+=complete_sites[i].size();ordinary+=ordinary_sites[i].size();for(auto pc:complete_sites[i])out<<i<<" "<<std::hex<<pc<<std::dec<<" "<<ordinary_sites[i].contains(pc)<<"\n";std::cout<<"Root "<<hex_address(action::entries[i])<<" complete_sites="<<complete_sites[i].size()<<" ordinary_sites="<<ordinary_sites[i].size()<<" productive_returns="<<root_returns[i]<<"\n";}
 require(total==12243,"Incomplete returning actor-action source coverage");
 std::cout<<"PASS: actor-action comparisons="<<cases<<" returns="<<returns<<" interruptions="<<interruptions<<" negatives="<<negatives<<" checkpoint_returns="<<checkpoint_returns<<" covered_family_sites="<<total<<"/12243 ordinary_sites="<<ordinary<<"; explicit modeled services and separate diagnostic checkpoints; actual-service proof is separate\n";
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
