#include "localization_format.hpp"
#include "semantic_io_checks.hpp"
#include <bit>
#include <fstream>
#include <set>

namespace cm::aot {
bool localization_format_reference(State&);
std::vector<std::uint32_t> localization_format_expected_sites();
void localization_format_fixture_tables(State&,std::uint32_t,std::uint32_t);
}
namespace {
using namespace cm::semantic_io_checks;
namespace format=cm::aot::localization_format;
constexpr std::array<std::uint32_t,1> ends{0x107283f8};
constexpr std::array<unsigned,1> site_counts{730};
constexpr std::uint32_t query=fixture+0x1000,argument_strings=fixture+0x3000,record_key=fixture+0x8000,
    argument_format=fixture+0x9000,translation=fixture+0xa000,substitution=fixture+0xb000,
    marker_buffer=fixture+0xc000,prefix_buffer=fixture+0xd000,pointer_table=fixture+0xe000,
    format_buffer=fixture+0xf000,tables=fixture+0x40000;
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{format::format_phrase};
unsigned index(std::uint32_t root){require(root==format::format_phrase_entry,"Known whole phrase-format root");return 0;}
void text(State& s,std::uint32_t at,const std::string& value){s.memory.copy_in(at,std::vector<std::uint8_t>(value.begin(),value.end()));s.memory.write<1>(at+std::uint32_t(value.size()),0);}
std::string text(const State& s,std::uint32_t at){std::string out;for(unsigned i=0;i<4096;++i){const auto c=s.memory.read<1>(at+i);if(!c)return out;out+=char(c);}throw std::runtime_error("Unterminated fixture text");}
struct Options {
    bool own_negative=false,effect_negative=false,child_negative=false,lookup_hit=true,empty_translation=false,
        null_record_strings=false,redirect_locale=false,redirect_mode=false,reverse_metadata=false,empty_substitution=false;
    unsigned interrupt=0,locale=1,mode=0,lookup_result=1;
    std::string source="Hello",translated="Translated",argument_list="Ada~Bob~Cat~Dan~Eve~Fox~Gil~Hal~Ian~Jay";
    std::vector<std::uint32_t> prefix_arguments;
};
struct Trace:SemanticGuestObserver {
    Options options;std::vector<std::uint32_t> sites;bool effect_negative=false,damaged=false;
    void before_step(State& s,std::uint32_t pc)override {
        sites.push_back(pc);
        if(effect_negative&&!damaged&&s.executed<s.budget){damaged=true;const auto value=s.memory.read<1>(fixture+0x20000);s.memory.write<1>(fixture+0x20000,value^1);s.memory.write<1>(fixture+0x20000,value);}
    }
};
thread_local Trace* original_trace=nullptr;
struct TimedEffects:MemoryObserver {
    State& state;Effects effects;std::vector<std::uint32_t> pcs;
    explicit TimedEffects(State& s):state(s){effects.state=&s;}
    void mapped(std::uint32_t a,std::uint64_t n)override{pcs.push_back(state.pc);effects.mapped(a,n);}
    void written(std::uint32_t a,const std::uint8_t* d,std::size_t n)override{pcs.push_back(state.pc);effects.written(a,d,n);}
    void cleared(std::uint32_t a,std::uint64_t n)override{pcs.push_back(state.pc);effects.cleared(a,n);}
    void read(std::uint32_t a,const std::uint8_t* d,unsigned n)override{pcs.push_back(state.pc);effects.read(a,d,n);}
};
struct Event {std::uint32_t target,stop;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};


struct Model:SemanticDependencyDispatch {
    Options options;bool child_negative=false;std::vector<Event> calls;std::vector<std::uint32_t> released;
    explicit Model(Options value):options(value){}
    void reference(State& s,std::uint32_t stop=return_sentinel){bool initial=true;while(s.pc!=stop){if(format::format_phrase_entry<=s.pc&&s.pc<ends[0]&&(initial||s.pc!=format::format_phrase_entry))require(localization_format_reference(s),"Whole independent phrase-format reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));initial=false;}}
    std::string formatted(State& s,std::uint32_t fmt,std::uint32_t args){
        const auto input=text(s,fmt);std::string result;unsigned argument=0;
        for(unsigned pos=0;pos<input.size();++pos){if(input[pos]!='%'){result+=input[pos];continue;}require(++pos<input.size(),"Complete modeled format specifier");if(input[pos]=='%'){result+='%';continue;}const auto value=std::uint32_t(s.memory.read<4>(args+argument++*4));if(input[pos]=='s')result+=text(s,value);else if(input[pos]=='d')result+=std::to_string(std::bit_cast<std::int32_t>(value));else if(input[pos]=='u')result+=std::to_string(value);else throw std::runtime_error("Unsupported component format specifier");}
        return result;
    }
    void call(State& s,std::uint32_t target,std::uint32_t stop)override {
        const auto serial=calls.size();calls.push_back({target,stop,s.registers,{}});
        try {
            const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};std::uint32_t result=0;
            // Explicit component boundaries: lookup, variadic formatting, string
            // services, substitution and deletion are models, not runtime proof.
            if(target==0x10728400){require(!arg(5),"Formatter requests no row-index output");if(options.lookup_hit){for(unsigned i=0;i<3;++i)s.memory.write<4>(arg(3)+i*4,options.null_record_strings&&i<2?0:i==0?record_key:i==1?argument_format:translation);for(unsigned i=0;i<10;++i)s.memory.write<4>(arg(3)+12+i*4,options.reverse_metadata?9-i:i);}result=options.lookup_hit?options.lookup_result:0;if(options.redirect_locale)s.memory.write<1>(arg(2)-0x35ac,0);if(options.redirect_mode)s.memory.write<4>(arg(2)-0x35a8,1);if(child_negative)s.memory.write<1>(translation,'X');}
            else if(target==0x1077b6f0){const auto value=formatted(s,arg(4),arg(5));text(s,arg(3),value);result=std::uint32_t(value.size());}
            else if(target==0x1077dda0){const auto haystack=text(s,arg(3)),needle=text(s,arg(4));const auto at=haystack.find(needle);result=at==std::string::npos?0:arg(3)+std::uint32_t(at);}
            else if(target==0x1077dbd0){result=std::uint32_t(text(s,arg(3)).size());}
            else if(target==0x1077dbf0){const auto value=text(s,arg(4));text(s,arg(3),value);result=arg(3);}
            else if(target==0x10728ac0){const auto value=options.empty_substitution?std::string{}:text(s,arg(4))+text(s,arg(3))+text(s,arg(5));text(s,substitution,value);result=substitution;}
            else if(target==0x10773610){released.push_back(arg(3));}
            else throw std::runtime_error("Unknown phrase-format dependency "+hex_address(target));
            for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xdd000000+n*0x101);
            for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(1000.25+n));
            s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=stop;calls[serial].after=s.registers;
            if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Localization fixture interruption");
        }catch(...){calls[serial].after=s.registers;throw;}
    }
};
std::unique_ptr<State> seed(Options options={},std::uint32_t actual_toc=toc){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);s->memory.map(fixture,0x50000);s->memory.map(stack-0x4000,0x5000);s->memory.map(actual_toc-0x8000,0x9000);
    s->pc=format::format_phrase_entry;s->wr<4>(4,stack);s->wr<4>(8,actual_toc);s->wr<4>(12,destination);s->wr<4>(16,query);s->wr<4>(4128,return_sentinel);s->budget=100000;
    localization_format_fixture_tables(*s,actual_toc,tables);s->memory.write<4>(actual_toc-0x3eb0,marker_buffer);s->memory.write<4>(actual_toc-0x3eac,prefix_buffer);s->memory.write<4>(actual_toc-0x3ea8,pointer_table);s->memory.write<4>(actual_toc-0x3ea0,format_buffer);
    s->memory.write<4>(actual_toc-0x35a8,options.mode);s->memory.write<1>(actual_toc-0x35ac,options.locale);
    text(*s,query,options.source);text(*s,translation,options.empty_translation?"":options.translated);text(*s,argument_format,options.argument_list);text(*s,record_key,options.source);text(*s,destination,"initial-output");
    std::vector<std::uint32_t> args=options.prefix_arguments;
    for(unsigned i=0;i<20;++i){text(*s,argument_strings+i*0x100,i%2?"Bob":"Ada");args.push_back(argument_strings+i*0x100);}
    for(unsigned i=0;i<args.size();++i){if(i<6)s->wr<4>((i+5)*4,args[i]);s->memory.write<4>(stack+0x20+i*4,args[i]);}
    return s;
}
unsigned cases=0,returns=0,stops=0,negatives=0;
std::array<unsigned,1> root_returns{},root_stops{},root_negatives{};
std::array<std::set<std::uint32_t>,1> root_complete_sites;
std::set<std::uint32_t> complete_sites,all_sites;
struct Result {std::unique_ptr<State> state;std::vector<Effects::Access> accesses;unsigned calls;std::string outcome;std::vector<Event> dependencies;};
Result compare(const State& input,Options options={}) {
    auto a=clone(input),b=clone(input);TimedEffects x(*a),y(*b);Trace ot,nt;ot.options=nt.options=options;nt.effect_negative=options.effect_negative;Model old(options),modern(options);modern.child_negative=options.child_negative;a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto run=[&](State& s,Model& d,bool reference){try{if(reference){original_trace=&ot;d.reference(s);}else methods[index(s.pc)](s,&d,&nt);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="Localization format root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Localization fixture interruption"||expected.starts_with("Unmapped data access at "),label+" unexpected reference: "+expected);require(expected==actual,label+" original="+expected+" modern="+actual);
    const auto negative=options.own_negative||options.effect_negative||options.child_negative;
    if(negative)require(expected.empty(),label+" negative needs a complete return");
    if(options.own_negative)b->wr<4>(12,b->r<4>(12)^1);
    if(options.effect_negative){require(nt.damaged,label+" effect negative executed");Image(*a).compare(*b,label+" restored effect image");}
    bool rejected=false;
    try {
        if(ot.sites!=nt.sites){const auto delta=std::mismatch(ot.sites.begin(),ot.sites.end(),nt.sites.begin(),nt.sites.end());throw std::runtime_error(label+" source order at="+std::to_string(delta.first-ot.sites.begin())+" old="+(delta.first==ot.sites.end()?"end":hex_address(*delta.first))+" new="+(delta.second==nt.sites.end()?"end":hex_address(*delta.second)));}
        require(old.calls==modern.calls&&old.released==modern.released,label+" dependency ABI and file/ownership outcomes");require(x.effects.bytes==y.effects.bytes&&x.pcs==y.pcs,label+" ordered timed effects");require(a->executed==b->executed,label+" instruction accounting");Image(*a).compare(*b,label);
    } catch(const std::runtime_error&) {if(!negative)throw;rejected=true;}
    require(rejected==negative,label+" discriminating negative");const auto n=index(input.pc);++cases;returns+=expected.empty()&&!negative;root_returns[n]+=expected.empty()&&!negative;stops+=!expected.empty();root_stops[n]+=!expected.empty();negatives+=negative;root_negatives[n]+=negative;
    if(!negative)for(auto pc:ot.sites)if(format::entries[n]<=pc&&pc<ends[n]){all_sites.insert(pc);if(expected.empty()){complete_sites.insert(pc);root_complete_sites[n].insert(pc);}}
    return {std::move(a),x.effects.log,unsigned(old.calls.size()),expected,std::move(old.calls)};
}
void boundaries(const State& input,Options options={}) {
    const auto base=compare(input,options);require(base.outcome.empty()&&base.state->pc==return_sentinel,"Complete format boundary baseline");const auto steps=base.state->executed-input.executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto s=clone(input);s->budget=s->executed+cut;compare(*s,options);}
    std::set<std::uint32_t> bytes;for(const auto& access:base.accesses)if(access.size){bytes.insert(access.address);bytes.insert(access.address+std::uint32_t(access.size)-1);}
    std::set<std::uint32_t> holes;for(unsigned n=0;n<7;++n){auto at=bytes.begin();std::advance(at,std::ptrdiff_t(n*(bytes.size()-1)/6));holes.insert(*at);}
    for(auto hole:holes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;s->budget=input.budget;s->executed=input.executed;for(auto [at,size]:input.memory.mapped_ranges())for(auto [address,n]:hole<at||std::uint64_t(hole)>=std::uint64_t(at)+size?std::vector<std::pair<std::uint32_t,std::uint64_t>>{{at,size}}:std::vector<std::pair<std::uint32_t,std::uint64_t>>{{at,hole-at},{hole+1,std::uint64_t(at)+size-hole-1}})if(n){s->memory.map(address,n);s->memory.copy_in(address,input.memory.copy_out(address,n));}require(compare(*s,options).outcome.starts_with("Unmapped data access at "),"Missing required format byte must fault");}
    for(unsigned cut:std::set<unsigned>{1,base.calls/2,base.calls})if(cut&&cut<=base.calls){Options o=options;o.interrupt=cut;compare(input,o);}
}

std::vector<const Event*> calls_to(const Result& result,std::uint32_t target){std::vector<const Event*> out;for(const auto& e:result.dependencies)if(e.target==target)out.push_back(&e);return out;}
void corpus(bool probe){
    auto initial=seed();compare(*initial);if(probe)return;
    {Options o;o.own_negative=true;compare(*initial,o);o.own_negative=false;o.effect_negative=true;compare(*initial,o);o.effect_negative=false;o.child_negative=true;compare(*initial,o);}
    boundaries(*initial);
    for(auto locale:{0u,1u,2u,127u,128u,255u})for(bool hit:{false,true})for(bool empty:{false,true})for(auto mode:{0u,1u,0xffffffffu}){Options o;o.locale=locale;o.lookup_hit=hit;o.empty_translation=empty;o.mode=mode;const auto result=compare(*seed(o),o);require(result.state->r<4>(12)==destination,"Whole formatter returns the effective destination");if(!mode)require(text(*result.state,destination)==(locale!=0&&locale!=255&&hit&&!empty?o.translated:o.source),"Plain source/translation selection");}
    const std::vector<std::string> sources{""," ","  Hello","Hello","%%","%s","%s %s","%d","{}word","[]word","{p}word","[p}word","word{q}","word[q}","{p}word{q}","{p}word{q} end","a {p}word","a\n{p}word","x{q}","<%s>","{p}<%s>","<%s>{q}","{p}<%s>{q}","<%s> next","<%s> {}next","<%s> []next","<%s> }","<%s> ]","}","]","{p}","{p}a{q}b","a{q}b {p}c","a{q}\n{p}b"};
    for(const auto& src:sources)for(auto locale:{0u,1u})for(auto mode:{0u,1u}){Options o;o.source=src;o.locale=locale;o.mode=mode;o.translated="translated";compare(*seed(o),o);}
    const std::vector<std::string> translations{"", " ","plain","%%","%%%","100%% done","{p}word","{p} word","{}word",std::string(1,'\b'),std::string("before ")+char(8)+" after",std::string("{p}")+char(8),std::string(1,'\b')+"{q}",std::string("{p}")+char(8)+"{q} end",std::string(1,'\b')+char(8)+" "+char(8)};
    for(const auto& tr:translations)for(const auto& src:std::vector<std::string>{"Hello","<%s>","{p}<%s>","<%s>{q}","{p}<%s>{q}"})for(auto mode:{0u,1u}){Options o;o.source=src;o.translated=tr;o.mode=mode;compare(*seed(o),o);}
    for(auto src:{"<%s x>{}word","<%s x>[]word","<%s x>{p}word","<%s x>[p}word","<%s x>{p]word","<%s x>}word","<%s x>]word","<%s x>word"})for(auto mode:{0u,1u}){Options o;o.locale=0;o.source=src;o.mode=mode;compare(*seed(o),o);}
    for(auto status:{0u,1u,128u,255u,256u,257u,0xffffff80u}){Options o;o.lookup_result=status;const auto result=compare(*seed(o),o);require(text(*result.state,destination)==((status&255)?o.translated:o.source),"Lookup success tests signed low byte only");}
    for(auto src:{"%d","%u","%%","<%s>","{}%s"}){Options o;o.locale=0;o.source=src;o.prefix_arguments={0x80000000};if(o.source.find('s')!=std::string::npos)o.prefix_arguments.clear();compare(*seed(o),o);}
    for(bool empty:{false,true}){Options o;o.source="{p}<%s>{q}";o.translated=std::string("{P}")+char(8)+"{Q}";o.prefix_arguments={5,9};o.empty_substitution=empty;const auto result=compare(*seed(o),o);require(calls_to(result,0x10728ac0).size()==1,"Translated modifiers call one substitution");require(text(*result.state,destination)==(empty?"":"PAdaQ"),"Independent translated modifier composition");}
    for(auto src:{"{p}word","word{q}","{p}word{q}"}){Options o;o.locale=0;o.source=src;const auto result=compare(*seed(o),o);const auto expected=o.source=="{p}word"?"pword":o.source=="word{q}"?"wordq":"pwordq";require(text(*result.state,destination)==expected,"Independent fallback prefix/suffix composition");}
    for(bool reverse:{false,true}){Options o;o.translated=std::string(1,'\b')+"-"+char(8);o.reverse_metadata=reverse;const auto result=compare(*seed(o),o);require(text(*result.state,destination)==(reverse?"Jay-Ian":"Ada-Bob"),"Metadata reorders the split variadic fields");}
    {Options o;o.argument_list="%s~%s";o.translated=std::string(1,'\b')+" "+char(8);const auto result=compare(*seed(o),o);require(text(*result.state,destination)=="Ada Bob","Original r5/r6 variadic spills reach argument formatting");}
    for(unsigned variant=0;variant<6;++variant){Options o;o.redirect_locale=variant==0;o.redirect_mode=variant==1;o.reverse_metadata=variant==2;o.null_record_strings=variant==3;o.source=variant==4?std::string(1000,'a')+"{p}word":variant==5?std::string(998,'b')+"{}":"<%s>";o.translated=std::string(1,'\b');compare(*seed(o),o);}
    for(unsigned length:{990u,997u,1000u}){Options o;o.locale=0;o.source=std::string(length,'a')+"{q}";auto input=seed(o);input->budget=500000;const auto result=compare(*input,o);require(result.outcome.empty(),"Complete original length-boundary witness");if(o.source.size()>=1000){require(text(*result.state,destination)==o.source,"Original1000-byte guard retains the formatted marker text");require(calls_to(result,0x10728ac0).empty(),"Long fallback skips secondary marker rewriting");}else require(calls_to(result,0x10728ac0).size()==1,"Short fallback rewrites the suffix marker");}
    for(auto modifiers:std::vector<std::vector<std::uint32_t>>{{5,9},{255,128}}){Options o;o.source="{p}<%s>{q}";o.translated=std::string("{P}")+char(8)+"{Q}";o.prefix_arguments=modifiers;const auto result=compare(*seed(o),o);const auto children=calls_to(result,0x10728ac0);require(children.size()==1&&read_be<4>(children[0]->before.data()+24)==modifiers[0]&&read_be<4>(children[0]->before.data()+28)==modifiers[1],"Translated substitution receives both live modifier bytes");}
    for(auto relocated:{toc-0x100000u,toc+0x100000u})compare(*seed({},relocated));
    for(auto alias:{query,query+1,argument_strings,prefix_buffer}){auto s=seed();s->wr<4>(12,alias);compare(*s);}
    for(auto shift:{1u,3u}){auto s=seed();s->wr<4>(12,destination+shift);compare(*s);}
    {Options o;o.source="  Hello";const auto result=compare(*seed(o),o);require(result.state->r<4>(12)==destination+2&&text(*result.state,destination)=="  Translated","Leading spaces retained before effective destination");}
    for(auto src:{"Hello","{p}word{q}","<%s>"})for(auto locale:{0u,1u}){Options o;o.source=src;o.locale=locale;boundaries(*seed(o),o);}
    for(auto root_arg:{3u,4u}){auto s=seed();s->wr<4>(root_arg*4,0);require(compare(*s).outcome.starts_with("Unmapped data access at "),"Null source/destination retains original access fault");}
}
}
namespace cm::aot {void localization_format_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try{
    require(argc==1||argc==2,"Optional phrase-format coverage output or probe");const bool probe=argc==2&&std::string_view(argv[1])=="--probe";corpus(probe);const auto expected=localization_format_expected_sites();
    if(argc==2&&!probe){std::ofstream out(argv[1]);for(auto pc:expected)out<<hex_address(pc)<<" "<<all_sites.contains(pc)<<" "<<complete_sites.contains(pc)<<'\n';}
    if(!probe)for(auto pc:expected)if(!complete_sites.contains(pc))std::cerr<<"Uncovered phrase-format site "<<hex_address(pc)<<std::endl;
    std::cout<<"Localization format comparisons="<<cases<<" returns="<<returns<<" stops="<<stops<<" negatives="<<negatives<<" completed_sites="<<complete_sites.size()<<"/"<<expected.size()<<std::endl;
    if(!probe){require(root_complete_sites[0].size()==site_counts[0],"Every own format source site needs a complete-return witness");require(root_negatives[0]==3,"Whole format root needs own/effect/child negatives");}
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
