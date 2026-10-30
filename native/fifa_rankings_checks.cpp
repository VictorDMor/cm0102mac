// Returned original/C++ comparisons. External boundaries are scripted;
// every owned nested call and comparator executes its real implementation.
#include "fifa_rankings_adapter.hpp"
#include "fifa_rankings_detail.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#if defined(CM_FIFA_RANKINGS_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
#endif
using namespace cm::aot;
using namespace cm::aot::fifa_rankings::detail;
namespace cm::aot { bool fifa_rankings_reference(State&); }
namespace {
constexpr unsigned data=0x60000000,object=data+0x1000,rows=data+0x2000,
    countries=data+0x10000,clubs=data+0x20000,match=data+0x30000,competition=data+0x40000,
    histories=data+0x50000,trace_at=data+0x100000,output=data+0x200000,
    stack=0x71010000,sentinel=0xfffffff0;
constexpr std::array entries{0x104866f0u,0x10486790u,0x10486830u,0x10486e40u,0x10487020u,
    0x10487140u,0x104871e0u,0x10487640u,0x10487970u,0x10487ae0u,0x10487c30u,
    0x10487c90u,0x10487ef0u,0x10487fa0u,0x10487fb0u};
struct Case {
    unsigned entry;
    std::string name="default";
    int countries=3,count=3,capacity=5,allocation=0,io=0,open=1,file=1,storage=1,
        deleting=1,null_manager=0,day=18,month=2,year=2002,stored_month=2,stored_year=2002,
        current_year=2002,reputation=7000,away_reputation=7000,position=10,away_position=40,
        home_goals=2,away_goals=0,penalties=-1,missing=0,same_confederation=1,
        competition_id=100,competition_parent=1,competition_type=0,continental=0,friendly=1,
        venue=1,venue_owner=1,history_year=2002,history_month=2,selected=1,
        first_month=1,first_year=2001,last_month=11,last_year=2003,actual_sort=0,
        history_pattern=0,seed=0,partial_bytes=0,date_day=10;
    unsigned left_bits=0x3f800000,right_bits=0x40000000;
};
Case active{entries[0]};
bool original_mode=false,production_mode=false,ownership_mode=false;
unsigned calls=0,allocations=0,io_calls=0,next_allocation=0,cursor=0;
std::vector<std::uint8_t> stream;
const std::set<unsigned> dependencies{0x10715880,0x10718360,0x107182f0,0x107156b0,0x10717b20,
    0x10775130,0x10775170,0x107751b0,0x10775200,0x107735a0,0x107475b0,0x10747a80,
    0x10747cc0,0x10747900,0x10768300,0x10739890,0x10768810,0x1077b720,0x10769b60,
    0x10451800,0x102aed80,0x10494d20};
void trace(Cpu& c,unsigned target) {
    if (++calls>10000)throw std::runtime_error("fixture call limit");
    auto size=c.u32(trace_at);
    const auto add=[&](unsigned value){c.put32(trace_at+4+size++*4,value);};
    add(target);add(c.lr());
    for(unsigned r=3;r<=10;++r)add(c.r(r));
    for(unsigned r=13;r<32;++r)add(c.r(r));
    for(unsigned f=0;f<8;++f)add(c.cr(f));
    // f1 is the floating argument/result that reaches external services.
    const auto bits=c.s.r<8>(guest::reg::fpr(1));add(bits>>32);add(bits);
    for(unsigned f=14;f<32;++f){const auto value=c.s.r<8>(guest::reg::fpr(f));add(value>>32);add(value);}
    c.put32(trace_at,size);
}
void date(Cpu& c,unsigned at,int day,int year) {
    c.put16(at,unsigned(day));c.put16(at+2,unsigned(year));c.put8(at+4,2);
}
bool probe(State& s) {
    Cpu c(s);const auto target=s.pc;
    if (run_leaf(s))return true;
    if (ownership_mode)s.budget=s.executed;
    if (entries.front()<=target && target<0x10487ff0) {
        if(original_mode)return fifa_rankings_reference(s);
        if(production_mode)return false;
        if(fifa_rankings_lists(target)){fifa_rankings_run_rewrite(s,target);return true;}
        return false;
    }
    if(target==0x10781320) {
        const auto descriptor=c.r(12);c.put32(c.sp()+0x14,c.r(2));c.set(2,c.u32(descriptor+4));s.pc=c.u32(descriptor);return true;
    }
    if (!dependencies.contains(target) || (target==0x10768300 && active.actual_sort))return false;
    trace(c,target);
    if(target==0x10775130 || target==0x10775200 || target==0x107751b0) {
        ++allocations;
        if(int(allocations)==active.allocation)c.set(3,0);
        else {
            const auto size=target==0x10775200?c.r(3)*c.r(4):target==0x107751b0?c.r(4):c.r(3);
            if(size>0x10000)throw std::runtime_error("fixture allocation bound");
            const auto at=next_allocation;next_allocation+=(size+31)&~15u;
            if(target==0x10775200)for(unsigned i=0;i<size;++i)c.put8(at+i,0);
            if(target==0x107751b0)for(unsigned i=0;i<std::min(size,unsigned(active.capacity*6));++i)c.put8(at+i,c.u8(c.r(3)+i));
            c.set(3,at);
        }
    } else if(target==0x107475b0)c.set(3,active.open?data+0x9000:0);
    else if(target==0x10747a80 || target==0x10747cc0) {
        ++io_calls;const auto destination=c.r(3),width=c.r(4),count=c.r(5);
        const auto transferred=int(io_calls)==active.io && count?count-1:count;
        const auto bytes=width*transferred+(int(io_calls)==active.io && active.partial_bytes && width>1?width/2:0);
        if(bytes>0x10000)throw std::runtime_error("fixture IO bound");
        if(target==0x10747a80) {
            if(cursor+bytes>stream.size())throw std::runtime_error("fixture stream exhausted");
            for(unsigned i=0;i<bytes;++i)c.put8(destination+i,stream[cursor+i]);
        } else {
            for(unsigned i=0;i<bytes;++i)c.put8(output+cursor+i,c.u8(destination+i));
            c.put32(output-4,cursor+bytes);
        }
        cursor+=bytes;c.set(3,transferred);
    } else if(target==0x10747900)c.put32(c.r(3),0);
    else if(target==0x10739890){c.put32(c.r(6),0x46494641);c.put32(c.r(7),0x43505000);}
    else if(target==0x10768810)c.set(3,data+0x8000);
    else if(target==0x1077b720)c.put32(c.r(3),c.r(8));
    else if(target==0x10715880)date(c,c.r(3),0,0);
    else if(target==0x107156b0)date(c,c.r(3),std::int16_t(c.r(4)),int(c.r(5)));
    else if(target==0x10718360)c.set(3,active.month);
    else if(target==0x107182f0)c.set(3,active.day);
    else if(target==0x10717b20)date(c,c.r(3),active.date_day,active.stored_year);
    else if(target==0x10451800)c.set(3,active.competition_type);
    else if(target==0x102aed80)c.set(3,active.continental);
    else if(target==0x10494d20)c.set(3,active.friendly);
    else if(target==0x10768300) {
        const auto array=c.r(3),count=c.r(4),width=c.r(5),descriptor=c.r(6),back=c.lr();
        for(unsigned i=1;i<count;++i)for(unsigned k=i;k>0;--k){
            const auto a=array+(k-1)*width,b=array+k*width;
            c.set(3,a);c.set(4,b);c.set(12,descriptor);c.set_lr(0x1000ffe0);
            c.put32(c.sp()+0x14,c.r(2));c.set(2,c.u32(descriptor+4));s.pc=c.u32(descriptor);run_until(s,0x1000ffe0);
            if(std::int32_t(c.r(3))<=0)break;
            const auto first=s.memory.copy_out(a,width),second=s.memory.copy_out(b,width);
            for(unsigned n=0;n<width;++n){c.put8(a+n,second[n]);c.put8(b+n,first[n]);}
        }
        c.set_lr(back);
    }
    s.pc=c.lr();return true;
}
void setup(Cpu& c,const Case& t) {
    c.s.memory.map(data,0x400000);c.s.memory.map(0x71000000,0x20000);
    c.s.memory.copy_in(data,std::vector<std::uint8_t>(0x400000,0xa5));
    c.s.memory.copy_in(0x71000000,std::vector<std::uint8_t>(0x20000,0xa5));
    for(unsigned r=0;r<32;++r)c.set(r,0x51000000+r*0x101);
    c.set(1,stack);c.set(2,0x108bba90);c.set_lr(sentinel);
    for(unsigned f=0;f<8;++f)c.set_cr(f,f+1);
    for(unsigned f=0;f<32;++f)c.set_f(f,1.25+f);
    const auto global=[&](int slot,unsigned offset,unsigned value){c.put32(c.r(2)+unsigned(slot),data+offset);c.put32(data+offset,value);};
    global(-0x7b04,0x100,t.countries);global(-0x7b98,0x104,countries);
    global(-0x7bac,0x108,0x01010101);c.put32(c.r(2)-0x7ba8,data+0x7000);
    global(-0x7bc0,0x10c,100);global(-0x6948,0x110,101);global(-0x7a64,0x114,data+0xb000);global(-0x7b54,0x118,data+0xc000);
    c.put32(c.r(2)-0x7a9c,data+0x300);date(c,data+0x300,50,t.current_year);
    c.put32(c.r(2)-0x7b80,data+0x310);date(c,data+0x310,50,t.year);
    c.put32(object,t.storage?rows:0);c.put8(object+4,t.stored_month);date(c,object+8,50,t.year);
    for(unsigned i=0;i<3;++i){
        const auto country=countries+i*0x128,row=rows+i*Ranking::size,hist=histories+i*0x1000;
        c.put_f64(country,30+i*7);c.put32(country+0x68,i);
        c.put32(country+0x74,t.selected && (t.selected!=2 || i!=1)?data+0x6000:0);
        c.put16(country+0xac,i==1?t.away_reputation:t.reputation);
        c.put32(row,i);c.put32(row+4,t.capacity?hist:0);c.put16(row+8,t.capacity);c.put16(row+10,t.count);
        c.put_f32(row+12,float(3-i));c.put16(row+16,i==1?t.away_position:t.position);c.put16(row+18,3);
        for(unsigned y=0;y<6;++y)c.put32(row+20+y*4,unsigned((y+1)*100+t.seed));
        for(unsigned n=0;n<unsigned(std::max(t.count,t.capacity));++n){
            c.put16(hist+n*6,unsigned(t.history_pattern?int(n%3)*600-500:600-int(n)*35));
            c.put8(hist+n*6+2,t.history_month);c.put16(hist+n*6+4,t.history_year+(t.history_pattern==2?int(n%2):0));
        }
    }
    c.put_f64(data+0x6000,0.85);c.put_f64(data+0x6100,0.9);
    c.put32(clubs+0x2c,t.missing==1?0:countries);c.put32(clubs+0x248+0x2c,t.missing==3?0:countries+0x128);
    if(t.missing==2)c.put32(countries+0x74,0);
    if(t.missing==4)c.put32(countries+0x128+0x74,0);
    if(!t.same_confederation)c.put32(countries+0x128+0x74,data+0x6100);
    c.put32(match+0x1c,clubs);c.put32(match+0x20,clubs+0x248);
    c.put32(match+0x14,t.missing==5?0:competition);c.put32(competition,t.competition_id);c.put32(competition+4,t.competition_parent);
    c.put32(match+0x18,t.venue?data+0x6200:0);c.put32(data+0x6210,t.venue_owner?data+0x6300:0);c.put32(data+0x6318,countries+(t.venue_owner==2?0x128:0));
    c.put16(match+0x28,t.year);c.put16(match+0x2a,18);c.put8(match+0x43,t.home_goals);c.put8(match+0x44,t.away_goals);
    c.put8(match+0x47,t.penalties);c.put8(match+0x48,t.away_goals);
    date(c,data+0x320,18,t.year);
    c.set({{3,object},{4,data+0x9000}});
    if(t.entry==entries[1]){c.set(3,t.null_manager?0:object);c.set(4,t.deleting);}
    if(t.entry==entries[0])c.set(4,t.file?data+0x9000:0);
    if(t.entry==entries[2])c.set(4,match);
    if(t.entry==entries[3])c.set(4,data+0x320);
    if(t.entry==entries[4])c.set(4,data+0x330);
    if(t.entry==entries[5])c.set({{4,countries},{5,unsigned(t.first_month)},{6,unsigned(t.first_year)},{7,unsigned(t.last_month)},{8,unsigned(t.last_year)}});
    if(t.entry==entries[9])c.set({{4,countries},{5,0xffffff9c},{6,match}});
    if(t.entry==entries[12])c.set(4,countries);
    if(t.entry==entries[13])c.set({{3,histories},{4,histories+6}});
    if(t.entry==entries[14]){
        c.put32(data+0x340,rows);c.put32(data+0x344,rows+44);c.put32(rows+12,t.left_bits);c.put32(rows+44+12,t.right_bits);
        c.set({{3,data+0x340},{4,data+0x344}});
    }
    stream.clear();const auto add=[&](unsigned at,unsigned size){const auto bytes=c.s.memory.copy_out(at,size);stream.insert(stream.end(),bytes.begin(),bytes.end());};
    add(rows,unsigned(t.countries)*44);
    for(unsigned i=0;i<unsigned(t.countries);++i){add(rows+i*44+10,2);add(histories+i*0x1000,unsigned(t.count)*6);}
    add(object+4,1);add(object+8,8);
    c.put32(trace_at,0);c.put32(output-4,0);calls=allocations=io_calls=cursor=0;next_allocation=data+0x80000;
    c.s.budget=2000000;
}
void returned(State& s) {
    Cpu c(s);if(c.sp()!=stack || c.lr()!=sentinel)throw std::runtime_error("fixture did not return");
    if(active.entry==entries[7] && active.open && !active.io) {
        if(c.u32(output-4)!=stream.size() || s.memory.copy_out(output,stream.size())!=stream)
            throw std::runtime_error("serialized bytes differ from independently prepared stream");
    }
}
void original(State& s,unsigned entry) {original_mode=true;s.pc=entry;run_until(s,sentinel);returned(s);}
void rewrite(State& s,unsigned entry) {
    original_mode=false;
    if(production_mode){s.pc=entry;run_until(s,sentinel);}else fifa_rankings_run_rewrite(s,entry);
    returned(s);
#if defined(CM_FIFA_RANKINGS_COVERAGE)
    __llvm_profile_write_file();
#endif
}
void corrupt(State& s,unsigned entry){rewrite(s,entry);Cpu c(s);const auto row=c.u32(object);c.put8(row+12,c.u8(row+12)^1);}
void corrupt_result(State& s,unsigned entry){rewrite(s,entry);Cpu c(s);c.set(3,c.r(3)^1);}
void corrupt_stream(State& s,unsigned entry){rewrite(s,entry);Cpu c(s);c.put8(output,c.u8(output)^1);}
struct Observed final:MemoryObserver {
    void mapped(unsigned,std::uint64_t)override{}
    void written(unsigned,const std::uint8_t*,std::size_t)override{}
    void cleared(unsigned,std::uint64_t)override{}
};
}
int main(int argc,char** argv) {
    if(argc!=2){std::cerr<<"Usage: cm-fifa-rankings-checks <original-segments>\n";return 2;}
    try {
        auto base=std::make_unique<State>();load_program(*base,argv[1]);dispatch_probe=probe;
        std::vector<Case> cases;
        for(auto entry:entries)cases.push_back(Case{entry});
        for(auto entry:{entries[0],entries[1],entries[3],entries[4],entries[5],entries[6],entries[7],entries[8],entries[10],entries[11]})
            for(int n:{0,1,8,9,12}) {Case t{entry,"history count "+std::to_string(n)};t.count=n;t.capacity=n+5;cases.push_back(t);}
        for(auto entry:{entries[0],entries[6],entries[7]}){
            for(int n=1;n<=9;++n){Case t{entry,"short IO "+std::to_string(n)};t.io=n;cases.push_back(t);}
            {Case t{entry,"open failure"};t.open=0;cases.push_back(t);}
        }
        for(auto entry:{entries[0],entries[4],entries[6],entries[8]}){Case t{entry,"allocation failure"};t.allocation=1;cases.push_back(t);}
        {Case t{entries[6],"history allocation failure"};t.allocation=2;cases.push_back(t);}
        for(int deleting:{-1,0,1})for(int storage:{0,1}){Case t{entries[1],"deletion"};t.deleting=deleting;t.storage=storage;cases.push_back(t);}
        {Case t{entries[1],"null manager"};t.null_manager=1;cases.push_back(t);}
        {Case t{entries[0],"new manager"};t.file=0;cases.push_back(t);}
        for(int reputation:{-32768,2999,3000,3499,3500,5999,6000,32767})for(int position:{14,15,20,21,49,50,60,61,100,101}){
            Case t{entries[12],"category"};t.reputation=reputation;t.position=position;cases.push_back(t);
        }
        for(int first:{-1,1,2,3,127})for(int last:{1,2,3,11}){Case t{entries[5],"period"};t.first_month=first;t.last_month=last;t.first_year=2002;t.last_year=2002;cases.push_back(t);}
        for(int day:{17,18,19})for(int month:{1,2,3})for(int year:{2001,2002,2003}){
            Case t{entries[3],"monthly rollover"};t.day=day;t.month=month;t.year=year;cases.push_back(t);
        }
        for(int hy:{2000,2001,2002,2003})for(int hm:{1,2,3}){Case t{entries[3],"expiry"};t.history_year=hy;t.history_month=hm;cases.push_back(t);}
        for(auto entry:{entries[4],entries[8],entries[10]}){Case t{entry,"no eligible nations"};t.selected=0;cases.push_back(t);}
        for(int missing:{1,2,3,4,5}){Case t{entries[2],"missing match dependency"};t.missing=missing;cases.push_back(t);}
        for(int home:{5,10,40,80,150})for(int away:{5,10,40,80,150})for(int hg:{0,2,8})for(int ag:{0,2,8}){
            Case t{entries[2],"match points"};t.position=home;t.away_position=away;t.home_goals=hg;t.away_goals=ag;cases.push_back(t);
        }
        for(int cid:{100,101,102})for(int type:{0,1,2})for(int parent:{0,1})for(int continental:{0,1}){
            Case t{entries[2],"match importance"};t.competition_id=cid;t.competition_type=type;t.competition_parent=parent;t.continental=continental;cases.push_back(t);
        }
        for(int rep:{2999,3499,3500,6000})for(int away:{2999,3499,3500,6000})for(int venue:{0,1,2}){
            Case t{entries[2],"caps and venue"};t.reputation=rep;t.away_reputation=away;t.venue=venue;t.venue_owner=venue==1;cases.push_back(t);
        }
        for(int same:{0,1})for(int penalties:{-1,0,3}){Case t{entries[2],"penalties/confederation"};t.same_confederation=same;t.penalties=penalties;cases.push_back(t);}
        {Case t{entries[2],"unknown competition assertion"};t.competition_id=102;t.competition_type=2;t.competition_parent=1;t.friendly=0;cases.push_back(t);}
        for(int n:{0,1,5}){Case t{entries[9],"append growth"};t.count=n;t.capacity=n?5:0;cases.push_back(t);}
        for(auto entry:{entries[3],entries[11]})for(int seed:{-100000,0,100000})for(int pattern:{0,1})for(int year:{2001,2002,2003}){
            Case t{entry,"weighted points"};t.seed=seed;t.history_pattern=pattern;t.stored_year=year;cases.push_back(t);
        }
        for(auto entry:{entries[3],entries[4],entries[8],entries[10]}){Case t{entry,"actual qsort integration"};t.actual_sort=1;cases.push_back(t);}
        for(auto left:{0u,0x80000000u,0x7f800000u,0xff800000u,0x7f800001u,0x7fc00000u,0x3f800000u})
            for(auto right:{0u,0x80000000u,0x7f800000u,0xff800000u,0x7f800001u,0x7fc00000u,0x3f800000u}){
                Case t{entries[14],"FP comparator"};t.left_bits=left;t.right_bits=right;cases.push_back(t);
            }
        for(auto entry:{entries[0],entries[1],entries[3],entries[4],entries[6],entries[7],entries[8],entries[10],entries[11]})for(int n:{0,1}){
            Case t{entry,"country count"};t.countries=n;cases.push_back(t);
        }
        for(auto entry:{entries[4],entries[8],entries[10]}){Case t{entry,"mixed eligible countries"};t.selected=2;cases.push_back(t);}
        for(auto entry:{entries[6],entries[7]})for(int io:{1,2,3,4,9}){Case t{entry,"partial item bytes"};t.io=io;t.partial_bytes=1;cases.push_back(t);}
        {Case t{entries[3],"mixed expiry swap-last"};t.history_year=2001;t.history_month=1;t.history_pattern=2;cases.push_back(t);}
        for(int day:{49,50,51}){Case t{entries[11],"same-year day comparison"};t.date_day=day;cases.push_back(t);}
        {Case t{entries[1],"null histories"};t.count=t.capacity=0;cases.push_back(t);}
        {Case t{entries[2],"away/neutral venue owner"};t.venue_owner=2;cases.push_back(t);}
        for(int reputation:{2999,7000})for(int away:{2999,7000})for(int hg:{0,1,5})for(int ag:{0,1,5})for(int hp:{5,150})for(int ap:{5,150}){
            Case t{entries[2],"point cap/floor bounds"};t.reputation=reputation;t.away_reputation=away;t.home_goals=hg;t.away_goals=ag;t.position=hp;t.away_position=ap;cases.push_back(t);
        }
        if(const auto path=std::getenv("CM_FIFA_RANKINGS_CASE_MANIFEST")) {
            std::ofstream manifest(path);
            if(!manifest)throw std::runtime_error("Cannot write case manifest");
            for(unsigned index=0;index<cases.size();++index){
                const auto& t=cases[index];manifest<<index<<' '<<hex_address(t.entry)<<' '<<t.name;
                manifest<<" countries="<<t.countries;
                manifest<<" count="<<t.count;
                manifest<<" capacity="<<t.capacity;
                manifest<<" allocation="<<t.allocation;
                manifest<<" io="<<t.io;
                manifest<<" open="<<t.open;
                manifest<<" file="<<t.file;
                manifest<<" storage="<<t.storage;
                manifest<<" deleting="<<t.deleting;
                manifest<<" null_manager="<<t.null_manager;
                manifest<<" day="<<t.day;
                manifest<<" month="<<t.month;
                manifest<<" year="<<t.year;
                manifest<<" stored_month="<<t.stored_month;
                manifest<<" stored_year="<<t.stored_year;
                manifest<<" current_year="<<t.current_year;
                manifest<<" reputation="<<t.reputation;
                manifest<<" away_reputation="<<t.away_reputation;
                manifest<<" position="<<t.position;
                manifest<<" away_position="<<t.away_position;
                manifest<<" home_goals="<<t.home_goals;
                manifest<<" away_goals="<<t.away_goals;
                manifest<<" penalties="<<t.penalties;
                manifest<<" missing="<<t.missing;
                manifest<<" same_confederation="<<t.same_confederation;
                manifest<<" competition_id="<<t.competition_id;
                manifest<<" competition_parent="<<t.competition_parent;
                manifest<<" competition_type="<<t.competition_type;
                manifest<<" continental="<<t.continental;
                manifest<<" friendly="<<t.friendly;
                manifest<<" venue="<<t.venue;
                manifest<<" venue_owner="<<t.venue_owner;
                manifest<<" history_year="<<t.history_year;
                manifest<<" history_month="<<t.history_month;
                manifest<<" selected="<<t.selected;
                manifest<<" first_month="<<t.first_month;
                manifest<<" first_year="<<t.first_year;
                manifest<<" last_month="<<t.last_month;
                manifest<<" last_year="<<t.last_year;
                manifest<<" actual_sort="<<t.actual_sort;
                manifest<<" history_pattern="<<t.history_pattern;
                manifest<<" seed="<<t.seed;
                manifest<<" partial_bytes="<<t.partial_bytes;
                manifest<<" date_day="<<t.date_day;
                manifest<<" left_bits="<<t.left_bits;
                manifest<<" right_bits="<<t.right_bits;
                manifest<<'\n';
            }
        }
        const auto only=std::getenv("CM_FIFA_RANKINGS_ONLY");const auto filter=only?std::strtoul(only,nullptr,0):0;
        unsigned attempted=0,completed=0,failures=0;std::map<unsigned,std::array<unsigned,2>> counts;
        for(bool production:{false,true}) {
            production_mode=production;
            for(const auto& test:cases){
                if(failures>=12)break;
                if(filter && test.entry!=filter)continue;
                active=test;auto s=cm::semantic_io_checks::clone(*base);Cpu c(*s);setup(c,test);Observed observer;if(production)s->memory.observer=&observer;
                const auto differences=compare_routine_call(*s,test.entry,original,rewrite,RoutineChecker::result_r3|RoutineChecker::result_f1);++attempted;
                if(!differences.empty()){
                    std::cout<<"DIFFERS "<<(production?"production ":"controlled ")<<hex_address(test.entry)<<' '<<test.name<<'\n';
                    for(const auto& d:differences)std::cout<<"  "<<d<<'\n';++failures;
                }else{++completed;++counts[test.entry][production?1:0];}
            }
        }
        production_mode=false;active=Case{entries[0]};auto s=cm::semantic_io_checks::clone(*base);Cpu c(*s);setup(c,active);
        if(compare_routine_call(*s,active.entry,original,corrupt,RoutineChecker::result_r3).empty()){++failures;std::cout<<"FAIL corruption control\n";}
        else std::cout<<"ok deliberate corruption detected\n";
        for(auto entry:{entries[5],entries[7]}){
            active=Case{entry};auto state=cm::semantic_io_checks::clone(*base);Cpu cpu(*state);setup(cpu,active);
            if(compare_routine_call(*state,entry,original,entry==entries[7]?corrupt_stream:corrupt_result,RoutineChecker::result_r3).empty()){
                ++failures;std::cout<<"FAIL result/serialized-byte corruption control\n";
            }else std::cout<<"ok result/serialized-byte corruption detected "<<hex_address(entry)<<'\n';
        }
        unsigned ownership=0;
        for(auto entry:entries){
            active=Case{entry};auto state=cm::semantic_io_checks::clone(*base);Cpu cpu(*state);setup(cpu,active);
            state->budget=state->executed;Observed observer;state->memory.observer=&observer;
            production_mode=true;ownership_mode=true;
            try{rewrite(*state,entry);++ownership;}
            catch(const std::exception& e){++failures;std::cout<<"FAIL production ownership "<<hex_address(entry)<<' '<<e.what()<<'\n';}
            ownership_mode=false;
            auto reference=cm::semantic_io_checks::clone(*base);Cpu rc(*reference);setup(rc,active);reference->budget=reference->executed;
            bool rejected=false;
            try{original(*reference,entry);}catch(const std::exception& e){rejected=std::string(e.what())=="Native instruction budget exhausted" && reference->pc==entry;}
            if(!rejected){++failures;std::cout<<"FAIL original instruction-budget control "<<hex_address(entry)<<'\n';}
        }
        production_mode=false;std::cout<<ownership<<"/15 observed normal-production ownership controls; originals rejected at zero budget\n";
        for(const auto& [entry,n]:counts)std::cout<<hex_address(entry)<<" returned "<<n[0]<<" controlled + "<<n[1]<<" production\n";
        std::cout<<completed<<" returned comparisons / "<<attempted<<" attempts; "<<failures<<" failures\n";return failures?1:0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
