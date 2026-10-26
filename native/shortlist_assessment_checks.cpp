#include "shortlist_assessment.hpp"
#include "semantic_io_checks.hpp"
#include "shortlist_rewrite_fixture.hpp"
#include "register_context.hpp"
#include "compiler_arithmetic.hpp"
#include <bit>
#include <set>
#include <map>
#include <fstream>
namespace cm::aot {std::vector<std::uint32_t> shortlist_assessment_expected_sites();bool shortlist_assessment_reference(State&);void shortlist_assessment_fixture_tables(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace rf=cm::shortlist_rewrite_fixture;
namespace assessment=cm::aot::shortlist_assessment;
constexpr std::uint32_t object=fixture+0x1000,clubs=fixture+0x10000,staff=fixture+0x20000,profiles=fixture+0x30000,countries=fixture+0x40000,related=fixture+0x50000,contracts=fixture+0x60000,globals=fixture+0x70000,metadata=fixture+0x74000,random=fixture+0x78000,tables=fixture+0x80000,limits=fixture+0x90000,row=fixture+0x1000+0x200,output=fixture+0x2000,flags=fixture+0x2100;
constexpr std::array<std::uint32_t,2> ends{0x10362d6c,0x1036d238};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{assessment::assess_candidate_contribution,assessment::evaluate_club_interest};
struct Options {
    std::map<std::uint32_t,std::uint32_t> words;
    std::map<std::uint32_t,double> reals;
    std::map<std::uint32_t,std::vector<std::pair<std::uint32_t,std::uint32_t>>> writes;
    unsigned interrupt=0;
    bool own_negative=false,child_negative=false,primary=false,secondary=false,competition=false,pointer_probe=false,vary_skills=false,qualification_by_country=false,floating_stop=false;
    std::uint32_t qualification=1,draw=0,renewal=65535,classification=1,estimate=1000,skill=10,group=1;
};
struct Trace:SemanticGuestObserver {
    std::vector<std::uint32_t> sites;std::map<std::uint32_t,decltype(State::registers)> gates;
    void before_step(State& state,std::uint32_t pc)override{sites.push_back(pc);if(pc==0x1036cf70||pc==0x1036c994||pc==0x1036caac||pc==0x1036c8d4)gates[pc]=state.registers;}
};
thread_local Trace* original_trace=nullptr;
unsigned index(std::uint32_t root){const auto i=std::find(assessment::entries.begin(),assessment::entries.end(),root);require(i!=assessment::entries.end(),"Known assessment root "+hex_address(root));return unsigned(i-assessment::entries.begin());}
void modern_root(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){methods[index(s.pc)](s,d,o);}
struct Event{std::uint32_t target,stop;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Model:SemanticDependencyDispatch {
    bool original;Options options;Trace& trace;std::vector<Event> calls;rf::Side side;
    Model(bool old,Options o,Trace& t):original(old),options(std::move(o)),trace(t){}
    void reference(State& s,std::uint32_t stop=return_sentinel){const auto root=s.pc,n=index(root);bool first=true;while(s.pc!=stop){if(s.pc>=root&&s.pc<ends[n]&&(first||s.pc!=root))require(shortlist_assessment_reference(s),"Complete assessment reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));first=false;}}
    void call(State& s,std::uint32_t target,std::uint32_t stop)override {
        const auto serial=calls.size();calls.push_back({target,stop,s.registers,{}});const bool counts=side.enter(target,stop);
        try {
            const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};const auto first=arg(3),second=arg(4);std::uint32_t result=0;double real=0.5;bool special=false;
            if(side.composes(target,stop)){
                if(std::find(assessment::entries.begin(),assessment::entries.end(),target)!=assessment::entries.end())reference(s,stop);else rf::compose(s,target,rf::answer_of(*this));
                require(s.pc==stop,"Composed assessment child return");calls[serial].after=s.registers;
                if(side.interrupts(counts,options.interrupt,calls.size()))throw std::runtime_error("Fixture assessment dependency interruption");
                return;
            }
            if(side.records(target,stop)){s.memory.write<4>(globals+0x500,target);s.memory.write<4>(globals+0x508,s.memory.read<4>(globals+0x508)+1);}
            if(target==0x10773808||target==0x107737fc){register_context::save_floating_suffix(s,target==0x10773808?28:25);special=true;}
            else if(target==0x10773858||target==0x1077384c){register_context::restore_floating_suffix(s,target==0x10773858?28:25);special=true;}
            else if(target==0x10773780){compiler_arithmetic::double_to_unsigned_word(s);special=true;}
            else if(target==0x10082f20){const auto id=std::uint32_t(s.memory.read<4>(second));require(id<32,"Bounded assessment contract staff");result=options.primary?contracts+id*0x100:0;}
            else if(target==0x10082f90)result=options.secondary?contracts+0x3000:0;
            else if(target==0x1077b770)result=options.renewal;
            else if(target==0x10356e40)result=options.draw;
            else if(target==0x100cdef0)result=10000;
            else if(target==0x100c96e0)result=std::uint32_t(s.memory.read<4>(first+0x2c));
            else if(target==0x10216830)result=options.estimate;
            else if(target==0x100d2f50||target==0x10219e40)result=options.skill+(options.vary_skills&&target==0x100d2f50?(first-related)/0x44:0);
            else if(target==0x10359980)result=options.qualification;
            else if(target==0x100ce0a0||target==0x100cab10)result=options.classification;
            else if(target==0x1007c140)result=options.group;
            else if(target==0x10103510)result=related+0x4000;
            else if(target==0x10459b50)result=options.competition?related+0x6000:0;
            else if(target==0x10715880){s.memory.clear(first,8);s.memory.write<2>(first,100);s.memory.write<2>(first+2,2001);result=first;}
            else if(target==0x10718130){s.memory.write<2>(first,second);s.memory.write<2>(first+2,arg(5));result=first;}
            else if(target==0x10717b20){const auto day=std::int16_t(s.memory.read<2>(second)),year=std::int16_t(s.memory.read<2>(second+2));const auto ordinal=std::int64_t(year)*365+day-arg(5);const auto y=std::int64_t(std::floor(double(ordinal)/365));s.memory.write<2>(first,ordinal-y*365);s.memory.write<2>(first+2,y);s.memory.write<1>(first+4,3);s.memory.write<1>(first+5,0x6d);s.memory.write<2>(first+6,0x1234);result=first;}
            else if(target==0x103721f0){s.memory.clear(first,0x40);result=first;}
            else if(target==0x101929b0){for(auto at:{second,arg(5)}){s.memory.write<2>(at,200);s.memory.write<2>(at+2,2001);}result=0;}
            else if(target==0x10780fc0||target==0x10780fd8){s.memory.write<4>(arg(1)+0x14,arg(2));s.wr<4>(8,0x18000000);}
            else if(target==0x1014c2e0||target==0x100d3230||target==0x1030fbf0||target==0x1014c5f0||target==0x100d3150||target==0x103f43b0||target==0x100da050||target==0x100da0b0||target==0x10464160||target==0x100d9520||target==0x100d9580||target==0x100f55b0||target==0x103092f0||target==0x100d3040||target==0x1007e820||target==0x100f2680||target==0x1037b6f0)result=0;
            else throw std::runtime_error("Unknown assessment dependency "+hex_address(target));
            if(!special){
                if(options.words.contains(target))result=options.words.at(target);if(target==0x100cab10&&options.qualification_by_country)result=second==countries+0x300;if(options.reals.contains(target))real=options.reals.at(target);
                for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2&&side.clobbers(target,n))s.wr<4>(n*4,0xdd000000+n*0x101);
                if(side.clobbers_fprs(target))for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(1000.25+n));
                if(side.writes_f1(target))s.wr<8>(0x4010,float_bits<8>(real));s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);
            }
            if(options.writes.contains(target))for(auto [address,value]:options.writes.at(target))s.memory.write<4>(address,value);
            s.pc=stop;if(options.child_negative&&!original)s.memory.write<1>(globals+0x700,1);calls[serial].after=s.registers;
            if(side.interrupts(counts,options.interrupt,calls.size()))throw std::runtime_error("Fixture assessment dependency interruption");
        }catch(...){calls[serial].after=s.registers;throw;}
    }
};
std::unique_ptr<State> seed(unsigned n){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned offset=1280;offset<1312;++offset)s->wr<1>(offset,0);s->memory.map(fixture,0x100000);s->memory.map(stack-0x6000,0x7000);s->memory.map(toc-0x8000,0x9000);s->pc=assessment::entries[n];s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->budget=1000000;s->wr<4>(12,object);s->wr<4>(16,n==0?clubs+0x248:staff+0x70);s->wr<4>(20,n==0?staff+0x70:clubs+0x248);s->wr<4>(24,n==0?row:1);s->wr<4>(28,n==0?output:0);s->wr<4>(32,n==0?1000:0);s->wr<4>(36,n==0?1:flags);
    const std::array slots{-0x7be8,-0x7bbc,-0x7b9c,-0x7b90,-0x7b88,-0x7b84,-0x7b7c,-0x7b78,-0x7b5c,-0x7b40,-0x7b3c,-0x7b1c,-0x7af4,-0x7ab8,-0x7a70,-0x7a30,-0x7a2c,-0x7a14,-0x7a0c,-0x7a08,-0x7a04,-0x7a00,-0x79fc,-0x79f8,-0x79f4,-0x79f0,-0x7984,-0x7590,-0x74ec,-0x7488};
    unsigned slot_index=0;for(auto slot:slots){const auto at=globals+0x100+slot_index++*4;s->memory.write<4>(toc+slot,at);s->memory.write<4>(at,0);}
    const auto global=[&](std::int32_t slot,std::uint32_t value){s->memory.write<4>(s->memory.read<4>(toc+slot),value);};
    global(-0x7be8,clubs);global(-0x7bbc,related+0xe000);global(-0x7b9c,metadata);global(-0x7b90,0);global(-0x7b88,random+0x20);global(-0x7b84,random);global(-0x7b7c,object+0x400);global(-0x7b5c,related+0x9000);global(-0x7b40,object+0x800);global(-0x7b3c,object+0x900);global(-0x7b1c,object+0xa00);global(-0x7ab8,object+0xb00);global(-0x79f8,related);global(-0x79f4,16);global(-0x7984,related+0xa000);global(-0x7a70,related+0xb000);global(-0x79fc,29);global(-0x7a00,28);global(-0x7a14,27);global(-0x7af4,26);global(-0x7a08,25);global(-0x7590,24);
    s->memory.write<4>(toc-0x7b8c,random);s->memory.write<4>(toc-0x7b80,globals+0x80);s->memory.write<2>(globals+0x80,100);s->memory.write<2>(globals+0x82,2001);s->memory.write<4>(toc-0x3b7c,limits);
    for(auto [offset,value]:{std::pair{0u,0.0},std::pair{8u,4294967296.0},std::pair{16u,2147483648.0}})s->memory.write<8>(limits+offset,float_bits<8>(value));
    for(unsigned k=0;k<32;++k){const auto person=staff+k*0x70,club=clubs+k*0x248,nation=countries+k*0x300,profile=profiles+k*0x100,record=contracts+k*0x100;
        s->memory.write<4>(person,k);s->memory.write<4>(person+8,10000);s->memory.write<4>(person+12,countries);s->memory.write<4>(person+0x14,profile);s->memory.write<4>(person+0x1c,related+k*0x44);s->memory.write<4>(person+0x24,clubs);s->memory.write<2>(person+0x5c,25);s->memory.write<1>(person+0x62,11);for(unsigned off=0x63;off<=0x69;++off)s->memory.write<1>(person+off,10);
        s->memory.write<2>(profile+10,8000);for(unsigned off=0x14;off<0x60;++off)s->memory.write<1>(profile+off,10);
        s->memory.write<4>(club,k);s->memory.write<4>(club+0x14,0xffffffff);s->memory.write<4>(related+0xe000+k*0x6c,k);s->memory.write<2>(related+0xe000+k*0x6c+0x18,20);s->memory.write<4>(club+0x2c,nation);s->memory.write<4>(club+0x90,staff+5*0x70);s->memory.write<4>(club+0x98,related+0x800);s->memory.write<2>(club+0x1ec,7500);s->memory.write<4>(nation+0x68,k);s->memory.write<4>(nation+0x98,related+0x1000);s->memory.write<2>(nation+0x18,20);s->memory.write<1>(nation+0x11f,10);
        s->memory.write<4>(record,1);s->memory.write<4>(record+4,0);s->memory.write<2>(record+0x2a,2000);s->memory.write<4>(related+0x8000+k*4,nation);s->memory.write<4>(related+0x9000+k*4,related+0xc000+k*0x100);s->memory.write<4>(related+0xa000+k*8,0xffffffff);s->memory.write<2>(random+k*2,10+k*13);
    }
    s->memory.write<1>(related+0x4000+0x165,1);
    s->memory.write<4>(object+0xdc,fixture+0x68000);for(unsigned k=0;k<32;++k){s->memory.write<4>(fixture+0x68000+k*12,row);s->memory.write<1>(fixture+0x68000+k*12+4,1);s->memory.write<2>(fixture+0x68000+k*12+6,100);}
    s->memory.write<4>(row,11);s->memory.write<2>(row+2,1);s->memory.write<4>(row+4,100);s->memory.write<4>(row+8,10000);s->memory.write<4>(row+0x10,0xffffffff);s->memory.write<4>(row+0x14,0xffffffff);s->memory.write<4>(row+0x18,0xffffffff);s->memory.write<4>(output,1000);
    shortlist_assessment_fixture_tables(*s,toc,tables);return s;
}
unsigned cases=0,completed=0,interrupted=0,negatives=0;
std::array<unsigned,2> root_cases{},root_returns{},root_stops{},root_negatives{};std::set<std::uint32_t> sites,completed_sites;std::map<std::string,unsigned> partial_record_faults;
std::map<std::uint32_t,std::pair<std::unique_ptr<State>,Options>> gate_inputs,first_gate_inputs;
std::map<std::uint32_t,decltype(State::registers)> gate_registers;
struct Result{std::unique_ptr<State> state;std::vector<Effects::Access> accesses;std::vector<Event> calls;std::string outcome;};
// The same case for the C++ SHORTLIST_MANAGER.CPP (shortlist_rewrite_fixture.hpp). The
// compiler's register save/restore and conversion helpers are inlined there.
const std::set<std::uint32_t> transparent{0x10773808,0x107737fc,0x10773858,0x1077384c,0x10773780};
void rewrite_side(const State& input,const Options& options,const std::string& expected,const std::string& label){
    const char* not_run=nullptr;
    static Trace unused;
    rf::rewrite_side(input,expected,not_run,{label,&transparent,rf::negative(options.own_negative,options.child_negative),{{tables,limits}},{}},
        [&](bool reference){return std::make_unique<Model>(reference,options,unused);},[](Model& m,State& s){m.reference(s);});
}
Result compare(const State& input,Options options={}){
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;Trace ot,nt;Model old(true,options,ot),modern(false,options,nt);
    const auto run=[&](State& state,Model& model,bool reference){try{if(reference){original_trace=&ot;model.reference(state);}else modern_root(state,&model,&nt);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;const auto label="Shortlist assessment root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Fixture assessment dependency interruption"||expected.starts_with("Unmapped data access at ")||(options.floating_stop&&expected=="Original enabled floating inexact exception requires a handler"),label+" fixture assertion: "+expected);
    require(expected==actual,label+" original="+expected+" modern="+actual);
    if(ot.sites!=nt.sites){for(unsigned k=0;k<std::min(ot.sites.size(),nt.sites.size());++k)if(ot.sites[k]!=nt.sites[k]){std::cerr<<"First assessment site "<<k<<" "<<hex_address(ot.sites[k])<<"/"<<hex_address(nt.sites[k])<<std::endl;break;}}
    if(options.own_negative){if(input.pc==assessment::entries[0])b->wr<4>(12,b->r<4>(12)^1);else b->wr<8>(0x4010,b->r<8>(0x4010)^1);}
    bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative&&!options.child_negative)throw;rejected=true;}require(rejected==(options.own_negative||options.child_negative),label+" discriminating negative");
    if(!rejected){require(x.bytes==y.bytes,label+" ordered effects");require(old.calls==modern.calls,label+" dependency ABI");require(ot.sites==nt.sites,label+" source order");require(a->executed==b->executed,label+" instruction accounting");}
    if(expected.starts_with("Unmapped data access at ")&&!options.pointer_probe){
        const std::map<std::uint32_t,std::string> declared{{0x10369828,"0x00000054"},{0x10369f54,"0x00000065"},{0x1036b430,"0x000001ec"},{0x1036b668,"0x00000050"},{0x1036b754,"0x0000004e"},{0x1036c520,"0x000001ec"}};
        require(declared.contains(a->pc)&&expected=="Unmapped data access at "+declared.at(a->pc),label+" undeclared incomplete fixture: "+expected);
        ++partial_record_faults[hex_address(a->pc)+":"+expected];
    }
    if(expected.empty()&&!rejected)for(const auto& [gate,registers]:ot.gates){
        if(!first_gate_inputs.contains(gate))first_gate_inputs.emplace(gate,std::pair{clone(input),options});
        bool replace=!gate_inputs.contains(gate);if(!replace&&gate!=0x1036cf70){State now,before;now.registers=registers;before.registers=gate_registers.at(gate);const auto metric=[&](State& state){const auto score=float_value<8>(state.r<8>(0x41f0));if(gate==0x1036c8d4){const auto elapsed=float_value<8>(state.r<8>(0x41b0)),difference=float_value<8>(state.r<8>(0x4030));return elapsed+(1-elapsed)*std::min(1.0,difference/2000.0);}if(gate==0x1036caac){const auto group=std::int8_t(state.r<4>(26*4));return group>=1&&group<=3&&input.memory.read<1>(contracts+0x3000+0x54)==0?score:-1.0;}return score;};const auto x=metric(now),y=metric(before);replace=std::isfinite(x)&&(gate==0x1036c8d4?x<y:x>y);}
        if(replace){gate_inputs.insert_or_assign(gate,std::pair{clone(input),options});gate_registers[gate]=registers;}
    }
    rewrite_side(input,options,expected,label);
    const auto root=index(input.pc);++root_cases[root];root_returns[root]+=expected.empty()&&!rejected;root_stops[root]+=!expected.empty();root_negatives[root]+=rejected;
    ++cases;completed+=expected.empty()&&!rejected;interrupted+=!expected.empty();negatives+=rejected;if(!rejected&&expected!="Native instruction budget exhausted")sites.insert(ot.sites.begin(),ot.sites.end());if(!rejected&&expected.empty())completed_sites.insert(ot.sites.begin(),ot.sites.end());return{std::move(a),x.log,old.calls,expected};
}
void boundaries(const State& input,Options options={}){
    auto base=compare(input,options);require(base.state->pc==return_sentinel,"Completed assessment boundary fixture root="+hex_address(input.pc)+" stop="+hex_address(base.state->pc)+" outcome="+base.outcome);const auto steps=base.state->executed-input.executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto state=clone(input);state->budget=state->executed+cut;compare(*state,options);}
    std::set<std::uint32_t> bytes;for(auto a:base.accesses)if(a.size){bytes.insert(a.address);bytes.insert(a.address+std::uint32_t(a.size)-1);}std::set<std::uint32_t> holes;if(!bytes.empty())for(unsigned n=0;n<6;++n){auto i=bytes.begin();std::advance(i,std::ptrdiff_t(n*(bytes.size()-1)/5));holes.insert(*i);}
    for(auto hole:holes){auto state=std::make_unique<State>();state->registers=input.registers;state->unique=input.unique;state->pc=input.pc;state->budget=input.budget;state->executed=input.executed;for(auto [start,size]:input.memory.mapped_ranges()){if(hole<start||std::uint64_t(hole)>=std::uint64_t(start)+size){state->memory.map(start,size);state->memory.copy_in(start,input.memory.copy_out(start,size));continue;}for(auto [a,n]:{std::pair{start,std::uint64_t(hole-start)},std::pair{hole+1,std::uint64_t(start)+size-hole-1}})if(n){state->memory.map(a,n);state->memory.copy_in(a,input.memory.copy_out(a,n));}}auto probe=options;probe.pointer_probe=true;compare(*state,probe);}
    for(unsigned n:std::set<unsigned>{1,2,unsigned(base.calls.size()/2),unsigned(base.calls.size())})if(n&&n<=base.calls.size()){auto o=options;o.interrupt=n;compare(input,o);}
}

void contribution_cases(){
    for(unsigned kind=0;kind<16;++kind)for(unsigned qualification=0;qualification<24;++qualification)for(auto mode:{0u,1u}){
        auto state=seed(0);state->memory.write<1>(row+0x28,kind*8);state->memory.write<2>(staff+0x70+0x5c,16+(kind+qualification)%25);state->wr<4>(36,mode);Options o;o.qualification=qualification;compare(*state,o);
    }
    for(auto field:{0u,1u,15u,16u,17u,20u,24u,25u,28u,31u,32u,35u,36u,39u,40u,41u,127u,128u,255u,256u,32767u,32768u,65535u})for(auto kind:{0u,1u,2u,3u,4u,5u,6u,15u}){
        auto state=seed(0);state->memory.write<2>(staff+0x70+0x5c,field);state->memory.write<1>(row+0x28,kind*8);compare(*state);
    }
    for(auto estimate:{0u,1u,1000u,10000u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto reference:{0u,1u,9999u,10000u,15000u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto marker:{0u,128u}){
        auto state=seed(0);state->wr<4>(32,reference);state->memory.write<1>(row+0x28,marker);Options o;o.estimate=estimate;o.words[0x100d3230]=1;compare(*state,o);
    }
    for(auto kind:{2u,3u,4u})for(auto classification:{0u,1u,2u,3u,4u,255u,256u})for(auto status:{0u,1u})for(auto qualification:{0u,1u}){
        auto state=seed(0);state->memory.write<1>(row+0x28,kind*8);for(unsigned k=0;k<7;++k)state->memory.write<4>(clubs+0x248+0x174+k*4,staff+(k+5)*0x70);Options o;o.classification=classification;o.words[0x1014c2e0]=status;o.words[0x1030fbf0]=qualification;o.words[0x100d3230]=status;o.words[0x100d3150]=qualification;boundaries(*state,o);
    }
    for(auto count:{0u,1u,16u,65535u,65536u,0xffff0000u,0xffffffffu})for(auto end:{random,random+0x20}){
        auto state=seed(0);state->memory.write<4>(staff+5*0x70+0x1c,0);state->memory.write<4>(state->memory.read<4>(toc-0x79f4),count);state->memory.write<4>(state->memory.read<4>(toc-0x7b88),end);Options o;o.draw=0;compare(*state,o);
    }
    for(auto country:{0u,28u,29u})for(auto cap:{0u,1u,100u,10000u,0x7fffffffu})for(auto mode:{0u,1u,128u,255u,256u}){
        auto state=seed(0);state->memory.write<4>(countries+0x300+0x68,country);state->memory.write<4>(row+8,cap);state->wr<4>(36,mode);compare(*state);
    }
    for(auto scale:{0.0,0.1,0.5,1.0,100.0}){auto state=seed(0);state->memory.write<1>(metadata+100+12,0x30);Options o;o.reals[0x10780fc0]=scale;boundaries(*state,o);}
    for(auto alias:{row+4,row+8,staff+0x70+8,flags}){auto state=seed(0);state->wr<4>(28,alias);compare(*state);}
}
void interest_cases(){
    for(unsigned category=0;category<9;++category)for(unsigned group=0;group<9;++group)for(auto mode:{0u,1u})for(bool primary:{false,true}){
        auto state=seed(1);state->wr<4>(32,category);state->wr<4>(24,mode);Options o;o.primary=primary;o.group=group;compare(*state,o);
    }
    const std::array values{0u,999u,1000u,3499u,3500u,5499u,5500u,6999u,7000u,7249u,7250u,7999u,8000u,9999u,10000u,15000u,32767u,32768u,65535u};
    for(auto ability:values)for(auto standing:values){auto state=seed(1);state->memory.write<2>(profiles+0x100+10,ability);state->memory.write<2>(clubs+0x248+0x1ec,standing);compare(*state);}
    for(unsigned off=0x63;off<=0x69;++off)for(auto value:{0u,1u,5u,10u,15u,20u,21u,127u,128u,255u})for(auto mode:{0u,1u}){
        auto state=seed(1);state->memory.write<1>(staff+0x70+off,value);state->wr<4>(24,mode);compare(*state);
    }
    for(auto field:{0u,1u,17u,18u,19u,20u,23u,24u,25u,28u,31u,32u,34u,35u,40u,60u,127u,128u,255u,32767u})for(auto category:{0u,1u,2u,3u,4u,5u,6u,7u}){
        auto state=seed(1);state->memory.write<2>(staff+0x70+0x5c,field);state->wr<4>(32,category);compare(*state);
    }
    for(bool primary:{false,true})for(bool secondary:{false,true})for(auto status:{0u,1u,2u,3u,7u,11u,255u})for(auto bits:{0u,0x10u,0x20u,0x200u}){
        auto state=seed(1);state->memory.write<1>(contracts+0x100+0x60,status);state->memory.write<4>(contracts+0x100+0x50,bits);Options o;o.primary=primary;o.secondary=secondary;compare(*state,o);
    }
    for(auto target:{0x100d3230u,0x100d3150u,0x100cab10u,0x100da050u,0x100da0b0u,0x10464160u,0x100d9520u,0x100d9580u,0x100f55b0u,0x103092f0u,0x100d3040u,0x1007e820u,0x101929b0u,0x103f43b0u,0x1014c2e0u})for(auto result:{0u,1u,2u,3u,4u,255u,256u}){
        auto state=seed(1);Options o;o.words[target]=result;compare(*state,o);
    }
    for(auto value:{0u,1u,11u,0xffffffffu})for(auto reason:{0u,flags}){auto state=seed(1);state->wr<4>(28,value);state->wr<4>(36,reason);compare(*state);}
    for(auto finance:{0u,1u,10u,100u,10000u})for(auto quote:{0.0,0.1,1.0,10.0,100.0,10000.0}){
        auto state=seed(1);Options o;o.words[0x100f2680]=finance;o.reals[0x1037b6f0]=quote;o.reals[0x10780fd8]=quote;compare(*state,o);
    }
    for(unsigned n:{0u,1u}){auto state=seed(n);constexpr std::uint32_t moved=0x18008000;state->memory.map(moved-0x8000,0x9000);state->memory.copy_in(moved-0x8000,state->memory.copy_out(toc-0x8000,0x9000));state->wr<4>(8,moved);shortlist_assessment_fixture_tables(*state,moved,fixture+0xa0000);compare(*state);}
}

void targeted_contribution_cases(){
    for(auto estimate:{0u,1u,10000u,20000u,20001u,50000u,1000000u,0x7fffffffu})for(auto kind:{0u,1u,2u,3u,4u,5u,6u,7u})for(auto field:{16u,20u,26u,27u,28u,40u}){
        auto state=seed(0);state->memory.write<1>(row+0x28,kind*8);state->memory.write<2>(staff+0x70+0x5c,field);Options o;o.estimate=estimate;compare(*state,o);
    }
    for(auto estimate:{35000u,35001u,50000u,1000000u})for(auto prior:{0u,10000u,1000000u})for(auto mask:{1u,127u,128u,0xffffu})for(auto kind:{0u,1u,3u,6u}){
        auto state=seed(0);state->memory.write<4>(row+12,prior);state->memory.write<2>(row+2,mask);state->memory.write<1>(row+0x28,kind*8);Options o;o.estimate=estimate;compare(*state,o);
    }
    for(unsigned field=16;field<=40;++field)for(auto estimate:{0u,1u})for(auto qualification:{0u,14u}){auto state=seed(0);state->memory.write<2>(staff+0x70+0x5c,field);Options o;o.estimate=estimate;o.qualification=qualification;compare(*state,o);}
    {auto state=seed(0);state->memory.write<4>(staff+5*0x70+0x1c,0);Options o;o.estimate=0xffffffff;const auto result=compare(*state,o);require(result.state->r<4>(12)==0&&result.state->memory.read<4>(output)==0xffffffff,"Negative provided contribution preserves the written auxiliary result");require(result.state->memory.read<4>(staff+5*0x70+0x1c)==related+7*0x44,"Independent lazy reference selection precedes later rejection");require(result.state->memory.read<4>(result.state->memory.read<4>(toc-0x7b84))==random+2,"Independent shared random cursor prefetch");}
}

// Deterministic boundary combinations over mapped objects and declared child results.
// These are component fixtures; the child services are not production implementations.
void context_combinations(){
    std::uint32_t random_word=0x7c53e291;
    const auto pick=[&](unsigned count){random_word^=random_word<<13;random_word^=random_word>>17;random_word^=random_word<<5;return random_word%count;};
    const std::array fields{0u,1u,15u,16u,20u,24u,25u,26u,27u,28u,32u,35u,40u,127u,255u};
    const std::array values{0u,1u,999u,1000u,3499u,3500u,4999u,5000u,5499u,5500u,6999u,7000u,7249u,7250u,7999u,8000u,8999u,9000u,9999u,10000u,15000u,32767u};
    const std::array prices{0u,1u,1000u,10000u,20001u,35000u,50000u,100000u,1000000u,0x7fffffffu};
    const std::array country_ids{0u,1u,24u,25u,26u,27u,28u,29u};
    for(unsigned n=0;n<2;++n)for(unsigned sample=0;sample<6000;++sample){
        auto state=seed(n);Options o;o.primary=n&&pick(5);o.secondary=o.primary||bool(n&&pick(3));o.qualification_by_country=pick(2);o.vary_skills=pick(2);o.competition=pick(2);o.classification=pick(22);o.qualification=pick(24);o.group=pick(9);o.estimate=prices[pick(prices.size())];o.skill=values[pick(values.size())];o.draw=pick(16);
        state->memory.write<2>(staff+0x70+0x5c,fields[pick(fields.size())]);state->memory.write<4>(staff+0x70+8,prices[pick(prices.size())]);
        state->memory.write<2>(staff+0x70+0x4c,pick(365));state->memory.write<2>(staff+0x70+0x4e,2000+pick(3));state->memory.write<1>(staff+0x70+0x5e,pick(2));
        for(unsigned off=0x63;off<=0x69;++off)state->memory.write<1>(staff+0x70+off,fields[pick(fields.size())]);
        state->memory.write<2>(profiles+0x100+6,pick(250));state->memory.write<2>(profiles+0x100+8,values[pick(values.size())]);state->memory.write<2>(profiles+0x100+10,values[pick(values.size())]);state->memory.write<2>(profiles+0x100+12,values[pick(values.size())]);state->memory.write<1>(profiles+0x100+15,pick(2)?0:20);state->memory.write<1>(profiles+0x100+0x45,5+pick(16));
        for(unsigned k=0;k<2;++k){const auto club=clubs+k*0x248,nation=countries+k*0x300;
            state->memory.write<2>(club+0x1ec,values[pick(values.size())]);state->memory.write<4>(club+0x14,pick(2)?0xffffffff:pick(3));state->memory.write<4>(club+0x30,pick(2)?nation:0);state->memory.write<4>(club+8,pick(2)?40000:0);state->memory.write<1>(club+0x23d,pick(4));
            state->memory.write<4>(nation+0x68,country_ids[pick(country_ids.size())]);state->memory.write<2>(nation+0x18,1+pick(25));state->memory.write<4>(state->memory.read<4>(toc-0x7a0c),24);state->memory.write<1>(nation+0x119,pick(3));state->memory.write<1>(nation+0x11c,1+pick(3));state->memory.write<1>(nation+0x11f,fields[pick(fields.size())]);
        }

        // Club histories, geographic records, affiliated clubs and national contexts.
        state->memory.write<4>(state->memory.read<4>(toc-0x79f0),pick(2)?related+0x12000:0);state->memory.write<4>(related+0x12000,related+0x13000);
        for(unsigned k=0;k<2;++k){const auto club=clubs+k*0x248,location=related+0x16000+k*0x100,coordinates=location+0x40;
            state->memory.write<4>(club+0x3c,pick(4)?location:0);state->memory.write<4>(location+0x10,pick(4)?coordinates:0);state->memory.write<1>(coordinates+0x37,pick(16));state->memory.write<8>(coordinates,float_bits<8>(double(pick(100))));state->memory.write<8>(coordinates+8,float_bits<8>(double(pick(100))));
            if(pick(8)==0)state->memory.write<4>(club+0x90,0);else if(pick(8)==0)state->memory.write<4>(staff+5*0x70+0x1c,0);state->memory.write<2>(related+5*0x44+0x2a,values[pick(values.size())]);state->memory.write<2>(staff+5*0x70+0x4c,pick(365));state->memory.write<2>(staff+5*0x70+0x4e,2000+pick(3));state->memory.write<4>(club+0x74,pick(4)?0:clubs+(1-k)*0x248);state->memory.write<4>(club+0x78,pick(4)?0:clubs+(1-k)*0x248);state->memory.write<4>(club+0x7c,pick(4)?0:clubs+(1-k)*0x248);
            const auto history=related+0x13000+k*0x4c;for(unsigned off:{6u,8u,10u})state->memory.write<2>(history+off,values[pick(values.size())]);
            const auto context=related+0x18000+k*0x100;state->memory.write<4>(countries+k*0x300+0x74,pick(2)?context:0);state->memory.write<4>(context+8,pick(2)?related+0xb000:0);
        }
        if(pick(3)==0)state->memory.write<4>(clubs+0x248+0x2c,countries);
        state->memory.write<4>(staff+0x70+0x24,pick(2)?clubs:clubs+0x248);state->memory.write<4>(row+0x10,pick(2)?1:0xffffffff);
        state->memory.write<4>(contracts+0x3000+12,prices[pick(prices.size())]);state->memory.write<1>(contracts+0x3000+0x44,fields[pick(fields.size())]);
        state->memory.write<4>(state->memory.read<4>(toc-0x7a04),pick(2)?1:0);state->memory.write<4>(state->memory.read<4>(toc-0x7a08),pick(2)?1:0);

        state->memory.write<4>(contracts+0x100+4,pick(2));state->memory.write<1>(contracts+0x100+0x38,pick(8)<<2);
        const auto contract=contracts+0x3000;const std::array contract_bits{0u,1u,0x10u,0x20u,0x200u,0x100u,0x61a8u,0xffffffffu};state->memory.write<4>(contract+0x50,contract_bits[pick(contract_bits.size())]);state->memory.write<1>(contract+0x54,pick(2));
        for(unsigned off:{0x3cu,0x40u,0x44u,0x48u,0x4cu})state->memory.write<1>(contract+off,pick(256));state->memory.write<1>(contract+0x5c,pick(16));state->memory.write<1>(staff+5*0x70+0x65,pick(21));state->memory.write<1>(contract+0x60,pick(8));state->memory.write<1>(contract+0x44,fields[pick(fields.size())]);state->memory.write<2>(contract+0x30,pick(365));state->memory.write<2>(contract+0x32,2000+pick(3));
        state->memory.write<1>(related+0x6000+4,pick(2));state->memory.write<1>(related+0x6000+0x75,1+pick(16));
        state->memory.write<1>(related+0x4000+0x165,pick(3));state->memory.write<4>(related+0xa000+8,pick(2)?0xffffffff:pick(3));
        state->memory.write<2>(fixture+0x68000+12+6,values[pick(values.size())]);state->memory.write<2>(row+2,pick(2)?0x80:pick(128));state->memory.write<1>(row+0x28,pick(32)*8);state->memory.write<4>(row+4,prices[pick(prices.size())]);state->memory.write<4>(row+8,prices[pick(prices.size())]);state->memory.write<4>(row+12,prices[pick(prices.size())]);
        state->memory.write<1>(metadata+100+12,pick(2)?0:0x30);
        for(unsigned k=0;k<7;++k)state->memory.write<4>(clubs+0x248+0x174+k*4,staff+(5+k)*0x70);
        if(n){state->wr<4>(24,pick(2));state->wr<4>(28,pick(3));state->wr<4>(32,pick(8));state->wr<4>(36,pick(2)?flags:0);}else{state->wr<4>(32,prices[pick(prices.size())]);state->wr<4>(36,pick(2));}
        for(auto target:{0x100d3230u,0x100d3150u,0x100cab10u,0x100da050u,0x100da0b0u,0x10464160u,0x100d9520u,0x100d9580u,0x103092f0u,0x100d3040u,0x1007e820u,0x101929b0u,0x1014c2e0u,0x1030fbf0u})o.words[target]=pick(4)==0;
        o.words[0x1007e820]=pick(5);o.words[0x100cdef0]=values[pick(values.size())];o.words[0x103092f0]=pick(5);o.words[0x100d3040]=pick(5);o.words[0x100f55b0]=pick(5)==4?0xfffffffe:pick(4);o.words[0x100f2680]=prices[pick(prices.size())];o.words[0x10718130]=pick(50);
        const std::array scales{0.0,0.1,0.5,1.0,10.0,100.0,10000.0};o.reals[0x1037b6f0]=scales[pick(scales.size())];o.reals[0x10780fc0]=scales[pick(scales.size())];o.reals[0x10780fd8]=scales[pick(scales.size())];
        compare(*state,o);
    }

}


void exceptional_boundaries(){
    for(unsigned field=16;field<=40;++field)for(auto profile:{5000u,6000u,8500u,9000u,10000u})for(auto skill:{0u,5u,10u,20u,30u}){auto state=seed(0);state->memory.write<2>(staff+0x70+0x5c,field);state->memory.write<2>(profiles+0x100+12,profile);Options o;o.estimate=0;o.skill=skill;compare(*state,o);}
    for(auto address:{staff+0x70+0x14,staff+0x70+12,clubs+0x248+0x2c}){auto state=seed(1);state->memory.write<4>(address,0);boundaries(*state);}
    {auto state=seed(1);state->memory.write<1>(metadata+100+12,4);boundaries(*state);}
    {auto state=seed(1);state->memory.write<4>(clubs+0x2c,0);Options o;o.primary=true;boundaries(*state,o);}
    for(auto category:{0u,1u,2u,3u,4u,5u,6u,7u}){auto state=seed(1);state->memory.write<4>(contracts+0x100+4,1);state->wr<4>(32,category);Options o;o.primary=true;boundaries(*state,o);}
    for(auto ability:{5000u,6500u,7500u,8000u}){auto state=seed(1);state->memory.write<4>(clubs+0x248+0x2c,countries);state->memory.write<2>(profiles+0x100+10,ability);state->memory.write<2>(clubs+0x248+0x1ec,5000);Options o;o.primary=o.secondary=true;compare(*state,o);}
}


void numeric_and_policy_boundaries(){
    for(auto skill:{104u,105u,106u,120u,144u}){auto state=seed(0);Options o;o.skill=skill;compare(*state,o);}
    for(auto ability:{6000u,6250u,6500u,7000u,7250u,7500u,8000u})for(auto standing:{4300u,4500u,5000u,5200u,5500u,6000u})for(auto policy:{0u,1u}){
        auto state=seed(1);state->memory.write<4>(clubs+0x248+0x2c,countries);state->memory.write<2>(profiles+0x100+10,ability);state->memory.write<2>(clubs+0x248+0x1ec,standing);Options o;o.primary=o.secondary=true;o.words[0x100d3230]=policy;compare(*state,o);
    }
    for(auto signed_field:{0u,10u,60u,100u,127u,128u,156u,200u,255u})for(auto year:{2000u,2001u,2002u})for(auto day:{314u,315u,316u})for(auto group:{0u,1u,2u,3u,4u,7u}){
        auto state=seed(1);for(unsigned k=0;k<2;++k){state->memory.write<2>(staff+5*0x70+0x4e,year);state->memory.write<2>(staff+5*0x70+0x4c,day);}state->memory.write<1>(contracts+0x3000+0x4c,signed_field);state->memory.write<1>(contracts+0x3000+0x44,signed_field);state->memory.write<1>(contracts+0x3000+0x5c,8);Options o;o.primary=o.secondary=true;o.group=group;compare(*state,o);
    }
    for(auto target:{0x10780fc0u,0x10780fd8u,0x1037b6f0u})for(auto bits:{UINT64_C(0),UINT64_C(0x8000000000000000),UINT64_C(0x7ff0000000000000),UINT64_C(0xfff0000000000000),UINT64_C(0x7ff8000000001234),UINT64_C(0x7ff0000000000001),UINT64_C(0x0010000000000000)})for(unsigned n=0;n<2;++n){
        auto state=seed(n);for(unsigned off=1280;off<1304;++off)state->wr<1>(off,1);state->memory.write<1>(metadata+100+12,0x30);Options o;o.primary=o.secondary=true;o.reals[target]=float_value<8>(bits);compare(*state,o);
    }
    for(auto reason:{output,row+4,staff+0x70+8}){auto state=seed(1);state->wr<4>(36,reason);compare(*state);}
    for(auto value:{0u,1u,10u,100u,1000u,10000u,1000000u})for(auto quote:{0.0,1.0,1000.0,100000.0})for(auto contract_value:{0u,1u,10000u,1000000u}){
        auto state=seed(1);state->wr<4>(28,0);state->memory.write<4>(contracts+0x3000+12,contract_value);Options o;o.primary=o.secondary=true;o.words[0x100f2680]=value;o.reals[0x1037b6f0]=quote;compare(*state,o);
    }
}


void mapped_zero_and_child_mutations(){
    for(bool primary:{false,true})for(bool secondary:{false,true})for(unsigned category=0;category<8;++category)for(auto mode:{0u,1u}){
        auto state=seed(1);state->memory.map(0,0x800);state->memory.copy_in(0,state->memory.copy_out(clubs+0x248,0x248));state->memory.write<4>(0,2);state->memory.write<4>(related+0x12000,related+0x13000);state->memory.write<4>(state->memory.read<4>(toc-0x79f0),related+0x12000);state->wr<4>(20,0);state->wr<4>(24,mode);state->wr<4>(32,category);Options o;o.primary=primary;o.secondary=secondary;compare(*state,o);
    }
    for(bool primary:{false,true})for(auto category:{0u,1u,2u,3u,4u,5u,6u,7u})for(auto signed_mode:{0u,1u}){
        auto state=seed(1);state->memory.map(0,0x800);state->wr<4>(24,signed_mode);state->wr<4>(32,category);state->memory.write<4>(related+0x12000,related+0x13000);state->memory.write<4>(state->memory.read<4>(toc-0x79f0),related+0x12000);
        Options o;o.primary=primary;o.secondary=true;o.writes[0x10082f90]={{clubs+0x2c,0},{clubs+0x248+0x2c,0}};compare(*state,o);
    }
    for(auto enabled:{0u,1u}){auto state=seed(0);state->wr<1>(1294,1);state->wr<1>(1308,enabled);Options o;o.floating_stop=enabled;compare(*state,o);}
    for(auto value:{0u,1u,100u}){auto state=seed(1);state->wr<4>(28,0);Options o;o.primary=o.secondary=true;o.words[0x10103510]=value?related+0x4000:0;o.reals[0x1037b6f0]=value;compare(*state,o);}
}


void remaining_rule_combinations(){
    for(auto skill:{106u,120u,144u})for(auto kind:{0u,1u,2u,3u,4u,5u,6u,7u})for(auto estimate:{0u,1000u,50000u,1000000u}){auto state=seed(0);state->memory.write<1>(row+0x28,kind*8);state->memory.write<2>(profiles+0x100+12,10000);Options o;o.skill=skill;o.estimate=estimate;compare(*state,o);}
    for(auto ability:{6500u,7000u,7500u})for(auto standing:{4500u,5000u,5500u,6000u})for(auto policy:{0u,1u}){auto state=seed(1);state->wr<4>(28,1);state->memory.write<4>(contracts+0x3000+0x50,0x10);state->memory.write<4>(clubs+0x248+0x2c,countries);state->memory.write<2>(clubs+0x1ec,10000);state->memory.write<2>(clubs+0x248+0x1ec,standing);state->memory.write<2>(profiles+0x100+10,ability);Options o;o.primary=o.secondary=true;o.words[0x100d3230]=policy;compare(*state,o);}
    for(auto preference:{128u,156u,200u,255u,0u,60u,100u})for(auto group:{0u,1u,2u,3u,4u})for(auto year:{1999u,2000u,2001u,2002u}){
        auto state=seed(1);state->memory.write<4>(contracts+0x100+4,1);state->memory.write<2>(staff+5*0x70+0x4e,year);state->memory.write<2>(staff+5*0x70+0x4c,315);state->memory.write<1>(contracts+0x3000+0x44,preference);state->memory.write<1>(contracts+0x3000+0x4c,preference);Options o;o.primary=o.secondary=true;o.group=group;o.words[0x1014c2e0]=1;compare(*state,o);
    }
    for(auto bit:{1u,16u})for(auto field:{16u,24u,25u})for(auto quote:{0.0,10.0,100.0}){auto state=seed(1);state->memory.write<4>(staff+0x70+0x1c,0);state->memory.write<4>(contracts+0x3000+0x50,bit);state->memory.write<2>(staff+0x70+0x5c,field);state->wr<4>(32,4);Options o;o.primary=o.secondary=true;o.reals[0x1037b6f0]=quote;compare(*state,o);}
    for(auto policy:{0u,1u})for(auto date_year:{2000u,2001u,2002u})for(auto date_day:{100u,200u,300u}){
        auto state=seed(1);state->memory.write<2>(profiles+0x100+6,200);state->memory.write<2>(staff+0x70+0x5c,20);state->memory.write<1>(contracts+0x3000+0x44,10);state->memory.write<2>(contracts+0x3000+0x30,date_day);state->memory.write<2>(contracts+0x3000+0x32,date_year);Options o;o.primary=o.secondary=true;o.words[0x101929b0]=1;o.words[0x1014c2e0]=policy;o.words[0x10718130]=100;compare(*state,o);
    }
}


void derived_input_boundaries(){
    require(gate_inputs.contains(0x1036cf70),"Complete input for financial refinement");
    auto base=clone(*gate_inputs.at(0x1036cf70).first);auto original_options=gate_inputs.at(0x1036cf70).second;
    for(auto quote:{1.0,10.0,100.0,1000.0,10000.0})for(auto finance:{0u,1u,10u,50u,100u,500u,700u,800u,900u,1000u,5000u,7000u,8000u,9000u,10000u,50000u,70000u,80000u,90000u,100000u}){
        auto options=original_options;options.reals[0x1037b6f0]=quote;options.words[0x100f2680]=finance;compare(*base,options);
    }

    {State observed;observed.registers=gate_registers.at(0x1036cf70);const auto score=float_value<8>(observed.r<8>(0x4000+31*16));const auto scale=std::max(0.9,3.5/std::max(0.1,score));
     for(auto ratio:{0.64,0.65,0.66,0.68,0.69,0.70,0.71,0.8,0.9,1.0,1.1}){auto options=original_options;options.reals[0x1037b6f0]=10000;options.words[0x100f2680]=std::uint32_t(10000*scale*ratio);compare(*base,options);}}
    {auto options=original_options;options.words[0x10103510]=0;compare(*base,options);}
    for(auto variants:{&first_gate_inputs,&gate_inputs})for(auto gate:{0x1036c994u,0x1036caacu,0x1036c8d4u})if(variants->contains(gate)){
        const auto original=variants->at(gate).second;auto input=clone(*variants->at(gate).first);
        for(auto group:{1u,2u,3u})for(auto field:{0u,10u,20u,50u,100u,127u,128u,200u,255u})for(auto request:{0u,1u}){
            auto state=clone(*input);state->wr<4>(28,request);state->memory.write<1>(staff+0x70+0x67,field);state->memory.write<1>(contracts+0x3000+0x54,0);state->memory.write<1>(contracts+0x3000+0x60,group);state->memory.write<1>(contracts+0x3000+0x5c,0);state->memory.write<2>(staff+5*0x70+0x4e,1900);Options options=original;options.group=group;options.words[0x1014c2e0]=0;compare(*state,options);
        }
    }

    if(gate_inputs.contains(0x1036c8d4))for(auto days:{1u,5u,10u,20u,50u,100u,200u})for(auto option:{0u,1u})for(auto attributes:{10u,20u,127u}){
        auto state=clone(*gate_inputs.at(0x1036c8d4).first);auto options=gate_inputs.at(0x1036c8d4).second;
        state->wr<4>(36,flags);state->wr<4>(28,option);state->memory.write<4>(contracts+0x100+4,0);state->memory.write<1>(contracts+0x3000+0x54,0);state->memory.write<4>(contracts+0x3000+0x50,0);state->memory.write<2>(staff+0x70+0x4e,2001);state->memory.write<2>(staff+0x70+0x4c,100-days);state->memory.write<1>(staff+0x70+0x65,attributes);options.primary=options.secondary=true;options.words[0x100d3150]=options.words[0x100d3230]=0;compare(*state,options);
    }
    for(auto gate:{0x1036c994u,0x1036caacu})if(gate_inputs.contains(gate))for(auto group:{1u,2u,3u})for(auto preference:{0u,10u,20u,60u,127u,128u,200u,255u})for(auto field:{10u,20u,50u,127u}){
        auto state=clone(*gate_inputs.at(gate).first);auto options=gate_inputs.at(gate).second;options.group=group;options.words[0x1014c2e0]=0;state->wr<4>(36,flags);state->memory.write<1>(contracts+0x3000+0x5c,0);state->memory.write<4>(contracts+0x3000+0x50,1);state->memory.write<1>(contracts+0x3000+0x54,0);state->memory.write<1>(contracts+0x3000+0x60,group);state->memory.write<1>(contracts+0x3000+0x44,preference);state->memory.write<1>(contracts+0x3000+0x4c,preference);state->memory.write<1>(staff+0x70+0x67,field);state->memory.write<2>(staff+5*0x70+0x4e,1900);compare(*state,options);
    }

    for(auto gap:{1u,10u,100u,500u,1000u})for(auto days:{1u,10u,50u,100u})for(auto mode:{0u,1u}){
        auto state=seed(1);state->wr<4>(28,0);state->wr<4>(24,mode);state->memory.write<2>(staff+0x70+0x4e,2001);state->memory.write<2>(staff+0x70+0x4c,100-days);state->memory.write<2>(clubs+0x248+0x1ec,5000);state->memory.write<2>(clubs+0x1ec,5000-gap);state->memory.write<2>(related+5*0x44+0x2a,10000);state->memory.write<4>(clubs+0x248+0x2c,countries);state->memory.write<2>(fixture+0x68000+6,5000-gap);state->memory.write<2>(fixture+0x68000+12+6,5000);Options options;options.primary=options.secondary=true;options.words[0x100cdef0]=10000;compare(*state,options);
    }

    for(auto variants:{&first_gate_inputs,&gate_inputs})for(auto gate:{0x1036c994u,0x1036caacu})if(variants->contains(gate))for(unsigned sample=0;sample<512;++sample){
        auto state=clone(*variants->at(gate).first);auto options=variants->at(gate).second;options.group=1+sample%3;options.words[0x1014c2e0]=0;state->wr<4>(36,flags);state->memory.write<4>(contracts+0x3000+0x50,1);state->memory.write<1>(contracts+0x3000+0x54,0);state->memory.write<1>(contracts+0x3000+0x5c,0);state->memory.write<1>(contracts+0x3000+0x60,1+sample%3);state->memory.write<2>(staff+5*0x70+0x4e,1900);
        const std::array fields{0u,10u,20u,50u,100u,127u,128u,255u};state->memory.write<1>(staff+0x70+0x63,fields[sample%8]);state->memory.write<1>(staff+0x70+0x65,fields[sample/8%8]);state->memory.write<1>(staff+0x70+0x67,fields[sample/64%8]);compare(*state,options);
    }



    if(gate_inputs.contains(0x1036c8d4)){
        auto base=clone(*gate_inputs.at(0x1036c8d4).first);auto options=gate_inputs.at(0x1036c8d4).second;
        for(unsigned standing=0;standing<=10000;standing+=10){auto state=clone(*base);state->wr<4>(36,flags);state->memory.write<2>(clubs+0x1ec,standing);compare(*state,options);}
        auto input=clone(*gate_inputs.at(0x1036c8d4).first);input->wr<4>(36,flags);compare(*input,gate_inputs.at(0x1036c8d4).second);
    }
    for(auto [gate,registers]:gate_registers){State value;value.registers=registers;std::cout<<"Closest gate "<<hex_address(gate)<<" group="<<std::int8_t(value.r<4>(26*4))+0<<" f3="<<float_value<8>(value.r<8>(0x4030))<<" f27="<<float_value<8>(value.r<8>(0x41b0))<<" f31="<<float_value<8>(value.r<8>(0x41f0))<<std::endl;}
}

void initial_cases(){for(unsigned n=0;n<2;++n){auto state=seed(n);boundaries(*state);Options options;options.own_negative=true;compare(*state,options);options.own_negative=false;options.child_negative=true;compare(*state,options);}}
}
namespace cm::aot {void shortlist_assessment_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try{rf::tally.name="Shortlist assessment";require(argc==1||argc==2,"Optional private coverage output path");initial_cases();contribution_cases();interest_cases();targeted_contribution_cases();context_combinations();exceptional_boundaries();numeric_and_policy_boundaries();mapped_zero_and_child_mutations();remaining_rule_combinations();derived_input_boundaries();const auto expected=shortlist_assessment_expected_sites();if(argc==2){std::ofstream out(argv[1]);for(auto pc:expected)out<<hex_address(pc)<<" "<<sites.contains(pc)<<" "<<completed_sites.contains(pc)<<"\n";}for(const auto& [error,count]:partial_record_faults)std::cout<<"Declared partial-record stop "<<count<<" "<<error<<std::endl;const std::set<std::uint32_t> structural(assessment::structurally_unreachable.begin(),assessment::structurally_unreachable.end());for(auto pc:expected)if(!sites.contains(pc)&&!structural.contains(pc))std::cerr<<"Uncovered assessment site "<<hex_address(pc)<<std::endl;std::cout<<"Shortlist assessment comparisons="<<cases<<" completions="<<completed<<" interruptions="<<interrupted<<" negatives="<<negatives<<" sites="<<sites.size()<<"/5797 completed_sites="<<completed_sites.size()<<" structural_sites="<<structural.size()<<std::endl;for(unsigned n=0;n<2;++n)std::cout<<"Root "<<hex_address(assessment::entries[n])<<" comparisons="<<root_cases[n]<<" completions="<<root_returns[n]<<" matched_stops="<<root_stops[n]<<" negatives="<<root_negatives[n]<<std::endl;
require(structural.size()==17&&sites.size()+structural.size()==expected.size()&&completed_sites==sites&&std::all_of(expected.begin(),expected.end(),[&](auto pc){return completed_sites.contains(pc)!=structural.contains(pc);}),"Every original assessment site requires a complete return or a checked structural proof");rf::finish();}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
