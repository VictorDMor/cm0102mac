#include "nation_awards_detail.hpp"

namespace cm::aot::nation_awards {
using namespace detail;
void select_world_players(Cpu& c,std::uint32_t date,std::uint32_t award,
                         std::uint32_t club_nation,std::uint32_t nation,std::uint32_t excluded_nation,
                         std::uint32_t either_nation,std::uint32_t age) {
    Frame frame(c,0x104459a0,0x130,{.first_gpr=13,.gpr_offset=-0x5c,.first_fpr=31}); Program p{c}; Fpu fp(c);
    const auto statistic=std::int8_t(c.u8(frame.at(0x16f)));
    const auto positions=c.i16(frame.at(0x16a));
    keep(c,{{13,c.toc(-0x7b3c)},{14,c.toc(-0x4a1c)},{15,age},{16,std::uint32_t(positions)},
            {17,std::uint32_t(std::uint8_t(statistic))},{18,excluded_nation},{19,0},{20,0},{21,0},{23,nation},
            {24,club_nation},{25,0},{26,0},{27,0},{28,0},{29,either_nation},
            {30,c.toc(-0x7b98)},{31,c.toc(-0x4a20)}});
    c.set_f(31,c.f32(c.r(31))); c.put32(frame.at(0x14c),date); c.put32(frame.at(0x150),award);
    if (compare(c,statistic,5)==0 || compare(c,statistic,2)==0) c.set_f(31,c.f32(c.r(31)+12));
    c.set(3,c.toc(-0x7be4));
    const auto candidates=p.call(0x10775130,0x10445a30,{c.u32(c.r(3))*12}); c.set(22,candidates);
    if (compare(c,candidates)==0) {
        assertion(c,frame,0x10445a4c,0x58,0x38,0x4fc,true); c.set(3,c.toc(-0x7bac)); return;
    }
    for (auto [value,reg] : {std::pair{club_nation,28u},{nation,27u},{excluded_nation,26u},{either_nation,25u}}) {
        if (compare(c,value,-1)>0) { c.set(3,indirect(c,-0x7b98)); c.set(reg,c.r(3)+value*0x128); }
    }
    c.set(4,6); if (compare(c,statistic,2)==0) c.set(4,2);
    keep(c,{{18,c.r(4)},{24,0},{30,0},{23,indirect(c,-0x7be0)}}); c.set(3,c.toc(-0x7be0));
    for (unsigned index=0; ;++index) {
        c.set(3,c.toc(-0x7be4));
        if (compare(c,index,c.u32(c.r(3)))>=0) break;
        const auto person=c.r(23);
        const auto eligible=[&]() {
            if (compare_logical(c,c.u32(person+0x14))==0) return false;
            c.set(4,c.u32(person+0x24));
            if (compare_logical(c,c.r(4))!=0) {
                c.set(3,indirect(c,-0x7b5c));
                if (compare_logical(c,c.r(3))!=0) {
                    c.set(5,c.u32(c.r(4)+0x30)); if (compare_logical(c,c.r(5))==0) return false;
                    if (compare_logical(c,c.u32(c.r(3)+c.u32(c.r(5))*4))==0) return false;
                }
            }
            if (compare_logical(c,c.r(28))!=0) {
                if (compare_logical(c,c.r(4))==0) return false;
                if (compare_logical(c,c.u32(c.r(4)+0x2c),c.r(28))!=0) return false;
            }
            if (compare_logical(c,c.r(27))!=0 && compare_logical(c,c.u32(person+12),c.r(27))!=0) return false;
            if (compare_logical(c,c.r(26))!=0 && compare_logical(c,c.u32(person+12),c.r(26))==0) return false;
            if (compare_logical(c,c.r(25))!=0 && compare_logical(c,c.u32(person+12),c.r(25))!=0) {
                if (compare_logical(c,c.r(4))==0) return false;
                if (compare_logical(c,c.u32(c.r(4)+0x2c),c.r(25))!=0) return false;
            }
            return true;
        };
        if (eligible()) {
            const auto candidate=candidates+c.r(30); c.set(29,candidate); c.put32(candidate,person);
            c.set_f(1,c.f32(c.r(31)+16));
            p.call(0x10303560,0x10445be8,{c.u32(c.r(13)),c.u32(person+0x14),c.r(18),std::uint32_t(std::uint8_t(statistic))});
            c.put_f32(candidate+4,float(c.f(1)));
            bool nonzero=true; compare(c,statistic,2);
            if (statistic==2) {
                c.set_f(1,c.f32(candidate+4)); c.set_f(0,c.f32(c.r(31)+20)); fp.fcmp(0,c.f(1),c.f(0));
                nonzero=(c.cr(0)&2)==0;
            }
            if (nonzero) {
                c.set_f(1,c.f32(c.r(31)+16));
                p.call(0x10303560,0x10445c20,{c.u32(c.r(13)),c.u32(person+0x14),c.r(18),1});
                c.put64(frame.at(0xc4),fp.fctiwz(0,c.f(1))); const auto games=c.i16(frame.at(0xca)); c.put16(candidate+8,std::uint32_t(games));
                if (compare(c,statistic,3)==0) {
                    c.set(3,std::uint32_t(games));
                    if (compare(c,games)>0) {
                        c.set(3,c.r(3)^0x80000000); c.put32(frame.at(0xc8),c.r(3)); c.put32(frame.at(0xc4),0x43300000);
                        c.set_f(1,c.f64(c.r(14))); c.set_f(2,c.f32(candidate+4)); c.set_f(0,c.f64(frame.at(0xc4)));
                        c.set_f(0,fp.fsubs(c.f(0),c.f(1))); c.set_f(0,fp.fdivs(c.f(2),c.f(0)));
                    } else c.set_f(0,c.f32(c.r(31)+20));
                    c.put_f32(candidate+4,float(c.f(0)));
                }
                if (compare(c,statistic,2)!=0) {
                    c.set(4,c.u32(person+0x24)); std::int16_t weight=1;
                    if (compare_logical(c,c.r(4))!=0) {
                        c.set(5,c.u32(c.r(4)+0x30));
                        if (compare_logical(c,c.r(5))!=0) {
                            const auto reputation=c.i16(c.r(4)+0x1ec),division=c.i16(c.r(5)+0x18);
                            const auto club_weight=std::int32_t(reputation)/500;
                            const auto total=club_weight+std::int32_t(division)*4;
                            weight=std::int16_t(total/5);
                            // The strength expression is the original's exact
                            // signed constant divisions, followed by a halfword.
                            c.set(4,std::uint32_t(club_weight)); c.set(5,0x66666667);
                        }
                    }
                    c.set(4,std::uint32_t(weight)); c.set(3,std::uint32_t(weight)<0x80000000u?0:1);
                    const auto half=std::int32_t(weight)/2;
                    c.put32(frame.at(0xc4),0x43300000); c.put32(frame.at(0xc8),std::uint32_t(half)^0x80000000);
                    c.set_f(1,c.f64(c.r(14))); c.set_f(2,c.f32(candidate+4)); c.set_f(0,c.f64(frame.at(0xc4)));
                    c.set_f(0,fp.fsubs(c.f(0),c.f(1))); c.set_f(0,fp.fmuls(c.f(2),c.f(0))); c.put_f32(candidate+4,float(c.f(0)));
                }
                c.set(30,c.r(30)+12); c.set(21,c.r(21)+1);
            }
        }
        keep(c,{{24,index+1},{23,person+Person::size}});
    }
    if (compare(c,c.r(21))==0) return; // The original retains this allocation.
    compare(c,statistic,3); c.set(6,c.toc(statistic==3?-0x4a28:-0x4a24));
    p.call(0x10768300,statistic==3?0x10445d84:0x10445d6c,{candidates,c.r(21),12}); c.set(18,0);
    while (true) {
        if (compare(c,std::int8_t(c.r(20)),3)==0) break;
        c.set_f(0,c.f64(c.r(14)+16)); fp.fcmp(0,c.f(31),c.f(0)); if (!(c.cr(0)&4)) break;
        c.set_f(0,c.f32(c.r(31)+24)); keep(c,{{24,candidates},{13,c.r(18)},{23,0}});
        c.set_f(0,fp.fmuls(c.f(0),c.f(31))); c.put64(frame.at(0xc4),fp.fctiwz(0,c.f(0))); c.set(25,c.u32(frame.at(0xc8)));
        for (unsigned index=0;compare(c,index,c.r(21))<0;++index) {
            const auto candidate=candidates+index*12,person=c.u32(candidate); c.set(26,person);
            const auto eligible=[&]() {
                if (compare(c,positions)!=0) {
                    p.call(0x100db1b0,0x10445dc4,{person}); c.set(3,extended(std::int16_t(c.r(3))));
                    if (compare(c,std::int32_t(c.r(3)&std::uint32_t(positions)))==0) return false;
                    if (compare(c,positions,4)==0 && compare(c,c.r(3)&2)!=0) return false;
                }
                c.set(3,frame.at(0x4c)); c.set(6,0); c.set(7,0); c.set(5,0);
                for (unsigned n=0; ;++n) {
                    c.set(4,std::uint32_t(std::int8_t(n)));
                    if (compare(c,c.r(4),std::int8_t(c.r(20)))>=0) break;
                    c.set(4,c.u32(c.r(3)));
                    if (compare_logical(c,c.r(4),person)==0) { c.set(6,1); break; }
                    c.set(5,c.r(5)+4); c.set(3,c.r(3)+4); c.set(7,n+1);
                }
                if (compare(c,std::int8_t(c.r(6)))!=0) return false;
                if (compare(c,c.i16(candidate+8),c.r(25))<0) return false;
                c.set(3,std::uint32_t(std::int8_t(age)));
                if (compare(c,c.r(3),-1)!=0 && compare(c,std::int8_t(c.i16(person+0x5c)),c.r(3))>0) {
                    c.set(19,c.r(19)+1); return false;
                }
                if (compare(c,std::int8_t(age),-1)!=0 && compare(c,c.r(19),3)<0) return false;
                return true;
            };
            if (eligible()) {
                compare(c,statistic,2); c.set(3,frame.at(0x4c)); c.put32(c.r(3)+c.r(13),person);
                if (statistic==2) {
                    c.set_f(0,c.f32(candidate+4)); c.set(3,frame.at(0x40));
                    c.put64(frame.at(0xc4),fp.fctiwz(0,c.f(0))); c.put32(c.r(3)+c.r(13),c.u32(frame.at(0xc8)));
                }
                c.set(20,c.r(20)+1); c.set(13,c.r(13)+4); c.set(18,c.r(18)+4);
                if (compare(c,std::int8_t(c.r(20)),3)==0) break;
            }
            keep(c,{{23,index+1},{24,candidate+12}});
        }
        c.set_f(0,c.f64(c.r(14)+8)); c.set_f(31,fp.fsub(c.f(31),c.f(0))); c.set_f(31,fp.frsp(c.f(31)));
    }
    p.call(0x10775170,0x10445ef4,{candidates}); c.set(4,std::uint32_t(std::int8_t(c.r(20))));
    if (compare(c,c.r(4),3)!=0) return;
    c.set(7,0); c.set(5,0); c.set(3,frame.at(0x4c)); c.put16(frame.at(0x78),c.u16(date+2)); compare(c,c.r(4));
    for (unsigned index=0;index<3;++index) {
        c.set(6,c.u32(c.r(3))); c.set(10,missing); const Person person{c,c.r(6)};
        c.set(4,person.name(0)); const auto first=compare_logical(c,c.r(4))!=0?c.u32(c.r(4)+0x20):missing;
        c.set(4,person.name(1)); c.set(8,compare_logical(c,c.r(4))!=0?c.u32(c.r(4)+0x20):missing);
        c.set(4,person.name(2)); c.set(9,compare_logical(c,c.r(4))!=0?c.u32(c.r(4)+0x20):missing);
        c.set(4,person.club()); if (compare_logical(c,c.r(4))!=0) c.set(10,c.u32(c.r(4)));
        compare(c,index); if (index) compare(c,index,1);
        const auto record=frame.at(0x7c+index*24); c.put32(record,first);
        c.set(4,frame.at(0x40)); const auto extra=c.u32(c.r(4)+c.r(5));
        c.put32(record+4,c.r(8)); c.put32(record+8,c.r(9)); c.put32(record+20,c.r(10));
        c.set(4,person.id()); c.put32(record+12,c.r(4)); c.put32(record+16,extra);
        c.set(5,c.r(5)+4); c.set(3,c.r(3)+4); c.set(7,index+1);
    }
    p.call(0x1044cf90,0x10446028,{award,frame.at(0x78),1});
}
}
