#include "nation_awards_detail.hpp"

namespace cm::aot::nation_awards {
using namespace detail;
namespace {
void set_team_style(Cpu& c,std::uint32_t award,std::uint32_t competitions) {
    c.set(4,competitions); c.set(5,3);
    if (compare_logical(c,c.r(4))!=0) {
        c.set(3,c.u32(award+12));
        if (compare_logical(c,c.r(3))!=0) {
            c.set(3,c.u32(competitions+c.u32(c.r(3))*4));
            if (compare_logical(c,c.r(3))!=0) c.set(5,c.u8(c.r(3)+0x46));
        }
    }
    c.put8(award+0x298,c.r(5));
}
void finish_team(Cpu& c,const Program& p,std::uint32_t award,std::uint32_t records,
                 std::uint32_t free_back,std::uint32_t year_back,std::uint32_t publish_back) {
    p.call(0x10775170,free_back,{records});
    p.call(0x1044b420,year_back,{award,c.u16(c.toc(-0x7b80)+2)});
    p.call(0x1044b4b0,publish_back,{award});
}
void ratio(Cpu& c,const Frame& frame,unsigned signed_at,unsigned unsigned_at,
           std::int16_t score,std::uint32_t appearances,std::uint32_t constants) {
    Fpu fp(c); c.set_f(2,c.f64(constants));
    c.put32(frame.at(signed_at),0x43300000);
    c.put32(frame.at(signed_at+4),std::uint32_t(score)^0x80000000);
    c.set_f(1,c.f64(constants+24)); c.set_f(0,c.f64(frame.at(signed_at)));
    c.put32(frame.at(unsigned_at+4),appearances);
    c.set_f(2,fp.fsubs(c.f(0),c.f(2)));
    c.put32(frame.at(unsigned_at),0x43300000); c.set_f(0,c.f64(frame.at(unsigned_at)));
    c.set_f(0,fp.fsubs(c.f(0),c.f(1))); c.set_f(1,fp.fdivs(c.f(2),c.f(0)));
}
}
void select_league_team(Cpu& c,std::uint32_t competition,std::uint32_t award) {
    Frame frame(c,0x10446040,0xb0,{.first_gpr=22,.gpr_offset=-0x38,.first_fpr=31}); Program p{c}; Fpu fp(c);
    keep(c,{{22,c.toc(-0x4a18)},{23,award},{24,0},{25,c.toc(-0x7ba8)},
            {28,c.toc(-0x4a1c)},{29,c.toc(-0x7be0)},{30,c.toc(-0x7b5c)}});
    c.set(4,c.toc(-0x4a20)); c.set_f(31,c.f32(c.r(4)));
    c.set(3,indirect(c,-0x7b5c)); c.set(5,c.u32(c.r(3)+competition*4)); const auto league=c.r(5);
    if (compare_logical(c,league)==0 || compare(c,c.u8(league+0x43),1)==0) {
        assertion(c,frame,0x104460ac,0x44,0x3c,0x658); c.set(3,c.toc(-0x7bac)); return;
    }
    auto matches=std::int32_t(c.i16(league+0xa4)); compare(c,matches,-1); c.set(22,std::uint32_t(matches));
    if (matches==-1) {
        c.set(3,std::uint32_t(c.i16(league+0x3e))-1);
        matches=std::int16_t(c.r(3)*std::uint32_t(c.i16(league+0x3c))); c.set(22,std::uint32_t(matches));
    }
    const auto records=p.call(0x1045a0a0,0x1044612c,{c.u32(league+8),17,frame.at(0x38),missing,missing}); c.set(27,records);
    if (compare(c,c.i32(frame.at(0x38)))==0) return;
    set_team_style(c,award,indirect(c,-0x7b5c)); p.call(0x1044b160,0x1044617c,{award});
    c.set(31,0x43300000); c.set(30,std::uint32_t(std::int16_t(matches))^0x80000000);
    while (true) {
        c.set_f(0,c.f64(c.r(28)+16)); fp.fcmp(0,c.f(31),c.f(0));
        if (!(c.cr(0)&4)) break;
        c.put32(frame.at(0x70),c.r(30)); keep(c,{{26,records},{25,0}});
        c.set_f(1,c.f64(c.r(28))); c.put32(frame.at(0x6c),c.r(31)); c.set_f(0,c.f64(frame.at(0x6c)));
        c.set_f(0,fp.fsubs(c.f(0),c.f(1))); c.set_f(0,fp.fmuls(c.f(0),c.f(31)));
        c.put64(frame.at(0x64),fp.fctiwz(0,c.f(0))); c.set(22,extended(c.i16(frame.at(0x6a))));
        for (unsigned index=0;compare(c,index,c.i32(frame.at(0x38)))<0;++index) {
            const auto record=records+index*0x30;
            c.set(3,c.u32(record+4)*Person::size); c.set(6,c.u8(record+12)); c.set(4,indirect(c,-0x7be0)+c.r(3));
            if (compare(c,std::int32_t(c.r(6)-c.u8(record+13)),c.r(22))>0) {
                c.set(5,std::uint32_t(c.i16(record+0x1a))^0x80000000); c.set(7,0);
                ratio(c,frame,0x64,0x6c,c.i16(record+0x1a),c.r(6),c.r(28));
                c.set(5,c.u32(c.r(4)+0x24));
                p.call(0x1044aca0,0x1044622c,{award,c.r(4),c.r(5)});
                if (compare(c,std::int8_t(c.r(3)))!=0) { c.set(24,1); break; }
            }
            keep(c,{{25,index+1},{26,record+0x30}});
        }
        if (compare(c,std::int8_t(c.r(24)))!=0) break;
        c.set_f(0,c.f64(c.r(28)+8)); c.set_f(31,fp.fsub(c.f(31),c.f(0))); c.set_f(31,fp.frsp(c.f(31)));
    }
    finish_team(c,p,award,records,0x10446278,0x10446288,0x10446290);
}
void select_nation_team(Cpu& c,std::uint32_t competition,std::uint32_t award) {
    Frame frame(c,0x104462b0,0xb0,{.first_gpr=22,.gpr_offset=-0x38,.first_fpr=31}); Program p{c}; Fpu fp(c);
    keep(c,{{22,award},{23,0},{24,0},{25,c.toc(-0x4a18)},{26,c.toc(-0x7ba8)},
            {28,c.toc(-0x4a20)},{29,c.toc(-0x4a1c)},{30,c.toc(-0x7be0)},{31,c.toc(-0x7b5c)}});
    c.set_f(31,c.f32(c.r(28))); c.set(3,indirect(c,-0x7b5c)); c.set(27,c.u32(c.r(3)+competition*4));
    const auto tournament=c.r(27);
    if (compare_logical(c,tournament)==0) return;
    p.call(0x10192130,0x10446308,{c.u32(tournament+4)});
    if (compare(c,std::int8_t(c.r(3)),1)==0 || compare(c,std::int8_t(c.r(3)))==0) c.set(24,1);
    // The original repeats this null test after the call. r27 is preserved by
    // that service; its false outcome is dominated by the entry null test.
    compare_logical(c,tournament);
    const auto records=p.call(0x1045a0a0,0x10446398,{c.u32(tournament+8),17,frame.at(0x38),missing,missing}); c.set(27,records);
    if (compare(c,c.i32(frame.at(0x38)))==0) return;
    set_team_style(c,award,indirect(c,-0x7b5c)); p.call(0x1044b160,0x104463e8,{award});
    while (true) {
        c.set_f(0,c.f64(c.r(29)+16)); fp.fcmp(0,c.f(31),c.f(0)); if (!(c.cr(0)&4)) break;
        c.set_f(0,c.f32(c.r(28)+8)); keep(c,{{26,records},{25,0}});
        c.set_f(0,fp.fmuls(c.f(0),c.f(31))); c.put64(frame.at(0x6c),fp.fctiwz(0,c.f(0)));
        c.set(31,extended(c.i16(frame.at(0x72))));
        for (unsigned index=0;compare(c,index,c.i32(frame.at(0x38)))<0;++index) {
            const auto record=records+index*0x30;
            c.set(3,c.u32(record+4)*Person::size); c.set(8,c.u8(record+12)); c.set(4,indirect(c,-0x7be0)+c.r(3));
            if (compare(c,std::int32_t(c.r(8)-c.u8(record+13)),c.r(31))>0) {
                compare(c,std::int8_t(c.r(24))); c.set(5,c.u32(c.r(4)+(c.r(24)?0x20:0x24)));
                c.set(6,std::uint32_t(c.i16(record+0x1a))^0x80000000); c.set(7,0);
                ratio(c,frame,0x6c,0x64,c.i16(record+0x1a),c.r(8),c.r(29));
                p.call(0x1044aca0,0x1044648c,{award,c.r(4),c.r(5)});
                if (compare(c,std::int8_t(c.r(3)))!=0) { c.set(23,1); break; }
            }
            keep(c,{{25,index+1},{26,record+0x30}});
        }
        if (compare(c,std::int8_t(c.r(23)))!=0) break;
        c.set_f(0,c.f64(c.r(29)+8)); c.set_f(31,fp.fsub(c.f(31),c.f(0))); c.set_f(31,fp.frsp(c.f(31)));
    }
    finish_team(c,p,award,records,0x104464dc,0x104464ec,0x104464f4);
}
}
