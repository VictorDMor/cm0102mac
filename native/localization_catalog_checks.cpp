#include "localization_catalog.hpp"
#include "semantic_io_checks.hpp"
#include <bit>
#include <fstream>
#include <set>

namespace cm::aot {
bool localization_catalog_reference(State&);
std::vector<std::uint32_t> localization_catalog_expected_sites();
void localization_catalog_fixture_tables(State&,std::uint32_t,std::uint32_t);
}
namespace {
using namespace cm::semantic_io_checks;
namespace lifecycle=cm::aot::localization_catalog;
constexpr std::array<std::uint32_t,4> ends{0x10726d04,0x10726d98,0x10726e88,0x10727854};
constexpr std::array<unsigned,4> site_counts{17,34,58,625};
constexpr std::uint32_t query=fixture+0x1000,catalog=fixture+0x2000,
    stream_buffer=fixture+0x5000,error_buffer=fixture+0x8000,error_flag=fixture+0x9000,
    allocations_base=fixture+0x10000,tables=fixture+0x80000;
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{lifecycle::release_catalog,lifecycle::destroy_record,lifecycle::language_availability,lifecycle::initialize_catalog};
unsigned index(std::uint32_t root){const auto i=std::find(lifecycle::entries.begin(),lifecycle::entries.end(),root);require(i!=lifecycle::entries.end(),"Known catalog lifecycle root");return unsigned(i-lifecycle::entries.begin());}
void text(State& s,std::uint32_t at,const std::string& value){s.memory.copy_in(at,std::vector<std::uint8_t>(value.begin(),value.end()));s.memory.write<1>(at+std::uint32_t(value.size()),0);}
std::string text(const State& s,std::uint32_t at){std::string out;for(unsigned i=0;i<2048;++i){const auto c=s.memory.read<1>(at+i);if(!c)return out;out+=char(c);}throw std::runtime_error("Unterminated fixture text");}
struct Options {
    bool cached=true,open=true,old_strings=true,own_negative=false,effect_negative=false;
    bool big_endian=false,fail_array=false,fail_constructor=false,construct_owned=false;
    bool resurrect_catalog=false,redirect_rows=false,redirect_strings=false;
    unsigned interrupt=0,rows=3,version=1,short_read=0,language=2,marker=2,ui=0;
    std::vector<std::uint32_t> ids{0,2,3,4};
    std::string key="{ H e l l o }";
};
struct Trace:SemanticGuestObserver {
    Options options;std::vector<std::uint32_t> sites;bool effect_negative=false,damaged=false;
    void before_step(State& s,std::uint32_t pc)override {
        sites.push_back(pc);
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
    Options options;bool original=false;Trace* trace=nullptr;
    std::vector<Event> calls;std::vector<std::uint8_t> file;
    std::uint32_t cursor=0;unsigned allocations=0;bool closed=false;
    std::vector<std::uint32_t> released;std::vector<std::array<std::uint32_t,4>> progress;
    explicit Model(Options value):options(value){
        const auto word=[&](std::uint32_t value){for(unsigned byte=0;byte<4;++byte)file.push_back(std::uint8_t(value>>((options.big_endian?3-byte:byte)*8)));};
        const auto field=[&](const std::string& value,unsigned size){require(value.size()<size,"Catalog fixture field width");const auto at=file.size();file.resize(at+size);std::copy(value.begin(),value.end(),file.begin()+std::ptrdiff_t(at));};
        word(options.version);word(options.rows);word(std::uint32_t(options.ids.size()));
        for(auto id:options.ids)word(id);
        for(unsigned row=0;row<options.rows;++row){
            field(options.key+std::to_string(row),550);field("descriptor"+std::to_string(row),30);
            for(unsigned col=1;col<options.ids.size();++col){field("translated"+std::to_string(row)+"-"+std::to_string(col),550);for(unsigned i=0;i<10;++i)word(0x12340000u+row*256+col*16+i);}
        }
    }
    void reference(State& s,std::uint32_t stop=return_sentinel){
        const auto n=index(s.pc),root=s.pc;bool initial=true;
        while(s.pc!=stop){
            if(root<=s.pc&&s.pc<ends[n]&&(initial||s.pc!=root))require(localization_catalog_reference(s),"Whole independent catalog reference");
            else call(s,s.pc,std::uint32_t(s.r<4>(4128)));
            initial=false;
        }
    }
    void call(State& s,std::uint32_t target,std::uint32_t stop)override {
        const auto serial=calls.size();calls.push_back({target,stop,s.registers,{}});
        try {
            const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};std::uint32_t result=0;
            // Explicit models isolate this component's child ABI. They do not
            // establish native file, allocator, sort, callback or UI coverage.
            if(target==lifecycle::initialize_catalog_entry){
                if(original)reference(s,stop);else lifecycle::initialize_catalog(s,this,trace);
                calls[serial].after=s.registers;
                if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Localization fixture interruption");
                return;
            }
            if(target==0x1077dbd0)result=std::uint32_t(text(s,arg(3)).size());
            else if(target==0x1077dbf0){const auto value=text(s,arg(4));text(s,arg(3),value);result=arg(3);}
            else if(target==0x107735f0){
                require(arg(3)>0&&arg(3)<=0x1000,"Catalog allocation ABI");
                const auto allocation=allocations++;
                if(options.fail_array&&allocation==0)result=0;
                else {result=allocations_base+allocation*0x1000;s.memory.clear(result,arg(3));}
                if(options.redirect_rows&&allocation==1)s.memory.write<4>(arg(2)-0x3594,1);
            }
            else if(target==0x10773150){
                require(arg(6)==52&&arg(7)==options.rows,"Original array construction width/count");
                if(!options.fail_constructor){result=arg(3)+16;for(unsigned row=0;row<arg(7);++row)for(unsigned field=0;field<3;++field)s.memory.write<4>(result+row*52+field*4,options.construct_owned?fixture+0xc000+field*0x100:0);}
            }
            else if(target==0x10773610||target==0x107735a0){
                released.push_back(arg(3));
                if(options.redirect_strings&&released.size()==1)s.memory.write<4>(destination+4,0);
            }
            else if(target==0x107734a0){require(arg(4)!=0,"Array destructor descriptor remains live");released.push_back(arg(3));}
            else if(target==0x10739de0){result=options.open?fixture+0xb000:0;cursor=0;closed=false;}
            else if(target==0x107464d0){require(arg(3)==fixture+0xb000,"File type query stream");result=options.big_endian?0xf1fffff1u:0;}
            else if(target==0x10775fe0){
                require(arg(6)==fixture+0xb000,"Catalog item-read stream ABI");const auto count=std::uint64_t(arg(4))*arg(5);require(count<=550,"Bounded catalog field read");
                const auto available=cursor<file.size()?file.size()-cursor:0;auto copied=std::min(count,std::uint64_t(available));if(options.short_read)copied=std::min(copied,std::uint64_t(options.short_read));
                if(copied)s.memory.copy_in(arg(3),std::vector<std::uint8_t>(file.begin()+cursor,file.begin()+cursor+std::ptrdiff_t(copied)));cursor+=std::uint32_t(copied);result=arg(4)?std::uint32_t(copied/arg(4)):0;
            }
            else if(target==0x107766f0){require(arg(3)==fixture+0xb000,"Catalog closes its owned stream");closed=true;}
            else if(target==0x10739890){s.memory.write<4>(arg(6),0x12345678);s.memory.write<4>(arg(7),0x9abcdef0);}
            else if(target==0x10768810)result=0x76543210;
            else if(target==0x1077b720){text(s,arg(3),"localization-error");result=18;}
            else if(target==0x10769b60){require(!arg(5),"Catalog error reporting ABI");}
            else if(target==0x10727890){text(s,arg(3),"Preparing language");result=18;}
            else if(target==0x10743120)progress.push_back({arg(3),arg(5),arg(6),arg(7)});
            else if(target==0x10768300){require(arg(5)==52&&arg(6)!=0,"Catalog sort width/comparator ABI");}
            else if(target==0x1076fa20){result=0x18273645;}
            else throw std::runtime_error("Unknown catalog dependency "+hex_address(target));
            if(options.resurrect_catalog&&target==0x1077dbf0&&options.fail_array)s.memory.write<4>(arg(2)-0x3590,catalog);
            for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xdd000000+n*0x101);
            for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(1000.25+n));
            s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=stop;calls[serial].after=s.registers;
            if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Localization fixture interruption");
        }catch(...){calls[serial].after=s.registers;throw;}
    }
};
std::unique_ptr<State> seed(unsigned n,Options options={},std::uint32_t actual_toc=toc){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);s->memory.map(fixture,0x90000);s->memory.map(stack-0x4000,0x5000);s->memory.map(actual_toc-0x8000,0x9000);
    s->pc=lifecycle::entries[n];s->wr<4>(4,stack);s->wr<4>(8,actual_toc);s->wr<4>(12,n==2||n==3?options.language:destination);s->wr<4>(16,options.ui);s->wr<4>(4128,return_sentinel);s->budget=1000000;
    localization_catalog_fixture_tables(*s,actual_toc,tables);
    s->memory.write<4>(actual_toc-0x3ea0,stream_buffer);s->memory.write<4>(actual_toc-0x7ba8,error_buffer);s->memory.write<4>(actual_toc-0x7bac,error_flag);
    s->memory.write<4>(actual_toc-0x3e9c,query);s->memory.write<4>(actual_toc-0x35a4,std::uint32_t(options.ids.size()));
    s->memory.write<4>(actual_toc-0x3590,options.old_strings?catalog:0);s->memory.write<4>(actual_toc-0x3594,options.rows);s->memory.write<4>(actual_toc-0x3598,std::uint32_t(options.ids.size()));
    s->memory.write<4>(actual_toc-0x35a0,1);s->memory.write<1>(actual_toc-0x359c,options.cached);s->memory.write<1>(actual_toc-0x35ac,options.marker);s->memory.write<1>(error_flag,1);
    for(unsigned i=0;i<options.ids.size();++i)s->memory.write<1>(query+i,options.ids[i]);
    for(unsigned field=0;field<3;++field){text(*s,fixture+0xc000+field*0x100,"owned"+std::to_string(field));s->memory.write<4>(destination+field*4,options.old_strings?fixture+0xc000+field*0x100:0);}
    for(unsigned word=0;word<10;++word)s->memory.write<4>(destination+12+word*4,0xfeed0000+word);
    return s;
}
unsigned cases=0,returns=0,stops=0,negatives=0;
std::array<unsigned,4> root_returns{},root_stops{},root_negatives{};
std::array<std::set<std::uint32_t>,4> root_complete_sites;
std::set<std::uint32_t> complete_sites,all_sites;
struct Result {std::unique_ptr<State> state;std::vector<Effects::Access> accesses;unsigned calls;std::string outcome;std::vector<Event> dependencies;};
Result compare(const State& input,Options options={}) {
    auto a=clone(input),b=clone(input);TimedEffects x(*a),y(*b);Trace ot,nt;ot.options=nt.options=options;nt.effect_negative=options.effect_negative;Model old(options),modern(options);old.original=true;old.trace=&ot;modern.trace=&nt;a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto run=[&](State& s,Model& d,bool reference){try{if(reference){original_trace=&ot;d.reference(s);}else methods[index(s.pc)](s,&d,&nt);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="Localization catalog root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Localization fixture interruption"||expected.starts_with("Unmapped data access at "),label+" unexpected reference: "+expected);require(expected==actual,label+" original="+expected+" modern="+actual);
    const auto negative=options.own_negative||options.effect_negative;
    if(negative)require(expected.empty(),label+" negative needs a complete return");
    if(options.own_negative){if(input.pc==lifecycle::destroy_record_entry)b->memory.write<1>(destination,b->memory.read<1>(destination)^1);else b->wr<4>(12,b->r<4>(12)^1);}
    if(options.effect_negative){require(nt.damaged,label+" effect negative executed");Image(*a).compare(*b,label+" restored effect image");}
    bool rejected=false;
    try {
        if(ot.sites!=nt.sites){const auto delta=std::mismatch(ot.sites.begin(),ot.sites.end(),nt.sites.begin(),nt.sites.end());throw std::runtime_error(label+" source order at="+std::to_string(delta.first-ot.sites.begin())+" old="+(delta.first==ot.sites.end()?"end":hex_address(*delta.first))+" new="+(delta.second==nt.sites.end()?"end":hex_address(*delta.second)));}
        require(old.calls==modern.calls&&old.cursor==modern.cursor&&old.released==modern.released&&old.allocations==modern.allocations&&old.closed==modern.closed&&old.progress==modern.progress,label+" dependency ABI and file/ownership outcomes");require(x.effects.bytes==y.effects.bytes&&x.pcs==y.pcs,label+" ordered timed effects");require(a->executed==b->executed,label+" instruction accounting");Image(*a).compare(*b,label);
    } catch(const std::runtime_error&) {if(!negative)throw;rejected=true;}
    require(rejected==negative,label+" discriminating negative");const auto n=index(input.pc);++cases;returns+=expected.empty()&&!negative;root_returns[n]+=expected.empty()&&!negative;stops+=!expected.empty();root_stops[n]+=!expected.empty();negatives+=negative;root_negatives[n]+=negative;
    if(!negative)for(auto pc:ot.sites)if(lifecycle::entries[n]<=pc&&pc<ends[n]){all_sites.insert(pc);if(expected.empty()){complete_sites.insert(pc);root_complete_sites[n].insert(pc);}}
    return {std::move(a),x.effects.log,unsigned(old.calls.size()),expected,std::move(old.calls)};
}
void boundaries(const State& input,Options options={}) {
    const auto base=compare(input,options);require(base.outcome.empty()&&base.state->pc==return_sentinel,"Complete catalog boundary baseline");const auto steps=base.state->executed-input.executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto s=clone(input);s->budget=s->executed+cut;compare(*s,options);}
    std::set<std::uint32_t> bytes;for(const auto& access:base.accesses)if(access.size){bytes.insert(access.address);bytes.insert(access.address+std::uint32_t(access.size)-1);}
    std::set<std::uint32_t> holes;for(unsigned n=0;n<7;++n){auto at=bytes.begin();std::advance(at,std::ptrdiff_t(n*(bytes.size()-1)/6));holes.insert(*at);}
    for(auto hole:holes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;s->budget=input.budget;s->executed=input.executed;for(auto [at,size]:input.memory.mapped_ranges())for(auto [address,n]:hole<at||std::uint64_t(hole)>=std::uint64_t(at)+size?std::vector<std::pair<std::uint32_t,std::uint64_t>>{{at,size}}:std::vector<std::pair<std::uint32_t,std::uint64_t>>{{at,hole-at},{hole+1,std::uint64_t(at)+size-hole-1}})if(n){s->memory.map(address,n);s->memory.copy_in(address,input.memory.copy_out(address,n));}require(compare(*s,options).outcome.starts_with("Unmapped data access at "),"Missing required catalog byte must fault");}
    for(unsigned cut:std::set<unsigned>{1,base.calls/2,base.calls})if(cut&&cut<=base.calls){Options o=options;o.interrupt=cut;compare(input,o);}
}

void corpus(bool probe){
    for(unsigned n=0;n<4;++n){auto s=seed(n);compare(*s);if(!probe){Options o;o.own_negative=true;compare(*s,o);o.own_negative=false;o.effect_negative=true;compare(*s,o);boundaries(*s);}}
    if(probe)return;
    for(bool owned:{false,true})for(unsigned flag:{0u,1u,0x7fffu,0x8000u,0xffffu,0x10000u,0x10001u,0x7fffffffu,0x80000001u,0xffffffffu}){
        Options o;o.old_strings=owned;auto s=seed(1,o);s->wr<4>(16,flag);const auto out=compare(*s,o);
        require(out.state->r<4>(12)==destination,"Destructor returns original record pointer");
        require(out.calls==unsigned(owned?3:0)+unsigned(std::int16_t(flag&0xffff)>0),"Only signed low halfword controls record deletion");
        for(unsigned word=0;word<13;++word)require(out.state->memory.read<4>(destination+word*4)==s->memory.read<4>(destination+word*4),"Destructor retains released pointer and metadata words");
    }
    for(unsigned mask=0;mask<8;++mask){auto s=seed(1);for(unsigned field=0;field<3;++field)if(!(mask&(1u<<field)))s->memory.write<4>(destination+field*4,0);const auto out=compare(*s);require(out.calls==unsigned(std::popcount(mask)),"Every independent owned field is released");}
    for(unsigned flag:{0u,1u,0xffffffffu}){auto s=seed(1);s->wr<4>(12,0);s->wr<4>(16,flag);require(compare(*s).calls==0,"Null destructor still returns without child calls");}
    for(bool owned:{false,true}){Options o;o.old_strings=owned;const auto out=compare(*seed(0,o),o);require(!out.state->memory.read<4>(toc-0x3590)&&out.state->memory.read<1>(toc-0x35ac)==255,"Release clears catalog and invalidates selected language");require(out.calls==unsigned(owned),"Only nonnull catalog requires array destruction");}
    for(unsigned language=0;language<256;++language){Options o;o.language=language;const auto out=compare(*seed(2,o),o);require(out.state->r<4>(12)==unsigned(language==0||language==2||language==3||language==4),"Availability matches signed byte identifiers");}
    for(unsigned marker:{0u,127u,128u,254u,255u}){Options o;o.marker=marker;const auto out=compare(*seed(2,o),o);require(out.state->r<4>(12)==unsigned(marker!=255),"Only signed -1 marker rejects availability");if(marker==255)require(!out.state->memory.read<1>(error_flag),"Unavailable catalog records application error status");}
    for(unsigned count:{0u,1u,4u,0xffffffffu}){auto s=seed(2);s->memory.write<4>(toc-0x35a4,count);compare(*s);}
    for(bool endian:{false,true})for(unsigned version:{1u,2u})for(bool cached:{false,true})for(unsigned language:{0u,2u,3u,4u,250u})for(unsigned ui:{0u,1u}){
        Options o;o.big_endian=endian;o.version=version;o.cached=cached;o.language=language;o.ui=ui;const auto out=compare(*seed(3,o),o);
        require(out.state->r<4>(12)==1,"Supported catalog initialization returns true");
        const auto selected=language==250?(version==2?2u:0u):(version==2&&language==0?2u:language);
        require(out.state->memory.read<1>(toc-0x35ac)==selected,"Absent or omitted language recursively selects first available ID");
        require(out.state->memory.read<4>(toc-0x35a4)==(version==2?3u:4u),"Version-specific advertised language count");
        const auto base=out.state->memory.read<4>(toc-0x3590);
        require(bool(base)==(cached&&selected!=0),"Only cached non-English language allocates records");
        if(base){for(unsigned row=0;row<o.rows;++row){require(text(*out.state,out.state->memory.read<4>(base+row*52))=="Hello"+std::to_string(row),"Two-stage source normalization");require(text(*out.state,out.state->memory.read<4>(base+row*52+4))=="descriptor"+std::to_string(row),"Descriptor preserved");const auto column=selected==2?1u:selected==3?2u:3u;require(text(*out.state,out.state->memory.read<4>(base+row*52+8))=="translated"+std::to_string(row)+"-"+std::to_string(column),"Physical selected language column");for(unsigned word=0;word<10;++word)require(out.state->memory.read<4>(base+row*52+12+word*4)==0x12340000u+row*256+column*16+word,"All ten metadata words preserve selected endianness");}}
    }
    for(unsigned variation=0;variation<9;++variation){Options o;o.marker=variation==0?255:2;o.open=variation!=1;o.version=variation==2?9:1;o.fail_array=variation==3||variation==4;o.resurrect_catalog=variation==4;o.fail_constructor=variation==5;o.construct_owned=variation==6;o.redirect_rows=variation==7;o.ui=variation==8;const auto out=compare(*seed(3,o),o);require(out.state->r<4>(12)==unsigned(variation>=6),"Catalog error and allocation result policy");if(variation<6)require(!out.state->memory.read<1>(error_flag),"Catalog errors clear live application status");}
    for(auto ids:std::vector<std::vector<std::uint32_t>>{{},{2},{2,3},{128,255,2},{2,2,3},{0,1,2,3}})for(unsigned version:{1u,2u}){Options o;o.ids=ids;o.version=version;compare(*seed(3,o),o);}
    for(unsigned size:{0u,1u,3u})for(bool endian:{false,true}){Options o;o.rows=size;o.big_endian=endian;compare(*seed(3,o),o);}
    for(auto key:{std::string(""),std::string("{} {}"),std::string("A")+char(92)+"n B"+char(92)+"q C"+char(92)+char(34),std::string("High")+char(128),std::string(500,'a')}){Options o;o.key=key;compare(*seed(3,o),o);}
    for(unsigned limit:{1u,2u,3u}){Options o;o.short_read=limit;compare(*seed(3,o),o);}
    for(unsigned n=0;n<4;++n)for(auto relocated:{toc-0x100000u,toc+0x100000u})compare(*seed(n,{},relocated));
    for(unsigned shift:{1u,3u}){auto s=seed(1);s->memory.copy_in(destination+shift,s->memory.copy_out(destination,52));s->wr<4>(12,destination+shift);compare(*s);}
    {Options o;o.redirect_strings=true;compare(*seed(1,o),o);}
    for(auto alias:{destination,query,catalog}){auto s=seed(1);s->wr<4>(12,alias);compare(*s);}
    for(unsigned n:{0u,2u,3u}){auto s=seed(n);s->wr<4>(8,0);require(compare(*s).outcome.starts_with("Unmapped data access at "),"Missing required TOC retains original fault");}
    {auto s=seed(1);s->wr<4>(8,0);require(compare(*s).outcome.empty(),"Destructor does not read a TOC slot itself");}
    for(bool cached:{false,true})for(bool endian:{false,true}){Options o;o.cached=cached;o.big_endian=endian;o.ui=1;boundaries(*seed(3,o),o);}
    for(unsigned n:{2u,3u}){Options o;o.marker=255;boundaries(*seed(n,o),o);}
    {Options o;o.open=false;boundaries(*seed(3,o),o);}
    {Options o;o.fail_array=true;boundaries(*seed(3,o),o);}
}
}
namespace cm::aot {void localization_catalog_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try{
    require(argc==1||argc==2,"Optional catalog coverage output or probe");const bool probe=argc==2&&std::string_view(argv[1])=="--probe";corpus(probe);const auto expected=localization_catalog_expected_sites();
    if(argc==2&&!probe){std::ofstream out(argv[1]);for(auto pc:expected)out<<hex_address(pc)<<" "<<all_sites.contains(pc)<<" "<<complete_sites.contains(pc)<<'\n';}
    if(!probe)for(auto pc:expected)if(!complete_sites.contains(pc))std::cerr<<"Uncovered localization site "<<hex_address(pc)<<std::endl;
    std::cout<<"Localization catalog comparisons="<<cases<<" returns="<<returns<<" stops="<<stops<<" negatives="<<negatives<<" completed_sites="<<complete_sites.size()<<"/"<<expected.size()<<std::endl;
    for(unsigned n=0;n<4;++n)std::cout<<"Root "<<hex_address(lifecycle::entries[n])<<" returns="<<root_returns[n]<<" stops="<<root_stops[n]<<" negatives="<<root_negatives[n]<<" completed_sites="<<root_complete_sites[n].size()<<std::endl;
    if(!probe){require(complete_sites.size()==expected.size(),"Every catalog source instruction needs a complete-return witness");for(unsigned n=0;n<4;++n){require(root_complete_sites[n].size()==site_counts[n],"Every root needs complete own-source coverage");require(root_negatives[n]==2,"Every root needs result and restored-effect negatives");}}
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
