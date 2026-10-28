// Returned original/C++ comparisons over valid mapped competition records.
// External service fixtures preserve call order and the full guest boundary;
// they establish this module's contracts, not those services' internals.
#include "euro_champ_qual_adapter.hpp"
#include "euro_champ_qual_detail.hpp"
#include "semantic_io_checks.hpp"
#include "routine_check.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <cctype>
#include <set>
#include <string>
#include <vector>
#include <limits>
#if defined(CM_EURO_CHAMP_QUAL_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
#endif
using namespace cm::aot;
using guest::Cpu;
namespace cm::aot { bool euro_champ_qual_reference(State&); }
namespace {
constexpr std::uint32_t data=0x60000000, object=data+0x1000, groups=data+0x2000,
    calendar=data+0x6000, list=data+0x9000, definitions=data+0x10000,
    clubs=data+0x20000, nations=data+0x40000, venues=data+0x50000,
    entrant_list=data+0x80000, draw_list=data+0x81000,
    descriptor_table=data+0x90000, vtable=data+0x91000, group_vtable=data+0x92000,
    buffer=data+0xa0000, date=data+0xa1000, output=data+0xa2000,
    seed=data+0xb0000, cursor=data+0xb0004, samples=data+0xb1000,
    file=data+0xc0000, serialized=data+0xd0000, appended=data+0xe0000,
    trace_at=data+0x200000, allocation_base=data+0x1000000,
    stack=0x71010000, sentinel=0xfffffff0;
constexpr std::array entries{0x104f9fa0u,0x104fa2a0u,0x104fa3d0u,0x104fa980u,
    0x104fab00u,0x104fab30u,0x104faf40u,0x104fb050u,0x104fbe40u,
    0x104fc3c0u,0x104fc4b0u,0x104fc5d0u,0x104fc5e0u,0x104fc770u,
    0x104fc840u,0x104fcbc0u,0x104fd320u,0x104fd680u,0x104fdae0u,
    0x104fdfc0u,0x104fe040u,0x104fe0b0u,0x104fe1d0u,0x104fe5a0u,
    0x104fe980u,0x104fed60u,0x104fef80u,0x104fefe0u,0x104ff090u,
    0x105022a0u,0x105026a0u};
struct Case {
    std::uint32_t entry; std::string name;
    int year=2000, phase=-1, hosts=1, count=50, current=8, event=7,
        kind=1001, flag=0, records=6, deleting=0, storage=15,
        fail_allocation=0, fail_io=0, file_records=2, bad_index=0,
        virtual_method=-1, virtual_result=0, weekday=5, score=2,
        bound=9, rng=0, seed_value=0, current_year=1998, current_day=1,
        active=0, capacity=100, filter=0, nation_count=80, started=0,
        final_count=6, final_flags=0, record_kind=164, service_result=0,
        anchor_year=-1, anchor_day=-1, summary_offset=0;
    double stat18=13, stat19=14;
    std::uint32_t service_target=0, null_group_mask=0;
    bool null_object=false, optional_output=true, supplied_list=false,
        missing_host_service=false, no_record=false, ranking_match=true, missing_group_array=false, host_missing_first=false;
};
Case active{entries[0],"default"};
bool reference_mode=false;
unsigned allocations=0, calls=0, io_calls=0;
std::uint32_t next_allocation=allocation_base;
unsigned corruption=0;
std::uint32_t reserve(Cpu& c,unsigned size) {
    const auto at=next_allocation;
    next_allocation+=(size+31)&~15u;
    if(next_allocation>=data+0x3f00000) throw std::runtime_error("fixture allocation limit");
    // Mapped sentinel bytes expose every byte the original leaves untouched.
    (void)c; return at;
}
void trace(Cpu& c,std::uint32_t target) {
    if(++calls>100000) throw std::runtime_error("fixture call limit");
    auto n=c.u32(trace_at);
    const auto add=[&](std::uint32_t x){c.put32(trace_at+4+n++*4,x);};
    add(target); add(c.lr());
    for(unsigned r=3;r<=10;++r) add(c.r(r));
    for(unsigned r=13;r<32;++r) add(c.r(r));
    for(unsigned cr=0;cr<8;++cr) add(c.cr(cr));
    // Large competition constructors and varargs consume guest stack words.
    if(target==0x1056dc60 || target==0x104bc3c0 || target==0x1058db20)
        for(unsigned off=0x38;off<=0x70;off+=4) add(c.u32(c.sp()+off));
    if(n>0x2fffff) throw std::runtime_error("fixture trace capacity");
    c.put32(trace_at,n);
}
void date_record(Cpu& c,std::uint32_t at,int day,int year) {
    c.put16(at,unsigned(day)); c.put16(at+2,unsigned(year)); c.put8(at+4,1);
}
void entrant_record(Cpu& c,std::uint32_t at,unsigned index) {
    c.put32(at,clubs+(index%80)*0x248);
    c.put8(at+0x40,index%3==0?7:0);
}
void group_record(Cpu& c,std::uint32_t at,int count,std::uint32_t record_at) {
    c.put32(at,group_vtable); c.put32(at+4,definitions);
    c.put16(at+0x3a,1001); c.put16(at+0x3e,unsigned(count));
    c.put32(at+0xb0,record_at); c.put32(at+0xa4,record_at+0x2000);
    for(unsigned i=0;i<64;++i) {
        entrant_record(c,record_at+i*0x44,i);
        c.put32(record_at+0x2000+i*8,clubs+i*0x248); c.put8(record_at+0x2005+i*8,i%2);
    }
}
void initialize_group_result(Cpu& c,std::uint32_t at,unsigned count) {
    const auto record_at=reserve(c,0x3000);
    group_record(c,at,int(count),record_at);
}
bool probe(State& s) {
    Cpu c(s); const auto target=s.pc;
    // Cpu::call runs native leaves before its probe. Original generated calls
    // reach this probe first, so use the same leaf path before recording or
    // scripting external dependencies on either side. Owned entries still
    // take the hook-free reference / rewrite routes below.
    if(!(target>=0x104f9fa0 && target<0x105026c0) && run_leaf(s)) return true;
    if(target==0x10781320) {
        const auto descriptor=c.r(12), entry=c.u32(descriptor), method=c.u32(descriptor+8);
        c.put32(c.sp()+0x14,c.r(2));
        trace(c,entry);
        if(active.virtual_method==int(method)) { c.set(3,std::uint32_t(active.virtual_result)); s.pc=c.lr(); return true; }
        if(std::ranges::find(entries,entry)!=entries.end()) { c.set(2,c.u32(descriptor+4)); s.pc=entry; return true; }
        c.set(3,method==0x20?0u:1u); s.pc=c.lr(); return true;
    }
    if(target>=0x104f9fa0 && target<0x105026c0) {
        if(reference_mode) return euro_champ_qual_reference(s);
        if(!try_modern_euro_champ_qual(s,target)) throw std::runtime_error("nested production adapter declined"); return true;
    }
    if(target==sentinel) return false;
    trace(c,target);
    const auto result=[&](std::uint32_t value){c.set(3,value);};
    if(target==active.service_target) {result(unsigned(active.service_result));s.pc=c.lr();return true;}
    if(target==0x10775130 || target==0x10773520) {
        const auto size=c.r(3); ++allocations;
        result(active.fail_allocation==int(allocations)?0:reserve(c,size));
    } else if(target==0x10775170 || target==0x107735a0 || target==0x10738c20 ||
              target==0x104536c0 || target==0x10738ca0 || target==0x10738c00) {
        // Caller contract only; object/allocator internals are outside ownership.
    } else if(target==0x1049e4f0) {
        for(unsigned off:{8,0xc,0x10,0xbc}) c.put32(c.r(3)+off,0);
    } else if(target==0x100c76c0) {
        const auto index=(c.r(3)-nations)/0x128; result(clubs+(index%80)*0x248);
    } else if(target==0x1049ea00) {
        result(c.u32(c.r(3)+0xb0)+c.r(4)*0x44);
    } else if(target==0x10459b50) result(output+0x100);
    else if(target==0x10457d00) c.set_f(1,c.r(5)==0x12?active.stat18:active.stat19);
    else if(target==0x10715880) date_record(c,c.r(3),1,1998);
    else if(target==0x107156f0) {
        const auto at=c.r(3); date_record(c,at,int(c.r(4))+(int(c.r(5))-1)*31,int(c.r(6)));
    } else if(target==0x107156b0) date_record(c,c.r(3),int(c.r(4)),int(c.r(5)));
    else if(target==0x10717a10) {
        const auto at=c.r(3), source=c.r(4); date_record(c,at,c.i16(source)+int(c.r(5)),c.u16(source+2));
    } else if(target==0x10715b20) result(unsigned(active.weekday));
    else if(target==0x104a9810 || target==0x104854f0 || target==0x10485570 || target==0x104a9890) {
        const unsigned stride=target==0x104a9810 || target==0x104a9890?0x4c:0x78;
        const auto at=c.r(3)+c.r(4)*stride;
        if(target!=0x104a9890) date_record(c,at,int(c.r(5))+(int(c.r(6))-1)*31,int(c.r(target==0x104854f0?9:10)));
    } else if(target==0x1049d1c0) {
        c.put32(c.r(6),active.hosts>0&&!active.host_missing_first?0u:~0u); c.put32(c.r(7),active.hosts>1||active.host_missing_first?1u:~0u); result(unsigned(std::max(0,active.hosts)));
    } else if(target==0x1056dc60) {
        const auto at=c.r(3), count=c.r(5), selected=c.r(6);
        initialize_group_result(c,at,std::min(count,64u));
        if(selected) for(unsigned i=0;i<std::min(count,64u);++i) c.put32(c.u32(at+0xb0)+i*0x44,c.u32(selected+i*4));
    } else if(target==0x104bc3c0) {
        const auto at=c.r(3), count=c.r(5), selected=c.r(6); initialize_group_result(c,at,std::min(count,64u));
        for(unsigned i=0;i<std::min(count,64u);++i) c.put32(c.u32(at+0xb0)+i*0x44,c.u32(selected+i*4));
    } else if(target==0x1058db20) {
        const auto at=c.r(3), count=c.r(4), selected=c.r(5); initialize_group_result(c,at,count);
        for(unsigned i=0;i<count;++i) entrant_record(c,c.u32(at+0xb0)+i*0x44,c.u32(selected+i*4)+(active.ranking_match?0:20));
    } else if(target==0x104a60e0) {
        const auto at=c.r(3), index=c.r(4); c.put32(c.u32(at+0xb0)+index*0x44,c.r(5)); result(1);
    } else if(target==0x1077b770) result(0x1234u+unsigned(active.seed_value));
    else if(target==0x10739890) {c.put32(c.r(6),0x4555524f); c.put32(c.r(7),0x5155414c);}
    else if(target==0x10768810) result(0x01020304);
    else if(target==0x1077b720 || target==0x10727890) {c.put32(c.r(3),c.r(4)); result(4);}
    else if(target==0x10775fe0 || target==0x10776360) {
        ++io_calls; const auto at=c.r(3), size=c.r(4), count=c.r(5), bytes=size*count;
        auto position=c.u32(file); const bool failed=active.fail_io==int(io_calls);
        const auto copied=failed?bytes?bytes-1:0:bytes;
        for(unsigned i=0;i<copied;++i) {
            if(target==0x10775fe0) c.put8(at+i,c.u8(file+0x100+position+i));
            else c.put8(file+0x100+position+i,c.u8(at+i));
        }
        c.put32(file,position+copied); result(failed?0:count);
    } else if(target==0x10777ce0 || target==0x10777f30) {
        ++io_calls; const bool failed=active.fail_io==int(io_calls);
        if(!failed) {
            if(target==0x10777ce0) c.put32(c.r(4),c.u32(file));
            else c.put32(file,c.u32(c.r(4)));
        }
        result(failed?~0u:0);
    } else if(target==0x10738e00) {
        const auto index=c.u32(list+8); c.put32(list+8,index+1);
        result(index<unsigned(std::max(0,active.file_records))?serialized+index*0x50:0);
    } else if(target==0x10738d10) {
        const auto count=c.u32(list); for(unsigned i=0;i<c.r(5);++i) c.put8(appended+count*0x50+i,c.u8(c.r(4)+i));
        c.put32(list,count+1); result(1);
    } else if(target==0x104888f0) {
        const auto n=c.u32(output+0x40); for(unsigned i=0;i<0x50;++i) c.put8(appended+n*0x50+i,c.u8(c.r(4)+i));
        c.put32(output+0x40,n+1); result(1);
    } else {
        // Every external call is a scripted boundary. New dependencies must
        // be added explicitly so an accidental call target cannot pass.
        static const std::set<std::uint32_t> boundaries{
            0x100c76c0,0x102b71e0,0x102b7280,0x102b7490,0x102b7750,0x102bacc0,0x10384500,0x103846e0,
            0x10384820,0x104533a0,0x104536c0,0x104564a0,0x10457490,0x10457d00,0x10459b50,0x10464d00,
            0x104698c0,0x1047ef50,0x104854f0,0x10485570,0x104888f0,0x1048d460,0x1049d1c0,0x1049e4f0,
            0x1049e600,0x1049ea00,0x1049eb30,0x104a00a0,0x104a0e40,0x104a1540,0x104a1c30,0x104a3930,
            0x104a60e0,0x104a6660,0x104a9810,0x104a9890,0x104aff20,0x104bc3c0,0x104f8ee0,0x1056dc60,
            0x1058db20,0x107156b0,0x107156f0,0x10715880,0x10715b20,0x10717a10,0x10727890,0x10738c00,
            0x10738c20,0x10738ca0,0x10738d10,0x10738e00,0x10739890,0x1074b830,0x1074bab0,0x10768300,
            0x10768810,0x10769b60,0x10773520,0x107735a0,0x10775130,0x10775170,0x10775fe0,0x10776360,
            0x10777ce0,0x10777f30,0x1077b720,0x1077b770,0x10781320,
        };
        if(!boundaries.contains(target)) throw std::runtime_error("unscripted fixture dependency "+hex_address(target));
        result(target==0x1048d460?0u:1u);
    }
    s.pc=c.lr(); return true;
}
void initialize(Cpu& c) {
    c.s.memory.map(data,0x4000000); c.s.memory.map(stack-0x10000,0x20000);
    c.s.memory.copy_in(data,std::vector<std::uint8_t>(0x4000000,0xa5));
    c.s.memory.copy_in(stack-0x10000,std::vector<std::uint8_t>(0x20000,0xa5));
    for(unsigned r=0;r<32;++r) c.set(r,0x51000000+r*0x101);
    c.set(1,stack); c.set(2,0x108bba90); c.set_lr(sentinel);
    for(unsigned cr=0;cr<8;++cr) c.set_cr(cr,cr+1);
    c.put32(trace_at,0); c.put32(output+0x40,0);
    const auto variable=[&](int slot,unsigned off,std::uint32_t value) {
        c.put32(c.r(2)+std::uint32_t(slot),data+off); c.put32(data+off,value);
    };
    variable(-0x7bac,0x100,1); variable(-0x7ba8,0x104,0);
    variable(-0x7b98,0x108,nations); variable(-0x7be8,0x10c,clubs);
    variable(-0x7bbc,0x110,definitions); variable(-0x7548,0x114,venues);
    variable(-0x7b04,0x118,unsigned(active.nation_count)); variable(-0x7a70,0x11c,0);
    variable(-0x7424,0x120,active.missing_host_service?0u:data+0x10000);
    variable(-0x6948,0x124,0); variable(-0x7a64,0x128,data+0xf100);
    variable(-0x7a60,0x12c,data+0xf200); variable(-0x742c,0x130,data+0xf300);
    variable(-0x7b48,0x134,data+0xf400); variable(-0x7b5c,0x138,data+0xf500);
    variable(-0x7520,0x13c,data+0xf600); variable(-0x7bb8,0x140,buffer);
    c.put32(c.r(2)-0x7a9c,date); date_record(c,date,active.current_day,active.current_year); c.put32(date+4,0x01020304);
    // These original TOC slots are independent dates, never alias them merely
    // because common fixtures initialize them to equal values.
    c.put32(c.r(2)-0x7b80,date+0x20);
    date_record(c,date+0x20,active.anchor_day<0?active.current_day:active.anchor_day,
        active.anchor_year<0?active.current_year:active.anchor_year);
    c.put32(c.r(2)-0x7b90,seed); c.put32(seed,unsigned(active.seed_value));
    c.put32(c.r(2)-0x7b84,cursor); c.put32(cursor,samples+unsigned(active.rng&7)*2);
    c.put32(c.r(2)-0x7b8c,samples); variable(-0x7b88,0x144,samples+14);
    for(unsigned i=0;i<16;++i) c.put16(samples+i*2,unsigned(active.rng)*1237+i*67);
    const auto sizes=data+0xf700; c.put32(c.r(2)-0x4594,sizes);
    for(unsigned i=0;i<9;++i) c.put8(sizes+i,0);
    // Keep the original immutable matrix: its five -1 holes are part of the contract.
    constexpr std::array nation_slots{
        -0x6508,-0x757c,-0x74f4,-0x750c,-0x7a14,-0x66a8,-0x6600,-0x65a4,
        -0x7598,-0x7504,-0x7584,-0x663c,-0x7578,-0x7594,-0x75b0,-0x7af4,
        -0x64ec,-0x75c4,-0x7658,-0x7a00,-0x758c,-0x64e0,-0x6618,-0x75a8,
        -0x7514,-0x6528,-0x7a10,-0x6690,-0x6564,-0x759c,-0x74f0,-0x7a18,
        -0x75c8,-0x75a0,-0x75c0,-0x79fc,-0x75ac,-0x64f4,-0x6630,-0x6598,
        -0x7574,-0x7590,-0x74ec,-0x7a1c,-0x6594,-0x651c,-0x6614,-0x658c,
        -0x74e0,
    };
    unsigned nation_index=0;
    for(int slot:nation_slots) { variable(slot,0xf0000+nation_index*4,nation_index); ++nation_index; }
    for(unsigned i=0;i<80;++i) {
        const auto club=clubs+i*0x248, nation=nations+i*0x128;
        c.put32(club,i); c.put32(club+0x2c,nation); c.put8(club+0x23b,i%2); c.put8(club+0x242,i%2);
        c.put32(nation+0x68,i); c.put32(nation+0x74,data+0xf900+(active.filter==1?i%3:0)*0x20);
        c.put32(data+0xf908+(active.filter==1?i%3:0)*0x20,i%3==0||active.filter==0?0u:1u);
        if(active.filter==2) c.put32(nation+0x74,0);
    }
    for(unsigned i=0;i<100;++i) { c.put32(definitions+i*0x6c,i); c.put8(definitions+i*0x6c+0x67,i%2); }
    c.put32(list,0); c.put32(list+4,serialized); c.put32(list+8,0); c.put32(list+0xc,serialized);
    c.put32(file,0);
    c.put32(file+0x100,unsigned(active.file_records));
    for(unsigned i=0;i<unsigned(std::max(0,active.file_records));++i) {
        const auto source=serialized+i*0x50;
        c.put32(source,i); c.put32(source+4,i%2?~0u:i); c.put32(source+0xc,i); c.put32(source+0x10,i+1);
        if(active.bad_index) c.put32(source+unsigned(active.bad_index-1)*4,~0u);
        for(unsigned b=0;b<0x50;++b) c.put8(file+0x104+i*0x50+b,c.u8(source+b));
    }
    c.put8(file+0x104+unsigned(std::max(0,active.file_records))*0x50,unsigned(active.hosts));
    c.put8(file+0x105+unsigned(std::max(0,active.file_records))*0x50,unsigned(active.flag));
    for(unsigned i=0;i<11;++i) {
        const auto group=data+0x3000+i*0x200;
        c.put32(groups+i*4,active.null_group_mask&(1u<<i)?0:group);
        group_record(c,group,i==8?9:i==9?active.final_count:active.records,data+0x120000+i*0x4000);
        if(i==8 && active.summary_offset) for(unsigned n=0;n<64;++n)
            c.put32(c.u32(group+0xb0)+n*0x44,clubs+((n+unsigned(active.summary_offset))%80)*0x248);
        if(i==9 && active.final_flags) for(unsigned n=0;n<64;++n) {
            const auto item=c.u32(group+0xa4)+n*8;
            c.put8(item+5,active.final_flags==1?0:1);
            if(active.final_flags==3)c.put32(item,clubs+((n+20)%80)*0x248);
        }
    }
    group_record(c,object,active.records,data+0x120000+11*0x4000);
    c.put32(object,vtable); c.put32(object+4,definitions);
    c.put32(object+8,active.storage&1?data+0xf000:0);
    c.put32(object+0xc,active.missing_group_array?0:groups);
    c.put32(groups-8,data+0x3000); c.put32(object+0x10,active.storage&2?list:0);
    c.put32(object+0xbc,active.storage&4?calendar:0);
    c.put32(object+0xec,active.storage&8?entrant_list:0);
    c.put32(object+0xf8,draw_list); c.put32(object+0xf0,unsigned(active.count));
    c.put16(object+0x40,unsigned(active.year)); c.put16(object+0x3c,2); c.put16(object+0x3a,1001);
    c.put32(object+0x2c,10); c.put32(object+0x30,unsigned(active.current));
    c.put8(object+0xf4,unsigned(active.hosts)); c.put8(object+0xfc,unsigned(active.flag));
    c.put8(object+0xfd,7); c.put8(object+0xfe,active.kind==-1?255:8); c.put8(object+0xff,9);
    c.put8(object+0x45,unsigned(active.started)); c.put8(object+0x46,7); c.put8(object+0x47,3); c.put8(object+0x4b,15);
    c.put8(object+0xc0,1); c.put8(object+0xc1,1); c.put8(object+0xc4,3); c.put8(object+0xc5,1); c.put8(object+0xc6,10);
    c.put16(object+0xa6,10); c.put16(object+0xa8,0); c.put16(object+0xdc,36); c.put16(object+0xde,0);
    c.put16(object+0xb6,1); c.put16(object+0xb8,0);
    for(unsigned i=0;i<10;++i) date_record(c,calendar+i*0x4c,1+int(i)*10,int(i>3));
    for(unsigned i=0;i<80;++i) c.put32(entrant_list+i*4,clubs+i*0x248);
    for(unsigned i=0;i<54;++i) c.put32(draw_list+i*4,clubs+(i%80)*0x248);
    unsigned descriptor_index=0;
    const auto method=[&](std::uint32_t table,unsigned offset,std::uint32_t entry) {
        const auto descriptor=descriptor_table+descriptor_index++*16;
        c.put32(table+offset,descriptor); c.put32(descriptor,entry); c.put32(descriptor+4,c.r(2)); c.put32(descriptor+8,offset);
    };
    for(unsigned offset=0;offset<0xa0;offset+=4) {
        method(group_vtable,offset,0x12000000+offset);
        auto target=0x12010000+offset;
        if(offset==0x44) target=0x104fa3d0;
        if(offset==0x94) target=0x104fc3c0;
        if(offset==0x64) target=0x104fe980;
        if(offset==0x0c) target=0x104fdfc0;
        method(vtable,offset,target);
    }
    c.put32(c.r(2)-0x606c,vtable);
    c.put32(data+0xf500,object);
    c.put32(buffer,0); c.put16(buffer+0x30,unsigned(active.record_kind)); c.put16(buffer+0x32,1); c.put8(buffer+0x42,unsigned(active.phase));
    c.set({{3,active.null_object?0:object},{4,unsigned(active.phase)},{5,output},{6,output+4},{7,active.optional_output?output+8:0}});
    switch(active.entry) {
    case 0x104f9fa0: c.set({{4,unsigned(active.year)},{5,definitions}}); break;
    case 0x104fa2a0: c.set(4,unsigned(active.deleting)); break;
    case 0x104fa980: c.set({{4,clubs},{5,unsigned(active.event)},{6,unsigned(active.phase)},{7,active.optional_output?output+8:0},{8,buffer}}); break;
    case 0x104fab30:
        c.set({{4,0},{5,unsigned(active.event)},{7,unsigned(active.kind)},{8,active.no_record?0:buffer},{9,1}});
        c.put8(stack+0x3b,1); c.put8(stack+0x3f,unsigned(active.flag)); c.put32(stack+0x40,output); break;
    case 0x104fbe40: c.set(3,unsigned(active.bound)); break;
    case 0x104fc4b0: c.set({{4,output},{5,unsigned(active.capacity)},{6,unsigned(active.kind)},{8,unsigned(active.event)},{9,clubs}}); break;
    case 0x104fc840: c.set({{4,unsigned(active.phase)},{5,active.supplied_list?entrant_list:0},{6,unsigned(active.records)}}); break;
    case 0x104fdae0: c.set({{4,date},{5,unsigned(active.active)}}); break;
    case 0x104fdfc0: c.set({{4,buffer},{5,output}}); break;
    case 0x104fe1d0: case 0x104fe5a0: c.set(4,file); break;
    case 0x104fed60: c.set({{4,0},{5,unsigned(active.phase)},{6,unsigned(active.event)},{7,unsigned(active.kind)},{8,unsigned(active.score)}}); break;
    case 0x104fef80: case 0x104fefe0: c.set(4,0); break;
    case 0x105022a0:
        c.set({{4,unsigned(active.phase)},{5,10},{6,8},{7,unsigned(active.year)},{8,2},{9,unsigned(active.score)},{10,3}});
        c.put8(stack+0x3b,unsigned(active.flag)); break;
    default: break;
    }
    c.s.budget=200000000;
}
void returned(State& s) {
    Cpu c(s);
    if(c.sp()!=stack || c.lr()!=sentinel || s.pc!=sentinel) throw std::runtime_error("fixture did not return");
}
void dump_trace(State& s,const char* side) noexcept {
    const auto* directory=std::getenv("CM_EURO_CHAMP_QUAL_TRACE_DIR");
    if(!directory || !*directory) return;
    try {
        Cpu c(s); auto name=hex_address(active.entry)+"-"+active.name;
        for(auto& character:name) if(!std::isalnum(static_cast<unsigned char>(character)) && character!='-') character='_';
        const auto root=std::filesystem::path(directory); std::filesystem::create_directories(root);
        const auto bytes=s.memory.copy_out(trace_at,4+std::min(c.u32(trace_at),0x2fffffu)*4);
        std::ofstream out(root/(name+"-"+side+".bin"),std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes.data()),std::streamsize(bytes.size()));
        std::ofstream meta(root/(name+"-"+side+".txt"));
        meta<<"pc="<<hex_address(s.pc)<<" calls="<<calls<<" allocations="<<allocations<<" io="<<io_calls<<'\n';
    } catch(...) { }
}
void original(State& s,std::uint32_t entry) {
    reference_mode=true;
    try {s.pc=entry;run_until(s,sentinel);returned(s);dump_trace(s,"original");}
    catch(...) {dump_trace(s,"original-stopped");throw;}
}
void rewrite(State& s,std::uint32_t entry) {
    reference_mode=false;
    try {if(!try_modern_euro_champ_qual(s,entry)) throw std::runtime_error("production adapter declined");returned(s);dump_trace(s,"rewrite");}
    catch(...) {dump_trace(s,"rewrite-stopped");throw;}
#if defined(CM_EURO_CHAMP_QUAL_COVERAGE)
    __llvm_profile_write_file();
#endif
}
void corrupted(State& s,std::uint32_t entry) {
    rewrite(s,entry); Cpu c(s);
    if(corruption==0) c.set(3,c.r(3)^1);
    else {const auto at=corruption==1?cursor:corruption==2?seed:file+0x100; c.put8(at,c.u8(at)^1);}
}
unsigned result_kind(std::uint32_t entry) {
    switch(entry) {
    case 0x104f9fa0: case 0x104fa2a0: case 0x104fa3d0: case 0x104fa980:
    case 0x104faf40: case 0x104fbe40: case 0x104fc3c0: case 0x104fc5d0:
    case 0x104fc840: case 0x104fd320: case 0x104fdae0: case 0x104fe1d0:
    case 0x104fef80: case 0x104fefe0: case 0x105026a0:return RoutineChecker::result_r3;
    default:return 0;
    }
}
std::vector<Case> cases() {
    std::vector<Case> out;
    for(auto entry:entries) {Case test{entry,"default"}; if(entry==0x105022a0)test.year=1998; if(entry==0x104fc840)test.phase=0; out.push_back(test);}
    for(int phase:{-128,-2,-1,0,7,8,9,10,127,255}) for(int hosts:{1,2,0,3}) {
        Case test{0x104fa3d0,"calendar phase/hosts "+std::to_string(phase)+"/"+std::to_string(hosts)};
        test.phase=phase; test.hosts=hosts; out.push_back(test); test.optional_output=false; test.name+=" no flag"; out.push_back(test);
    }
    for(int hosts:{0,1,2,3}) for(int phase:{-1,0,1,2,5,7,8}) {
        Case test{0x104faf40,"group size "+std::to_string(hosts)+"/"+std::to_string(phase)};test.hosts=hosts;test.phase=phase;out.push_back(test);
    }
    for(int n:{0,1,-1,9,-9,65535,-65535,65536,-65536,2147483647,(-2147483647-1)}) for(int rng:{0,7}) {
        Case test{0x104fbe40,"RNG "+std::to_string(n)+"/"+std::to_string(rng)};test.bound=n;test.rng=rng;test.seed_value=rng?65535:0;out.push_back(test);
    }
    for(int year:{1998,2000}) for(int count:{0,1,49,50}) for(int hosts:{1,2}) {
        Case test{0x104fb050,"draw "+std::to_string(year)+"/"+std::to_string(count)+"/"+std::to_string(hosts)};test.year=year;test.count=count;test.hosts=hosts;out.push_back(test);
    }
    for(auto entry:{0x104fe1d0u,0x104fe5a0u}) for(int records:{0,1,3}) for(int failure=0;failure<11;++failure) {
        Case test{entry,"IO records/failure "+std::to_string(records)+"/"+std::to_string(failure)};test.file_records=records;test.fail_io=failure;out.push_back(test);
    }
    for(int phase:{-1,0,7,8,9}) for(int event:{-2,0,1,2,7}) {
        Case test{0x104fa980,"notification "+std::to_string(phase)+"/"+std::to_string(event)};test.phase=phase;test.event=event;out.push_back(test);
    }
    for(int a:{-1,13,14,15,255}) for(int b:{13,14}) {
        Case test{0x104fefe0,"category "+std::to_string(a)+"/"+std::to_string(b)};test.stat18=a;test.stat19=b;out.push_back(test);
    }
    for(auto entry:{0x104f9fa0u,0x104fa3d0u,0x104fc840u,0x104fcbc0u}) for(int failure:{1,2}) {
        Case test{entry,"allocation failure "+std::to_string(failure)};test.fail_allocation=failure;
        if(entry==0x104fc840)test.phase=0;out.push_back(test);
    }
    for(int method:{0x20,0x94}) for(int value:{-1,0,1}) {
        Case test{0x104f9fa0,"constructor virtual "+std::to_string(method)+"/"+std::to_string(value)};
        test.virtual_method=method;test.virtual_result=value;out.push_back(test);
    }
    for(int deleting:{-32768,-1,0,1,32767,65535,65537}) for(int storage:{0,15}) {
        Case test{0x104fa2a0,"destructor "+std::to_string(deleting)+"/"+std::to_string(storage)};
        test.deleting=deleting;test.storage=storage;test.current=storage?2:-1;out.push_back(test);
    }
    {Case test{0x104fa2a0,"null destructor"};test.null_object=true;out.push_back(test);}
    for(int phase:{-1,0,7,8,9,10}) {
        Case test{0x104fc770,"round transition "+std::to_string(phase)};test.current=phase;out.push_back(test);
    }
    for(bool supplied:{false,true}) for(int hosts:{1,2}) {
        Case test{0x104fc840,"group external/hosts "+std::to_string(supplied)+"/"+std::to_string(hosts)};
        test.phase=0;test.supplied_list=supplied;test.hosts=hosts;out.push_back(test);
    }
    for(int hosts:{1,2}) for(int kind:{-1,1001}) {
        Case test{0x104fcbc0,"playoffs hosts/sentinel "+std::to_string(hosts)+"/"+std::to_string(kind)};
        test.hosts=hosts;test.kind=kind;out.push_back(test);
    }
    for(int kind:{999,1000,1008,1009,1020}) for(int event:{-2,0,1,7}) for(int flag:{0,1}) {
        Case test{0x104fab30,"event text "+std::to_string(kind)+"/"+std::to_string(event)+"/"+std::to_string(flag)};
        test.kind=kind;test.event=event;test.flag=flag;out.push_back(test);
    }
    for(int capacity:{0,29,30,100}) for(int kind:{999,1000,1013,1014}) for(int event:{0,1,7}) {
        Case test{0x104fc4b0,"stage text "+std::to_string(capacity)+"/"+std::to_string(kind)+"/"+std::to_string(event)};
        test.capacity=capacity;test.kind=kind;test.event=event;out.push_back(test);
    }
    for(int phase:{-1,7,9,10}) for(int rank=0;rank<=7;++rank) {
        Case test{0x104fed60,"record mapping "+std::to_string(phase)+"/"+std::to_string(rank)};
        test.phase=phase;test.event=test.kind=test.score=rank;out.push_back(test);
    }
    for(int hosts:{0,1,2,3}) for(int count:{0,1,9,10,12,14,15,18,27,36,45,50}) {
        Case test{0x104fe980,"seeding "+std::to_string(hosts)+"/"+std::to_string(count)};
        test.hosts=hosts;test.count=count;out.push_back(test);
    }
    for(int field:{1,4,5}) {
        Case test{0x104fe1d0,"invalid serialized index "+std::to_string(field)};test.bad_index=field;out.push_back(test);
    }
    for(int phase:{-1,0,7,8}) for(int year:{1997,1998,1999,2000}) for(int weekday:{4,5,6}) {
        Case test{0x105022a0,"insert phase/year/weekday "+std::to_string(phase)+"/"+std::to_string(year)+"/"+std::to_string(weekday)};
        test.phase=phase;test.year=year;test.weekday=weekday;out.push_back(test);
    }
    for(int score:{-1,0,3}) for(int year:{1997,1998,1999}) for(int day:{1,230,231,232}) {
        Case test{0x105022a0,"insert score/current date "+std::to_string(score)+"/"+std::to_string(year)+"/"+std::to_string(day)};
        test.year=1998;test.score=score;test.current_year=year;test.current_day=day;out.push_back(test);
    }
    for(auto entry:{0x104fe040u,0x104fe0b0u}) for(int flag:{0,1}) for(int record_count:{0,6}) {
        Case test{entry,"publication flag/count "+std::to_string(flag)+"/"+std::to_string(record_count)};
        test.flag=flag;test.records=record_count;out.push_back(test);
    }
    for(int hosts:{0,1,2}) for(int nations_count:{0,1,50,51,80}) for(int mode:{0,1,2}) {
        Case test{0x104fd320,"selection hosts/count/filter "+std::to_string(hosts)+"/"+std::to_string(nations_count)+"/"+std::to_string(mode)};
        test.hosts=hosts;test.nation_count=nations_count;test.filter=mode;out.push_back(test);
    }
    {Case test{0x104fd320,"selection missing host query service"};test.missing_host_service=true;out.push_back(test);}
    for(int storage=0;storage<16;++storage) for(int current:{-1,0,2}) {
        Case test{0x104fc5e0,"season storage/current "+std::to_string(storage)+"/"+std::to_string(current)};
        test.storage=storage;test.current=current;test.null_group_mask=2;out.push_back(test);
    }
    for(int year:{1996,2000,2004}) for(int current_year:{1998,2004,2010}) {
        Case test{0x104fc5e0,"season year catchup "+std::to_string(year)+"/"+std::to_string(current_year)};
        test.year=year;test.current_year=current_year;out.push_back(test);
    }
    {Case test{0x104fc5e0,"season configuration failure"};test.virtual_method=0x94;test.virtual_result=0;out.push_back(test);}
    for(int year:{1998,2000}) for(bool match:{true,false}) {
        Case test{0x104fd680,"ranked table year/match "+std::to_string(year)+"/"+std::to_string(match)};
        test.year=year;test.ranking_match=match;out.push_back(test);
    }
    for(int failure:{1,3,5}) {
        Case test{0x104fd680,"ranked table ID allocation failure "+std::to_string(failure)};test.fail_allocation=failure;out.push_back(test);
    }
    for(int started:{0,1}) for(int enabled:{0,1,256}) {
        Case test{0x104fdae0,"daily equal-date started/enabled "+std::to_string(started)+"/"+std::to_string(enabled)};
        test.year=test.current_year=2000;test.current_day=1;test.active=enabled;test.started=started;out.push_back(test);
    }
    for(auto service:{0x104a00a0u,0x1049eb30u}) {
        Case test{0x104fdae0,"daily setup service failure "+hex_address(service)};
        test.year=test.current_year=2000;test.active=1;test.service_target=service;test.service_result=0;out.push_back(test);
    }
    {Case test{0x104fdae0,"daily root allocation failure"};test.year=test.current_year=2000;test.active=1;test.fail_allocation=3;out.push_back(test);}
    {Case test{0x104fdae0,"daily ordinary date notification refresh"};test.current_year=2000;test.current_day=2;test.service_target=0x1048d460;test.service_result=1;out.push_back(test);}
    for(int count:{0,1,6}) for(int flags:{1,2,3}) {
        Case test{0x104fe0b0,"publication finals count/flags "+std::to_string(count)+"/"+std::to_string(flags)};
        test.final_count=count;test.final_flags=flags;out.push_back(test);
    }
    for(int result:{0,1,256}) {
        Case test{0x104fe040,"finalization virtual low-byte result "+std::to_string(result)};test.virtual_method=0x18;test.virtual_result=result;out.push_back(test);
    }
    for(bool missing:{false,true}) for(int type:{163,164,165}) {
        Case test{0x104fab30,"event missing/type "+std::to_string(missing)+"/"+std::to_string(type)};
        test.kind=999;test.no_record=missing;test.record_kind=type;test.flag=1;out.push_back(test);
    }
    for(auto entry:{0x104fef80u,0x104fefe0u}) for(double stat:{-1.5,13.75,14.0,14.75,255.0,256.0,
            std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}) {
        Case test{entry,"FP category "+std::to_string(stat)};test.stat18=stat;test.stat19=stat;out.push_back(test);
    }
    for(int phase:{-1,9,10,255}) {
        Case test{0x104fed60,"independent rank-byte mapping "+std::to_string(phase)};
        test.phase=phase;test.event=0x101;test.kind=0x102;test.score=0x106;out.push_back(test);
    }
    for(int phase:{-2,-1,0}) {
        Case test{0x105022a0,"insert mapped invalid phase "+std::to_string(phase)};
        test.phase=phase;test.year=1998;if(phase==-1)test.current=-2;if(phase==0)test.null_group_mask=1;out.push_back(test);
    }
    {Case test{0x105022a0,"insert missing group array"};test.year=1998;test.missing_group_array=true;out.push_back(test);}
    for(int day:{226,227,228}) {
        Case test{0x105022a0,"insert equal-year date boundary "+std::to_string(day)};test.year=test.current_year=1998;test.current_day=day;out.push_back(test);
    }
    for(int phase:{-1,9}) {
        Case test{0x104fa980,"notification without optional output "+std::to_string(phase)};test.phase=phase;test.optional_output=false;out.push_back(test);
    }
    {Case test{0x104fe1d0,"load negative serialized count"};test.file_records=-1;out.push_back(test);}
    for(auto entry:{0x104ff090u,0x104fb050u}) for(int failure:{1,2}) {
        Case test{entry,"handled allocation prefix failure "+std::to_string(failure)};test.fail_allocation=failure;out.push_back(test);
    }
    for(int year:{1996,1997,1998,1999}) {
        Case test{0x104f9fa0,"constructor year residue "+std::to_string(year)};test.year=year;out.push_back(test);
    }
    {Case test{0x104fa2a0,"destructor null groups and storage"};test.missing_group_array=true;test.current=-1;test.storage=0;out.push_back(test);}
    {Case test{0x104fa2a0,"destructor null intermediate group"};test.current=2;test.null_group_mask=2;out.push_back(test);}
    {Case test{0x104fa980,"finals notification without output"};test.phase=9;test.event=1;test.optional_output=false;out.push_back(test);}
    {Case test{0x104fab30,"two-host best runner-up text"};test.kind=1020;test.event=7;test.flag=1;test.hosts=2;out.push_back(test);}
    {Case test{0x104fc840,"group supplied list with absent calendar"};test.phase=0;test.supplied_list=true;test.virtual_method=0x44;test.virtual_result=0;out.push_back(test);}
    {Case test{0x104fd320,"selection no prior entrant allocation"};test.storage=7;out.push_back(test);}
    {Case test{0x104fd320,"selection first host missing second present"};test.hosts=1;test.host_missing_first=true;out.push_back(test);}
    {Case test{0x104fd320,"selection allocation failure"};test.fail_allocation=1;out.push_back(test);}
    for(int records_count:{6,10}) for(int summary:{0,20}) {
        Case test{0x104fcbc0,"playoff qualification scans "+std::to_string(records_count)+"/"+std::to_string(summary)};
        test.records=records_count;test.summary_offset=summary;out.push_back(test);
    }
    for(int current_year:{1997,1998,1999}) for(int current_day:{1,2}) {
        Case test{0x104fcbc0,"playoff historical date "+std::to_string(current_year)+"/"+std::to_string(current_day)};
        test.hosts=2;test.current_year=current_year;test.current_day=current_day;test.anchor_year=1998;test.anchor_day=1;out.push_back(test);
    }
    for(int phase:{0,7}) {
        Case test{0x105022a0,"insert past group fixture "+std::to_string(phase)};test.year=1998;test.current_year=1999;test.phase=phase;out.push_back(test);
    }
    {Case test{0x104fdfc0,"skip record append beyond group phase"};test.phase=9;out.push_back(test);}
    {Case test{0x104fe980,"seed rankings missing fixtures"};test.storage=14;out.push_back(test);}
    {Case test{0x104fdae0,"daily later year"};test.current_year=2001;test.current_day=1;out.push_back(test);}
    for(int bound:{429496730,429496729,65536,-65536,2147483647,(-2147483647-1)}) for(int seed_value:{-1,(-2147483647-1)}) {
        Case test{0x104fbe40,"wide RNG wrapped product/seed "+std::to_string(bound)+"/"+std::to_string(seed_value)};
        test.bound=bound;test.seed_value=seed_value;out.push_back(test);
    }
    return out;
}
}
int main(int argc,char** argv) {
    if(argc<2) {std::cerr<<"Usage: cm-euro-champ-qual-checks <original-segments> [--entry HEX] [--filter TEXT]\n";return 2;}
    try {
        std::uint32_t selected=0;std::string filter;
        for(int i=2;i<argc;++i) {
            if(std::string(argv[i])=="--entry" && i+1<argc)selected=std::stoul(argv[++i],nullptr,0);
            else if(std::string(argv[i])=="--filter" && i+1<argc)filter=argv[++i];
            else throw std::runtime_error("unknown checker option");
        }
        auto base=std::make_unique<State>();load_program(*base,argv[1]);dispatch_probe=probe;
        unsigned failures=0,completed=0;
        for(const auto& test:cases()) {
            if((selected&&selected!=test.entry)||(!filter.empty()&&test.name.find(filter)==std::string::npos))continue;
            active=test;allocations=calls=io_calls=0;next_allocation=allocation_base;
            auto state=cm::semantic_io_checks::clone(*base);Cpu c(*state);initialize(c);state->pc=test.entry;
            const auto differences=compare_routine_call(*state,test.entry,original,rewrite,result_kind(test.entry)|RoutineChecker::result_f1);
            std::cout<<(differences.empty()?"ok ":"DIFFERS ")<<hex_address(test.entry)<<' '<<test.name<<'\n';
            for(const auto& difference:differences)std::cout<<"  "<<difference<<'\n';
            failures+=!differences.empty();++completed;
        }
        if(!selected&&filter.empty()) for(corruption=0;corruption<4;++corruption) {
            active={corruption==3?0x104fe5a0u:0x104fbe40u,"negative control"};
            allocations=calls=io_calls=0;next_allocation=allocation_base;
            auto state=cm::semantic_io_checks::clone(*base);Cpu c(*state);initialize(c);
            const auto differences=compare_routine_call(*state,active.entry,original,corrupted,result_kind(active.entry)|RoutineChecker::result_f1);
            const bool detected=std::ranges::any_of(differences,[&](const std::string& value){const auto at=corruption==1?cursor:corruption==2?seed:file+0x100; const auto prefix="byte "+hex_address(at).substr(2)+" "; return corruption==0?value.starts_with("r3 "):value.starts_with(prefix);});
            failures+=!detected;std::cout<<(detected?"ok ":"FAIL ")<<"negative control "<<corruption<<'\n';
        }
        if(!completed)throw std::runtime_error("no fixture selected");
        std::cout<<completed<<" returned comparison attempts, "<<failures<<" failed\n";
        return failures?1:0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 2;}
}
