#include "money_value.hpp"
#include "compiler_arithmetic.hpp"
#include "register_context.hpp"
#include "semantic_io_checks.hpp"
#include <bit>
#include <set>
#include <fstream>
namespace cm::aot {bool money_value_reference(State&);std::vector<std::uint32_t> money_value_expected_sites();void money_value_fixture_tables(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace money=cm::aot::money_value;
constexpr std::uint32_t amount=fixture+0x1000,custom_table=fixture+0x3000,tables=fixture+0x10000,globals=fixture+0x20000;
constexpr std::array<std::uint32_t,7> ends{0x1000a008,0x1000a05c,0x1000a164,0x1000a26c,0x1000a32c,0x1000a440,0x1000cbb4};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{money::to_display_amount,money::from_display_amount,money::increase_amount,money::decrease_amount,money::select_standard_amount,money::select_table_amount,money::amount_quantum};
struct Options{unsigned interrupt=0;bool own_negative=false,effect_negative=false;};
struct Trace:SemanticGuestObserver {
    std::vector<std::uint32_t> sites;bool effect_negative=false,damaged=false;
    void before_step(State& s,std::uint32_t pc)override {sites.push_back(pc);if(effect_negative&&!damaged&&s.executed<s.budget){damaged=true;const auto v=s.memory.read<1>(globals+0x700);s.memory.write<1>(globals+0x700,v^1);s.memory.write<1>(globals+0x700,v);}}
};
thread_local Trace* original_trace=nullptr;
unsigned index(std::uint32_t root){const auto i=std::find(money::entries.begin(),money::entries.end(),root);require(i!=money::entries.end(),"Known monetary root");return unsigned(i-money::entries.begin());}
struct TimedEffects:MemoryObserver {
    State& state;Effects effects;std::vector<std::uint32_t> pcs;
    explicit TimedEffects(State& s):state(s){effects.state=&s;}
    void mapped(std::uint32_t a,std::uint64_t n)override{pcs.push_back(state.pc);effects.mapped(a,n);}
    void written(std::uint32_t a,const std::uint8_t* d,std::size_t n)override{pcs.push_back(state.pc);effects.written(a,d,n);}
    void cleared(std::uint32_t a,std::uint64_t n)override{pcs.push_back(state.pc);effects.cleared(a,n);}
    void read(std::uint32_t a,const std::uint8_t* d,unsigned n)override{pcs.push_back(state.pc);effects.read(a,d,n);}
};
struct Event{std::uint32_t target,stop;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
std::string text(State& s,std::uint32_t at){std::string out;for(unsigned n=0;n<1024;++n){const auto byte=s.memory.read<1>(at+n);if(!byte)return out;out.push_back(char(byte));}throw std::runtime_error("Unterminated monetary fixture string");}
void put(State& s,std::uint32_t at,const std::string& value){std::vector<std::uint8_t> bytes(value.begin(),value.end());bytes.push_back(0);s.memory.copy_in(at,bytes);}
std::uint64_t pair(const State& s){return (s.r<4>(12)<<32)|s.r<4>(16);}
void pair(State& s,std::uint64_t v){s.wr<4>(12,v>>32);s.wr<4>(16,std::uint32_t(v));}
struct Model:SemanticDependencyDispatch {
    bool original;Options options;Trace& trace;std::vector<Event> calls;std::array<unsigned,7> completed{};
    Model(bool old,Options o,Trace& t):original(old),options(o),trace(t){}
    void reference(State& s,std::uint32_t stop=return_sentinel){const auto n=index(s.pc),root=s.pc;while(s.pc!=stop){if(root<=s.pc&&s.pc<ends[n])require(money_value_reference(s),"Complete original monetary reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));}}
    void call(State& s,std::uint32_t target,std::uint32_t stop)override {
        const auto serial=calls.size();calls.push_back({target,stop,s.registers,{}});
        try {
            const auto entry=std::find(money::entries.begin(),money::entries.end(),target);
            if(entry!=money::entries.end()){
                const auto n=unsigned(entry-money::entries.begin());if(original)reference(s,stop);else methods[n](s,this,&trace);++completed[n];
            } else if(target==0x10773de0)compiler_arithmetic::double_to_signed_pair(s);
            else if(target==0x10773c80)compiler_arithmetic::signed_pair_to_double(s);
            else if(target==0x10773960)compiler_arithmetic::signed_quotient(s);
            else if(target==0x10773808)register_context::save_floating_suffix(s,28);
            else if(target==0x10773858)register_context::restore_floating_suffix(s,28);
            else {
                const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};std::uint32_t result=globals+0x900;
                // Diagnostic services are explicit external models. Arithmetic
                // and floating-register suffix bodies above are accepted code
                // reused on both sides; their original proofs are not redefined.
                s.memory.write<4>(globals+0x500,target);s.memory.write<4>(globals+0x504,s.memory.read<4>(globals+0x504)+1);
                if(target==0x10739890){(void)text(s,arg(3));put(s,arg(6),"X");put(s,arg(7),"Y");}
                else if(target==0x10768810)result=globals+0x920;
                else if(target==0x1077b720){(void)text(s,arg(4));(void)text(s,arg(5));(void)text(s,arg(6));(void)text(s,arg(7));put(s,arg(3),"DIAGNOSTIC");}
                else if(target==0x10769b60){(void)text(s,arg(3));(void)text(s,arg(4));}
                else throw std::runtime_error("Unknown monetary dependency "+hex_address(target));
                for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xdd000000+n*0x101);
                for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(1000.25+n));
                s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=stop;
            }
            require(s.pc==stop,"Monetary dependency return ABI");calls[serial].after=s.registers;if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Monetary fixture interruption");
        }catch(...){calls[serial].after=s.registers;throw;}
    }
};
std::unique_ptr<State> seed(unsigned n){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);s->memory.map(fixture,0x40000);s->memory.map(stack-0x1000,0x2000);s->memory.map(toc-0x8000,0x9000);s->pc=money::entries[n];s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->budget=100000;
    for(unsigned f=1280;f<=1311;++f)s->wr<1>(f,0);
    money_value_fixture_tables(*s,toc,tables);for(auto offset:{0x3aecu,0x3af4u,0x3afcu})s->memory.write<8>(toc-offset,float_bits<8>(1.0));
    s->memory.write<4>(amount,1000);s->memory.write<4>(toc-0x7ba8,globals+0x1000);s->memory.write<4>(toc-0x7bac,globals+0x400);s->memory.write<1>(globals+0x400,255);put(*s,globals+0x920,"TEST");
    for(unsigned k=0;k<128;++k)s->memory.write<4>(custom_table+k*4,k<7?s->memory.read<4>(tables+0x1000+k*4):k*100);
    s->wr<4>(12,amount);s->wr<4>(16,n==4||n==5?3:8);s->wr<4>(20,custom_table);s->wr<4>(24,7);
    if(n==1){s->wr<4>(16,0);s->wr<4>(20,1000);}if(n==6){pair(*s,1000);s->wr<4>(20,8);}return s;
}
unsigned cases=0,returns=0,stops=0,negatives=0;std::array<unsigned,7> root_cases{},root_returns{},root_stops{},root_negatives{};std::set<std::uint32_t> complete_sites,all_sites;std::array<std::set<std::uint32_t>,7> root_complete_sites;
struct Result{std::unique_ptr<State> state;std::vector<Effects::Access> accesses;unsigned calls;std::string outcome;std::vector<Event> dependencies;};
std::array<std::uint64_t,7> child_returns{};
Result compare(const State& input,Options options={}){
    auto a=clone(input),b=clone(input);TimedEffects x(*a),y(*b);Trace ot,nt;nt.effect_negative=options.effect_negative;Model old(true,options,ot),modern(false,options,nt);a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto run=[&](State& s,Model& d,bool reference){try{if(reference){original_trace=&ot;d.reference(s);}else methods[index(s.pc)](s,&d,&nt);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;const auto label="Monetary root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Monetary fixture interruption"||expected.starts_with("Unmapped data access at "),label+" unexpected reference: "+expected);require(expected==actual,label+" original="+expected+" modern="+actual);
    const auto negative=options.own_negative||options.effect_negative;if(negative)require(expected.empty(),label+" negative needs a complete return");if(options.own_negative)b->wr<4>(12,b->r<4>(12)^1);if(options.effect_negative){require(nt.damaged,label+" effect negative must execute");Image(*a).compare(*b,label+" restored effect image");}
    bool rejected=false;try{require(old.calls==modern.calls,label+" dependency ABI/order");require(x.effects.bytes==y.effects.bytes&&x.pcs==y.pcs,label+" ordered timed effects");require(ot.sites==nt.sites,label+" source order");require(a->executed==b->executed,label+" instruction count");Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!negative)throw;rejected=true;}require(rejected==negative,label+" discriminating negative");
    const auto root=index(input.pc);++cases;++root_cases[root];returns+=expected.empty()&&!negative;root_returns[root]+=expected.empty()&&!negative;stops+=!expected.empty();root_stops[root]+=!expected.empty();negatives+=negative;root_negatives[root]+=negative;if(!negative){all_sites.insert(ot.sites.begin(),ot.sites.end());if(expected.empty()){complete_sites.insert(ot.sites.begin(),ot.sites.end());for(auto pc:ot.sites)if(money::entries[root]<=pc&&pc<ends[root])root_complete_sites[root].insert(pc);}}
    if(expected.empty()&&!negative)for(unsigned n=0;n<7;++n)child_returns[n]+=modern.completed[n];
    return{std::move(a),x.effects.log,unsigned(old.calls.size()),expected,std::move(old.calls)};
}
void boundaries(const State& input,Options options={}){
    const auto base=compare(input,options);require(base.outcome.empty()&&base.state->pc==return_sentinel,"Monetary boundary baseline return");const auto steps=base.state->executed-input.executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto s=clone(input);s->budget=s->executed+cut;compare(*s,options);}
    std::set<std::uint32_t> bytes;for(const auto& a:base.accesses)if(a.size){bytes.insert(a.address);bytes.insert(a.address+std::uint32_t(a.size)-1);}std::set<std::uint32_t> holes;if(!bytes.empty())for(unsigned n=0;n<5;++n){auto i=bytes.begin();std::advance(i,std::ptrdiff_t(n*(bytes.size()-1)/4));holes.insert(*i);}
    for(auto hole:holes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;s->budget=input.budget;s->executed=input.executed;for(auto [at,size]:input.memory.mapped_ranges())for(auto [address,n]:hole<at||std::uint64_t(hole)>=std::uint64_t(at)+size?std::vector<std::pair<std::uint32_t,std::uint64_t>>{{at,size}}:std::vector<std::pair<std::uint32_t,std::uint64_t>>{{at,hole-at},{hole+1,std::uint64_t(at)+size-hole-1}})if(n){s->memory.map(address,n);s->memory.copy_in(address,input.memory.copy_out(address,n));}require(compare(*s,options).outcome.starts_with("Unmapped data access at "),"Removed money byte must fault");}
    for(unsigned cut:std::set<unsigned>{1,base.calls/2,base.calls})if(cut&&cut<=base.calls){auto o=options;o.interrupt=cut;compare(input,o);}
}
std::int64_t expected_quantum(std::int64_t value,unsigned flags){
    const std::vector<std::pair<std::int64_t,std::int64_t>> coarse{{10000,1000},{30000,2000},{100000,5000},{250000,10000},{1000000,25000},{5000000,100000},{20000000,250000},{50000000,500000}};
    const std::vector<std::pair<std::int64_t,std::int64_t>> fine{{10,1},{100,5},{250,10},{1000,25},{5000,100},{20000,250},{50000,500},{100000,5000},{250000,10000},{1000000,25000},{5000000,100000},{20000000,250000},{50000000,500000}};
    if(!(flags&12))return 1000;for(auto [limit,step]:flags&4?coarse:fine)if(value<=limit)return step;return 1000000;
}
void corpus(bool probe){
    for(unsigned n=0;n<7;++n){auto s=seed(n);if(probe){compare(*s);continue;}boundaries(*s);Options o;o.own_negative=true;compare(*s,o);o.own_negative=false;o.effect_negative=true;compare(*s,o);}
    if(probe)return;
    {auto s=seed(1);auto out=compare(*s);require(out.dependencies.size()==1,"Inverse conversion uses the existing signed-pair service");std::uint32_t result=0;for(unsigned k=12;k<16;++k)result=(result<<8)|out.dependencies.back().after[k];require(out.state->r<4>(12)==result&&result!=amount&&out.state->memory.read<4>(amount)==1000,"Inverse conversion retains helper return residue and separately stores the amount");}
    const std::array<std::int64_t,13> limits{10,100,250,1000,5000,10000,20000,30000,50000,100000,250000,1000000,50000000};
    std::set<std::int64_t> values{-1000000000,-1001,-1,0,1,5000000,19999999,20000000,20000001,50000001,1000000000};
    for(auto v:limits)for(auto delta:{-1,0,1})values.insert(v+delta);
    for(unsigned flags=0;flags<256;++flags)for(auto value:values){auto s=seed(6);pair(*s,std::uint64_t(value));s->wr<4>(20,flags|0x12340000);auto out=compare(*s);require(pair(*out.state)==std::uint64_t(expected_quantum(value,flags)),"Independent step profile flags="+std::to_string(flags)+" value="+std::to_string(value)+" expected="+std::to_string(expected_quantum(value,flags))+" actual="+std::to_string(pair(*out.state)));}
    for(unsigned n:{0u,2u,3u})for(unsigned flags=0;flags<256;++flags)for(auto value:{-1000001,-1000,-1,0,999,1000,1001,999999,1000000,1000001}){auto s=seed(n);s->memory.write<4>(amount,std::uint32_t(value));s->wr<4>(16,flags|0x43210000);auto out=compare(*s);const auto step=expected_quantum(value,flags);const auto aligned=(flags&32)?std::int64_t(value):(value+step/2)/step*step;const auto expected=n==0?aligned:n==2?aligned+expected_quantum(aligned+1,flags):aligned-expected_quantum(aligned,flags);if(n==0)require(pair(*out.state)==std::uint64_t(expected)&&out.state->memory.read<4>(amount)==std::uint32_t(value),"Independent scaled alignment and source preservation");else require(out.state->memory.read<4>(amount)==std::uint32_t(expected)&&out.state->r<4>(12)==std::uint32_t(expected),"Independent next/previous value and stored result");}
    for(unsigned n:{0u,2u,3u}){auto s=seed(n);s->memory.write<4>(amount,50000);const auto out=compare(*s);const auto observed=n==0?pair(*out.state):out.state->memory.read<4>(amount);require(observed==(n==0?50000u:n==2?55000u:49500u),"Verified fine-profile transition:50000 aligns unchanged, increases by5000 and decreases by500");}
    for(unsigned n:{0u,1u,2u,3u,6u})for(auto scale:{0.125,1.0,1.49,1000.0,10000000000.0,-1.0,0.0})for(auto word:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto flags:{0u,4u,8u,12u,32u,36u,40u}){auto s=seed(n);s->memory.write<8>(toc-0x3aec,float_bits<8>(scale));s->memory.write<8>(toc-0x3afc,float_bits<8>(scale));if(n==1){s->wr<4>(16,word);s->wr<4>(20,0xffffffff);}else if(n==6){pair(*s,(std::uint64_t(word)<<32)|0xffffffffu);s->wr<4>(20,flags);}else{s->memory.write<4>(amount,word);s->wr<4>(16,flags);}compare(*s);}
    for(unsigned n:{4u,5u})for(unsigned byte=0;byte<256;++byte){auto s=seed(n);s->wr<4>(16,byte|0x12340000);auto out=compare(*s);const auto valid=byte<7;if(valid){require(out.state->memory.read<4>(amount)==s->memory.read<4>((n==4?tables+0x1000:custom_table)+byte*4),"Independent selected preset at unit scales");}else require(out.state->memory.read<4>(amount)==1000&&out.state->r<4>(12)==1000&&!out.state->memory.read<1>(globals+0x400),"Invalid preset preserves value, clears diagnostic flag and returns stored word");}
    for(auto length:{0u,1u,7u,127u,128u,255u,256u,32767u,32768u,65535u,0x10007u})for(auto byte:{0u,1u,6u,7u,126u,127u,128u,255u}){auto s=seed(5);s->wr<4>(24,length);s->wr<4>(16,byte);compare(*s);}
    for(unsigned n:{4u,5u})for(auto numerator:{0.5,1.0,2.0,1.49,-2.0,0.0})for(auto denominator:{0.5,1.0,2.0,0.0}){auto s=seed(n);s->memory.write<8>(toc-0x3af4,float_bits<8>(numerator));s->memory.write<8>(toc-0x3aec,float_bits<8>(denominator));compare(*s);}
    for(auto flags:{0u,4u,8u,12u})for(auto value:{0ll,10000ll,10001ll,50000001ll}){auto s=seed(6);pair(*s,std::uint64_t(value));s->wr<4>(20,flags);boundaries(*s);}
    for(unsigned n:{4u,5u}){auto s=seed(n);s->wr<4>(16,255);boundaries(*s);}
    {auto s=seed(5);s->wr<4>(20,0);boundaries(*s);require(compare(*s).state->memory.read<4>(amount)==1000,"Null custom table takes diagnostic path");}
    for(unsigned n=0;n<6;++n){auto s=seed(n);s->wr<4>(12,0);require(compare(*s).outcome.starts_with("Unmapped data access at "),"Mandatory amount pointer null fault retained");}
    for(unsigned n:{0u,2u,3u}){auto s=seed(n);s->wr<4>(12,stack-(n==0?0x50:0x60)+0x38);compare(*s);}
    for(auto pointer:{custom_table,custom_table+12,globals+0x400,globals+0x1000})for(auto byte:{3u,255u}){auto s=seed(5);s->wr<4>(12,pointer);s->wr<4>(16,byte);compare(*s);}
    for(unsigned n=0;n<7;++n){auto s=seed(n);s->wr<4>(8,0);compare(*s);}
    for(unsigned n:{0u,2u,3u}){auto s=seed(n);s->wr<4>(16,32);boundaries(*s);}
    {auto s=seed(6);s->wr<4>(20,0);s->memory.write<4>(toc-0x59d4,0);require(pair(*compare(*s).state)==1000,"Default quantum does not dereference unused constant bank");}
    require(child_returns[5]&&child_returns[6],"Complete modern table-selection and quantum children required");
}
}
namespace cm::aot {void money_value_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try{require(argc==1||argc==2,"Optional coverage output or probe");const bool probe=argc==2&&std::string_view(argv[1])=="--probe";corpus(probe);const auto expected=money_value_expected_sites();if(argc==2&&!probe){std::ofstream out(argv[1]);for(auto pc:expected)out<<hex_address(pc)<<" "<<all_sites.contains(pc)<<" "<<complete_sites.contains(pc)<<'\n';}if(!probe)for(auto pc:expected)if(!complete_sites.contains(pc))std::cerr<<"Uncovered monetary site "<<hex_address(pc)<<std::endl;std::cout<<"Monetary comparisons="<<cases<<" returns="<<returns<<" stops="<<stops<<" negatives="<<negatives<<" completed_sites="<<complete_sites.size()<<"/"<<expected.size()<<std::endl;for(unsigned n=0;n<7;++n)std::cout<<"Root "<<hex_address(money::entries[n])<<" comparisons="<<root_cases[n]<<" returns="<<root_returns[n]<<" stops="<<root_stops[n]<<" negatives="<<root_negatives[n]<<" completed_sites="<<root_complete_sites[n].size()<<std::endl;if(!probe){require(complete_sites.size()==expected.size(),"Every monetary instruction needs complete-return evidence");for(unsigned n=0;n<7;++n)require(root_complete_sites[n].size()==(ends[n]-money::entries[n])/4,"Every entry needs its own complete-return source coverage");require(std::all_of(root_negatives.begin(),root_negatives.end(),[](auto n){return n==2;}),"Every monetary root needs result and restored-effect negatives");std::cout<<"Completed modern table children="<<child_returns[5]<<" quantum children="<<child_returns[6]<<std::endl;}}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
