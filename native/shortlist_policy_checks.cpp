#include "shortlist_policy.hpp"
#include "semantic_io_checks.hpp"
#include "compiler_arithmetic.hpp"
#include <bit>
#include <map>
#include <set>
#include <fstream>
namespace cm::aot {bool shortlist_policy_reference(State&);std::vector<std::uint32_t> shortlist_policy_expected_sites();void shortlist_policy_fixture_tables(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace policy=cm::aot::shortlist_policy;
constexpr std::uint32_t object=fixture+0x1000,staff=fixture+0x2000,profiles=fixture+0x4000,row=fixture+0x6000,contract=fixture+0x8000,countries=fixture+0xa000,clubs=fixture+0x10000,tables=fixture+0x20000,globals=fixture+0x30000,output=fixture+0x35000;
constexpr std::array<std::uint32_t,3> ends{0x1036f588,0x1036f664,0x1036ff00};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{policy::classify_contract_offer,policy::check_candidate_country_policy,policy::evaluate_row_contract_gate};
struct Options{std::map<std::uint32_t,std::uint32_t> words;unsigned interrupt=0;bool own_negative=false,effect_negative=false,contract_present=true;};
struct Trace:SemanticGuestObserver{std::vector<std::uint32_t> sites;void before_step(State&,std::uint32_t pc)override{sites.push_back(pc);}};
thread_local Trace* original_trace=nullptr;
unsigned index(std::uint32_t root){const auto i=std::find(policy::entries.begin(),policy::entries.end(),root);require(i!=policy::entries.end(),"Known policy root");return unsigned(i-policy::entries.begin());}
struct TimedEffects:MemoryObserver {
    State& state;Effects effects;std::vector<std::uint32_t> pcs;
    explicit TimedEffects(State& s):state(s){effects.state=&s;}
    void mapped(std::uint32_t a,std::uint64_t n)override{pcs.push_back(state.pc);effects.mapped(a,n);}
    void written(std::uint32_t a,const std::uint8_t* d,std::size_t n)override{pcs.push_back(state.pc);effects.written(a,d,n);}
    void cleared(std::uint32_t a,std::uint64_t n)override{pcs.push_back(state.pc);effects.cleared(a,n);}
    void read(std::uint32_t a,const std::uint8_t* d,unsigned n)override{pcs.push_back(state.pc);effects.read(a,d,n);}
};
struct Event{std::uint32_t target,stop;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Model:SemanticDependencyDispatch {
    bool original;Options options;std::vector<Event> calls;
    Model(bool old,Options o):original(old),options(std::move(o)){}
    void reference(State& s){const auto n=index(s.pc),root=s.pc;bool first=true;while(s.pc!=return_sentinel){if(root<=s.pc&&s.pc<ends[n]&&(first||s.pc!=root))require(shortlist_policy_reference(s),"Complete original policy reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));first=false;}}
    void call(State& s,std::uint32_t target,std::uint32_t stop)override {
        const auto serial=calls.size();calls.push_back({target,stop,s.registers,{}});
        try{
            const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};std::uint32_t result=0;bool raw=false;
            s.memory.write<4>(globals+0x500,target);s.memory.write<4>(globals+0x504,s.memory.read<4>(globals+0x504)+1);
            if(target==0x10082f90)result=options.contract_present?contract:0;
            else if(target==0x1014c2e0||target==0x100d3230||target==0x10380520||target==0x10147e70||target==0x10149df0||target==0x100d3040||target==0x100ca6a0||target==0x103ed520)result=0;
            else if(target==0x10089a40||target==0x10085860||target==0x103fac70)result=1000;
            else if(target==0x100c96e0)result=std::uint32_t(s.memory.read<4>(arg(3)+0x2c));
            else if(target==0x10380f20||target==0x103faff0)result=1;
            else if(target==0x10380710)result=1;
            else if(target==0x10773c80){compiler_arithmetic::signed_pair_to_double(s);raw=true;}
            else throw std::runtime_error("Unknown policy dependency "+hex_address(target));
            if(!raw){
                if(options.words.contains(target))result=options.words.at(target);
                for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xdd000000+n*0x101);
                for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(1000.25+n));
                s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);
            }
            s.pc=stop;
            if(!original&&options.effect_negative){const auto before=s.memory.read<1>(globals+0x700);s.memory.write<1>(globals+0x700,before^1);s.memory.write<1>(globals+0x700,before);}
            calls[serial].after=s.registers;if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Policy fixture interruption");
        }catch(...){calls[serial].after=s.registers;throw;}
    }
};
std::unique_ptr<State> seed(unsigned n){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);s->memory.map(fixture,0x40000);s->memory.map(stack-0x1000,0x2000);s->memory.map(toc-0x8000,0x9000);s->pc=policy::entries[n];s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->budget=100000;
    for(auto [slot,ptr,value]:{std::tuple{-0x7b7c,globals,object},std::tuple{-0x7a54,globals+4,object+0x100},std::tuple{-0x7a30,globals+8,99u},std::tuple{-0x7b40,globals+12,object+0x200},std::tuple{-0x7be0,globals+16,staff}}){s->memory.write<4>(toc+slot,ptr);s->memory.write<4>(ptr,value);}
    for(unsigned i=0;i<4;++i){s->memory.write<4>(staff+i*0x70,i);s->memory.write<4>(staff+i*0x70+8,20000);s->memory.write<4>(staff+i*0x70+0xc,countries);s->memory.write<4>(staff+i*0x70+0x14,profiles+i*0x80);s->memory.write<2>(profiles+i*0x80+10,8000);s->memory.write<1>(profiles+i*0x80+15,0);s->memory.write<4>(clubs+i*0x248+0x14,7);s->memory.write<4>(clubs+i*0x248+0x2c,countries+i*0x200);s->memory.write<4>(clubs+i*0x248+0x90,staff+3*0x70);s->memory.write<2>(clubs+i*0x248+0x1ec,5000);}
    s->memory.write<4>(contract,1);s->memory.write<1>(contract+0x38,8);s->memory.write<1>(contract+0x60,6);s->memory.write<1>(contract+0x44,0xcd);s->memory.write<4>(row+4,100);s->memory.write<4>(row+12,10);s->memory.write<4>(output,0x76543210);s->memory.write<1>(countries+0x200+0x118,14);
    s->wr<4>(12,object);s->wr<4>(16,n==0?clubs:n==1?staff+0x70:row);s->wr<4>(20,n==2?contract:staff+0x70);s->wr<4>(24,n==0?0:clubs+0x248);s->wr<4>(28,n==0?10000:output);s->wr<4>(32,clubs+0x248);s->wr<4>(36,0);s->wr<4>(40,2000);s->memory.write<1>(stack+0x3b,0);
    shortlist_policy_fixture_tables(*s,toc,tables);return s;
}
unsigned cases=0,returns=0,stops=0,negatives=0;std::array<unsigned,3> root_cases{},root_returns{},root_stops{},root_negatives{};std::set<std::uint32_t> complete_sites,all_sites;
struct Result{std::unique_ptr<State> state;std::vector<Effects::Access> accesses;unsigned calls;std::string outcome;};
Result compare(const State& input,Options options={}){
    auto a=clone(input),b=clone(input);TimedEffects x(*a),y(*b);Trace ot,nt;Model old(true,options),modern(false,options);a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto run=[&](State& s,Model& d,bool reference){try{if(reference){original_trace=&ot;d.reference(s);}else methods[index(s.pc)](s,&d,&nt);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;const auto label="Policy root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Policy fixture interruption"||expected.starts_with("Unmapped data access at "),label+" unexpected reference: "+expected);require(expected==actual,label+" original="+expected+" modern="+actual);
    const auto negative=options.own_negative||options.effect_negative;if(negative)require(expected.empty(),label+" negative needs a complete return");if(options.own_negative)b->wr<4>(12,b->r<4>(12)^1);if(options.effect_negative){require(!old.calls.empty(),label+" effect negative requires an actual dependency");Image(*a).compare(*b,label+" restored effect image");}
    bool rejected=false;try{require(old.calls==modern.calls,label+" dependency ABI/order");require(x.effects.bytes==y.effects.bytes&&x.pcs==y.pcs,label+" ordered timed effects");require(ot.sites==nt.sites,label+" source order");require(a->executed==b->executed,label+" instruction count");Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!negative)throw;rejected=true;}require(rejected==negative,label+" discriminating negative");
    const auto root=index(input.pc);++cases;++root_cases[root];returns+=expected.empty()&&!negative;root_returns[root]+=expected.empty()&&!negative;stops+=!expected.empty();root_stops[root]+=!expected.empty();negatives+=negative;root_negatives[root]+=negative;if(!negative){all_sites.insert(ot.sites.begin(),ot.sites.end());if(expected.empty())complete_sites.insert(ot.sites.begin(),ot.sites.end());}
    return{std::move(a),x.effects.log,unsigned(old.calls.size()),expected};
}
void boundaries(const State& input,Options options={}){
    const auto base=compare(input,options);require(base.outcome.empty()&&base.state->pc==return_sentinel,"Policy boundary baseline return");const auto steps=base.state->executed-input.executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto s=clone(input);s->budget=s->executed+cut;compare(*s,options);}
    std::set<std::uint32_t> bytes;for(const auto& a:base.accesses)if(a.size){bytes.insert(a.address);bytes.insert(a.address+std::uint32_t(a.size)-1);}std::set<std::uint32_t> holes;if(!bytes.empty())for(unsigned n=0;n<5;++n){auto i=bytes.begin();std::advance(i,std::ptrdiff_t(n*(bytes.size()-1)/4));holes.insert(*i);}
    for(auto hole:holes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;s->budget=input.budget;s->executed=input.executed;for(auto [at,size]:input.memory.mapped_ranges())for(auto [address,n]:hole<at||std::uint64_t(hole)>=std::uint64_t(at)+size?std::vector<std::pair<std::uint32_t,std::uint64_t>>{{at,size}}:std::vector<std::pair<std::uint32_t,std::uint64_t>>{{at,hole-at},{hole+1,std::uint64_t(at)+size-hole-1}})if(n){s->memory.map(address,n);s->memory.copy_in(address,input.memory.copy_out(address,n));}require(compare(*s,options).outcome.starts_with("Unmapped data access at "),"Removed policy byte must fault");}
    for(unsigned cut:std::set<unsigned>{1,base.calls/2,base.calls})if(cut&&cut<=base.calls){auto o=options;o.interrupt=cut;compare(input,o);}
}
void corpus(){
    for(unsigned n=0;n<3;++n){auto s=seed(n);boundaries(*s);Options o;o.own_negative=true;compare(*s,o);o.own_negative=false;o.effect_negative=true;if(n==1)s->memory.write<4>(staff+0x70+0x20,1);compare(*s,o);}
    for(unsigned present:{0u,1u})for(unsigned mode:{0u,2u,5u})for(unsigned flag:{0u,1u})for(unsigned kind:{0u,1u})for(unsigned domestic:{0u,1u,2u,0xffffffffu}){
        auto s=seed(0);s->memory.write<1>(contract+0x38,mode*4);s->memory.write<1>(stack+0x3b,flag);Options o;o.contract_present=present;o.words[0x1014c2e0]=kind;o.words[0x100d3230]=domestic;o.words[0x10147e70]=domestic;o.words[0x10149df0]=domestic;compare(*s,o);
    }
    for(unsigned variant=0;variant<12;++variant){auto s=seed(0);if(variant==0)s->wr<4>(16,0);if(variant==1)s->memory.write<1>(contract+0x54,1);if(variant==2)s->memory.write<4>(clubs+0x90,staff+0x70);if(variant>=3)s->memory.write<1>(stack+0x3b,1);if(variant==4)s->memory.write<2>(profiles+0x80+10,0);if(variant==5)s->memory.write<1>(contract+0x60,0);if(variant==6)s->memory.write<1>(profiles+0x80+15,20);if(variant==7)s->memory.write<1>(contract+0x44,0xce);if(variant==8)s->memory.write<4>(clubs+0x14,99);if(variant==9)s->memory.write<1>(contract+0x60,7);Options o;o.words[0x10380520]=variant&1;o.words[0x100d3040]=variant&1;compare(*s,o);}
    for(auto high:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto low:{0u,999u,1000u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto price:{0u,999u,1000u,0x7fffffffu,0x80000000u,0xffffffffu}){
        auto s=seed(0);s->wr<4>(24,high);s->wr<4>(28,low);s->wr<4>(36,high);s->wr<4>(40,low);Options o;o.words[0x10085860]=price;compare(*s,o);
    }
    // The second signed pair is an explicit optional amount; the -1 sentinel
    // selects the service-derived reference rather than reusing the first pair.
    for(auto amount:{0u,100u,500u,900u,999u,1000u,19999u})for(unsigned profile:{0u,1u}){
        auto state=seed(0);state->wr<4>(28,amount);state->wr<4>(36,0xffffffffu);state->wr<4>(40,0xffffffffu);if(!profile)state->memory.write<4>(staff+0x70+0x14,0);compare(*state);
    }
    {auto state=seed(0);state->wr<4>(28,2500000);Options o;o.words[0x1014c2e0]=1;o.words[0x10380520]=1;o.words[0x10089a40]=4000000;o.words[0x100d3040]=1;compare(*state,o);}
    {auto state=seed(0);state->wr<4>(28,900);state->wr<4>(36,0xffffffffu);state->wr<4>(40,0xffffffffu);boundaries(*state);}
    for(unsigned mask=0;mask<128;++mask){auto s=seed(1);if(mask&1)s->memory.write<4>(staff+0x70+0xc,0);if(mask&2)s->memory.write<1>(countries+0x200+0x118,0);if(mask&4)s->memory.write<4>(staff+0x70+0xc,countries+0x200);if(mask&8)s->memory.write<4>(staff+0x70+0x10,countries+0x200);if(mask&16)s->memory.write<4>(staff+0x70+0x20,1);if(mask&32)s->memory.write<1>(staff+0x70+0x5e,1);Options o;o.words[0x100ca6a0]=(mask&64)?1:0;o.words[0x103ed520]=mask&1;compare(*s,o);}
    for(unsigned available:{0u,1u})for(unsigned permitted:{0u,1u})for(unsigned kind:{0u,1u,2u,3u,4u,5u,6u,7u,8u,9u,14u,15u,0xffffffffu})for(auto value:{0u,10u,100u,1000u,30000u,0xffffffffu}){
        auto s=seed(2);s->memory.write<4>(row+12,value);Options o;o.words[0x10380f20]=available;o.words[0x103faff0]=permitted;o.words[0x10380710]=kind;o.words[0x103fac70]=value;compare(*s,o);
    }
    for(unsigned which=0;which<3;++which){auto s=seed(2);if(which==0)s->wr<4>(20,0);if(which==1)s->wr<4>(16,0);if(which==2)s->memory.write<1>(contract+0x54,255);compare(*s);}
    for(auto alias:{row,row+4,row+12,contract,contract+0x54,staff+0x70,staff+0x70+8}){auto s=seed(2);s->wr<4>(28,alias);compare(*s);}
}
}
namespace cm::aot {void shortlist_policy_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try{require(argc==1||argc==2,"Optional coverage output");corpus();const auto expected=shortlist_policy_expected_sites();if(argc==2){std::ofstream out(argv[1]);for(auto pc:expected)out<<hex_address(pc)<<" "<<all_sites.contains(pc)<<" "<<complete_sites.contains(pc)<<'\n';}for(auto pc:expected)if(!complete_sites.contains(pc))std::cerr<<"Uncovered policy site "<<hex_address(pc)<<std::endl;std::cout<<"Policy comparisons="<<cases<<" returns="<<returns<<" stops="<<stops<<" negatives="<<negatives<<" completed_sites="<<complete_sites.size()<<"/"<<expected.size()<<std::endl;for(unsigned n=0;n<3;++n)std::cout<<"Root "<<hex_address(policy::entries[n])<<" comparisons="<<root_cases[n]<<" returns="<<root_returns[n]<<" stops="<<root_stops[n]<<" negatives="<<root_negatives[n]<<std::endl;require(complete_sites.size()==expected.size(),"Every policy instruction needs complete-return evidence");require(std::all_of(root_negatives.begin(),root_negatives.end(),[](auto n){return n==2;}),"Every policy root needs result and restored-effect negatives");}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
