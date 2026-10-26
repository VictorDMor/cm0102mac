#include "shortlist_manager_detail.hpp"

// The staff-club links, status: the link status of a staff at a club (from a
// row, from the contract, or derived from ratings), and the marking of the
// rows of a staff's links.
namespace cm::aot::shortlist_manager {
using namespace detail;
using namespace links;
namespace {
// bge / ble after an fcmpo into cr0: branch when the less-than (greater-than) bit is clear.
bool not_less(Cpu& c) { return (c.cr(0) & 8) == 0; }
bool not_greater(Cpu& c) { return (c.cr(0) & 4) == 0; }
bool greater(Cpu& c) { return (c.cr(0) & 4) != 0; }
}

std::uint32_t derive_link_status(Cpu& c, std::uint32_t /* links: unused */, std::uint32_t staff, std::uint32_t club) {
    Frame frame(c, 0x1036d2f0, 0x60, {.first_gpr = 28});
    const auto constants = c.toc(slot::constants);
    const auto constant = [&](std::uint32_t offset) { return c.f64(constants + offset); };
    keep(c, {{31, constants}, {30, staff}, {28, club}});
    Program program(c);
    // The staff's best count (0x100cdef0 for the club's key), at least 1.
    std::int32_t first = half_of(program.call(0x100cdef0, 0x1036d330, {staff, program.call(0x100c96e0, 0x1036d320, {club}), 0}));
    if (compare(c, first, 1) > 0) {
        const auto key = program.call(0x100c96e0, 0x1036d344, {club});
        first = half_of(program.call(0x100cdef0, 0x1036d354, {staff, key, 0}));
    } else {
        first = 1;
    }
    Kept<29> top(c, extended(std::int16_t(first)));
    // The club's rating, at least 1.
    const std::int32_t rating = Field<std::int16_t>{c, club + 0x1ec};
    std::int32_t clamped = 1;
    if (compare(c, rating, 1) > 0) clamped = rating;
    Kept<28> bottom(c, extended(std::int16_t(clamped)));
    const auto key = program.call(0x100c96e0, 0x1036d384, {club});
    const auto service = c.toc(link_slot::staff_service);
    c.set(6, service);
    Fpu fpu{c};
    const auto accepted = program.call(0x1030fbf0, 0x1036d398, {c.u32(service), staff, key});
    if (nonzero(c, byte_of(accepted))) {
        // The count becomes the larger of (count * constant +0x178) and (rating * constant +0xa0), truncated.
        const double minuend = constant(8);
        const double scaled_top = fpu.fmul(constant(0x178),
                                           fpu.fsub(stacked_integer(c, frame.at(0x48), signed_word(std::int32_t(top))), minuend));
        const double scaled_bottom = fpu.fmul(constant(0xa0),
                                              fpu.fsub(stacked_integer(c, frame.at(0x40), signed_word(std::int32_t(bottom))), minuend));
        fpu.fcmp(0, scaled_top, scaled_bottom);
        const double larger = not_greater(c) ? scaled_bottom : scaled_top;
        const auto bits = fpu.fctiwz(0, larger);
        c.put64(frame.at(0x38), bits);
        top = c.u32(frame.at(0x3c));
    }
    // The ratio of the count to the rating, as singles.
    const std::int32_t count = half_of(top);
    const double minuend = constant(8);
    // (The low byte of the halfword at +0x5c, i.e. the byte at +0x5d.)
    const std::int32_t staff_byte = byte_of(std::uint32_t(Field<std::int16_t>{c, staff + 0x5c}.get()));
    const double numerator = fpu.fsubs(stacked_integer(c, frame.at(0x38), signed_word(count)), minuend);
    c.set_f(1, numerator);  // the original computes it into f1, left at return
    const double low = constant(0x48);
    const double denominator =
        fpu.fsubs(stacked_integer(c, frame.at(0x40), signed_word(std::int32_t(bottom))), minuend);
    const double ratio = fpu.fdivs(numerator, denominator);
    // Clamped between the constants +0x48 and +0xe8.
    double clamp = low;
    fpu.fcmp(0, low, ratio);
    if (!greater(c)) {
        clamp = constant(0xe8);
        fpu.fcmp(0, clamp, ratio);
        if (not_less(c)) clamp = ratio;
    }
    clamp = fpu.frsp(clamp);
    c.set(4, std::uint32_t(staff_byte));
    fpu.fcmp(0, clamp, constant(0x178));
    if (greater(c)) return result(c, 1);
    fpu.fcmp(0, clamp, constant(0x170));
    if (greater(c)) {
        if (compare(c, std::int32_t(bottom), 0x2134) < 0) return result(c, 2);
    }
    fpu.fcmp(0, clamp, constant(0x170));
    if (greater(c)) return result(c, 3);
    if (compare(c, half_of(top), 0x1964) > 0) return result(c, 3);
    fpu.fcmp(0, clamp, constant(0x180));
    if (greater(c) && compare(c, staff_byte, 0x17) < 0) return result(c, 5);
    if (compare(c, staff_byte, 0x18) < 0) return result(c, 6);
    return result(c, 4);
}

std::uint32_t link_status(Cpu& c, std::uint32_t links, std::uint32_t staff, std::uint32_t club, std::uint32_t type) {
    Frame frame(c, 0x1036d510, 0x50, {.first_gpr = 26});
    const auto contracts = c.toc(slot::contracts);
    keep(c, {{30, club}, {31, contracts}, {27, c.toc(link_slot::last_contract)}, {28, links}, {29, staff}, {26, type}});
    if (!nonzero(c, std::int32_t(club))) return result(c, 7);
    Program program(c);
    // The row of the club for the staff: as his primary staff id (if he has a
    // nonzero one), or else with the intervening id.
    const std::int8_t ordinary = Field<std::uint32_t>{c, staff + 0x14}.get() == 0;
    enter(c, 0x1036d564, {links, club, staff, std::uint32_t(ordinary), 0});
    std::uint32_t row = find_link_row(c, links, club, staff, ordinary, 0);
    if (!nonzero_logical(c, row) && nonzero_logical(c, Field<std::uint32_t>{c, staff + 0x1c})) {
        enter(c, 0x1036d590, {links, club, staff, 1, 0});
        row = find_link_row(c, links, club, staff, 1, 0);
    }
    if (nonzero_logical(c, row)) {
        if (nonzero_logical(c, type)) c.put8(type, c.u8(row));
        return result(c, std::uint32_t(byte_of((c.u8(row + 0x28) >> 3) & 0xf)));
    }
    if (nonzero_logical(c, type)) c.put8(type, c.u8(staff + 0x62));
    // The status in the staff's contract at the club.
    const auto contract_status = [&](std::uint32_t back) {
        const auto contract = program.call(0x10082f90, back, {c.u32(contracts), staff, club});
        if (!nonzero_logical(c, contract)) return result(c, 7);
        return result(c, c.u8(contract + 0x60) & 0xf);
    };
    // The club of the staff's contract (0 if he has none), kept in the shared scratch word as well.
    const auto holder = program.call(0x10082f20, 0x1036d5b4, {c.u32(contracts), staff});
    const bool has_contract = nonzero_logical(c, holder);
    c.put32(c.r(27), holder);
    std::uint32_t contract_club = 0;
    if (has_contract) contract_club = c.u32(c.toc(slot::clubs)) + c.u32(holder + 4) * 0x248;
    if (compare_logical(c, contract_club, club) == 0) return contract_status(0x1036d5f4);
    const auto reserved = program.call(0x1014c2e0, 0x1036d628, {club, 1, 1, 0, 0});
    if (nonzero(c, byte_of(reserved))) {
        const auto nation = c.u8(staff + 0x62);
        c.set(3, nation);
        const auto valid = program.call(0x100c7a20, 0x1036d638, {nation});
        if (nonzero(c, byte_of(valid))) {
            enter(c, 0x1036d650, {links, staff, club});
            return derive_link_status(c, links, staff, club);
        }
    }
    if (compare_logical(c, c.u32(staff + 0x24), club) != 0) {
        const auto home = program.call(0x100ce320, 0x1036d670, {club, 0, 1});
        if (compare_logical(c, c.u32(staff + 0x24), home) != 0) return result(c, 7);
    }
    return contract_status(0x1036d694);
}

void mark_linked_rows(Cpu& c, std::uint32_t links, std::uint32_t staff, std::uint32_t skip_club, std::uint32_t list) {
    Frame frame(c, 0x1036fa00, 0x70, {.first_gpr = 22});
    const auto clubs = c.toc(slot::clubs);
    const auto manager = c.toc(link_slot::transfer_manager);
    keep(c, {{30, manager}, {24, links}, {31, clubs}, {25, staff}, {26, skip_club}, {27, list}});
    Program program(c);
    const auto own = frame.at(0x38);  // a list of pairs, 16 bytes
    program.call(0x10738c00, 0x1036fa30, {own});
    enter(c, 0x1036fa4c, {links, staff, own, 0, 1, 0});
    collect_staff_links(c, links, staff, own, 0, 1, 0);
    c.put32(frame.at(0xc + 0x38), c.u32(frame.at(0x3c)));  // the cursor starts at the head
    Kept<23> pair(c, c.r(23));
    Kept<22> found(c, c.r(22));
    Kept<28> holder(c, c.r(28));
    Kept<29> club(c, c.r(29));
    const auto finish = [&](std::uint32_t back) { program.call(0x10738c20, back, {own, std::uint32_t(-1)}); };
    for (;;) {
        pair = program.call(0x10738e00, 0x1036fbbc, {own});
        if (!nonzero(c, std::int32_t(std::uint32_t(pair)))) {
            finish(0x1036fbd0);
            return;
        }
        club = c.u32(clubs) + c.u32(std::uint32_t(pair) + 4) * 0x248;
        const auto reserved = program.call(0x1014c2e0, 0x1036fa80, {club, 0, 1, 0, 0});
        if (nonzero(c, byte_of(reserved))) continue;
        if (nonzero_logical(c, list)) {
            c.put32(list + 0xc, c.u32(list + 4));
            found = 0;
            for (;;) {
                const auto element = program.call(0x10738e00, 0x1036fadc, {list});
                c.set(4, element);
                if (!nonzero(c, std::int32_t(element))) break;
                holder = program.call(0x103eb440, 0x1036faac, {c.u32(manager), c.u32(element)});
                if (!nonzero(c, std::int32_t(std::uint32_t(holder)))) continue;
                const auto id = c.u32(c.u32(holder) + 8);
                c.set(3, id);
                if (compare(c, std::int32_t(id), std::int32_t(c.u32(std::uint32_t(pair) + 4))) == 0) {
                    found = 1;
                    break;
                }
            }
            if (nonzero(c, byte_of(found))) continue;
            const auto record = c.u32(holder);
            const std::int32_t kind = byte_of(c.u8(record + 0x11));
            // 1 when the record's kind is not 11 (the sign bit of (11 - kind) | (kind - 11)).
            const std::uint32_t other = ((11u - std::uint32_t(kind)) | (std::uint32_t(kind) - 11u)) >> 31;
            enter(c, 0x1036fb20, {links, club, staff, other, 0});
            const auto row = find_link_row(c, links, club, staff, std::int8_t(other), 0);
            if (!nonzero_logical(c, row)) {
                finish(0x1036fb34);
                return;
            }
            c.put8(row + 0x28, c.u8(row + 0x28) | 4);
            continue;
        }
        if (compare_logical(c, club, skip_club) == 0) continue;
        enter(c, 0x1036fb6c, {links, club, staff, 0, 0});
        auto row = find_link_row(c, links, club, staff, 0, 0);
        if (!nonzero_logical(c, row)) {
            enter(c, 0x1036fb8c, {links, club, staff, 1, 0});
            row = find_link_row(c, links, club, staff, 1, 0);
        }
        if (!nonzero_logical(c, row)) {
            finish(0x1036fba0);
            return;
        }
        c.put8(row + 0x28, c.u8(row + 0x28) | 4);
    }
}
}
