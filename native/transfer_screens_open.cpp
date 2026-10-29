#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;

void open_player_offer(Cpu& c, std::uint32_t player, std::uint32_t offer_link) {
    Frame f(c, 0x10417590, 0x50, {.first_gpr = 27});
    Kept<30> link(c, offer_link);
    compare(c, link);
    Kept<27> contract_or_club(c, c.toc(slot::current_contract));
    Kept<31> screens(c, c.toc(slot::screens));
    Kept<29> person(c, player);
    Kept<28> revision_or_manager(c, missing);
    Program p{c};
    if (offer_link && compare_logical(c, c.u32(link)) != 0) {
        c.set(3, c.u32(link)); revision_or_manager = Offer{c, c.r(3)}.revision();
    }
    if (!byte_result(c, p.call(0x1073e4b0, 0x104175e8,
        {screens, c.toc(-0x4abc), c.toc(-0x4ac0), 1, c.toc(-0x4ac4), 0}))) return;
    p.history(0x10417604, screens, 0, c.u32(person));
    p.history(0x10417618, screens, 1, link);
    p.history(0x1041762c, screens, 2, 0);
    p.history(0x10417640, screens, 3, revision_or_manager);
    p.history(0x10417654, screens, 4, 0);
    p.history(0x10417668, screens, 5, 0);
    p.history(0x1041767c, screens, 6, 1);
    revision_or_manager = 0;
    const auto contract = p.call(0x10082f20, 0x10417690, {indirect(c, slot::contracts), person});
    compare_logical(c, contract); c.put32(contract_or_club, contract);
    if (contract) {
        c.set(3, c.u32(contract_or_club));
        // The original interprets the signed six-bit contract type.
        const auto type = std::int8_t(c.u8(c.r(3) + 0x38)) >> 2;
        if (compare(c, type, 5) != 0) revision_or_manager = 1;
    }
    if (compare(c, revision_or_manager) != 0) {
        c.set({{3, c.u32(contract_or_club)}, {4, c.toc(slot::clubs)}});
        c.set(3, c.u32(c.r(4)) + c.u32(c.r(3) + 4) * 0x248);
    } else c.set(3, 0);
    contract_or_club = c.r(3);
    const auto group = std::uint32_t(screens) + std::uint32_t(c.i16(std::uint32_t(screens) + 0x2c58)) * 0x2c0;
    c.set(7, c.u32(group + 4)); revision_or_manager = c.u32(c.r(7) + 0x24);
    p.history(0x10417710, screens, 8, 0);
    const auto eligible = byte_result(c, p.call(0x103f8820, 0x10417734,
        {indirect(c, slot::transfers), revision_or_manager, c.toc(slot::transfer_context), person, contract_or_club, 1, 0}));
    p.history(eligible ? 0x10417750 : 0x10417768, screens, 9, eligible);
    bool pending = false;
    if (compare_logical(c, link) != 0 && compare_logical(c, c.u32(link)) != 0) {
        c.set(3, c.u32(link));
        if (compare(c, c.i8(c.r(3) + 0x34), 5) >= 0) {
            c.set(3, c.u32(link)); pending = compare(c, c.u8(c.r(3) + 0x34), 16) != 0;
        }
    }
    p.history(pending ? 0x104177b4 : 0x104177cc, screens, 7, pending);
    // Each history slot has its own original call boundary, observable by callees.
    for (unsigned index = 15; index <= 22; ++index)
        p.history(0x104177e0 + (index - 15) * 20, screens, index, missing);
}

void open_existing_offer(Cpu& c, std::uint32_t offer_link, std::uint32_t create_screen) {
    Frame f(c, 0x10417880, 0x50, {.first_gpr = 27});
    Kept<27> current_contract(c, c.toc(slot::current_contract));
    Kept<28> link(c, offer_link);
    Kept<30> clubs(c, c.toc(slot::clubs));
    Kept<31> screens(c, c.toc(slot::screens));
    Program p{c};
    if (byte_result(c, create_screen) && !byte_result(c, p.call(0x1073e4b0, 0x104178c4,
        {screens, c.toc(-0x4abc), c.toc(-0x4ac0), 1, c.toc(-0x4ac4), 0}))) return;
    p.history(0x104178e4, screens, 0, c.u32(c.u32(link) + 12));
    p.history(0x104178f8, screens, 1, link);
    p.history(0x1041790c, screens, 2, 0);
    p.history(0x10417924, screens, 3, c.u32(c.u32(link) + 0x2c));
    p.history(0x10417938, screens, 4, 0);
    p.history(0x1041794c, screens, 5, 0);
    p.history(0x10417960, screens, 6, 0);
    Kept<29> offer_address(c, c.u32(link)); const Offer offer{c, offer_address};
    c.set(5, c.toc(slot::contracts));
    const auto person = indirect(c, slot::people) + std::uint32_t(offer.person()) * 0x70;
    const auto contract = p.call(0x10082f20, 0x10417984, {c.u32(c.r(5)), person});
    compare_logical(c, contract); c.put32(current_contract, contract);
    std::uint32_t club = 0;
    if (contract) {
        c.set({{3, c.u32(current_contract)}, {4, c.u32(clubs)}});
        club = c.r(4) + c.u32(c.r(3) + 4) * 0x248;
    }
    const auto controlled = p.call(0x101929b0, 0x104179bc, {club, 0, 0});
    p.history(0x104179d4, screens, 8, std::uint32_t(std::int32_t(std::int8_t(controlled))));
    if (compare(c, std::int8_t(offer.response().get())) == 0) offer.response() = 2;
    const auto bidder = c.u32(clubs) + c.u32(c.u32(link) + 8) * 0x248;
    const auto eligible = byte_result(c, p.call(0x103f8820, 0x10417a1c,
        {indirect(c, slot::transfers), bidder, c.toc(slot::transfer_context), 0, 0, 0, 0}));
    p.history(eligible ? 0x10417a38 : 0x10417a50, screens, 9, eligible);
    bool pending = false;
    if (compare_logical(c, link) != 0 && compare_logical(c, c.u32(link)) != 0) {
        c.set(3, c.u32(link)); pending = compare(c, c.i8(c.r(3) + 0x34), 5) >= 0;
    }
    p.history(pending ? 0x10417a8c : 0x10417aa4, screens, 7, pending);
    for (unsigned index = 15; index <= 22; ++index)
        p.history(0x10417ab8 + (index - 15) * 20, screens, index, missing);
}

void idle_callback(Cpu&) {}

void open_offer_context(Cpu& c, std::uint32_t first, std::uint32_t second, std::uint32_t third) {
    Frame f(c, 0x1041ea60, 0x50, {.first_gpr = 28});
    Kept<31> screens(c, c.toc(slot::screens));
    Kept<30> context(c, third); Kept<29> other(c, second); Kept<28> primary(c, first);
    Program p{c};
    if (!byte_result(c, p.call(0x1073e4b0, 0x1041eaa8,
        {screens, c.toc(-0x4ad8), c.toc(-0x4adc), 1, 0, 0}))) return;
    p.history(0x1041eac4, screens, 0, primary);
    p.history(0x1041ead8, screens, 1, other);
    p.history(0x1041eaec, screens, 2, context);
}

void open_previous_offer(Cpu& c, std::uint32_t offer, std::uint32_t revision) {
    Frame f(c, 0x10422570, 0x50, {.first_gpr = 29});
    Kept<31> screens(c, c.toc(slot::screens));
    Kept<30> previous_revision(c, revision); Kept<29> previous_offer(c, offer);
    Program p{c};
    if (!byte_result(c, p.call(0x1073e4b0, 0x104225b0,
        {screens, c.toc(-0x4b00), c.toc(-0x4b04), 1, 0, 0}))) return;
    p.history(0x104225cc, screens, 0, previous_offer);
    p.history(0x104225e0, screens, 1, previous_revision);
}
}
