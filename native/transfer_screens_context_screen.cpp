#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
namespace {
struct OfferContextScreen {
    Cpu& c; Frame f; Program p;
    Kept<17> offer_or_owner;
    Kept<18> reason_or_owner;
    Kept<19> manager_or_owner;
    Kept<20> second_owner;
    Kept<21> first_owner;
    Kept<22> disabled_colour;
    Kept<23> foreground;
    Kept<24> error_buffer_or_rows;
    Kept<25> screens;
    Kept<26> pool;
    Kept<27> person;
    Kept<28> contracts;
    Kept<29> text;
    Kept<30> clubs;
    Kept<31> current_contract;
    explicit OfferContextScreen(Cpu& cpu)
        : c(cpu),f(c,0x1041eb10,0x2c0,{.first_gpr=17}),p{c},
          offer_or_owner(c,c.r(17)),reason_or_owner(c,c.r(18)),manager_or_owner(c,c.r(19)),
          second_owner(c,c.r(20)),first_owner(c,c.r(21)),disabled_colour(c,c.toc(-0x7bc4)),
          foreground(c,c.toc(-0x7bc8)),error_buffer_or_rows(c,c.toc(-0x7ba8)),screens(c,c.toc(slot::screens)),
          pool(c,c.toc(slot::pool)),person(c,c.r(27)),contracts(c,c.toc(slot::contracts)),
          text(c,c.toc(-0x7bb8)),clubs(c,c.toc(slot::clubs)),current_contract(c,c.toc(slot::current_contract)) {
        c.set({{3,c.u16(c.toc(-0x7b14))},{5,c.u16(c.r(2)-0x37c8)},
            {6,c.u16(c.r(2)-0x37cc)},{7,c.u16(foreground)}});
        c.put16(f.at(0x4e),c.r(3)); c.put16(f.at(0x4c),c.r(7)); c.put16(f.at(0x48),c.r(6));
        c.put16(f.at(0x50),c.r(5)); c.put8(f.at(0x52),c.u8(c.r(2)-0x37c6));
        auto link=p.call(0x1073ef70,0x1041eb7c,{screens,0});
        if (compare_logical(c,link)==0) link=p.call(0x1073ef70,0x1041eb90,{screens,2});
        offer_or_owner=c.u32(link); c.set(5,indirect(c,slot::people));
        person=c.r(5)+c.u32(offer_or_owner+12)*0x70;
        reason_or_owner=p.call(0x1073ef70,0x1041ebb4,{screens,1});
        p.call(0x107136d0,0x1041ebcc,{person,0,0});
    }
    void stack_args(std::initializer_list<std::uint32_t> values) {
        unsigned offset=0x38; for(auto value:values) { c.put32(f.at(offset),value); offset+=4; }
    }
    bool heading() {
        const auto manager=c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+4);
        c.set(3,manager);
        if (compare(c,manager)==0) {
            report_assertion(c,{0x1041ec00,pool,pool+0x15,pool+0x22,f.at(0x64),f.at(0x5c),0xd42});
            c.set(3,c.toc(-0x7bac));
            return false;
        }
        manager_or_owner=c.u32(manager+0x24);
        p.call(0x100c7a50,0x1041ec58,{manager_or_owner,1,f.at(0x4e),f.at(0x4c)});
        p.call(0x10744250,0x1041ec6c,{screens,c.u16(f.at(0x4c)),c.u16(f.at(0x4e)),manager_or_owner+0x1ee});
        const bool permanent=compare(c,c.i8(offer_or_owner+0x24))==0;
        p.call(0x100caeb0,permanent?0x1041ec8c:0x1041ecc0,{person,f.at(0x14c),300,1});
        p.call(0x10727890,permanent?0x1041ec9c:0x1041ecd0,{text,pool+(permanent?0x626:0x65d),f.at(0x14c)});
        p.call(0x107442f0,permanent?0x1041eca8:0x1041ecdc,{screens,text});
        const auto height=p.call(0x1076d470,0x1041ece4,{3});
        error_buffer_or_rows=std::uint32_t(std::int32_t(std::int8_t(divide_signed<4>(405,height))));
        return true;
    }
    std::uint32_t owner(std::uint32_t back) {
        const auto contract=p.call(0x10082f20,back,{c.u32(contracts),person});
        compare_logical(c,contract); c.put32(current_contract,contract);
        if (!contract) return 0;
        c.set({{3,c.u32(current_contract)},{4,c.u32(clubs)}});
        return c.r(4)+c.u32(c.r(3)+4)*0x248;
    }
    std::uint32_t gender(std::uint32_t club) const { return std::uint32_t(std::int32_t(c.i8(club+0x23b))); }
    void owner_message(std::uint32_t first,std::uint32_t second,std::uint32_t third,unsigned caption,std::uint32_t back) {
        offer_or_owner=owner(first); reason_or_owner=owner(second); c.set(3,owner(third));
        p.call(0x10727890,back,{text,pool+caption,gender(c.r(3)),gender(reason_or_owner),f.at(0x14c),offer_or_owner+0x221});
    }
    void bidder_message(unsigned caption,std::uint32_t back) {
        const auto club=c.u32(clubs)+c.u32(offer_or_owner+8)*0x248;
        c.set({{9,c.u32(clubs)},{10,c.u32(clubs)}});
        p.call(0x10727890,back,{text,pool+caption,gender(club),gender(club),club+0x221,f.at(0x14c)});
    }
    void message() {
        if (compare(c,reason_or_owner,1)==0) {
            p.call(0x10715880,0x1041ed00,{f.at(0x54)});
            p.call(0x103f89e0,0x1041ed14,{indirect(c,slot::transfers),offer_or_owner,f.at(0x54)});
            p.call(0x10716eb0,0x1041ed28,{f.at(0x54),f.at(0x84),200,255});
            if (compare(c,c.i8(offer_or_owner+0x38)>>4,1)==0) {
                first_owner=owner(0x1041ed4c); offer_or_owner=owner(0x1041ed80); c.set(3,owner(0x1041edb4));
                p.call(0x10727890,0x1041ee08,{text,pool+0x690,gender(c.r(3)),gender(offer_or_owner),100,
                    f.at(0x14c),first_owner+0x221,f.at(0x84)});
            } else if (compare(c,c.i8(offer_or_owner+0x38)>>4,2)==0) {
                first_owner=owner(0x1041ee30); second_owner=owner(0x1041ee64); manager_or_owner=owner(0x1041ee98);
                reason_or_owner=owner(0x1041eecc); offer_or_owner=owner(0x1041ef00); c.set(6,owner(0x1041ef34));
                stack_args({second_owner+0x221,f.at(0x84),first_owner+0x221});
                p.call(0x10727890,0x1041efa8,{text,pool+0x747,gender(c.r(6)),gender(offer_or_owner),100,
                    gender(reason_or_owner),gender(manager_or_owner),f.at(0x14c)});
            } else owner_message(0x1041efb8,0x1041efec,0x1041f020,0x83c,0x1041f06c);
        } else if (compare(c,reason_or_owner,35)==0)
            owner_message(0x1041f084,0x1041f0b8,0x1041f0ec,0x89f,0x1041f138);
        else if (compare(c,reason_or_owner,2)==0) p.call(0x10727890,0x1041f154,{text,pool+0x911,f.at(0x14c)});
        else if (compare(c,reason_or_owner,28)==0) p.call(0x10727890,0x1041f170,{text,pool+0x94f,f.at(0x14c)});
        else if (compare(c,reason_or_owner,29)==0) {
            if (compare(c,c.u8(offer_or_owner+0x24),1)==0) bidder_message(0x990,0x1041f1dc);
            else p.call(0x10727890,0x1041f1f0,{text,pool+0x9ed,f.at(0x14c)});
        } else if (compare(c,reason_or_owner,27)==0) bidder_message(0xa2b,0x1041f250);
    }
    void footer() {
        stack_args({c.u16(foreground),text,0});
        p.call(0x1074a350,0x1041f288,{110,140,780,545,2,0,
            std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(error_buffer_or_rows)))),3});
        p.call(0x102a1070,0x1041f294,{4,0});
        stack_args({0,1,0,missing});
        offer_or_owner=p.call(0x107198c0,0x1041f2e4,
            {c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+0x24),100,555,790,590,2,f.at(0x48),1});
        p.call(0x10727890,0x1041f2f8,{text,pool+0x5b2});
        stack_args({c.u16(foreground),text,2,0});
        p.call(0x107444e0,0x1041f338,{screens,offer_or_owner,0,0,0x30,c.u16(disabled_colour),12,3});
        p.call(0x10727890,0x1041f344,{text,pool+0x5b5});
        stack_args({c.u16(foreground),text,3,0});
        p.call(0x107444e0,0x1041f384,{screens,offer_or_owner,1,0,0x30,c.u16(disabled_colour),12,3});
    }
};
}
void render_offer_context(Cpu& c) {
    OfferContextScreen screen(c);
    if (screen.heading()) { screen.message(); screen.footer(); }
}
}
