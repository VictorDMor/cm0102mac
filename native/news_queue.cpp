#include "news_detail.hpp"
#include <array>

namespace cm::aot::news {
using namespace detail;

std::uint32_t construct_queue(Cpu& c, std::uint32_t address, std::uint32_t context) {
    Frame frame(c, 0x102cb930, 0x380, {.first_gpr = 24});
    Kept<25> queue_address(c, address); Kept<26> file_context(c, context);
    keep(c, {{28,c.toc(-0x5114)}, {29,c.toc(-0x5110)},
             {30,c.toc(slot::message)}, {31,c.toc(slot::pool)}});
    Kept<27> located(c, 0); Kept<24> name_cursor(c, frame.at(0x41));
    Program program{c}; Queue queue{c,address};
    do {
        // These two original Pascal-name fragments are copied before being
        // interleaved. The second fragment determines the stopping point.
        const std::array<std::uint32_t,4> second{
            c.u32(c.r(29)),c.u32(c.r(29)+4),c.u32(c.r(29)+8),c.u32(c.r(29)+12)};
        const auto tail = c.u8(c.r(29)+16);
        const std::array<std::uint32_t,4> first{
            c.u32(c.r(28)),c.u32(c.r(28)+4),c.u32(c.r(28)+8),c.u32(c.r(28)+12)};
        for (unsigned n=0; n<4; ++n) c.put32(frame.at(0x70+n*4),second[n]);
        c.put8(frame.at(0x80),tail);
        for (unsigned n=0; n<4; ++n) c.put32(frame.at(0x40+n*4),first[n]);
        auto second_cursor=frame.at(0x71), destination=frame.at(0xcd);
        name_cursor=frame.at(0x41); located=0;
        while (compare(c,c.i8(second_cursor))!=0) {
            const auto b=c.u8(second_cursor++), a=c.u8(name_cursor);
            name_cursor=std::uint32_t(name_cursor)+1;
            c.put8(destination,b); c.put8(destination+1,a); destination+=2;
            c.set(3,b);
        }
        const auto length=c.u8(frame.at(0x70));
        c.put8(frame.at(0xcc),length+c.u8(frame.at(0x40)));
        leave(c,{{7,length},{8,second[2]},{9,second[1]},{10,second[0]}});
        const auto status=program.call(0x1077fee0,0x102cba00,{0,0,frame.at(0xcc),frame.at(0x84)});
        c.set(2,c.u32(frame.at(0x14)));
        if (compare(c,std::int16_t(status))==0) located=1;
        if (compare(c,located)==0) {
            program.call(0x10727890,0x102cba24,{frame.at(0x294),c.r(31)+0x3fac});
            program.call(0x10727890,0x102cba30,{frame.at(0x1cc),c.r(31)+0x3fb6});
            program.call(0x10769b60,0x102cba40,{frame.at(0x294),frame.at(0x1cc),1});
        }
    } while (compare(c,located)==0);
    if (compare_logical(c,file_context)!=0) {
        enter(c,0x102cba5c,{queue_address,file_context});
        c.set(3,load_queue(c,queue_address,file_context));
        if (compare(c,std::int8_t(c.r(3)))!=0) return queue_address;
    }
    queue.item_count()=0; queue.items()=0; queue.manager_count()=16;
    queue.managers()=program.call(0x10775130,0x102cba90,{std::uint32_t(queue.manager_count())*Inbox::size});
    if (compare_logical(c,queue.managers())==0) {
        assertion(c,frame,0x102cbab4,0x50,0x38,0x1ef9);
        c.set(4,c.toc(slot::success));
        program.call(0x1076f7d0,0x102cbafc,{0xffffffff});
        queue.manager_count()=0;
        return queue_address;
    }
    std::uint32_t index=0;
    while (compare(c,index,queue.manager_count())<0) {
        Inbox inbox{c,queue.managers()+index*Inbox::size};
        ++index; inbox.first()=-1; inbox.next()=-1; inbox.unread()=0;
        leave(c,{{4,0xffffffff},{5,inbox.address},{6,index}});
    }
    queue.last_id()=0;
    return queue_address;
}
}
