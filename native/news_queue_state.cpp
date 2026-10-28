#include "news_detail.hpp"

namespace cm::aot::news {
using namespace detail;

std::uint32_t recent_items(Cpu& c,std::uint32_t address,std::uint32_t manager,
                            std::uint32_t count_address,std::uint32_t filter) {
    Frame frame(c,0x102ccb20,0xe0,{.first_gpr=23});
    keep(c,{{23,address},{24,count_address},{25,filter},{29,c.toc(slot::success)},
            {30,c.toc(slot::message)},{31,c.toc(slot::pool)}});
    Queue queue{c,address};
    if (compare_logical(c,manager)==0) { assertion(c,frame,0x102ccb64,0x90,0x48,0x21fa); return 0; }
    const auto index=manager_index(c,manager);
    if (compare(c,index)<0) { assertion(c,frame,0x102ccbd4,0x70,0x40,0x2205); return 0; }
    Kept<27> inbox_address(c,queue.managers()+index*Inbox::size); Inbox inbox{c,inbox_address};
    if (compare(c,inbox.first(),-1)==0) return 0;
    Kept<26> result(c,0); Kept<28> cursor(c,std::uint32_t(inbox.next())-1);
    do {
        if (compare(c,cursor,inbox.first())<0) cursor=std::uint32_t(inbox.first())+Inbox::capacity-1;
        if (compare(c,c.u32(queue.item(cursor)+0xdc))==0) break;
        bool accepted=compare(c,std::int8_t(filter))==0;
        if (!accepted) {
            enter(c,0x102ccc88,{queue.item(cursor)}); c.set(3,category(c,queue.item(cursor)));
            accepted=compare(c,std::int8_t(filter),std::int8_t(c.r(3)))==0;
        }
        if (accepted) {
            if (compare_logical(c,result)==0) {
                result=Program{c}.call(0x10775130,0x102ccca8,{Inbox::capacity*Item::size});
                if (compare(c,result)==0) { assertion(c,frame,0x102cccc4,0x50,0x38,0x222a); return 0; }
                c.put32(count_address,0);
            }
            const auto count=c.u32(count_address), source_base=queue.items().get();
            c.put32(count_address,count+1);
            const auto source=source_base+std::uint32_t(cursor)*Item::size;
            const auto destination=std::uint32_t(result)+count*Item::size;
            leave(c,{{6,destination},{7,source}}); copy_item(c,destination,source,3,4,5);
        }
        cursor=std::uint32_t(cursor)-1;
    } while (compare(c,cursor,std::uint32_t(inbox.next())-1)!=0);
    return result;
}

std::uint32_t acknowledge(Cpu& c,std::uint32_t address,std::uint32_t manager,
                           std::uint32_t id,std::uint32_t propagation) {
    Frame frame(c,0x102cd5f0,0xf0,{.first_gpr=20});
    keep(c,{{20,c.toc(slot::pool)},{21,address},{22,manager},{23,id},{24,propagation},{25,0},
            {26,c.toc(slot::first_manager)},{27,c.toc(-0x7be0)},{28,c.toc(-0x7bb0)},
            {29,c.toc(-0x7bb8)},{30,c.toc(slot::success)},{31,c.toc(slot::message)}});
    Queue queue{c,address}; Program program{c};
    if (compare(c,manager)==0) { assertion(c,frame,0x102cd648,0x94,0x4c,0x2358); return 0; }
    if (compare(c,id)==0) { assertion(c,frame,0x102cd6a8,0x74,0x44,0x2360); return 0; }
    const auto index=manager_index(c,manager);
    if (compare(c,index)<0) { assertion(c,frame,0x102cd714,0x54,0x3c,0x236b); return 0; }
    c.set(31,queue.managers()+index*Inbox::size); Inbox inbox{c,c.r(31)};
    if (compare(c,inbox.first(),-1)==0) return 0;
    Kept<30> offset(c,0); Kept<25> visited(c,0);
    while (compare(c,offset,Inbox::capacity)<0) {
        visited=queue.item(std::uint32_t(inbox.first())+std::uint32_t(offset));
        if (compare(c,c.u32(std::uint32_t(visited)+0xdc),id)==0 && compare(c,c.i8(std::uint32_t(visited)+0xe0))==0) {
            enter(c,0x102cd7c4,{visited,manager,c.r(29),2000,frame.at(0x38)});
            const auto action=action_label(c,visited,manager,c.r(29),2000,frame.at(0x38)); c.set(3,action);
            if (compare(c,std::int8_t(action),2)!=0) {
                c.put8(std::uint32_t(visited)+0xe0,1); c.set(3,inbox.unread());
                inbox.unread()=c.r(3)-1; break;
            }
        }
        offset=std::uint32_t(offset)+1;
    }
    // The reference tests the last visited address, not whether ID matched.
    if (compare_logical(c,visited)==0) return 0;
    if (compare(c,std::int8_t(propagation))==0) return 1;
    const auto game=c.r(28);
    c.set(24,program.call(0x1073ffe0,0x102cd81c,{game,c.u32(game+std::uint32_t(c.i16(game+0x2c58))*0x2c0+4)}));
    offset=0;
    while (compare(c,offset,16)<0) {
        const auto people=c.u32(c.r(27)); c.set(4,people);
        c.set(20,people+(c.u32(c.r(26))-std::uint32_t(offset)-1)*0x70);
        const auto candidate=c.r(20);
        if (compare(c,c.u8(candidate+0x6b),1)==0 && compare_logical(c,candidate,manager)!=0) {
            const auto group=program.call(0x1073ffe0,0x102cd860,{game,candidate});
            if (compare(c,std::int16_t(c.r(24)),std::int16_t(group))==0) {
                enter(c,0x102cd884,{address,candidate,id,0}); c.set(3,acknowledge(c,address,candidate,id,0));
            }
        }
        offset=std::uint32_t(offset)+1;
    }
    return 1;
}

std::uint32_t requires_attention(Cpu& c,std::uint32_t address,std::uint32_t manager,std::uint32_t inspect) {
    Frame frame(c,0x102cd8b0,0x80,{.first_gpr=27});
    keep(c,{{27,address},{28,manager},{29,c.toc(slot::pool)},{30,c.toc(slot::message)},{31,c.toc(-0x7bb8)}});
    Queue queue{c,address};
    if (compare(c,std::int8_t(inspect))==0) {
        enter(c,0x102cd8ec,{address,manager,0}); const auto unread=unread_count(c,address,manager,0);
        return std::int32_t(unread)>0;
    }
    const auto index=manager_index(c,manager);
    if (compare(c,index)<0) { assertion(c,frame,0x102cd92c,0x44,0x3c,0x241e); c.set(4,c.toc(slot::success)); return 0; }
    c.set(29,queue.managers()+index*Inbox::size); Inbox inbox{c,c.r(29)};
    if (compare(c,inbox.first(),-1)==0) return 0;
    Kept<30> cursor(c,std::uint32_t(inbox.next())-1);
    do {
        if (compare(c,cursor,inbox.first())<0) cursor=std::uint32_t(inbox.first())+Inbox::capacity-1;
        if (compare(c,c.u32(queue.item(cursor)+0xdc))==0) break;
        const auto item=queue.item(cursor);
        enter(c,0x102cd9e8,{item,manager,c.r(31),2000,frame.at(0x38)});
        const auto action=action_label(c,item,manager,c.r(31),2000,frame.at(0x38)); c.set(3,action);
        if (compare(c,std::int8_t(action),2)==0) return 1;
        if (compare(c,c.u8(queue.item(cursor)+4),1)==0 && compare(c,c.i8(queue.item(cursor)+0xe0))==0) return 1;
        cursor=std::uint32_t(cursor)-1;
    } while (compare(c,cursor,std::uint32_t(inbox.next())-1)!=0);
    return 0;
}

std::uint32_t perform_matching_actions(Cpu& c,std::uint32_t address,std::uint32_t manager,std::uint32_t club) {
    Frame frame(c,0x102cda60,0xa0,{.first_gpr=26});
    keep(c,{{26,c.toc(slot::success)},{27,c.toc(slot::message)},{28,c.toc(slot::pool)},
            {29,address},{30,manager},{31,club}});
    Queue queue{c,address};
    if (compare(c,manager)==0) { assertion(c,frame,0x102cdaa0,0x68,0x40,0x2460); return 0; }
    const auto index=manager_index(c,manager);
    if (compare(c,index)<0) { assertion(c,frame,0x102cdb10,0x48,0x38,0x246b); return 0; }
    c.set(27,queue.managers()+index*Inbox::size); Inbox inbox{c,c.r(27)};
    if (compare(c,inbox.first(),-1)==0) return 1;
    Kept<26> cursor(c,std::uint32_t(inbox.next())-1);
    do {
        if (compare(c,cursor,inbox.first())<0) cursor=std::uint32_t(inbox.first())+Inbox::capacity-1;
        if (compare(c,c.u32(queue.item(cursor)+0xdc))==0) break;
        const auto item=queue.item(cursor);
        enter(c,0x102cdbc0,{item,manager,club}); perform_action(c,item,manager,club);
        cursor=std::uint32_t(cursor)-1;
    } while (compare(c,cursor,std::uint32_t(inbox.next())-1)!=0);
    return 1;
}

void clear_inbox(Cpu& c,std::uint32_t address,std::uint32_t manager) {
    Frame frame(c,0x102cdc80,0xa0,{.first_gpr=27});
    keep(c,{{27,manager},{28,c.toc(slot::success)},{29,c.toc(slot::message)},
            {30,c.toc(slot::pool)},{31,address}});
    Queue queue{c,address};
    if (compare(c,manager)==0) { assertion(c,frame,0x102cdcbc,0x68,0x40,0x24c1); return; }
    const auto index=manager_index(c,manager);
    if (compare(c,index)<0) { assertion(c,frame,0x102cdd28,0x48,0x38,0x24cc); return; }
    c.set(28,queue.managers()+index*Inbox::size); Inbox inbox{c,c.r(28)};
    if (compare_logical(c,c.u32(manager+0x24))!=0) {
        enter(c,0x102cdd90,{address,manager,c.u32(manager+0x24)}); c.set(3,perform_matching_actions(c,address,manager,c.u32(manager+0x24)));
    }
    if (compare_logical(c,c.u32(manager+0x20))!=0) {
        enter(c,0x102cddac,{address,manager,c.u32(manager+0x20)}); c.set(3,perform_matching_actions(c,address,manager,c.u32(manager+0x20)));
    }
    c.set(3,inbox.first()); inbox.next()=inbox.first().get(); inbox.unread()=0;
    if (compare(c,inbox.first(),-1)==0) return;
    std::uint32_t offset=0; c.set(6,offset);
    while (compare(c,offset,Inbox::capacity)<0) {
        c.put32(queue.item(std::uint32_t(inbox.first())+offset)+0xdc,0);
        const auto item=queue.item(std::uint32_t(inbox.first())+offset); ++offset; c.put8(item+0xe0,1);
        leave(c,{{3,item},{4,1},{5,0},{6,offset}});
    }
}
}
