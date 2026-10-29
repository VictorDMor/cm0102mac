#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;

void update_proposal_total(Cpu& c, std::uint32_t draft_offer, std::uint32_t offer_link) {
    Frame f(c, 0x1041e770, 0x70, {.first_gpr = 23});
    Kept<28> link(c, offer_link);
    Kept<31> screens(c, c.toc(slot::screens));
    Kept<27> draft(c, draft_offer);
    Kept<29> text(c, c.toc(-0x7bb8));
    Kept<30> pool(c, c.toc(slot::pool));
    Kept<26> people(c, c.toc(slot::people));
    Program p{c};
    Kept<25> control(c, extended(std::int16_t(p.call(0x1073ef70, 0x1041e7a4, {screens, 20}))));
    c.put32(f.at(0x44), c.u32(draft + 0x20));
    p.call(0x10009f20, 0x1041e7bc, {f.at(0x44), 4});
    Kept<23> total_low(c, c.r(4));
    Kept<24> total_high(c, c.r(3));
    const auto add_amount = [&] {
        const auto prior = (std::uint64_t(total_high) << 32) | std::uint32_t(total_low);
        const auto amount = (std::uint64_t(c.r(3)) << 32) | c.r(4);
        const auto sum = prior + amount;
        c.s.wr<1>(reg::xer_ca, sum < prior);
        c.set(4, std::uint32_t(sum));
        total_low = c.r(4); total_high = std::uint32_t(sum >> 32);
    };
    if (compare(c, c.i32(draft + 0x40)) >= 0) {
        c.put32(f.at(0x40), c.u32(draft + 0x40));
        p.call(0x10009f20, 0x1041e7e4, {f.at(0x40), 4}); add_amount();
    }
    if (compare(c, c.i32(draft + 0x48)) >= 0) {
        c.put32(f.at(0x3c), c.u32(draft + 0x48));
        p.call(0x10009f20, 0x1041e814, {f.at(0x3c), 4}); add_amount();
    }
    if (compare_logical(c, c.u32(draft + 4)) != 0) {
        c.set(3, c.u32(draft + 4));
        c.put32(c.r(3) + 12, c.u32(c.r(3) + 4));
        while (compare_logical(c, p.call(0x10738e00, 0x1041e87c, {c.u32(draft + 4)})) != 0) {
            c.set(5, c.u32(people) + c.u32(c.r(3)) * 0x70);
            c.put32(f.at(0x38), c.u32(c.r(5) + 8));
            p.call(0x10009f20, 0x1041e864, {f.at(0x38), 4}); add_amount();
        }
    }
    p.call(0x1000b410, 0x1041e89c, {text, 2000, total_high, total_low, 0x22});
    p.call(0x10741da0, 0x1041e8ac, {screens, control, text});
    if (compare_logical(c, link) == 0) return;

    const bool unchanged = byte_result(c, p.call(0x10416e10, 0x1041e8c0, {draft, c.u32(link)}));
    if (!unchanged) {
        control = extended(std::int16_t(p.call(0x1073ef70, 0x1041e8d4, {screens, 21})));
        c.set(4, c.u32(link));
        const bool negotiation_disabled = compare(c, c.u8(c.r(4) + 0x3c) >> 4) != 0;
        const auto offset = negotiation_disabled ? 0x40u : 0u;
        p.call(0x10727890, 0x1041e8f4 + offset, {text, pool + (negotiation_disabled ? 0x619u : 0x60fu)});
        p.call(0x10741da0, 0x1041e904 + offset, {screens, control, text});
        p.call(0x10742100, 0x1041e914 + offset, {screens, control, negotiation_disabled ? 0u : 28u});
        p.call(0x10741450, 0x1041e924 + offset, {screens, control, negotiation_disabled ? 0x2cu : 0xcu});
        control = extended(std::int16_t(p.call(0x1073ef70, 0x1041e970, {screens, 22})));
        p.call(0x10740620, 0x1041e988, {screens, control, 2});
        p.call(0x10741450, 0x1041e998, {screens, control, 0xc});
        p.call(0x10742100, 0x1041e9a8, {screens, control, 26});
    } else {
        control = extended(std::int16_t(p.call(0x1073ef70, 0x1041e9b8, {screens, 21})));
        p.call(0x10727890, 0x1041e9cc, {text, pool + 0x619});
        p.call(0x10741da0, 0x1041e9dc, {screens, control, text});
        p.call(0x10742100, 0x1041e9ec, {screens, control, 29});
        p.call(0x10741450, 0x1041e9fc, {screens, control, 0xc});
        control = extended(std::int16_t(p.call(0x1073ef70, 0x1041ea08, {screens, 22})));
        p.call(0x10740620, 0x1041ea20, {screens, control, 1});
        p.call(0x10741450, 0x1041ea30, {screens, control, 0x2c});
        p.call(0x10742100, 0x1041ea40, {screens, control, 0});
    }
}
}
