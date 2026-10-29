#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
void render_offer_restriction(Cpu& c) {
    Frame f(c, 0x10420b50, 0x1b0, {.first_gpr = 23});
    Kept<23> manager_club(c, c.r(23));
    Kept<24> bidder_or_rows(c, c.r(24));
    Kept<25> person(c, c.r(25));
    Kept<26> reason(c, c.r(26));
    Kept<27> action(c, c.r(27));
    Kept<28> foreground(c, c.toc(-0x7bc8));
    Kept<29> text(c, c.toc(-0x7bb8));
    Kept<30> pool(c, c.toc(slot::pool));
    Kept<31> screens(c, c.toc(slot::screens));
    Program p{c};
    c.put16(f.at(0x50), c.u16(foreground)); c.put16(f.at(0x52), c.u16(c.toc(-0x7b14)));
    manager_club = p.call(0x1073ef70, 0x10420b90, {screens,0});
    action = p.call(0x1073ef70, 0x10420ba4, {screens,1});
    reason = extended(std::int16_t(p.call(0x1073ef70, 0x10420bb8, {screens,2})));
    const auto draft = c.u32(manager_club);
    c.set({{6,c.u32(draft + 8)}, {7,indirect(c,slot::clubs)},
           {8,indirect(c,slot::people)}, {9,c.toc(slot::people)}});
    person = c.r(8) + c.u32(draft + 12) * 0x70;
    manager_club = c.u32(c.u32(screens + std::uint32_t(c.i16(screens + 0x2c58)) * 0x2c0 + 4) + 0x24);
    bidder_or_rows = c.r(7) + c.r(6) * 0x248;
    p.call(0x107136d0,0x10420c10,{person,0,0});
    p.call(0x100c7a50,0x10420c24,{manager_club,1,f.at(0x52),f.at(0x50)});
    p.call(0x10744250,0x10420c38,{screens,c.u16(f.at(0x50)),c.u16(f.at(0x52)),manager_club + 0x1ee});
    p.call(0x100caeb0,0x10420c4c,{person,f.at(0x54),300,5});
    const auto gender = std::uint32_t(std::int32_t(c.i8(bidder_or_rows + 0x23b)));
    p.call(0x10727890,0x10420c70,{text,pool + 0xadf,gender,gender,bidder_or_rows + 0x221,f.at(0x54)});
    p.call(0x107442f0,0x10420c7c,{screens,text});
    const auto height = p.call(0x1076d470,0x10420c84,{3});
    bidder_or_rows = std::uint32_t(std::int32_t(std::int8_t(divide_signed<4>(405,height))));
    p.call(0x100caeb0,0x10420ca4,{person,f.at(0x54),300,1});
    struct Message { unsigned individual, club; std::uint32_t individual_back, club_back; };
    static constexpr Message rejected{0xb2d,0xb95,0x10420ccc,0x10420cf0};
    static constexpr Message negotiated{0xc22,0xc8c,0x10420d1c,0x10420d40};
    static constexpr Message delayed{0xd1b,0xd82,0x10420d6c,0x10420d90};
    const Message* message = nullptr;
    if (compare(c,action,27) == 0) message = &rejected;
    else if (compare(c,action,28) == 0) message = &negotiated;
    else if (compare(c,action,35) == 0) message = &delayed;
    if (message) {
        if (compare(c,std::int16_t(std::uint32_t(reason)),27) == 0)
            p.call(0x10727890,message->individual_back,{text,pool + message->individual,100,f.at(0x54)});
        else {
            const auto manager_gender = std::uint32_t(std::int32_t(c.i8(manager_club + 0x242)));
            p.call(0x10727890,message->club_back,{text,pool + message->club,manager_gender,manager_gender,manager_club + 0x1ee});
        }
    }
    c.put32(f.at(0x38),c.u16(foreground)); c.put32(f.at(0x3c),text); c.put32(f.at(0x40),0);
    p.call(0x1074a350,0x10420dc8,{110,140,780,545,2,0,
        std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(bidder_or_rows)))),3});
    p.call(0x102a1070,0x10420dd4,{4,0});
    p.call(0x10727890,0x10420de0,{text,pool + 0x5ed});
    c.put32(f.at(0x38),3); c.put32(f.at(0x3c),c.u16(foreground)); c.put32(f.at(0x40),text);
    c.put32(f.at(0x44),1); c.put32(f.at(0x48),0);
    p.call(0x10744430,0x10420e2c,{screens,100,555,790,590,0x30,c.u16(c.toc(-0x7bc4)),12});
}
}
