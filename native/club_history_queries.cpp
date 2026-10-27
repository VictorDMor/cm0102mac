#include "club_history_detail.hpp"
namespace cm::aot::club_history {
using namespace detail;
namespace {
// The league and cup versions have identical logic and frame layout; their
// original call sites and diagnostic line numbers differ by fixed offsets.
std::uint32_t competitions(Cpu& c,std::uint32_t list,bool cup) {
    const std::uint32_t delta=cup?0x280:0;
    Frame f(c,0x1000d8a0+delta,0xe0,{.first_gpr=23});
    keep(c,{{25,list},{23,c.toc(slot::record_count)},{29,c.toc(-0x7bac)},
            {30,c.toc(-0x7ba8)},{31,c.toc(slot::pool)}});Program p{c};
    auto call=[&](unsigned target,unsigned back,std::initializer_list<std::uint32_t> args={}) {return p.call(target,back+delta,args);};
    auto error=[&](unsigned at,unsigned name,unsigned ext,unsigned line) {assertion(c,f,at+delta,name,ext,line+(cup?0x60:0));};
    if(compare(c,list)==0)return 0;
    c.set(28,call(0x107735f0,0x1000d8dc,{c.u32(c.r(23))*4}));
    if(compare(c,c.r(28))==0){error(0x1000d8f8,0x90,0x48,0x238);return 0;}
    c.set(3,c.u32(c.r(23)));keep(c,{{27,0},{24,0}});
    c.set(26,call(0x107735f0,0x1000d94c));
    if(compare(c,c.r(26))==0){error(0x1000d968,0x70,0x40,0x243);call(0x10773610,0x1000d9ac,{c.r(28)});return 0;}
    c.set({{5,0},{4,0}});
    while(compare(c,c.r(5),c.i32(c.r(23)))<0){c.put8(c.r(3),0);c.set(5,c.r(5)+1);c.set(3,c.r(3)+1);}
    c.put32(list+12,c.u32(list+4));c.set(23,1);
    while(compare_logical(c,call(0x10738e00,0x1000da18,{c.r(25)}))!=0) {
        c.set(4,c.u32(c.r(3)));
        if(compare(c,c.i8(c.r(26)+c.r(4)))==0) {
            c.put32(c.r(28)+c.r(24),c.r(4));c.set(27,c.r(27)+1);c.set(24,c.r(24)+4);
            c.put8(c.r(26)+c.u32(c.r(3)),c.r(23));
        }
    }
    call(0x10773610,0x1000da28,{c.r(26)});
    call(0x10768300,0x1000da3c,{c.r(28),c.r(27),4,c.toc(slot::compare_tv)});
    keep(c,{{24,0},{25,0},{23,0}});
    while(compare(c,c.r(24),c.r(27))<0) {
        if(compare_logical(c,c.r(25))==0) {
            c.set(25,call(0x10773520,0x1000da60,{16}));
            if(compare(c,c.r(25))!=0)call(0x10738c00,0x1000da6c);
            if(compare_logical(c,c.r(25))==0) {
                error(0x1000da88,0x50,0x38,0x26e);call(0x10773610,0x1000dacc,{c.r(28)});return 0;
            }
        }
        call(0x10738d10,0x1000dae4,{c.r(25),c.r(28)+c.r(23),4});c.set(23,c.r(23)+4);c.set(24,c.r(24)+1);
    }
    call(0x10773610,0x1000dafc,{c.r(28)});return c.r(25);
}
}
std::uint32_t league_competitions(Cpu& c,std::uint32_t list){return competitions(c,list,false);}
std::uint32_t cup_competitions(Cpu& c,std::uint32_t list){return competitions(c,list,true);}
std::uint32_t league_history(Cpu& c,std::uint32_t address,std::uint32_t club) {
    Frame f(c,0x1000d4f0,0x90,{.first_gpr=28});
    keep(c,{{31,c.toc(slot::pool)},{30,c.toc(-0x7ba8)},{29,club},{28,address}});Program p{c};History h{c,address};
    if(compare(c,club)==0){assertion(c,f,0x1000d534,0x44,0x3c,0x184,true);return 0;}
    c.set(30,0);enter(c,0x1000d598,{club,f.at(0x64),f.at(0x38),0,0});
    result(c,current_league(c,club,f.at(0x64),f.at(0x38),0,0));
    if(byte_nonzero(c,c.r(3)) && compare(c,c.i8(f.at(0x38)))==0) {
        c.set(30,p.call(0x10773520,0x1000d5b4,{16}));
        if(compare(c,c.r(30))!=0)p.call(0x10738c00,0x1000d5c0);
        p.call(0x10738d10,0x1000d5d0,{c.r(30),f.at(0x64),24});
    }
    const auto index=h.club_index(club);
    c.set(31,p.call(0x10749fd0,0x1000d5f0,{h.league_file(),c.u32(index),c.u32(index+4)}));
    if(compare(c,c.r(31))!=0) {
        c.put32(c.r(31)+12,c.u32(c.r(31)+8));
        for(;;) {
            c.set(29,p.call(0x10738e30,0x1000d63c,{c.r(31)}));
            if(compare(c,c.r(29))==0)break;
            if(compare_logical(c,c.r(30))==0) {
                c.set(28,p.call(0x10773520,0x1000d614,{16}));
                if(compare(c,c.r(28))!=0)p.call(0x10738c00,0x1000d620);
                c.set(30,c.r(28));
            }
            p.call(0x10738d10,0x1000d634,{c.r(30),c.r(29),24});
        }
        p.call(0x10738c20,0x1000d650,{c.r(31),1});
    }
    return c.r(30);
}
std::uint32_t cup_history(Cpu& c,std::uint32_t address,std::uint32_t club) {
    Frame f(c,0x1000d680,0xa0,{.first_gpr=23});
    keep(c,{{24,club},{29,c.toc(slot::competition_count)},{30,c.toc(slot::competitions)},
            {23,address},{31,c.toc(slot::excluded_competitions)},{26,c.toc(-0x7ba8)},{25,c.toc(slot::pool)}});
    Program p{c};History h{c,address};
    if(compare(c,club)==0){assertion(c,f,0x1000d6c4,0x4c,0x38,0x1d0,true);return 0;}
    keep(c,{{27,0},{26,0},{28,0}});
    while(compare(c,c.r(26),c.i32(c.r(29)))<0) {
        c.set(3,c.u32(c.r(30)));c.set(25,c.u32(c.r(3)+c.r(28)));
        if(compare_logical(c,c.r(25))!=0) {
            c.set(3,c.r(25));
            if(!byte_nonzero(c,p.virtual_call(c.r(25),0x14,0x1000d740)) &&
               !byte_nonzero(c,p.call(0x104646d0,0x1000d758,{c.u32(c.u32(c.r(25)+4))})) &&
               !byte_nonzero(c,p.call(0x10494d20,0x1000d76c,{c.u32(c.r(31)),c.u32(c.r(25)+4)}))) {
                c.put32(f.at(0x40),c.u32(c.u32(c.r(25)+4)));
                c.set(9,extended(c.i16(c.r(25)+0x40)));c.put16(f.at(0x44),c.r(9));
                c.put8(f.at(0x46),0);c.put16(f.at(0x48),missing);c.put16(f.at(0x4a),missing);
                p.call(0x10450710,0x1000d7b4,{c.r(25),c.r(24),f.at(0x48),f.at(0x4a),f.at(0x46),0});
                if(compare(c,c.i16(f.at(0x48)),-1)>0) {
                    if(compare_logical(c,c.r(27))==0) {
                        c.set(25,p.call(0x10773520,0x1000d7d0,{16}));
                        if(compare(c,c.r(25))!=0)p.call(0x10738c00,0x1000d7dc);
                        c.set(27,c.r(25));
                    }
                    p.call(0x10738d10,0x1000d7f0,{c.r(27),f.at(0x40),12});
                }
            }
        }
        c.set(28,c.r(28)+4);c.set(26,c.r(26)+1);
    }
    const auto index=h.club_index(club);
    c.set(23,p.call(0x10749fd0,0x1000d824,{h.cup_file(),c.u32(index+8),c.u32(index+12)}));
    if(compare(c,c.r(23))!=0) {
        c.put32(c.r(23)+12,c.u32(c.r(23)+8));
        for(;;) {
            c.set(24,p.call(0x10738e30,0x1000d874,{c.r(23)}));
            if(compare(c,c.r(24))==0)break;
            if(compare_logical(c,c.r(27))==0) {
                c.set(25,p.call(0x10773520,0x1000d84c,{16}));
                if(compare(c,c.r(25))!=0)p.call(0x10738c00,0x1000d858);
                c.set(27,c.r(25));
            }
            p.call(0x10738d10,0x1000d86c,{c.r(27),c.r(24),12});
        }
        p.call(0x10738c20,0x1000d888,{c.r(23),1});
    }
    return c.r(27);
}
std::uint32_t current_league(Cpu& c,std::uint32_t club,std::uint32_t row,std::uint32_t provisional,
                             std::uint32_t stage_out,std::uint32_t historical_date) {
    Frame f(c,0x1000e2b0,0xb0,{.first_gpr=19});
    keep(c,{{22,stage_out},{30,c.toc(slot::base_league)},{31,c.toc(slot::current_date)},
            {19,club},{26,c.toc(-0x7ba8)},{20,row},{24,c.toc(slot::pool)},{21,provisional},{27,0},{23,7}});Program p{c};
    if(compare(c,stage_out)!=0)c.put32(stage_out,0);
    if(compare_logical(c,club)==0){assertion(c,f,0x1000e310,0x50,0x48,0x36a,true);return 0;}
    c.set(4,c.u32(club+0x2c));c.set(24,c.u32(club+0x30));
    if(compare_logical(c,c.r(4))!=0) {
        c.set(3,c.toc(slot::special_nation));c.set(4,c.u32(c.r(4)+0x68));
        if(compare(c,c.r(4),c.i32(c.r(3)))==0 && byte_nonzero(c,historical_date)) {
            p.call(0x107156f0,0x1000e39c,{f.at(0x40),1,1,2001,missing});c.set(4,extended(c.i16(c.r(31))));
            if(compare(c,c.r(4),c.i16(c.r(3)))>0) {
                p.call(0x107156f0,0x1000e3c4,{f.at(0x38),20,5,2001,missing});c.set(4,extended(c.i16(c.r(31))));
                if(compare(c,c.r(4),c.i16(c.r(3)))<0)c.set(24,c.u32(club+0x38));
            }
        }
    }
    if(compare_logical(c,c.r(24))!=0) {
        c.set(3,c.u32(c.toc(slot::competitions)));c.set(29,c.u32(c.r(3)+c.u32(c.r(24))*4));
        if(compare_logical(c,c.r(29))!=0) {
            c.set(5,c.u32(c.r(29)+4));c.set(6,std::uint32_t(std::int8_t(c.u32(c.r(29)+0x30))));
            if(compare_logical(c,c.r(5))!=0) {
                c.set(3,c.toc(slot::single_stage));c.set(4,c.u32(c.r(5)));
                if(compare(c,c.r(4),c.i32(c.r(3)))==0)c.set(6,missing);
            }
            if(compare_logical(c,c.r(5))!=0) {
                c.set(3,c.toc(slot::league_exception));c.set(4,c.u32(c.r(5)));
                if(compare(c,c.r(4),c.i32(c.r(3)))==0)c.set(23,0);
            }
            c.set(26,std::uint32_t(std::int8_t(c.r(6))));c.set(31,c.r(26)*4);
            while(compare(c,std::int8_t(c.r(26)),-1)>=0) {
                if(compare(c,std::int8_t(c.r(26)),-1)==0)c.set(3,c.r(29));
                else c.set(3,c.u32(c.u32(c.r(29)+12)+c.r(31)));
                if(compare(c,c.u8(c.r(3)+0x43),2)==0) {
                    c.set(4,extended(c.i16(c.r(3)+0xa6)));
                    if(compare(c,c.r(4),std::int8_t(c.r(23)))>0) {
                        c.set(28,c.r(3));c.put32(c.r(3)+0xac,0);
                        p.call(0x1049eb30,0x1000e4a0,{c.r(3),0});
                        c.set({{4,c.u32(c.r(30))},{3,std::uint32_t(std::int8_t(c.r(26)))},{9,0},{6,0}});
                        while(compare(c,c.r(9),c.i16(c.r(28)+0x3e))<0) {
                            c.set(7,c.u32(c.r(28)+0xb0)+c.r(6));c.set(8,c.u32(c.r(7)));
                            if(compare_logical(c,c.r(8),club)==0) {
                                c.set(5,c.u32(c.r(29)+4));
                                if(compare(c,c.i32(c.r(5)),c.r(4))==0 || compare(c,c.r(3),-1)==0 ||
                                   compare(c,c.i8(c.r(7)+0x14))!=0) {
                                    c.put16(row+4,c.u16(c.r(28)+0x40));
                                    c.set(6,c.r(9)*0x44);c.set(7,c.r(6)+0x10);
                                    c.put32(row,c.u32(c.r(24)));c.set(6,c.u32(c.r(28)+0xb0)+c.r(6));
                                    c.put8(row+6,c.u8(c.r(6)+4));c.set(7,c.u32(c.r(28)+0xb0)+c.r(7));
                                    copy_standing(c,row+8,c.r(7));
                                    c.set({{3,c.r(29)},{4,1},{5,1}});
                                    p.virtual_call(c.r(29),0x18,0x1000e574);
                                    compare_logical(c,stage_out);c.put8(provisional,c.r(3));
                                    if(stage_out)c.put32(stage_out,c.r(28));return 1;
                                }
                            }
                            if(compare_logical(c,c.r(8),club)==0) {c.set(27,c.r(28));c.set(25,c.r(9));break;}
                            c.set(6,c.r(6)+0x44);c.set(9,c.r(9)+1);
                        }
                    }
                }
                c.set(31,c.r(31)-4);c.set(26,c.r(26)-1);
            }
        }
    }
    if(compare_logical(c,c.r(27))==0)return 0;
    c.put16(row+4,c.u16(c.r(27)+0x40));c.set(5,c.r(25)*0x44);
    compare_logical(c,stage_out);c.set(4,c.r(5)+0x10);c.set(3,c.u32(club+0x30));c.put32(row,c.u32(c.r(3)));
    c.set(3,c.u32(c.r(27)+0xb0)+c.r(5));c.put8(row+6,c.u8(c.r(3)+4));
    c.set(4,c.u32(c.r(27)+0xb0)+c.r(4));copy_standing(c,row+8,c.r(4));
    if(stage_out)c.put32(stage_out,c.r(27));
    // The fallback deliberately leaves the provisional byte untouched.
    return 1;
}
}
