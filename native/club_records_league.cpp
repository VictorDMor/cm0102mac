#include "club_records_detail.hpp"

namespace cm::aot::club_records {
using namespace detail;
std::uint32_t record_league_positions(Cpu& c,std::uint32_t manager,std::uint32_t competition,std::uint32_t update_modern) {
    Frame frame(c,0x10011750,0xd0,{.first_gpr=17});Program p{c};
    keep(c,{{18,competition},{26,c.toc(-0x7b64)},{27,c.toc(-0x7bbc)},{17,manager},
            {22,c.toc(slot::success)},{19,update_modern},{21,c.toc(slot::message)},{20,c.toc(slot::pool)}});
    if(compare(c,competition)==0){assertion(c,frame,0x10011798,0x68,0x40,0x4e9);c.set(3,0);return 0;}
    if(compare(c,c.i8(competition+0x44),-1)!=0){assertion(c,frame,0x10011800,0x48,0x38,0x4f1);c.set(3,0);return 0;}
    Kept<24> root(c,competition);Kept<23> competition_id(c,c.u32(c.u32(competition+4)));
    for(;;) {
        p.call(0x1049eb30,0x1001185c,{c.r(18),0});
        Kept<22> index(c,0);Kept<25> offset(c,0);
        while(compare(c,index,c.i16(c.r(18)+0x3e))<0) {
            const auto standing=c.u32(c.r(18)+0xb0)+std::uint32_t(offset);c.set(3,standing);
            if(compare(c,c.i8(standing+20))!=0) {
                Kept<21> club(c,c.u32(standing));
                if(compare_logical(c,club)!=0) {
                    Kept<28> season(c,p.call(0x1001b9c0,0x1001189c,{manager,club,1}));
                    Kept<29> modern(c,p.call(0x1001b830,0x100118b0,{manager,club,1}));
                    c.set(4,c.u32(manager+28));Kept<20> record_index(c,c.r(4)+c.u32(club)*index_size);
                    Kept<30> year(c,p.call(0x10192610,0x100118cc,{club}));
                    c.set(4,c.u32(c.r(18)+0xb0));Kept<31> position(c,c.u8(c.r(4)+std::uint32_t(offset)+4));
                    const auto improves=[&](std::uint32_t record,unsigned field,bool lowest,bool historical) {
                        c.set(6,c.u32(record+field+4));
                        if(compare(c,c.r(6))<0)return true;
                        if(historical && compare(c,c.r(6),c.i32(c.r(26)))>0)return true;
                        c.set(3,c.r(6)*index_size);c.set(7,c.u32(c.r(27)));c.set(4,c.r(7)+24);
                        c.set(5,std::uint32_t(competition_id)*index_size);
                        c.set(3,std::uint32_t(std::int32_t(c.i16(c.r(5)+c.r(4)))));
                        const auto previous=c.i16(c.r(7)+c.r(6)*index_size+24);
                        const auto quality=compare(c,c.r(3),previous);
                        if(lowest?quality<0:quality>0)return true;
                        if(compare(c,competition_id,c.r(6))!=0)return false;
                        c.set(3,std::uint32_t(position)&0xff);
                        const auto place=compare_logical(c,c.r(3),c.u8(record+field));
                        return lowest?place>0:place<0;
                    };
                    const auto notify=[&](unsigned kind,std::uint32_t controlled_back,std::uint32_t national_back,
                                           std::uint32_t check_back,std::uint32_t append_back) {
                        bool visible=nonzero_byte(c,p.call(0x1014c2e0,controlled_back,{club,0,1,0,0}));
                        if(!visible)visible=nonzero_byte(c,p.call(0x100c7d00,national_back,{club}));
                        if(visible && compare(c,c.i16(record_index+0x68),5)>=0 &&
                           !nonzero_byte(c,p.call(0x1001be60,check_back,{manager,c.u32(club),kind})))
                            p.call(0x1001bd60,append_back,{manager,c.u32(club),kind});
                    };
                    if(nonzero_byte(c,p.call(0x10013cc0,0x100118f0,{manager,1,club,1}))) {
                        if(improves(season,0,false,false)) {c.put8(season,position);c.put32(season+4,competition_id);c.put16(season+8,0);}
                        if(compare(c,std::int8_t(update_modern))!=0 && improves(modern,0,false,true)) {
                            c.put8(modern,position);c.put32(modern+4,competition_id);c.put16(modern+8,year);
                            notify(1,0x100119d0,0x100119e0,0x10011a04,0x10011a1c);
                        }
                    }
                    if(nonzero_byte(c,p.call(0x10013cc0,0x10011a30,{manager,2,club,1}))) {
                        if(improves(season,12,true,false)) {c.put8(season+12,position);c.put32(season+16,competition_id);c.put16(season+20,0);}
                        if(compare(c,std::int8_t(update_modern))!=0 && improves(modern,12,true,true)) {
                            c.put8(modern+12,position);c.put32(modern+16,competition_id);c.put16(modern+20,year);
                            notify(2,0x10011b10,0x10011b20,0x10011b44,0x10011b5c);
                        }
                    }
                }
            }
            offset=std::uint32_t(offset)+0x44;index=std::uint32_t(index)+1;
        }
        c.set(4,std::uint32_t(std::int32_t(c.i8(c.r(18)+0x44))));
        if(compare(c,c.r(4),c.i32(root+0x30))>=0)break;
        const auto next=p.call(0x10450b60,0x10011b90,{root,c.r(4)+1});
        if(compare(c,c.u8(next+0x43),2)!=0)break;
        c.set(4,std::uint32_t(std::int32_t(c.i16(next+0x3e))));
        if(compare(c,c.r(4),c.i16(c.r(18)+0x3e))!=0)break;
        c.set(18,next);
    }
    c.set(3,1);return 1;
}
}
