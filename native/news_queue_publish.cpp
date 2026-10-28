#include "news_detail.hpp"

namespace cm::aot::news {
using namespace detail;
std::uint32_t publish_to_manager(Cpu& c,std::uint32_t address,std::uint32_t source,std::uint32_t manager) {
    Frame frame(c,0x102cbc10,0x100,{.first_gpr=24});
    keep(c,{{24,address},{25,source},{26,manager},{28,c.toc(-0x7bb0)},
            {29,c.toc(slot::success)},{30,c.toc(slot::message)},{31,c.toc(slot::pool)}});
    Queue queue{c,address}; Item item{c,source}; Program program{c};
    if (compare(c,source)==0) { assertion(c,frame,0x102cbc54,0xb8,0x50,0x1f3f); return 0; }
    if (compare_logical(c,manager)==0) { assertion(c,frame,0x102cbcb4,0x98,0x48,0x1f47); return 0; }
    enter(c,0x102cbd04,{source,manager}); c.set(3,priority_for_manager(c,source,manager));
    item.priority()=c.r(3); c.set(4,c.toc(slot::first_manager)); c.put8(source+0xe0,0);
    const auto index=manager_index(c,manager);
    if (compare(c,index)<0) { assertion(c,frame,0x102cbd3c,0x78,0x40,0x1f58); return 0; }
    Kept<27> inbox_address(c,queue.managers()+index*Inbox::size);
    Inbox inbox{c,inbox_address};
    if (compare(c,inbox.first(),-1)==0) {
        inbox.first()=queue.item_count().get(); inbox.next()=queue.item_count().get();
        queue.item_count()=std::uint32_t(queue.item_count())+Inbox::capacity;
        queue.items()=program.call(0x107751b0,0x102cbdc4,{queue.items(),std::uint32_t(queue.item_count())*Item::size});
        if (compare_logical(c,queue.items())==0) { assertion(c,frame,0x102cbde8,0x58,0x38,0x1f6d); return 0; }
        std::uint32_t count=0;
        while (compare(c,count,Inbox::capacity)<0) {
            auto at=queue.item(std::uint32_t(inbox.first())+count); c.put32(at+0xdc,0);
            at=queue.item(std::uint32_t(inbox.first())+count); ++count; c.put8(at+0xe0,1);
            leave(c,{{3,at},{4,1},{5,0},{6,count}});
        }
    }
    if (compare(c,item.id())==0) { queue.last_id()=std::uint32_t(queue.last_id())+1; item.id()=queue.last_id().get(); }
    auto previous=std::uint32_t(inbox.next())-1;
    if (compare(c,previous,inbox.first())<0) previous=std::uint32_t(inbox.first())+Inbox::capacity-1;
    c.set(4,queue.items());
    if (compare(c,item.id(),c.u32(queue.item(previous)+0xdc))==0) return 1;
    const auto destination=queue.item(inbox.next());
    if (compare(c,c.i8(destination+0xe0))==0) inbox.unread()=std::uint32_t(inbox.unread())-1;
    c.set(6,destination); copy_item(c,destination,source,3,4,5);
    Kept<29> published_index(c,inbox.next());
    inbox.next()=std::uint32_t(inbox.next())+1;
    c.set(3,inbox.first()); c.set(4,inbox.next());
    if (compare(c,inbox.next(),std::uint32_t(inbox.first())+Inbox::capacity)==0) inbox.next()=inbox.first().get();
    c.set(3,c.r(28)+0x260000);
    bool action=compare_logical(c,c.u16(c.r(28)+0x2581ac))!=0;
    if (!action) action=compare(c,std::int8_t(program.call(0x10147e70,0x102cbfc4,{c.u32(c.toc(-0x7a54)),manager})))!=0;
    if (action) {
        auto at=queue.item(published_index),club=c.u32(manager+0x24);
        enter(c,0x102cbfe4,{at,manager,club}); perform_action(c,at,manager,club);
        if (compare_logical(c,c.u32(manager+0x20))!=0) {
            at=queue.item(published_index); club=c.u32(manager+0x20);
            enter(c,0x102cc008,{at,manager,club}); perform_action(c,at,manager,club);
        }
    }
    c.set(4,inbox.unread()); inbox.unread()=c.r(4)+1;
    return 1;
}
}
