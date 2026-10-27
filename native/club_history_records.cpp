#include "club_history_detail.hpp"
namespace cm::aot::club_history {
using namespace detail;
std::uint32_t record_competition(Cpu& c,std::uint32_t address,std::uint32_t competition) {
    Frame f(c,0x1000cfc0,0x110,{.first_gpr=20});
    keep(c,{{31,competition},{26,c.toc(slot::league_exception)},{27,c.toc(-0x7bac)},
            {30,address},{28,c.toc(-0x7ba8)},{29,c.toc(slot::pool)}});Program p{c};History h{c,address};
    if(compare(c,competition)==0){assertion(c,f,0x1000d000,0x9c,0x48,0xab);return 0;}
    if(compare(c,c.i8(competition+0x44),-1)!=0){assertion(c,f,0x1000d068,0x7c,0x40,0xb3);return 0;}
    if(byte_nonzero(c,p.call(0x10494d20,0x1000d0bc,{c.u32(c.toc(slot::excluded_competitions)),c.u32(competition+4)})))return 0;
    if(byte_nonzero(c,p.call(0x104646d0,0x1000d0d8,{c.u32(c.u32(competition+4))}))) {
        c.set(23,competition);
        for(;;) {
            bool store=false;
            if(compare(c,c.i16(c.r(23)+0xa6),7)>0) {
                c.set(3,c.u32(c.u32(competition+4)));
                store=compare(c,c.r(3),c.i32(c.r(26)))!=0;
            }
            if(!store) {
                c.set(3,c.u32(c.u32(competition+4)));
                if(compare(c,c.r(3),c.i32(c.r(26)))==0)store=compare(c,c.u8(c.r(23)+0x44),1)==0;
            }
            if(store) {
                p.call(0x1049eb30,0x1000d130,{c.r(23),0});
                c.set(3,c.u32(competition+4));keep(c,{{24,0},{27,0}});
                c.put32(f.at(0xbc),c.u32(c.r(3)));c.put16(f.at(0xc0),c.u16(competition+0x40));
                while(compare(c,c.r(24),c.i16(c.r(23)+0x3e))<0) {
                    const auto entry=c.u32(c.r(23)+0xb0)+c.r(27);
                    c.set(6,entry+0x10);c.set(5,h.index());c.set(4,c.u32(c.u32(entry)));
                    c.set(3,c.u8(entry+4));c.put8(f.at(0xc2),c.r(3));
                    c.set(3,c.r(4)*16);c.set(4,c.r(5)+c.r(3));
                    c.set(3,c.u32(c.r(6)));c.put32(f.at(0xc4),c.r(3));
                    for(unsigned i=4;i<8;++i){c.set(3,c.u8(c.r(6)+i));c.put8(f.at(0xc4+i),c.r(3));}
                    for(unsigned i=8;i<14;i+=2){c.set(3,extended(c.i16(c.r(6)+i)));c.put16(f.at(0xc4+i),c.r(3));}
                    c.set(3,c.u32(c.u32(c.r(23)+4)));
                    if(compare(c,c.r(3),c.i32(c.r(26)))==0){c.set(3,c.u8(f.at(0xc8)));c.put8(f.at(0xc8),c.r(3)+14);}
                    p.call(0x10748ce0,0x1000d1ec,{h.league_file(),c.r(4),c.r(4)+4,f.at(0xbc)});
                    c.set(27,c.r(27)+0x44);c.set(24,c.r(24)+1);
                }
            }
            c.set(4,std::uint32_t(c.i8(c.r(23)+0x44)));
            if(compare(c,c.r(4),c.i32(competition+0x30))>=0)break;
            p.call(0x10450b60,0x1000d220,{competition,c.r(4)+1});
            if(compare(c,c.u8(c.r(3)+0x43),2)!=0)break;
            c.set(23,c.r(3));
        }
        return 1;
    }
    if(compare(c,c.i8(competition+0x45))==0)return 0;
    c.set(21,p.call(0x10775200,0x1000d258,{c.u32(c.toc(slot::clubs)),1}));
    if(compare(c,c.r(21))==0){assertion(c,f,0x1000d274,0x5c,0x38,0xfd);return 0;}
    c.set(3,c.u32(competition+4));c.put32(f.at(0x50),c.u32(c.r(3)));c.put16(f.at(0x54),c.u16(competition+0x40));
    c.set(25,c.u32(competition+0x30));
    while(compare(c,c.r(25),-1)>=0) {
        c.set(29,p.call(0x10450b60,0x1000d2e0,{competition,c.r(25)}));
        if(compare(c,c.u8(c.r(29)+0x43),2)==0) {
            keep(c,{{23,0},{26,0}});
            while(compare(c,c.r(23),c.i16(c.r(29)+0x3e))<0) {
                c.set(6,c.u32(c.r(29)+0xb0)+c.r(26));c.set(3,c.u32(c.u32(c.r(6))));
                if(compare(c,c.i8(c.r(21)+c.r(3)))==0) {
                    c.set(5,h.index());c.set(4,c.r(3)*16);c.set(3,c.u8(c.r(6)+0x40));
                    c.set(5,c.r(5)+c.r(4));c.set(6,f.at(0x50));c.put8(f.at(0x56),c.r(3));
                    c.set(4,c.r(5)+8);c.set(5,c.r(5)+12);c.set(3,extended(c.i16(c.r(29)+0x3a)));
                    c.put16(f.at(0x58),c.r(3));c.put16(f.at(0x5a),missing);
                    p.call(0x10748ce0,0x1000d350,{h.cup_file()});
                    c.set(3,c.u32(c.u32(c.r(29)+0xb0)+c.r(26)));c.set(4,1);c.put8(c.r(21)+c.u32(c.r(3)),1);
                }
                c.set(26,c.r(26)+0x44);c.set(23,c.r(23)+1);
            }
        } else {
            c.set(3,extended(c.i16(c.r(29)+0x3c)));c.set(24,c.r(3)-1);
            c.set(28,c.r(24)*0x78);c.set(27,c.r(24)*4);
            while(compare(c,c.r(24))>=0) {
                keep(c,{{22,0},{26,0}});
                for(;;) {
                    c.set(3,c.u32(c.r(29)+0xa0));
                    if(compare(c,c.r(22),c.i16(c.r(3)+c.r(28)+0x1c))>=0)break;
                    c.set(20,c.u32(c.u32(c.r(29)+0xac)+c.r(27))+c.r(26));
                    for(unsigned side=0;side<2;++side) {
                        auto participant=c.i16(c.r(20)+side*2);
                        if(compare(c,participant,-1)==0)continue;
                        c.set(3,c.u32(c.r(29)+0xa4));c.set(5,c.r(3)+std::uint32_t(participant)*8);
                        c.set(3,c.u32(c.r(5)));c.set(23,c.u32(c.r(3)));
                        if(compare(c,c.i8(c.r(21)+c.r(23)))!=0)continue;
                        c.set(4,h.index());c.set(3,c.r(23)*16);c.set(6,f.at(0x50));c.set(3,c.r(4)+c.r(3));
                        c.put8(f.at(0x56),c.u8(c.r(5)+5));c.set(4,c.r(3)+8);c.set(5,c.r(3)+12);
                        c.put16(f.at(0x58),c.u16(c.r(29)+0x3a));c.set(3,c.u32(c.r(29)+0xa0));
                        c.put16(f.at(0x5a),c.u16(c.r(3)+c.r(28)+8));
                        p.call(0x10748ce0,side?0x1000d48c:0x1000d418,{h.cup_file()});c.put8(c.r(21)+c.r(23),1);
                    }
                    c.set(26,c.r(26)+0x36);c.set(22,c.r(22)+1);
                }
                c.set(27,c.r(27)-4);c.set(28,c.r(28)-0x78);c.set(24,c.r(24)-1);
            }
        }
        c.set(25,c.r(25)-1);
    }
    p.call(0x10775170,0x1000d4d8,{c.r(21)});return 1;
}
}
