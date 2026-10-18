#include "profile_qualification.hpp"
#include "semantic_io_checks.hpp"
#include <map>
#include <set>
#include <fstream>
namespace cm::aot {bool profile_qualification_reference(State&);std::vector<std::uint32_t> profile_qualification_expected_sites();void profile_qualification_fixture_tables(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace qualification=cm::aot::profile_qualification;
constexpr std::uint32_t object=fixture+0x1000,staff=fixture+0x2000,profile=fixture+0x4000,club=fixture+0x6000,country=fixture+0x8000,tables=fixture+0x10000,globals=fixture+0x20000;
constexpr std::array<std::uint32_t,4> ends{0x10117488,0x10117688,0x1036f6e8,0x1036f7d8};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{qualification::combine_qualification_masks,qualification::classify_profile_qualifications,qualification::candidate_category_reference,qualification::club_category_reference};
struct Options{unsigned interrupt=0;bool own_negative=false,effect_negative=false,children=true;std::uint32_t mask=0x801,reference=12345;};
struct Trace:SemanticGuestObserver {
    std::vector<std::uint32_t> sites;bool effect_negative=false,damaged=false;
    void before_step(State& s,std::uint32_t pc)override {
        sites.push_back(pc);
        if(effect_negative&&!damaged&&s.executed<s.budget){damaged=true;const auto value=s.memory.read<1>(globals+0x700);s.memory.write<1>(globals+0x700,value^1);s.memory.write<1>(globals+0x700,value);}
    }
};
thread_local Trace* original_trace=nullptr;
unsigned index(std::uint32_t root){const auto i=std::find(qualification::entries.begin(),qualification::entries.end(),root);require(i!=qualification::entries.end(),"Known qualification root");return unsigned(i-qualification::entries.begin());}
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
    bool original;Options options;Trace* trace=nullptr;
    std::array<unsigned,4> completed_children{};
    struct Event{std::uint32_t target,stop;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};std::vector<Event> calls;
    Model(bool old,Options o):original(old),options(o){}
    void reference(State& s,std::uint32_t root,std::uint32_t stop){const auto n=index(root);while(s.pc!=stop){if(root<=s.pc&&s.pc<ends[n])require(profile_qualification_reference(s),"Complete original qualification reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));}}
    void call(State& s,std::uint32_t target,std::uint32_t stop)override {
        const auto serial=calls.size();calls.push_back({target,stop,s.registers,{}});
        try {
            const auto selected=std::find(qualification::entries.begin(),qualification::entries.end(),target);
            if(selected!=qualification::entries.end()&&options.children){if(original)reference(s,target,stop);else {methods[index(target)](s,this,trace);++completed_children[index(target)];}}
            else {
                std::uint32_t result=0;
                s.memory.write<4>(globals+0x500,target);s.memory.write<4>(globals+0x504,s.memory.read<4>(globals+0x504)+1);
                if(target==qualification::classify_profile_qualifications_entry)result=options.mask;
                else if(target==qualification::club_category_reference_entry)result=options.reference;
                else if(target==0x10739890){(void)s.memory.read<1>(std::uint32_t(s.r<4>(12)));s.memory.write<4>(s.r<4>(24),0x12345678);s.memory.write<4>(s.r<4>(28),0x87654321);result=globals+0x800;}
                else if(target==0x10768810)result=globals+0x900;
                else if(target==0x1077b720)(void)s.memory.read<1>(std::uint32_t(s.r<4>(16)));
                else if(target==0x10769b60)(void)s.memory.read<1>(std::uint32_t(s.r<4>(12)));
                else throw std::runtime_error("Unknown qualification dependency "+hex_address(target));
                for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xdd000000+n*0x101);
                s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=stop;
            }
            calls[serial].after=s.registers;if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Qualification fixture interruption");
        }catch(...){calls[serial].after=s.registers;throw;}
    }
};
std::unique_ptr<State> seed(unsigned n){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);s->memory.map(fixture,0x30000);s->memory.map(stack-0x1000,0x2000);s->memory.map(toc-0x8000,0x9000);s->pc=qualification::entries[n];s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->budget=100000;
    s->wr<4>(0,0);s->wr<4>(20,0);s->memory.write<4>(toc-0x7ba8,globals+0x100);s->memory.write<4>(toc-0x7bac,globals+0x400);s->memory.write<1>(globals+0x400,0xff);
    const std::array<int,5> slots{-0x7a14,-0x7a10,-0x7a18,-0x7af4,-0x7a00};for(unsigned i=0;i<slots.size();++i){s->memory.write<4>(toc+slots[i],globals+i*4);s->memory.write<4>(globals+i*4,100+i);}
    s->memory.write<4>(staff+0x14,profile);s->memory.write<4>(club+0x2c,country);s->memory.write<4>(country+0x68,100);s->memory.write<2>(club+0x1ec,1000);s->memory.write<1>(profile+15,15);s->memory.write<1>(profile+23,15);
    if(n==0){s->wr<4>(12,0x801);s->wr<4>(16,0x204);}
    else if(n==1)s->wr<4>(12,profile);
    else {s->wr<4>(12,object);s->wr<4>(16,club);s->wr<4>(20,n==2?staff:0x801);s->wr<4>(24,4);}
    profile_qualification_fixture_tables(*s,toc,tables);return s;
}
unsigned cases=0,returns=0,stops=0,negatives=0;std::array<unsigned,4> root_cases{},root_returns{},root_stops{},root_negatives{};std::set<std::uint32_t> complete_sites,all_sites;
std::array<unsigned,4> accepted_children{};
struct Result{std::unique_ptr<State> state;std::vector<Effects::Access> accesses;unsigned calls;std::string outcome;};
Result compare(const State& input,Options options={}){
    auto a=clone(input),b=clone(input);TimedEffects x(*a),y(*b);Trace ot,nt;nt.effect_negative=options.effect_negative;Model old(true,options),modern(false,options);old.trace=&ot;modern.trace=&nt;a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto run=[&](State& s,Model& d,bool reference){try{if(reference){original_trace=&ot;d.reference(s,input.pc,return_sentinel);}else methods[index(s.pc)](s,&d,&nt);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;const auto label="Qualification root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Qualification fixture interruption"||expected.starts_with("Unmapped data access at "),label+" unexpected reference: "+expected);require(expected==actual,label+" original="+expected+" modern="+actual);
    const auto negative=options.own_negative||options.effect_negative;if(negative)require(expected.empty(),label+" negative needs a complete return");if(options.own_negative)b->wr<4>(12,b->r<4>(12)^1);if(options.effect_negative){require(nt.damaged,label+" effect negative must execute");Image(*a).compare(*b,label+" restored effect image");}
    bool rejected=false;try{require(old.calls==modern.calls,label+" dependency ABI/order");require(x.effects.bytes==y.effects.bytes&&x.pcs==y.pcs,label+" ordered timed effects");require(ot.sites==nt.sites,label+" source order");require(a->executed==b->executed,label+" instruction count");Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!negative)throw;rejected=true;}require(rejected==negative,label+" discriminating negative");
    if(expected.empty()&&!negative&&options.children)for(unsigned n=0;n<qualification::entries.size();++n){const auto calls=std::count_if(old.calls.begin(),old.calls.end(),[&](const auto& call){return call.target==qualification::entries[n];});require(modern.completed_children[n]==calls,label+" complete selected child evidence");accepted_children[n]+=modern.completed_children[n];}
    const auto root=index(input.pc);++cases;++root_cases[root];returns+=expected.empty()&&!negative;root_returns[root]+=expected.empty()&&!negative;stops+=!expected.empty();root_stops[root]+=!expected.empty();negatives+=negative;root_negatives[root]+=negative;if(!negative){all_sites.insert(ot.sites.begin(),ot.sites.end());if(expected.empty())complete_sites.insert(ot.sites.begin(),ot.sites.end());}
    return{std::move(a),x.effects.log,unsigned(old.calls.size()),expected};
}
void boundaries(const State& input,Options options={}){
    const auto base=compare(input,options);require(base.outcome.empty()&&base.state->pc==return_sentinel,"Qualification boundary baseline return");const auto steps=base.state->executed-input.executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto s=clone(input);s->budget=s->executed+cut;compare(*s,options);}
    std::set<std::uint32_t> bytes;for(const auto& a:base.accesses)if(a.size){bytes.insert(a.address);bytes.insert(a.address+std::uint32_t(a.size)-1);}std::set<std::uint32_t> holes;if(!bytes.empty())for(unsigned n=0;n<5;++n){auto i=bytes.begin();std::advance(i,std::ptrdiff_t(n*(bytes.size()-1)/4));holes.insert(*i);}
    for(auto hole:holes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;s->budget=input.budget;s->executed=input.executed;for(auto [at,size]:input.memory.mapped_ranges())for(auto [address,n]:hole<at||std::uint64_t(hole)>=std::uint64_t(at)+size?std::vector<std::pair<std::uint32_t,std::uint64_t>>{{at,size}}:std::vector<std::pair<std::uint32_t,std::uint64_t>>{{at,hole-at},{hole+1,std::uint64_t(at)+size-hole-1}})if(n){s->memory.map(address,n);s->memory.copy_in(address,input.memory.copy_out(address,n));}require(compare(*s,options).outcome.starts_with("Unmapped data access at "),"Removed qualification byte must fault");}
    for(unsigned cut:std::set<unsigned>{1,base.calls/2,base.calls})if(cut&&cut<=base.calls){auto o=options;o.interrupt=cut;compare(input,o);}
}
void corpus(){
    for(unsigned n=0;n<4;++n){auto s=seed(n);boundaries(*s);Options o;o.own_negative=true;compare(*s,o);o.own_negative=false;o.effect_negative=true;compare(*s,o);}
    // Every pair of primary and secondary table positions, plus multi-bit masks.
    for(unsigned x=0;x<7;++x)for(unsigned y=0;y<7;++y)for(unsigned side=7;side<12;++side)for(unsigned other=7;other<12;++other){auto s=seed(0);s->wr<4>(12,(1u<<x)|(1u<<side));s->wr<4>(16,(1u<<y)|(1u<<other));compare(*s);}
    for(auto mask:{0u,1u,0x7fu,0xf80u,0xfffu,0x8000u,0xffffu,0x80000801u})for(auto other:{0u,1u,0x7fu,0xf80u,0xfffu,0xffffu})for(auto residue:{0u,0x7fu,0x80u,0xffu}){auto s=seed(0);s->wr<4>(12,mask);s->wr<4>(16,other);s->wr<4>(0,residue);s->wr<4>(20,residue);compare(*s);}
    {auto s=seed(0);s->wr<4>(12,0x801);s->wr<4>(16,0x204);require(compare(*s).state->r<4>(12)==0x402,"Independent table-rank mean");}
    // Signed bytes, first successful primary threshold and central-field priority.
    for(unsigned field=15;field<=25;++field)for(auto value:{0u,9u,10u,14u,15u,16u,127u,128u,255u}){auto s=seed(1);for(unsigned k=15;k<=25;++k)s->memory.write<1>(profile+k,0);s->memory.write<1>(profile+field,value);compare(*s);}
    for(unsigned bits=0;bits<128;++bits){auto s=seed(1);for(unsigned k=0;k<7;++k)s->memory.write<1>(profile+15+k,(bits&(1u<<k))?15:14);compare(*s);}
    for(unsigned side=0;side<8;++side)for(unsigned level=9;level<=16;++level){auto s=seed(1);for(unsigned k=0;k<3;++k)s->memory.write<1>(profile+23+k,(side&(1u<<k))?level:0);compare(*s);}
    {auto s=seed(1);for(unsigned k=15;k<=21;++k)s->memory.write<1>(profile+k,10);for(unsigned k=23;k<=25;++k)s->memory.write<1>(profile+k,14);require(compare(*s).state->r<4>(12)==0x4f,"Primary threshold descends; secondary threshold stays fifteen");s->memory.write<1>(profile+23,15);s->memory.write<1>(profile+24,15);s->memory.write<1>(profile+25,15);require(compare(*s).state->r<4>(12)==0xacf,"Independent qualification priority and secondary bits");}
    {auto s=seed(1);s->wr<4>(12,0);boundaries(*s);}
    for(unsigned category=0;category<256;++category)for(auto amount:{0u,1u,0x7fffu,0x8000u,0xffffu}){auto s=seed(3);s->wr<4>(24,category);s->memory.write<2>(club+0x1ec,amount);compare(*s);}
    for(unsigned group=0;group<7;++group)for(auto amount:{0u,1u,0x7fffu,0x8000u,0xffffu}){auto s=seed(3);s->memory.write<2>(club+0x1ec,amount);if(group==6)s->memory.write<4>(club+0x2c,0);else s->memory.write<4>(country+0x68,100+group);compare(*s);}
    for(unsigned group=0;group<6;++group){auto s=seed(3);s->memory.write<4>(country+0x68,100+group);boundaries(*s);}
    for(unsigned category=0;category<256;++category){auto s=seed(2);s->wr<4>(24,category);compare(*s);}
    for(unsigned absent=0;absent<2;++absent){auto s=seed(2);if(absent)s->memory.write<4>(staff+0x14,0);else s->wr<4>(20,0);boundaries(*s);}
    for(auto mask:{0u,1u,0xffffu})for(auto result:{0u,12345u,0xffffffffu}){auto s=seed(2);Options o;o.children=false;o.mask=mask;o.reference=result;require(compare(*s,o).state->r<4>(12)==result,"Wrapper forwards the category reference without filtering mask");}
}
}
namespace cm::aot {void profile_qualification_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try{require(argc==1||argc==2,"Optional coverage output");corpus();const auto expected=profile_qualification_expected_sites();if(argc==2){std::ofstream out(argv[1]);for(auto pc:expected)out<<hex_address(pc)<<" "<<all_sites.contains(pc)<<" "<<complete_sites.contains(pc)<<'\n';}for(auto pc:expected)if(!complete_sites.contains(pc))std::cerr<<"Uncovered qualification site "<<hex_address(pc)<<std::endl;std::cout<<"Qualification comparisons="<<cases<<" returns="<<returns<<" stops="<<stops<<" negatives="<<negatives<<" completed_sites="<<complete_sites.size()<<"/"<<expected.size()<<std::endl;for(unsigned n=0;n<4;++n)std::cout<<"Root "<<hex_address(qualification::entries[n])<<" comparisons="<<root_cases[n]<<" returns="<<root_returns[n]<<" stops="<<root_stops[n]<<" negatives="<<root_negatives[n]<<std::endl;require(accepted_children[1]&&accepted_children[3],"Candidate reference must complete both selected native children");std::cout<<"Completed selected native children classification="<<accepted_children[1]<<" reference="<<accepted_children[3]<<std::endl;require(complete_sites.size()==expected.size(),"Every qualification instruction needs complete-return evidence");require(std::all_of(root_negatives.begin(),root_negatives.end(),[](auto n){return n==2;}),"Every qualification root needs result and restored-effect negatives");}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
