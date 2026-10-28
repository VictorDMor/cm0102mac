#include "club_records_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::club_records {
using namespace detail;
std::uint32_t archive_season(Cpu& c,std::uint32_t manager,std::uint32_t season) {
    Frame frame(c,0x10011bd0,0x590,{.first_gpr=15});Program p{c};Fpu fp(c);
    keep(c,{{23,c.toc(-0x7b48)},{30,manager},{24,c.toc(slot::news)},{31,season},
            {25,c.toc(-0x7b44)},{26,c.toc(slot::clubs)},{27,c.toc(slot::doubles)},
            {28,c.toc(-0x7be0)},{29,c.toc(-0x7b50)},{16,c.toc(slot::message)},
            {15,c.toc(slot::pool)},{21,frame.at(0x148)}});
    p.call(0x10012d30,0x10011c18,{c.r(21)});
    if(compare_logical(c,season)==0){assertion(c,frame,0x10011c34,0x40,0x38,0x58f,true);c.set(3,0);return 0;}
    Kept<20> club_index(c,0);Kept<22> club_offset(c,0);
    while(compare(c,club_index,c.i32(c.r(29)))<0) {
        Kept<19> club(c,c.u32(c.r(26))+std::uint32_t(club_offset));
        const auto season_index=p.call(0x10191fd0,0x10011c98,{club});
        const auto club_season=c.r(25)+std::uint32_t(std::int32_t(std::int8_t(season_index)))*0x4c;
        if(compare_logical(c,club_season,season)==0) {
            Kept<18> current(c,p.call(0x1001b9c0,0x10011cbc,{manager,club,0}));
            const auto modern_result=p.call(0x1001b830,0x10011cd4,{manager,club,0});c.set(15,modern_result);
            c.set(4,c.u32(manager+28));Kept<17> index(c,c.r(4)+c.u32(club)*index_size);
            const auto year_result=p.call(0x10192610,0x10011cf0,{club});c.set(16,year_result);
            const auto notify=[&](unsigned kind,std::uint32_t controlled_back,std::uint32_t national_back,
                                   std::uint32_t checked_back,std::uint32_t appended_back) {
                bool visible=nonzero_byte(c,p.call(0x1014c2e0,controlled_back,{club,0,1,0,0}));
                if(!visible)visible=nonzero_byte(c,p.call(0x100c7d00,national_back,{club}));
                if(visible && compare(c,c.i16(index+0x68),5)>=0 &&
                   !nonzero_byte(c,p.call(0x1001be60,checked_back,{manager,c.u32(club),kind})))
                    p.call(0x1001bd60,appended_back,{manager,c.u32(club),kind});
            };
            if(nonzero_byte(c,p.call(0x10013cc0,0x10011d0c,{manager,6,club,1})) && compare_logical(c,current)!=0) {
                c.set(3,c.u32(current+0x90));
                if(compare(c,c.r(3),-1)>0 &&
                   (compare(c,c.i32(c.r(15)+0x90),-1)==0 || compare(c,c.r(3),c.i32(c.r(15)+0x90))>0)) {
                    if(compare_logical(c,c.r(15))==0)c.set(15,p.call(0x1001b830,0x10011d54,{manager,club,1}));
                    c.put32(c.r(15)+0x90,c.u32(current+0x90));c.put16(c.r(15)+0x94,c.u16(current+0x94));
                    c.put16(c.r(15)+0x94,c.r(16));notify(6,0x10011d84,0x10011d94,0x10011db8,0x10011dd0);
                }
            }
            if(compare_logical(c,current)!=0) {
                c.set_f(1,c.f64(c.r(27)+0x48));fp.fcmp(0,c.f(1),c.f32(current+0x23c));
                if(c.cr(0)&2)p.call(0x1001bb50,0x10011df4,{manager,club});
            }
            if(nonzero_byte(c,p.call(0x10013cc0,0x10011e08,{manager,20,club,1})) && compare_logical(c,current)!=0) {
                const auto average=double(c.f32(current+0x23c)),empty=c.f64(c.r(27)+0x48);
                fp.fcmp(0,average,empty);
                if(c.cr(0)&4) {
                    c.set_f(1,c.f32(c.r(15)+0x23c));fp.fcmp(0,empty,c.f(1));
                    bool improve=(c.cr(0)&2)!=0;
                    if(!improve){fp.fcmp(0,average,c.f(1));improve=(c.cr(0)&4)!=0;}
                    if(improve) {
                        if(compare_logical(c,c.r(15))==0)c.set(15,p.call(0x1001b830,0x10011e54,{manager,club,1}));
                        assign(c,c.r(15)+0x23c,current+0x23c,RecordKind::average);
                        c.put16(c.r(15)+0x25a,c.r(16));notify(20,0x10011ec4,0x10011ed4,0x10011ef8,0x10011f10);
                    }
                }
            }
            if(nonzero_byte(c,p.call(0x10013cc0,0x10011f24,{manager,32,club,2})) && compare_logical(c,current)!=0 &&
               nonzero_byte(c,p.call(0x1001bec0,0x10011f44,{manager,club,current}))) {
                if(nonzero_byte(c,p.call(0x102cdbf0,0x10011f58,{c.u32(c.r(24)),club}))) {
                    p.call(0x102bdf90,0x10011f70,{frame.at(0x60),1000,0});
                    p.call(0x1001c290,0x10011f84,{c.u32(club),current,32,frame.at(0x60)});
                    p.call(0x102cc140,0x10011f94,{c.u32(c.r(24)),frame.at(0x60),club});
                }
                p.call(0x103865b0,0x10011fb8,{c.u32(c.r(23)),c.u32(c.r(28))+c.u32(current+0x3ec)*0x70,
                            club,c.r(16),c.u8(season+0x18)});
            }
            if(compare_logical(c,current)!=0) {
                c.put16(frame.at(0x144),c.r(16));copy_archive_record(c,c.r(21),current,0x10011fd0);
                p.call(0x10748ce0,0x10012158,{c.u32(manager),index+8,index+12,frame.at(0x144)});
                p.call(0x1001c160,0x10012160,{current});
            }
            c.put8(index+16,0);c.put8(index+17,0);
            for(auto field:{20u,24u,44u,64u,84u})c.put32(index+field,0);
            c.set(3,std::uint32_t(std::int32_t(c.i16(index+0x68))));c.put16(index+0x68,c.r(3)+1);
        }
        club_offset=std::uint32_t(club_offset)+0x248;club_index=std::uint32_t(club_index)+1;
    }
    c.set(3,1);return 1;
}
}
