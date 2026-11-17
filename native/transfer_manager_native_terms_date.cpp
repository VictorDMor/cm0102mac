#include "transfer_manager_native_terms.hpp"
#include "transfer_manager_native_stages_detail.hpp"

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
using namespace stages;

namespace {
// A five-byte date (halfword, halfword, byte) copied the way the original does.
void copy_date(Cpu& c, std::uint32_t to, std::uint32_t from) {
    c.put16(to + 2, c.u16(from + 2));
    c.put16(to, std::uint16_t(c.i16(from)));
    c.put8(to + 4, c.u8(from + 4));
}
// The original's test "is date `a` before date `b`" for dates whose halfwords at +2 are equal: the sign of
// ((x >> 1) - (x & a)) with x = a ^ b, each halfword at +0 sign-extended.
bool first_halfword_decides(std::int32_t a, std::int32_t b) {
    const auto x = a ^ b;
    return ((x >> 1) - (x & a)) < 0;
}
}

std::uint32_t term_date(Cpu& c, std::uint32_t manager, std::uint32_t record, std::uint32_t out) {
    Frame frame(c, 0x103f89e0, 0xa0, {.first_gpr = 22});
    const auto contracts_cell = c.toc(toc::contracts), found_cell = c.toc(toc::found_contract);
    const auto default_date = c.toc(-0x7b80), log_cell = c.toc(-0x7ba8), strings = c.toc(-0x4b9c);
    keep(c, 22, found_cell);
    keep(c, 23, 0);
    keep(c, 24, manager);
    keep(c, 25, record);
    keep(c, 26, out);
    keep(c, 28, default_date);
    keep(c, 29, contracts_cell);
    keep(c, 30, log_cell);
    keep(c, 31, strings);
    const auto person = person_record(c, c.u32(record + 0xc));
    keep(c, 27, person);
    const auto contract = invoke(c, find_contract, 0x103f8a34, {c.u32(contracts_cell), person});
    c.put32(found_cell, contract);
    const bool has_club = contract && contract_kind(c, contract) != no_club_contract_kind;
    keep(c, 23, has_club ? 1 : 0);
    const auto club = has_club ? club_record(c, c.u32(c.u32(found_cell) + 4)) : 0;
    const auto kind = std::int32_t(std::int8_t(c.u8(record + 0x38)) >> 4);
    switch (kind) {
    case 0:
        copy_date(c, out, default_date);
        return 0;
    case 5:
        invoke_native(c, 0x103fb130, 0x103f8ad4, {manager, out, 0});
        return 0;
    case 1:
    case 4:
        return invoke_native(c, 0x103f8cc0, 0x103f8afc, {manager, record, out});
    case 2: {
        if (!club) {
            report_bad_offer(c, strings, log_cell, c.toc(-0x7bac), frame.at(0x58), frame.at(0x50), 0x31d9,
                             {0x103f8b28, 0x103f8b2c, 0x103f8b48, 0x103f8b58});
            return 0;
        }
        const auto found = std::int8_t(invoke(c, 0x101929b0, 0x103f8b78, {club, 0, out}));
        if (found) {
            invoke(c, 0x10717890, 0x103f8b90, {frame.at(0x48), out, 7});
            copy_date(c, out, frame.at(0x48));
        } else {
            copy_date(c, out, default_date);
        }
        return 0;
    }
    case 3: {
        const auto second = invoke(c, find_contract, 0x103f8be0, {c.u32(contracts_cell), person});
        if (!second) {
            copy_date(c, out, default_date);
            return 0;
        }
        invoke(c, 0x10717890, 0x103f8bf8, {frame.at(0x40), second + 0x30, 1});
        copy_date(c, out, frame.at(0x40));
        // The default date is copied to the frame's scratch words first; the comparison reads its halfwords.
        bool replace;
        const auto default_second = c.u16(default_date + 2), own_second = c.u16(out + 2);
        if (default_second > own_second) replace = true;
        else if (default_second < own_second) replace = false;
        else replace = first_halfword_decides(c.i16(default_date), c.i16(out));
        if (replace) copy_date(c, out, default_date);
        return 0;
    }
    default:
        return 0;
    }
}
}
