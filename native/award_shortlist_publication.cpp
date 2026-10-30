#include "award_shortlist_detail.hpp"

namespace cm::aot::award_shortlist {
using namespace detail;
void publish(Cpu& c,std::uint32_t object,std::uint32_t date,std::uint32_t first,std::uint32_t second) {
    Frame frame(c,0x10438450,0x140,{.first_gpr=25});Program p{c};
    keep(c,{{25,date},{26,first},{27,object},{28,second},{29,c.toc(-0x7bf0)},
            {30,c.toc(-0x7be4)},{31,c.toc(-0x7be0)}});
    p.call(0x10768300,0x10438490,{c.u32(object),std::uint32_t(c.i16(object+4)),16,c.toc(-0x4a68)});
    p.call(0x10437730,0x104384a0,{c.u32(c.toc(-0x7bb4)),object});c.put16(object+0x3c,c.r(3));
    c.put16(object+0x42,c.u16(date+2));c.put16(object+0x40,c.u16(date));c.put8(object+0x44,c.u8(date+4));
    p.call(0x102bdf90,0x104384cc,{frame.at(0x38),0x1c,0});
    p.call(0x102cb4b0,0x104384dc,{frame.at(0x38),0,first});
    p.call(0x102cb4b0,0x104384ec,{frame.at(0x38),1,second});
    p.call(0x102cb4b0,0x104384fc,{frame.at(0x38),2,std::uint32_t(c.i16(date))});
    p.call(0x102cb4b0,0x1043850c,{frame.at(0x38),3,c.u16(date+2)});
    p.call(0x102cb4b0,0x1043851c,{frame.at(0x38),4,0});
    c.set(28,0);
    do {
        c.set(3,c.r(28)+1);c.set(4,c.u32(c.r(31)));
        auto manager=c.r(4)+(c.u32(c.r(30))-c.r(3))*0x70;c.set(6,manager);
        if (compare(c,c.i8(manager+0x6b))!=0) {
            c.set(3,c.u8(object+0x28));const auto electorate=std::int8_t(c.r(3));
            bool eligible=false;
            if (compare(c,electorate)==0) {
                auto club=c.u32(manager+0x24);c.set(3,club);
                if (compare_logical(c,club)!=0) {
                    auto nation=c.u32(club+0x30);c.set(3,nation);
                    if (compare_logical(c,nation)!=0) { c.set(3,c.u32(nation));eligible=compare(c,c.r(3),c.u32(object+0x2c))==0; }
                }
            } else if (compare(c,electorate,1)==0) {
                auto club=c.u32(manager+0x24);c.set(3,club);
                if (compare_logical(c,club)!=0) {
                    auto nation=c.u32(club+0x2c);c.set(3,nation);
                    if (compare_logical(c,nation)!=0) { c.set(3,c.u32(nation+0x68));eligible=compare(c,c.r(3),c.u32(object+0x30))==0; }
                }
            } else if (compare(c,electorate,2)==0 || compare(c,electorate,3)==0) {
                auto club=c.u32(manager+(electorate==2?0x24:0x20));c.set(3,club);
                if (compare_logical(c,club)!=0) {
                    auto nation=c.u32(club+0x2c);c.set(3,nation);
                    if (compare_logical(c,nation)!=0) {
                        auto region=c.u32(nation+0x74);c.set(3,region);
                        if (compare_logical(c,region)!=0) { c.set(3,c.u32(region+8));eligible=compare(c,c.r(3),c.u32(object+0x34))==0; }
                    }
                }
            } else if (compare(c,electorate,4)==0) eligible=compare_logical(c,c.u32(manager+0x24))!=0;
            else if (compare(c,electorate,5)==0) eligible=compare_logical(c,c.u32(manager+0x20))!=0;
            else eligible=true;
            if (eligible) {
                c.set(5,c.u32(manager+0x24));
                if (compare_logical(c,c.r(5))!=0) p.call(0x102cc140,0x10438678,{c.u32(c.r(29)),frame.at(0x38),c.r(5)});
                else p.call(0x102cc140,0x1043868c,{c.u32(c.r(29)),frame.at(0x38),c.u32(c.r(6)+0x20)});
            }
        }
        c.set(28,c.r(28)+1);
    } while (compare(c,c.r(28),16)<0);
}
}
