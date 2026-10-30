#include "nation_awards_detail.hpp"

namespace cm::aot::nation_awards {
using namespace detail;
namespace {
void publish_selected(Cpu& c,const Frame& frame,unsigned selected_at,unsigned extra_at,
                      unsigned output_at,std::uint32_t date,std::uint32_t award,std::uint32_t back) {
    Program p{c}; c.set(7,0); c.set(5,0); c.set(3,frame.at(selected_at));
    c.put16(frame.at(output_at),c.u16(date+2)); compare(c,c.r(4));
    for (unsigned index=0;index<3;++index) {
        c.set(6,c.u32(c.r(3))); c.set(10,missing); const Person person{c,c.r(6)};
        c.set(4,person.name(0)); const auto first=compare_logical(c,c.r(4))!=0?c.u32(c.r(4)+0x20):missing;
        c.set(4,person.name(1)); c.set(8,compare_logical(c,c.r(4))!=0?c.u32(c.r(4)+0x20):missing);
        c.set(4,person.name(2)); c.set(9,compare_logical(c,c.r(4))!=0?c.u32(c.r(4)+0x20):missing);
        c.set(4,person.club()); if (compare_logical(c,c.r(4))!=0) c.set(10,c.u32(c.r(4)));
        compare(c,index); if (index) compare(c,index,1);
        const auto record=frame.at(output_at+4+index*24);
        c.put32(record,first); c.set(4,frame.at(extra_at)); const auto extra=c.u32(c.r(4)+c.r(5));
        c.put32(record+4,c.r(8)); c.put32(record+8,c.r(9)); c.put32(record+20,c.r(10));
        c.set(4,person.id()); c.put32(record+12,c.r(4)); c.put32(record+16,extra);
        c.set(5,c.r(5)+4); c.set(3,c.r(3)+4); c.set(7,index+1);
    }
    p.call(0x1044cf90,back,{award,frame.at(output_at),1});
}
bool duplicate(Cpu& c,const Frame& frame,unsigned selected_at,std::uint32_t person,unsigned count,
               unsigned flag_register,unsigned counter_register) {
    c.set(3,frame.at(selected_at)); c.set(counter_register,0); c.set(5,0);
    bool found=false;
    for (unsigned index=0; ;++index) {
        c.set(4,std::uint32_t(std::int8_t(index)));
        if (compare(c,c.r(4),std::int8_t(count))>=0) break;
        c.set(4,c.u32(c.r(3)));
        if (compare_logical(c,c.r(4),person)==0) { c.set(flag_register,1); found=true; break; }
        c.set(5,c.r(5)+4); c.set(3,c.r(3)+4); c.set(counter_register,index+1);
    }
    return found;
}
}
void select_league_players(Cpu& c,std::uint32_t competition,std::uint32_t date,std::uint32_t award,
                          std::uint32_t age,std::uint32_t positions,std::uint32_t statistic,std::uint32_t nation) {
    Frame frame(c,0x104450c0,0x140,{.first_gpr=13,.gpr_offset=-0x5c,.first_fpr=31}); Program p{c}; Fpu fp(c);
    const auto constants=c.toc(-0x4a20),club_nation=c.u32(frame.at(0x178)),
        excluded=c.u32(frame.at(0x17c)),both=c.u32(frame.at(0x180));
    keep(c,{{13,c.toc(-0x4a18)},{14,c.toc(-0x7ba8)},{16,0},{18,0},
            {23,c.toc(-0x4a1c)},{24,c.toc(-0x7be0)},{25,age},{26,positions},
            {27,statistic},{28,nation},{29,club_nation},{30,excluded},{31,both}});
    c.set(3,c.toc(-0x7b5c)); c.set(9,c.u32(c.r(3)));
    c.set_f(31,c.f32(constants));
    const auto kind=std::int8_t(statistic);
    compare(c,kind,3);
    if (kind!=3) {
        if (kind<3) { if (compare(c,kind,2)>=0) c.set_f(31,c.f32(constants+4)); }
        else if (compare(c,kind,6)<0) c.set_f(31,c.f32(constants+4));
    }
    c.put32(frame.at(0x160),date); c.put32(frame.at(0x164),award);
    c.set(7,c.u32(c.r(9)+competition*4)); const auto league=c.r(7);
    if (compare_logical(c,league)==0 || compare(c,c.u8(league+0x43),1)==0) {
        assertion(c,frame,0x10445178,0x5c,0x3c,0x2e1); c.set(3,c.toc(-0x7bac)); return;
    }
    c.set(3,std::uint32_t(c.i16(league+0x3e))-1); c.set(4,statistic);
    const auto matches=std::int16_t(c.r(3)*std::uint32_t(c.i16(league+0x3c))); c.set(14,std::uint32_t(matches));
    const auto records=p.call(0x1045a0a0,0x104451e8,{c.u32(league+8),statistic,frame.at(0x38),100,missing}); c.set(13,records);
    if (compare(c,c.i32(frame.at(0x38)))==0) return;
    c.put32(frame.at(0xd8),std::uint32_t(matches)^0x80000000); c.set(21,0);
    while (true) {
        if (compare(c,std::int8_t(c.r(18)),3)==0) break;
        c.set_f(0,c.f64(c.r(23)+16)); fp.fcmp(0,c.f(31),c.f(0)); if (!(c.cr(0)&4)) break;
        keep(c,{{20,records},{22,c.r(21)},{17,0}});
        c.set_f(1,c.f64(c.r(23))); c.put32(frame.at(0xd4),c.u32(frame.at(0xd8)));
        c.put32(frame.at(0xd0),0x43300000); c.set_f(0,c.f64(frame.at(0xd0)));
        c.set_f(0,fp.fsubs(c.f(0),c.f(1))); c.set_f(0,fp.fmuls(c.f(0),c.f(31)));
        c.put64(frame.at(0xc8),fp.fctiwz(0,c.f(0))); c.set(19,c.u32(frame.at(0xcc)));
        for (unsigned index=0;compare(c,index,c.i32(frame.at(0x38)))<0;++index) {
            const auto row=records+index*0x30; const auto person=indirect(c,-0x7be0)+c.u32(row+4)*Person::size;
            keep(c,{{14,person},{15,0}}); c.set(3,indirect(c,-0x7be0));
            const auto eligible=[&]() {
                if (compare_logical(c,c.u32(person+0x14))==0) return false;
                if (compare(c,std::int16_t(positions))!=0) {
                    p.call(0x100db1b0,0x10445274,{person}); c.set(3,extended(std::int16_t(c.r(3))));
                    if (compare(c,std::int32_t(std::uint32_t(std::int16_t(positions))&c.r(3)))==0) return false;
                }
                compare(c,std::int8_t(c.r(18)));
                duplicate(c,frame,0x50,person,c.r(18),15,6);
                if (compare(c,std::int8_t(c.r(15)))!=0) return false;
                c.set(4,c.u8(row+13)); c.set(3,std::uint32_t(c.u8(row+12))-c.r(4));
                if (compare(c,c.r(3),std::int16_t(c.r(19)))<0) return false;
                if (compare(c,nation,missing)!=0) {
                    c.set(3,c.u32(person+12)); if (compare_logical(c,c.r(3))==0) return false;
                    if (compare(c,c.u32(c.r(3)+0x68),nation)!=0) return false;
                }
                if (compare(c,club_nation,missing)!=0) {
                    c.set(3,c.u32(person+0x24)); if (compare_logical(c,c.r(3))==0) return false;
                    c.set(3,c.u32(c.r(3)+0x2c)); if (compare_logical(c,c.r(3))==0) return false;
                    if (compare(c,c.u32(c.r(3)+0x68),club_nation)!=0) return false;
                }
                if (compare(c,excluded,missing)!=0) {
                    c.set(3,c.u32(person+12)); if (compare_logical(c,c.r(3))==0) return false;
                    if (compare(c,c.u32(c.r(3)+0x68),excluded)==0) return false;
                }
                if (compare(c,both,missing)!=0) {
                    c.set(3,c.u32(person+0x24)); if (compare_logical(c,c.r(3))==0) return false;
                    c.set(3,c.u32(c.r(3)+0x2c)); if (compare_logical(c,c.r(3))==0) return false;
                    if (compare(c,c.u32(c.r(3)+0x68),both)!=0) return false;
                    c.set(3,c.u32(person+12)); if (compare_logical(c,c.r(3))==0) return false;
                    if (compare(c,c.u32(c.r(3)+0x68),both)!=0) return false;
                }
                c.set(3,std::uint32_t(std::int8_t(age)));
                if (compare(c,c.r(3),-1)!=0 && compare(c,std::int8_t(c.i16(person+0x5c)),c.r(3))>0) {
                    c.set(16,c.r(16)+1); return false;
                }
                if (compare(c,std::int8_t(age),-1)!=0 && compare(c,c.r(16),3)<0) return false;
                return true;
            };
            if (eligible()) {
                compare(c,kind,2); c.set(3,frame.at(0x50)); c.put32(c.r(3)+c.r(22),person);
                c.set(3,frame.at(0x44)); c.put32(c.r(3)+c.r(22),kind==2?c.u8(row+14):0);
                c.set(18,c.r(18)+1); c.set(22,c.r(22)+4); c.set(21,c.r(21)+4);
                if (compare(c,std::int8_t(c.r(18)),3)==0) break;
            }
            keep(c,{{17,index+1},{20,row+0x30}});
        }
        c.set_f(0,c.f64(c.r(23)+8)); c.set_f(31,fp.fsub(c.f(31),c.f(0))); c.set_f(31,fp.frsp(c.f(31)));
    }
    p.call(0x10775170,0x10445458,{records}); c.set(4,std::uint32_t(std::int8_t(c.r(18))));
    if (compare(c,c.r(4),3)==0) publish_selected(c,frame,0x50,0x44,0x7c,date,award,0x10445590);
}
void select_nation_players(Cpu& c,std::uint32_t competition,std::uint32_t date,std::uint32_t award,
                          std::uint32_t age,std::uint32_t positions,std::uint32_t statistic) {
    Frame frame(c,0x104455b0,0x130,{.first_gpr=13,.gpr_offset=-0x5c,.first_fpr=31}); Program p{c}; Fpu fp(c);
    keep(c,{{13,award},{14,c.toc(-0x4a18)},{15,c.toc(-0x7ba8)},{16,0},{18,0},
            {24,c.toc(-0x4a20)},{25,c.toc(-0x4a1c)},{26,c.toc(-0x7be0)},
            {27,date},{28,age},{29,positions},{30,statistic},{31,0}});
    c.set_f(31,c.f32(c.r(24))); c.set(3,indirect(c,-0x7b5c)); c.set(17,c.u32(c.r(3)+competition*4));
    const auto tournament=c.r(17); if (compare_logical(c,tournament)==0) return;
    p.call(0x10192130,0x1044561c,{c.u32(tournament+4)});
    if (compare(c,std::int8_t(c.r(3)),1)==0 || compare(c,std::int8_t(c.r(3)))==0) c.set(31,1);
    compare_logical(c,tournament);
    const auto records=p.call(0x1045a0a0,0x104456ac,{c.u32(tournament+8),statistic,frame.at(0x38),100,missing}); c.set(21,records);
    if (compare(c,c.i32(frame.at(0x38)))==0) return;
    c.set(22,0);
    while (true) {
        if (compare(c,std::int8_t(c.r(18)),3)==0) break;
        c.set_f(0,c.f64(c.r(25)+16)); fp.fcmp(0,c.f(31),c.f(0)); if (!(c.cr(0)&4)) break;
        c.set_f(0,c.f32(c.r(24)+8)); keep(c,{{20,records},{23,c.r(22)},{17,0}});
        c.set_f(0,fp.fmuls(c.f(0),c.f(31))); c.put64(frame.at(0xc8),fp.fctiwz(0,c.f(0))); c.set(19,c.u32(frame.at(0xcc)));
        for (unsigned index=0;compare(c,index,c.i32(frame.at(0x38)))<0;++index) {
            const auto row=records+index*0x30,person=indirect(c,-0x7be0)+c.u32(row+4)*Person::size;
            keep(c,{{14,person},{15,0}}); c.set(3,indirect(c,-0x7be0));
            const auto eligible=[&]() {
                if (compare_logical(c,c.u32(person+0x14))==0) return false;
                if (compare(c,std::int16_t(positions))!=0) {
                    p.call(0x100db1b0,0x10445718,{person}); c.set(3,extended(std::int16_t(c.r(3))));
                    if (compare(c,std::int32_t(std::uint32_t(std::int16_t(positions))&c.r(3)))==0) return false;
                }
                duplicate(c,frame,0x50,person,c.r(18),15,6);
                if (compare(c,std::int8_t(c.r(15)))!=0) return false;
                c.set(4,c.u8(row+13)); c.set(3,std::uint32_t(c.u8(row+12))-c.r(4));
                if (compare(c,c.r(3),std::int16_t(c.r(19)))<0) return false;
                c.set(3,std::uint32_t(std::int8_t(age)));
                if (compare(c,c.r(3),-1)!=0 && compare(c,std::int8_t(c.i16(person+0x5c)),c.r(3))>0) {
                    c.set(16,c.r(16)+1); return false;
                }
                if (compare(c,std::int8_t(age),-1)!=0 && compare(c,c.r(16),3)<0) return false;
                return true;
            };
            if (eligible()) {
                compare(c,std::int8_t(statistic),2); c.set(3,frame.at(0x50)); c.put32(c.r(3)+c.r(23),person);
                if (std::int8_t(statistic)==2) { c.set(3,frame.at(0x44)); c.put32(c.r(3)+c.r(23),c.u8(row+14)); }
                c.set(18,c.r(18)+1); c.set(23,c.r(23)+4); c.set(22,c.r(22)+4);
                if (compare(c,std::int8_t(c.r(18)),3)==0) break;
            }
            keep(c,{{17,index+1},{20,row+0x30}});
        }
        c.set_f(0,c.f64(c.r(25)+8)); c.set_f(31,fp.fsub(c.f(31),c.f(0))); c.set_f(31,fp.frsp(c.f(31)));
    }
    p.call(0x10775170,0x1044583c,{records}); c.set(14,std::uint32_t(std::int8_t(c.r(18))));
    if (compare(c,c.r(14),3)!=0) return;
    keep(c,{{16,0},{15,0},{19,frame.at(0x50)}}); c.put16(frame.at(0x7c),c.u16(date+2));
    for (unsigned index=0;compare(c,index,c.r(14))<0;++index) {
        const Person person{c,c.u32(c.r(19))}; c.set(20,person.address); c.set(4,missing);
        c.set(3,person.name(0)); c.set(21,compare_logical(c,c.r(3))!=0?c.u32(c.r(3)+0x20):missing);
        c.set(3,person.name(1)); c.set(17,compare_logical(c,c.r(3))!=0?c.u32(c.r(3)+0x20):missing);
        c.set(3,person.name(2)); c.set(18,compare_logical(c,c.r(3))!=0?c.u32(c.r(3)+0x20):missing);
        if (compare(c,std::int8_t(c.r(31)))!=0) {
            c.set(3,c.u32(person.address+12));
            if (compare_logical(c,c.r(3))!=0) { p.call(0x100c76c0,0x104458c8); c.set(4,c.u32(c.r(3))); }
        } else {
            c.set(3,person.club()); if (compare_logical(c,c.r(3))!=0) c.set(4,c.u32(c.r(3)));
        }
        compare(c,index); if (index) compare(c,index,1);
        const auto record=frame.at(0x80+index*24); c.put32(record,c.r(21)); c.set(3,frame.at(0x44)); const auto extra=c.u32(c.r(3)+c.r(15));
        c.put32(record+4,c.r(17)); c.put32(record+8,c.r(18)); c.put32(record+20,c.r(4));
        c.set(3,person.id()); c.put32(record+12,c.r(3)); c.put32(record+16,extra);
        keep(c,{{15,c.r(15)+4},{19,c.r(19)+4},{16,index+1}});
    }
    p.call(0x1044cf90,0x10445988,{award,frame.at(0x7c),1});
}
}
