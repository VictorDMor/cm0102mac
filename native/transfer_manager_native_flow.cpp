#include "transfer_manager_native_flow.hpp"
#include "transfer_manager_native_flow_common.hpp"

namespace cm::aot::transfer_manager_native {
using flow::s8;
using guest::Cpu;
using guest::Frame;

std::uint32_t make_candidate_offer(Cpu& c, std::uint32_t club, std::uint32_t person, std::uint32_t out_offer,
                                   std::uint32_t out_kind, std::uint32_t fee) {
    Frame frame(c, 0x103eec00, 0x80, {.first_gpr = 24});
    keep(c, 30, c.toc(-0x7ba8));
    keep(c, 25, club);
    keep(c, 31, c.toc(-0x4b9c));
    keep(c, 26, person);
    keep(c, 27, out_offer);
    keep(c, 28, out_kind);
    keep(c, 29, fee);
    const auto object = invoke(c, 0x10773520, 0x103eec34, {0x54});
    keep(c, 24, object);
    if (object) invoke(c, 0x10411e60, 0x103eec40, {object});
    c.put32(out_offer, object);
    invoke(c, 0x104164f0, 0x103eec58, {c.u32(out_offer), person, club, fee});
    if (!invoke(c, 0x10103510, 0x103eec68, {c.u32(c.toc(-0x7ab8)), club})) {
        flow::report_failure(c, frame.at(0x40), frame.at(0x38), 0x1f33, {0x103eec88, 0x103eec8c, 0x103eeca8, 0x103eecb8});
        c.put8(c.toc(-0x7bac), 0);
    }
    c.put8(out_kind, 0x32);
    return 1;
}

void settle_contract_fee_offer(Cpu& c, std::uint32_t manager, std::uint32_t slot) {
    constexpr std::uint32_t row_date = 0x30, row_stamp = 0x32, row_day = 0x34, row_share = 0xc, contract_share = 0xc;
    constexpr std::uint32_t object_at = 0xa0, answer_at = 0xf4, date_out = 0x48;
    Frame frame(c, 0x103e5810, 0x180, {.first_gpr = 22});
    const auto text = c.toc(-0x4b9c);
    keep(c, 25, slot);
    keep(c, 28, c.toc(-0x7b80));
    keep(c, 24, manager);
    keep(c, 29, c.toc(toc::contracts));
    keep(c, 30, c.toc(-0x7bac));
    keep(c, 22, c.toc(-0x7ba8));
    keep(c, 23, text);
    const auto offer = c.u32(slot);
    const auto person = flow::person_of(c, c.u32(offer + flow::offer_at::person));
    const auto club = flow::club_of(c, c.u32(offer + flow::offer_at::club));
    keep(c, 27, person);
    keep(c, 26, club);
    // Without a row or a contract the failure is reported and the offer is closed (stage 16).
    const auto close_offer = [&](std::uint32_t line, const flow::ReportSites& sites, std::uint32_t name_at,
                                 std::uint32_t extension_at, std::uint32_t back) {
        flow::report_failure(c, name_at, extension_at, line, sites);
        c.put8(c.toc(-0x7bac), 0);
        c.put8(c.u32(slot) + flow::offer_at::stage, 0x10);
        invoke(c, 0x103eb4a0, back, {manager, c.u32(slot), 1, 0});
    };
    const auto row = invoke(c, 0x103ecb80, 0x103e5874, {manager, person, club});
    keep(c, 31, row);
    if (!row) return close_offer(0xb11, {0x103e5894, 0x103e5898, 0x103e58b4, 0x103e58c4}, frame.at(0x80), frame.at(0x58), 0x103e58ec);
    if (!invoke(c, 0x10082f20, 0x103e58fc, {c.u32(c.toc(toc::contracts)), person}))
        return close_offer(0xb1a, {0x103e591c, 0x103e5920, 0x103e593c, 0x103e594c}, frame.at(0x60), frame.at(0x50), 0x103e5974);
    const auto contract = invoke(c, 0x10082f20, 0x103e5984, {c.u32(c.toc(toc::contracts)), person});
    // The row's share of the contract's, as a byte rounded down to tenths.
    const auto share = std::int8_t(flow::div_s(std::int32_t(c.u32(row + row_share) * 100u), std::int32_t(c.u32(contract + contract_share))));
    std::int32_t percent = std::int8_t((share / 10) * 10);
    keep(c, 30, flow::word(percent));
    const auto budget = invoke(c, 0x10103510, 0x103e59c8, {c.u32(c.toc(-0x7ab8)), club});
    const auto affordable = std::int32_t(invoke(c, 0x100f2680, 0x103e59d4, {budget, person, 0xffffffff}));
    keep(c, 23, flow::word(affordable));
    keep(c, 25, 0x51eb851f);
    // Lower the percentage by a tenth until the club can pay it.
    while (std::int8_t(percent) > 0) {
        const auto whole = invoke(c, 0x10082f20, 0x103e59f0, {c.u32(c.toc(toc::contracts)), person});
        const auto part = std::int32_t(std::uint32_t(flow::s8(flow::word(percent))) * c.u32(whole + contract_share));
        if (part / 100 <= affordable) break;
        percent -= 10;
        keep(c, 30, flow::word(percent));
    }
    if (std::int8_t(percent) < 0) {
        percent = 0;
        keep(c, 30, 0);
    }
    // The row's date (halfwords at +0x30 and +0x32) against today's: renewed when it is before today or less than
    // 7 days away.
    const auto date = c.toc(-0x7b80);
    const auto today0 = c.u32(date), today1 = c.u32(date + 4);
    const std::uint32_t today_low = today0 & 0xffff, row_low = c.u16(row + row_stamp);
    std::uint32_t before_today;
    if (today_low > row_low) before_today = 1;
    else if (today_low < row_low) before_today = 0;
    else {
        const auto high = flow::s16(today0 >> 16), mixed = high ^ flow::s16(c.u16(row + row_date));
        before_today = (std::uint32_t(mixed >> 1) - std::uint32_t(mixed & high)) >> 31;
    }
    if (before_today || std::int32_t(invoke(c, 0x10718130, 0x103e5aa0, {row + row_date, today0, today1})) < 7) {
        invoke(c, 0x10717a10, 0x103e5ab8, {frame.at(date_out), date, 2});
        c.put16(row + row_stamp, c.u16(frame.at(date_out + 2)));
        c.put16(row + row_date, c.u16(frame.at(date_out)));
        c.put8(row + row_day, c.u8(frame.at(date_out + 4)));
    }
    const auto scale_high = c.u8(row + 0x1d), scale_low = c.u8(row + 0x1c);
    keep(c, 23, scale_high);
    keep(c, 22, scale_low);
    invoke(c, 0x10411e60, 0x103e5ae0, {frame.at(object_at)});
    invoke(c, 0x104163d0, 0x103e5af0, {frame.at(object_at), person, club});
    invoke(c, 0x103721f0, 0x103e5af8, {frame.at(answer_at)});
    invoke(c, 0x103723b0, 0x103e5b18, {frame.at(answer_at), person, club, row + row_date, flow::word(percent), scale_low, scale_high});
    invoke_native(c, 0x103e25d0, 0x103e5b28, {manager, frame.at(object_at), frame.at(answer_at)});
    invoke(c, 0x1007f200, 0x103e5b4c,
           {c.u32(c.toc(toc::contracts)), frame.at(answer_at), c.u32(frame.at(0xc8)), c.u32(frame.at(0xcc)), 0xb, 0, 1, 0});
    invoke(c, 0x10411f10, 0x103e5b58, {frame.at(object_at), 0xffffffff});
}
}
