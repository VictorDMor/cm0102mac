#include "club_records_detail.hpp"

namespace cm::aot::club_records {
using namespace detail;
std::uint32_t construct(Cpu& c,std::uint32_t manager,std::uint32_t file) {
    Frame frame(c,0x1000e6b0,0x3c0,{.first_gpr=21});Program p{c};
    keep(c,{{25,c.toc(slot::staff_count)},{22,manager},{26,c.toc(-0x7be0)},{23,file},
            {27,c.toc(slot::success)},{28,c.toc(slot::message)},{29,c.toc(slot::pool)},
            {30,c.toc(-0x5998)},{31,c.toc(-0x5994)}});
    bool disc_found=false;
    do {
        const auto first=c.r(31),second=c.r(30);
        c.set({{10,c.u32(first)},{9,c.u32(first+4)},{8,c.u32(first+8)},
                {7,c.u32(first+12)},{6,c.u8(first+16)},{5,c.u32(second)},
                {4,c.u32(second+4)},{3,c.u32(second+8)}});
        for(unsigned off=0;off<16;off+=4)c.put32(frame.at(0x98+off),c.u32(first+off));
        c.put8(frame.at(0xa8),c.r(6));
        for(unsigned off=0;off<16;off+=4)c.put32(frame.at(0x48+off),c.u32(second+off));
        c.set(21,frame.at(0x49));c.set(24,0);
        auto left=frame.at(0x99),destination=frame.at(0xf5);
        while(compare(c,c.i8(left))!=0) {
            c.set(3,c.u8(left++));const auto other=c.u8(c.r(21));c.set(21,c.r(21)+1);
            c.put8(destination,c.r(3));c.put8(destination+1,other);destination+=2;
        }
        c.set(7,c.u8(frame.at(0x98)));c.put8(frame.at(0xf4),c.r(7)+c.u8(frame.at(0x48)));
        const auto result=p.call(0x1077fee0,0x1000e78c,{0,0,frame.at(0xf4),frame.at(0xac)});
        c.set(2,c.u32(frame.at(0x14)));
        if(compare(c,std::int16_t(result))==0)c.set(24,1);
        if(compare(c,c.r(24))==0) {
            p.call(0x10727890,0x1000e7b0,{frame.at(0x2bc),c.r(29)});
            p.call(0x10727890,0x1000e7bc,{frame.at(0x1f4),c.r(29)+10});
            p.call(0x10769b60,0x1000e7cc,{frame.at(0x2bc),frame.at(0x1f4),1});
        }
        disc_found=compare(c,c.r(24))!=0;
    }while(!disc_found);
    c.set(21,p.call(0x10773520,0x1000e7dc,{8}));
    if(compare(c,c.r(21))!=0) {
        const auto name=p.call(0x10746280,0x1000e7f0,{c.r(29)+0x37,c.r(29)+0x40});
        p.call(0x10748bb0,0x1000e800,{c.r(21),name,0x400});
    }
    c.put32(manager,c.r(21));
    if(compare_logical(c,c.u32(manager))==0) {
        assertion(c,frame,0x1000e824,0x78,0x40,0x50);p.call(0x1076f7d0,0x1000e868,{missing});c.set(3,manager);return manager;
    }
    if(compare_logical(c,file)!=0 && nonzero_byte(c,p.call(0x1001a690,0x1000e884,{manager,file}))) {
        c.set(3,manager);return manager;
    }
    c.set(3,c.toc(-0x7b50));
    for(auto offset:{8u,12u,4u,20u,24u,16u})c.put32(manager+offset,0);
    c.put32(manager+32,c.u32(c.r(3)));
    c.put32(manager+28,p.call(0x10775130,0x1000e8c8,{c.u32(manager+32)*index_size}));
    if(compare_logical(c,c.u32(manager+28))==0) {
        assertion(c,frame,0x1000e8ec,0x58,0x38,0x77);p.call(0x1076f7d0,0x1000e930,{missing});
        c.put32(manager+32,0);c.set(3,manager);return manager;
    }
    c.set({{4,0},{6,0},{5,0},{3,missing}});
    c.put32(manager+40,0);c.put32(manager+44,0);c.put32(manager+36,0);
    std::uint32_t index=0,offset=0;
    while(compare(c,index,c.i32(manager+32))<0) {
        const auto row=c.u32(manager+28)+offset;++index;offset+=index_size;c.set({{6,index},{5,offset},{7,row}});
        for(auto field:{0u,4u,8u,12u})c.put32(row+field,missing);
        c.put8(row+16,0);c.put8(row+17,0);c.put16(row+18,0);
        for(auto field:{20u,24u,44u,64u,84u})c.put32(row+field,0);
        c.put16(row+104,0);
    }
    Kept<24> player_index(c,0);Kept<23> player_offset(c,0);
    while(compare(c,player_index,c.i32(c.r(25)))<0) {
        Kept<27> player(c,c.u32(c.r(26))+std::uint32_t(player_offset));
        if(compare_logical(c,c.u8(player+0x5e))!=0) {
            c.set(3,c.u32(player+12));
            if(compare_logical(c,c.r(3))!=0) {
                const auto club=p.call(0x100c76c0,0x1000e9e4);
                const auto record=p.call(0x1001b830,0x1000e9f4,{manager,club,1});c.set(5,record);
                c.set(3,std::uint32_t(c.i16(record+0x35c)));
                if(compare(c,c.r(3),-1)==0 || compare(c,c.u8(player+0x5e),c.r(3))>0) {
                    c.put16(record+0x35c,c.u8(player+0x5e));p.call(0x1030d6e0,0x1000ea24,{player,record+0x360});
                }
            }
        }
        player_offset=std::uint32_t(player_offset)+0x70;player_index=std::uint32_t(player_index)+1;
    }
    c.set(3,manager);return manager;
}
}
