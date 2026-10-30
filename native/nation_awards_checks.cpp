// Original/C++ returned comparisons with deterministic external boundaries.
// Owned virtual calls and sorting callbacks execute the module's real bodies.
#include "nation_awards_adapter.hpp"
#include "nation_awards_detail.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>

#if defined(CM_NATION_AWARDS_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
#endif
using namespace cm::aot;
using namespace cm::aot::nation_awards::detail;
namespace cm::aot { bool nation_awards_reference(State&); }
namespace {
constexpr std::uint32_t data=0x60000000,object=data+0x1000,award=data+0x2000,
    date=data+0x3000,ratings=data+0x4000,competition=data+0x5000,
    competitions=data+0x6000,people=data+0x10000,clubs=data+0x20000,nations=data+0x30000,
    rows=data+0x40000,selected=data+0x50000,matches=data+0x60000,
    trace_at=data+0x100000,output=data+0x200000,stack=0x71010000,sentinel=0xfffffff0;
constexpr std::array entries{0x10444390u,0x104443b0u,0x10444490u,0x10444550u,
    0x10444580u,0x104449c0u,0x10444c40u,0x10444e80u,0x104450c0u,0x104455b0u,
    0x104459a0u,0x10446040u,0x104462b0u,0x10446510u,0x10446b00u,0x10446c00u,
    0x10446c10u,0x10446d20u,0x10446d30u,0x10446d60u};
struct Case {
    std::uint32_t entry;
    std::string name="default";
    int count=6,returned=3,history=1,allocation=0,statistic=2,age=-1,position=0,
        nation=-1,club_nation=-1,exclude=-1,both=-1,type=2,null_competition=0,
        pointers=15,attributes=1,io=1,flag=1,children=2,month=1,year=2002,
        season_matches=10,appearances=10,substitutions=0,trait=2,accepted=1,
        recent=1,fixture_valid=1,substitute=0,comparator=0,rank_count=3,old=0,
        match_count=2,competition_table=1,division=1,club_reputation=5000,division_reputation=10,
        score=7,actual_sort=0,stored_month=1,stored_year=2002,null_manager=0,
        storage=1,null_child=0,deleting=1,null_ratings=0,selected_list=0,
        nation_present=1,club_country_present=1,nationality=0,club_nationality=0,
        varied_appearances=0,award_competition=1,match_override=-1,fixture_missing=0,
        player_missing=0,player_club_missing=0,award_day=180,award_year=2002,
        style_competition_missing=0,raw_comparator=0,table_disappears=0;
    std::uint32_t left_bits=0,right_bits=0;
};
Case active{entries[0]};
bool original_mode=false,production_mode=false;
bool ownership_mode=false;
unsigned calls=0,allocations=0,reads=0;
std::uint32_t next_allocation=0;
const std::set<std::uint32_t> dependencies{
    0x100c76c0,0x100db1b0,0x10192130,0x1029ec40,0x10303560,0x10388b80,
    0x104382c0,0x10438450,0x104386b0,0x10439ea0,0x10443850,0x104438b0,
    0x10443c20,0x10443e20,0x10443f00,0x1044aca0,0x1044b160,0x1044b3d0,
    0x1044b420,0x1044b4b0,0x1044cf90,0x1044ec90,0x1044efb0,0x1045a0a0,
    0x1048b180,0x107156b0,0x10715880,0x10715a30,0x10717890,0x10717b20,
    0x10717cb0,0x10718360,0x10738c00,0x10738c20,0x10738ca0,0x10738d10,
    0x10738e00,0x10739890,0x10747a80,0x10747cc0,0x10768300,0x10768810,
    0x10769b60,0x10773320,0x10773520,0x107735a0,0x10775130,0x10775170,
    0x107751b0,0x1077b720,0x1000fff0};
void trace(Cpu& c,std::uint32_t target) {
    if (++calls>5000) throw std::runtime_error("fixture call limit");
    auto size=c.u32(trace_at);
    const auto add=[&](std::uint32_t value) { c.put32(trace_at+4+size++*4,value); };
    add(target);add(c.lr());
    for (unsigned r=3;r<=10;++r) add(c.r(r));
    for (unsigned r=13;r<32;++r) add(c.r(r));
    for (unsigned f=0;f<8;++f) add(c.cr(f));
    for (unsigned f=1;f<=3;++f) { const auto bits=c.s.r<8>(guest::reg::fpr(f)); add(bits>>32);add(bits); }
    if (target==0x1044ec90 || target==0x1048b180) { add(c.u32(c.sp()+0x38));add(c.u32(c.sp()+0x3c)); }
    c.put32(trace_at,size);
}
void original(State&,std::uint32_t);
void rewrite(State&,std::uint32_t);
void write_date(Cpu& c,std::uint32_t at,int day,int year=2002) {
    c.put16(at,std::uint32_t(day));c.put16(at+2,std::uint32_t(year));c.put8(at+4,2);
}
bool probe(State& s) {
    Cpu c(s);const auto target=s.pc;
    if (run_leaf(s)) return true;
    if(ownership_mode)s.budget=s.executed;
    if (target>=entries[0] && target<0x10446d90) {
        if (original_mode) return nation_awards_reference(s);
        if (production_mode) return false;
        if (nation_awards_lists(target)) { nation_awards_run_rewrite(s,target);return true; }
        return false;
    }
    if (target==0x10781320) {
        const auto descriptor=c.r(12); c.put32(c.sp()+0x14,c.r(2));
        c.set(2,c.u32(descriptor+4));s.pc=c.u32(descriptor);return true;
    }
    if (!dependencies.contains(target) || (target==0x10768300 && active.actual_sort)) return false;
    trace(c,target);
    if (target==0x10775130 || target==0x10773520 || target==0x107751b0) {
        ++allocations;
        if (int(allocations)==active.allocation) c.set(3,0);
        else {
            const auto at=next_allocation,size=target==0x107751b0?c.r(4):c.r(3);
            if (size>0x100000) throw std::runtime_error("fixture allocation bound");
            next_allocation+=(size+31)&~15u;
            if (target==0x107751b0) for (unsigned i=0;i<std::min(size,2400u);++i) c.put8(at+i,c.u8(c.r(3)+i));
            c.set(3,at);
        }
    } else if (target==0x10739890) { c.put32(c.r(6),0x4e415400);c.put32(c.r(7),0x43505000); }
    else if (target==0x1077b720) c.put32(c.r(3),c.r(8));
    else if (target==0x10747a80) { ++reads;if (active.io) c.put8(c.r(3),active.flag);c.set(3,active.io); }
    else if (target==0x10747cc0) { c.put8(output,c.u8(c.r(3)));c.set(3,active.io); }
    else if (target==0x1044ec90 || target==0x1044efb0 || target==0x104386b0) {
        for (int i=0;i<std::max(0,active.returned);++i)
            c.put32(c.r(4)+unsigned(i)*4,target==0x104386b0?people+unsigned(i%6)*Person::size:rows+unsigned(i%6)*0x30);
        c.set(3,active.returned);
    } else if (target==0x1000fff0) {
        if (c.lr()==0x10444d44) { c.put32(c.r(4),selected);c.put16(c.r(5),active.returned); }
    } else if (target==0x10388b80) c.set(3,active.returned);
    else if (target==0x1045a0a0) {
        c.put32(c.r(5),active.count);c.set(3,rows);
        // Noncanonical external-state mutation: compare the caller's explicit
        // null-table fallback without claiming the actual service unloads it.
        if(active.table_disappears)c.put32(data+0x108,0);
    }
    else if (target==0x100db1b0) c.set(3,active.trait);
    else if (target==0x10192130) c.set(3,active.type);
    else if (target==0x100c76c0) c.set(3,clubs);
    else if (target==0x1044aca0) c.set(3,active.accepted);
    else if (target==0x10303560) {
        const auto index=(c.r(4)-(data+0xe000))/32;
        c.set_f(1,c.r(6)==1?(active.varied_appearances?std::max(0,8-int(index)*2):active.appearances):active.statistic==2?float(active.score):float(70));
    }
    else if (target==0x1044cf90) {
        for (unsigned i=0;i<74;++i) c.put8(output+i,c.u8(c.r(4)+i));
    } else if (target==0x10717890) write_date(c,c.r(3),200,active.year);
    else if (target==0x10718360) c.set(3,active.month);
    else if (target==0x10717b20 || target==0x10717cb0) {
        write_date(c,c.r(3),c.i16(c.r(4))+(target==0x10717b20?-int(c.r(5)):-7*int(std::uint8_t(c.r(5)))));
    } else if (target==0x107156b0) write_date(c,c.r(3),std::int16_t(c.r(4)),int(c.r(5)));
    else if (target==0x10715880) write_date(c,c.r(3),0);
    else if (target==0x10715a30) {}
    else if (target==0x10738c00) { c.put16(c.r(3),0);c.put32(c.r(3)+4,0);c.put32(c.r(3)+8,0);c.put32(c.r(3)+12,0); }
    else if (target==0x1048b180) {
        const auto list=c.r(9),nodes=data+0x70000;
        const auto count=active.recent?unsigned(active.match_count):0;
        c.put16(list,count);c.put32(list+4,count?nodes:0);c.put32(list+8,count?nodes+(count-1)*16:0);c.put32(list+12,count?nodes:0);
        for(unsigned i=0;i<count;++i) {const auto node=nodes+i*16;c.put32(node,matches+i*0x40);c.put16(node+4,0x40);c.put32(node+8,i+1<count?node+16:0);c.put32(node+12,i?node-16:0);}
        c.set(3,count);
    } else if (target==0x10738e00) {
        const auto node=c.u32(c.r(3)+12);c.set(5,node);c.compare_logical(0,node,0);
        if(node){c.set(4,c.u32(node));c.put32(c.r(3)+12,c.u32(node+8));c.set(3,c.r(4));}else c.set(3,0);
    } else if(target==0x10738ca0) {c.put16(c.r(3),0);c.put32(c.r(3)+4,0);c.put32(c.r(3)+8,0);c.put32(c.r(3)+12,0);}
    else if (target==0x1029ec40) {
        const auto at=c.r(5);
        for (unsigned side=0;side<2;++side) for (unsigned slot=0;slot<20;++slot) {
            const auto player=at+side*0x664+0x194+slot*0x4c;
            c.put32(player,active.player_missing && slot==0?missing:slot%6);c.put32(player+4,active.player_club_missing && slot==1?missing:0);
            c.put8(player+0x32,active.substitute?255:0);c.put8(player+0x35,7+slot%4);
            c.put16(at+side*0x664+0x156+slot*2,1u<<(slot%4));
        }
        c.set(3,active.fixture_valid);
    } else if (target==0x10768300) {
        // Run every comparator through its real transition vector while using
        // deterministic insertion sort as the external qsort boundary model.
        const auto array=c.r(3),count=c.r(4),width=c.r(5),descriptor=c.r(6);
        const auto back=c.lr();
        for (unsigned i=1;i<count;++i) for (unsigned k=i;k>0;--k) {
            const auto a=array+(k-1)*width,b=array+k*width;
            c.set(3,a);c.set(4,b);c.set(12,descriptor);c.set_lr(0x1000ffe0);
            c.put32(c.sp()+0x14,c.r(2));c.set(2,c.u32(descriptor+4));s.pc=c.u32(descriptor);
            run_until(s,0x1000ffe0);
            if (std::int32_t(c.r(3))<=0) break;
            const auto first=s.memory.copy_out(a,width),second=s.memory.copy_out(b,width);
            for (unsigned n=0;n<width;++n) { c.put8(a+n,second[n]);c.put8(b+n,first[n]); }
        }
        c.set_lr(back);
    }
    // run_until executes the following lowered list constructor before the
    // dispatch probe. Allow precisely that six-instruction external leaf at
    // its preceding boundary; owned entry and continuation bodies get zero.
    if(ownership_mode && target==0x10715880 && c.lr()==0x10446590)s.budget=s.executed+6;
    s.pc=c.lr();return true;
}
void initialize(Cpu& c,const Case& t) {
    c.s.memory.map(data,0x500000);c.s.memory.map(0x71000000,0x20000);
    c.s.memory.copy_in(data,std::vector<std::uint8_t>(0x500000,0xa5));
    c.s.memory.copy_in(0x71000000,std::vector<std::uint8_t>(0x20000,0xa5));
    for (unsigned r=0;r<32;++r) c.set(r,0x51000000+r*0x101);
    c.set(1,stack);c.set(2,0x108bba90);c.set_lr(sentinel);
    for (unsigned f=0;f<8;++f) c.set_cr(f,f+1);
    for (unsigned f=0;f<32;++f) c.set_f(f,1.25+f);
    const auto global=[&](int slot,unsigned offset,std::uint32_t value) {
        c.put32(c.r(2)+std::uint32_t(slot),data+offset);c.put32(data+offset,value);
    };
    global(-0x7be0,0x100,people);global(-0x7be4,0x104,t.count);global(-0x7b5c,0x108,t.competition_table?competitions:0);
    global(-0x7b98,0x10c,nations);global(-0x7b3c,0x110,data+0x7000);
    global(-0x7b4c,0x114,data+0x7000);global(-0x7b48,0x118,data+0x7000);global(-0x7a60,0x11c,data+0x7000);
    c.put32(c.r(2)-0x7ba8,data+0x8000);c.put32(c.r(2)-0x7bac,data+0x9000);
    c.put32(c.r(2)-0x7b80,date);write_date(c,date,200,t.year);
    const auto vtable=data+0xa000,descriptor=data+0xb000;
    c.put32(object,vtable);c.put32(competition,vtable);
    for (unsigned method:{8u,12u,0x14u,0xa8u}) {
        c.put32(vtable+method,descriptor+method*2);c.put32(descriptor+method*2,method==8?0x10446c00:0x1000fff0);c.put32(descriptor+method*2+4,c.r(2));
    }
    c.put32(object+4,t.storage?data+0x18000:0);c.put16(object+8,t.children);c.put32(object+12,t.history?ratings:0);c.put32(object+24,0x123);
    for (unsigned i=0;i<4;++i) { c.put32(data+0x18000+i*4,t.null_child && !i?0:data+0x19000+i*64);c.put32(data+0x19000+i*64,vtable); }
    c.put8(ratings+12,t.stored_month);c.put16(ratings+14,t.stored_year);c.put16(ratings+6,t.rank_count);
    c.put32(award+0x24,t.history?ratings:0);c.put32(award+20,0x456);c.put32(award+12,t.award_competition?data+0xc000:0);c.put32(data+0xc000,0);
    if(t.entry==entries[13]) write_date(c,award+0x20,t.award_day,t.award_year);
    for (unsigned i=0;i<10;++i) c.put32(competitions+i*4,t.null_competition?0:competition);
    if(t.style_competition_missing){c.put32(data+0xc000,1);c.put32(competitions+4,0);}
    c.put32(competition+4,data+0xc000);c.put32(competition+8,data+0xd000);
    c.put8(competition+0x43,t.type);c.put8(competition+0x46,5);
    c.put16(competition+0x3e,t.season_matches+1);c.put16(competition+0x3c,1);c.put16(competition+0xa4,t.match_override);
    for (unsigned i=0;i<6;++i) {
        const auto person=people+i*Person::size,club=clubs+i*0x248,row=rows+i*0x30;
        c.put32(person,i);c.put32(person+0x14,t.attributes?data+0xe000+i*32:0);
        c.put32(person+12,t.nation_present?nations+unsigned(t.nationality)*0x128:0);c.put32(person+0x20,club);c.put32(person+0x24,t.pointers&8?club:0);
        c.put16(person+0x5c,i<unsigned(t.old)?35:20);
        for (unsigned part=0;part<3;++part) {
            c.put32(person+0x28+part*4,t.pointers&(1<<part)?data+0xf000+part*0x100+i*0x40:0);
            c.put32(data+0xf000+part*0x100+i*0x40+0x20,100+part*10+i);
        }
        c.put32(club,i);c.put32(club+0x90,person);c.put32(club+0x2c,t.club_country_present?nations+unsigned(t.club_nationality)*0x128:0);c.put32(club+0x30,t.division?data+0xc000:0);
        c.put16(club+0x1ec,t.club_reputation);c.put16(data+0xc000+0x18,t.division_reputation);
        c.put32(row,i);c.put32(row+4,i);c.put16(row+8,70);c.put16(row+10,20);c.put16(row+12,10);
        const auto appearances=t.varied_appearances?std::max(0,8-int(i)*2):t.appearances;
        c.put8(row+12,appearances);c.put8(row+13,t.substitutions);c.put8(row+14,appearances);c.put16(row+16,20);c.put16(row+18,30);c.put16(row+0x1a,70);
        c.put32(selected+i*4,clubs+i*0x248);
    }
    for(unsigned i=0;i<3;++i)c.put32(nations+i*0x128+0x68,i);
    for (unsigned i=0;i<8;++i) { const auto match=matches+i*0x40;c.put32(match+8,t.fixture_missing && !i?missing:i);c.put16(match+0x2a,195+i);c.put16(match+0x28,t.year);c.put32(match+0x1c,clubs);c.put32(match+0x20,clubs+0x248); }
    c.put32(trace_at,0);c.put32(output,0);
    c.set(3,object);c.set(4,1);c.set(5,award);c.set(6,ratings);c.set(7,0);c.set(8,0);c.set(9,0);c.set(10,0);
    if (t.entry>=entries[4] && t.entry<=entries[7]) { c.set(4,date);c.set(5,award);c.set(6,ratings); }
    if (t.entry==entries[4]) {
        c.set(3,object);c.put32(stack+0x38,0);c.put8(stack+0x3f,t.statistic);c.put8(stack+0x43,1);
    }
    if(t.entry==entries[4] || t.entry==entries[5])c.set(6,t.null_ratings?0:ratings);
    if(t.entry==entries[5])c.set(10,t.selected_list?data+0x13000:0);
    if(t.entry==entries[1]){c.set(3,t.null_manager?0:object);c.set(4,t.deleting);}
    if (t.entry==entries[6]) c.set(6,0);
    if (t.entry==entries[8]) {
        c.set(4,0);c.set(5,date);c.set(6,award);c.set(7,t.age);c.set(8,t.position);c.set(9,t.statistic);c.set(10,t.nation);
        c.put32(stack+0x38,t.club_nation);c.put32(stack+0x3c,t.exclude);c.put32(stack+0x40,t.both);
    }
    if (t.entry==entries[9]) { c.set(4,0);c.set(5,date);c.set(6,award);c.set(7,t.age);c.set(8,t.position);c.set(9,t.statistic); }
    if (t.entry==entries[10]) {
        c.set(4,date);c.set(5,award);c.set(6,t.club_nation);c.set(7,t.nation);c.set(8,t.exclude);c.set(9,t.both);c.set(10,t.age);
        c.put16(stack+0x3a,t.position);c.put8(stack+0x3f,t.statistic);
    }
    if (t.entry>=entries[11] && t.entry<=entries[13]) { c.set(4,0);c.set(5,award); }
    if (t.entry>=entries[17]) {
        c.set(3,rows);c.set(4,rows+0x30);c.put8(rows+8,t.comparator);c.put8(rows+0x38,127);
        c.put32(rows+4,t.comparator==2?0x7fc00000:std::bit_cast<std::uint32_t>(float(t.comparator)));
        c.put_f32(rows+0x34,1.0f);
        if(t.raw_comparator){c.put32(rows+4,t.left_bits);c.put32(rows+0x34,t.right_bits);}
    }
    c.s.budget=c.s.executed+3000000;
}
void original(State& s,std::uint32_t entry) {
    original_mode=true;calls=allocations=reads=0;next_allocation=data+0x300000;
    s.pc=entry;run_until(s,sentinel);
}
void rewrite(State& s,std::uint32_t entry) {
    original_mode=false;calls=allocations=reads=0;next_allocation=data+0x300000;
    if (production_mode) { s.pc=entry;run_until(s,sentinel); }
    else nation_awards_run_rewrite(s,entry);
#if defined(CM_NATION_AWARDS_COVERAGE)
    __llvm_profile_write_file();
#endif
}
void corrupt(State& s,std::uint32_t entry) { rewrite(s,entry);Cpu c(s);c.put8(output,c.u8(output)^1); }
struct Observed final:MemoryObserver {
    void mapped(std::uint32_t,std::uint64_t) override {}
    void written(std::uint32_t,const std::uint8_t*,std::size_t) override {}
    void cleared(std::uint32_t,std::uint64_t) override {}
};
}
int main(int argc,char** argv) {
    if (argc!=2) return 2;
    try {
        auto base=std::make_unique<State>();load_program(*base,argv[1]);dispatch_probe=probe;
        std::vector<Case> cases;
        for (auto entry:entries) cases.push_back(Case{entry});
        for (auto entry:{entries[4],entries[5],entries[6],entries[7]}) {
            for (int history:{0,1}) for (int returned:{0,2,3}) for (int pointers:{0,15}) { Case t{entry,"publication"};t.history=history;t.returned=returned;t.pointers=pointers;cases.push_back(t); }
            { Case t{entry,"allocation failure"};t.allocation=1;cases.push_back(t); }
        }
        for (auto entry:{entries[8],entries[9],entries[10],entries[11],entries[12]})
            for (int count:{0,1,3,6}) for (int appearances:{0,1,10}) { Case t{entry,"selection"};t.count=count;t.appearances=appearances;cases.push_back(t); }
        for (auto entry:{entries[4],entries[8],entries[9],entries[10]}) for (int kind:{0,1,2,3,4,5,6}) { Case t{entry,"statistic"};t.statistic=kind;cases.push_back(t); }
        for (auto entry:{entries[8],entries[9],entries[10]}) for (int age:{-1,21}) for (int old:{0,3}) for (int pos:{0,2,4}) { Case t{entry,"age/position"};t.age=age;t.old=old;t.position=pos;cases.push_back(t); }
        for (auto entry:{entries[14],entries[16]}) for (int io:{0,1}) for (int flag:{0,1,255}) for (int history:{0,1}) { Case t{entry,"persistence"};t.io=io;t.flag=flag;t.history=history;cases.push_back(t); }
        for (auto entry:{entries[17],entries[18],entries[19]}) for (int value:{-128,-1,0,1,2,127,255}) { Case t{entry,"comparator"};t.comparator=value;cases.push_back(t); }
        for (int recent:{0,1}) for (int valid:{0,1}) for (int substitute:{0,1}) { Case t{entries[13],"recent fixtures"};t.recent=recent;t.fixture_valid=valid;t.substitute=substitute;cases.push_back(t); }
        for(auto entry:{entries[8],entries[10]})for(int field=0;field<4;++field)for(int value:{0,1}){
            Case t{entry,"nation filter"};if(field==0)t.nation=value;if(field==1)t.club_nation=value;if(field==2)t.exclude=value;if(field==3)t.both=value;cases.push_back(t);
        }
        for(int pointers:{0,7,15})for(int division:{0,1})for(int table:{0,1}){
            Case t{entries[10],"world optional records"};t.pointers=pointers;t.division=division;t.competition_table=table;t.statistic=3;cases.push_back(t);
        }
        for(int club:{-32768,0,499,500,32767})for(int division:{-32768,0,32767}){
            Case t{entries[10],"signed strength"};t.club_reputation=club;t.division_reputation=division;t.statistic=3;cases.push_back(t);
        }
        {Case t{entries[10],"world allocation failure"};t.allocation=1;cases.push_back(t);}
        for(int allocation:{0,1,2}){Case t{entries[13],"recent growth"};t.match_count=6;t.allocation=allocation;cases.push_back(t);}
        for(int trait:{4,6}){Case t{entries[10],"special position"};t.position=4;t.trait=trait;cases.push_back(t);}
        {Case t{entries[10],"zero world score"};t.score=0;cases.push_back(t);}
        for(auto entry:{entries[10],entries[13]}){Case t{entry,"actual qsort integration"};t.actual_sort=1;cases.push_back(t);}
        for(int deleting:{-1,0,1})for(int history:{0,1}){Case t{entries[1],"destructor storage"};t.deleting=deleting;t.history=history;cases.push_back(t);}
        {Case t{entries[1],"null destructor"};t.null_manager=1;cases.push_back(t);}
        {Case t{entries[1],"no child array"};t.storage=0;t.children=0;cases.push_back(t);}
        {Case t{entries[1],"null child"};t.null_child=1;cases.push_back(t);}
        for(int history:{0,1})for(int month:{1,2})for(int year:{2001,2002}){
            Case t{entries[2],"monthly rollover"};t.history=history;t.stored_month=month;t.stored_year=year;cases.push_back(t);
        }
        {Case t{entries[3],"no monthly ratings"};t.history=0;cases.push_back(t);}
        for(auto entry:{entries[4],entries[5]}){Case t{entry,"missing input ratings"};t.null_ratings=1;cases.push_back(t);}
        {Case t{entries[5],"selected managers list"};t.selected_list=1;cases.push_back(t);}
        for(int count:{0,1,2,4,5}){Case t{entries[4],"rank history size"};t.rank_count=count;t.returned=count;cases.push_back(t);}
        for(auto entry:{entries[6],entries[8],entries[9],entries[11],entries[12]}){
            Case t{entry,"missing competition"};t.null_competition=1;cases.push_back(t);
            for(int type:{0,1}){Case v{entry,"competition type"};v.type=type;cases.push_back(v);}
        }
        for(auto entry:{entries[8],entries[9],entries[10]}){
            Case t{entry,"missing player attributes"};t.attributes=0;cases.push_back(t);
            Case v{entry,"varied appearances"};v.varied_appearances=1;v.statistic=3;cases.push_back(v);
            Case w{entry,"missing winner names and clubs"};w.pointers=0;cases.push_back(w);
        }
        for(auto entry:{entries[9],entries[12]})for(int type:{0,1})for(int pointers:{0,15}){
            Case t{entry,"national award source"};t.type=type;t.pointers=pointers;cases.push_back(t);
        }
        for(auto entry:{entries[8],entries[10]})for(int filter=0;filter<4;++filter)for(int absent=0;absent<3;++absent){
            Case t{entry,"missing filter record"};if(filter==0)t.nation=0;if(filter==1)t.club_nation=0;if(filter==2)t.exclude=0;if(filter==3)t.both=0;
            if(absent==0)t.nation_present=0;if(absent==1)t.pointers=7;if(absent==2)t.club_country_present=0;cases.push_back(t);
        }
        {Case t{entries[8],"club country matches but person differs"};t.both=0;t.nationality=1;cases.push_back(t);}
        {Case t{entries[10],"either country matches club"};t.both=1;t.club_nationality=1;cases.push_back(t);}
        {Case t{entries[10],"inactive club competition"};t.null_competition=1;cases.push_back(t);}
        {Case t{entries[10],"zero scoring appearances"};t.statistic=3;t.appearances=0;cases.push_back(t);}
        for(auto entry:{entries[11],entries[12]}){
            Case t{entry,"team service refuses candidates"};t.accepted=0;cases.push_back(t);
            for(int source=0;source<3;++source){Case v{entry,"team style fallback"};if(source==0)v.competition_table=0;if(source==1)v.award_competition=0;if(source==2)v.null_competition=1; if(source==0 || source==2)continue;cases.push_back(v);}
        }
        {Case t{entries[11],"explicit season games"};t.match_override=5;cases.push_back(t);}
        {Case t{entries[14],"month allocation failure"};t.allocation=1;cases.push_back(t);}
        for(int missing=0;missing<4;++missing){Case t{entries[13],"recent optional data"};if(missing==0)t.fixture_missing=1;if(missing==1)t.player_missing=1;if(missing==2)t.player_club_missing=1;if(missing==3)t.attributes=0;cases.push_back(t);}
        for(int source=0;source<3;++source){Case t{entries[13],"recent team style fallback"};if(source==0)t.competition_table=0;if(source==1)t.award_competition=0;if(source==2)t.null_competition=1;cases.push_back(t);}
        for(auto [day,year]:{std::pair{50,2001},std::pair{50,2002},std::pair{180,2003}}){Case t{entries[13],"recent date boundary"};t.award_day=day;t.award_year=year;t.recent=0;cases.push_back(t);}
        {Case t{entries[9],"national winner has no country"};t.type=1;t.nation_present=0;cases.push_back(t);}
        {Case t{entries[10],"either country missing club"};t.both=1;t.pointers=7;cases.push_back(t);}
        for(auto entry:{entries[11],entries[12],entries[13]}){Case t{entry,"style competition absent"};t.style_competition_missing=1;cases.push_back(t);}
        for(auto entry:{entries[11],entries[12]}){Case t{entry,"noncanonical dependency clears competition table"};t.table_disappears=1;cases.push_back(t);}
        for(auto entry:{entries[18],entries[19]})for(auto left:{0u,0x80000000u,0x7f800000u,0xff800000u,0x7f800001u,0x7fc00000u,0xffc00000u,0x3f800000u})
            for(auto right:{0u,0x80000000u,0x7f800000u,0xff800000u,0x7f800001u,0x7fc00000u,0xffc00000u,0x3f800000u}){
                Case t{entry,"raw FP comparator"};t.raw_comparator=1;t.left_bits=left;t.right_bits=right;cases.push_back(t);
            }
        if (const auto path=std::getenv("CM_NATION_AWARDS_CASE_MANIFEST")) {
            std::ofstream manifest(path);
            for (unsigned index=0;index<cases.size();++index) {
                const auto& t=cases[index];manifest<<index<<' '<<hex_address(t.entry)<<' '<<t.name
                    <<" count="<<t.count<<" stat="<<t.statistic<<" history="<<t.history
                    <<" returned="<<t.returned<<" pointers="<<t.pointers<<" age="<<t.age
                    <<" old="<<t.old<<" pos="<<t.position<<" appearances="<<t.appearances<<'\n';
            }
        }
        unsigned attempted=0,completed=0,failures=0;
        std::map<std::uint32_t,std::array<unsigned,2>> per_entry;
        const auto only=std::getenv("CM_NATION_AWARDS_ONLY");
        const auto filter=only?std::strtoul(only,nullptr,0):0;
        for (bool production:{false,true}) {
            production_mode=production;
            for (const auto& test:cases) {
                if(filter && test.entry!=filter)continue;
                active=test;auto s=cm::semantic_io_checks::clone(*base);Cpu c(*s);initialize(c,test);s->pc=test.entry;
                Observed observer;if(production)s->memory.observer=&observer;
                const auto differences=compare_routine_call(*s,test.entry,original,rewrite,RoutineChecker::result_r3|RoutineChecker::result_f1);
                ++attempted;
                if (!differences.empty()) {
                    std::cout<<"DIFFERS "<<(production?"production ":"controlled ")<<hex_address(test.entry)<<' '<<test.name
                        <<" count="<<test.count<<" stat="<<test.statistic<<" history="<<test.history
                        <<" returned="<<test.returned<<" pointers="<<test.pointers<<" age="<<test.age
                        <<" old="<<test.old<<" pos="<<test.position<<" appearances="<<test.appearances<<'\n';
                    for (const auto& difference:differences) std::cout<<"  "<<difference<<'\n';
                    if (++failures>=12) break;
                } else {++completed;++per_entry[test.entry][production?1:0];}
            }
            if(failures>=12)break;
        }
        production_mode=false;active=cases.front();auto s=cm::semantic_io_checks::clone(*base);Cpu c(*s);initialize(c,active);
        if(compare_routine_call(*s,active.entry,original,corrupt,RoutineChecker::result_r3).empty()) {++failures;std::cout<<"FAIL corruption control\n";}
        else std::cout<<"ok deliberate memory corruption detected\n";
        unsigned ownership=0;
        for(auto entry:entries){
            active=Case{entry};auto normal=cm::semantic_io_checks::clone(*base);Cpu nc(*normal);initialize(nc,active);
            normal->budget=normal->executed;Observed observer;normal->memory.observer=&observer;
            production_mode=true;ownership_mode=true;
            try{rewrite(*normal,entry);++ownership;}
            catch(const std::exception& e){++failures;std::cout<<"FAIL production ownership "<<hex_address(entry)<<" at "<<hex_address(normal->pc)<<' '<<e.what()<<'\n';}
            ownership_mode=false;
            auto reference=cm::semantic_io_checks::clone(*base);Cpu rc(*reference);initialize(rc,active);reference->budget=reference->executed;
            bool rejected=false;
            try{original(*reference,entry);}catch(const std::exception& e){rejected=std::string(e.what())=="Native instruction budget exhausted" && reference->pc==entry;}
            if(!rejected){++failures;std::cout<<"FAIL original budget negative "<<hex_address(entry)<<'\n';}
        }
        production_mode=false;std::cout<<ownership<<"/20 observed production ownership controls; original budgets rejected\n";
        for (const auto& [entry,counts]:per_entry)std::cout<<hex_address(entry)<<" returned "<<counts[0]<<" controlled + "<<counts[1]<<" production\n";
        std::cout<<completed<<" returned comparisons / "<<attempted<<" attempts; "<<failures<<" failures\n";return failures?1:0;
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
