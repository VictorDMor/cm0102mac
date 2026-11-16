// TRANSFER_MANAGER.CPP: fee refusals, staff offer reasons, the status row ageing and the may-offer test
// (block "queries").
#include "transfer_manager_native_queries.hpp"
#include "transfer_manager_native_queries_detail.hpp"

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
using namespace queries_detail;
namespace {
constexpr std::uint32_t money_of = 0x10009f20, staff_flag = 0x10380520, club_budget_record = 0x10103510, worth_of_contract = 0x10085860,
                        offer_value = 0x10416840, staff_offer_reasons = 0x101a0f50;
constexpr std::int32_t budget_cell = -0x7ab8, season_cell = -0x6678, staff_cell = -0x79f0;

// The value of an offer as the transfer screens count it (0x10416840 with its usual flags).
std::uint32_t value_of(Cpu& c, std::uint32_t offer, std::uint32_t back) { return invoke(c, offer_value, back, {offer, 0, 0, 1, 0, 1}); }
void store_reason(Cpu& c, std::uint32_t reason_out, std::uint32_t reason) {
    if (reason_out != 0) c.put16(reason_out, reason);
}
}

bool offer_fee_refused(Cpu& c, std::uint32_t offer, std::uint32_t competing, std::uint32_t reason_out) {
    Frame frame(c, 0x103f4d10, 0x80, {.first_gpr = 23});
    const auto clubs_cell = c.toc(toc::clubs), contracts_cell = c.toc(toc::contracts), found_cell = c.toc(toc::found_contract);
    const auto person = c.u32(c.toc(toc::people)) + c.u32(offer + 0xc) * person_size;
    const auto buyer = c.u32(clubs_cell) + c.u32(offer + 8) * club_size;
    keep(c, 23, 0);
    keep(c, 24, found_cell);
    keep(c, 25, offer);
    keep(c, 26, competing);
    keep(c, 27, reason_out);
    keep(c, 28, person);
    keep(c, 29, buyer);
    keep(c, 30, contracts_cell);
    keep(c, 31, clubs_cell);
    const auto found = invoke(c, find_contract, 0x103f4d6c, {c.u32(contracts_cell), person});
    c.put32(found_cell, found);
    const auto club = contract_club_of(c, found);
    if (club == 0) return true;
    keep(c, 23, 1);
    keep(c, 31, club);
    const auto contract = invoke(c, find_contract, 0x103f4dd4, {c.u32(contracts_cell), person});
    keep(c, 23, contract);
    // 0x10380520 is handed what the lookup returned.
    if (std::int8_t(invoke(c, staff_flag, 0x103f4ddc, {contract})) != 0 && c.u32(offer + 4) == 0 && c.i8(offer + 0x51) == 0) {
        c.put32(frame.at(0x3c), c.u32(offer + 0x20));
        c.put32(frame.at(0x38), c.u32(contract + 0x24));
        invoke(c, money_of, 0x103f4e18, {frame.at(0x38), 4});
        const auto first = std::int64_t(std::uint64_t(c.r(3)) << 32 | c.r(4));
        keep(c, 24, c.r(3));
        keep(c, 23, c.r(4));
        invoke(c, money_of, 0x103f4e2c, {frame.at(0x3c), 4});
        const auto second = std::int64_t(std::uint64_t(c.r(3)) << 32 | c.r(4));
        if (first <= second) {
            store_reason(c, reason_out, 0x1b);
            return true;
        }
    }
    const auto budget = invoke(c, club_budget_record, 0x103f4e78, {c.u32(c.toc(budget_cell)), club});
    if (c.u8(budget + 0x165) != 1) return false;
    const auto fee = invoke(c, worth_of_contract, 0x103f4eac, {c.u32(contracts_cell), club, buyer, person, 0, 1, 1});
    keep(c, 28, fee);
    const auto value = value_of(c, offer, 0x103f4ecc);
    const auto scaled_squad = std::int32_t(std::uint32_t(std::int32_t(c.i16(club + 0x1ec))) * 0x14u);
    const auto floor = scaled_squad >= 20000 ? scaled_squad : 20000;
    if (std::int32_t(fee) < floor) return false;
    if (std::int32_t(value << 1) <= std::int32_t(fee) || std::int32_t(value) <= 50000) return false;
    if (competing == 0) {
        store_reason(c, reason_out, 0x1c);
        return true;
    }
    const auto other = value_of(c, competing, 0x103f4f34);
    guest::Fpu fpu(c);
    const auto pool = c.toc(pool_cell);
    auto scaled = to_double(fpu, c, pool, std::int32_t(other));
    scaled = fpu.fmul(c.f64(pool + 0x150), scaled);
    const auto budgeted = to_double(fpu, c, pool, std::int32_t(fee));
    fpu.fcmp(0, scaled, budgeted);
    if (!(scaled > budgeted)) return false;  // ble: taken when not greater, including unordered
    store_reason(c, reason_out, 0x1c);
    return true;
}

bool staff_offer_refused(Cpu& c, std::uint32_t offer, std::uint32_t club_a, std::uint32_t person, std::uint32_t club_b,
                         std::uint32_t reason_out) {
    Frame frame(c, 0x103f5a50, 0x80, {.first_gpr = 26});
    const auto pool = c.toc(pool_cell);
    keep(c, 26, offer);
    keep(c, 27, club_a);
    keep(c, 28, club_b);
    keep(c, 29, person);
    keep(c, 30, reason_out);
    keep(c, 31, pool);
    const auto fee = value_of(c, offer, 0x103f5a94);
    std::uint32_t reasons = 0;
    if (offer != 0 && (c.u32(offer + 4) == 0 || c.i16(c.u32(offer + 4)) == 0)) {
        reasons = invoke(c, staff_offer_reasons, 0x103f5adc, {c.u32(c.toc(staff_cell)), person, club_b, fee, club_a == club_b ? 1u : 0u});
    }
    if (reasons & 0x2000) {
        store_reason(c, reason_out, 0x24);
        guest::Fpu fpu(c);
        // The person's three profile values (+0xa, +8, +0xc), each scaled by the pool factor and truncated.
        for (const std::uint32_t field : {0xau, 0x8u, 0xcu}) {
            const auto profile = c.u32(person + 0x14);
            auto value = to_double(fpu, c, pool, c.i16(profile + field));
            value = fpu.fmul(c.f64(pool + 0xc8), value);
            c.put16(profile + field, std::uint32_t(fpu.fctiwz(0, value)));
        }
        return true;
    }
    if (reasons & 0x800) {
        store_reason(c, reason_out, 0x17);
        return true;
    }
    if (reasons & 0x200) {
        store_reason(c, reason_out, 0x18);
        return true;
    }
    return false;
}

void touch_status_row(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t date) {
    const auto row = status_row(c, manager, c.u32(person));
    const auto shifts = std::int32_t(std::int16_t(date - std::uint32_t(c.i16(row + 6))));
    if (shifts > 0) {
        // The original shifts the row's 16-bit history left one day at a time, clearing bit 0 after each
        // step: the end state is one store of the history shifted by the days, 0 from 16 days on.
        const auto history = std::uint32_t(c.i16(row + 4));
        c.put16(row + 4, shifts >= 16 ? 0 : (history << shifts) & 0xfffe);
    }
    c.put16(row + 4, std::uint32_t(c.i16(row + 4)) | 1);
    c.put16(row + 6, date);
}

bool offer_allowed(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t other, std::uint32_t reason_out,
                   std::uint32_t skip_pending_checks) {
    Frame frame(c, 0x103f83c0, 0x60, {.first_gpr = 26});
    const auto season = c.toc(season_cell);
    keep(c, 26, manager);
    keep(c, 27, person);
    keep(c, 28, other);
    keep(c, 29, reason_out);
    keep(c, 30, skip_pending_checks);
    keep(c, 31, season);
    const auto iterator = frame.at(iterator_slot);
    invoke(c, iterator_construct, 0x103f83f0, {iterator});
    const auto refuse = [&](std::uint32_t reason, std::uint32_t back) {
        store_reason(c, reason_out, reason);
        destroy_iterator(c, iterator, back);
        return false;
    };
    // A player of a club that must keep him, offered to a club of another nation.
    const auto club = c.u32(person + 0xc);
    if (club != 0 && c.u32(club + 0x68) == c.u32(c.toc(-0x7588)) &&
        (c.u32(person + 0x24) == 0 || c.u32(c.u32(person + 0x24) + 0x2c) == club) && other != 0 && c.u32(other + 0x2c) != club)
        return refuse(0x27, 0x103f8458);
    const auto id = c.u32(person);
    const auto row = c.u32(manager + status_rows) + id * status_row_size;
    bool contracted = false;
    if (c.u8(row) & 0x40) {
        if (invoke(c, 0x1007f7b0, 0x103f8484, {c.u32(c.toc(toc::contracts)), id}) != 0) {
            contracted = true;
        } else {
            const auto cleared = c.u32(manager + status_rows) + c.u32(person) * status_row_size;
            c.put8(cleared, c.u8(cleared) & 0xbf);
        }
    }
    if (contracted) return refuse(0x21, 0x103f84dc);
    if (std::int8_t(skip_pending_checks) == 0 &&
        (invoke(c, 0x103eb020, 0x103f84fc, {manager, person, 0}) != 0 || invoke(c, 0x103eb1f0, 0x103f8514, {manager, person, 0}) != 0))
        return refuse(7, 0x103f8538);
    // Already in the manager's list of people being dealt with.
    c.put32(manager + 0xa68, c.u32(manager + 0xa60));
    while (const auto entry = invoke(c, iterator_next, 0x103f856c, {manager + 0xa5c}))
        if (c.u32(person) == c.u32(entry)) return refuse(1, 0x103f859c);
    if (std::int8_t(invoke(c, has_offers, 0x103f85b4, {manager, person, 0})) != 0) {
        start_offer_walk(c, frame, manager, person, 0, 0x103f85d0);
        while (const auto step = invoke(c, iterator_next, 0x103f868c, {iterator})) {
            const auto offer = offer_of_step(c, manager, step);
            const auto kind = c.i8(offer + offer_kind);
            if (kind == 10 || std::uint8_t(std::uint8_t(kind) - 0xb) <= 2 || kind == 0xe) {
                c.put8(offer + 0x35, 1);
                return refuse(0x12, 0x103f8650);
            }
            if (kind == 0x13) return refuse(0x20, 0x103f867c);
        }
    }
    if (other != 0 && person != 0 && c.u32(person + 0x14) != 0) {
        if (std::int8_t(invoke(c, 0x100ca800, 0x103f86b8, {other})) != 0) {
            const auto limit = c.u32(season);
            const auto table = c.u32(c.toc(-0x7b98)) + limit * 0x128;
            if (std::int8_t(invoke(c, 0x100cab10, 0x103f86e0, {person, table, 1})) != 0) return refuse(0x19, 0x103f8704);
        } else if (std::int8_t(invoke(c, 0x10411e10, 0x103f8714, {other})) == 0) {
            if (std::int8_t(invoke(c, 0x100cab10, 0x103f872c, {person, c.u32(other + 0x2c), 1})) != 0) return refuse(0x28, 0x103f8750);
        } else {
            const auto other_id = c.u32(other);
            if ((other_id == c.u32(c.toc(-0x7724)) || other_id == c.u32(c.toc(-0x7864))) &&
                (c.u32(person + 0xc) == 0 || c.u32(c.u32(person + 0xc) + 0x68) == c.u32(c.toc(-0x7a18)))) {
                const auto holder = c.u32(person + 0x10);
                if (holder == 0 || c.u32(holder + 0x68) != c.u32(season)) return refuse(0x1a, 0x103f87d4);
            }
        }
    }
    store_reason(c, reason_out, 0);
    destroy_iterator(c, iterator, 0x103f87f8);
    return true;
}
}
