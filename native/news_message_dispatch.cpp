#include "news_message_detail.hpp"
#include <algorithm>
#include <array>

namespace cm::aot::news {
using namespace detail;
namespace {
enum class Owner {free_function,awards,national_teams,competition_word1,competition_word0,service_37,
    uefa_seeding,media,weather,managers,squads,club_records,injuries,discipline,finance,fines,
    regeneration,contracts,disputes,absence,transfers,service_5009,notes,service_5010,scouting};
enum class Order {standard,free_message,award_detail,award_subjects};
struct Delegate {unsigned category,type;std::uint32_t target,back;Owner owner;Order order;};
constexpr auto delegates=std::to_array<Delegate>({
#include "news_message_delegates.inc"
});
static_assert(delegates.size()==144);
std::uint32_t owning_object(const Message& m,Owner owner) {
    auto& c=m.c;
    switch(owner) {
    case Owner::free_function:return 0;
    case Owner::awards:return c.u32(c.toc(-0x7bb4));
    case Owner::national_teams:return c.u32(c.r(25));
    case Owner::competition_word1:return m.competition(m.value(1));
    case Owner::competition_word0:return m.competition(m.value(0));
    case Owner::service_37:return c.u32(c.toc(-0x7b34));
    case Owner::uefa_seeding:return c.u32(c.toc(-0x7a48));
    case Owner::media:return c.u32(c.r(14));
    case Owner::weather:return c.u32(c.toc(-0x7aac));
    case Owner::managers:return c.u32(c.r(18));
    case Owner::squads:return c.u32(c.toc(-0x7a98));
    case Owner::club_records:return c.u32(c.toc(-0x7afc));
    case Owner::injuries:return c.u32(c.r(26));
    case Owner::discipline:return c.u32(c.toc(-0x7aa4));
    case Owner::finance:return c.u32(c.r(21));
    case Owner::fines:return c.u32(c.toc(-0x7ba4));
    case Owner::regeneration:return c.u32(c.toc(-0x7b28));
    case Owner::contracts:return c.u32(c.r(22));
    case Owner::disputes:return c.u32(c.toc(-0x7484));
    case Owner::absence:return c.u32(c.toc(-0x7b24));
    case Owner::transfers:return c.u32(c.r(23));
    case Owner::service_5009:return c.toc(-0x633c);
    case Owner::notes:return c.u32(c.toc(-0x7440));
    case Owner::service_5010:return c.toc(-0x6340);
    case Owner::scouting:return c.u32(c.toc(-0x7b1c));
    }
    throw std::logic_error("Unknown NEWS delegate owner");
}
void render_delegate(const Message& m,const Delegate& delegate) {
    Program program{m.c};
    switch(delegate.order) {
    case Order::standard:program.call(delegate.target,delegate.back,{owning_object(m,delegate.owner),m.item,m.detail,m.output});break;
    case Order::free_message:program.call(delegate.target,delegate.back,{m.item,m.detail,m.output});break;
    case Order::award_detail:program.call(delegate.target,delegate.back,{owning_object(m,delegate.owner),m.item,m.output,m.detail});break;
    case Order::award_subjects:program.call(delegate.target,delegate.back,{owning_object(m,delegate.owner),m.value(0),m.value(1),m.item,m.output,m.detail});break;
    }
}
struct Group {unsigned category;std::uint32_t back;std::int32_t last_type;unsigned unknown_group;};
constexpr std::array<Group,7> groups{{
    {2,0x102be180,40,3},{7,0x102c3e40,1001,3},{3,0x102c3fa8,2003,3},
    {6,0x102c5618,6023,7},{4,0x102c5b04,3040,7},{5,0x102c61a4,4035,7},{1,0x102c6720,5003,7}}};
constexpr auto local_types=std::to_array<std::pair<unsigned,unsigned>>({
    {1,2},{3,2},{4,2},{5,2},{6,2},{7,2},{8,2},{9,2},{10,2},{11,2},{12,2},{13,2},
    {16,2},{17,2},{18,2},{19,2},{20,2},{38,2},{39,2},{21,2},{22,2},{23,2},{26,2},{40,2},
    {1001,7},{2000,3},{5000,1}});
}

std::uint32_t render_message(Cpu& c,std::uint32_t item,std::uint32_t manager,
                             std::uint32_t output,std::uint32_t detail,std::uint32_t suppress_reply) {
    Frame frame(c,0x102be040,0x25c0,{.first_gpr=13});
    keep(c,{{13,c.toc(slot::message)},{14,c.toc(-0x7b94)},{15,item},{16,output},{17,detail},
            {18,c.toc(-0x79f0)},{19,c.toc(-0x7548)},{20,c.toc(-0x7310)},{21,c.toc(-0x7ab8)},
            {22,c.toc(-0x7b7c)},{23,c.toc(-0x7b40)},{24,c.toc(-0x7b5c)},{25,c.toc(-0x7a64)},
            {26,c.toc(-0x7ba0)},{27,c.toc(-0x7b98)},{28,c.toc(-0x7be8)},{29,c.toc(-0x7bb8)},
            {30,c.toc(-0x7bbc)},{31,c.toc(slot::pool)}});
    c.put32(frame.at(0x25dc),manager);c.put8(frame.at(0x25eb),suppress_reply);
    if(compare_logical(c,c.u32(frame.at(0x25dc)))==0) {
        assertion(c,frame,0x102be0c4,0x278,0x170,0x9f);c.set(4,c.toc(slot::success));return 0;
    }
    if(compare_logical(c,output)==0) {
        assertion(c,frame,0x102be128,0x258,0x168,0xa7);c.set(4,c.toc(slot::success));return 0;
    }
    Program{c}.call(0x1074c570,0x102be178,{output});
    Message message{c,frame,item,output,detail};unsigned next_group=0;
    while(next_group<groups.size()) {
        const auto& group=groups[next_group];
        enter(c,group.back,{item});c.set(3,category(c,item));
        if(compare(c,std::int8_t(c.r(3)),group.category)!=0){++next_group;continue;}
        const auto type=c.i32(item);
        const auto delegate=std::ranges::find_if(delegates,[&](const auto& row){return row.category==group.category&&row.type==std::uint32_t(type);});
        if(delegate!=delegates.end()){compare(c,type,type);render_delegate(message,*delegate);return 1;}
        if(std::ranges::find(local_types,std::pair{std::uint32_t(type),group.category})!=local_types.end()) {
            compare(c,type,type);return message.local(type);
        }
        compare(c,type,group.last_type);next_group=group.unknown_group;
    }
    assertion(c,frame,0x102c6ab8,0x1a0,0xc0,0x1226);c.set(4,c.toc(slot::success));return 0;
}
}
