#include "contract_draft.hpp"
#include "contract_terms.hpp"
#include "semantic_io_checks.hpp"
#include <bit>
#include <set>
#include <fstream>
namespace cm::aot {bool contract_draft_reference(State&);std::vector<std::uint32_t> contract_draft_expected_sites();void contract_draft_fixture_tables(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace draft=cm::aot::contract_draft;
constexpr std::uint32_t record=fixture+0x1000,staff=fixture+0x3000,club=fixture+0x4000,contract=fixture+0x5000,today=fixture+0x6000,ending=fixture+0x6100,tables=fixture+0x10000,globals=fixture+0x20000;
constexpr std::array<std::uint32_t,3> ends{0x10372254,0x103723b0,0x1037258c};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{draft::initialize_draft,draft::prepare_new_contract,draft::prepare_from_primary_contract};
struct Options{unsigned interrupt=0;bool own_negative=false,effect_negative=false,primary_present=true,override_term_result=false;std::uint32_t primary_pointer=contract;std::uint16_t day=177,year=2025;std::uint8_t flag=3;};
struct Trace:SemanticGuestObserver {
    std::vector<std::uint32_t> sites;bool effect_negative=false,damaged=false;
    void before_step(State& s,std::uint32_t pc)override {sites.push_back(pc);if(effect_negative&&!damaged&&s.executed<s.budget){damaged=true;const auto v=s.memory.read<1>(globals+0x700);s.memory.write<1>(globals+0x700,v^1);s.memory.write<1>(globals+0x700,v);}}
};
thread_local Trace* original_trace=nullptr;
unsigned index(std::uint32_t root){const auto i=std::find(draft::entries.begin(),draft::entries.end(),root);require(i!=draft::entries.end(),"Known draft root");return unsigned(i-draft::entries.begin());}
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
    bool original;Options options;
    struct Event{std::uint32_t target,stop;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};std::vector<Event> calls;
    Model(bool old,Options o):original(old),options(o){}
    void reference(State& s){const auto n=index(s.pc),root=s.pc;while(s.pc!=return_sentinel){if(root<=s.pc&&s.pc<ends[n])require(contract_draft_reference(s),"Complete original draft reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));}}
    void call(State& s,std::uint32_t target,std::uint32_t stop)override {
        const auto serial=calls.size();calls.push_back({target,stop,s.registers,{}});
        try {
            const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};std::uint32_t result=0;bool raw=false;
            // Reuse accepted native leaf bodies on both sides of this boundary;
            // this component makes no new claim about their original proof.
            if(target==0x10374b60){contract_terms::clear_option(s);raw=true;}
            else if(target==0x10374b80){contract_terms::clear_bonus(s);raw=true;}
            else if(target==0x10380700){contract_terms::set_release_amount(s);raw=true;}
            else {
                s.memory.write<4>(globals+0x500,target);s.memory.write<4>(globals+0x504,s.memory.read<4>(globals+0x504)+1);
                if(target==0x10715880){s.memory.write<2>(arg(3),40);s.memory.write<2>(arg(3)+2,1972);s.memory.write<1>(arg(3)+4,1);result=arg(3);}
                else if(target==0x10717a10){(void)s.memory.read<2>(arg(4));(void)s.memory.read<2>(arg(4)+2);(void)s.memory.read<1>(arg(4)+4);s.memory.write<2>(arg(3),options.day);s.memory.write<2>(arg(3)+2,options.year);s.memory.write<1>(arg(3)+4,options.flag);result=arg(3);}
                else if(target==0x10082f20)result=options.primary_present?options.primary_pointer:0;
                else if(target==0x1037f050){const auto base=arg(3);for(auto offset:{0x4cu,0x45u,0x46u})s.memory.write<1>(base+offset,0);s.memory.write<1>(base+0x5c,4);s.memory.write<4>(base+0x48,0);s.memory.write<4>(base+0x50,0);result=base;}
                else if(target==0x10739890){(void)s.memory.read<1>(arg(3));s.memory.write<4>(arg(6),0x12345678);s.memory.write<4>(arg(7),0x87654321);result=globals+0x800;}
                else if(target==0x10768810)result=globals+0x900;
                else if(target==0x1077b720)(void)s.memory.read<1>(arg(4));
                else if(target==0x10769b60)(void)s.memory.read<1>(arg(3));
                else throw std::runtime_error("Unknown draft dependency "+hex_address(target));
                for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xdd000000+n*0x101);
                for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(1000.25+n));
                s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=stop;
            }
            if(raw&&options.override_term_result)s.wr<4>(12,0xaabbccdd);
            calls[serial].after=s.registers;if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Draft fixture interruption");
        }catch(...){calls[serial].after=s.registers;throw;}
    }
};
std::unique_ptr<State> seed(unsigned n){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);s->memory.map(fixture,0x30000);s->memory.map(stack-0x1000,0x2000);s->memory.map(toc-0x8000,0x9000);s->pc=draft::entries[n];s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->budget=100000;
    s->memory.copy_in(record,std::vector<std::uint8_t>(100,0xa5));s->memory.write<4>(staff,1);s->memory.write<4>(club,2);s->memory.write<4>(contract+12,1000);
    s->memory.write<2>(today,364);s->memory.write<2>(today+2,2000);s->memory.write<1>(today+4,1);s->memory.write<2>(ending,10);s->memory.write<2>(ending+2,2002);s->memory.write<1>(ending+4,1);
    s->memory.write<4>(toc-0x7b80,today);s->memory.write<4>(toc-0x7b7c,globals);s->memory.write<4>(globals,globals+0x100);s->memory.write<4>(toc-0x7ba8,globals+0x200);s->memory.write<4>(toc-0x7bac,globals+0x400);s->memory.write<1>(globals+0x400,255);
    s->wr<4>(12,record);s->wr<4>(16,staff);s->wr<4>(20,club);s->wr<4>(24,n==2?ending:12);s->wr<4>(28,100);s->wr<4>(32,0x12345678);s->wr<4>(36,0xabcdef01);contract_draft_fixture_tables(*s,toc,tables);return s;
}
unsigned cases=0,returns=0,stops=0,negatives=0;std::array<unsigned,3> root_cases{},root_returns{},root_stops{},root_negatives{};std::set<std::uint32_t> complete_sites,all_sites;
struct Result{std::unique_ptr<State> state;std::vector<Effects::Access> accesses;unsigned calls;std::string outcome;};
Result compare(const State& input,Options options={}){
    auto a=clone(input),b=clone(input);TimedEffects x(*a),y(*b);Trace ot,nt;nt.effect_negative=options.effect_negative;Model old(true,options),modern(false,options);a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto run=[&](State& s,Model& d,bool reference){try{if(reference){original_trace=&ot;d.reference(s);}else methods[index(s.pc)](s,&d,&nt);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;const auto label="Draft root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Draft fixture interruption"||expected.starts_with("Unmapped data access at "),label+" unexpected reference: "+expected);require(expected==actual,label+" original="+expected+" modern="+actual);
    const auto negative=options.own_negative||options.effect_negative;if(negative)require(expected.empty(),label+" negative needs a complete return");if(options.own_negative)b->wr<4>(12,b->r<4>(12)^1);if(options.effect_negative){require(nt.damaged,label+" effect negative must execute");Image(*a).compare(*b,label+" restored effect image");}
    bool rejected=false;try{require(old.calls==modern.calls,label+" dependency ABI/order");require(x.effects.bytes==y.effects.bytes&&x.pcs==y.pcs,label+" ordered timed effects");require(ot.sites==nt.sites,label+" source order");require(a->executed==b->executed,label+" instruction count");Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!negative)throw;rejected=true;}require(rejected==negative,label+" discriminating negative");
    const auto root=index(input.pc);++cases;++root_cases[root];returns+=expected.empty()&&!negative;root_returns[root]+=expected.empty()&&!negative;stops+=!expected.empty();root_stops[root]+=!expected.empty();negatives+=negative;root_negatives[root]+=negative;if(!negative){all_sites.insert(ot.sites.begin(),ot.sites.end());if(expected.empty())complete_sites.insert(ot.sites.begin(),ot.sites.end());}
    return{std::move(a),x.effects.log,unsigned(old.calls.size()),expected};
}
void boundaries(const State& input,Options options={}){
    const auto base=compare(input,options);require(base.outcome.empty()&&base.state->pc==return_sentinel,"Draft boundary baseline return");const auto steps=base.state->executed-input.executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto s=clone(input);s->budget=s->executed+cut;compare(*s,options);}
    std::set<std::uint32_t> bytes;for(const auto& a:base.accesses)if(a.size){bytes.insert(a.address);bytes.insert(a.address+std::uint32_t(a.size)-1);}std::set<std::uint32_t> holes;if(!bytes.empty())for(unsigned n=0;n<5;++n){auto i=bytes.begin();std::advance(i,std::ptrdiff_t(n*(bytes.size()-1)/4));holes.insert(*i);}
    for(auto hole:holes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;s->budget=input.budget;s->executed=input.executed;for(auto [at,size]:input.memory.mapped_ranges())for(auto [address,n]:hole<at||std::uint64_t(hole)>=std::uint64_t(at)+size?std::vector<std::pair<std::uint32_t,std::uint64_t>>{{at,size}}:std::vector<std::pair<std::uint32_t,std::uint64_t>>{{at,hole-at},{hole+1,std::uint64_t(at)+size-hole-1}})if(n){s->memory.map(address,n);s->memory.copy_in(address,input.memory.copy_out(address,n));}require(compare(*s,options).outcome.starts_with("Unmapped data access at "),"Removed draft byte must fault");}
    for(unsigned cut:std::set<unsigned>{1,base.calls/2,base.calls})if(cut&&cut<=base.calls){auto o=options;o.interrupt=cut;compare(input,o);}
}
void corpus(){
    for(unsigned n=0;n<3;++n){auto s=seed(n);boundaries(*s);Options o;o.own_negative=true;compare(*s,o);o.own_negative=false;o.effect_negative=true;compare(*s,o);}
    for(auto byte:{0u,1u,0x7fu,0x80u,0xffu}){auto s=seed(0);s->memory.copy_in(record,std::vector<std::uint8_t>(100,byte));auto out=compare(*s);require(out.state->memory.read<4>(record)==byte*0x01010101u,"Default initialization retains unwritten ID");require(out.state->memory.read<1>(record+0x2d)==byte,"Default date retains adjacent padding");require(out.state->memory.read<4>(record+4)==0xffffffff&&out.state->memory.read<4>(record+0x58)==0xffffffff,"Independent default sentinels");}
    for(unsigned n:{1u,2u})for(unsigned bits=0;bits<256;++bits){auto s=seed(n);s->memory.write<1>(record+0x38,bits);auto out=compare(*s);require(out.state->memory.read<1>(record+0x38)==((bits&3)|(n==1?20:24)),"Packed kind preserves only the low two bits");}
    for(auto weeks:{0u,1u,12u,26u,52u,127u,128u,255u,256u,0xffffffffu}){auto s=seed(1);s->wr<4>(24,weeks);compare(*s);}
    for(auto day:{0u,1u,0x7fffu,0x8000u,0xffffu})for(auto year:{0u,1972u,2000u,0x7fffu,0xffffu})for(auto flag:{0u,1u,255u}){auto s=seed(1);Options o;o.day=day;o.year=year;o.flag=flag;auto out=compare(*s,o);require(out.state->memory.read<2>(record+0x30)==day&&out.state->memory.read<2>(record+0x32)==year&&out.state->memory.read<1>(record+0x34)==flag,"Declared end-date bytes forwarded exactly");}
    for(auto amount:{0u,1u,99u,1000u,0x7fffffffu,0x80000000u,0xffffffffu})for(unsigned rate=0;rate<256;++rate){auto s=seed(2);s->memory.write<4>(contract+12,amount);s->wr<4>(28,rate);auto out=compare(*s);const auto product=amount*std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(rate))));const auto expected=std::uint32_t(std::bit_cast<std::int32_t>(product)/100);require(out.state->memory.read<4>(record+12)==expected&&out.state->memory.read<4>(record+0x3c)==expected,"Wrapped product then signed division toward zero");}
    for(auto first:{0u,1u,0x80u,0xffu,0x12345678u})for(auto second:{0u,1u,255u,0xabcdef01u}){auto s=seed(2);s->wr<4>(32,first);s->wr<4>(36,second);auto out=compare(*s);require(out.state->memory.read<1>(record+0x1c)==(first&255)&&out.state->memory.read<1>(record+0x1d)==(second&255),"Final option bytes follow clause reset");}
    {auto s=seed(2);Options o;o.primary_present=false;boundaries(*s,o);auto out=compare(*s,o);require(out.state->memory.copy_out(record,100)==s->memory.copy_out(record,100)&&out.state->r<4>(12)==globals+0x400,"Missing primary preserves draft and diagnostic return residue");}
    for(unsigned n:{1u,2u}){auto s=seed(n);Options o;o.override_term_result=true;require(compare(*s,o).state->r<4>(12)==0xaabbccdd,"Final dependency result remains observable");}
    for(unsigned n:{1u,2u})for(unsigned which=0;which<(n==1?3u:5u);++which){auto s=seed(n);Options o;if(which==0)s->wr<4>(16,record);if(which==1)s->wr<4>(20,record);if(which==2)s->memory.write<4>(toc-0x7b80,record);if(which==3)s->wr<4>(24,record+0x28);if(which==4)o.primary_pointer=record;compare(*s,o);}
    for(unsigned n=0;n<3;++n)for(unsigned arg:{3u,4u,5u}){auto s=seed(n);s->wr<4>(arg*4,0);compare(*s);}
}
}
namespace cm::aot {void contract_draft_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try{require(argc==1||argc==2,"Optional coverage output");corpus();const auto expected=contract_draft_expected_sites();if(argc==2){std::ofstream out(argv[1]);for(auto pc:expected)out<<hex_address(pc)<<" "<<all_sites.contains(pc)<<" "<<complete_sites.contains(pc)<<'\n';}for(auto pc:expected)if(!complete_sites.contains(pc))std::cerr<<"Uncovered draft site "<<hex_address(pc)<<std::endl;std::cout<<"Draft comparisons="<<cases<<" returns="<<returns<<" stops="<<stops<<" negatives="<<negatives<<" completed_sites="<<complete_sites.size()<<"/"<<expected.size()<<std::endl;for(unsigned n=0;n<3;++n)std::cout<<"Root "<<hex_address(draft::entries[n])<<" comparisons="<<root_cases[n]<<" returns="<<root_returns[n]<<" stops="<<root_stops[n]<<" negatives="<<root_negatives[n]<<std::endl;require(complete_sites.size()==expected.size(),"Every draft instruction needs complete-return evidence");require(std::all_of(root_negatives.begin(),root_negatives.end(),[](auto n){return n==2;}),"Every draft root needs result and restored-effect negatives");}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
