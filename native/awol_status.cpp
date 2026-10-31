#include "awol_detail.hpp"

namespace cm::aot::awol {
using namespace detail;
namespace {
// The id guard the three status routines share. A null staff member is
// answered 0 before it, and only the upper bound is tested.
bool known_staff(Cpu& c, std::uint32_t staff) {
    if (compare(c, std::int32_t(staff)) == 0) return false;
    c.set(4, c.u32(staff));
    return compare(c, std::int32_t(c.r(4)), c.i32(c.toc(-0x7be4))) < 0;
}
std::uint32_t flag_of(Cpu& c, std::uint32_t records, std::uint32_t staff, std::uint32_t bit) {
    return c.u8(c.u32(records + flags_buffer) + c.u32(staff)) & bit;
}
// A flagged staff member without a club loses the record. Returns with the
// contract cached, as a side effect every caller of the status sees.
template <class Remove>
void drop_if_clubless(Cpu& c, std::uint32_t records, std::uint32_t staff, std::uint32_t cached, std::uint32_t lookup_back,
                      std::uint32_t remove_back, Remove remove) {
    const auto contract = call(c, contract_of, lookup_back, {c.u32(c.toc(-0x7b7c)), staff});
    c.put32(cached, contract);
    if (compare_logical(c, club_of_contract(c, contract)) == 0) {
        enter(c, remove_back, {records, staff});
        remove(c, records, staff);
    }
}
}

std::uint32_t primary_status(Cpu& c, std::uint32_t records, std::uint32_t staff, std::uint32_t raw) {
    Frame frame(c, 0x10003dc0, 0x50, {.first_gpr = 29});
    keep(c, {{31, staff}, {30, c.toc(-0x7b78)}, {29, records}});
    if (!known_staff(c, staff)) return result(c, 0);
    if (compare(c, std::int8_t(raw)) != 0) return result(c, flag_of(c, records, staff, primary_bit));
    if (compare(c, std::int32_t(flag_of(c, records, staff, primary_bit))) != 0)
        drop_if_clubless(c, records, staff, c.r(30), 0x10003e54, 0x10003e90, remove_primary_by_staff);
    return result(c, flag_of(c, records, staff, primary_bit));
}

std::uint32_t secondary_status(Cpu& c, std::uint32_t records, std::uint32_t staff) {
    Frame frame(c, 0x10003ec0, 0x50, {.first_gpr = 29});
    keep(c, {{31, c.toc(-0x7b78)}, {30, staff}, {29, records}});
    if (!known_staff(c, staff)) return result(c, 0);
    if (compare(c, std::int32_t(flag_of(c, records, staff, secondary_bit))) != 0)
        drop_if_clubless(c, records, staff, c.r(31), 0x10003f34, 0x10003f70, remove_secondary_by_staff);
    return result(c, flag_of(c, records, staff, secondary_bit));
}

std::uint32_t pending_status(Cpu& c, std::uint32_t records, std::uint32_t staff) {
    Frame frame(c, 0x10003fa0, 0x50, {.first_gpr = 29});
    keep(c, {{30, staff}, {29, records}});
    if (!known_staff(c, staff)) return result(c, 0);
    c.set(3, c.u32(records + pending_list));
    if (compare_logical(c, c.r(3)) == 0) return result(c, 0);
    rewind(c, c.r(3));
    c.set(31, 0);
    for (;;) {
        c.set(3, c.u32(records + pending_list));
        if (compare(c, std::int32_t(c.r(31)), list_count(c, c.r(3))) >= 0) return result(c, 0);
        c.call(list_next, 0x1000400c);
        if (compare_logical(c, c.r(3)) == 0) return result(c, 0);
        c.set(3, c.u32(c.r(3)));
        if (compare(c, std::int32_t(c.r(3)), c.i32(staff)) == 0) return result(c, 1);
        c.set(31, c.r(31) + 1);
    }
}

namespace {
// Clears the staff member's bit and removes their record from the list. The
// original tests the bit, then XORs it behind a test of `flag | bit` that can
// never be zero.
void remove_by_staff(Cpu& c, std::uint32_t records, std::uint32_t staff, std::uint32_t member, std::uint32_t bit, std::uint32_t entry) {
    Frame frame(c, entry, 0x50, {.first_gpr = 29});
    keep(c, {{30, staff}, {29, records}});
    if (compare(c, std::int32_t(staff)) == 0) return;
    const auto flags = c.u32(records + flags_buffer);
    c.set(4, flags);
    if (compare_logical(c, flags) == 0) return;
    const auto id = c.u32(staff);
    c.set(3, id);
    const auto flag = signed_byte(c.u8(flags + id));
    c.set(5, flag);
    if (compare(c, std::int32_t(flag & bit)) == 0) return;
    compare(c, std::int32_t(flag | bit));
    c.put8(flags + id, c.u8(flags + id) ^ bit);
    c.set(3, c.u32(records + member));
    if (compare_logical(c, c.r(3)) == 0) return;
    rewind(c, c.r(3));
    c.set(31, 0);
    const auto next_back = entry == 0x10006600 ? 0x1000667c : 0x100068ac;
    const auto remove_back = entry == 0x10006600 ? 0x1000669c : 0x100068cc;
    for (;;) {
        c.set(3, c.u32(records + member));
        if (compare(c, std::int32_t(c.r(31)), list_count(c, c.r(3))) >= 0) return;
        c.call(list_next, next_back);
        c.set(4, c.r(3));
        if (compare(c, std::int32_t(c.r(4))) == 0) return;
        c.set(3, c.u32(c.r(4)));
        if (compare(c, std::int32_t(c.r(3)), c.i32(staff)) == 0) {
            call(c, list_remove, remove_back, {c.u32(records + member)});
            return;
        }
        c.set(31, c.r(31) + 1);
    }
}
}
void remove_primary_by_staff(Cpu& c, std::uint32_t records, std::uint32_t staff) {
    remove_by_staff(c, records, staff, primary_list, primary_bit, 0x10006600);
}
void remove_secondary_by_staff(Cpu& c, std::uint32_t records, std::uint32_t staff) {
    remove_by_staff(c, records, staff, secondary_list, secondary_bit, 0x10006830);
}

// Ends a primary absence: tells the club if the staff member still has one,
// clears the flag and removes the record.
void remove_primary_item(Cpu& c, std::uint32_t records, std::uint32_t item) {
    Frame frame(c, 0x100066d0, 0x50, {.first_gpr = 28});
    keep(c, {{31, item}, {30, records}, {29, c.toc(-0x7b78)}, {28, c.toc(-0x7be0)}});
    if (compare(c, std::int32_t(item)) == 0) return;
    c.set(3, c.u32(records + flags_buffer));
    c.set(4, c.u32(item));
    if (compare(c, std::int8_t(c.u8(c.r(3) + c.r(4)))) == 0) return;
    const auto staff = c.u32(c.r(28)) + c.r(4) * staff_size;
    const auto contract = call(c, contract_of, 0x1000672c, {c.u32(c.toc(-0x7b7c)), staff});
    c.put32(c.r(29), contract);
    if (compare_logical(c, club_of_contract(c, contract)) != 0) {
        const auto owner = c.u32(c.r(28)) + c.u32(item) * staff_size;
        enter(c, 0x10006780, {records, owner, c.u32(owner + staff_club), 0, 0xffffffff});
        notify(c, c.r(4), c.r(5), c.r(6), c.r(7));
    }
    const auto flags = c.u32(records + flags_buffer), id = c.u32(item);
    c.set(5, flags);
    c.set(4, id);
    const auto flag = c.u8(flags + id);
    compare(c, std::int32_t(signed_byte(flag) | primary_bit));
    c.put8(flags + id, flag ^ primary_bit);
    call(c, list_remove, 0x100067b0, {c.u32(records + primary_list), item});
}

// Ends a training absence. Any nonzero flag byte toggles bit 1.
void remove_secondary_item(Cpu& c, std::uint32_t records, std::uint32_t item) {
    Frame frame(c, 0x100067d0, 0x40, {});
    if (compare_logical(c, item) == 0) return;
    const auto flags = c.u32(records + flags_buffer), id = c.u32(item);
    const std::uint32_t flag = c.u8(flags + id);
    c.set({{6, flags}, {5, id}, {7, flag}});
    if (compare(c, std::int8_t(flag)) == 0) return;
    compare(c, std::int32_t(signed_byte(flag) | secondary_bit));
    c.put8(flags + id, flag ^ secondary_bit);
    call(c, list_remove, 0x10006818, {c.u32(records + secondary_list)});
}

// The nine-field absence message (type 0xbc5). Above 7500 on the profile's
// ability field it goes to everyone, otherwise to the club's manager and,
// if different, the staff member's own (staff+0xc).
void notify(Cpu& c, std::uint32_t staff, std::uint32_t club, std::uint32_t kind, std::uint32_t value) {
    Frame frame(c, 0x10007100, 0x140, {.first_gpr = 26});
    const bool present = compare(c, std::int32_t(staff)) != 0;
    keep(c, {{31, staff}, {30, c.toc(-0x7bf0)}, {26, club}, {27, kind}, {28, value}, {29, 0xffffffff}});
    if (!present) return;
    if (compare_logical(c, c.u32(staff + staff_club)) == 0) return;
    if (compare_logical(c, c.u32(staff + staff_profile)) == 0) return;
    if (compare_logical(c, club) == 0) return;
    const auto message = frame.at(0x38);
    const auto field = [&](std::uint32_t index, std::uint32_t data, std::uint32_t back) { call(c, 0x102cb4b0, back, {message, index, data}); };
    call(c, 0x102bdf90, 0x1000715c, {message, 0xbc5, 0});
    field(0, c.u32(staff), 0x1000716c);
    field(1, c.u32(c.u32(staff + 0x28) + 0x20), 0x10007180);
    field(2, c.u32(c.u32(staff + 0x2c) + 0x20), 0x10007194);
    field(3, c.u32(c.u32(staff + 0x30) + 0x20), 0x100071a8);
    compare_logical(c, club);
    field(4, c.u32(club), 0x100071c0);
    // The message's setters can clear the staff member's club.
    c.set(3, c.u32(staff + staff_club));
    if (compare_logical(c, c.r(3)) != 0) field(5, c.u32(c.r(3)), 0x100071f4);
    else {
        c.set(29, 0xffffffff);
        field(5, 0xffffffff, 0x1000720c);
    }
    field(6, signed_byte(kind), 0x1000721c);
    field(7, extended(std::int16_t(value)), 0x1000722c);
    c.set(3, c.u32(staff + staff_club));
    bool manager = compare_logical(c, c.r(3)) != 0;
    if (manager) {
        c.set(3, c.u32(c.r(3) + 0x2c));
        manager = compare_logical(c, c.r(3)) != 0;
    }
    field(8, manager ? c.u32(c.r(3) + 0x68) : c.r(29), manager ? 0x10007254 : 0x10007268);
    c.set(3, c.u32(staff + staff_profile));
    if (compare(c, c.i16(c.r(3) + 0xc), 0x1d4c) > 0) {
        call(c, 0x102cc8e0, 0x10007284, {c.u32(c.r(30)), message});
        return;
    }
    c.set(3, c.u32(staff + staff_club));
    if (compare_logical(c, c.r(3)) != 0) {
        c.set(5, c.u32(c.r(3) + 0x2c));
        if (compare_logical(c, c.r(5)) != 0) call(c, 0x102cc400, 0x100072ac, {c.u32(c.r(30)), message});
    }
    const auto own = c.u32(staff + 0xc);
    c.set(3, c.u32(staff + staff_club));
    c.set(5, own);
    if (compare_logical(c, c.u32(c.r(3) + 0x2c), own) != 0) call(c, 0x102cc400, 0x100072cc, {c.u32(c.r(30)), message});
}
}
