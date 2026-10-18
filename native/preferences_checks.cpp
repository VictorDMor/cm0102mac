#include "preferences.hpp"
#include "money_configuration.hpp"
#include "money_text.hpp"
#include "semantic_io_checks.hpp"
#include <bit>
#include <set>
#include <fstream>
namespace cm::aot {bool money_configuration_reference(State&);void money_configuration_fixture_tables(State&,std::uint32_t,std::uint32_t);bool preferences_reference(State&);std::vector<std::uint32_t> preferences_expected_sites();void preferences_fixture_tables(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace money=cm::aot::preferences;
constexpr std::uint32_t tables=fixture+0x10000,prefs=fixture+0x20000,source_prefs=fixture+0x21000,globals=fixture+0x30000,cash_tables=fixture+0x40000;
constexpr std::array<std::uint32_t,12> ends{0x1013f404,0x1013f4c4,0x1013f564,0x1013f67c,0x1013fb58,0x1013fc44,0x1013fcfc,0x1013fdf4,0x1013feac,0x101406d4,0x101406e8,0x10140724};
constexpr std::array<unsigned,12> site_counts{245,45,37,67,310,57,43,61,43,521,2,13};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{money::initialize_preferences,money::copy_preferences,money::destroy_preferences,money::set_defaults,money::select_option,money::set_language,money::set_currency,money::currency_for_language,money::set_interval,money::apply_preferences,money::video_mode,money::set_video_mode};
struct Options{
    unsigned interrupt=0;bool own_negative=false,effect_negative=false,redirect_flag=false,redirect_audio=false,redirect_link=false,redirect_audio_slot=false;
    bool file_exists=false,open_write=true,valid_magic=true,description_matches=true,language_valid=true,allocation=true,audio_ready=true,resource_ready=true,terminate=false;
    int fail_read=-1,fail_write=-1;std::uint32_t platform=0,capability=128,capability_flag=1,current_language=0,bound_language=0,file_language=2,file_currency=4,file_flags=0x212552a9,file_interval=30;
    std::array<std::uint32_t,2> screen_results{1,1};std::uint32_t windowed=1,audio_query=0;
};
struct Trace:SemanticGuestObserver {
    std::vector<std::uint32_t> sites;bool effect_negative=false,damaged=false;
    void before_step(State& s,std::uint32_t pc)override {sites.push_back(pc);if(effect_negative&&!damaged&&s.executed<s.budget){damaged=true;const auto v=s.memory.read<1>(globals+0x700);s.memory.write<1>(globals+0x700,v^1);s.memory.write<1>(globals+0x700,v);}}
};
thread_local Trace* original_trace=nullptr;
unsigned index(std::uint32_t root){const auto i=std::find(money::entries.begin(),money::entries.end(),root);require(i!=money::entries.end(),"Known preferences root");return unsigned(i-money::entries.begin());}
struct TimedEffects:MemoryObserver {
    State& state;Effects effects;std::vector<std::uint32_t> pcs;
    explicit TimedEffects(State& s):state(s){effects.state=&s;}
    void mapped(std::uint32_t a,std::uint64_t n)override{pcs.push_back(state.pc);effects.mapped(a,n);}
    void written(std::uint32_t a,const std::uint8_t* d,std::size_t n)override{pcs.push_back(state.pc);effects.written(a,d,n);}
    void cleared(std::uint32_t a,std::uint64_t n)override{pcs.push_back(state.pc);effects.cleared(a,n);}
    void read(std::uint32_t a,const std::uint8_t* d,unsigned n)override{pcs.push_back(state.pc);effects.read(a,d,n);}
};
struct Event{std::uint32_t target,stop;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
std::string text(State& s,std::uint32_t at){std::string out;for(unsigned n=0;n<1024;++n){const auto byte=s.memory.read<1>(at+n);if(!byte)return out;out.push_back(char(byte));}throw std::runtime_error("Unterminated preferences fixture string");}
void put(State& s,std::uint32_t at,const std::string& value){std::vector<std::uint8_t> bytes(value.begin(),value.end());bytes.push_back(0);s.memory.copy_in(at,bytes);}
struct Model:SemanticDependencyDispatch {
    bool original;Options options;Trace& trace;std::vector<Event> calls;std::array<unsigned,12> completed{};
    unsigned read_number=0,write_number=0,screen_number=0,configured=0,termination_requests=0;
    Model(bool old,Options o,Trace& t):original(old),options(o),trace(t){}
    void reference(State& s,std::uint32_t stop=return_sentinel){const auto n=index(s.pc),root=s.pc;bool initial=true;while(s.pc!=stop){if(root<=s.pc&&s.pc<ends[n]&&(initial||s.pc!=root))require(preferences_reference(s),"Complete original preferences reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));initial=false;}}
    void configuration_reference(State& s,std::uint32_t stop){while(s.pc!=stop){if(0x1000a440<=s.pc&&s.pc<0x1000b148)require(money_configuration_reference(s),"Independent complete currency dependency");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));}}
    void call(State& s,std::uint32_t target,std::uint32_t stop)override {
        const auto serial=calls.size();calls.push_back({target,stop,s.registers,{}});
        try {
            const auto entry=std::find(money::entries.begin(),money::entries.end(),target);
            if(entry!=money::entries.end()){
                const auto n=unsigned(entry-money::entries.begin());if(original)reference(s,stop);else methods[n](s,this,&trace);++completed[n];
            }else if(target==0x1000a440){if(original)configuration_reference(s,stop);else cm::aot::money_configuration::configure_currency(s,this,&trace);++configured;}
            else if(target==0x1000b150){cm::aot::money_text::display_setting(s,this,&trace);}
            else {
                const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};std::uint32_t result=globals+0x900;
                // These are explicit service models, not production platform or file I/O.
                s.memory.write<4>(globals+0x500,target);s.memory.write<4>(globals+0x504,s.memory.read<4>(globals+0x504)+1);
                if(target==0x1077dbf0||target==0x1077dc10){
                    const auto from=arg(4),to=arg(3),limit=target==0x1077dc10?arg(5):1024;result=to;bool padding=false;
                    for(unsigned n=0;n<limit;++n){const auto byte=padding?0:s.memory.read<1>(from+n);s.memory.write<1>(to+n,byte);if(!byte){if(target==0x1077dbf0)break;padding=true;}require(n<1023,"Terminated preferences copy model");}
                    if(options.redirect_link&&target==0x1077dbf0)s.memory.write<4>(prefs,source_prefs);
                }
                else if(target==0x1077dc90){const auto x=text(s,arg(3)),y=text(s,arg(4));result=x==y?0:x<y?0xffffffff:1;}
                else if(target==0x1077dda0){const auto haystack=text(s,arg(3)),needle=text(s,arg(4));const auto at=haystack.find(needle);result=at==std::string::npos?0:arg(3)+std::uint32_t(at);}
                else if(target==0x10739890){(void)text(s,arg(3));put(s,arg(6),"X");put(s,arg(7),"Y");}
                else if(target==0x10768810)result=globals+0x920;
                else if(target==0x1077b720){
                    const auto format=text(s,arg(4));std::string out;unsigned next=5;
                    const auto word=[&](){const auto i=next++;return i<=10?arg(i):std::uint32_t(s.memory.read<4>(arg(1)+0x38+(i-11)*4));};
                    for(std::size_t i=0;i<format.size();++i){if(format[i]!='%'){out+=format[i];continue;}require(++i<format.size(),"Complete diagnostic format");const auto v=word();if(format[i]=='s')out+=text(s,v);else if(format[i]=='c')out+=char(v);else if(format[i]=='d')out+=std::to_string(std::bit_cast<std::int32_t>(v));else throw std::runtime_error("Unknown preferences diagnostic conversion");}
                    put(s,arg(3),out);result=std::uint32_t(out.size());
                }
                else if(target==0x10769b60){(void)text(s,arg(3));(void)text(s,arg(4));if(options.redirect_flag)s.memory.write<4>(toc-0x7bac,globals+0x401);if(options.redirect_audio_slot)s.memory.write<4>(toc-0x7468,globals+0x5100);}
                else if(target==0x1073af50)result=options.platform;
                else if(target==0x1073b060)result=options.capability;
                else if(target==0x1074cc20)result=options.capability_flag;
                else if(target==0x10727880)result=options.current_language;
                else if(target==0x10726da0)result=options.language_valid&&arg(3)<11;
                else if(target==0x10726e90){options.current_language=arg(3);result=1;}
                else if(target==0x100b8b70)result=options.bound_language;
                else if(target==0x100b6050){options.bound_language=arg(3);result=1;}
                else if(target==0x10726be0){(void)text(s,arg(3));result=options.resource_ready;}
                else if(target==0x10746280){(void)text(s,arg(3));(void)text(s,arg(4));put(s,globals+0x1000,"Fixture resource path");result=globals+0x1000;}
                else if(target==0x10746300){(void)text(s,arg(3));put(s,arg(4),"Fixture games folder");result=arg(4);}
                else if(target==0x10739de0){(void)text(s,arg(3));const auto mode=text(s,arg(4));require(mode=="rb"||mode=="wb","Known preference open mode");result=(mode=="rb"?options.file_exists:options.open_write)?globals+0x2000:0;}
                else if(target==0x10775fe0){
                    const auto field=read_number++;require(field<8&&arg(5)==1&&arg(6)==globals+0x2000,"Eight ordered modeled preference reads");result=options.fail_read==int(field)?0:1;
                    if(result){switch(field){case 0:s.memory.write<4>(arg(3),options.valid_magic?0x1553117c:0);break;case 1:s.memory.write<4>(arg(3),options.file_flags);break;case 2:s.memory.write<1>(arg(3),options.file_language);break;case 3:s.memory.write<1>(arg(3),options.file_currency);break;case 4:s.memory.write<2>(arg(3),options.file_interval);break;case 5:for(unsigned n=0;n<arg(4);++n)s.memory.write<1>(arg(3)+n,n==255?0:'Z');break;case 6:s.memory.write<4>(arg(3),7);break;case 7:put(s,arg(3),options.description_matches?"Device7":"Other device");break;}}
                }
                else if(target==0x10776360){
                    const auto field=write_number++;require(field<8&&arg(5)==1&&arg(6)==globals+0x2000,"Eight ordered modeled preference writes");result=options.fail_write==int(field)?0:1;
                    for(unsigned n=0;n<arg(4);++n){const auto byte=s.memory.read<1>(arg(3)+n);s.memory.write<1>(globals+0x2100+field*0x200+n,byte);}
                }
                else if(target==0x10750b60){const auto description="Device"+std::to_string(std::bit_cast<std::int32_t>(arg(3)));require(description.size()+1<=arg(5),"Bounded device description model");put(s,arg(4),description);result=1;}
                else if(target==0x10768820){result=options.screen_results[std::min(screen_number++,1u)];}
                else if(target==0x10769b50)result=options.windowed;
                else if(target==0x10773520){require(arg(3)==44,"Audio allocation model size");result=options.allocation?globals+0x4000:0;}
                else if(target==0x10744670){s.memory.write<1>(arg(3)+0x28,options.audio_ready);result=arg(3);}
                else if(target==0x10744700){if(options.redirect_audio)s.memory.write<4>(tables+0x1000,globals+0x5000);}
                else if(target==0x10712ce0)result=options.audio_query;
                else if(target==0x10712cc0){options.audio_query=arg(3);result=1;}
                else if(target==0x1076f7d0){++termination_requests;if(options.terminate)throw std::runtime_error("Preferences fixture termination");result=0;}
                else if(target==0x107766f0||target==0x10776690||target==0x107446b0||target==0x107735a0||target==0x10768990||target==0x10726cc0||target==0x10712c50||target==0x10742ec0||target==0x10744860||target==0x10711b40)result=1;
                else throw std::runtime_error("Unknown preferences dependency "+hex_address(target));
                for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xdd000000+n*0x101);
                for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(1000.25+n));
                s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=stop;
            }
            require(s.pc==stop,"Preferences dependency return ABI");calls[serial].after=s.registers;if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Preferences fixture interruption");
        }catch(...){calls[serial].after=s.registers;throw;}
    }
};
std::unique_ptr<State> seed(unsigned n){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);s->memory.map(fixture,0x60000);s->memory.map(stack-0x2000,0x3000);s->memory.map(toc-0x8000,0x9000);s->pc=money::entries[n];s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->budget=100000;
    for(unsigned f=1280;f<=1311;++f)s->wr<1>(f,0);
    preferences_fixture_tables(*s,toc,tables);money_configuration_fixture_tables(*s,toc,cash_tables);
    s->memory.write<1>(tables+0x7000,255);s->memory.write<1>(globals+0x401,255);put(*s,globals+0x920,"TEST");
    s->memory.write<4>(toc-0x7ba8,globals+0x100);s->memory.write<4>(toc-0x7bac,globals+0x400);s->memory.write<1>(globals+0x400,255);
    for(auto at:{prefs,source_prefs}){s->memory.write<4>(at,0);s->memory.write<4>(at+4,0x212552a9);s->memory.write<1>(at+8,0);s->memory.write<1>(at+9,0);s->memory.write<2>(at+10,30);put(*s,at+12,"Fixture games folder");put(*s,at+0x10c,"Fixture text");s->memory.write<4>(at+0x12c,0xfffffffe);put(*s,at+0x130,"Device-2");}
    s->wr<4>(12,prefs);s->wr<4>(16,n==1?source_prefs:n==4?1:n==8?30:0);return s;
}
unsigned cases=0,returns=0,stops=0,negatives=0,returning_exit_cases=0,terminated_cases=0,currency_children=0;std::array<unsigned,12> root_cases{},root_returns{},root_stops{},root_negatives{};std::set<std::uint32_t> complete_sites,all_sites;std::array<std::set<std::uint32_t>,12> root_complete_sites;
struct Result{std::unique_ptr<State> state;std::vector<Effects::Access> accesses;unsigned calls;std::string outcome;std::vector<Event> dependencies;};
std::array<std::uint64_t,12> child_returns{};
Result compare(const State& input,Options options={}){
    auto a=clone(input),b=clone(input);TimedEffects x(*a),y(*b);Trace ot,nt;nt.effect_negative=options.effect_negative;Model old(true,options,ot),modern(false,options,nt);a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto run=[&](State& s,Model& d,bool reference){try{if(reference){original_trace=&ot;d.reference(s);}else methods[index(s.pc)](s,&d,&nt);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;const auto label="Preferences root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Preferences fixture interruption"||expected=="Preferences fixture termination"||expected.starts_with("Unmapped data access at "),label+" unexpected reference: "+expected);require(expected==actual,label+" original="+expected+" modern="+actual);
    const auto negative=options.own_negative||options.effect_negative;if(negative)require(expected.empty(),label+" negative needs a complete return");if(options.own_negative)b->wr<4>(12,b->r<4>(12)^1);if(options.effect_negative){require(nt.damaged,label+" effect negative must execute");Image(*a).compare(*b,label+" restored effect image");}
    bool rejected=false;try{
        if(ot.sites!=nt.sites){const auto d=std::mismatch(ot.sites.begin(),ot.sites.end(),nt.sites.begin(),nt.sites.end());throw std::runtime_error(label+" source order at="+std::to_string(d.first-ot.sites.begin())+" old="+(d.first==ot.sites.end()?"end":hex_address(*d.first))+" new="+(d.second==nt.sites.end()?"end":hex_address(*d.second)));}
        if(old.calls!=modern.calls){for(unsigned i=0;i<std::min(old.calls.size(),modern.calls.size());++i)if(!(old.calls[i]==modern.calls[i])){const auto d=std::mismatch(old.calls[i].before.begin(),old.calls[i].before.end(),modern.calls[i].before.begin());throw std::runtime_error(label+" dependency="+std::to_string(i)+" target="+hex_address(old.calls[i].target)+" before register="+std::to_string(d.first-old.calls[i].before.begin()));}throw std::runtime_error(label+" dependency count");}
        require(x.effects.bytes==y.effects.bytes&&x.pcs==y.pcs,label+" ordered timed effects");require(a->executed==b->executed,label+" instruction count");Image(*a).compare(*b,label);
    }catch(const std::runtime_error&){if(!negative)throw;rejected=true;}require(rejected==negative,label+" discriminating negative");
    const auto root=index(input.pc);++cases;++root_cases[root];returns+=expected.empty()&&!negative;root_returns[root]+=expected.empty()&&!negative;stops+=!expected.empty();root_stops[root]+=!expected.empty();negatives+=negative;root_negatives[root]+=negative;if(!negative){for(auto pc:ot.sites)for(unsigned i=0;i<12;++i)if(money::entries[i]<=pc&&pc<ends[i])all_sites.insert(pc);if(expected.empty()){for(auto pc:ot.sites)for(unsigned i=0;i<12;++i)if(money::entries[i]<=pc&&pc<ends[i])complete_sites.insert(pc);for(auto pc:ot.sites)if(money::entries[root]<=pc&&pc<ends[root])root_complete_sites[root].insert(pc);}}
    require(old.completed==modern.completed&&old.configured==modern.configured&&old.termination_requests==modern.termination_requests,label+" dependency completion accounting");
    if(expected.empty()&&!negative){for(unsigned n=0;n<12;++n)child_returns[n]+=modern.completed[n];currency_children+=modern.configured;returning_exit_cases+=modern.termination_requests!=0;}
    terminated_cases+=expected=="Preferences fixture termination";
    return{std::move(a),x.effects.log,unsigned(old.calls.size()),expected,std::move(old.calls)};
}
void boundaries(const State& input,Options options={}){
    const auto base=compare(input,options);require(base.outcome.empty()&&base.state->pc==return_sentinel,"Preferences boundary baseline return");const auto steps=base.state->executed-input.executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto s=clone(input);s->budget=s->executed+cut;compare(*s,options);}
    std::set<std::uint32_t> bytes;for(const auto& a:base.accesses)if(a.size){bytes.insert(a.address);bytes.insert(a.address+std::uint32_t(a.size)-1);}std::set<std::uint32_t> holes;if(!bytes.empty())for(unsigned n=0;n<5;++n){auto i=bytes.begin();std::advance(i,std::ptrdiff_t(n*(bytes.size()-1)/4));holes.insert(*i);}
    for(auto hole:holes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;s->budget=input.budget;s->executed=input.executed;for(auto [at,size]:input.memory.mapped_ranges())for(auto [address,n]:hole<at||std::uint64_t(hole)>=std::uint64_t(at)+size?std::vector<std::pair<std::uint32_t,std::uint64_t>>{{at,size}}:std::vector<std::pair<std::uint32_t,std::uint64_t>>{{at,hole-at},{hole+1,std::uint64_t(at)+size-hole-1}})if(n){s->memory.map(address,n);s->memory.copy_in(address,input.memory.copy_out(address,n));}require(compare(*s,options).outcome.starts_with("Unmapped data access at "),"Removed money byte must fault");}
    for(unsigned cut:std::set<unsigned>{1,base.calls/2,base.calls})if(cut&&cut<=base.calls){auto o=options;o.interrupt=cut;compare(input,o);}
}
void corpus(bool probe){
    for(unsigned n=0;n<12;++n){auto initial=seed(n);compare(*initial);if(!probe){Options o;o.own_negative=true;compare(*initial,o);o.own_negative=false;o.effect_negative=true;compare(*initial,o);boundaries(*initial);}}
    if(probe)return;
    for(unsigned byte=0;byte<256;++byte){
        for(auto n:{5u,6u,7u})for(auto upper:{0u,0x12340000u}){auto s=seed(n);if(n==7)s->memory.write<1>(prefs+8,byte);else s->wr<4>(16,upper|byte);const auto out=compare(*s);if(n==6)require(out.state->r<4>(12)==(byte<25),"Currency setter accepts exactly the signed-byte choices 0..24");}
    }
    for(auto v:{0u,1u,6u,7u,8u,30u,364u,365u,366u,32767u,32768u,65535u,0x12340007u,0xffff016du,0xffffffffu}){auto s=seed(8);s->wr<4>(16,v);const auto out=compare(*s);const auto signed_value=std::bit_cast<std::int16_t>(std::uint16_t(v));require(out.state->r<4>(12)==(7<=signed_value&&signed_value<=365),"Interval signed-halfword bounds");}
    constexpr std::array valid_options{1u,2u,4u,8u,16u,32u,64u,128u,256u,512u,1024u,2048u,4096u,8192u,16384u,32768u,65536u,131072u,262144u,524288u,1048576u,2097152u,4194304u,8388608u,16777216u,33554432u,67108864u,536870912u,1073741824u};
    for(auto choice:valid_options)for(auto flags:{0u,0xffffffffu,0x212552a9u,0x96149614u}){auto s=seed(4);s->wr<4>(16,choice);s->memory.write<4>(prefs+4,flags);const auto out=compare(*s);require(out.state->r<4>(12)==1,"Valid mutually exclusive option");}
    for(auto choice:{0u,3u,5u,6u,7u,24u,96u,384u,0x8000000u,0x10000000u,0x60000000u,0x80000000u,0xffffffffu}){auto s=seed(4);s->wr<4>(16,choice);const auto out=compare(*s);require(out.state->r<4>(12)==0&&!out.state->memory.read<1>(globals+0x400),"Combined and unknown options are rejected");}
    for(auto choice:valid_options)for(auto delta:{-1,1}){auto s=seed(4);s->wr<4>(16,choice+std::uint32_t(delta));compare(*s);}
    for(auto platform:{0u,1u,2u,3u,255u,0x12340002u})for(auto capability:{0u,62u,63u,128u,0xffffffffu})for(auto flag:{0u,1u,128u,256u}){auto s=seed(3);Options o;o.platform=platform;o.capability=capability;o.capability_flag=flag;compare(*s,o);}
    for(auto n:{0u,4u,9u})for(auto flags:{0x212552a9u,0x21255329u})for(auto active:{false,true})for(auto ready:{false,true})for(auto allocation:{false,true}){auto s=seed(n);s->memory.write<4>(prefs+4,flags);if(active)s->memory.write<4>(tables+0x1000,globals+0x4000);s->memory.write<1>(globals+0x4028,ready);if(n==4)s->wr<4>(16,flags&128?128:256);Options o;o.audio_ready=ready;o.allocation=allocation;o.redirect_audio=true;compare(*s,o);}
    for(auto n:{0u,4u,9u}){auto s=seed(n);if(n==4)s->wr<4>(16,128);Options o;o.audio_ready=false;o.redirect_audio_slot=true;const auto out=compare(*s,o);require(!out.state->memory.read<4>(tables+0x1000),"Outer audio owner cleans its cached cell after the recursive option reloads the changed TOC slot");}
    for(auto n:{0u,9u})for(auto first:{0u,1u,128u,256u})for(auto second:{0u,1u})for(auto windowed:{0u,1u}){auto s=seed(n);s->memory.write<4>(prefs+0x12c,7);Options o;o.screen_results={first,second};o.windowed=windowed;compare(*s,o);if(!first&&!second){o.terminate=true;compare(*s,o);}}
    for(auto n:{0u,9u})for(auto flag:{0x212552a9u,0x212552b1u,0x212552c9u})for(auto query:{0u,1u}){auto s=seed(n);s->memory.write<4>(prefs+4,flag);Options o;o.audio_query=query;compare(*s,o);}
    for(int failed=-1;failed<8;++failed)for(auto magic:{false,true}){auto s=seed(0);Options o;o.file_exists=true;o.fail_read=failed;o.valid_magic=magic;compare(*s,o);}
    for(auto language:{0u,2u,10u,11u,255u})for(auto currency:{0u,4u,24u,25u,128u,255u})for(auto matching:{false,true}){auto s=seed(0);Options o;o.file_exists=true;o.file_language=language;o.file_currency=currency;o.description_matches=matching;compare(*s,o);}
    for(auto interval:{0u,6u,7u,365u,366u,65535u}){auto s=seed(0);Options o;o.file_exists=true;o.file_interval=interval;const auto out=compare(*s,o);require(out.state->memory.read<2>(prefs+10)==interval,"Saved interval is loaded directly, without calling the bounded setter");require(text(*out.state,prefs+12)=="Fixture games folder","Saved folder payload is consumed into the temporary buffer, retaining the initialized games path");}
    for(auto command:{"-windowed","-nosound","-defaultfont","-windowed -nosound -defaultfont"}){auto s=seed(0);put(*s,tables+0x2000,command);compare(*s);}
    for(auto ready:{false,true}){auto s=seed(0);Options o;o.resource_ready=ready;compare(*s,o);if(!ready){o.terminate=true;compare(*s,o);}}
    for(int failed=-1;failed<8;++failed){auto s=seed(9);Options o;o.fail_write=failed;o.redirect_flag=true;compare(*s,o);}
    {auto s=seed(9);Options o;o.open_write=false;compare(*s,o);}
    for(auto lang:{0u,2u,10u,255u})for(auto bound:{0u,2u})for(auto current:{0u,2u})for(auto context:{0u,1u}){auto s=seed(9);s->memory.write<1>(prefs+8,lang);s->memory.write<4>(tables+0x5000,context);Options o;o.current_language=current;o.bound_language=bound;compare(*s,o);}
    for(unsigned currency=0;currency<25;++currency){auto s=seed(9);s->memory.write<1>(prefs+9,currency);compare(*s);}
    {auto s=seed(9);s->memory.write<4>(prefs,source_prefs);compare(*s);Options o;o.redirect_link=true;compare(*s,o);}
    for(auto flag:{0u,1u,32767u,32768u,65535u,0x12340001u,0xffffffffu})for(auto linked:{false,true})for(auto active:{false,true}){auto s=seed(2);s->wr<4>(16,flag);if(linked)s->memory.write<4>(prefs,source_prefs);if(active)s->memory.write<4>(tables+0x1000,globals+0x4000);Options o;o.redirect_audio=true;compare(*s,o);}
    {auto s=seed(2);s->wr<4>(12,0);compare(*s);}
    for(auto displacement:{0u,1u,4u,8u,12u,0x10cu,0x130u}){auto s=seed(1);s->wr<4>(16,prefs+displacement);compare(*s);}
    for(auto length:{0u,1u,29u,30u,31u,254u,255u,256u}){auto s=seed(1);put(*s,source_prefs+12,std::string(length,'A'));compare(*s);}
    for(auto n:{1u,5u,6u,7u,8u,9u,10u,11u}){auto s=seed(n);s->wr<4>(12,0);require(compare(*s).outcome.starts_with("Unmapped data access at "),"Required preference object keeps its null fault");}
    for(auto n:{4u,5u,6u,8u}){auto s=seed(n);s->wr<4>(16,0xffffffff);Options o;o.redirect_flag=true;const auto out=compare(*s,o);require(out.state->memory.read<1>(globals+0x400)==255&&!out.state->memory.read<1>(globals+0x401),"Setter diagnostics load their flag pointer after the services");}
    {auto s=seed(9);Options o;o.fail_write=0;o.redirect_flag=true;const auto out=compare(*s,o);require(!out.state->memory.read<1>(globals+0x400)&&out.state->memory.read<1>(globals+0x401)==255,"Applying preferences keeps its cached diagnostic flag pointer");}
}

}
namespace cm::aot {void preferences_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}
void money_configuration_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try{require(argc==1||argc==2,"Optional coverage output or probe");const bool probe=argc==2&&std::string_view(argv[1])=="--probe";corpus(probe);const auto expected=preferences_expected_sites();if(argc==2&&!probe){std::ofstream out(argv[1]);for(auto pc:expected)out<<hex_address(pc)<<" "<<all_sites.contains(pc)<<" "<<complete_sites.contains(pc)<<'\n';}if(!probe)for(auto pc:expected)if(!complete_sites.contains(pc))std::cerr<<"Uncovered preferences site "<<hex_address(pc)<<std::endl;std::cout<<"Preferences comparisons="<<cases<<" returns="<<returns<<" stops="<<stops<<" negatives="<<negatives<<" completed_sites="<<complete_sites.size()<<"/"<<expected.size()<<std::endl;std::cout<<"Returning_exit_model_cases="<<returning_exit_cases<<" modeled_termination_stops="<<terminated_cases<<" currency_children="<<currency_children<<std::endl;for(unsigned n=0;n<12;++n)std::cout<<"Root "<<hex_address(money::entries[n])<<" comparisons="<<root_cases[n]<<" returns="<<root_returns[n]<<" stops="<<root_stops[n]<<" negatives="<<root_negatives[n]<<" completed_sites="<<root_complete_sites[n].size()<<" child_returns="<<child_returns[n]<<std::endl;if(!probe){require(currency_children>25,"Completed independent configuration dependency matrix");for(auto n:{3u,4u,7u,9u,10u})require(child_returns[n]>0,"Related preference child must return");require(returning_exit_cases>0&&terminated_cases>0,"Both explicit returning-exit and terminating-exit models must be distinguished");require(complete_sites.size()==expected.size(),"Every preferences instruction needs complete-return evidence");for(unsigned n=0;n<12;++n)require(root_complete_sites[n].size()==site_counts[n],"Every entry needs its own complete-return source coverage");require(std::all_of(root_negatives.begin(),root_negatives.end(),[](auto n){return n==2;}),"Every preferences root needs result and restored-effect negatives");}}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
