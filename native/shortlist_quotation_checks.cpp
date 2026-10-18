#include "shortlist_quotation.hpp"
#include "semantic_io_checks.hpp"
#include "register_context.hpp"
#include "compiler_arithmetic.hpp"
#include "stdlib_core.hpp"
#include <bit>
#include <set>
#include <map>
#include <fstream>
namespace cm::aot {std::vector<std::uint32_t> shortlist_quotation_expected_sites();bool shortlist_quotation_reference(State&);void shortlist_quotation_fixture_tables(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace quotation=cm::aot::shortlist_quotation;
constexpr std::uint32_t object=fixture+0x1000,clubs=fixture+0x10000,staff=fixture+0x20000,profiles=fixture+0x30000,countries=fixture+0x40000,related=fixture+0x50000,contracts=fixture+0x60000,globals=fixture+0x70000,metadata=fixture+0x74000,random=fixture+0x78000,tables=fixture+0x80000,limits=fixture+0x90000,row=fixture+0x1000+0x200,output=fixture+0x2000,flags=fixture+0x2100;
constexpr std::array<std::uint32_t,2> ends{0x10369074,0x1037e3d8};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{quotation::evaluate_candidate_context,quotation::calculate_reference_quote};
struct TimedEffects final:MemoryObserver {
    State& state;Effects effects;std::vector<std::uint32_t> source_pcs;
    explicit TimedEffects(State& s):state(s){effects.state=&s;}
    void mapped(std::uint32_t a,std::uint64_t n)override{source_pcs.push_back(state.pc);effects.mapped(a,n);}
    void written(std::uint32_t a,const std::uint8_t* b,std::size_t n)override{source_pcs.push_back(state.pc);effects.written(a,b,n);}
    void cleared(std::uint32_t a,std::uint64_t n)override{source_pcs.push_back(state.pc);effects.cleared(a,n);}
    void read(std::uint32_t a,const std::uint8_t* b,unsigned n)override{source_pcs.push_back(state.pc);effects.read(a,b,n);}
};
struct Options {
    std::map<std::uint32_t,std::uint32_t> words;
    std::map<std::uint32_t,double> reals;
    std::map<std::pair<std::uint32_t,std::uint32_t>,std::uint32_t> first_words,second_words;
    std::map<std::uint32_t,std::vector<std::pair<std::uint32_t,std::uint32_t>>> writes;
    unsigned interrupt=0;
    bool own_negative=false,child_negative=false,quote_result_negative=false,quote_effect_negative=false;
    bool primary=false,secondary=false,pointer_probe=false,floating_stop=false;
    std::uint32_t renewal=65535,classification=1;
};
struct Trace:SemanticGuestObserver {
    std::vector<std::uint32_t> sites;
    void before_step(State&,std::uint32_t pc)override{sites.push_back(pc);}
};
thread_local Trace* original_trace=nullptr;
unsigned index(std::uint32_t root){const auto i=std::find(quotation::entries.begin(),quotation::entries.end(),root);require(i!=quotation::entries.end(),"Known quotation root "+hex_address(root));return unsigned(i-quotation::entries.begin());}
void modern_root(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){methods[index(s.pc)](s,d,o);}
struct Event{std::uint32_t target,stop;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Model:SemanticDependencyDispatch {
    bool original;Options options;Trace& trace;std::vector<Event> calls;unsigned completed_selected_children=0;
    Model(bool old,Options o,Trace& t):original(old),options(std::move(o)),trace(t){}
    void reference(State& s,std::uint32_t stop=return_sentinel){const auto root=s.pc,n=index(root);bool first=true;while(s.pc!=stop){if(s.pc>=root&&s.pc<ends[n]&&(first||s.pc!=root))require(shortlist_quotation_reference(s),"Complete quotation reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));first=false;}}
    void call(State& s,std::uint32_t target,std::uint32_t stop)override {
        const auto serial=calls.size();calls.push_back({target,stop,s.registers,{}});
        try {
            const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};const auto first=arg(3),second=arg(4);std::uint32_t result=0;double real=0.5;bool special=false;
            s.memory.write<4>(globals+0x500,target);s.memory.write<4>(globals+0x508,s.memory.read<4>(globals+0x508)+1);
            if(target==0x107737f8){register_context::save_floating_suffix(s,24);special=true;}
            else if(target==0x10773848){register_context::restore_floating_suffix(s,24);special=true;}
            else if(target==0x10775540){stdlib_core::absolute_word(s);special=true;}
            else if(target==0x1037b6f0){if(original)reference(s,stop);else quotation::calculate_reference_quote(s,this,&trace);
                require(s.pc==stop,"Quotation child continuation");++completed_selected_children;
                if(!original&&options.quote_result_negative)s.wr<8>(0x4010,s.r<8>(0x4010)^1);
                if(!original&&options.quote_effect_negative){const auto before=s.memory.read<1>(globals+0x704);s.memory.write<1>(globals+0x704,before^1);s.memory.write<1>(globals+0x704,before);}
                special=true;}
            else if(target==0x10773780){compiler_arithmetic::double_to_unsigned_word(s);special=true;}
            else if(target==0x10082f20){const auto id=std::uint32_t(s.memory.read<4>(second));require(id<32,"Bounded quotation contract staff");result=options.primary?contracts+id*0x100:0;}
            else if(target==0x10082f90)result=options.secondary?contracts+0x3000:0;
            else if(target==0x1077b770)result=options.renewal;
            else if(target==0x100cdef0||target==0x100f4790)result=10000;
            else if(target==0x100c7d00||target==0x100de260||target==0x100d9440||target==0x1036ff00)result=0;
            else if(target==0x100ce250)result=1;
            else if(target==0x1077b720){s.memory.write<1>(first,0);result=0;}
            else if(target==0x100cab10)result=options.classification;
            else if(target==0x10103510)result=related+0x4000;
            else if(target==0x10718130)result=1000; // Explicit scalar day-count fixture; no date construction.
            else if(target==0x103721f0){s.memory.clear(first,0x40);result=first;}
            else if(target==0x10780fc0){s.memory.write<4>(arg(1)+0x14,arg(2));s.wr<4>(8,0x18000000);}
            else if(target==0x103f43b0||target==0x100da050||target==0x100da0b0||target==0x10464160||target==0x100d9520||target==0x100d9580||target==0x100f55b0||target==0x103092f0||target==0x100f2680)result=0;
            else throw std::runtime_error("Unknown quotation dependency "+hex_address(target));
            if(!special){
                if(options.words.contains(target))result=options.words.at(target);
                if(options.first_words.contains({target,first}))result=options.first_words.at({target,first});
                if(options.second_words.contains({target,second}))result=options.second_words.at({target,second});if(options.reals.contains(target))real=options.reals.at(target);
                for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xdd000000+n*0x101);
                for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(1000.25+n));
                s.wr<8>(0x4010,float_bits<8>(real));s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);
            }
            if(options.writes.contains(target))for(auto [address,value]:options.writes.at(target))s.memory.write<4>(address,value);
            s.pc=stop;if(options.child_negative&&!original){const auto before=s.memory.read<1>(globals+0x700);s.memory.write<1>(globals+0x700,before^1);s.memory.write<1>(globals+0x700,before);}calls[serial].after=s.registers;
            if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Fixture quotation dependency interruption");
        }catch(...){calls[serial].after=s.registers;throw;}
    }
};
std::unique_ptr<State> seed(unsigned n){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned offset=1280;offset<1312;++offset)s->wr<1>(offset,0);s->memory.map(fixture,0x100000);s->memory.map(stack-0x6000,0x7000);s->memory.map(toc-0x8000,0x9000);s->pc=quotation::entries[n];s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->budget=1000000;s->wr<4>(12,n==0?object:row);s->wr<4>(16,staff+0x70);s->wr<4>(20,clubs+0x248);s->wr<4>(24,0);s->wr<4>(28,n==0?0:0xffffffff);s->wr<4>(32,n==0?flags:1);s->wr<4>(36,0);
    const std::array slots{-31720,-31716,-31676,-31672,-31644,-31632,-31628,-31624,-31620,-31616,-31612,-31608,-31592,-31580,-31552,-31548,-31516,-31476,-31416,-31388,-31344,-31336,-31280,-31276,-31272,-31268,-31264,-31260,-31256,-31252,-31248,-31244,-31240,-31236,-31232,-31228,-31224,-31220,-31216,-31108,-30288,-30120,-30100,-30096,-30076,-30072,-30068,-29972,-29968,-29964,-29960,-29956,-29932,-29832,-26488,-15228};
    unsigned slot_index=0;for(auto slot:slots){const auto at=globals+0x100+slot_index++*16;s->memory.write<4>(toc+slot,at);s->memory.write<4>(at,0);}
    const auto global=[&](std::int32_t slot,std::uint32_t value){s->memory.write<4>(s->memory.read<4>(toc+slot),value);};
    global(-0x7be4,32);global(-0x7a9c,0);global(-0x7be8,clubs);global(-0x7bbc,related+0xe000);global(-0x7b9c,metadata);global(-0x7b90,0);global(-0x7b88,random+0x20);global(-0x7b84,random);global(-0x7b7c,object+0x400);global(-0x7b5c,related+0x9000);global(-0x7b40,object+0x800);global(-0x7b3c,object+0x900);global(-0x7b1c,object+0xa00);global(-0x7ab8,object+0xb00);global(-0x79f8,related);global(-0x79f4,16);global(-0x7984,related+0xa000);global(-0x7a70,related+0xb000);global(-0x79fc,29);global(-0x7a00,28);global(-0x7a14,27);global(-0x7af4,26);global(-0x7a08,25);global(-0x7590,24);
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
    unsigned identity=0;for(auto slot:{-0x7a18,-0x7a14,-0x7a10,-0x7a24,-0x7a20,-0x7a1c,-0x7af4,-0x7a00,-0x7a28,-0x7514,-0x757c,-0x7594,-0x7574,-0x7578,-0x7650,-0x750c,-0x7504,-0x7510,-0x7590,-0x75a8,-0x7508})global(slot,identity++);
    global(-0x79f0,related+0x2000);s->memory.write<4>(related+0x2000,related+0x2100);
    for(unsigned k=0;k<2;++k){s->memory.write<4>(clubs+k*0x248+0x3c,related+0x3000+k*0x100);s->memory.write<4>(related+0x3000+k*0x100+0x10,related+0x3400+k*0x100);s->memory.write<1>(related+0x3400+k*0x100+0x37,10);s->memory.write<2>(related+0x2100+k*0x4c+6,2000);s->memory.write<2>(related+0x2100+k*0x4c+8,2000);}
    const auto context_date=s->memory.read<4>(toc-0x7a9c);s->memory.write<2>(context_date,100);s->memory.write<2>(context_date+2,2001);
    shortlist_quotation_fixture_tables(*s,toc,tables);return s;
}unsigned cases=0,completed=0,interrupted=0,negatives=0,completed_quote_children=0;
std::array<unsigned,2> root_cases{},root_returns{},root_stops{},root_negatives{};std::set<std::uint32_t> sites,completed_sites;
struct CorpusCase{std::unique_ptr<State> state;Options options;};
std::unique_ptr<CorpusCase> selected_child_input;
struct Result{std::unique_ptr<State> state;std::vector<Effects::Access> accesses;std::vector<Event> calls;std::string outcome;};
Result compare(const State& input,Options options={}){
    auto a=clone(input),b=clone(input);TimedEffects x(*a),y(*b);a->memory.date_observer=&x;b->memory.date_observer=&y;Trace ot,nt;Model old(true,options,ot),modern(false,options,nt);
    const auto run=[&](State& state,Model& model,bool reference){try{if(reference){original_trace=&ot;model.reference(state);}else modern_root(state,&model,&nt);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;const auto label="Shortlist quotation root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Fixture quotation dependency interruption"||expected.starts_with("Unmapped data access at ")||(options.floating_stop&&expected=="Original enabled floating inexact exception requires a handler"),label+" fixture assertion: "+expected);
    require(expected==actual,label+" original="+expected+" modern="+actual);
    if(ot.sites!=nt.sites){for(unsigned k=0;k<std::min(ot.sites.size(),nt.sites.size());++k)if(ot.sites[k]!=nt.sites[k]){std::cerr<<"First quotation site "<<k<<" "<<hex_address(ot.sites[k])<<"/"<<hex_address(nt.sites[k])<<std::endl;break;}}
    const bool negative=options.own_negative||options.child_negative||options.quote_result_negative||options.quote_effect_negative;
    if(negative)require(expected.empty(),label+" negative requires a complete original return");
    if(options.quote_result_negative||options.quote_effect_negative)
        require(old.completed_selected_children&&modern.completed_selected_children,label+" selected child must actually complete");
    if(options.own_negative)b->wr<8>(0x4010,b->r<8>(0x4010)^1);
    // Effect negatives deliberately restore their bytes. Only the ordered-effect
    // comparator may reject them; checking final memory alone is insufficient.
    if(options.child_negative||options.quote_effect_negative)Image(*a).compare(*b,label+" restored-effect image");
    bool rejected=false;
    try{
        require(old.calls==modern.calls,label+" dependency ABI");
        require(x.effects.bytes==y.effects.bytes,label+" ordered effects");
        require(x.source_pcs==y.source_pcs,label+" memory-effect instruction boundaries");
        require(ot.sites==nt.sites,label+" source order");
        require(a->executed==b->executed,label+" instruction accounting");
        Image(*a).compare(*b,label);
    }catch(const std::runtime_error&){if(!negative)throw;rejected=true;}
    require(rejected==negative,label+" discriminating negative");
    if(expected.empty()&&!rejected&&input.pc==quotation::entries[0]&&old.completed_selected_children&&!selected_child_input)
        selected_child_input=std::make_unique<CorpusCase>(CorpusCase{clone(input),options});
    const auto root=index(input.pc);++root_cases[root];root_returns[root]+=expected.empty()&&!rejected;root_stops[root]+=!expected.empty();root_negatives[root]+=rejected;
    if(input.pc==quotation::entries[0]&&expected.empty()&&!rejected)completed_quote_children+=std::count_if(old.calls.begin(),old.calls.end(),[](const auto& call){return call.target==quotation::entries[1];});
    ++cases;completed+=expected.empty()&&!rejected;interrupted+=!expected.empty();negatives+=rejected;if(!rejected&&expected!="Native instruction budget exhausted")sites.insert(ot.sites.begin(),ot.sites.end());if(!rejected&&expected.empty())completed_sites.insert(ot.sites.begin(),ot.sites.end());return{std::move(a),x.effects.log,old.calls,expected};
}
void boundaries(const State& input,Options options={}){
    auto base=compare(input,options);require(base.state->pc==return_sentinel,"Completed quotation boundary fixture root="+hex_address(input.pc)+" stop="+hex_address(base.state->pc)+" outcome="+base.outcome);const auto steps=base.state->executed-input.executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto state=clone(input);state->budget=state->executed+cut;compare(*state,options);}
    std::set<std::uint32_t> bytes;for(auto a:base.accesses)if(a.size){bytes.insert(a.address);bytes.insert(a.address+std::uint32_t(a.size)-1);}std::set<std::uint32_t> holes;if(!bytes.empty())for(unsigned n=0;n<6;++n){auto i=bytes.begin();std::advance(i,std::ptrdiff_t(n*(bytes.size()-1)/5));holes.insert(*i);}
    for(auto hole:holes){auto state=std::make_unique<State>();state->registers=input.registers;state->unique=input.unique;state->pc=input.pc;state->budget=input.budget;state->executed=input.executed;for(auto [start,size]:input.memory.mapped_ranges()){if(hole<start||std::uint64_t(hole)>=std::uint64_t(start)+size){state->memory.map(start,size);state->memory.copy_in(start,input.memory.copy_out(start,size));continue;}for(auto [a,n]:{std::pair{start,std::uint64_t(hole-start)},std::pair{hole+1,std::uint64_t(start)+size-hole-1}})if(n){state->memory.map(a,n);state->memory.copy_in(a,input.memory.copy_out(a,n));}}auto probe=options;probe.pointer_probe=true;const auto fault=compare(*state,probe);require(fault.outcome.starts_with("Unmapped data access at "),"Removed accessed byte must fault");}
    for(unsigned n:std::set<unsigned>{1,2,unsigned(base.calls.size()/2),unsigned(base.calls.size())})if(n&&n<=base.calls.size()){auto o=options;o.interrupt=n;compare(input,o);}
}

#include "shortlist_quotation_cases.inc"
void retained_cases(){
    for(unsigned n=0;n<retained_quotation_cases;++n){auto input=retained_quotation_case(n);const auto result=compare(*input.state,input.options);require(result.outcome.empty(),"Retained complete-entry input must return");}
}
void boundary_and_negative_cases(){
    for(unsigned n=0;n<2;++n){auto state=seed(n);boundaries(*state);Options options;options.own_negative=true;compare(*state,options);options.own_negative=false;options.child_negative=true;compare(*state,options);}
    require(bool(selected_child_input),"Completed contextual quotation child fixture");
    auto state=clone(*selected_child_input->state);auto options=selected_child_input->options;boundaries(*state,options);
    options.quote_result_negative=true;compare(*state,options);options.quote_result_negative=false;options.quote_effect_negative=true;compare(*state,options);
    for(auto alias:{staff+0x70+0x24,staff+0x70+12,staff+0x70+0x1c,staff+0x70+0x4c,clubs+0x248+0x2c}){
        auto input=seed(0);input->wr<4>(28,5);input->wr<4>(32,alias);compare(*input);
    }
    for(auto unused:{0u,flags,staff+0x70}){auto input=seed(1);input->wr<4>(12,unused);compare(*input);}
    for(auto total:{0u,1u,0xffffffffu})for(auto id:{0u,1u}){
        auto input=seed(1);input->memory.write<4>(input->memory.read<4>(toc-0x7be4),total);input->memory.write<4>(staff+0x70,id);compare(*input);
    }
    for(auto bits:{UINT64_C(0),UINT64_C(0x8000000000000000),UINT64_C(0x7ff0000000000000),UINT64_C(0xfff0000000000000),UINT64_C(0x7ff8123456789abc)}){
        auto input=seed(1);input->wr<4>(28,1);input->memory.write<2>(staff+0x70+0x5c,18);Options variant;variant.reals[0x10780fc0]=std::bit_cast<double>(bits);compare(*input,variant);
    }
}
}
namespace cm::aot {void shortlist_quotation_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){
    try{
        require(argc==1||argc==2,"Optional private coverage output path");retained_cases();boundary_and_negative_cases();
        const auto expected=shortlist_quotation_expected_sites();
        const std::set<std::uint32_t> structural(quotation::structurally_unreachable.begin(),quotation::structurally_unreachable.end());
        if(argc==2){std::ofstream out(argv[1]);for(auto pc:expected)out<<hex_address(pc)<<" "<<sites.contains(pc)<<" "<<completed_sites.contains(pc)<<"\n";}
        for(auto pc:expected)if(!completed_sites.contains(pc)&&!structural.contains(pc))std::cerr<<"Uncovered quotation site "<<hex_address(pc)<<std::endl;
        std::cout<<"Shortlist quotation comparisons="<<cases<<" completions="<<completed<<" interruptions="<<interrupted<<" negatives="<<negatives<<" sites="<<sites.size()<<"/5689 completed_sites="<<completed_sites.size()<<" structural_sites="<<structural.size()<<" quote_children="<<completed_quote_children<<std::endl;
        for(unsigned n=0;n<2;++n)std::cout<<"Root "<<hex_address(quotation::entries[n])<<" comparisons="<<root_cases[n]<<" completions="<<root_returns[n]<<" matched_stops="<<root_stops[n]<<" negatives="<<root_negatives[n]<<std::endl;
        require(structural.size()==75&&completed_sites.size()+structural.size()==expected.size()&&std::all_of(expected.begin(),expected.end(),[&](auto pc){return completed_sites.contains(pc)!=structural.contains(pc);}),"Every original quotation site requires a complete return or a checked structural proof");
        require(completed_quote_children>0&&root_negatives[0]==4&&root_negatives[1]==2,"Individual and selected-child negative controls");
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}
}
