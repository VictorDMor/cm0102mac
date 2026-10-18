#include "awol_records.hpp"
#include "semantic_io_checks.hpp"
#include "application_exit.hpp"
#include <set>

namespace cm::aot {bool awol_records_reference(State&);void awol_records_fixture_tables(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace awol=cm::aot::awol;
constexpr std::uint32_t manager=fixture+0x1000,staff=fixture+0x2000,flags=fixture+0x3000;
constexpr std::uint32_t lists=fixture+0x4000,rows=fixture+0x8000,objects=fixture+0x10000;
constexpr std::uint32_t stream=fixture+0x18000,path=fixture+0x19000,globals=fixture+0x20000,dynamic=fixture+0x30000,random_words=fixture+0x28000;
constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,14> ranges{{{0x100031b0,0x100032f8},{0x10003300,0x100033b8},{0x100033c0,0x100036dc},{0x100036e0,0x10003dc0},{0x10003dc0,0x10003ec0},{0x10003ec0,0x10003fa0},{0x10003fa0,0x10004068},{0x10006600,0x100066d0},{0x100066d0,0x100067d0},{0x100067d0,0x10006828},{0x10006830,0x10006900},{0x10007100,0x100072e0},{0x10006080,0x100065f8},{0x10004070,0x10006074}}};
struct Control {std::uint32_t pc;unsigned reg=32;std::uint32_t value=0;int cr=-1;};
struct Options {
    unsigned interrupt=0,allocation_fail=0,read_fail=0,write_fail=0,next_fail=0;
    std::uint32_t version=0x3e006,random_seed=0,lookup=objects+0x80,load_result=1,flag_count=3;
    std::array<std::int32_t,3> counts{1,1,1};
    bool open_fail=false,partial_load=false,real_children=false,own_negative=false,child_negative=false;
    bool grow_on_next=false,clear_club_on_message=false;
    std::uint32_t contract=objects+0x500,contract_flags=0;
    bool clear_profile_on_register=false;
    std::vector<Control> controls;
};
struct Trace:SemanticGuestObserver {
    Options options;std::vector<std::uint32_t> sites;
    explicit Trace(Options o):options(o){}
    void before_step(State& s,std::uint32_t pc)override{sites.push_back(pc);for(auto c:options.controls)if(pc==c.pc){if(c.reg<32)s.wr<4>(c.reg*4,c.value);if(c.cr>=0)s.wr<1>(2304,(s.r<1>(2304)&0xf0)|unsigned(c.cr)|(s.r<1>(1024)&1));}}
};
thread_local Trace* original_trace=nullptr;
void modern_root(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
    constexpr Function methods[]{awol::construct_records,awol::destroy_records,awol::load_records,awol::save_records,awol::primary_absence_status,awol::secondary_absence_status,awol::has_pending_record,awol::remove_primary_by_staff,awol::remove_primary_item,awol::remove_secondary_item,awol::remove_secondary_by_staff,awol::notify_absence_change,awol::sample_absence_range,awol::process_daily_absences};
    auto it=std::find(awol::record_entries.begin(),awol::record_entries.end(),s.pc);require(it!=awol::record_entries.end(),"Known AWOL component root");methods[it-awol::record_entries.begin()](s,d,o);
}
struct Event {std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Model:SemanticDependencyDispatch {
    bool original;Options options;Trace& trace;std::vector<Event> calls;std::vector<std::uint8_t> input,output;std::size_t cursor=0;
    unsigned allocations=0,reads=0,writes=0,nexts=0;
    Model(bool old,Options o,Trace& t):original(old),options(o),trace(t) {
        const auto word=[&](std::uint32_t value){for(int shift=24;shift>=0;shift-=8)input.push_back(std::uint8_t(value>>shift));};
        word(o.flag_count);for(unsigned n=0;n<o.flag_count;++n)input.push_back(std::uint8_t(n+1));
        for(unsigned slot=0;slot<3;++slot){word(std::uint32_t(o.counts[slot]));for(int n=0;n<o.counts[slot];++n){word(unsigned(n));for(unsigned k=4;k<12+slot*4;++k)input.push_back(std::uint8_t(slot*40+unsigned(n)*20+k));}}
    }
    void reference(State& s,std::uint32_t stop=return_sentinel) {
        const auto entry=s.pc;const auto range=std::find_if(ranges.begin(),ranges.end(),[&](auto v){return v.first==entry;});require(range!=ranges.end(),"Known original AWOL root");
        bool first=true;while(s.pc!=stop){if(s.pc>=entry && s.pc<range->second && (first||s.pc!=entry))require(awol_records_reference(s),"Complete original AWOL reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));first=false;}
    }
    static std::uint32_t row_base(std::uint32_t object) {return object>=dynamic?fixture+0x40000+(object-dynamic)/0x100*0x1000:rows+(object-lists)/0x100*0x1000;}
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override {
        constexpr std::array allowed{0x1077b770u,0x10738c00u,0x10738c20u,0x10773520u,0x107735a0u,0x10775170u,0x10775200u,0x10775130u,0x10715880u,0x10738d10u,0x10738e00u,0x10738e60u,0x107475b0u,0x10747900u,0x10747a80u,0x10747cc0u,0x10775550u,0x10739890u,0x10768810u,0x10769b60u,0x1077b720u,0x10082f20u,0x102bdf90u,0x102cb4b0u,0x102cc400u,0x102cc8e0u,0x10717a10u,0x10082f90u,0x1037fa70u,0x1037f8a0u,0x10649fb0u,0x10007a50u};
        const bool own=std::find(awol::record_entries.begin(),awol::record_entries.end(),target)!=awol::record_entries.end();
        require(own||std::find(allowed.begin(),allowed.end(),target)!=allowed.end(),"Known AWOL dependency "+hex_address(target));
        const auto serial=calls.size();calls.push_back({target,continuation,s.registers,{}});
        try {
            if(own && options.real_children) {
                if(original)reference(s,continuation);else modern_root(s,this,&trace);
            } else {
                const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};
                const auto first=arg(3),second=arg(4),third=arg(5);std::uint32_t result=0;
                s.memory.write<4>(fixture+0x60000+unsigned(serial)*4,target^first^(options.child_negative&&!original?1u:0u));
                if(target==awol::load_entry){result=options.load_result;if(options.partial_load){s.memory.write<4>(first+12,flags);for(unsigned n=0;n<3;++n)s.memory.write<4>(first+n*4,lists+n*0x100);}}
                if(target==awol::remove_primary_staff_entry || target==awol::remove_secondary_staff_entry){const auto id=std::uint32_t(s.memory.read<4>(second));const auto buffer=std::uint32_t(s.memory.read<4>(first+12));const auto bit=target==awol::remove_primary_staff_entry?1u:2u;s.memory.write<1>(buffer+id,s.memory.read<1>(buffer+id)^bit);}
                if(target==0x10773520 || target==0x10775130 || target==0x10775200){++allocations;result=allocations==options.allocation_fail?0:dynamic+allocations*0x100;if(result && target==0x10775200)s.memory.copy_in(result,std::vector<std::uint8_t>(std::min(first*second,1024u)));}
                if(target==0x10715880){s.memory.write<4>(first,0);s.memory.write<4>(first+4,0);}
                if(target==0x10738c00){(void)s.memory.read<1>(first);const auto data=row_base(first);s.memory.write<2>(first,0);s.memory.write<4>(first+4,data);s.memory.write<4>(first+8,data);s.memory.write<4>(first+12,data);}
                if(target==0x10738d10){const auto count=std::uint16_t(s.memory.read<2>(first));const auto data=std::uint32_t(s.memory.read<4>(first+4));s.memory.copy_in(data+unsigned(count)*32,s.memory.copy_out(second,third));s.memory.write<2>(first,count+1);}
                if(target==0x10738e00){++nexts;const auto data=std::uint32_t(s.memory.read<4>(first+4)),current=std::uint32_t(s.memory.read<4>(first+12));const auto count=signed_value<2>(s.memory.read<2>(first));if(nexts!=options.next_fail && current>=data && (current-data)/32<unsigned(std::max(std::int64_t(0),count))){result=current;s.memory.write<4>(first+12,current+32);}if(options.grow_on_next && nexts==1){s.memory.write<2>(first,2);s.memory.write<4>(data+32,0);}}
                if(target==0x10738e60){const auto data=std::uint32_t(s.memory.read<4>(first+4)),current=std::uint32_t(s.memory.read<4>(first+12));const auto count=std::uint16_t(s.memory.read<2>(first));require(current>data && (current-data)/32<=count,"Valid AWOL fixture removal");const auto index=(current-data)/32-1;for(unsigned n=index+1;n<count;++n)s.memory.copy_in(data+(n-1)*32,s.memory.copy_out(data+n*32,32));s.memory.write<2>(first,count-1);s.memory.write<4>(first+12,current-32);}
                if(target==0x107475b0)result=options.open_fail?0:stream;
                if(target==0x10747900)s.memory.write<4>(first,0);
                if(target==0x10747a80){++reads;const auto size=second*third;const bool failed=reads==options.read_fail;const auto bytes=failed&&size?size-1:size;require(cursor+bytes<=input.size(),"Complete AWOL fixture read bytes");if(bytes)s.memory.copy_in(first,std::vector<std::uint8_t>(input.begin()+std::ptrdiff_t(cursor),input.begin()+std::ptrdiff_t(cursor+bytes)));cursor+=bytes;result=failed?0:third;}
                if(target==0x10747cc0){++writes;const auto size=second*third;const bool failed=writes==options.write_fail;const auto bytes=failed&&size?size-1:size;if(bytes){auto data=s.memory.copy_out(first,bytes);output.insert(output.end(),data.begin(),data.end());}result=failed?0:third;}
                if(target==0x10775550)result=options.version;
                if(target==0x1077b770)result=options.random_seed;
                if(target==0x10739890){s.memory.write<4>(arg(6),0x12345678);s.memory.write<4>(arg(7),0x87654321);}
                if(target==0x1077b720){s.memory.write<4>(first,second);s.memory.write<4>(first+4,third);}
                if(target==0x10082f20)result=options.lookup;
                if(target==0x10082f90)result=options.contract;
                if(target==0x1037fa70)result=options.contract_flags;
                if(target==0x1037f8a0)s.memory.write<1>(first+0x61,second);
                if(target==0x10007a50 && options.clear_profile_on_register)s.memory.write<4>(second+0x14,0);
                if(target==0x10717a10){const auto date=s.memory.copy_out(second,8);s.memory.copy_in(first,date);s.memory.write<2>(first,std::uint16_t(s.memory.read<2>(first)+third));}
                if(target==0x102bdf90){s.memory.write<4>(first,second);for(unsigned n=0;n<9;++n)s.memory.write<4>(first+4+n*4,0);if(options.clear_club_on_message)s.memory.write<4>(staff+0x24,0);}
                if(target==0x102cb4b0)s.memory.write<4>(first+4+second*4,third);
                for(unsigned n=3;n<=12;++n)s.wr<4>(n*4,0xde000000|n*0x101);
                for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-20.0-n));
                s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=continuation;
            }
            calls[serial].after=s.registers;
            if(options.interrupt && calls.size()>=options.interrupt)throw std::runtime_error("Fixture AWOL dependency interruption");
        }catch(...){calls[serial].after=s.registers;throw;}
    }
};
std::unique_ptr<State> seed(unsigned index) {
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned n=1280;n<1312;++n)s->wr<1>(n,0);s->memory.map(fixture,0x80000);s->memory.map(stack-0x5000,0x6000);s->memory.map(toc-0x8000,0x9000);
    s->pc=awol::record_entries[index];s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->budget=100000;s->wr<4>(12,manager);s->wr<4>(16,staff);
    for(auto [offset,pointer]:{std::pair{-0x7b78,globals},std::pair{-0x7b7c,globals+0x10},std::pair{-0x7be8,globals+0x20},std::pair{-0x7be4,globals+0x30},std::pair{-0x7be0,globals+0x40},std::pair{-0x7bac,globals+0x50},std::pair{-0x7ba8,globals+0x100},std::pair{-0x7b74,globals+0x60},std::pair{-0x7bf0,globals+0x70}})s->memory.write<4>(toc+offset,pointer);
    s->memory.write<4>(globals+0x10,objects+0x800);s->memory.write<4>(globals+0x20,objects+0x900);s->memory.write<4>(globals+0x30,3);s->memory.write<4>(globals+0x40,staff);s->memory.write<1>(globals+0x50,1);s->memory.write<4>(globals+0x60,objects+0xa00);s->memory.write<4>(globals+0x70,objects+0xb00);
    s->memory.write<4>(manager+12,flags);s->memory.write<1>(flags,3);s->memory.write<1>(flags+1,2);s->memory.write<1>(flags+2,1);
    for(unsigned n=0;n<3;++n){const auto object=lists+n*0x100,data=rows+n*0x1000;s->memory.write<4>(manager+n*4,object);s->memory.write<2>(object,1);s->memory.write<4>(object+4,data);s->memory.write<4>(object+12,data);s->memory.write<4>(data,0);for(unsigned k=4;k<20;++k)s->memory.write<1>(data+k,n*40+k);}
    s->memory.write<4>(staff,0);s->memory.write<4>(staff+12,objects+0x3000);s->memory.write<4>(staff+0x14,objects);s->memory.write<4>(staff+0x24,objects+0x1000);s->memory.write<4>(objects+0x1000,4);s->memory.write<4>(objects+0x1000+0x2c,objects+0x3000);s->memory.write<4>(objects+0x3000+0x68,6);s->memory.write<2>(objects+12,7000);
    for(unsigned n=0;n<3;++n){s->memory.write<4>(staff+0x28+n*4,objects+0x5000+n*0x100);s->memory.write<4>(objects+0x5000+n*0x100+0x20,n+1);}
    awol_records_fixture_tables(*s,toc,fixture+0x70000);
    s->wr<4>(20,0);s->wr<4>(24,1);s->wr<4>(28,65535);
    if(index==0)s->wr<4>(16,0);if(index==1)s->wr<4>(16,1);if(index==2||index==3)s->wr<4>(16,path);
    if(index==8){s->wr<4>(16,rows);s->memory.write<4>(lists+12,rows+32);}if(index==9){s->wr<4>(16,rows+0x1000);s->memory.write<4>(lists+0x100+12,rows+0x1000+32);}
    if(index==11)s->wr<4>(20,objects+0x1000);
    if(index==12){for(auto [off,pointer]:{std::pair{-0x7b90,globals+0x180},std::pair{-0x7b8c,random_words},std::pair{-0x7b88,globals+0x184},std::pair{-0x7b84,globals+0x188}})s->memory.write<4>(toc+off,pointer);s->memory.write<4>(globals+0x180,0);s->memory.write<4>(globals+0x184,random_words+512);s->memory.write<4>(globals+0x188,random_words);for(unsigned n=0;n<256;++n)s->memory.write<2>(random_words+n*2,12345);s->wr<4>(12,100);}
    return s;
}
unsigned cases=0,completed=0,interrupted=0,negatives=0,structural=0;std::set<std::uint32_t> sites;
struct Result {std::unique_ptr<State> state;std::vector<Effects::Access> accesses;std::vector<Event> calls;std::vector<std::uint8_t> output;};
Result compare(const State& input,Options options={}) {
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;Trace old_trace(options),new_trace(options);Model old(true,options,old_trace),modern(false,options,new_trace);
    const auto run=[&](State& s,Model& model,bool reference){try{if(reference){original_trace=&old_trace;model.reference(s);}else modern_root(s,&model,&new_trace);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="AWOL entry="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected.empty() || expected=="Native instruction budget exhausted" || expected=="Fixture AWOL dependency interruption" || expected.starts_with("Unmapped data access at "),label+" unexpected fixture failure: "+expected);
    if(old_trace.sites!=new_trace.sites){for(unsigned n=0;n<std::min(old_trace.sites.size(),new_trace.sites.size());++n)if(old_trace.sites[n]!=new_trace.sites[n]){std::cerr<<"First AWOL site "<<n<<" "<<hex_address(old_trace.sites[n])<<"/"<<hex_address(new_trace.sites[n])<<std::endl;break;}}
    require(expected==actual,label+" original="+expected+" modern="+actual);if(options.own_negative)b->wr<4>(52,b->r<4>(52)^1);
    bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative&&!options.child_negative)throw;rejected=true;}
    require(rejected==(options.own_negative||options.child_negative),label+" discriminating negative");
    if(!rejected){require(x.bytes==y.bytes,label+" ordered effects");require(a->executed==b->executed,label+" accounting");require(old.calls==modern.calls,label+" dependency ABI");require(old_trace.sites==new_trace.sites,label+" source order");require(old.output==modern.output && old.cursor==modern.cursor,label+" archive IO effects");}
    if(expected.empty())for(auto control:options.controls)require(std::find(old_trace.sites.begin(),old_trace.sites.end(),control.pc)!=old_trace.sites.end(),label+" structural control reached");
    ++cases;completed+=expected.empty()&&!rejected;interrupted+=!expected.empty();negatives+=rejected;structural+=!options.controls.empty();sites.insert(old_trace.sites.begin(),old_trace.sites.end());return{std::move(a),x.log,old.calls,old.output};
}
void boundaries(const State& input,Options options={}) {
    auto base=compare(input,options);require(base.state->pc==return_sentinel,"Complete selected AWOL boundary fixture");const auto steps=base.state->executed-input.executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto s=clone(input);s->budget=s->executed+cut;compare(*s,options);}
    std::set<std::uint32_t> bytes;for(auto access:base.accesses)if(access.size){bytes.insert(access.address);bytes.insert(access.address+std::uint32_t(access.size)-1);}
    std::set<std::uint32_t> holes;if(!bytes.empty())for(unsigned n=0;n<4;++n){auto it=bytes.begin();std::advance(it,std::ptrdiff_t(n*(bytes.size()-1)/3));holes.insert(*it);}
    for(auto hole:holes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;s->executed=input.executed;s->budget=input.budget;s->native_calls=input.native_calls;s->heap_end=input.heap_end;s->memory_error=input.memory_error;s->allocated=input.allocated;s->available=input.available;for(auto [start,size]:input.memory.mapped_ranges()){if(hole<start||std::uint64_t(hole)>=std::uint64_t(start)+size){s->memory.map(start,size);s->memory.copy_in(start,input.memory.copy_out(start,size));continue;}for(auto [address,count]:{std::pair{start,std::uint64_t(hole-start)},std::pair{hole+1,std::uint64_t(start)+size-hole-1}})if(count){s->memory.map(address,count);s->memory.copy_in(address,input.memory.copy_out(address,count));}}compare(*s,options);}
    for(unsigned n=1;n<=base.calls.size();++n){auto o=options;o.interrupt=n;compare(input,o);}
}
void lifecycle_cases() {
    for(unsigned n=0;n<4;++n)boundaries(*seed(n));
    for(unsigned fail=1;fail<=4;++fail){Options o;o.allocation_fail=fail;compare(*seed(0),o);compare(*seed(2),o);}
    for(auto result:{0u,1u,256u,255u}){auto s=seed(0);s->wr<4>(16,path);Options o;o.load_result=result;o.partial_load=true;compare(*s,o);}
    auto linked=seed(0);linked->wr<4>(16,path);Options real;real.real_children=true;compare(*linked,real);real.read_fail=4;compare(*linked,real);
    for(auto flag:{0u,65535u,65536u,65537u,0x8000u}){auto s=seed(1);s->wr<4>(16,flag);compare(*s);}
    for(unsigned index:{0u,1u,2u,3u}){auto s=seed(index);s->wr<4>(12,0);compare(*s);}
    auto empty=seed(1);for(unsigned n=0;n<4;++n)empty->memory.write<4>(manager+n*4,0);compare(*empty);
    for(unsigned index:{2u,3u}){auto s=seed(index);s->wr<4>(16,0);compare(*s);Options o;o.open_fail=true;compare(*seed(index),o);}
    for(unsigned fail=1;fail<=8;++fail){Options o;o.read_fail=fail;compare(*seed(2),o);o.read_fail=0;o.write_fail=fail;compare(*seed(3),o);}
    for(unsigned fail=1;fail<=3;++fail){Options o;o.next_fail=fail;compare(*seed(3),o);}
    for(auto version:{0x3e005u,0x3e006u,0x80000000u}){Options o;o.version=version;compare(*seed(2),o);}
    Options zero;zero.flag_count=0;compare(*seed(2),zero);
    for(unsigned slot=0;slot<3;++slot)for(auto count:{-1,0,2}){Options o;o.counts[slot]=count;compare(*seed(2),o);auto s=seed(3);s->memory.write<2>(lists+slot*0x100,std::uint16_t(count));s->memory.write<4>(rows+slot*0x1000+32,1);compare(*s);}
    auto written=compare(*seed(3));require(written.output.size()==67 && std::equal(written.output.begin(),written.output.begin()+7,std::array<std::uint8_t,7>{0,0,0,3,3,2,1}.begin()),"Independent AWOL archive header, flag bytes and record widths");
    auto created=compare(*seed(0));require(created.state->r<4>(12)==manager && created.state->memory.read<4>(manager+12)!=0,"Independent AWOL constructor result and flag allocation");
}
void query_cases() {
    for(unsigned index=4;index<11;++index)boundaries(*seed(index));
    for(unsigned index=4;index<11;++index){auto s=seed(index);s->wr<4>(16,0);compare(*s);}
    for(unsigned index:{4u,5u,6u})for(auto id:{3u,0x7fffffffu,0xffffffffu}){auto s=seed(index);s->memory.write<4>(staff,id);compare(*s);}
    for(unsigned index:{4u,5u,7u,8u,9u,10u})for(auto flag:{0u,1u,2u,3u,128u,255u}){auto s=seed(index);s->memory.write<1>(flags,flag);compare(*s);}
    for(unsigned index:{4u,5u,8u}){Options o;o.lookup=0;compare(*seed(index),o);o.real_children=true;compare(*seed(index),o);}
    for(auto raw:{1u,255u,256u}){auto s=seed(4);s->wr<4>(20,raw);auto result=compare(*s);require(result.state->r<4>(12)==1,"Independent primary status/raw flag result");}
    for(unsigned index:{6u,7u,10u}){auto s=seed(index);const auto slot=index==6?2u:index==7?0u:1u;s->memory.write<4>(manager+slot*4,0);compare(*s);for(auto count:{0u,65535u,2u}){s=seed(index);s->memory.write<2>(lists+slot*0x100,count);s->memory.write<4>(rows+slot*0x1000,1);s->memory.write<4>(rows+slot*0x1000+32,0);compare(*s);}Options o;o.next_fail=1;compare(*seed(index),o);}
    for(unsigned index:{6u,7u,10u}){auto s=seed(index);const auto slot=index==6?2u:index==7?0u:1u;s->memory.write<4>(rows+slot*0x1000,1);Options o;o.grow_on_next=true;compare(*s,o);}
    for(unsigned index:{4u,5u,6u}){auto s=seed(index);constexpr auto moved=toc+0x10000;s->memory.map(moved-0x8000,0x9000);s->memory.copy_in(moved-0x8000,s->memory.copy_out(toc-0x8000,0x9000));s->wr<4>(8,moved);compare(*s);}
}
void notice_cases() {
    boundaries(*seed(11));
    for(auto off:{0x14u,0x24u}){auto s=seed(11);s->memory.write<4>(staff+off,0);compare(*s);}
    auto missing=seed(11);missing->wr<4>(16,0);compare(*missing);missing=seed(11);missing->wr<4>(20,0);compare(*missing);
    for(auto ability:{7500u,7501u,0x8000u}){auto s=seed(11);s->memory.write<2>(objects+12,ability);compare(*s);}
    auto no_manager=seed(11);no_manager->memory.write<4>(objects+0x1000+0x2c,0);compare(*no_manager);
    auto different=seed(11);different->memory.write<4>(staff+12,objects+0x3100);compare(*different);
    Options changed;changed.clear_club_on_message=true;auto high=seed(11);high->memory.write<2>(objects+12,8000);compare(*high,changed);
    Options forced;forced.controls={{0x100071ac,32,0,2}};compare(*seed(11),forced);
    for(auto byte:{128u,255u,256u})for(auto half:{32768u,65535u,65536u}){auto s=seed(11);s->wr<4>(24,byte);s->wr<4>(28,half);compare(*s);}
}
void sampling_cases() {
    Options actual;actual.real_children=true;
    for(auto bound:{0u,1u,100u,65535u,65536u,65537u,0x7fffffffu,0x80000000u,0xffff0000u,0xffff0001u,0xffffffffu}){auto s=seed(12);s->wr<4>(12,bound);auto r=compare(*s,actual);require(r.state->pc==return_sentinel,"Complete AWOL bounded sample");if(bound<=65535)require(r.state->r<4>(12)==(bound?12345u%bound:0u),"Independent AWOL narrow sample");}
    for(auto bound:{100u,65536u,0xffff0000u})for(auto value:{0u,65535u,65536u,0xffffffffu}){auto s=seed(12);s->wr<4>(12,bound);s->memory.write<4>(globals+0x184,random_words);auto o=actual;o.random_seed=value;compare(*s,o);}
    boundaries(*seed(12),actual);auto wide=seed(12);wide->wr<4>(12,65536);boundaries(*wide,actual);
    auto wrapped=seed(12);wrapped->wr<4>(12,65536);wrapped->memory.write<4>(globals+0x184,random_words);boundaries(*wrapped,actual);
    for(unsigned depth=1;depth<=3;++depth)for(auto value:{65536u,0xffff0000u}){auto s=seed(12);s->wr<4>(12,65536);auto o=actual;for(unsigned n=0;n<depth;++n)o.controls.push_back({std::array{0x101865b8u,0x101866a4u,0x10186790u}[n]+0xffe7fbe0u,22-n,value,4});compare(*s,o);}
    for(unsigned depth=1;depth<=2;++depth){auto s=seed(12);s->wr<4>(12,65536);auto o=actual;o.controls.push_back({0x101865b8u+0xffe7fbe0u,22,depth==1?0x7fffffffu:65536u,4});if(depth==2)o.controls.push_back({0x101866a4u+0xffe7fbe0u,21,0x7fffffffu,4});compare(*s,o);}
    auto nested=actual;nested.controls={{0x101865b8u+0xffe7fbe0u,22,65536,4},{0x101866a4u+0xffe7fbe0u,21,65536,4},{0x10186790u+0xffe7fbe0u,20,65536,4}};compare(*wrapped,nested);
}

std::unique_ptr<State> daily_seed(unsigned slot,unsigned mode=1,std::uint16_t random=0) {
    auto s=seed(13);
    for(auto [off,pointer]:{std::pair{-0x7b90,globals+0x180},std::pair{-0x7b8c,random_words},std::pair{-0x7b88,globals+0x184},std::pair{-0x7b84,globals+0x188},std::pair{-0x7b80,globals+0x190},std::pair{-0x7b94,globals+0x1a0}})s->memory.write<4>(toc+off,pointer);
    s->memory.write<4>(globals+0x180,0);s->memory.write<4>(globals+0x184,random_words+512);s->memory.write<4>(globals+0x188,random_words);s->memory.write<2>(globals+0x190,100);s->memory.write<2>(globals+0x192,2001);s->memory.write<4>(globals+0x1a0,objects+0x600);
    for(unsigned n=0;n<256;++n)s->memory.write<2>(random_words+n*2,random);
    for(unsigned n=0;n<3;++n){s->memory.write<2>(lists+n*0x100,n==slot?1:0);const auto data=rows+n*0x1000;s->memory.copy_in(data,std::vector<std::uint8_t>(20));s->memory.write<2>(data+(n==2?8:4),80);s->memory.write<2>(data+(n==2?10:6),2001);s->memory.write<1>(data+(n==2?16:12),mode);}
    s->memory.write<4>(objects+0x80+4,0);s->memory.write<4>(globals+0x20,objects+0x1000);s->memory.write<1>(objects+0x500+0x60,1);
    for(unsigned n=0;n<4;++n){const auto person=staff+n*0x70,profile=objects+n*0x100;s->memory.write<4>(person,n);s->memory.write<4>(person+0x14,n==3?0:profile);s->memory.write<2>(person+0x5c,n==1?25:20);s->memory.write<1>(person+0x67,n?18:4);s->memory.write<1>(person+0x69,3);s->memory.write<1>(person+0x63,15);s->memory.write<2>(profile+10,6000);s->memory.write<1>(profile+0x44,3);s->memory.write<4>(objects+0x1000+0x98+n*4,person);}
    s->wr<4>(12,manager);return s;
}
void daily_cases() {
    for(unsigned slot=0;slot<3;++slot)for(unsigned mode=0;mode<2;++mode)boundaries(*daily_seed(slot,mode));
    auto empty=daily_seed(3);boundaries(*empty);empty->wr<4>(12,0);compare(*empty);
    for(unsigned slot=0;slot<3;++slot) {
        auto s=daily_seed(slot);s->memory.write<4>(manager+slot*4,0);compare(*s);
        for(auto count:{0u,65535u}){s=daily_seed(slot);s->memory.write<2>(lists+slot*0x100,count);compare(*s);}
        Options fail;fail.next_fail=1;compare(*daily_seed(slot),fail);
        for(auto day:{79u,80u,81u,82u,100u,101u,32768u,65535u}){s=daily_seed(slot);s->memory.write<2>(globals+0x190,day);compare(*s);}
        for(auto year:{2000u,2002u,32768u,65535u}){s=daily_seed(slot);s->memory.write<2>(globals+0x192,year);compare(*s);}
        s=daily_seed(slot);const auto data=rows+slot*0x1000;s->memory.write<2>(lists+slot*0x100,2);s->memory.copy_in(data+32,s->memory.copy_out(data,32));s->memory.write<2>(data+(slot==2?8:4),120);compare(*s);
        Options linked;linked.real_children=true;compare(*daily_seed(slot),linked);
    }
    for(unsigned slot=0;slot<3;++slot)for(unsigned delta=0;delta<3;++delta) {
        auto s=daily_seed(slot);s->memory.write<2>(globals+0x190,80+delta);const auto r=compare(*s);
        const auto target=slot==0?awol::remove_primary_item_entry:slot==1?awol::remove_secondary_item_entry:0x10738e60u;
        const auto removals=std::count_if(r.calls.begin(),r.calls.end(),[&](const auto& c){return c.target==target;});
        require(removals==(delta>(slot==0?0u:1u)?1:0),"Independent primary/adjusted secondary/pending expiry dates");
    }
    for(unsigned slot=0;slot<3;++slot) {
        auto s=daily_seed(slot);const auto data=rows+slot*0x1000;s->memory.write<2>(lists+slot*0x100,2);s->memory.copy_in(data+32,s->memory.copy_out(data,32));Options o;o.real_children=true;const auto r=compare(*s,o);
        require(r.state->memory.read<2>(lists+slot*0x100)==1,"Independent daily review removes at most one expired item per list");
        if(slot<2)require(r.state->memory.read<1>(flags)==(slot==0?2u:1u),"Independent daily expiry clears only its own absence flag");
        s=daily_seed(slot);s->memory.write<2>(data+(slot==2?8:4),120);o.real_children=false;o.grow_on_next=true;compare(*s,o);
    }
    for(unsigned slot:{1u,2u})for(unsigned mode=0;mode<2;++mode) {
        for(auto trait:{0u,1u,2u,15u,20u,127u,128u,255u})for(auto random:{0u,1u,12345u,65535u}) {
            auto s=daily_seed(slot,mode,std::uint16_t(random));s->memory.write<1>(staff+0x67,trait);s->memory.write<1>(staff+0x69,trait);s->memory.write<1>(objects+0x44,trait);compare(*s);
        }
        auto s=daily_seed(slot,mode);s->memory.write<4>(staff+0x14,0);compare(*s);
    }
    for(auto lookup:{0u,objects+0x80})for(auto contract:{0u,objects+0x500})for(auto flags:{0u,2u}){Options o;o.lookup=lookup;o.contract=contract;o.contract_flags=flags;compare(*daily_seed(1,0),o);}
    auto contract=daily_seed(1,0);contract->memory.write<2>(random_words+2,17);const auto changed=compare(*contract);require(changed.state->memory.read<1>(objects+0x500+0x61)==2,"Independent older peer contract flag update");
    for(auto kind:{0u,2u,3u}){auto s=clone(*contract);s->memory.write<1>(objects+0x500+0x60,kind);compare(*s);}
    for(auto ability:{5250u,5251u,32768u})for(auto age:{20u,21u,23u,24u,128u}){auto s=clone(*contract);s->memory.write<2>(objects+0x100+10,ability);s->memory.write<2>(staff+0x70+0x5c,age);compare(*s);}
    Options missing_club;missing_club.lookup=0;
    auto decremented=daily_seed(1,0);decremented->memory.write<2>(random_words+6,1);decremented->memory.write<2>(random_words+8,1);const auto own=compare(*decremented,missing_club);require(own.state->memory.read<1>(staff+0x67)==3 && own.state->memory.read<1>(objects+0x44)==2,"Independent unapproved return nonzero-draw trait decrements");
    auto no_profile=missing_club;no_profile.clear_profile_on_register=true;compare(*daily_seed(1,0),no_profile);no_profile.lookup=objects+0x80;compare(*daily_seed(1,0),no_profile);
    for(unsigned slot:{1u,2u})for(unsigned mode=0;mode<2;++mode){auto s=daily_seed(slot,mode);s->memory.write<4>(globals+0x184,random_words);Options o;o.random_seed=0x80000000;boundaries(*s,o);}
    struct Draw {std::uint32_t pool;unsigned bound,slot,mode;bool cached;};
    constexpr Draw draws[]{{0x10004224,15,1,1,false},{0x10004388,15,1,1,false},{0x100044f8,15,1,1,false},{0x10004674,15,1,1,false},{0x100048c8,16,1,0,false},{0x10004afc,15,1,0,false},{0x10004c80,15,1,0,false},{0x10004e00,15,1,0,false},{0x10004f94,14,1,0,false},{0x1000516c,14,1,0,true},{0x100052e8,14,1,0,false},{0x100055a0,14,2,1,false},{0x10005704,14,2,1,false},{0x10005874,14,2,1,false},{0x100059f0,14,2,1,false},{0x10005b84,15,2,0,false},{0x10005d58,15,2,0,true},{0x10005edc,15,2,0,false}};
    for(auto draw:draws) {
        for(auto bound:{65536u,0xffff0000u}){auto s=daily_seed(draw.slot,draw.mode);Options o;o.controls={{draw.pool+(draw.cached?0x24u:0x28u),draw.bound,bound}};compare(*s,o);}
        // Explicit controls cover the zero guard behind an earlier >1 check.
        auto s=daily_seed(draw.slot,draw.mode);Options o;o.controls={{draw.pool-12,draw.bound,0,2}};compare(*s,o);
    }
    auto moved=daily_seed(1,0);constexpr auto relocated=toc+0x10000;moved->memory.map(relocated-0x8000,0x9000);moved->memory.copy_in(relocated-0x8000,moved->memory.copy_out(toc-0x8000,0x9000));moved->wr<4>(8,relocated);compare(*moved);
    auto negative=daily_seed(1,0);Options o;o.own_negative=true;compare(*negative,o);o.own_negative=false;o.child_negative=true;compare(*negative,o);
}

}
namespace cm::aot {void awol_records_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try {
    const auto mode=argc==2?std::string_view(argv[1]):std::string_view{};
    if(mode.empty()||mode=="--lifecycle")lifecycle_cases();if(mode.empty()||mode=="--queries")query_cases();if(mode.empty()||mode=="--notice")notice_cases();
    if(mode.empty()||mode=="--sampling")sampling_cases();
    if(mode.empty()||mode=="--daily")daily_cases();
    if(mode.empty())for(unsigned index=0;index<13;++index){Options o;o.own_negative=true;compare(*seed(index),o);o.own_negative=false;o.child_negative=true;auto child=seed(index);if(index==12)child->memory.write<4>(globals+0x184,random_words);compare(*child,o);}
    constexpr std::array padding{0x10003540u,0x100035dcu,0x10003694u,0x100039e0u,0x10003b78u,0x10003d10u,0x100040c4u,0x1000413cu,0x1000484cu,0x10005588u,0x10005b5cu};
    unsigned uncovered=0;
    for(auto [start,end]:ranges)if(mode.empty() || (mode=="--daily" && start==awol::daily_entry))for(auto pc=start;pc<end;pc+=4)if(std::find(padding.begin(),padding.end(),pc)==padding.end() && !sites.contains(pc)){++uncovered;std::cerr<<"Uncovered AWOL "<<hex_address(pc)<<std::endl;}
    require(uncovered==0,"Every selected original AWOL source site covered");
    if(mode.empty())require(sites.size()==3635,"Complete prepared AWOL records source coverage");
    std::cout<<"AWOL records comparisons="<<cases<<" completions="<<completed<<" interruptions="<<interrupted<<" negatives="<<negatives<<" structural="<<structural<<" sites="<<sites.size()<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
