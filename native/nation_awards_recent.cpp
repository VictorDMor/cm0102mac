#include "nation_awards_detail.hpp"

namespace cm::aot::nation_awards {
using namespace detail;
namespace {
void date_copy(Cpu& c,std::uint32_t destination,std::uint32_t source) {
    c.put16(destination,c.u16(source)); c.put16(destination+2,c.u16(source+2)); c.put8(destination+4,c.u8(source+4));
}
bool before(Cpu& c,std::uint32_t left,std::uint32_t right) {
    const auto year=c.u16(left+2),other_year=c.u16(right+2);
    if (compare_logical(c,year,other_year)<0) return compare(c,1)!=0;
    if (year>other_year) return compare(c,0)!=0;
    c.set(4,std::uint32_t(c.i16(right)));
    const auto bits=std::uint32_t(c.i16(left))^c.r(4);
    c.set(3,std::uint32_t(std::int32_t(bits)>>1));
    const auto difference=c.r(3)-(bits&c.r(4));
    return compare(c,std::int8_t(difference>>31))!=0;
}
}
void select_recent_team(Cpu& c,std::uint32_t competition,std::uint32_t award) {
    Frame frame(c,0x10446510,0xf30,{.first_gpr=13}); Program p{c}; Fpu fp(c);
    keep(c,{{13,award},{14,c.toc(-0x7b4c)},{15,c.toc(-0x7b80)},
            {29,c.toc(-0x7be4)},{30,c.toc(-0x7be0)}});
    c.put32(frame.at(0xf4c),competition);
    p.call(0x10717b20,0x10446548,{frame.at(0x70),c.r(15),100});
    leave(c,{{8,std::uint32_t(c.i16(frame.at(0x70)))},{7,c.u16(frame.at(0x72))},
             {6,c.u8(frame.at(0x74))},{5,std::uint32_t(c.i16(c.r(15)))},{4,c.u16(c.r(15)+2)}});
    date_copy(c,frame.at(0xa0),frame.at(0x70)); date_copy(c,frame.at(0x98),c.r(15));
    p.call(0x10715880,0x10446580,{frame.at(0x90)});
    p.call(0x10715880,0x10446588,{frame.at(0x88)});
    p.call(0x10715880,0x10446590,{frame.at(0x80)});
    p.call(0x10738c00,0x10446598,{frame.at(0xa8)});
    keep(c,{{21,0},{20,0},{19,0}});
    p.call(0x10773320,0x104465bc,{frame.at(0x1f8),c.toc(-0x7acc),0,0x664,2});
    c.set(3,c.u32(frame.at(0xa0))); c.put32(frame.at(0x48),c.r(3)); c.put32(frame.at(0x4c),c.u32(frame.at(0xa4)));
    c.set(5,c.u16(award+0x22)); c.set(3,c.u16(frame.at(0x4a)));
    if (before(c,frame.at(0x48),award+0x20)) date_copy(c,frame.at(0xa0),award+0x20);
    p.call(0x10715a30,0x1044663c,{frame.at(0x98),0,frame.at(0x98)});
    c.set(3,c.u32(frame.at(0xa4))); c.put32(frame.at(0x44),c.r(3)); c.put32(frame.at(0xedc),c.u32(frame.at(0xa0)));
    auto has_dates=[&]() {
        c.put32(frame.at(0x40),c.u32(frame.at(0xedc)));
        c.set(3,c.u16(frame.at(0x42)));
        return before(c,frame.at(0x40),frame.at(0x98));
    };
    while (has_dates()) {
        p.call(0x10717b20,0x10446660,{frame.at(0x68),frame.at(0x98),3});
        leave(c,{{4,c.u8(frame.at(0x6c))},{3,0xfffffffe},{6,c.u16(frame.at(0x6a))},
                 {5,std::uint32_t(c.i16(frame.at(0x68)))},{9,frame.at(0xa8)},{8,missing},{10,competition}});
        date_copy(c,frame.at(0x90),frame.at(0x68)); c.put32(frame.at(0x38),0xfffffffe); c.put32(frame.at(0x3c),missing);
        p.call(0x1048b180,0x104466b8,{indirect(c,-0x7a60),c.u16(frame.at(0x90)),c.u16(frame.at(0x92)),
                                    c.u16(frame.at(0x98)),c.u16(frame.at(0x9a))});
        if (compare_logical(c,c.r(3)&0xffff,1)>0) {
            c.put32(frame.at(0xb4),c.u32(frame.at(0xac))); c.set(24,c.r(20)*12); c.set(16,0);
            while (true) {
                p.call(0x10738e00,0x10446958,{frame.at(0xa8)}); c.set(31,c.r(3));
                const auto match=c.r(31); if (compare(c,match)==0) break;
                c.set(4,c.u32(match+8));
                if (compare(c,c.r(4),-1)==0) { c.set(20,0); break; }
                p.call(0x1029ec40,0x104466f0,{c.u32(c.r(14)),c.r(4),frame.at(0xd8)});
                if (compare(c,std::int8_t(c.r(3)))==0) { c.set(20,0); break; }
                if (compare(c,c.r(16))==0) {
                    p.call(0x107156b0,0x1044671c,{frame.at(0x60),std::uint32_t(c.i16(match+0x2a)),c.u16(match+0x28)});
                    leave(c,{{4,c.u16(frame.at(0x62))},{3,std::uint32_t(c.i16(frame.at(0x60)))}});
                    date_copy(c,frame.at(0x88),frame.at(0x60));
                    date_copy(c,frame.at(0x80),frame.at(0x88));
                }
                c.set(4,c.u16(match+0x2a));
                if (compare(c,c.r(4),c.i16(frame.at(0x80)))!=0) {
                    p.call(0x107156b0,0x1044676c,{frame.at(0x58),std::uint32_t(std::int16_t(c.r(4))),c.u16(match+0x28)});
                    leave(c,{{4,c.u16(frame.at(0x5a))},{3,std::uint32_t(c.i16(frame.at(0x58)))}});
                    date_copy(c,frame.at(0x80),frame.at(0x58));
                }
                keep(c,{{25,c.r(24)},{18,0},{26,0}});
                for (unsigned side=0;compare(c,std::int8_t(side),2)<0;++side) {
                    const auto team=frame.at(0xd8+side*0x664);
                    keep(c,{{22,team},{28,c.r(25)},{17,0},{15,team+0x194},{27,0}});
                    for (unsigned slot=0;compare(c,std::int8_t(slot),20)<0;++slot) {
                        const auto player=team+0x194+slot*0x4c;
                        const auto eligible=[&]() {
                            if (compare(c,std::int8_t(slot),10)>0 && compare(c,std::int8_t(c.u8(player+0x32)),-1)==0) return false;
                            c.set(3,c.u32(player)); if (compare(c,c.r(3),-1)==0) return false;
                            if (compare(c,c.u32(player+4),-1)==0) return false;
                            return compare(c,c.r(3),c.u32(c.r(29)))<=0;
                        };
                        if (eligible()) {
                            if (compare(c,c.r(20),c.r(19))==0) {
                                const auto old=c.r(21); compare_logical(c,old); c.set(19,c.r(19)+200);
                                if (old) p.call(0x107751b0,0x10446810,{old,c.r(19)*12});
                                else p.call(0x10775130,0x1044681c,{c.r(19)*12});
                                c.set(21,c.r(3));
                                if (compare_logical(c,c.r(3))==0) {
                                    assertion(c,frame,0x1044683c,0xb8,0x78,0x7a8,true);
                                    c.set(5,c.toc(-0x7bac));
                                    p.call(0x10738c20,0x10446890,{frame.at(0xa8),missing}); return;
                                }
                            }
                            c.set(3,c.u32(player)); c.set(23,c.r(21)+c.r(28));
                            compare(c,std::int8_t(side)); c.put32(c.r(23),c.r(3));
                            c.put32(c.r(23)+4,c.u32(match+(side?0x20:0x1c)));
                            c.set(3,c.u8(player+0x35)); c.put8(c.r(23)+8,c.r(3));
                            if (compare(c,std::int8_t(slot),11)<0) c.put16(c.r(23)+10,c.u16(team+0x156+slot*2));
                            else {
                                c.set(3,indirect(c,-0x7be0)+c.u32(player)*Person::size);
                                if (compare_logical(c,c.u32(c.r(3)+0x14))!=0) {
                                    p.call(0x100db1b0,0x10446900); c.put16(c.r(23)+10,c.r(3));
                                } else c.put16(c.r(23)+10,0);
                            }
                            keep(c,{{28,c.r(28)+12},{25,c.r(25)+12},{24,c.r(24)+12},{20,c.r(20)+1}});
                        }
                        keep(c,{{27,c.r(27)+2},{17,slot+1},{15,player+0x4c}});
                    }
                    keep(c,{{26,c.r(26)+0x664},{18,side+1}});
                }
                c.set(16,c.r(16)+1);
            }
            if (compare(c,c.r(20),20)>0) break;
        }
        p.call(0x10738ca0,0x10446970,{frame.at(0xa8)});
        p.call(0x10717cb0,0x10446980,{frame.at(0x50),frame.at(0x98),1});
        leave(c,{{4,c.u16(frame.at(0x52))},{3,std::uint32_t(c.i16(frame.at(0x50)))}});
        date_copy(c,frame.at(0x98),frame.at(0x50));
    }
    if (compare(c,c.r(20))==0) {
        p.call(0x10738c20,0x10446a00,{frame.at(0xa8),missing}); return;
    }
    p.call(0x10768300,0x10446a18,{c.r(21),c.r(20),12,c.toc(-0x4a2c)});
    p.call(0x1044b160,0x10446a20,{award});
    c.set(3,c.toc(-0x7b5c)); c.set(4,c.u32(c.r(3))); c.set(5,3);
    if (compare_logical(c,c.r(4))!=0) {
        c.set(3,c.u32(award+12));
        if (compare_logical(c,c.r(3))!=0) {
            c.set(3,c.u32(c.r(4)+c.u32(c.r(3))*4));
            if (compare_logical(c,c.r(3))!=0) c.set(5,c.u8(c.r(3)+0x46));
        }
    }
    c.put8(award+0x298,c.r(5));
    keep(c,{{17,c.r(21)},{15,c.toc(-0x4a1c)},{16,0},{14,0x43300000}});
    for (unsigned index=0;compare(c,index,c.r(20))<0;++index) {
        const auto candidate=c.r(17);
        c.set(4,c.u32(candidate)); c.put32(frame.at(0xed8),c.u8(candidate+8));
        c.set_f(1,c.f64(c.r(15)+24)); c.put32(frame.at(0xed4),c.r(14)); c.set_f(0,c.f64(frame.at(0xed4)));
        c.set(4,indirect(c,-0x7be0)+c.r(4)*Person::size); c.set(5,c.u32(candidate+4));
        c.set_f(1,fp.fsubs(c.f(0),c.f(1))); c.set(7,std::uint32_t(c.i16(candidate+10)));
        p.call(0x1044aca0,0x10446aa8,{award,c.r(4),c.r(5)});
        keep(c,{{16,index+1},{17,candidate+12}});
    }
    p.call(0x1044b3d0,0x10446acc,{award,frame.at(0x88),frame.at(0x80),frame.at(0x98)});
    if (compare_logical(c,c.r(21))!=0) p.call(0x10775170,0x10446adc,{c.r(21)});
    p.call(0x10738c20,0x10446ae8,{frame.at(0xa8),missing});
}
}
