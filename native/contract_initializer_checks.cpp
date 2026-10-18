#include "contract_storage.hpp"
#include "semantic_io_checks.hpp"
#include <set>

namespace cm::aot {bool contract_storage_reference(State&);void contract_storage_fixture_tables(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace storage=cm::aot::contract_storage;
constexpr std::uint32_t record=fixture+0x1000,staff=fixture+0x2000,profile=fixture+0x3000,club=fixture+0x4000,nation=fixture+0x5000;
constexpr std::uint32_t alternate_club=fixture+0x6000,nonplayer=fixture+0x7000,competition=fixture+0x8000,service=fixture+0x9000,vtable=fixture+0xa000;
constexpr std::uint32_t globals=fixture+0xc000,today=fixture+0x10000,threshold=fixture+0x10100,pool=fixture+0x14000,competition_array=fixture+0x18000;
constexpr std::uint32_t manager=fixture+0x1a000,statistics=fixture+0x28000,allocated_rows=fixture+0x30000,tables=fixture+0x50000;
constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,3> ranges{{{0x10078e70,0x10078ef8},{0x10078f00,0x10079048},{0x103727c0,0x10374570}}};
constexpr std::array nation_slots{-0x7af4,-0x7a28,-0x7a24,-0x7a20,-0x7a1c,-0x7a18,-0x7a14,-0x7a10,-0x7a00,-0x79fc,-0x7650,-0x75a8,-0x7594,-0x7590,-0x757c,-0x7578,-0x7574,-0x7514,-0x7510,-0x750c,-0x7504,-0x6778};
struct Control {std::uint32_t pc;unsigned offset;std::uint64_t bits;unsigned width=4;};
struct Options {
    std::vector<Control> controls;
    unsigned interrupt=0,output_flag=0,virtual_answer=1,seed_return=12345;
    std::uint32_t returned_club=0,contract_value=2000000,variation=7,duration_years=2,duration_days=15;
    double calculated_value=30000;
    bool own_negative=false,child_negative=false,clear_profile_on_seed=false,mutate_contract_on_clause=false;
};
struct Trace final:SemanticGuestObserver {
    Options options;std::vector<std::uint32_t> sites;
    explicit Trace(Options o):options(std::move(o)){}
    void before_step(State& s,std::uint32_t pc)override {
        sites.push_back(pc);for(auto c:options.controls)if(c.pc==pc){if(c.width==1)s.wr<1>(c.offset,c.bits);else if(c.width==8)s.wr<8>(c.offset,c.bits);else s.wr<4>(c.offset,c.bits);}
    }
};
thread_local Trace* original_trace=nullptr;
void modern_root(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    if(s.pc==storage::initializer_entry)storage::initialize_contract(s,d,o);
    else if(s.pc==storage::reserve_entry)storage::reserve_record(s,d,o);
    else {require(s.pc==storage::bind_primary_entry,"Known complete contract storage root");storage::bind_primary(s,d,o);}
}
struct Event {std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Model final:SemanticDependencyDispatch {
    bool original;Options options;Trace& trace;std::vector<Event> calls;
    Model(bool old,Options o,Trace& t):original(old),options(std::move(o)),trace(t){}
    void reference(State& s,std::uint32_t stop=return_sentinel) {
        const auto entry=s.pc;const auto range=std::find_if(ranges.begin(),ranges.end(),[&](auto r){return r.first==entry;});require(range!=ranges.end(),"Known original storage root");
        while(s.pc!=stop){if(s.pc>=entry && s.pc<range->second)require(contract_storage_reference(s),"Complete original storage root");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));}
    }
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override {
        constexpr std::array allowed{storage::reserve_entry,storage::initializer_entry,0x10715880u,0x107156f0u,0x1037f050u,0x100ce320u,0x10084be0u,0x10718130u,0x10717dd0u,0x1037b6f0u,0x10380700u,0x10374b60u,0x10374af0u,0x1077b770u,0x10781320u,0x10374570u,0x10374b40u,0x10374b80u,0x10775130u,0x107751b0u};
        require(std::find(allowed.begin(),allowed.end(),target)!=allowed.end(),"Known initializer dependency "+hex_address(target));
        const auto serial=calls.size();calls.push_back({target,continuation,s.registers,{}});
        try {
            if(target==storage::reserve_entry || target==storage::initializer_entry){if(original)reference(s,continuation);else modern_root(s,this,&trace);}
            else {
                const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};const auto first=arg(3),second=arg(4),third=arg(5);std::uint32_t result=0;
                if(target==0x10715880){s.memory.write<4>(first,0);s.memory.write<4>(first+4,0);}
                if(target==0x107156f0){s.memory.write<2>(first,10);s.memory.write<2>(first+2,arg(6));s.memory.write<1>(first+4,0);}
                if(target==0x1037f050)s.memory.write<4>(first+0x3c,0);
                if(target==0x100ce320){s.memory.write<1>(second,options.output_flag);result=options.returned_club;}
                if(target==0x10084be0)result=options.contract_value;
                if(target==0x10718130)result=17;
                if(target==0x10717dd0){s.memory.write<2>(first,options.duration_days);s.memory.write<2>(first+2,options.duration_years);s.memory.write<1>(first+4,0);}
                if(target==0x10380700)s.memory.write<4>(first+0x24,second);
                if(target==0x10374b60){const auto index=std::uint32_t(signed_value<1>(second));s.memory.write<1>(first+0x1c+index,0);}
                if(target==0x10374af0){const auto index=std::uint32_t(signed_value<1>(second));if(index==1)for(unsigned n=0;n<5;++n)s.memory.write<1>(first+0x1c+n,0);else s.memory.write<1>(first+0x1d,0);s.memory.write<1>(first+0x1c+index,1);}
                if(target==0x10374b40)s.memory.write<4>(first+0x10+std::uint32_t(signed_value<1>(second))*4,third);
                if(target==0x10374b80)s.memory.write<4>(first+0x10+std::uint32_t(signed_value<1>(second))*4,0xffffffff);
                if(target==0x1077b770){result=options.seed_return;if(options.clear_profile_on_seed)s.memory.write<4>(staff+0x14,0);}
                if(target==0x10374570){result=options.variation;s.memory.write<4>(globals+4,s.memory.read<4>(globals+4)+3);}
                if(target==0x10781320){s.memory.write<4>(s.r<4>(4)+0x14,s.r<4>(8));s.wr<4>(8,0x11223344);result=options.virtual_answer;}
                if(target==0x10775130 || target==0x107751b0){result=allocated_rows;if(target==0x107751b0)s.memory.copy_in(result,s.memory.copy_out(first,400));else s.memory.clear(result,400);}
                if(options.mutate_contract_on_clause && target==0x10374b60)s.memory.write<1>(staff+0x62,0);
                for(unsigned n=0;n<=12;++n)if(n!=1 && n!=2)s.wr<4>(n*4,0xdc000000+n*0x101);
                for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-50.0-n));
                s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=continuation;
                if(target==0x1037b6f0)s.wr<8>(0x4010,float_bits<8>(options.calculated_value));
            }
            if(options.child_negative && !original)s.memory.write<1>(globals+0x700,1);
            calls[serial].after=s.registers;if(options.interrupt && calls.size()>=options.interrupt)throw std::runtime_error("Fixture initializer dependency interruption");
        }catch(...){calls[serial].after=s.registers;throw;}
    }
};
std::unique_ptr<State> seed() {
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned n=1280;n<1312;++n)s->wr<1>(n,0);
    s->memory.map(fixture,0x80000);s->memory.map(stack-0x3000,0x4000);s->memory.map(toc-0x8000,0x9000);
    s->pc=storage::initializer_entry;s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->budget=100000;
    s->wr<4>(12,record);s->wr<4>(16,staff);s->wr<4>(20,2000000);
    s->memory.copy_in(record,std::vector<std::uint8_t>(100,0x5a));s->memory.write<4>(staff,0);s->memory.write<4>(staff+4,1000);s->memory.write<4>(staff+12,nation);s->memory.write<4>(staff+0x14,profile);s->memory.write<4>(staff+0x1c,nonplayer);s->memory.write<4>(staff+0x20,club);s->memory.write<4>(staff+0x24,club);s->memory.write<2>(staff+0x5c,30);
    for(unsigned n=0x62;n<=0x67;++n)s->memory.write<1>(staff+n,10);s->memory.write<1>(staff+0x62,11);
    s->memory.write<2>(profile+10,10000);s->memory.write<2>(profile+12,5000);for(unsigned n=15;n<=0x15;++n)s->memory.write<1>(profile+n,20);s->memory.write<2>(nonplayer+0x2a,8000);
    for(auto c:{club,alternate_club}){s->memory.write<4>(c,7);s->memory.write<4>(c+8,20000);s->memory.write<4>(c+0x2c,nation);s->memory.write<4>(c+0x30,competition);s->memory.write<2>(c+0x1ec,1000);s->memory.write<1>(c+0x23d,1);}
    s->memory.write<4>(nation+0x68,999);s->memory.write<1>(nation+0x11f,10);s->memory.write<4>(competition,0);s->memory.write<4>(competition_array,service);s->memory.write<4>(service,vtable);s->memory.write<1>(service+0x43,2);s->memory.write<4>(vtable+0xa0,fixture+0xb000);s->memory.write<4>(vtable+0xa4,fixture+0xb008);
    for(auto [slot,address]:{std::pair{-0x7b5c,globals},std::pair{-0x7b90,globals+4},std::pair{-0x7b8c,pool},std::pair{-0x7b88,globals+8},std::pair{-0x7b84,globals+12},std::pair{-0x7a9c,threshold},std::pair{-0x7b80,today},std::pair{-0x7b7c,globals+16},std::pair{-0x7b9c,globals+20}})s->memory.write<4>(toc+slot,address);
    s->memory.write<4>(globals,competition_array);s->memory.write<4>(globals+4,0);s->memory.write<4>(globals+8,pool+256);s->memory.write<4>(globals+12,pool);s->memory.write<4>(globals+16,manager);s->memory.write<4>(globals+20,statistics);
    s->memory.write<4>(manager,allocated_rows);s->memory.write<4>(manager+4,4);s->memory.write<4>(manager+8,0);s->memory.write<4>(statistics,-1);
    s->memory.write<2>(today,20);s->memory.write<2>(today+2,2001);s->memory.write<2>(threshold,10);s->memory.write<2>(threshold+2,2000);
    s->memory.write<2>(staff+0x4c,10);s->memory.write<2>(staff+0x4e,1999);s->memory.write<2>(staff+0x54,10);s->memory.write<2>(staff+0x56,2004);
    for(unsigned n=0;n<128;++n)s->memory.write<2>(pool+n*2,(12345+n*1009)&65535);
    for(unsigned n=0;n<nation_slots.size();++n){s->memory.write<4>(toc+nation_slots[n],globals+0x100+n*4);s->memory.write<4>(globals+0x100+n*4,100+n);}
    contract_storage_fixture_tables(*s,toc,tables);return s;
}
unsigned cases=0,completed=0,interrupted=0,negatives=0,structural=0;std::set<std::uint32_t> sites;
struct Result {std::unique_ptr<State> state;std::vector<Effects::Access> accesses;std::vector<Event> calls;std::vector<std::uint32_t> trace;};
Result compare(const State& input,Options options={}) {
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;Trace ot(options),nt(options);Model old(true,options,ot),modern(false,options,nt);
    const auto run=[&](State& s,Model& m,bool reference){try{if(reference){original_trace=&ot;m.reference(s);}else modern_root(s,&m,&nt);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="Contract initializer case="+std::to_string(cases);
    require(expected.empty() || expected=="Native instruction budget exhausted" || expected=="Fixture initializer dependency interruption" || expected.starts_with("Unmapped data access at "),label+" unexpected fixture failure: "+expected);
    if(expected!=actual || ot.sites!=nt.sites){for(unsigned n=0;n<std::min(ot.sites.size(),nt.sites.size());++n)if(ot.sites[n]!=nt.sites[n]){std::cerr<<"First initializer site "<<n<<" "<<hex_address(ot.sites[n])<<"/"<<hex_address(nt.sites[n])<<std::endl;break;}}
    require(expected==actual,label+" original="+expected+" modern="+actual);if(options.own_negative)b->wr<4>(12,b->r<4>(12)^1);
    bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative&&!options.child_negative)throw;rejected=true;}
    require(rejected==(options.own_negative||options.child_negative),label+" discriminating negative");
    if(!rejected){require(x.bytes==y.bytes,label+" ordered effects");require(old.calls==modern.calls,label+" dependency ABI");require(ot.sites==nt.sites,label+" source order");require(a->executed==b->executed,label+" accounting");}
    ++cases;completed+=expected.empty()&&!rejected;interrupted+=!expected.empty();negatives+=rejected;structural+=!options.controls.empty();if(expected.empty())sites.insert(ot.sites.begin(),ot.sites.end());return{std::move(a),x.log,old.calls,ot.sites};
}
void boundaries(const State& input,Options options={}) {
    const auto base=compare(input,options);require(base.state->pc==return_sentinel,"Completed initializer boundary fixture");const auto steps=base.state->executed-input.executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto s=clone(input);s->budget=s->executed+cut;compare(*s,options);}
    std::set<std::uint32_t> bytes;for(auto a:base.accesses)if(a.size){bytes.insert(a.address);bytes.insert(a.address+std::uint32_t(a.size)-1);}
    std::set<std::uint32_t> holes;if(!bytes.empty())for(unsigned n=0;n<6;++n){auto it=bytes.begin();std::advance(it,std::ptrdiff_t(n*(bytes.size()-1)/5));holes.insert(*it);}
    for(auto hole:holes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;s->budget=input.budget;for(auto [start,size]:input.memory.mapped_ranges()){if(hole<start||std::uint64_t(hole)>=std::uint64_t(start)+size){s->memory.map(start,size);s->memory.copy_in(start,input.memory.copy_out(start,size));continue;}for(auto [address,count]:{std::pair{start,std::uint64_t(hole-start)},std::pair{hole+1,std::uint64_t(start)+size-hole-1}})if(count){s->memory.map(address,count);s->memory.copy_in(address,input.memory.copy_out(address,count));}}compare(*s,options);}
    for(unsigned n=1;n<=base.calls.size();++n){auto o=options;o.interrupt=n;compare(input,o);}
}
void basic_cases() {
    boundaries(*seed());
    for(auto value:{0u,1u,1000u,150000u,150001u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto wage:{0u,1000u,50000u})for(auto mode:{1u,2u,3u}){auto s=seed();s->wr<4>(20,value);s->memory.write<4>(staff+4,wage);s->memory.write<1>(club+0x23d,mode);compare(*s);}
    for(auto club_pointer:{0u,club})for(auto player_pointer:{0u,profile})for(auto nonplayer_pointer:{0u,nonplayer})for(auto other_club:{0u,club}){auto s=seed();s->memory.write<4>(staff+0x24,club_pointer);s->memory.write<4>(staff+0x14,player_pointer);s->memory.write<4>(staff+0x1c,nonplayer_pointer);s->memory.write<4>(staff+0x20,other_club);compare(*s);}
    for(auto returned:{0u,alternate_club})for(auto flag:{0u,1u,128u,255u,256u}){Options o;o.returned_club=returned;o.output_flag=flag;boundaries(*seed(),o);}
    for(auto age:{0u,23u,24u,29u,30u,32u,33u,127u,128u,255u,256u})for(auto years:{0u,1u,2u,3u,4u}){auto s=seed();s->memory.write<2>(staff+0x5c,age);Options o;o.duration_years=years;compare(*s,o);}
    for(auto reputation:{0u,999u,1000u,10000u,32767u,32768u,65535u})for(auto extra:{0u,1u,255u}){auto s=seed();s->memory.write<2>(club+0x1ec,reputation);s->memory.write<1>(staff+0x5e,extra);compare(*s);}
}
Control gpr(std::uint32_t pc,unsigned reg,std::uint32_t bits){return {pc,reg*4,bits,4};}
Control fpr(std::uint32_t pc,unsigned reg,double value){return {pc,0x4000+reg*16,float_bits<8>(value),8};}
Options country_controls(double value){Options o;o.controls={gpr(0x10373de4,17,0),fpr(0x10373de4,31,value)};return o;}
void country_cases() {
    // This phase's two earlier gates are correlated. Explicit register controls
    // exercise the preserved body without claiming natural career traversal.
    for(unsigned n=0;n<=nation_slots.size();++n)for(auto mode:{1u,2u})for(auto value:{-1.0,0.0,1.0,300.0,3000.0,30000.0,1e9,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}) {
        auto s=seed();s->memory.write<4>(nation+0x68,n==nation_slots.size()?999:100+n);s->memory.write<1>(club+0x23d,mode);compare(*s,country_controls(value));
    }
    for(auto reputation:{0u,14999u,15000u,27499u,27500u,0xffffffffu})for(auto value:{1.0,1e6}){auto s=seed();s->memory.write<4>(nation+0x68,104);s->memory.write<4>(club+8,reputation);compare(*s,country_controls(value));}
    for(auto club_pointer:{0u,club})for(auto value:{1.0,1e6}){auto s=seed();s->memory.write<4>(club+0x2c,0);auto o=country_controls(value);if(!club_pointer)o.controls.push_back(gpr(0x10373de4,28,0));boundaries(*s,o);}
    for(auto age:{23u,24u,31u,32u})for(auto ability:{3749u,3750u})for(auto local:{false,true})for(auto special:{false,true})for(auto value:{1.0,1e6}) {
        auto s=seed();s->memory.write<2>(staff+0x5c,age);s->memory.write<2>(profile+12,ability);s->memory.write<4>(staff+12,local?nation:0);s->memory.write<4>(competition,special?121:0);
        auto o=country_controls(value);o.controls.push_back(gpr(0x10374344,0,103));compare(*s,o);
    }
}
void date_cases() {
    for(auto threshold_year:{2000u,2001u,2002u})for(auto threshold_day:{19u,20u,21u})for(auto start_year:{2000u,2001u,2002u})for(auto start_day:{19u,20u,21u})for(auto years:{2u,3u}) {
        auto s=seed();s->wr<4>(20,0xffffffff);s->memory.write<2>(threshold,threshold_day);s->memory.write<2>(threshold+2,threshold_year);s->memory.write<2>(staff+0x4c,start_day);s->memory.write<2>(staff+0x4e,start_year);Options o;o.duration_years=years;compare(*s,o);
    }
    for(auto wage:{21249u,21250u,21251u,32499u,32500u,32501u})for(auto age:{23u,24u,29u,30u})for(auto role:{0u,20u}){auto s=seed();s->memory.write<4>(staff+4,wage);s->memory.write<2>(staff+0x5c,age);s->memory.write<1>(profile+0x11,role);Options o;o.duration_years=4;compare(*s,o);}
    {auto s=seed();s->wr<4>(20,0xffffffff);s->memory.write<4>(staff+0x24,0);s->memory.write<2>(threshold+2,2002);boundaries(*s);}
}
void option_cases() {
    for(auto ability:{0u,14u,15u,19u,20u,127u,128u,255u})for(auto calculated:{0.0,1.0,-1.0,1e7,-1e7,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()}) {
        auto s=seed();s->memory.write<4>(staff+4,0);for(auto off:{15u,0x13u,0x14u,0x15u})s->memory.write<1>(profile+off,ability);Options o;o.calculated_value=calculated;compare(*s,o);
    }
    for(auto first:{0u,11u,255u})for(auto second:{0u,4u,5u,14u,15u,255u}){auto s=seed();s->memory.write<1>(staff+0x63,first);s->memory.write<1>(staff+0x65,second);compare(*s);}
    for(auto reputation:{0u,1000u,10000u}){auto s=seed();s->memory.write<1>(club+0x23d,2);s->memory.write<2>(club+0x1ec,reputation);compare(*s);}
    {auto s=seed();s->memory.write<4>(staff+0x24,0);s->memory.write<2>(club+0x1ec,0);compare(*s);}
    {auto s=seed();s->memory.write<1>(staff+0x5e,1);s->memory.write<4>(staff+12,0);compare(*s);}
    {auto s=seed();s->memory.write<4>(staff+0x24,0);Options o;o.controls={gpr(0x103729d4,3,0)};compare(*s,o);}
    for(auto answer:{0u,1u,128u,255u,256u}){Options o;o.virtual_answer=answer;boundaries(*seed(),o);}
    for(auto off:{0x30u,0x43u}){auto s=seed();if(off==0x30)s->memory.write<4>(club+off,0);else s->memory.write<1>(service+off,0);compare(*s);}
    {auto s=seed();s->memory.write<4>(competition_array,0);compare(*s);}
    // Exercise each original wrap/reseed site, including later bonus draws.
    for(auto pc:{0x10372f20u,0x1037302cu,0x103731ccu,0x10373274u,0x103734c4u,0x103735d4u,0x10373864u,0x10373a3cu,0x10373bf0u}) {
        Options o;o.controls={gpr(pc,3,pool+256)};boundaries(*seed(),o);
    }
    constexpr std::array samplers{std::pair{0x1037304cu,14u},std::pair{0x10373294u,14u},std::pair{0x103735f4u,13u},std::pair{0x10373884u,13u},std::pair{0x10373a5cu,13u},std::pair{0x10373c10u,13u}};
    for(auto [pc,reg]:samplers)for(auto bound:{1u,7u,65535u,65536u,0xffffffffu,0xffff0001u,0xffff0000u,0x80000000u,0x7fffffffu})for(auto variation:{0u,10000000u,0xffffffffu}) {
        Options o;o.controls={gpr(pc,reg,bound)};o.variation=variation;compare(*seed(),o);
    }
    for(auto pc:{0x10373010u,0x1037325cu,0x103735b4u})for(auto value:{0u,1u,3u,4u,0xffffffffu,0xfffffffdu,0x80000000u,0x7fffffffu}) {
        Options o;o.controls={gpr(pc,24,value)};compare(*seed(),o);
    }
}
void seam_and_negative_cases() {
    for(auto count:{0u,2u,4u})for(auto value:{1000u,0xffffffffu}) {
        auto s=seed();s->pc=storage::bind_primary_entry;s->wr<4>(12,manager);s->wr<4>(16,staff);s->wr<4>(20,value);s->memory.write<4>(manager+8,count);const auto result=compare(*s);
        require(result.state->memory.read<4>(statistics)==count,"Independent primary contract index");
        require(result.state->memory.read<4>(manager+8)==count+1,"Independent contract used count");
        require(result.state->memory.read<4>(allocated_rows+count*100)==0,"Independent bound staff ID");
        require(result.state->memory.read<4>(allocated_rows+count*100+4)==7,"Independent initialized contract club");
        boundaries(*s);
    }
    for(auto child:{false,true}){Options o;if(child)o.child_negative=true;else o.own_negative=true;compare(*seed(),o);auto s=seed();s->pc=storage::bind_primary_entry;s->wr<4>(12,manager);compare(*s,o);}
    for(auto mutation:{false,true}){Options o;o.mutate_contract_on_clause=mutation;o.clear_profile_on_seed=mutation;o.controls={gpr(0x10372f20,3,pool+256)};compare(*seed(),o);}
    for(auto high:{0u,0xf0u})for(auto so:{0u,1u})for(auto pc:{0x10373014u,0x10373260u,0x103735bcu}) {
        Options o;o.controls={{pc,2304,high,1},{pc,1024,so,1},gpr(pc,0,0xffffffff)};boundaries(*seed(),o);
    }
    for(auto argument:{12u,16u}){auto s=seed();s->wr<4>(argument,0);compare(*s);}
}
void independent_expectations() {
    auto s=seed();const auto base=compare(*s);
    require(base.state->memory.read<4>(record+4)==7,"Independent initialized club ID");
    require(base.state->memory.read<4>(record+12)==1000,"Independent retained contract numeric value");
    require(base.state->memory.read<2>(record+0x32)==2004,"Independent retained end year");
    s=seed();s->memory.write<1>(club+0x23d,2);const auto limited=compare(*s);require((limited.state->memory.read<1>(record+0x38)&1)==1,"Independent mode-two flag");
    s=seed();s->memory.write<1>(club+0x23d,3);const auto clear=compare(*s);
    for(unsigned n=0;n<5;++n)require(clear.state->memory.read<1>(record+0x1c+n)==0,"Independent mode-three option clearing");
    for(unsigned n=0;n<3;++n)require(clear.state->memory.read<4>(record+0x10+n*4)==0xffffffff,"Independent mode-three bonus clearing");
    require(clear.state->memory.read<4>(record+0x24)==0,"Independent mode-three release value clearing");
}
}
namespace cm::aot {void contract_storage_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(){try {
    basic_cases();
    date_cases();country_cases();option_cases();seam_and_negative_cases();independent_expectations();
    unsigned initializer_sites=0;
    for(auto pc=storage::initializer_entry;pc<0x10374570;pc+=4){require(sites.contains(pc),"Missing original initializer site "+hex_address(pc));++initializer_sites;}
    require(initializer_sites==1900,"Complete original initializer source coverage");
    std::cout<<"Initializer comparisons="<<cases<<" completions="<<completed<<" interruptions="<<interrupted<<" negatives="<<negatives<<" structural="<<structural<<" initializer-sites="<<initializer_sites<<" linked-helper-sites="<<sites.size()-initializer_sites<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
