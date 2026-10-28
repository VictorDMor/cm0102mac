#include "news_detail.hpp"

namespace cm::aot::news {
using namespace detail;
namespace {
enum class Audience { competition, nation, club_nation, continent, everyone };
struct Distribution {
    std::uint32_t entry, item_assert, target_assert, publish_back;
    unsigned item_line, target_line, kind;
    Audience audience;
};
constexpr Distribution competition{0x102cc280,0x102cc2c8,0x102cc328,0x102cc3c8,0x205b,0x2063,1,Audience::competition};
constexpr Distribution nation{0x102cc400,0x102cc448,0x102cc4a8,0x102cc570,0x209d,0x20a5,2,Audience::nation};
constexpr Distribution club_nation{0x102cc5b0,0x102cc5f8,0x102cc658,0x102cc6f8,0x20e2,0x20ea,2,Audience::club_nation};
constexpr Distribution continent{0x102cc730,0x102cc778,0x102cc7d8,0x102cc8a4,0x2125,0x212d,3,Audience::continent};
constexpr Distribution everyone{0x102cc8e0,0x102cc920,0,0x102cc9ac,0x219b,0,4,Audience::everyone};

bool belongs(Cpu& c, Audience audience, std::uint32_t manager, std::uint32_t target) {
    if (audience==Audience::everyone) return true;
    const auto primary=c.u32(manager+0x24); c.set(3,primary);
    if (audience==Audience::nation) c.set(4,c.u32(manager+0x20));
    if (compare_logical(c,primary)!=0) {
        if (audience==Audience::competition) return compare_logical(c,c.u32(primary+0x30),target)==0;
        const auto home=c.u32(primary+0x2c);
        if (audience!=Audience::continent) {
            if (compare_logical(c,home,target)==0) return true;
        } else if (compare_logical(c,home)!=0) {
            c.set(3,home);
            if (compare_logical(c,c.u32(home+0x74),target)==0) return true;
        }
    }
    if (audience==Audience::competition || audience==Audience::club_nation) return false;
    if (audience==Audience::nation) {
        const auto secondary=c.u32(manager+0x20);
        if (compare_logical(c,secondary)==0) return false;
        c.set(3,secondary);
        return compare_logical(c,c.u32(secondary+0x2c),target)==0;
    }
    const auto home=c.u32(manager+0xc);
    if (compare_logical(c,home)==0) return false;
    c.set(3,home);
    return compare_logical(c,c.u32(home+0x74),target)==0;
}

std::uint32_t distribute_group(Cpu& c, const Distribution& group,
                                std::uint32_t queue, std::uint32_t item, std::uint32_t target) {
    const bool all=group.audience==Audience::everyone, continental=group.audience==Audience::continent;
    Frame frame(c,group.entry,all?0x80:0xb0,{.first_gpr=all?26u:24u});
    const auto counter_register=all?28u:continental?29u:24u;
    const auto first_slot=c.toc(slot::first_manager), managers_slot=c.toc(-0x7be0);
    if (all) keep(c,{{26,first_slot},{27,managers_slot},{28,c.toc(slot::message)},
                     {29,c.toc(slot::pool)},{30,queue},{31,item}});
    else if (continental) keep(c,{{24,c.toc(slot::success)},{25,c.toc(slot::message)},
        {26,queue},{27,item},{28,target},{29,c.toc(slot::pool)},{30,first_slot},{31,managers_slot}});
    else keep(c,{{24,c.toc(slot::success)},{25,c.toc(slot::message)},
        {26,c.toc(slot::pool)},{27,queue},{28,item},{29,target},{30,first_slot},{31,managers_slot}});
    if (compare(c,item)==0) {
        assertion(c,frame,group.item_assert,all?0x40:0x68,all?0x38:0x40,group.item_line);
        if (all) c.set(4,c.toc(slot::success));
        return 0;
    }
    if (!all && compare_logical(c,target)==0) {
        assertion(c,frame,group.target_assert,0x48,0x38,group.target_line); return 0;
    }
    c.put8(item+0xe1,group.kind);
    std::uint32_t index=0; c.set(counter_register,index);
    while (compare(c,index,16)<0) {
        const auto manager_base=c.u32(managers_slot);
        const auto manager=manager_base+(c.u32(first_slot)+index-16)*0x70;
        leave(c,{{4,manager_base},{5,manager}});
        if (compare(c,c.u8(manager+0x6b),1)==0 && belongs(c,group.audience,manager,target)) {
            enter(c,group.publish_back,{queue,item,manager,0});
            c.set(3,publish_to_manager(c,queue,item,manager));
            if (compare(c,std::int8_t(c.r(3)))==0) return 0;
        }
        c.set(counter_register,++index);
    }
    // Both nation variants deliberately report zero after exhausting the loop.
    return group.audience==Audience::nation || group.audience==Audience::club_nation ? 0 : 1;
}
}

std::uint32_t distribute_to_competition(Cpu& c,std::uint32_t q,std::uint32_t i,std::uint32_t t) { return distribute_group(c,competition,q,i,t); }
std::uint32_t distribute_to_nation(Cpu& c,std::uint32_t q,std::uint32_t i,std::uint32_t t) { return distribute_group(c,nation,q,i,t); }
std::uint32_t distribute_to_club_nation(Cpu& c,std::uint32_t q,std::uint32_t i,std::uint32_t t) { return distribute_group(c,club_nation,q,i,t); }
std::uint32_t distribute_to_continent(Cpu& c,std::uint32_t q,std::uint32_t i,std::uint32_t t) { return distribute_group(c,continent,q,i,t); }
std::uint32_t distribute_to_all(Cpu& c,std::uint32_t q,std::uint32_t i) { return distribute_group(c,everyone,q,i,0); }

std::uint32_t distribute_to_club(Cpu& c,std::uint32_t q,std::uint32_t i,std::uint32_t club) {
    Frame frame(c,0x102cc140,0xa0,{.first_gpr=29});
    keep(c,{{29,c.toc(slot::success)},{30,c.toc(slot::message)},{31,c.toc(slot::pool)}});
    if (compare_logical(c,i)==0) { assertion(c,frame,0x102cc180,0x68,0x40,0x2026); return 0; }
    if (compare_logical(c,club)==0) { assertion(c,frame,0x102cc1e0,0x48,0x38,0x202e); return 0; }
    const auto manager=c.u32(club+0x90); c.set(5,manager); c.put8(i+0xe1,0);
    if (compare_logical(c,manager)==0) return 0;
    c.set(6,c.u32(c.toc(slot::first_manager))); c.set(7,c.u32(manager));
    const auto first=c.r(6)-16;
    if (compare(c,c.r(7),first)<0) return 0;
    enter(c,0x102cc258,{q,i,manager,0}); return publish_to_manager(c,q,i,manager);
}

std::uint32_t distribute_to_entity(Cpu& c,std::uint32_t q,std::uint32_t i,std::uint32_t entity) {
    Frame frame(c,0x102cc9e0,0xa0,{.first_gpr=29});
    keep(c,{{29,c.toc(slot::success)},{30,c.toc(slot::message)},{31,c.toc(slot::pool)}});
    if (compare_logical(c,i)==0) { assertion(c,frame,0x102cca20,0x68,0x40,0x21c5); return 0; }
    if (compare_logical(c,entity)==0) { assertion(c,frame,0x102cca80,0x48,0x38,0x21cd); return 0; }
    const auto home=c.u32(entity+8);
    if (compare_logical(c,home)!=0) { enter(c,0x102ccad8,{q,i,home}); return distribute_to_club_nation(c,q,i,home); }
    const auto region=c.u32(entity+4);
    if (compare_logical(c,region)!=0) { enter(c,0x102ccaf0,{q,i,region}); return distribute_to_continent(c,q,i,region); }
    enter(c,0x102ccaf8); return distribute_to_all(c,q,i);
}

std::uint32_t distribute(Cpu& c,std::uint32_t q,std::uint32_t i,std::uint32_t club,
                          std::uint32_t entity,std::uint32_t home,std::uint32_t region,std::uint32_t kind_word) {
    Frame frame(c,0x102cc030,0x70,{.first_gpr=30});
    keep(c,{{30,c.toc(slot::message)},{31,c.toc(slot::pool)}});
    const auto kind=std::int8_t(kind_word);
    if (compare(c,kind,3)<0) {
        const auto middle=compare(c,kind,1);
        if (middle==0) { enter(c,0x102cc094,{q,i,entity}); return distribute_to_competition(c,q,i,entity); }
        if (middle>0) { enter(c,0x102cc0a0,{q,i,home}); return distribute_to_nation(c,q,i,home); }
        if (compare(c,kind)>=0) { enter(c,0x102cc088); return distribute_to_club(c,q,i,club); }
    } else if (kind==3) { enter(c,0x102cc0ac,{q,i,region}); return distribute_to_continent(c,q,i,region); }
    else {
        const auto middle=compare(c,kind,5);
        if (middle==0) { enter(c,0x102cc0b8,{q,i,entity}); return distribute_to_entity(c,q,i,entity); }
        if (middle<0) { enter(c,0x102cc0c0); return distribute_to_all(c,q,i); }
    }
    assertion(c,frame,0x102cc0d8,0x40,0x38,0x1fd3); c.set(4,c.toc(slot::success)); return 0;
}
}
