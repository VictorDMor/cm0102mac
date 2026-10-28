// Returned comparisons against hook-free original CLUB_RECORDS pages.
// Scripted external dates/deallocation characterize the owned call boundaries.
#include "club_records_adapter.hpp"
#include "guest_abi.hpp"
#include "semantic_io_checks.hpp"
#include "routine_check.hpp"
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>

#if defined(CM_CLUB_RECORDS_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
#endif
using namespace cm::aot;
using guest::Cpu;
namespace cm::aot { bool club_records_reference(State&); }
namespace {
constexpr std::uint32_t data=0x60000000, object=data+0x1000, source=data+0x3000,
    rows=data+0x5000, trace_at=data+0x3f0000, stack=0x71010000, sentinel=0xfffffff0,
    clubs=data+0x80000,players=data+0x90000,competitions=data+0xa0000,
    stream=data+0x200000,message=data+0x300000,output=data+0x320000;
struct Case {
    std::uint32_t entry;
    int variant=0, overlap=0;
};
Case active{};
bool original_mode=false;
unsigned allocation_count=0,io_count=0,stream_position=0,allocation_position=0,history_cursor=0,disc_attempts=0;
void trace(Cpu& c,std::uint32_t target) {
    if(std::getenv("CM_CLUB_RECORDS_TRACE")){std::cerr<<(original_mode?'O':'C')<<' '<<hex_address(target)<<' '<<hex_address(c.lr());for(unsigned r=3;r<32;++r)if(r<=10||r>=13)std::cerr<<' '<<hex_address(c.r(r));for(unsigned r=0;r<8;++r)std::cerr<<' '<<unsigned(c.cr(r));std::cerr<<'\n';}
    auto offset=c.u32(trace_at);
    const auto add=[&](std::uint32_t value) { c.put32(trace_at+4+offset,value); offset+=4; };
    add(target); add(c.lr());
    for(unsigned r=3;r<=10;++r) add(c.r(r));
    for(unsigned r=13;r<32;++r) add(c.r(r));
    for(unsigned field=0;field<8;++field) add(c.cr(field));
    if(target==0x10780fc0 || target==0x10303980 || target==0x10303560) {
        const auto count=target==0x10780fc0?2u:1u;
        for(unsigned r=1;r<=count;++r){const auto bits=std::bit_cast<std::uint64_t>(c.f(r));add(bits>>32);add(bits);}
    }
    if(target==0x10727890 || target==0x1077b720)
        for(unsigned at=0x38;at<=0x60;at+=4)add(c.u32(c.sp()+at));
    c.put32(trace_at,offset);
}
bool probe(State& s) {
    Cpu c(s);const auto target=s.pc;
    // Cpu::call executes lowered leaves before the probe. Give reference
    // calls the same real leaf behavior before scripting external services.
    if(run_leaf(s))return true;
    const auto field_write=[&](std::uint32_t container,unsigned index,std::uint32_t value){c.put32(container+index*4,value);};
    const auto field_read=[&](std::uint32_t container,unsigned index){return c.u32(container+index*4);};
    const std::array services{
        0x10773520u,0x10775130u,0x107475b0u,0x10747a80u,0x10747cc0u,0x10747900u,
        0x10746280u,0x10748bb0u,0x1077fee0u,0x10082f20u,0x10192610u,0x1030d6e0u,
        0x100ce320u,0x103054e0u,0x10303980u,0x100d9480u,0x100d94d0u,0x100da0b0u,0x100da050u,
        0x102cb4b0u,0x102cb570u,0x1030dcd0u,0x1030dee0u,0x1030e110u,0x1030e2a0u,0x1030e450u,0x1030e5d0u,
        0x107156b0u,0x10064030u,0x100d12d0u,0x1073ef70u,0x1014c490u,0x10149c90u,0x1073e4b0u,0x1073f1e0u,
        0x102cdbf0u,0x102bdf90u,0x102cc400u,0x102cc140u,0x10749fd0u,0x10738e30u,0x10738c20u,
        0x10191fd0u,0x1049eb30u,0x1014c2e0u,0x10450b60u,0x100c76c0u,0x10748ce0u,0x103865b0u};
    if(std::ranges::find(services,target)!=services.end()) {
        trace(c,target);
        if(target==0x10775130 || target==0x10773520) {
            ++allocation_count;const auto size=c.r(3),at=data+0x100000+allocation_position;
            const bool persistence=active.entry==0x1001a690 || active.entry==0x1001b120;
            const bool fail=persistence?(active.variant>=16 && active.variant<=19 && allocation_count==unsigned(active.variant-15)):(active.variant==4 && active.entry!=0x1000eb40)||(active.entry==0x1000e6b0 && active.variant==14 && allocation_count==2);
            allocation_position+=(size+31)&~15u;c.set(3,fail?0:at);
        }else if(target==0x107475b0)c.set(3,active.variant==((active.entry==0x1001a690||active.entry==0x1001b120)?12:7)?0:data+0xe000);
        else if(target==0x10747a80 || target==0x10747cc0) {
            ++io_count;auto count=c.r(5);if(active.variant>0 && unsigned(active.variant)==io_count && count)--count;
            const auto bytes=count*c.r(4);
            if(target==0x10747a80)s.memory.copy_in(c.r(3),s.memory.copy_out(stream+stream_position,bytes));
            else s.memory.copy_in(stream+stream_position,s.memory.copy_out(c.r(3),bytes));
            stream_position+=bytes;c.set(3,count);
        }else if(target==0x10747900)c.put32(c.r(3),0);
        else if(target==0x10746280)c.set(3,data+0xe100);
        else if(target==0x10748bb0)c.put32(c.r(3),data+0xe200);
        else if(target==0x1077fee0){c.put32(c.sp()+0x14,c.r(2));c.set(3,active.entry==0x1000e6b0 && active.variant==13 && disc_attempts++==0?1:0);}
        else if(target==0x10082f20)c.set(3,active.variant==0?0:data+0xe300);
        else if(target==0x10192610)c.set(3,2002);
        else if(target==0x1030d6e0) {
            const auto destination=c.r(4);for(unsigned i=0;i<3;++i)c.put32(destination+i*4,c.u32(c.r(3)+0x28+i*4));c.put32(destination+12,c.u32(c.r(3)));
            c.put16(destination+16,200);c.put16(destination+18,2002);c.put8(destination+20,0);c.set(3,1);
        }else if(target==0x100ce320)c.set(3,active.entry==0x1001bb50&&active.variant==20?clubs+0x248:clubs);
        else if(target==0x103054e0)c.set(3,active.variant==0||(active.entry==0x1000eb40&&(active.variant==71||active.variant==75))?0:data+0xe400);
        else if(target==0x10303980)c.set_f(1,c.r(4)==0?0.0:active.entry==0x1000eb40&&active.variant==56&&c.r(5)==2?1.0:active.entry==0x1000eb40&&active.variant==56&&c.r(5)==12?2.0:active.entry==0x1000eb40&&(active.variant==57||active.variant==58)&&c.r(5)==11?1.0:active.entry==0x1000eb40&&active.variant==57&&c.r(5)==10?1.0:c.r(5)==1?10.0:7.5);
        else if(target==0x100d9480 || target==0x100d94d0 || target==0x100da0b0 || target==0x100da050)c.set(3,active.variant==18?(target==0x100d94d0||target==0x100da050):unsigned(active.variant)&1);
        else if(target==0x102cb4b0)field_write(c.r(3),c.r(4),c.r(5));
        else if(target==0x102cb570)c.set(3,field_read(c.r(3),c.r(4)));
        else if(target==0x1030e110 || target==0x1030dcd0 || target==0x1030e450) {
            const auto words=target==0x1030dcd0?9u:target==0x1030e110?6u:5u;
            auto index=c.u8(c.r(5));for(unsigned i=0;i<words;++i)field_write(c.r(4),index++,c.u32(c.r(3)+i*4));
            c.put8(c.r(5),index);c.set(3,1);
        }else if(target==0x1030e2a0 || target==0x1030dee0 || target==0x1030e5d0) {
            const auto words=target==0x1030dee0?9u:target==0x1030e2a0?6u:5u;
            auto index=c.u8(c.r(4));for(unsigned i=0;i<words;++i)c.put32(c.r(5)+i*4,field_read(c.r(3),index++));
            c.put8(c.r(4),index);c.set(3,1);
        }else if(target==0x107156b0) {c.put16(c.r(3),c.r(4));c.put16(c.r(3)+2,c.r(5));c.put32(c.r(3)+4,0x12345678);}
        else if(target==0x10064030){for(unsigned i=0;i<16;++i)c.put8(c.r(3)+i,i);}
        else if(target==0x100d12d0 || target==0x1014c490 || target==0x1014c2e0 || target==0x102cdbf0)c.set(3,unsigned(active.variant)&1);
        else if(target==0x1073ef70)c.set(3,c.r(4)*3);
        else if(target==0x10149c90)c.set(3,data+0xc0000);
        else if(target==0x1073e4b0)c.set(3,active.variant!=0);
        else if(target==0x1073f1e0){field_write(data+0x340000,c.r(4),c.r(5));c.set(3,1);}
        else if(target==0x102bdf90){for(unsigned i=0;i<0xc0;i+=4)c.put32(c.r(3)+i,0);c.put32(c.r(3),c.r(4));}
        else if(target==0x10749fd0)c.set(3,active.entry==0x10019720 && active.variant>=2?data+0xee00:0);
        else if(target==0x10738e30)c.set(3,history_cursor++<(active.variant==6?500u:1u)?data+0xd0000:0);
        else if(target==0x10191fd0)c.set(3,0);
        else if(target==0x10450b60)c.set(3,active.entry==0x10011750&&active.variant>=23?data+0xc1800:data+0xc1000);
        else if(target==0x100c76c0)c.set(3,clubs);
        c.finish();return true;
    }
    const std::array match_services{0x104abf40u,0x10464160u,0x10494d20u,0x1030d4a0u,0x1030d860u,0x1030d960u,0x1030da60u,
        0x104646d0u,0x100ce250u,0x10303560u,0x10306a80u,0x103f43b0u,0x10717dd0u,0x10780fc0u,0x1077b770u,
        0x100d9960u,0x100d95e0u};
    if(std::ranges::find(match_services,target)!=match_services.end()) {
        trace(c,target);
        if(target==0x104abf40)c.set(3,active.variant==2);
        else if(target==0x10464160)c.set(3,(unsigned(active.variant)/8)&1);
        else if(target==0x10494d20)c.set(3,active.variant==3||active.variant==68||active.variant==72);
        else if(target==0x104646d0)c.set(3,active.variant!=5);
        else if(target==0x100ce250)c.set(3,active.variant==40);
        else if(target==0x1030d860 || target==0x1030d960 || target==0x1030da60)c.set(3,active.variant==54?(c.r(4)>=data+0x20000&&c.r(4)<data+0x21000):active.variant==83?0:active.variant&1);
        else if(target==0x1030d4a0) {
            const auto at=c.r(5),own=active.variant==73||active.variant==83?8u:active.variant==76?3u:active.variant%3==0?8u:active.variant%3==1?0u:2u,other=2u;const auto home=c.u32(c.r(4))==0;
            c.put16(at,200);c.put16(at+2,2002);c.put8(at+4,0);c.put32(at+8,0);c.put16(at+12,0);c.put16(at+14,0);c.put8(at+16,home?1:2);
            c.put32(at+20,c.u32(c.r(4)));c.put32(at+24,home?1:0);c.put8(at+28,own);c.put8(at+29,other);c.put32(at+32,7);
        }else if(target==0x10303560)c.set_f(1,12);
        else if(target==0x10306a80)c.set_f(1,c.r(5)==1?40:20);
        else if(target==0x103f43b0)c.set(3,active.variant==9);
        else if(target==0x10780fc0){c.put32(c.sp()+0x14,c.r(2));c.set_f(1,std::pow(c.f(1),c.f(2)));}
        else if(target==0x1077b770)c.set(3,0x1234);
        else if(target==0x10717dd0) {
            const auto birth_day=std::int16_t(c.r(5)>>16);const auto birth_year=c.r(5)&0xffff;
            const auto current_day=c.i16(c.r(4));auto years=c.u16(c.r(4)+2)-birth_year;auto days=current_day-birth_day;
            if(days<0){days+=365;--years;}c.put16(c.r(3),days);c.put16(c.r(3)+2,years);c.put32(c.r(3)+4,0);
        }
        c.finish();return true;
    }
    const std::array text_services{0x107467a0u,0x1077dbf0u,0x10715e00u,0x10717270u,0x104769d0u,0x10476ec0u,
        0x100cb590u,0x100cbde0u,0x1030db50u,0x100cd7c0u,0x10009f20u,0x1000b410u,
        0x1029eb20u,0x1074c570u,0x1074b830u,0x1074bab0u,0x10192350u};
    if(std::ranges::find(text_services,target)!=text_services.end()) {
        trace(c,target);
        if(target==0x107467a0){c.put32(data+0xef00,0x31535400);c.set(3,data+0xef00);}
        else if(target==0x1077dbf0){auto from=c.r(4),to=c.r(3);std::uint8_t byte;do{byte=c.u8(from++);c.put8(to++,byte);}while(byte);}
        else if(target==0x10715e00 || target==0x10717270)c.put32(c.r(4),0x44415400);
        else if(target==0x104769d0)c.put32(c.r(7),0x53544700);
        else if(target==0x10476ec0 || target==0x100cbde0 || target==0x1029eb20)c.set(3,unsigned(active.variant)&1);
        else if(target==0x100cb590)c.put32(c.r(6),0x50455200);
        else if(target==0x1030db50)c.put32(c.r(4),0x484f4d00);
        else if(target==0x100cd7c0)c.put32(c.r(5),0x41474500);
        else if(target==0x10192350)c.put32(c.r(5),0x59454100);
        else if(target==0x10009f20){const auto value=c.u32(c.r(3));c.set(3,0);c.set(4,value);}
        else if(target==0x1000b410)c.put32(c.r(3),0x4d4f4e00);
        else if(target==0x1074c570)c.put32(c.r(3),0);
        else if(target==0x1074b830 || target==0x1074bab0)c.put32(c.r(3),c.u32(c.r(3))+1);
        c.finish();return true;
    }
    if(target==0x10715880 || target==0x107735a0 || target==0x10775170 ||
       target==0x10748c80 || target==0x107751b0 || target==0x10739890 ||
       target==0x10768810 || target==0x1077b720 || target==0x10769b60 ||
       target==0x1076f7d0 || target==0x10727890 || target==0x100c7d00) {
        trace(c,target);
        if(target==0x10715880) { c.put16(c.r(3),0xffff);c.put16(c.r(3)+2,0xffff);c.put8(c.r(3)+4,0xff); }
        else if(target==0x107751b0)c.set(3,active.variant==4 && active.entry!=0x1000eb40?0:data+0x30000);
        else if(target==0x10739890) {c.put32(c.r(6),0x434c5542);c.put32(c.r(7),0x43505000);}
        else if(target==0x10768810)c.set(3,data+0x9000);
        else if(target==0x1077b720)c.put32(c.r(3),c.r(8));
        else if(target==0x10727890) {
            auto from=c.r(4),to=c.r(3);std::uint8_t byte;
            do {byte=c.u8(from++);c.put8(to++,byte);}while(byte);
        } else if(target==0x100c7d00)c.set(3,active.overlap!=0 || (active.entry==0x1000eb40&&(active.variant==20||active.variant==21||active.variant==40||active.variant==72||(active.variant>=65&&active.variant<=67))) || (active.entry==0x100163f0&&active.variant/34==17) || (active.entry==0x10013650&&active.variant==6) || ((active.entry==0x10011750||active.entry==0x10011bd0||active.entry==0x1001d410)&&active.variant==18) || (active.entry==0x100134b0&&active.variant==13));
        c.finish();return true;
    }
    if(original_mode && target>=0x1000e6b0 && target<0x1001dae0) return club_records_reference(s);
    if(club_records_lists(target)) return club_records_dispatch_probe(s);
    return false;
}
void initialize(State& s,const Case& test) {
    s.memory.map(data,0x420000);s.memory.map(stack-0x10000,0x20000);
    std::vector<std::uint8_t> bytes(0x400000);
    for(unsigned i=0;i<bytes.size();++i)bytes[i]=std::uint8_t((i*37+(i>>3)*11+test.variant*17)^0xa5);
    s.memory.copy_in(data,bytes);s.memory.copy_in(stack-0x10000,std::vector<std::uint8_t>(0x20000,0x5a));
    Cpu c(s);for(unsigned r=0;r<32;++r)c.set(r,0x51000000+r*0x101);
    for(unsigned field=0;field<8;++field)c.set_cr(field,field+1);
    c.set(1,stack);c.set(2,0x108bba90);c.set_lr(sentinel);c.put32(trace_at,0);
    allocation_count=io_count=stream_position=allocation_position=history_cursor=disc_attempts=0;
    c.set({{3,object},{4,source+std::uint32_t(test.overlap)}});
    if(test.overlap!=0)c.set(4,object+std::uint32_t(test.overlap));
    if(test.overlap==999)c.set(4,object);
    const auto variable=[&](int slot,unsigned offset,std::uint32_t value) {
        c.put32(c.r(2)+std::uint32_t(slot),data+offset);c.put32(data+offset,value);
    };
    variable(-0x7bac,0x100,1);c.put32(c.r(2)-0x7ba8,data+0xa000);
    if(test.entry!=0x1000ea50 && !(test.entry>=0x100121c0&&test.entry<0x10012e90) &&
       !(test.entry>=0x100190b0&&test.entry<0x10019720) && test.entry!=0x10019ca0 && test.entry!=0x100163b0 && test.entry!=0x1001daa0) {
        variable(-0x7be8,0x104,clubs);variable(-0x7be0,0x108,players);variable(-0x7b50,0x10c,2);
        variable(-0x7be4,0x110,2);variable(-0x7b3c,0x114,data+0xe500);variable(-0x7b7c,0x118,data+0xe600);
        variable(-0x7b78,0x11c,0);variable(-0x7bf0,0x120,data+0xe700);variable(-0x7bbc,0x124,competitions);
        variable(-0x7b64,0x128,2);variable(-0x7b48,0x12c,data+0xe800);variable(-0x7a54,0x130,data+0xe900);
        c.put32(c.r(2)-0x7b44,data+0xc2000);c.put32(c.r(2)-0x7bb0,data+0xc3000);
        c.put32(c.r(2)-0x7bb8,data+0xc4000);variable(-0x7b4c,0x134,data+0xc5000);
        c.put32(c.r(2)-0x7b80,data+0xc6000);c.put16(data+0xc6000,200);c.put16(data+0xc6002,2002);
        for(unsigned offset=0;offset<0x30;offset+=4)c.put32(object+offset,0);
        c.put32(object,data+0xe200);c.put32(object+4,data+0x10000);c.put32(object+8,2);c.put32(object+12,2);
        c.put32(object+16,data+0x20000);c.put32(object+20,2);c.put32(object+24,2);c.put32(object+28,rows);c.put32(object+32,2);
        c.put32(object+36,rows+0x1000);c.put32(object+40,5);c.put32(object+44,0);
        for(unsigned i=0;i<2;++i) {
            const auto club=clubs+i*0x248;for(unsigned off=0;off<0x248;off+=4)c.put32(club+off,0);
            c.put32(club,i);c.put16(club+0x1ec,3000);c.put8(club+0x23b,1);
            c.put32(rows+i*0x6c,i);c.put32(rows+i*0x6c+4,i);c.put32(rows+i*0x6c+8,0);c.put32(rows+i*0x6c+12,0);
            c.put8(rows+i*0x6c+16,10);c.put16(rows+i*0x6c+0x68,6);
            for(auto base:{data+0x10000+i*0x3fc,data+0x20000+i*0x3fc}) {
                for(unsigned off=0;off<0x3fc;off+=4)c.put32(base+off,0);
                c.put_f32(base+0x23c,7.0f);c.put32(base+0x378,0xffffffff);c.put32(base+0x3a4,0xffffffff);
                c.put16(base+0x29c,0xffff);c.put16(base+0x2b8,0xffff);c.put16(base+0x35c,0xffff);
            }
            c.put32(competitions+i*0x6c,i);c.put16(competitions+i*0x6c+24,100+i);c.put8(competitions+i*0x6c+0x67,1);
            const auto player=players+i*0x70;for(unsigned off=0;off<0x70;off+=4)c.put32(player+off,0);
            c.put32(player,i);c.put32(player+20,data+0xe400);c.put32(player+0x24,club);
            c.put32(club+0x98,player);c.put8(player+0x5e,3);c.put32(player+12,data+0xe000);
        }
        c.put8(data+0xe400,10);c.put8(data+0xe402,2);c.put16(data+0xe404,5);
        c.put8(data+0xe406,1);c.put16(data+0xe40a,10);c.put16(data+0xe40c,20);c.put16(data+0xe40e,80);
        c.put32(data+0xe304,1);for(unsigned i=0;i<16;++i)c.put8(data+0xc0000+i,i+3);
        c.put8(data+0xc1000+0x43,1);
        c.set({{3,object},{4,clubs},{5,1},{6,1},{7,1}});
        switch(test.entry) {
        case 0x1000e6b0:
            // The original CD-name interleave reads the byte after the copied
            // Pascal name. It is caller stack input, not a trailing byte the
            // constructor initializes itself.
            c.put8(stack-0x3c0+0xa9,0);c.set(4,0);break;
        case 0x1000eb40: {
            const auto match=data+0xd1000,stats=data+0xd2000;
            variable(-0x7b34,0x138,data+0xe800);variable(-0x7b54,0x13c,data+0xe900);
            variable(-0x7b38,0x140,data+0xd4000);variable(-0x7b40,0x144,test.variant==9?data+0xe900:0);
            variable(-0x7b90,0x148,0);variable(-0x7b88,0x14c,data+0xdf010);variable(-0x7b84,0x150,test.variant==10?data+0xdf010:data+0xdf000);
            c.put32(c.r(2)-0x7b8c,data+0xdf000);for(unsigned i=0;i<10;++i)c.put16(data+0xdf000+i*2,test.variant>=11?0:65000);
            for(unsigned i=0;i<0x1000;i+=4)c.put32(stats+i,0);
            for(unsigned side=0;side<2;++side)for(unsigned i=0;i<20;++i){const auto row=stats+0x194+side*0x664+i*0x4c;c.put32(row,0xffffffff);c.put32(row+4,0xffffffff);}
            for(unsigned i=0;i<0x50;i+=4)c.put32(match+i,0);
            c.put32(match,competitions);c.put32(match+20,competitions);c.put32(match+28,clubs);c.put32(match+32,clubs+0x248);c.put16(match+0x30,150);c.put8(match+0x40,1);
            c.put32(stats+0x34,50000);c.put32(stats+0x38,700000);c.put32(stats+0x3c,0);
            if(test.variant>=4)for(unsigned side=0;side<2;++side){const auto row=stats+0x194+side*0x664;c.put32(row,side);c.put32(row+4,side);c.put8(row+0x21,4);c.put8(row+0x27,2);c.put8(row+0x2c,1);c.put8(row+0x2d,1);c.put8(row+0x35,10);}
            for(unsigned side=0;side<2;++side){const auto row=rows+side*0x6c;for(unsigned i=16;i<0x68;i+=4)c.put32(row+i,0);c.put16(row+18,50);
                const auto birth=players+side*0x70+0x34;c.put16(birth,100);c.put16(birth+2,test.variant&1?1984:1960);c.put8(birth+4,0);
                c.put32(clubs+side*0x248+0x74,clubs+(1-side)*0x248);
                if(test.variant>=11)c.put32(clubs+side*0x248+0x9c,players+(1-side)*0x70);}
            if(test.variant>=16) {
                c.put16(match+0x30,0);
                for(unsigned side=0;side<2;++side){const auto club=clubs+side*0x248,row=rows+side*0x6c,player=players+side*0x70,card=stats+0x194+side*0x664;
                    if(test.variant==16||test.variant==17){c.put32(club+0x74,0);c.put16(competitions+24,5);}
                    if(test.variant==18)c.put16(competitions+24,0);
                    if(test.variant==19)c.put32(match+20,0);
                    if(test.variant==21)c.put8(match+0x40,0);
                    if(test.variant==23)c.put16(row+0x68,2);
                    if(test.variant==27)c.put32(stats+0x3c,99);
                    if(test.variant==28||test.variant==35){c.put8(card+0x35,7);c.put8(card+0x21,test.variant==35?1:0);c.put8(card+0x27,0);c.put8(card+0x2c,0);c.put8(card+0x2d,0);}
                    if(test.variant==29){c.put8(card+0x35,7);c.put8(card+0x21,0);c.put8(card+0x27,0);}
                    if(test.variant==30){c.put32(card,0xffffffff);c.put32(card+11*0x4c,side);c.put32(card+11*0x4c+4,side);c.put8(card+11*0x4c+0x32,0xff);}
                    if(test.variant==31)c.put32(card+4,0xffffffff);
                    if(test.variant==32)variable(-0x7be4,0x110,1);
                    if(test.variant==37)c.put16(player+0x36,1986);
                    if(test.variant==38)c.put16(player+0x36,1966);
                    if(test.variant==39)c.put16(row+18,48);
                    if(test.variant==41)c.put32(club+0x74,0),c.put32(club+0x78,clubs+(1-side)*0x248);
                    if(test.variant==42)c.put32(club+0x74,0),c.put32(club+0x7c,clubs+(1-side)*0x248);
                    if(test.variant>=44) {
                        for(unsigned off:{0x18,0x2c,0x40,0x54}){c.put32(row+off,off<=0x2c?5:11);c.put16(row+off+4,100);c.put16(row+off+6,2001);c.put8(row+off+8,0);}
                        for(unsigned off:{0x14c,0x160,0x174,0x188}) {
                            const auto a=data+0x10000+side*0x3fc+off,b=data+0x20000+side*0x3fc+off;
                            c.put32(a,test.variant==51?100:0);c.put32(b,test.variant==50?100:0);
                            c.put16(a+4,test.variant%2?100:99);c.put16(a+6,2001);c.put8(a+8,0);
                        }
                    }
                    if(test.variant==53){for(unsigned off:{0x19c,0x1bc,0x1dc,0x21c,0x25c,0x27c})c.put8(data+0x10000+side*0x3fc+off,100);c.put32(data+0x10000+side*0x3fc+0x18,1000000);c.put32(data+0x10000+side*0x3fc+0x40,1000000);c.put32(data+0x10000+side*0x3fc+0x68,1);}
                    if(test.variant==55){c.put8(card+0x35,7);c.put8(card+0x21,0);c.put8(card+0x27,0);c.put8(card+0x2c,0);c.put8(card+0x2d,1);}
                    if(test.variant>=58&&test.variant<=60){for(unsigned off:{0x29c,0x2b8}){c.put16(data+0x20000+side*0x3fc+off,test.variant==59?100:10);c.put16(data+0x10000+side*0x3fc+off,test.variant>=59?100:10);}}
                    if(test.variant>=61&&test.variant<=64){c.put16(player+0x36,1984);for(auto base:{data+0x10000+side*0x3fc,data+0x20000+side*0x3fc})for(unsigned off:{0x2d4,0x318}){c.put16(base+off,test.variant==62?99:test.variant==63?101:100);c.put16(base+off+2,1984);c.put16(base+off+32,200);c.put16(base+off+34,2002);if(test.variant==64&&base==data+0x20000+side*0x3fc)c.put16(base+off+2,off==0x2d4?1970:1990);}}
                    if(test.variant>=65&&test.variant<=67)c.put16(data+0x10000+side*0x3fc+0x35c,test.variant==65?2:test.variant==66?10:3);
                    if(test.variant==69)c.put16(match+0x32,1);
                    if(test.variant==70)c.put16(competitions+24,4);
                    if(test.variant==74)for(unsigned off:{0x150,0x164,0x178,0x18c})c.put16(data+0x10000+side*0x3fc+off+2,2000);
                    if(test.variant==75)c.put32(match+20,0);
                    if(test.variant==76)c.put8(card+0x21,1);
                    if(test.variant==77)for(auto base:{data+0x10000+side*0x3fc,data+0x20000+side*0x3fc})for(unsigned off:{0x19c,0x1bc,0x1dc,0x21c,0x25c,0x27c})c.put8(base+off,0xff);
                    if(test.variant==78||test.variant==79){c.put8(card+0x35,test.variant==78?7:8);c.put8(card+0x21,test.variant==78?0:1);c.put8(card+0x27,test.variant==78?0:1);c.put8(card+0x2c,0);c.put8(card+0x2d,0);c.put32(stats+0x3c,99);for(unsigned off:{0x2ac,0x2c8})c.put32(data+0x10000+side*0x3fc+off,side);c.put16(player+0x36,1984);}
                    if(test.variant==80||test.variant==81){c.put32(club+0x74,0);c.put16(competitions+24,test.variant==80?10:5);}
                    if(test.variant==81)c.put32(c.toc(-0x7b84),data+0xdf010);
                    if(test.variant==82)c.put32(players+(1-side)*0x70+20,0);
                    if(test.variant==83)for(auto base:{data+0x10000+side*0x3fc,data+0x20000+side*0x3fc})for(unsigned off:{0xc4,0x130})c.put32(base+off,0xffffffff);
                    for(auto base:{data+0x10000+side*0x3fc,data+0x20000+side*0x3fc}) {
                        if(test.variant==22||test.variant==35){for(auto off:{0x18,0x40,0x68,0x14c,0x160,0x174,0x188})c.put32(base+off,1000000);for(auto off:{0x19c,0x1bc,0x1dc,0x21c,0x25c,0x27c})c.put8(base+off,100);}
                        if(test.variant==25||test.variant==37||test.variant==38)for(auto off:{0x18,0x40,0x68,0xa0,0xc4,0xe8,0x10c,0x130,0x2fc,0x340})c.put32(base+off,0xffffffff);
                        if(test.variant==33){for(auto off:{0x1ac,0x1cc,0x22c,0x26c,0x28c,0x2ac,0x2c8})c.put32(base+off,side);for(auto off:{0x1b8,0x1d8,0x238,0x278,0x298})c.put16(base+off,2002);}
                    }
                }
                if(test.variant==24||test.variant==49||test.variant==73){c.put32(object+44,64);c.put32(object+40,64);for(unsigned side=0;side<2;++side)for(unsigned kind=1;kind<=32;++kind){const auto row=rows+0x1000+(side*32+kind-1)*8;c.put32(row,side);c.put8(row+4,kind);}}
                if(test.variant==43){c.put32(stats+0x34,0);c.put32(stats+0x38,0);}
            }
            c.set({{4,test.variant==0?0:match},{5,test.variant==1?0:stats}});break;
        }
        case 0x10012e90:case 0x10012fb0:
            c.set(4,source);c.put32(source+4,1);c.put32(source+8,0);c.put32(source+12,0);c.put32(source+0x20,unsigned(test.variant)*1000000);break;
        case 0x100132f0:c.set({{4,unsigned(test.variant)*1000000},{5,clubs+0x248},{6,clubs}});break;
        case 0x100134b0:c.set({{4,players},{5,clubs},{6,10},{7,5}});break;
        case 0x10011bd0:c.set(4,data+0xc2000);break;
        case 0x10011750:
            c.set(4,data+0xc1000);c.put8(data+0xc1044,0xff);c.put32(data+0xc1004,competitions);
            c.put32(data+0xc1030,0);c.put16(data+0xc103e,1);c.put32(data+0xc10b0,data+0xc8000);
            c.put32(data+0xc8000,clubs);c.put8(data+0xc8004,2);c.put8(data+0xc8014,1);break;
        case 0x1001bec0:c.set(5,data+0x20000);break;
        case 0x10019720:
            c.set({{4,test.variant==0?0:clubs},{5,data+0xd000}});
            for(unsigned off=0;off<0x400;off+=4)c.put32(data+0xd0000+off,0);
            c.put16(data+0xd0000,2001);c.put32(data+0xee08,123);
            if(test.variant==3)c.put_f32(data+0x20000+0x23c,-1);
            if(test.variant==5)c.put32(rows,0xffffffff);
            if(test.variant==7)c.put32(rows+4,0xffffffff);
            break;
        case 0x10013650:
            c.put32(object+44,test.variant==0?0:3);if(test.variant==0)c.put32(object+36,0);if(test.variant>=4)c.put32(object+40,100);
            for(unsigned i=0;i<3;++i){const auto row=rows+0x1000+i*8;c.put32(row,i&1);c.put8(row+4,i==0?1:i==1?7:32);c.put8(row+5,i==1?2:1);}
            break;
        case 0x1001d410:c.set({{3,test.variant?clubs:0},{4,1},{5,unsigned(test.variant)&1},{6,unsigned(test.variant)&2},{7,unsigned(test.variant)&4}});break;
        case 0x1001c290:case 0x1001cb30:case 0x10014080:case 0x10013ee0:case 0x100163f0: {
            const auto kind=unsigned(test.variant)%34,scenario=unsigned(test.variant)/34,record=data+0x20000;
            for(unsigned i=0;i<0x3fc;i+=4)c.put32(record+i,0);
            for(auto offset:{0x14c,0x160,0x174,0x188})c.put32(record+offset,scenario==0?0:scenario==1?0xffffffff:5);
            for(auto offset:{0x98,0xbc,0xe0,0x104,0x128,0x1f8,0x2f4,0x338}) {
                c.put8(record+offset+16,scenario%3+1);c.put8(record+offset+28,scenario>=4?1:3);c.put8(record+offset+29,2);
                if(scenario==16){c.put8(record+offset+28,4);c.put8(record+offset+29,4);}
            }
            for(auto offset:{8,20,0x94,0x1b8,0x1d8,0x238,0x25a,0x278,0x298,0x3d4,0x3dc})c.put16(record+offset,scenario>=3?2002:0);
            c.put_f32(record+0x23c,scenario==1||scenario==11?-1:7.25f);c.put8(record+0x3a0,scenario&1);c.put8(record+0x3cc,scenario&1);
            if(scenario==7)c.put32(record+0x23c,0x7fc12345);
            if(scenario==14)c.put32(record+0x23c,0x7f800000);
            if(scenario==15)c.put32(record+0x23c,0x80000000);
            if(scenario>=19)for(unsigned off:{0x40,0x54}){c.put32(rows+off,5);c.put32(rows+off+4,scenario==20?1:scenario==21?0x10000:0);c.put32(rows+off+8,0);}
            if(scenario==1){for(auto off:{4,16,0x18,0x40,0x68,0x90,0xa0,0xc4,0xe8,0x10c,0x130,0x200,0x2fc,0x340,0x378,0x3a4,0x3d0,0x3d8,0x3ec})c.put32(record+off,0xffffffff);
                for(auto off:{0x19c,0x1bc,0x1dc,0x21c,0x25c,0x27c})c.put8(record+off,0xff);
                for(auto off:{0x29c,0x2b8,0x35c})c.put16(record+off,0xffff);}
            if(test.entry==0x1001c290)c.set({{3,0},{4,scenario==8?0:record},{5,kind},{6,scenario==9?0:message}});
            else if(test.entry==0x10014080)c.set({{4,scenario==9?0:clubs},{5,scenario==10?0:record},{6,kind},{7,scenario==8?0:output}});
            else if(test.entry==0x10013ee0)c.set({{4,scenario==9?0:clubs},{5,scenario>=11?2u:1u},{6,kind},{7,scenario==8?0:output}});
            else {
                const auto registers=s.registers;
                c.set({{3,0},{4,record},{5,kind},{6,message}});s.budget=4000000;s.pc=0x1001c290;original_mode=true;run_until(s,sentinel);
                s.registers=registers;c.put32(trace_at,0);
                if(test.entry==0x100163f0){if(scenario==10||scenario==18)c.put32(message,scenario==10?2:0xffffffff);c.set({{4,scenario==9?0:message},{5,scenario==6||scenario==17?0u:1u},{6,scenario==8?0:output}});}
                else c.set({{3,scenario==8?0:message},{4,data+0xd000},{5,scenario==9?0:record},{6,data+0xd004}});
            }
            if(test.entry==0x10013ee0&&scenario==10){c.set(5,2);c.put32(rows+4,0xffffffff);}
            break;
        }
        case 0x1001a690:case 0x1001b120: {
            c.set(4,test.variant==13?0:data+0xf000);unsigned offset=0;
            c.put32(object+44,2);for(unsigned i=0;i<2;++i){c.put32(rows+0x1000+i*8,i);c.put8(rows+0x1004+i*8,i+1);}
            if(test.variant>=20 && test.variant<=25)for(unsigned i=0;i<2;++i)c.put32(rows+i*0x6c+(test.variant==20||test.variant==24?4:test.variant==21||test.variant==25?0:test.variant==22?8:12),test.variant<=21?2:0xfffffffe);
            if(test.variant>=26 && test.variant<=27) {
                for(unsigned at:{8,12,20,24,40,44})c.put32(object+at,0);
                for(unsigned i=0;i<2;++i){c.put32(rows+i*0x6c,0xffffffff);c.put32(rows+i*0x6c+4,0xffffffff);}
                if(test.variant==27)c.put32(object+32,0);
            }
            if(test.variant==28)for(unsigned at:{12,24,44})c.put32(object+at,0);
            const auto add=[&](unsigned address,unsigned size){s.memory.copy_in(stream+offset,s.memory.copy_out(address,size));offset+=size;};
            add(object+8,4);add(object+12,4);add(data+0x10000,c.u32(object+12)*0x3fc);add(object+20,4);add(object+24,4);add(data+0x20000,c.u32(object+24)*0x3fc);
            add(object+32,4);add(rows,c.u32(object+32)*0x6c);add(object+40,4);add(object+44,4);add(rows+0x1000,c.u32(object+44)*8);
            break;
        }
        default:break;
        }
    }
    if(test.entry==0x10012650 || test.entry==0x10019ca0) {
        constexpr std::array bits{0u,0x80000000u,1u,0x7f800000u,0xff800000u,0x7fc12345u,0x7f812345u,0xff812345u};
        c.put32(c.r(4)+(test.entry==0x10019ca0?0x23c:0),bits[unsigned(test.variant)%bits.size()]);
    }
    if(test.entry==0x1001c160 && test.variant>=4)c.set(3,0);
    if(test.entry==0x1001b830 || test.entry==0x1001b9c0 || test.entry==0x1001bd60 || test.entry==0x1000ea50) {
        for(unsigned offset=0;offset<0x30;offset+=4)c.put32(object+offset,0);
        c.put32(object+4,data+0x10000);c.put32(object+0x10,data+0x20000);
        c.put32(object+0x1c,rows);c.put32(object+0x24,rows+0x1000);
        c.put32(source,1);
        if(test.entry==0x1001b830 || test.entry==0x1001b9c0) {
            const bool season=test.entry==0x1001b9c0;
            c.put32(rows+0x6c+(season?4:0),test.variant==1?0:0xffffffff);
            c.put32(object+(season?0x14:8),test.variant==2?50:0);
            c.set({{4,test.overlap?0:source},{5,test.variant>=2?1u:0u}});
        }else if(test.entry==0x1001bd60) {
            c.put32(object+0x28,test.variant==0?5:0);c.set({{4,17},{5,0xff}});
        }else {
            c.put32(object,test.variant&1?data+0xb000:0);
            for(unsigned i=0;i<4;++i)if(!(test.variant&(1<<i)))c.put32(object+std::array{4,0x10,0x1c,0x24}[i],0);
            c.set(4,test.variant&2?1:0);if(test.variant==7)c.set(3,0);
        }
    }
    if(test.entry==0x100137c0 || test.entry==0x10013910 || test.entry==0x10013cc0) {
        constexpr std::array extra{0xffffff80u,0xffffffffu,33u,127u,256u,257u,258u,0x80000000u};
        const auto kind=test.variant<33?std::uint32_t(test.variant):extra[unsigned(test.variant-33)%extra.size()];
        c.set({{4,kind},{5,source},{6,50}});
        if(test.entry==0x10013cc0){c.set(6,((test.variant&1)^(test.overlap==1))?1:2);if(test.variant==40)c.set(5,0);}
        else {if(test.overlap==1)c.set(5,0);if(test.overlap==-1)c.set(6,49);}
    }
    if(test.entry==0x100163b0 || test.entry==0x1001daa0) {
        constexpr std::array flags{0u,1u,0xffffu,0x10001u,0x7fffu,0x8000u};
        c.set(4,flags[unsigned(test.variant)%flags.size()]);if(test.variant>=6)c.set(3,0);
    }
    if(test.entry==0x1001be60) {
        c.put32(object+0x24,rows);c.put32(object+0x2c,test.variant<0?0xffffffffu:unsigned(test.variant));
        for(unsigned i=0;i<8;++i) { c.put32(rows+i*8,17+i);c.put8(rows+i*8+4,i==2?0xff:i); }
        c.set({{4,test.overlap==0?19u:99u},{5,0xffffffff}});
    }
    if(test.variant>=8) {
        const auto modern=data+0x10000,season=data+0x20000;const int v=test.variant;
        const auto notifications=[&](){c.put32(object+44,64);c.put32(object+40,64);for(unsigned side=0;side<2;++side)for(unsigned kind=1;kind<=32;++kind){const auto row=rows+0x1000+(side*32+kind-1)*8;c.put32(row,side);c.put8(row+4,kind);}};
        switch(test.entry) {
        case 0x1000e6b0:
            if(v<=12){c.set(4,data+0xf000);unsigned offset=0;const auto add=[&](unsigned at,unsigned size){s.memory.copy_in(stream+offset,s.memory.copy_out(at,size));offset+=size;};
                add(object+8,4);add(object+12,4);add(modern,2*0x3fc);add(object+20,4);add(object+24,4);add(season,2*0x3fc);add(object+32,4);add(rows,2*0x6c);add(object+40,4);add(object+44,4);}
            if(v==15)c.put8(players+0x5e,0),c.put8(players+0x70+0x5e,0);
            if(v==16)c.put32(players+12,0),c.put32(players+0x70+12,0);
            if(v==17)c.put8(players+0x70+0x5e,10);
            break;
        case 0x10011bd0:
            if(v==8)c.set(4,0);
            if(v==9)c.set(4,data+0xc204c);
            if(v==10)c.put32(rows+4,0xffffffff);
            if(v==11)c.put32(season+0x90,0xffffffff);
            if(v>=12){c.put32(season+0x90,50000);c.put32(modern+0x90,v==13?0xffffffff:0);c.put_f32(season+0x23c,v==12?-1:v==13?8:7);c.put_f32(modern+0x23c,v==13?-1:6);}
            if(v==14)c.put32(season+0x23c,0x7fc12345);
            if(v==15)c.put32(modern+0x23c,0x7fc12345);
            if(v==16)c.put16(rows+0x68,4);
            if(v==21)c.put16(rows+0x68,4);
            if(v==17)notifications();
            if(v==19)c.put32(c.toc(-0x7b50),0);
            break;
        case 0x10011750:
            if(v==8)c.set(4,0);
            if(v==9)c.put8(data+0xc1044,0);
            if(v==10)c.put16(data+0xc103e,0);
            if(v==11)c.put8(data+0xc8014,0);
            if(v==12)c.put32(data+0xc8000,0);
            if(v==13)c.set(5,0);
            if(v>=14){c.put32(season+4,0xffffffff);c.put32(modern+4,0xffffffff);c.put32(season+16,0xffffffff);c.put32(modern+16,0xffffffff);}
            if(v==15){c.put32(modern+4,3);c.put32(modern+16,3);}
            if(v>=16){c.put32(data+0xc1004,competitions+0x6c);c.put16(competitions+0x6c+24,v==16?200:50);c.put32(season+4,0);c.put32(modern+4,0);c.put32(season+16,0);c.put32(modern+16,0);}
            if(v==18){c.put16(competitions+0x6c+24,100);}
            if(v==19)notifications();
            if(v==22)c.put32(data+0xc1030,0xffffffff);
            if(v==23||v==24){for(unsigned off=0;off<0xc0;off+=4)c.put32(data+0xc1800+off,0);c.put32(data+0xc1804,competitions);c.put8(data+0xc1843,2);c.put8(data+0xc1844,0);c.put16(data+0xc183e,v==23?2:1);c.put32(data+0xc18b0,data+0xc8000);}
            if(v==25)c.put16(rows+0x68,4);
            break;
        case 0x10012e90:
            if(v==8)c.set(4,0);
            if(v==9)c.set(5,0);
            if(v==10)c.put32(source+0x20,700000);
            if(v==11)c.put32(source+0x20,0),c.put16(clubs+0x1ec,-11000),c.set(5,0);
            if(v==12)c.put32(modern+0x378,20000000);
            // A malformed negative index returns no modern record even with
            // create=1; unlike -1 it does not request a new allocation.
            if(v==21)c.put32(rows,0xfffffffe);
            break;
        case 0x10012fb0:
            if(v==8)c.put32(source+0x20,0);
            if(v==9)c.put32(season+0x378,20000000);
            if(v==10)c.put32(modern+0x378,20000000);
            if(v==11)c.put32(season+0x3fc+0x3a4,20000000);
            if(v==12)c.put32(modern+0x3fc+0x3a4,20000000);
            if(v==13)notifications();
            break;
        case 0x100132f0:
            if(v==8)c.set(5,0);
            if(v==9)c.set(6,0);
            if(v==10)c.put32(season+0x3d8,0xffffffff),c.put32(season+0x3fc+0x3d0,0xffffffff);
            if(v==11)c.put32(modern+0x3d8,20000000),c.put32(modern+0x3fc+0x3d0,20000000);
            if(v==12)c.put16(modern+0x3dc,2002);
            if(v==13)c.put32(season+0x3fc+0x3d0,6000000),c.put32(modern+0x3fc+0x3d0,5000000);
            if(v==14)notifications();
            break;
        case 0x100134b0:
            if(v==8)c.set(4,0);
            if(v==9)c.set(5,0);
            if(v==10)c.put32(rows,0xffffffff);
            if(v==11)c.put16(modern+0x29c,20),c.put16(modern+0x2b8,20);
            if(v==12)c.put16(modern+0x29c,0),c.put16(modern+0x2b8,0);
            if(v==21)c.put32(rows,0xfffffffe);
            break;
        case 0x1001bb50:case 0x1001bec0:
            if(v==8 && test.entry==0x1001bb50)c.set(4,0);
            if(v==9)c.put8(rows+16,0);
            if(v==10 && test.entry==0x1001bb50)c.put32(rows+4,0xffffffff);
            if(v==11)c.put32(players+20,0);
            if(v==12)c.put32(players,10);
            if(v==13)c.put32(players+0x24,clubs+0x248);
            if(v==14)c.put8(data+0xe400,0);
            if(v==15)c.put8(rows+16,100);
            if(v==16)c.put16(clubs+0x1ec,2999),c.put8(rows+16,0);
            if(v>=17){c.put32(clubs+0x9c,players+0x70);c.put32(clubs+0x15c,players);c.put32(players+0x70+0x24,clubs);}
            if(v==20)c.put32(players+0x24,clubs+0x248);
            if(v==21)c.put8(rows+16,0),c.put8(data+0xe400,0);
            break;
        case 0x1001d410:
            c.set(3,clubs);c.put32(clubs+0x98,0);
            if(v>=9)for(unsigned i=0;i<unsigned(v-7)&&i<5;++i){const auto card=data+0xe400+i*0x40,player=players+i*0x70;for(unsigned off=0;off<0x70;off+=4)c.put32(player+off,0);c.put32(player,i);c.put32(player+20,card);c.put32(clubs+0x98+i*4,player);c.put16(card+4,5);c.put16(card+10,unsigned(v)&1?50-i*10:10+i*10);c.put16(card+12,30);}
            if(v==20)c.set(3,0);
            if(v==21)c.put32(players+20,0);
            break;
        default:break;
        }
    }
    s.budget=4000000;s.pc=test.entry;
}
void returned(State& s) {
    Cpu c(s);if(s.pc!=sentinel || c.lr()!=sentinel || c.sp()!=stack) throw std::runtime_error("fixture did not return");
}
void original(State& s,std::uint32_t entry) {
    original_mode=true;s.pc=entry;run_until(s,sentinel);returned(s);
}
void rewrite(State& s,std::uint32_t entry) {
    original_mode=false;if(!try_modern_club_records(s,entry))throw std::runtime_error("production adapter declined");
    returned(s);
#if defined(CM_CLUB_RECORDS_COVERAGE)
    __llvm_profile_write_file();
#endif
}
void wrong_result(State& s,std::uint32_t entry) {
    rewrite(s,entry);Cpu c(s);c.set(3,c.r(3)^1);
}
void wrong_cursor(State& s,std::uint32_t entry) {rewrite(s,entry);Cpu c(s);c.put32(c.toc(-0x7b84),c.u32(c.toc(-0x7b84))+2);}
void wrong_seed(State& s,std::uint32_t entry) {rewrite(s,entry);Cpu c(s);c.put32(c.toc(-0x7b90),c.u32(c.toc(-0x7b90))^1);}
void wrong_file_bytes(State& s,std::uint32_t entry) {rewrite(s,entry);Cpu c(s);c.put8(stream,c.u8(stream)^1);}
#define CLUB_RECORDS_ROUTINE(entry,end,results,digest,...) entry,
constexpr auto entries=std::to_array<std::uint32_t>({
#include "club_records_routines.inc"
});
#undef CLUB_RECORDS_ROUTINE
}
int main(int argc,char** argv) {
    if(argc<2 || argc>4) {std::cerr<<"Usage: cm-club-records-checks <private-export> [entry [variant]]\n";return 2;}
    try {
        auto base=std::make_unique<State>();load_program(*base,argv[1]);dispatch_probe=probe;
        const auto selected=argc>=3?std::uint32_t(std::stoul(argv[2],nullptr,0)):0u;
        unsigned failures=0,compared=0;
        for(auto entry:entries) {
            if(selected && selected!=entry)continue;
            const bool assignment=(entry>=0x100121c0 && entry<0x10012d30)||entry==0x10019ca0;
            const bool label=entry==0x100137c0||entry==0x10013910||entry==0x10013cc0;
            const bool formatted=entry==0x1001c290||entry==0x1001cb30||entry==0x10014080||entry==0x10013ee0||entry==0x100163f0;
            const bool expanded=entry==0x1000e6b0||entry==0x10011bd0||entry==0x10011750||entry==0x10012e90||entry==0x10012fb0||entry==0x100132f0||entry==0x100134b0||entry==0x1001bb50||entry==0x1001bec0||entry==0x1001d410;
            const int variants=label?41:formatted?748:entry==0x1000eb40?84:(entry==0x1001a690||entry==0x1001b120)?29:entry==0x10011750?26:entry==0x1000ea50?9:expanded?22:8;
            for(int variant=0;variant<variants;++variant) for(int overlap:std::array{0,1,-1,4,-4,999}) {
                if(argc==4 && variant!=std::stoi(argv[3]))continue;
                if(!assignment && overlap!=0 && entry!=0x1001be60 && !label && entry!=0x1001b830 && entry!=0x1001b9c0)continue;
                if(!assignment && overlap!=0 && overlap!=1 && !(label && overlap==-1))continue;
                active={entry,variant,overlap};auto state=cm::semantic_io_checks::clone(*base);initialize(*state,active);
                const auto differences=compare_routine_call(*state,entry,original,rewrite,RoutineChecker::result_r3);
                ++compared;
                std::cout<<(differences.empty()?"ok ":"DIFFERS ")<<hex_address(entry)<<" variant "<<variant<<" overlap "<<overlap<<'\n';
                for(const auto& difference:differences)std::cout<<"  "<<difference<<'\n';
                failures+=!differences.empty();if(failures>=5)return 1;
            }
        }
        active={selected?selected:entries[0],0,0};auto state=cm::semantic_io_checks::clone(*base);initialize(*state,active);
        const auto negative=compare_routine_call(*state,selected?selected:entries[0],original,wrong_result,RoutineChecker::result_r3);
        const bool detected=std::ranges::any_of(negative,[](const auto& d){return d.starts_with("r3 ");});
        if(!detected){++failures;for(const auto& d:negative)std::cout<<d<<'\n';}
        if(!selected) {
            struct Control{std::uint32_t entry;int variant;void(*run)(State&,std::uint32_t);const char* name;};
            const Control controls[]={{0x1000eb40,11,wrong_cursor,"RNG cursor"},{0x1000eb40,10,wrong_seed,"RNG seed"},{0x1001b120,0,wrong_file_bytes,"serialized byte"}};
            for(const auto& control:controls){active={control.entry,control.variant,0};auto fixture=cm::semantic_io_checks::clone(*base);initialize(*fixture,active);
                const auto bad=compare_routine_call(*fixture,control.entry,original,control.run,RoutineChecker::result_r3);
                const bool found=std::ranges::any_of(bad,[](const auto& difference){return difference.starts_with("byte ");});
                std::cout<<control.name<<" corruption "<<(found?"detected":"FAILED")<<'\n';failures+=!found;
            }
        }
        std::cout<<compared<<" returned comparisons requested; wrong-result control "<<(detected?"detected":"FAILED")<<"; "<<failures<<" failed\n";
        return failures?1:0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 2;}
}
