#include "localization_lookup.hpp"
#include "semantic_io_checks.hpp"
#include <bit>
#include <fstream>
#include <set>

namespace cm::aot {
bool localization_lookup_reference(State&);
std::vector<std::uint32_t> localization_lookup_expected_sites();
void localization_lookup_fixture_tables(State&,std::uint32_t,std::uint32_t);
}
namespace {
using namespace cm::semantic_io_checks;
namespace lookup=cm::aot::localization_lookup;
constexpr std::array<std::uint32_t,3> ends{0x1072787c,0x10727888,0x10728ac0};
constexpr std::array<unsigned,3> site_counts{7,2,432};
constexpr std::uint32_t query=fixture+0x1000,catalog=fixture+0x2000,cache_strings=fixture+0x3000,
    stream_buffer=fixture+0x5000,error_buffer=fixture+0x8000,error_flag=fixture+0x9000,
    result_index=fixture+0xa000,allocations_base=fixture+0x10000,tables=fixture+0x40000;
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{lookup::initialize_record,lookup::current_language,lookup::lookup};
unsigned index(std::uint32_t root){const auto i=std::find(lookup::entries.begin(),lookup::entries.end(),root);require(i!=lookup::entries.end(),"Known localization lookup root");return unsigned(i-lookup::entries.begin());}
void text(State& s,std::uint32_t at,const std::string& value){s.memory.copy_in(at,std::vector<std::uint8_t>(value.begin(),value.end()));s.memory.write<1>(at+std::uint32_t(value.size()),0);}
std::string text(const State& s,std::uint32_t at){std::string out;for(unsigned i=0;i<2048;++i){const auto c=s.memory.read<1>(at+i);if(!c)return out;out+=char(c);}throw std::runtime_error("Unterminated fixture text");}
struct Options {
    bool cached=true,hit=true,open=true,old_strings=true,want_index=true,own_negative=false,effect_negative=false;
    bool redirect_output=false,redirect_catalog=false,redirect_count=false,redirect_column=false,dirty_cleanup=false,trim_key=false;
    unsigned interrupt=0,language_count=4,column=2,rows=3,hit_row=1,short_read=0;
    std::string key="Hello",file_key="H e l l o";
};
struct Trace:SemanticGuestObserver {
    Options options;std::vector<std::uint32_t> sites;bool effect_negative=false,damaged=false,trimmed=false;
    void before_step(State& s,std::uint32_t pc)override {
        sites.push_back(pc);
        const auto frame=std::uint32_t(s.r<4>(4));
        if(options.dirty_cleanup){
            if(pc==0x1072855c)s.memory.write<4>(frame+0x60,fixture+0xc000);
            if(pc==0x107286d8)s.memory.write<4>(frame+0x64,fixture+0xc100);
            if(pc==0x107286ec)s.memory.write<4>(frame+0x68,fixture+0xc200);
        }
        if(options.trim_key&&!trimmed&&pc==0x10728524){trimmed=true;s.memory.write<1>(frame+0x2bc+std::uint32_t(s.r<4>(24*4)),32);}
        if(effect_negative&&!damaged&&s.executed<s.budget){damaged=true;const auto value=s.memory.read<1>(error_flag+0x100);s.memory.write<1>(error_flag+0x100,value^1);s.memory.write<1>(error_flag+0x100,value);}
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
    Options options;std::vector<Event> calls;std::vector<std::uint8_t> file;
    std::uint32_t cursor=0;unsigned allocations=0;bool closed=false;std::vector<std::uint32_t> released;
    explicit Model(Options value):options(value){
        file.resize(12+options.language_count*4);
        const auto field=[&](std::string value,unsigned size){require(value.size()<size,"Fixture string fits disk field");const auto at=file.size();file.resize(at+size);std::copy(value.begin(),value.end(),file.begin()+std::ptrdiff_t(at));};
        for(unsigned row=0;row<options.rows;++row){
            field(options.hit&&row==options.hit_row?options.file_key:"missing"+std::to_string(row),0x226);
            field("descriptor"+std::to_string(row),0x1e);
            for(unsigned col=1;col<options.language_count;++col){
                field("translation"+std::to_string(row)+"-"+std::to_string(col),0x226);
                for(unsigned word=0;word<10;++word){const auto value=0x12340000u+row*0x1000+col*0x100+word;for(unsigned byte=0;byte<4;++byte)file.push_back(std::uint8_t(value>>(byte*8)));}
            }
        }
    }
    void reference(State& s,std::uint32_t stop=return_sentinel){const auto n=index(s.pc),root=s.pc;bool initial=true;while(s.pc!=stop){if(root<=s.pc&&s.pc<ends[n]&&(initial||s.pc!=root))require(localization_lookup_reference(s),"Whole independent localization lookup reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));initial=false;}}
    void call(State& s,std::uint32_t target,std::uint32_t stop)override {
        const auto serial=calls.size();calls.push_back({target,stop,s.registers,{}});
        try {
            const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};std::uint32_t result=0;
            // Explicit component boundaries: these children model strings,
            // allocation, comparator search, streams and error reporting.
            // Actual native services and child production coverage are separate.
            if(target==0x1077dbd0){result=std::uint32_t(text(s,arg(3)).size());}
            else if(target==0x1077dbf0){const auto value=text(s,arg(4));text(s,arg(3),value);result=arg(3);}
            else if(target==0x1077dc90){const auto left=text(s,arg(3)),right=text(s,arg(4));result=left<right?0xffffffffu:left==right?0u:1u;}
            else if(target==0x107735f0){require(arg(3)>0&&arg(3)<=1024,"Original bounded string allocation ABI");result=allocations_base+allocations++*0x1000;s.memory.clear(result,arg(3));if(options.redirect_output&&allocations==2)s.memory.write<4>(destination,result_index+0x100);if(options.redirect_catalog)s.memory.write<4>(arg(2)-0x3590,catalog+52);if(options.redirect_column)s.memory.write<4>(arg(2)-0x35a0,3);}
            else if(target==0x10773610){released.push_back(arg(3));}
            else if(target==0x10775600){require(arg(6)==52,"Original cached row width");const auto key=text(s,s.memory.read<4>(arg(3)));for(unsigned row=0;row<arg(5);++row){const auto at=arg(4)+row*arg(6);if(text(s,s.memory.read<4>(at))==key){result=at;break;}}}
            else if(target==0x10739de0){require(!options.cached,"File path opens a stream");result=options.open?fixture+0xb000:0;}
            else if(target==0x10777f10){require(arg(3)==fixture+0xb000&&(arg(5)==0||arg(5)==1),"Original stream seek ABI");cursor=arg(5)?cursor+arg(4):arg(4);}
            else if(target==0x10775fe0){require(arg(6)==fixture+0xb000,"Original item-read stream ABI");const auto count=std::uint64_t(arg(4))*arg(5);require(count<=0x400,"Bounded disk field read");const auto available=cursor<file.size()?file.size()-cursor:0;auto copied=std::min(count,std::uint64_t(available));if(options.short_read)copied=std::min(copied,std::uint64_t(options.short_read));if(copied)s.memory.copy_in(arg(3),std::vector<std::uint8_t>(file.begin()+cursor,file.begin()+cursor+std::ptrdiff_t(copied)));cursor+=std::uint32_t(copied);result=arg(4)?std::uint32_t(copied/arg(4)):0;if(options.redirect_count)s.memory.write<4>(arg(2)-0x3594,1);}
            else if(target==0x107766f0){require(arg(3)==fixture+0xb000,"Close the owned file stream");closed=true;}
            else if(target==0x10739890){s.memory.write<4>(arg(6),0x12345678);s.memory.write<4>(arg(7),0x9abcdef0);}
            else if(target==0x10768810){result=0x76543210;}
            else if(target==0x1077b720){text(s,arg(3),"localization-error");result=18;}
            else if(target==0x10769b60){require(!arg(5),"Original error-report ABI");}
            else throw std::runtime_error("Unknown localization dependency "+hex_address(target));
            for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xdd000000+n*0x101);
            for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(1000.25+n));
            s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=stop;calls[serial].after=s.registers;
            if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Localization fixture interruption");
        }catch(...){calls[serial].after=s.registers;throw;}
    }
};
std::unique_ptr<State> seed(unsigned n,Options options={},std::uint32_t language=0,std::uint32_t actual_toc=toc){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);s->memory.map(fixture,0x50000);s->memory.map(stack-0x3000,0x4000);s->memory.map(actual_toc-0x8000,0x9000);
    s->pc=lookup::entries[n];s->wr<4>(4,stack);s->wr<4>(8,actual_toc);s->wr<4>(12,destination);s->wr<4>(16,query);s->wr<4>(20,options.want_index?result_index:0);s->wr<4>(4128,return_sentinel);s->budget=100000;
    localization_lookup_fixture_tables(*s,actual_toc,tables);
    s->memory.write<4>(actual_toc-0x3ea0,stream_buffer);s->memory.write<4>(actual_toc-0x7ba8,error_buffer);s->memory.write<4>(actual_toc-0x7bac,error_flag);
    s->memory.write<4>(actual_toc-0x3590,catalog);s->memory.write<4>(actual_toc-0x3594,options.rows);s->memory.write<4>(actual_toc-0x3598,options.language_count);s->memory.write<4>(actual_toc-0x35a0,options.column);
    s->memory.write<1>(actual_toc-0x359c,options.cached?1:0);s->memory.write<1>(actual_toc-0x35ac,language);s->memory.write<1>(error_flag,1);s->memory.write<4>(result_index,0x87654321);
    text(*s,query,options.key);text(*s,result_index+0x100,"redirected");text(*s,stream_buffer,"initial");
    for(unsigned row=0;row<options.rows;++row){for(unsigned field=0;field<3;++field){const auto at=cache_strings+row*0x300+field*0x100;text(*s,at,field==0?(options.hit&&row==options.hit_row?"Hello":"missing"+std::to_string(row)):"cached"+std::to_string(row)+"-"+std::to_string(field));s->memory.write<4>(catalog+row*52+field*4,at);}for(unsigned word=0;word<10;++word)s->memory.write<4>(catalog+row*52+12+word*4,0x89120000+row*256+word);}
    for(unsigned field=0;field<3;++field){text(*s,fixture+0xc000+field*0x100,"owned"+std::to_string(field));s->memory.write<4>(destination+field*4,options.old_strings?fixture+0xc000+field*0x100:0);}for(unsigned word=0;word<10;++word)s->memory.write<4>(destination+12+word*4,0xfeed0000+word);
    return s;
}
unsigned cases=0,returns=0,stops=0,negatives=0;
std::array<unsigned,3> root_returns{},root_stops{},root_negatives{};
std::array<std::set<std::uint32_t>,3> root_complete_sites;
std::set<std::uint32_t> complete_sites,all_sites;
struct Result {std::unique_ptr<State> state;std::vector<Effects::Access> accesses;unsigned calls;std::string outcome;std::vector<Event> dependencies;};
Result compare(const State& input,Options options={}) {
    auto a=clone(input),b=clone(input);TimedEffects x(*a),y(*b);Trace ot,nt;ot.options=nt.options=options;nt.effect_negative=options.effect_negative;Model old(options),modern(options);a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto run=[&](State& s,Model& d,bool reference){try{if(reference){original_trace=&ot;d.reference(s);}else methods[index(s.pc)](s,&d,&nt);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="Localization lookup root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Localization fixture interruption"||expected.starts_with("Unmapped data access at "),label+" unexpected reference: "+expected);require(expected==actual,label+" original="+expected+" modern="+actual);
    const auto negative=options.own_negative||options.effect_negative;
    if(negative)require(expected.empty(),label+" negative needs a complete return");
    if(options.own_negative){if(input.pc==lookup::initialize_record_entry)b->memory.write<1>(destination,b->memory.read<1>(destination)^1);else b->wr<4>(12,b->r<4>(12)^1);}
    if(options.effect_negative){require(nt.damaged,label+" effect negative executed");Image(*a).compare(*b,label+" restored effect image");}
    bool rejected=false;
    try {
        if(ot.sites!=nt.sites){const auto delta=std::mismatch(ot.sites.begin(),ot.sites.end(),nt.sites.begin(),nt.sites.end());throw std::runtime_error(label+" source order at="+std::to_string(delta.first-ot.sites.begin())+" old="+(delta.first==ot.sites.end()?"end":hex_address(*delta.first))+" new="+(delta.second==nt.sites.end()?"end":hex_address(*delta.second)));}
        require(old.calls==modern.calls&&old.cursor==modern.cursor&&old.released==modern.released&&old.allocations==modern.allocations&&old.closed==modern.closed,label+" dependency ABI and file/ownership outcomes");require(x.effects.bytes==y.effects.bytes&&x.pcs==y.pcs,label+" ordered timed effects");require(a->executed==b->executed,label+" instruction accounting");Image(*a).compare(*b,label);
    } catch(const std::runtime_error&) {if(!negative)throw;rejected=true;}
    require(rejected==negative,label+" discriminating negative");const auto n=index(input.pc);++cases;returns+=expected.empty()&&!negative;root_returns[n]+=expected.empty()&&!negative;stops+=!expected.empty();root_stops[n]+=!expected.empty();negatives+=negative;root_negatives[n]+=negative;
    if(!negative)for(auto pc:ot.sites)if(lookup::entries[n]<=pc&&pc<ends[n]){all_sites.insert(pc);if(expected.empty()){complete_sites.insert(pc);root_complete_sites[n].insert(pc);}}
    return {std::move(a),x.effects.log,unsigned(old.calls.size()),expected,std::move(old.calls)};
}
void boundaries(const State& input,Options options={}) {
    const auto base=compare(input,options);require(base.outcome.empty()&&base.state->pc==return_sentinel,"Complete lookup boundary baseline");const auto steps=base.state->executed-input.executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto s=clone(input);s->budget=s->executed+cut;compare(*s,options);}
    std::set<std::uint32_t> bytes;for(const auto& access:base.accesses)if(access.size){bytes.insert(access.address);bytes.insert(access.address+std::uint32_t(access.size)-1);}
    std::set<std::uint32_t> holes;for(unsigned n=0;n<7;++n){auto at=bytes.begin();std::advance(at,std::ptrdiff_t(n*(bytes.size()-1)/6));holes.insert(*at);}
    for(auto hole:holes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;s->budget=input.budget;s->executed=input.executed;for(auto [at,size]:input.memory.mapped_ranges())for(auto [address,n]:hole<at||std::uint64_t(hole)>=std::uint64_t(at)+size?std::vector<std::pair<std::uint32_t,std::uint64_t>>{{at,size}}:std::vector<std::pair<std::uint32_t,std::uint64_t>>{{at,hole-at},{hole+1,std::uint64_t(at)+size-hole-1}})if(n){s->memory.map(address,n);s->memory.copy_in(address,input.memory.copy_out(address,n));}require(compare(*s,options).outcome.starts_with("Unmapped data access at "),"Missing required lookup byte must fault");}
    for(unsigned cut:std::set<unsigned>{1,base.calls/2,base.calls})if(cut&&cut<=base.calls){Options o=options;o.interrupt=cut;compare(input,o);}
}

void corpus(bool probe){
    for(unsigned n=0;n<3;++n){auto s=seed(n);compare(*s);if(!probe){Options o;o.own_negative=true;compare(*s,o);o.own_negative=false;o.effect_negative=true;compare(*s,o);boundaries(*s);}}
    if(probe)return;
    for(unsigned language=0;language<256;++language){auto s=seed(1,{},language);const auto result=compare(*s);require(result.state->r<4>(12)==language,"Current language returns the unsigned live byte");}
    for(bool old:{false,true})for(bool want:{false,true})for(unsigned row=0;row<3;++row){Options o;o.old_strings=old;o.want_index=want;o.hit_row=row;const auto result=compare(*seed(2,o),o);require(result.state->r<4>(12)==1,"Cached query hit returns true");require(text(*result.state,result.state->memory.read<4>(destination))=="Hello","Cached source string copied");require(text(*result.state,result.state->memory.read<4>(destination+4))=="cached"+std::to_string(row)+"-1","Cached secondary string copied");for(unsigned word=0;word<10;++word)require(result.state->memory.read<4>(destination+12+word*4)==0x89120000+row*256+word,"All cached metadata words copied");require(result.state->memory.read<4>(result_index)==(want?row:0x87654321),"Optional cached index uses original row quotient");}
    for(auto key:{"Hello"," H e l l o ","{Hello}","{} Hello {}","H\\ello",R"(H\nello)","H\nello",R"(H\"ello)","H\"ello",""," ","{}","missing"})for(bool hit:{false,true}){Options o;o.key=key;o.hit=hit;compare(*seed(2,o),o);}
    for(auto byte:{0u,1u,2u,127u,128u,255u}){Options o;auto s=seed(2,o);s->memory.write<1>(query,byte);s->memory.write<1>(query+1,0);compare(*s,o);}
    for(unsigned rows:{0u,1u,3u}){Options o;o.rows=rows;o.hit=false;const auto result=compare(*seed(2,o),o);require(result.state->r<4>(12)==0,"Absent cached query returns false");require(result.state->memory.read<4>(destination)==fixture+0xc000&&result.state->memory.read<4>(result_index)==0x87654321,"Cache miss preserves destination and optional index");}
    for(bool hit:{false,true})for(bool old:{false,true})for(unsigned col:{1u,2u,3u})for(unsigned row=0;row<3;++row){Options o;o.cached=false;o.hit=hit;o.old_strings=old;o.column=col;o.hit_row=row;const auto result=compare(*seed(2,o),o);require(result.state->r<4>(12)==unsigned(hit),"Disk lookup hit policy");require(result.state->memory.read<4>(result_index)==0x87654321,"File path retains optional index");if(hit){require(text(*result.state,result.state->memory.read<4>(destination))==o.file_key,"File source string stays unnormalized in result");require(text(*result.state,result.state->memory.read<4>(destination+4))=="descriptor"+std::to_string(row),"Descriptor copied from disk field");require(text(*result.state,result.state->memory.read<4>(destination+8))=="translation"+std::to_string(row)+"-"+std::to_string(col),"Selected language field copied");for(unsigned word=0;word<10;++word)require(result.state->memory.read<4>(destination+12+word*4)==0x12340000+row*0x1000+col*0x100+word,"Little-endian metadata restored as guest word");}}
    for(auto query_text:{"Hello"," H e l l o ",R"(H\nello)","H\nello",R"(H\"ello)","H\"ello","","{}","\x80"})for(auto source_text:{"Hello","H e l l o",R"(H\nello)","H\nello",R"(H\"ello)","H\"ello","","\x80"}){Options o;o.cached=false;o.key=query_text;o.file_key=source_text;compare(*seed(2,o),o);}
    for(unsigned n=0;n<3;++n)for(auto key:{std::string(64,'a'),std::string(0x226-1,'b')}){Options o;o.key=key;o.file_key=key;o.cached=n==0;o.open=n!=2;compare(*seed(2,o),o);}
    {Options o;o.cached=false;o.open=false;const auto result=compare(*seed(2,o),o);require(result.state->r<4>(12)==0&&!result.state->memory.read<1>(error_flag),"Open failure clears application status and returns false");require(!text(*result.state,result.state->memory.read<4>(toc-0x3e8c)).empty(),"Open failure records its original error text");}
    for(unsigned rows:{0u,1u}){Options o;o.cached=false;o.rows=rows;o.hit=false;compare(*seed(2,o),o);}
    for(unsigned cut:{1u,7u,64u}){Options o;o.cached=false;o.short_read=cut;o.hit=false;compare(*seed(2,o),o);}
    for(unsigned variation=0;variation<6;++variation){Options o;o.cached=variation<3;o.redirect_output=variation==0;o.redirect_catalog=variation==1;o.dirty_cleanup=variation==2;o.redirect_column=variation==3;o.redirect_count=variation==4;o.trim_key=variation==5;compare(*seed(2,o),o);}

    for(const auto& [raw,normalized]:std::vector<std::pair<std::string,std::string>>{
            {" H e l l o ","Hello"},{"{Hello}","Hello"},{R"(H\nello)","H\nello"},
            {R"(H\"ello)","H\"ello"},{R"(H\qello)",R"(H\qello)"},{"{}",""}}){
        Options o;o.key=raw;const auto result=compare(*seed(2,o),o);
        require(text(*result.state,stack-0x510+0x2bc)==normalized,"Independent query normalization contract");
    }
    for(unsigned n=0;n<3;++n)for(auto relocated:{toc-0x100000u,toc+0x100000u}){
        const auto result=compare(*seed(n,{},129,relocated));
        require(result.outcome.empty(),"Relocated TOC returns normally");
    }
    for(auto alias:{query,catalog,catalog+52,result_index}){
        Options o;auto s=seed(2,o);s->wr<4>(12,alias);compare(*s,o);
    }
    for(auto alias:{destination,destination+4,catalog+12}){
        Options o;auto s=seed(2,o);s->wr<4>(20,alias);compare(*s,o);
    }
    for(bool cached:{false,true})for(auto shift:{1u,3u}){
        Options o;o.cached=cached;auto s=seed(2,o);s->memory.copy_in(destination+shift,s->memory.copy_out(destination,52));s->wr<4>(12,destination+shift);compare(*s,o);
    }
    for(unsigned n:{0u,2u}){auto s=seed(n);s->wr<4>(12,0);require(compare(*s).outcome.starts_with("Unmapped data access at "),"Null record retains original access fault");}
    {auto s=seed(2);s->wr<4>(16,0);require(compare(*s).outcome.starts_with("Unmapped data access at "),"Null key retains original access fault");}
    for(bool cached:{false,true})for(bool hit:{false,true})for(bool opened:{false,true}){Options o;o.cached=cached;o.hit=hit;o.open=opened;boundaries(*seed(2,o),o);}
}
}
namespace cm::aot {void localization_lookup_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try{
    require(argc==1||argc==2,"Optional lookup coverage output or probe");const bool probe=argc==2&&std::string_view(argv[1])=="--probe";corpus(probe);const auto expected=localization_lookup_expected_sites();
    if(argc==2&&!probe){std::ofstream out(argv[1]);for(auto pc:expected)out<<hex_address(pc)<<" "<<all_sites.contains(pc)<<" "<<complete_sites.contains(pc)<<'\n';}
    if(!probe)for(auto pc:expected)if(!complete_sites.contains(pc))std::cerr<<"Uncovered localization site "<<hex_address(pc)<<std::endl;
    std::cout<<"Localization lookup comparisons="<<cases<<" returns="<<returns<<" stops="<<stops<<" negatives="<<negatives<<" completed_sites="<<complete_sites.size()<<"/"<<expected.size()<<std::endl;
    for(unsigned n=0;n<3;++n)std::cout<<"Root "<<hex_address(lookup::entries[n])<<" returns="<<root_returns[n]<<" stops="<<root_stops[n]<<" negatives="<<root_negatives[n]<<" completed_sites="<<root_complete_sites[n].size()<<std::endl;
    if(!probe){require(complete_sites.size()==expected.size(),"Every lookup source instruction needs a complete-return witness");for(unsigned n=0;n<3;++n){require(root_complete_sites[n].size()==site_counts[n],"Every root needs complete own-source coverage");require(root_negatives[n]==2,"Every root needs result and restored-effect negatives");}}
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
