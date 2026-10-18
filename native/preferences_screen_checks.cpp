#include "preferences_screen.hpp"
#include "preferences.hpp"
#include "money_text.hpp"
#include "semantic_io_checks.hpp"
#include <bit>
#include <fstream>
#include <set>

namespace cm::aot {
bool preferences_screen_reference(State&);
std::vector<std::uint32_t> preferences_screen_expected_sites();
void preferences_screen_fixture_tables(State&,std::uint32_t,std::uint32_t);
bool money_text_reference(State&);
void money_text_fixture_tables(State&,std::uint32_t,std::uint32_t);
bool preferences_reference(State&);
}
namespace {
using namespace cm::semantic_io_checks;
namespace screen=cm::aot::preferences_screen;
constexpr std::uint32_t tables=fixture+0x10000,context=fixture+0x30000,prefs=fixture+0x40000,
    buffer=fixture+0x50000,globals=fixture+0x60000,currency_tables=fixture+0x70000,
    widgets=fixture+0x90000,page=fixture+0xa0000;
struct Options {
    std::uint32_t mode=0,flags=0x21255329,warm=0,color=0x2345,language=0,currency=0,interval=30;
    std::int32_t devices=2,view=0;
    std::array<std::uint32_t,11> first{},second{},bound{};
    bool panel=true,own_negative=false,effect_negative=false,redirect_context=false,
        shift_view=false,redirect_style_pointer=false,mutate_style_value=false,mutate_selection=false;
    unsigned interrupt=0;
    Options(){first.fill(1);second.fill(1);bound.fill(1);}
};
struct Trace:SemanticGuestObserver {
    std::vector<std::uint32_t> sites;bool effect_negative=false,damaged=false;
    void before_step(State& s,std::uint32_t pc)override {
        sites.push_back(pc);
        if(effect_negative&&!damaged&&s.executed<s.budget){damaged=true;const auto value=s.memory.read<1>(globals+0x3f0);s.memory.write<1>(globals+0x3f0,value^1);s.memory.write<1>(globals+0x3f0,value);}
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
std::string text(State& s,std::uint32_t address){std::string out;for(unsigned i=0;i<4096;++i){const auto byte=s.memory.read<1>(address+i);if(!byte)return out;out+=char(byte);}throw std::runtime_error("Unterminated preferences screen model string");}
void put(State& s,std::uint32_t address,const std::string& value){require(value.size()<2000,"Bounded presentation model output");for(unsigned i=0;i<value.size();++i)s.memory.write<1>(address+i,std::uint8_t(value[i]));s.memory.write<1>(address+std::uint32_t(value.size()),0);}
struct Event {
    std::uint32_t target,stop;decltype(State::registers) before,after;
    std::array<std::uint32_t,12> stack{};std::string label;
    bool operator==(const Event&)const=default;
};
struct Model:SemanticDependencyDispatch {
    bool original;Options options;Trace& trace;std::vector<Event> calls;
    unsigned language_queries=0,widget_count=0,currency_children=0,video_children=0;
    Model(bool old,Options value,Trace& observed):original(old),options(value),trace(observed){}
    void reference(State& s,std::uint32_t stop=return_sentinel){while(s.pc!=stop){if(screen::build_screen_entry<=s.pc&&s.pc<0x1033f840)require(preferences_screen_reference(s),"Complete original presentation reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));}}
    void child_reference(State& s,std::uint32_t stop,std::uint32_t first,std::uint32_t end,bool money){while(s.pc!=stop){if(first<=s.pc&&s.pc<end)require(money?money_text_reference(s):preferences_reference(s),"Complete independent presentation child");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));}}
    void call(State& s,std::uint32_t target,std::uint32_t stop)override {
        const auto serial=calls.size();calls.push_back({target,stop,s.registers,{},{},{}});
        try {
            const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};
            for(unsigned n=0;n<12;++n)calls[serial].stack[n]=std::uint32_t(s.memory.read<4>(arg(1)+0x38+n*4));
            if(target==0x1000b160){
                if(original)child_reference(s,stop,0x1000b160,0x1000b410,true);
                else cm::aot::money_text::currency_label(s,this,&trace);
                ++currency_children;
            } else if(target==0x101406e0){
                if(original)child_reference(s,stop,0x101406e0,0x101406e8,false);
                else cm::aot::preferences::video_mode(s,this,&trace);
                ++video_children;
            } else {
                // These are declared UI, formatting, language/device and layout
                // models. They do not claim native rendering or service proof.
                std::uint32_t result=1;s.memory.write<4>(globals+0x300,target);s.memory.write<4>(globals+0x304,s.memory.read<4>(globals+0x304)+1);
                if(target==0x1073ef70){require(arg(3)==context&&arg(4)<2,"Presentation session user-data ABI");result=arg(4)?options.mode:prefs;}
                else if(target==0x10726da0){require(arg(3)<11,"Bounded language enumeration");const auto phase=language_queries++<11;result=(phase?options.first:options.second)[arg(3)];if(options.mutate_selection&&!phase){s.memory.write<1>(prefs+8,10);s.memory.write<1>(prefs+9,24);}}
                else if(target==0x100b8b80){require(arg(3)<11,"Bound language query ABI");result=options.bound[arg(3)];}
                else if(target==0x1076bb20){require(!arg(3)&&!arg(5)&&!arg(6),"Presentation color lookup ABI");result=0x10000|arg(4);}
                else if(target==0x10712e60){require(!arg(3),"Original presentation setup argument");}
                else if(target==0x10768810){result=globals+0x800;}
                else if(target==0x1077dbf0){const auto value=text(s,arg(4));put(s,arg(3),value);result=arg(3);}
                else if(target==0x10727890||target==0x1077b720){
                    const auto format=text(s,arg(4));std::string value;unsigned next=5;
                    const auto word=[&](){const auto n=next++;return n<=10?arg(n):std::uint32_t(s.memory.read<4>(arg(1)+0x38+(n-11)*4));};
                    for(std::size_t i=0;i<format.size();++i){if(format[i]!='%'){value+=format[i];continue;}require(++i<format.size(),"Complete presentation model conversion");if(format[i]=='%'){value+='%';continue;}if(format[i]=='l')require(++i<format.size(),"Complete long presentation conversion");const auto v=word();if(format[i]=='s')value+=text(s,v);else if(format[i]=='d')value+=std::to_string(std::bit_cast<std::int32_t>(v));else if(format[i]=='u')value+=std::to_string(v);else throw std::runtime_error("Unknown presentation model format");}
                    put(s,arg(3),value);result=std::uint32_t(value.size());
                } else if(target==0x10750b00){result=std::bit_cast<std::uint32_t>(options.devices);}
                else if(target==0x10750b60){const auto value="Fixture device "+std::to_string(std::bit_cast<std::int32_t>(arg(3)));require(value.size()+1<=arg(5),"Bounded presentation device model");put(s,arg(4),value);result=1;}
                else if(target==0x103cbb30){for(unsigned n=7;n<=10;++n)s.memory.write<4>(arg(n),100+n*20);result=1;}
                else if(target==0x107198c0||target==0x10719740||target==0x107445a0||target==0x107444e0||target==0x10744390||target==0x10744430){
                    const bool panel=target==0x107198c0||target==0x10719740;
                    require(panel?(arg(3)==page||arg(3)==page+0x1000||arg(3)==page+0x2000):arg(3)==context,"UI context/view ownership");
                    const auto string=target==0x10719740?calls[serial].stack[6]:target==0x107445a0||target==0x107444e0?calls[serial].stack[1]:target==0x10744390||target==0x10744430?calls[serial].stack[2]:0;
                    if(string)calls[serial].label=text(s,string);
                    result=widgets+0x20*++widget_count;
                    if(widget_count==1){if(options.redirect_context)s.memory.write<4>(toc-0x7bb0,0xf0000000);if(options.shift_view)s.memory.write<2>(context+0x2c58,1);if(options.redirect_style_pointer)s.memory.write<4>(toc-0x7bd4,globals+0x700);if(options.mutate_style_value)s.memory.write<2>(tables,0x4321);}
                } else throw std::runtime_error("Unknown preferences screen dependency "+hex_address(target));
                for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xdd000000+n*0x101);
                for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(1000.25+n));
                s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=stop;
            }
            require(s.pc==stop,"Presentation dependency return ABI");calls[serial].after=s.registers;
            if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Preferences screen fixture interruption");
        } catch(...) {calls[serial].after=s.registers;throw;}
    }
};
std::unique_ptr<State> seed(const Options& o={}){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);s->memory.map(fixture,0xc0000);s->memory.map(stack-0x2000,0x3000);s->memory.map(toc-0x8000,0x9000);s->pc=screen::build_screen_entry;s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->budget=1000000;
    for(unsigned f=1280;f<=1311;++f)s->wr<1>(f,0);
    preferences_screen_fixture_tables(*s,toc,tables);money_text_fixture_tables(*s,toc,currency_tables);
    s->memory.write<4>(toc-0x7bb0,context);s->memory.write<4>(toc-0x7bb8,buffer);s->memory.write<4>(toc-0x7ba8,globals+0x900);s->memory.write<4>(toc-0x7bac,globals+0x980);
    s->memory.write<4>(tables+0xd000,o.panel);s->memory.write<1>(toc-0x3892,o.warm);s->memory.write<2>(toc-0x3890,o.color);
    for(unsigned n=0;n<4;++n)s->memory.write<2>(tables+n*0x1000,0x1111+n*0x1111);
    s->memory.write<2>(tables+0x9000,0x5555);s->memory.write<2>(tables+0xa000,0x6666);s->memory.write<2>(globals+0x700,0x9876);
    s->memory.write<2>(context+0x2c58,std::uint32_t(o.view));for(auto view:{-1,0,1})s->memory.write<4>(context+0x24+std::uint32_t(view*0x2c0),page+std::uint32_t((view+1)*0x1000));
    s->memory.write<4>(prefs+4,o.flags);s->memory.write<1>(prefs+8,o.language);s->memory.write<1>(prefs+9,o.currency);s->memory.write<2>(prefs+10,o.interval);s->memory.write<4>(prefs+0x12c,0xfffffffe);put(*s,globals+0x800,"Fixture version");
    return s;
}
unsigned cases=0,returns=0,stops=0,negatives=0,currency_returns=0,video_returns=0;
std::set<std::uint32_t> complete_sites,all_sites;
struct Result {std::unique_ptr<State> state;std::vector<Effects::Access> accesses;unsigned calls;std::string outcome;std::vector<Event> dependencies;unsigned language_queries,widgets;};
Result compare(const State& input,Options options={}){
    auto a=clone(input),b=clone(input);TimedEffects x(*a),y(*b);Trace ot,nt;nt.effect_negative=options.effect_negative;Model old(true,options,ot),modern(false,options,nt);a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto run=[&](State& s,Model& d,bool reference){try{if(reference){original_trace=&ot;d.reference(s);}else screen::build_screen(s,&d,&nt);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;const auto label="Preferences screen case="+std::to_string(cases);
    require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Preferences screen fixture interruption"||expected.starts_with("Unmapped data access at "),label+" unexpected reference: "+expected);require(expected==actual,label+" original="+expected+" modern="+actual);
    const auto negative=options.own_negative||options.effect_negative;if(negative)require(expected.empty(),label+" negative requires full return");if(options.own_negative)b->wr<4>(12,b->r<4>(12)^1);if(options.effect_negative){require(nt.damaged,label+" effect negative executed");Image(*a).compare(*b,label+" restored effect image");}
    bool rejected=false;
    try {
        if(ot.sites!=nt.sites){const auto d=std::mismatch(ot.sites.begin(),ot.sites.end(),nt.sites.begin(),nt.sites.end());throw std::runtime_error(label+" source order at="+std::to_string(d.first-ot.sites.begin())+" old="+(d.first==ot.sites.end()?"end":hex_address(*d.first))+" new="+(d.second==nt.sites.end()?"end":hex_address(*d.second)));}
        require(old.calls==modern.calls,label+" dependency entry/return ABI, stack words and labels");require(x.effects.bytes==y.effects.bytes&&x.pcs==y.pcs,label+" ordered timed effects");require(a->executed==b->executed,label+" accounting");Image(*a).compare(*b,label);
    } catch(const std::runtime_error&) {if(!negative)throw;rejected=true;}
    require(rejected==negative,label+" discriminating negative");require(old.currency_children==modern.currency_children&&old.video_children==modern.video_children,label+" completed child accounting");++cases;returns+=expected.empty()&&!negative;stops+=!expected.empty();negatives+=negative;
    if(!negative)for(auto pc:ot.sites)if(screen::build_screen_entry<=pc&&pc<0x1033f840){all_sites.insert(pc);if(expected.empty())complete_sites.insert(pc);}
    if(expected.empty()&&!negative){require(modern.currency_children==25&&modern.video_children==1,label+" whole currency labels and video getter children");currency_returns+=modern.currency_children;video_returns+=modern.video_children;}
    return {std::move(a),x.effects.log,unsigned(old.calls.size()),expected,std::move(old.calls),old.language_queries,old.widget_count};
}
Result check(Options o={}){return compare(*seed(o),o);}
void boundaries(Options o={}){
    auto input=seed(o);const auto base=compare(*input,o);require(base.outcome.empty()&&base.state->pc==return_sentinel,"Whole presentation boundary baseline");const auto steps=base.state->executed-input->executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto s=clone(*input);s->budget=s->executed+cut;compare(*s,o);}
    std::set<std::uint32_t> bytes;for(const auto& access:base.accesses)if(access.size){bytes.insert(access.address);bytes.insert(access.address+std::uint32_t(access.size)-1);}std::set<std::uint32_t> holes;for(unsigned n=0;n<7;++n){auto at=bytes.begin();std::advance(at,std::ptrdiff_t(n*(bytes.size()-1)/6));holes.insert(*at);}
    for(auto hole:holes){auto s=std::make_unique<State>();s->registers=input->registers;s->unique=input->unique;s->pc=input->pc;s->budget=input->budget;s->executed=input->executed;for(auto [at,size]:input->memory.mapped_ranges())for(auto [address,n]:hole<at||std::uint64_t(hole)>=std::uint64_t(at)+size?std::vector<std::pair<std::uint32_t,std::uint64_t>>{{at,size}}:std::vector<std::pair<std::uint32_t,std::uint64_t>>{{at,hole-at},{hole+1,std::uint64_t(at)+size-hole-1}})if(n){s->memory.map(address,n);s->memory.copy_in(address,input->memory.copy_out(address,n));}require(compare(*s,o).outcome.starts_with("Unmapped data access at "),"Removed presentation byte must fault");}
    for(unsigned cut:std::set<unsigned>{1,base.calls/2,base.calls}){auto interrupted=o;interrupted.interrupt=cut;compare(*input,interrupted);}
}
std::vector<const Event*> calls_to(const Result& r,std::uint32_t target){std::vector<const Event*> out;for(const auto& e:r.dependencies)if(e.target==target)out.push_back(&e);return out;}
std::uint32_t argument(const Event& e,unsigned n){return std::uint32_t(read_be<4>(e.before.data()+n*4));}
void corpus(bool probe){
    check();if(probe)return;
    {Options o;o.own_negative=true;check(o);o.own_negative=false;o.effect_negative=true;check(o);}
    for(auto mode:{0u,1u,2u,3u,255u,256u,257u,259u})for(auto flag:{0u,0xffffffffu,0x21255329u,0x40000000u}){Options o;o.mode=mode;o.flags=flag;check(o);}
    for(unsigned bit=0;bit<32;++bit){Options o;o.flags=std::uint32_t(1)<<bit;check(o);o.flags=~o.flags;check(o);}
    for(auto warm:{0u,1u,128u,255u,256u}){Options o;o.warm=warm;const auto result=check(o);require(result.state->memory.read<2>(toc-0x3890)==((warm&255)?o.color:0xc0),"Lazy color lookup depends on the stored flag byte");}
    for(unsigned language=0;language<11;++language){Options o;o.language=language;check(o);o.first.fill(0);o.first[language]=1;check(o);o.first[(language+1)%11]=1;check(o);}
    for(unsigned currency=0;currency<25;++currency){Options o;o.currency=currency;check(o);}
    for(auto value:{0u,7u,14u,30u,90u,180u,360u,365u,366u,0x8000u,0xffffu})for(auto flag:{0x1000u,0x2000u,0x3000u}){Options o;o.interval=value;o.flags=flag;check(o);}
    for(auto devices:{-1,0,1,19,20,21,100}){Options o;o.devices=devices;const auto result=check(o);const auto descriptions=calls_to(result,0x10750b60);require(descriptions.size()==1+unsigned(std::clamp(devices,0,20)),"Video enumeration clamps only the upper count to20 and skips nonpositive counts");require(argument(*descriptions[0],3)==0xfffffffe,"Current video description uses the complete stored mode");for(unsigned n=1;n<descriptions.size();++n)require(argument(*descriptions[n],3)==n-1,"Additional video descriptions enumerate consecutive ids");}
    for(auto mode:{0u,1u,2u,3u})for(bool panel:{false,true})for(auto allowed:{0u,1u,256u,257u}){Options o;o.mode=mode;o.panel=panel;o.bound.fill(allowed);check(o);}
    {Options o;o.first.fill(0);const auto result=check(o);require(result.language_queries==11,"A single/empty language set skips the second language enumeration");}
    {Options o;o.second.fill(0);const auto result=check(o);require(result.language_queries==22,"Availability is queried anew while building language rows");}
    {Options o;o.first.fill(0x100);o.second.fill(0x101);check(o);}
    for(auto view:{-1,0,1}){Options o;o.view=view;check(o);}
    for(unsigned kind=0;kind<5;++kind){Options o;o.redirect_context=kind==0;o.shift_view=kind==1;o.redirect_style_pointer=kind==2;o.mutate_style_value=kind==3;o.mutate_selection=kind==4;check(o);}
    {Options o;o.mode=3;const auto result=check(o);for(const auto* event:calls_to(result,0x10719740))require(event->stack[8]!=42&&event->stack[8]!=43&&event->stack[8]!=45&&event->stack[8]!=61,"Mode3 omits editable language/currency/auto-save/video choices");}
    {Options o;const auto result=check(o);std::set<unsigned> intervals,currencies;for(const auto* event:calls_to(result,0x10719740)){if(event->stack[8]==45)intervals.insert(event->stack[9]);if(event->stack[8]==43)currencies.insert(event->stack[9]);}require(intervals==std::set<unsigned>{7,14,30,90,180,360},"Original six auto-save intervals become editable controls");require(currencies.size()==25&&*currencies.begin()==0&&*currencies.rbegin()==24,"Every currency receives its action/payload");}
    boundaries();{Options o;o.mode=3;o.first.fill(0);o.warm=1;boundaries(o);} {Options o;o.devices=20;o.panel=true;o.bound.fill(0);boundaries(o);}
}
}
namespace cm::aot {
void preferences_screen_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}
void money_text_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}
void preferences_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}
}
int main(int argc,char** argv){try{
    require(argc==1||argc==2,"Optional whole presentation coverage output or probe");const bool probe=argc==2&&std::string_view(argv[1])=="--probe";corpus(probe);const auto expected=preferences_screen_expected_sites();
    if(argc==2&&!probe){std::ofstream out(argv[1]);for(auto pc:expected)out<<hex_address(pc)<<" "<<all_sites.contains(pc)<<" "<<complete_sites.contains(pc)<<'\n';}
    if(!probe)for(auto pc:expected)if(!complete_sites.contains(pc))std::cerr<<"Uncovered presentation site "<<hex_address(pc)<<std::endl;
    std::cout<<"Preferences screen comparisons="<<cases<<" returns="<<returns<<" stops="<<stops<<" negatives="<<negatives<<" completed_sites="<<complete_sites.size()<<"/"<<expected.size()<<" currency_children="<<currency_returns<<" video_children="<<video_returns<<std::endl;
    if(!probe){require(complete_sites.size()==expected.size(),"Whole presentation needs every own site in complete starting-root comparisons");require(negatives==2,"Whole presentation requires result and restored-effect negatives");require(currency_returns==returns*25&&video_returns==returns,"Every returning parent needs completed independent currency/video children");}
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
