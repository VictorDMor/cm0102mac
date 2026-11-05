#include "staff_contracts.hpp"
#include "staff_contracts_detail.hpp"
#include "guest_fpu.hpp"
#include "contract_storage_adapter.hpp"
#include <utility>

namespace cm::aot::staff_contracts {
namespace {
using detail::ContractRecord;
using guest::Cpu;

constexpr auto initial_dates = 0x10372ab0u, initialize_return = 0x10374558u;
constexpr unsigned frame_size = 0x100;
constexpr auto double_header = 0x43300000u;
constexpr unsigned registered_club = 0x24, linked_club = 0x20, represented_league = 0xc;
constexpr unsigned profile_field = 0x14, nation_field = 0x1c;

struct InitialCpu : Cpu {
    SemanticGuestObserver* const observer;
    explicit InitialCpu(Cpu& c, SemanticGuestObserver* trace) : Cpu(c.s), observer(trace) {}
};

void block(InitialCpu& c, std::uint32_t pc, unsigned instructions) {
    if (c.observer) [[unlikely]] c.observer->before_step(c.s, pc);
    detail::block(c, pc, instructions);
}

struct InitialContract {
    InitialCpu& c;
    guest::Fpu fp;
    const guest::Kept<31> contract;
    const guest::Kept<30> person;
    guest::Kept<24> valuation;
    guest::Kept<28> club;
    guest::Kept<27> band;

    InitialContract(InitialCpu& cpu, std::uint32_t draft, std::uint32_t staff, std::uint32_t value)
        : c(cpu), fp(cpu), contract(cpu, draft), person(cpu, staff), valuation(cpu, value),
          club(cpu, cpu.r(28)), band(cpu, cpu.r(27)) {}

    double number(unsigned offset) const { return c.f64(c.r(29) + offset); }
    double single(unsigned offset) const { return c.f32(c.r(16) + offset); }
};

void open_initial_contract_frame(InitialCpu& c, GuestNativeScope& scope) {
    block(c, initialize_entry, 20);
    const auto stack = c.sp(), caller = c.lr();
    c.set(0, caller);
    c.put64(stack - 8, c.s.r<8>(guest::reg::fpr(31)));
    detail::transfer_saved_registers<13>(c, stack - 16);
    c.set(31, c.r(3));
    c.set(29, c.toc(-0x4dac));
    c.set(30, c.r(4));
    c.set(13, c.toc(-0x7b5c));
    c.set(24, c.r(5));
    c.set(25, c.toc(-0x7b90));
    c.set(17, 1);
    c.set(20, c.toc(-0x7b8c));
    c.set(19, c.toc(-0x7b88));
    c.set(26, c.toc(-0x7b84));
    c.set(14, c.toc(-0x7a9c));
    c.set(16, c.toc(-0x4da8));
    c.set(18, c.toc(-0x7b80));
    c.put32(stack + 8, caller);
    const auto frame = stack - frame_size;
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    c.set(3, frame + 0x64);
    c.call(0x10715880, 0x10372810);
}

void initialize_contract_identity(InitialContract& draft) {
    InitialCpu& c = draft.c;
    const auto frame = c.sp();
    block(c, 0x10372810, 6);
    c.set({{3, frame + 0x5c}, {4, 10}, {5, 6}, {6, 2000}, {7, UINT32_MAX}});
    c.call(0x107156f0, 0x10372828);
    block(c, 0x10372828, 19);
    const auto packed = c.u8(draft.contract + ContractRecord::packed_kind);
    c.set({{3, packed & 0xfeu}, {6, 0}, {0, UINT32_MAX}});
    c.put8(draft.contract + ContractRecord::packed_kind, std::uint8_t(c.r(3)));
    c.set({{5, 2}, {3, draft.contract}, {4, draft.person}});
    c.put8(draft.contract + ContractRecord::terms_flag, 0);
    c.put32(draft.contract + ContractRecord::terms_marker, UINT32_MAX);
    const auto category = (c.u8(draft.contract + 0x60) & 0xf0u) | 2;
    c.set(0, category);
    c.put8(draft.contract + 0x60, std::uint8_t(category));
    c.put16(draft.contract + ContractRecord::review_marker, 0);
    c.put32(draft.contract + ContractRecord::draft_auxiliary, 0);
    c.put8(draft.contract + 0x40, 0);
    c.put32(draft.contract + ContractRecord::draft_auxiliary, 0);
    const auto club = c.u32(draft.person + registered_club);
    c.set(5, club);
    detail::call_owned(c, 0x1037f050, 0x10372874, reset_review_state,
                       std::uint32_t(draft.contract), std::uint32_t(draft.person), club);
}

void find_initial_contract_club(InitialContract& draft) {
    InitialCpu& c = draft.c;
    block(c, 0x10372874, 3);
    draft.club = c.u32(draft.person + registered_club);
    if (!guest::nonzero_logical(c, draft.club)) return;
    block(c, 0x10372880, 4);
    c.set({{3, draft.club}, {4, c.sp() + 0x38}, {5, 1}});
    c.call(0x100ce320, 0x10372890);
    block(c, 0x10372890, 2);
    const auto resolved = c.r(3);
    if (!guest::nonzero_logical(c, resolved)) return;
    block(c, 0x10372898, 3);
    if (!detail::record_nonzero(c, c.i8(c.sp() + 0x38))) {
        block(c, 0x103728a4, 1);
        draft.club = resolved;
    }
}

void load_initial_valuation(InitialContract& draft) {
    InitialCpu& c = draft.c;
    block(c, 0x103728a8, 2);
    if (guest::compare(c, std::int32_t(std::uint32_t(draft.valuation))) >= 0) return;
    block(c, 0x103728b0, 9);
    const auto manager = c.toc(-0x7b7c);
    c.set({{3, manager}, {4, draft.person}, {5, draft.club}, {17, 0}});
    c.set(3, c.u32(manager));
    c.set({{6, 0}, {7, UINT32_MAX}, {8, 0}});
    c.call(0x10084be0, 0x103728d4);
    block(c, 0x103728d4, 1);
    draft.valuation = c.r(3);
}

bool finish_without_initial_club(InitialContract& draft, std::uint32_t site) {
    InitialCpu& c = draft.c;
    block(c, site, 5);
    const auto packed = c.u8(draft.contract + ContractRecord::packed_kind);
    c.set({{0, (packed & 3u) | 12u}, {3, 3}});
    c.put8(draft.contract + ContractRecord::packed_kind, std::uint8_t(c.r(0)));
    return false;
}

void seed_initial_band(InitialContract& draft, std::int16_t reputation) {
    InitialCpu& c = draft.c;
    const auto high = (INT64_C(0x10624dd3) * reputation) >> 32;
    c.s.wr<1>(guest::reg::xer_ca, high < 0 && (high & 31));
    const auto units = reputation / 500;
    c.set({{3, std::uint32_t(units)}, {0, std::uint32_t(units + 1)}});
    draft.band = std::uint32_t(std::int8_t(units + 1));
}

bool choose_initial_band(InitialContract& draft) {
    InitialCpu& c = draft.c;
    block(c, 0x103728d8, 2);
    if (!guest::nonzero_logical(c, draft.club)) {
        block(c, 0x103728e0, 3);
        const auto related = c.u32(draft.person + linked_club);
        c.set(0, related);
        if (!guest::nonzero_logical(c, related)) return finish_without_initial_club(draft, 0x103728ec);
    }
    block(c, 0x10372900, 3);
    const auto has_club = guest::nonzero_logical(c, draft.club);
    c.set(21, c.u32(draft.person + profile_field));
    if (has_club) {
        block(c, 0x1037290c, 10);
        c.set(3, 0x10620000);
        seed_initial_band(draft, c.i16(draft.club + 0x1ec));
        // The preceding club-null branch already guarantees this CR0 condition.
        block(c, 0x10372934, 3);
        const auto club_type = c.u8(draft.club + 0x23d);
        c.set(0, club_type);
        if (guest::compare(c, club_type, 2) != 0) return true;
        block(c, 0x10372940, 4);
        draft.band = std::uint32_t(draft.band) - 1;
        auto lowered = std::int8_t(std::uint32_t(draft.band));
        c.set(0, std::uint32_t(lowered));
        if (guest::compare(c, lowered, 1) < 0) {
            block(c, 0x10372950, 1);
            draft.band = 1;
            lowered = 1;
        }
        block(c, 0x10372954, 3);
        c.set(0, std::uint32_t(lowered));
        if (guest::compare(c, lowered, 4) > 0) {
            block(c, 0x10372960, 2);
            draft.band = 4;
        }
        return true;
    }
    block(c, 0x10372968, 3);
    const auto nation = c.u32(draft.person + nation_field);
    c.set(0, nation);
    if (!guest::nonzero_logical(c, nation)) return finish_without_initial_club(draft, 0x10372974);
    block(c, 0x10372988, 12);
    const auto associated = c.u32(draft.person + linked_club);
    c.set({{4, associated}, {3, 0x10624dd3}});
    seed_initial_band(draft, c.i16(associated + 0x1ec));
    if (guest::compare(c, std::int32_t(std::uint32_t(draft.band)), 3) < 0) {
        block(c, 0x103729b8, 1);
        draft.band = 3;
    }
    return true;
}

void assign_initial_club_id(InitialContract& draft) {
    InitialCpu& c = draft.c;
    block(c, 0x103729bc, 2);
    if (guest::nonzero_logical(c, draft.club)) {
        block(c, 0x103729c4, 3);
        const auto id = c.u32(draft.club);
        c.set(0, id);
        c.put32(draft.contract + ContractRecord::club_id, id);
        return;
    }
    block(c, 0x103729d0, 3);
    const auto associated = c.u32(draft.person + linked_club);
    c.set(3, associated);
    if (guest::nonzero_logical(c, associated)) {
        block(c, 0x103729dc, 3);
        const auto id = c.u32(associated);
        c.set(0, id);
        c.put32(draft.contract + ContractRecord::club_id, id);
    } else {
        block(c, 0x103729e8, 2);
        c.set(0, UINT32_MAX);
        c.put32(draft.contract + ContractRecord::club_id, UINT32_MAX);
    }
}

void adjust_initial_band(InitialContract& draft) {
    InitialCpu& c = draft.c;
    block(c, 0x103729f0, 3);
    const auto experience = c.u8(draft.person + 0x5e);
    c.set(4, experience);
    if (!guest::nonzero_logical(c, experience)) return;
    block(c, 0x103729fc, 3);
    const auto league = c.u32(draft.person + represented_league);
    c.set(3, league);
    if (!guest::nonzero_logical(c, league)) {
        block(c, 0x10372a64, 1);
        c.set(3, 1);
    } else {
        block(c, 0x10372a08, 23);
        const auto raw_level = c.u8(league + 0x11f);
        const auto level = std::int8_t(raw_level);
        const auto frame = c.sp();
        c.set({{3, raw_level}, {0, double_header}});
        c.put32(frame + 0x94, double_header);
        c.set(3, std::uint32_t(level));
        c.set_f(4, draft.number(0xc0));
        const auto encoded_level = std::uint32_t(level) ^ 0x80000000;
        c.set(3, encoded_level);
        c.put32(frame + 0x90, experience);
        c.set_f(2, draft.number(0xc8));
        c.put32(frame + 0x98, encoded_level);
        c.set_f(3, draft.single(0));
        c.put32(frame + 0x8c, double_header);
        c.set_f(0, c.f64(frame + 0x94));
        c.set_f(1, c.f64(frame + 0x8c));
        c.set_f(4, draft.fp.fsubs(c.f(0), c.f(4)));
        c.set_f(0, draft.single(4));
        c.set_f(1, draft.fp.fsubs(c.f(1), c.f(2)));
        c.set_f(2, draft.fp.fmuls(c.f(4), c.f(3)));
        c.set_f(0, draft.fp.fmuls(c.f(1), c.f(0)));
        c.set_f(0, draft.fp.fmuls(c.f(2), c.f(0)));
        c.put64(frame + 0x84, draft.fp.fctiwz(0, c.f(0)));
        c.set(3, c.u32(frame + 0x88));
    }
    block(c, 0x10372a68, 3);
    const auto adjustment = std::int8_t(c.r(3));
    c.set(0, std::uint32_t(adjustment));
    if (guest::compare(c, adjustment, 4) > 0) {
        block(c, 0x10372a74, 2);
        c.set(3, 4);
    } else {
        block(c, 0x10372a7c, 2);
        if (!detail::record_nonzero(c, adjustment)) {
            block(c, 0x10372a84, 1);
            c.set(3, 1);
        }
    }
    block(c, 0x10372a88, 1);
    draft.band = std::uint32_t(draft.band) + c.r(3);
}

void read_initial_reputation(InitialContract& draft) {
    InitialCpu& c = draft.c;
    block(c, 0x10372a8c, 3);
    const auto profile = c.u32(draft.person + profile_field);
    c.set(3, profile);
    if (guest::nonzero_logical(c, profile)) {
        block(c, 0x10372a98, 2);
        c.set(23, guest::extended(c.i16(profile + 10)));
        return;
    }
    block(c, 0x10372aa0, 3);
    const auto nation = c.u32(draft.person + nation_field);
    c.set(3, nation);
    if (guest::nonzero_logical(c, nation)) {
        block(c, 0x10372aac, 1);
        c.set(23, guest::extended(c.i16(nation + 0x2a)));
    }
}

bool prepare_initial_contract(InitialContract& draft) {
    initialize_contract_identity(draft);
    find_initial_contract_club(draft);
    load_initial_valuation(draft);
    if (!choose_initial_band(draft)) return false;
    assign_initial_club_id(draft);
    adjust_initial_band(draft);
    read_initial_reputation(draft);
    return true;
}

bool use_existing_contract_value(InitialContract& draft) {
    InitialCpu& c = draft.c;
    block(c, initial_dates, 2);
    if (!detail::record_nonzero(c, std::int8_t(c.r(17)))) return false;
    block(c, 0x10372ab8, 3);
    const auto existing_value = c.i32(draft.person + 4);
    c.set(3, std::uint32_t(existing_value));
    if (!guest::nonzero(c, existing_value)) return false;
    block(c, 0x10372ac4, 4);
    const auto ceiling = std::int16_t(c.r(23)) * 15;
    c.set(0, std::uint32_t(ceiling));
    return guest::compare(c, existing_value, ceiling) <= 0;
}

void copy_initial_contract_date(Cpu& c, std::uint32_t source, std::uint32_t destination) {
    const auto year = c.u16(source + 2);
    c.set(0, year);
    c.put16(destination + 2, year);
    const auto day = c.i16(source);
    c.set(0, guest::extended(day));
    c.put16(destination, std::uint16_t(day));
    const auto flags = c.u8(source + 4);
    c.set(0, flags);
    c.put8(destination + 4, flags);
}

void adjust_existing_contract_year(InitialContract& draft) {
    InitialCpu& c = draft.c;
    const auto year = std::uint32_t(draft.contract) + ContractRecord::start_date + 2;
    const auto reported_years = [&](std::uint32_t pc, unsigned limit) {
        block(c, pc, 3);
        const auto count = c.u16(c.sp() + 0x66);
        c.set(0, count);
        return guest::compare_logical(c, count, limit) > 0;
    };
    const auto use_reference_year = [&](std::uint32_t pc) {
        block(c, pc, 3);
        const auto replacement = c.u16(c.r(14) + 2);
        c.set(0, replacement);
        c.put16(year, replacement);
    };
    if ((c.cr(0) & 4) && reported_years(0x10372b6c, 1)) {
        use_reference_year(0x10372b78);
        return;
    }
    block(c, 0x10372b84, 3);
    c.set_f(0, draft.single(12));
    draft.fp.fcmp(0, c.f(31), c.f(0));
    if (c.cr(0) & 4) {
        block(c, 0x10372b90, 3);
        const auto profile = c.u32(draft.person + profile_field);
        c.set(3, profile);
        if (guest::nonzero_logical(c, profile)) {
            block(c, 0x10372b9c, 3);
            const auto merit = c.u8(profile + 0x11);
            c.set(0, merit);
            if (guest::compare(c, merit, 20) == 0) {
                use_reference_year(0x10372ba8);
                return;
            }
        }
    }

    block(c, 0x10372bb4, 4);
    const auto age = std::int8_t(c.i16(draft.person + 0x5c));
    c.set(0, std::uint32_t(age));
    auto within_prime = false;
    if (guest::compare(c, age, 24) >= 0) {
        block(c, 0x10372bc4, 2);
        within_prime = guest::compare(c, age, 29) <= 0;
    }
    const auto move_start = [&](std::uint32_t pc, unsigned offset, bool jump) {
        block(c, pc, jump ? 4 : 3);
        const auto current = c.u16(c.r(18) + 2);
        c.set({{3, current}, {0, current - offset}});
        c.put16(year, std::uint16_t(current - offset));
    };
    if (!within_prime && reported_years(0x10372bcc, 2)) {
        move_start(0x10372bd8, 2, true);
        return;
    }
    if (reported_years(0x10372be8, 3)) move_start(0x10372bf4, 3, false);
}

void preserve_existing_contract_dates(InitialContract& draft) {
    InitialCpu& c = draft.c;
    const auto frame = c.sp();
    block(c, 0x10372ad4, 18);
    const auto encoded = c.r(3) ^ 0x80000000;
    c.set({{0, encoded}, {4, double_header}});
    c.put32(frame + 0x88, encoded);
    c.set(3, 2);
    const auto packed = (c.u8(draft.contract + ContractRecord::packed_kind) & 3u) | 8u;
    c.set(0, packed);
    c.put32(frame + 0x84, double_header);
    c.set(3, c.r(18));
    c.set_f(1, draft.number(0xc0));
    c.set_f(0, c.f64(frame + 0x84));
    c.put8(draft.contract + ContractRecord::packed_kind, std::uint8_t(packed));
    c.set_f(31, draft.fp.fsubs(c.f(0), c.f(1)));
    c.set(4, c.u32(draft.person + 0x4c));
    c.set(5, c.u32(draft.person + 0x50));
    original_fctiwz(c.s, guest::reg::fpr(0), guest::reg::fpr(31));
    c.put64(frame + 0x8c, c.s.r<8>(guest::reg::fpr(0)));
    c.set(22, c.u32(frame + 0x90));
    c.call(0x10718130, 0x10372b1c);

    block(c, 0x10372b1c, 5);
    c.set(5, c.u32(draft.person + 0x4c));
    c.set(4, c.r(18));
    c.set(6, c.u32(draft.person + 0x50));
    c.set(3, frame + 0x54);
    c.call(0x10717dd0, 0x10372b30);

    block(c, 0x10372b30, 15);
    const auto flags = c.u8(frame + 0x58);
    c.set(0, flags);
    const auto years = c.u16(frame + 0x56);
    c.set(4, years);
    const auto days = c.i16(frame + 0x54);
    c.set(3, guest::extended(days));
    c.put16(frame + 0x66, years);
    c.set_f(0, draft.single(8));
    c.put16(frame + 0x64, std::uint16_t(days));
    draft.fp.fcmp(0, c.f(31), c.f(0));
    c.put8(frame + 0x68, flags);
    copy_initial_contract_date(c, draft.person + 0x4c, draft.contract + ContractRecord::start_date);
    adjust_existing_contract_year(draft);
    block(c, 0x10372c00, 7);
    copy_initial_contract_date(c, draft.person + 0x54, draft.contract + ContractRecord::end_date);
}

bool date_precedes_current_year(InitialContract& draft, std::uint32_t source, std::uint32_t pc,
                                unsigned result, unsigned other_day, unsigned scratch) {
    InitialCpu& c = draft.c;
    const auto year_order = c.cr(0);
    if (year_order & 8) {
        block(c, pc, 2);
        c.set(result, 1);
        return true;
    }
    block(c, pc + 8, 1);
    if (year_order & 4) {
        block(c, pc + 12, 2);
        c.set(result, 0);
        return false;
    }
    block(c, pc + 20, 8);
    const auto day = c.i16(source);
    c.set(result, guest::extended(day));
    const auto today = c.i16(c.r(18));
    const auto different = std::int32_t(day ^ today);
    c.s.wr<1>(guest::reg::xer_ca, different < 0 && (different & 1));
    const auto earlier = day < today;
    c.set({{other_day, guest::extended(today)}, {scratch, std::uint32_t(different >> 1)}, {result, unsigned(earlier)}});
    return earlier;
}

bool start_from_registered_date(InitialContract& draft) {
    InitialCpu& c = draft.c;
    const auto frame = c.sp();
    block(c, 0x10372c1c, 8);
    c.set({{3, draft.contract}, {4, draft.person}, {5, draft.club}, {6, 0}, {7, UINT32_MAX}, {8, 11}, {9, 0}});
    c.call(reference_quote_entry, 0x10372c3c);

    block(c, 0x10372c3c, 13);
    const auto fallback = c.r(14);
    const auto first = c.u32(fallback);
    c.set(0, first);
    original_fctiwz(c.s, guest::reg::fpr(0), guest::reg::fpr(1));
    const auto year_address = c.r(18) + 2;
    c.set(15, year_address);
    const auto second = c.u32(fallback + 4);
    c.set(3, second);
    c.put32(frame + 0x44, first);
    const auto current_year = c.u16(year_address);
    c.set(0, current_year);
    const auto fallback_year = c.u16(frame + 0x46);
    c.set(4, fallback_year);
    c.put64(frame + 0x84, c.s.r<8>(guest::reg::fpr(0)));
    guest::compare_logical(c, fallback_year, current_year);
    c.set_f(31, draft.single(16));
    c.put32(frame + 0x48, second);
    c.set(22, c.u32(frame + 0x88));
    const auto past_fallback = date_precedes_current_year(draft, frame + 0x44, 0x10372c70, 3, 5, 4);

    block(c, 0x10372ca4, 2);
    if (detail::record_nonzero(c, past_fallback, 3)) return false;
    block(c, 0x10372cac, 2);
    if (guest::nonzero_logical(c, draft.club)) return true;
    block(c, 0x10372cb4, 3);
    const auto associated = c.u32(draft.person + linked_club);
    c.set(3, associated);
    return guest::nonzero_logical(c, associated);
}

bool had_earlier_start_date(InitialContract& draft) {
    InitialCpu& c = draft.c;
    const auto frame = c.sp();
    const auto current_year = c.r(0);
    block(c, 0x10372cc0, 7);
    const auto first = c.u32(draft.person + 0x4c);
    c.set(4, first);
    const auto second = c.u32(draft.person + 0x50);
    c.set(3, second);
    c.put32(frame + 0x3c, first);
    const auto previous_year = c.u16(frame + 0x3e);
    c.set(4, previous_year);
    c.put32(frame + 0x40, second);
    guest::compare_logical(c, previous_year, current_year);
    return date_precedes_current_year(draft, frame + 0x3c, 0x10372cdc, 0, 4, 3);
}

void keep_initial_start_date(InitialContract& draft) {
    InitialCpu& c = draft.c;
    const auto frame = c.sp();
    block(c, 0x10372d18, 11);
    const auto year = c.u16(draft.person + 0x4e);
    c.set({{0, year}, {4, c.r(18)}, {3, frame + 0x4c}});
    const auto date = draft.contract + ContractRecord::start_date;
    c.put16(date + 2, year);
    const auto day = c.i16(draft.person + 0x4c);
    c.set(0, guest::extended(day));
    c.put16(date, std::uint16_t(day));
    const auto flags = c.u8(draft.person + 0x50);
    c.set(0, flags);
    c.put8(date + 4, flags);
    c.set(5, c.u32(date));
    c.set(6, c.u32(date + 4));
    c.call(0x10717dd0, 0x10372d44);

    block(c, 0x10372d44, 9);
    const auto years = c.u16(frame + 0x4e);
    c.set(0, years);
    const auto delta_flags = c.u8(frame + 0x50);
    c.set(3, delta_flags);
    c.put16(frame + 0x66, years);
    const auto days = c.i16(frame + 0x4c);
    c.set(4, guest::extended(days));
    const auto stored_years = c.u16(frame + 0x66);
    c.set(0, stored_years);
    c.put16(frame + 0x64, std::uint16_t(days));
    const auto long_elapsed = guest::compare_logical(c, stored_years, 2) > 0;
    c.put8(frame + 0x68, delta_flags);
    if (long_elapsed) {
        block(c, 0x10372d68, 4);
        const auto current = c.u16(c.r(15));
        c.set({{3, current}, {0, std::uint32_t(current) - 1}});
        c.put16(date + 2, std::uint16_t(current - 1));
    }
}

void set_fallback_contract_dates(InitialContract& draft) {
    InitialCpu& c = draft.c;
    const auto start = draft.contract + ContractRecord::start_date;
    const auto end = draft.contract + ContractRecord::end_date;
    block(c, 0x10372dbc, 17);
    const auto packed = c.u8(draft.contract + ContractRecord::packed_kind);
    c.set({{3, (packed & 3u) | 8u}, {0, 2}});
    c.put8(draft.contract + ContractRecord::packed_kind, std::uint8_t(c.r(3)));
    c.set(0, 0);
    const auto base_year = c.u16(c.r(15));
    c.set(3, base_year);
    c.put16(start + 2, base_year);
    const auto day = c.i16(c.r(18));
    c.set(3, guest::extended(day));
    c.put16(start, std::uint16_t(day));
    const auto flags = c.u8(c.r(18) + 4);
    c.set(3, flags);
    c.put8(start + 4, flags);
    const auto renewed = c.u16(c.r(15)) + 1u;
    c.set(3, renewed);
    c.put16(end + 2, std::uint16_t(renewed));
    const auto renewed_day = c.i16(c.r(18));
    c.set(3, guest::extended(renewed_day));
    c.put16(end, std::uint16_t(renewed_day));
    c.put32(draft.contract + 0x3c, 0);
}

void set_prospective_contract_dates(InitialContract& draft) {
    InitialCpu& c = draft.c;
    if (start_from_registered_date(draft)) {
        const auto earlier = had_earlier_start_date(draft);
        block(c, 0x10372d10, 2);
        if (detail::record_nonzero(c, earlier)) keep_initial_start_date(draft);
        else {
            block(c, 0x10372d78, 6);
            copy_initial_contract_date(c, c.sp() + 0x5c, draft.contract + ContractRecord::start_date);
        }
        block(c, 0x10372d90, 11);
        const auto packed = c.u8(draft.contract + ContractRecord::packed_kind);
        c.set({{0, (packed & 3u) | 8u}, {3, 2}});
        c.put8(draft.contract + ContractRecord::packed_kind, std::uint8_t(c.r(0)));
        copy_initial_contract_date(c, draft.person + 0x54, draft.contract + ContractRecord::end_date);
    } else set_fallback_contract_dates(draft);

    block(c, 0x10372e00, 2);
    c.set(0, 0);
    c.put32(draft.contract + ContractRecord::draft_auxiliary, 0);
}

void choose_initial_contract_dates(InitialContract& draft) {
    if (use_existing_contract_value(draft)) preserve_existing_contract_dates(draft);
    else set_prospective_contract_dates(draft);
}

bool initial_nation_matches(InitialContract& draft, std::uint32_t pc, int slot) {
    InitialCpu& c = draft.c;
    block(c, pc, 4);
    c.set(4, c.toc(slot));
    const auto nation = c.u32(c.r(4));
    c.set(4, nation);
    return guest::compare(c, std::int32_t(c.r(0)), std::int32_t(nation)) == 0;
}

void enforce_initial_wage_floor(InitialContract& draft, std::uint32_t pc, unsigned factor) {
    InitialCpu& c = draft.c;
    const auto band = std::int8_t(std::uint32_t(draft.band));
    const auto minimum = std::uint32_t(band * band) * factor;
    const auto encoded = minimum ^ 0x80000000;
    const auto frame = c.sp();
    const bool keep_nation = pc == 0x10374304;
    const auto value_reg = keep_nation ? 5u : 3u;
    const auto header_reg = keep_nation ? 4u : 0u;
    const auto crossing_page = pc == 0x10373ff4;
    block(c, pc, crossing_page ? 3 : 12);
    c.set({{value_reg, std::uint32_t(band * band)}, {header_reg, double_header}});
    if (crossing_page) block(c, pc + 12, 9);
    c.put32(frame + 0x6c, double_header);
    c.set_f(1, draft.number(0xc0));
    c.set(value_reg, encoded);
    c.put32(frame + 0x70, encoded);
    c.set_f(0, c.f64(frame + 0x6c));
    c.set_f(0, draft.fp.fsubs(c.f(0), c.f(1)));
    draft.fp.fcmp(0, c.f(31), c.f(0));
    if (!(c.cr(0) & 8)) return;

    block(c, pc + 48, 4);
    if (pc == 0x10374468) {
        c.put32(frame + 0x70, encoded);
        c.put32(frame + 0x6c, double_header);
        c.set_f(0, c.f64(frame + 0x6c));
    } else {
        c.put32(frame + 0x78, encoded);
        c.put32(frame + 0x74, double_header);
        c.set_f(0, c.f64(frame + 0x74));
    }
    c.set_f(31, draft.fp.fsubs(c.f(0), c.f(1)));
}

void taper_initial_wage(InitialContract& draft, std::uint32_t pc, unsigned cap, unsigned fraction) {
    InitialCpu& c = draft.c;
    block(c, pc, 3);
    c.set_f(2, draft.number(cap));
    draft.fp.fcmp(0, c.f(31), c.f(2));
    if (!(c.cr(0) & 4)) return;
    block(c, pc + 12, 5);
    c.set_f(0, draft.fp.fsub(c.f(31), c.f(2)));
    c.set_f(1, draft.number(fraction));
    c.set_f(31, draft.fp.multiply_add(pc + 20, c.f(1), c.f(0), c.f(2)));
    c.set_f(31, draft.fp.frsp(c.f(31)));
}

void apply_initial_club_scale(InitialContract& draft, std::uint32_t pc, unsigned minimum,
                              unsigned standard_cap, unsigned special_cap, unsigned fraction) {
    InitialCpu& c = draft.c;
    enforce_initial_wage_floor(draft, pc, minimum);
    block(c, pc + 64, 3);
    const auto type = c.u8(draft.club + 0x23d);
    c.set(0, type);
    if (guest::compare(c, type, 1) == 0) taper_initial_wage(draft, pc + 108, special_cap, fraction);
    else taper_initial_wage(draft, pc + 76, standard_cap, fraction);
}

void initial_wage_from_club_resources(InitialContract& draft) {
    InitialCpu& c = draft.c;
    enforce_initial_wage_floor(draft, 0x10373e20, 5);
    block(c, 0x10373e60, 3);
    const auto resources = c.i32(draft.club + 8);
    c.set(0, std::uint32_t(resources));
    if (guest::compare(c, resources, 15000) < 0) {
        taper_initial_wage(draft, 0x10373e6c, 0x40, 0x48);
        return;
    }
    block(c, 0x10373e8c, 2);
    if (guest::compare(c, resources, 27500) < 0) taper_initial_wage(draft, 0x10373e94, 0x50, 0x48);
    else taper_initial_wage(draft, 0x10373eb4, 0x58, 0x48);
}

void initial_wage_for_competition(InitialContract& draft) {
    InitialCpu& c = draft.c;
    block(c, 0x1037434c, 3);
    const auto competition = c.u32(draft.club + 0x30);
    c.set(5, competition);
    if (guest::nonzero_logical(c, competition)) {
        block(c, 0x10374358, 5);
        const auto featured = c.toc(-0x6778);
        c.set(4, featured);
        const auto actual = c.u32(competition);
        c.set(5, actual);
        const auto expected = c.u32(featured);
        c.set(0, expected);
        if (guest::compare(c, std::int32_t(actual), std::int32_t(expected)) == 0) {
            taper_initial_wage(draft, 0x1037436c, 0x68, 0x78);
            return;
        }
    }
    block(c, 0x1037438c, 3);
    const auto league = c.u32(draft.person + represented_league);
    c.set(0, league);
    if (guest::compare_logical(c, league, c.r(3)) != 0) {
        taper_initial_wage(draft, 0x10374428, 0xb0, 0x48);
        return;
    }
    block(c, 0x10374398, 4);
    const auto age = std::int8_t(c.i16(draft.person + 0x5c));
    c.set(0, std::uint32_t(age));
    if (guest::compare(c, age, 31) > 0) {
        taper_initial_wage(draft, 0x103743a8, 0x98, 0xa0);
        return;
    }
    block(c, 0x103743c8, 2);
    if (guest::compare(c, age, 24) < 0) {
        block(c, 0x103743d0, 3);
        const auto profile = c.u32(draft.person + profile_field);
        c.set(3, profile);
        if (guest::nonzero_logical(c, profile)) {
            block(c, 0x103743dc, 3);
            const auto reputation = c.i16(profile + 12);
            c.set(0, guest::extended(reputation));
            if (guest::compare(c, reputation, 3750) < 0) {
                taper_initial_wage(draft, 0x103743e8, 0x98, 0xa0);
                return;
            }
        }
    }
    taper_initial_wage(draft, 0x10374408, 0xa8, 0xa0);
}

void apply_initial_nation_limits(InitialContract& draft) {
    InitialCpu& c = draft.c;
    block(c, 0x10373de4, 2);
    if (detail::record_nonzero(c, std::int8_t(c.r(17)))) return;
    block(c, 0x10373dec, 3);
    c.set_f(0, draft.single(16));
    draft.fp.fcmp(0, c.f(0), c.f(31));
    if (c.cr(0) & 2) return;
    block(c, 0x10373df8, 2);
    if (!guest::nonzero_logical(c, draft.club)) {
        enforce_initial_wage_floor(draft, 0x10374468, 10);
        return;
    }
    block(c, 0x10373e00, 3);
    const auto league = c.u32(draft.club + 0x2c);
    c.set(3, league);
    if (!guest::nonzero_logical(c, league)) {
        enforce_initial_wage_floor(draft, 0x10374468, 10);
        return;
    }

    block(c, 0x10373e0c, 5);
    c.set(4, c.toc(-0x7a1c));
    const auto nation = c.u32(league + 0x68);
    c.set(0, nation);
    const auto first_nation = c.u32(c.r(4));
    c.set(4, first_nation);
    if (guest::compare(c, std::int32_t(nation), std::int32_t(first_nation)) == 0) {
        initial_wage_from_club_resources(draft);
        return;
    }
    if (initial_nation_matches(draft, 0x10373ed4, -0x7590)) {
        enforce_initial_wage_floor(draft, 0x10373ee4, 10);
        taper_initial_wage(draft, 0x10373f24, 0x58, 0x48);
        return;
    }
    if (initial_nation_matches(draft, 0x10373f44, -0x7af4) || initial_nation_matches(draft, 0x10373f54, -0x7a00)
        || initial_nation_matches(draft, 0x10373f64, -0x7a28)) {
        enforce_initial_wage_floor(draft, 0x10373f74, 10);
        taper_initial_wage(draft, 0x10373fb4, 0x60, 0x48);
        return;
    }
    if (initial_nation_matches(draft, 0x10373fd4, -0x7514) || initial_nation_matches(draft, 0x10373fe4, -0x7650)) {
        apply_initial_club_scale(draft, 0x10373ff4, 4, 0x68, 0x40, 0x48);
        return;
    }
    if (initial_nation_matches(draft, 0x10374080, -0x757c) || initial_nation_matches(draft, 0x10374090, -0x7594)
        || initial_nation_matches(draft, 0x103740a0, -0x7574) || initial_nation_matches(draft, 0x103740b0, -0x7578)) {
        apply_initial_club_scale(draft, 0x103740c0, 5, 0x70, 0x80, 0x78);
        return;
    }
    if (initial_nation_matches(draft, 0x1037414c, -0x750c) || initial_nation_matches(draft, 0x1037415c, -0x75a8)
        || initial_nation_matches(draft, 0x1037416c, -0x7504)) {
        apply_initial_club_scale(draft, 0x1037417c, 5, 0x68, 0x90, 0x88);
        return;
    }
    if (initial_nation_matches(draft, 0x10374208, -0x7510) || initial_nation_matches(draft, 0x10374218, -0x79fc)) {
        apply_initial_club_scale(draft, 0x10374228, 5, 0x68, 0x40, 0x78);
        return;
    }
    if (initial_nation_matches(draft, 0x103742b4, -0x7a24) || initial_nation_matches(draft, 0x103742c4, -0x7a10)
        || initial_nation_matches(draft, 0x103742d4, -0x7a14) || initial_nation_matches(draft, 0x103742e4, -0x7a18))
        return;
    block(c, 0x103742f4, 4);
    c.set(4, c.toc(-0x7a20));
    const auto special = c.u32(c.r(4));
    c.set(6, special);
    if (guest::compare(c, std::int32_t(nation), std::int32_t(special)) == 0) return;
    enforce_initial_wage_floor(draft, 0x10374304, 10);
    block(c, 0x10374344, 2);
    // The floor preserves both operands. Keep the compare for live CR0.
    if (guest::compare(c, std::int32_t(c.r(0)), std::int32_t(c.r(6))) != 0)
        taper_initial_wage(draft, 0x10374448, 0xb8, 0x48);
    else initial_wage_for_competition(draft);
}

void initial_option(InitialContract& draft, std::uint32_t pc, std::int8_t option, bool enabled) {
    InitialCpu& c = draft.c;
    block(c, pc, 3);
    c.set({{3, draft.contract}, {4, std::uint32_t(option)}});
    if (enabled)
        detail::call_owned(c, 0x10374af0, pc + 12, enable_option, std::uint32_t(draft.contract), option);
    else
        detail::call_owned(c, 0x10374b60, pc + 12, clear_option, std::uint32_t(draft.contract), option);
}

void initial_bonus(InitialContract& draft, std::uint32_t pc, std::int8_t index, bool enabled, std::uint32_t amount = 0) {
    InitialCpu& c = draft.c;
    if (enabled) {
        detail::call_owned(c, 0x10374b40, pc, set_bonus, std::uint32_t(draft.contract), index, amount);
    } else {
        block(c, pc, 3);
        c.set({{3, draft.contract}, {4, std::uint32_t(index)}});
        detail::call_owned(c, 0x10374b80, pc + 12, clear_bonus, std::uint32_t(draft.contract), index);
    }
}

void initial_release_amount(InitialContract& draft, std::uint32_t pc, std::uint32_t value = 0) {
    InitialCpu& c = draft.c;
    block(c, pc, 3);
    c.set({{3, draft.contract}, {4, value}});
    detail::call_owned(c, 0x10380700, pc + 12, set_release_amount, std::uint32_t(draft.contract), value);
}

void clear_initial_clauses(InitialContract& draft, std::uint32_t pc) {
    for (std::int8_t option = 0; option < 5; ++option, pc += 12) initial_option(draft, pc, option, false);
    for (std::int8_t bonus = 0; bonus < 3; ++bonus, pc += 12) initial_bonus(draft, pc, bonus, false);
    initial_release_amount(draft, pc);
}

bool prepare_initial_options(InitialContract& draft) {
    InitialCpu& c = draft.c;
    initial_release_amount(draft, 0x10372e08);
    initial_option(draft, 0x10372e14, 4, false);
    initial_option(draft, 0x10372e20, 1, false);
    block(c, 0x10372e2c, 3);
    const auto profile = c.u32(draft.person + profile_field);
    c.set(0, profile);
    if (!guest::nonzero_logical(c, profile)) return false;
    initial_option(draft, 0x10372e38, 4, false);
    if (detail::qualifies_for_veteran_option(c, draft.person, 0x10372e44, 23)) {
        initial_option(draft, 0x10372ed8, 4, true);
        block(c, 0x10372ee4, 1);
    } else {
        block(c, 0x10372ee8, 3);
        const auto nation = c.u32(draft.person + nation_field);
        c.set(0, nation);
        if (guest::nonzero_logical(c, nation)) initial_option(draft, 0x10372ef4, 4, true);
    }
    return true;
}

void clear_initial_options_without_profile(InitialContract& draft) {
    InitialCpu& c = draft.c;
    block(c, 0x10373d6c, 3);
    const auto profile = c.u32(draft.person + profile_field);
    c.set(0, profile);
    if (!guest::nonzero_logical(c, profile)) clear_initial_clauses(draft, 0x10373d78);
}

void finish_initial_contract(InitialContract& draft) {
    InitialCpu& c = draft.c;
    block(c, 0x103744a8, 2);
    if (guest::nonzero_logical(c, draft.club)) {
        block(c, 0x103744b0, 4);
        const auto type = c.i8(draft.club + 0x23d);
        c.set(0, std::uint32_t(type));
        if (guest::compare(c, type, 2) == 0) {
            block(c, 0x103744c0, 5);
            const auto packed = c.u8(draft.contract + ContractRecord::packed_kind) | 1u;
            c.set({{0, packed}, {3, 1}});
            c.put8(draft.contract + ContractRecord::packed_kind, std::uint8_t(packed));
        } else {
            block(c, 0x103744d4, 2);
            if (guest::compare(c, type, 3) == 0) clear_initial_clauses(draft, 0x103744dc);
        }
    }
    // The ordinary exit shares one charged run with the frame restore.
    block(c, 0x10374548, 10);
    const auto scratch = c.sp() + 0x6c;
    original_fctiwz(c.s, guest::reg::fpr(0), guest::reg::fpr(31));
    c.put64(scratch, c.s.r<8>(guest::reg::fpr(0)));
    const auto final_wage = c.u32(scratch + 4);
    c.set(0, final_wage);
    c.put32(draft.contract + 12, final_wage);
}

void restore_initial_contract_frame(InitialCpu& c) {
    const auto stack = c.sp() + frame_size;
    const auto caller = c.u32(stack + 8);
    c.set({{0, caller}, {1, stack}});
    c.s.wr<8>(guest::reg::fpr(31), c.u64(stack - 8));
    c.set_lr(caller);
    detail::transfer_saved_registers<13, true>(c, stack - 16);
}

constexpr auto reseed_initial_range = 0x1077b770u;

void reset_initial_random_pool(InitialContract& draft, std::uint32_t pc) {
    InitialCpu& c = draft.c;
    block(c, pc, 2);
    c.put32(c.r(26), c.r(20));
    c.call(reseed_initial_range, pc + 8);
    block(c, pc + 8, 2);
    const auto salt = c.r(3) & 0xffffu;
    c.set(0, salt);
    c.put32(c.r(25), salt);
}

void advance_initial_random_pool(InitialContract& draft, std::uint32_t pc, bool cursor_live = false) {
    InitialCpu& c = draft.c;
    block(c, pc, cursor_live ? 5 : 6);
    const auto current = cursor_live ? c.r(3) : c.u32(c.r(26));
    c.set(3, current);
    const auto last = c.u32(c.r(19));
    c.set(0, last);
    const auto next = current + 2;
    c.set(3, next);
    if (cursor_live) c.put32(c.r(26), next);
    const auto beyond = guest::compare_logical(c, next, last) > 0;
    if (!cursor_live) c.put32(c.r(26), next);
    if (beyond) reset_initial_random_pool(draft, pc + (cursor_live ? 20 : 24));
}

struct InitialDraw {
    std::uint32_t site;
    unsigned bound, result, scratch;
    bool alternate = false;
};

struct WordRangeReduction {
    std::int32_t upper_product, quotient;
    bool carry;
};

WordRangeReduction reduce_initial_word_range(std::uint32_t word) {
    constexpr auto reciprocal = std::int32_t(0x80008001u);
    const auto product = std::int32_t((std::int64_t(reciprocal) * std::int32_t(word)) >> 32);
    const auto shifted = std::int32_t(std::uint32_t(product) + word);
    return {product, std::int32_t(word) / 65535, shifted < 0 && (shifted & 0x7fff)};
}

void wide_initial_draw(InitialContract& draft, InitialDraw draw, std::int32_t bound) {
    InitialCpu& c = draft.c;
    const auto frame = c.sp() + draw.scratch;
    const auto start = draw.site + 0x20;
    const auto header_reg = draw.alternate ? 4u : 0u;
    const auto encoded_reg = draw.alternate ? 0u : 3u;
    const auto reciprocal_reg = draw.alternate ? 5u : 3u;
    const auto size_reg = draw.alternate ? 3u : 4u;
    const auto encoded_bound = std::uint32_t(bound) ^ 0x80000000;
    block(c, start, 40);
    c.set({{header_reg, double_header}, {encoded_reg, encoded_bound}});
    c.put32(frame + 4, encoded_bound);
    c.set(reciprocal_reg, 0x80010000);
    const auto position = c.u32(c.r(26));
    c.set({{7, position}, {size_reg, 0x10000}});
    c.put32(frame, double_header);
    c.set(6, 0x80008001);
    const auto bias = draft.number(0xc0);
    c.set_f(2, bias);
    const auto expanded_bound = std::uint32_t(bound) * 10;
    c.set(encoded_reg, expanded_bound);
    c.set_f(1, c.f64(frame));
    c.set(5, 65535);
    const auto unit = draft.number(0);
    c.set_f(0, unit);
    c.set_f(1, draft.fp.fsub(c.f(1), bias));
    const auto range = reduce_initial_word_range(expanded_bound);
    c.set(draw.alternate ? 3 : 4, std::uint32_t(range.upper_product));
    const auto word = c.u16(position);
    c.set(8, word);
    const auto salt = c.u32(c.r(25));
    c.set(7, salt);
    c.set_f(1, draft.fp.fdiv(c.f(1), unit));
    c.put32(frame + 8, double_header);

    const auto biased = word + salt;
    const auto reduced = reduce_initial_word_range(biased);
    const auto remainder = biased - std::uint32_t(reduced.quotient) * 65535;
    const auto encoded_remainder = remainder ^ 0x80000000;
    const auto bound_sign = range.quotient < 0 || (range.quotient == 0 && std::int32_t(expanded_bound) < 0);
    if (draw.alternate) c.set({{3, std::uint32_t(range.quotient)}, {4, std::uint32_t(reduced.quotient) * 65535}, {7, biased}});
    else c.set({{3, std::uint32_t(range.quotient)}, {4, unsigned(bound_sign)},
                {5, std::uint32_t(reduced.quotient) * 65535}, {6, unsigned(std::int32_t(biased) < 0)}});
    c.s.wr<1>(guest::reg::xer_ca, range.carry);
    c.set(0, encoded_remainder);
    c.put32(frame + 12, encoded_remainder);
    c.set_f(0, c.f64(frame + 8));
    c.set_f(0, draft.fp.fsub(c.f(0), bias));
    c.set_f(0, draft.fp.fmul(c.f(1), c.f(0)));
    c.put64(frame + 16, draft.fp.fctiwz(0, c.f(0)));
    c.set(draw.result, c.u32(frame + 20));
    detail::call_owned(c, 0x10374570, draw.site + 0xc0, sample_range, range.quotient);

    block(c, draw.site + 0xc0, 11);
    const auto correction = std::int32_t(c.r(3) + 5);
    const auto ceiling = std::uint32_t(bound) - 1;
    const auto high = (INT64_C(0x66666667) * correction) >> 32;
    const auto sample = c.r(draw.result) + std::uint32_t(correction / 10);
    c.s.wr<1>(guest::reg::xer_ca, high < 0 && (high & 3));
    c.set({{0, std::uint32_t(correction / 10)}, {3, unsigned(high < 0)}, {4, ceiling}, {draw.result, sample}});
    if (guest::compare(c, std::int32_t(sample), std::int32_t(ceiling)) > 0) {
        block(c, draw.site + 0xec, 2);
        c.set(draw.result, ceiling);
    }
}

void sample_initial_range(InitialContract& draft, InitialDraw draw) {
    InitialCpu& c = draft.c;
    const auto bound = std::int32_t(c.r(draw.bound));
    block(c, draw.site, 4);
    c.set({{3, 0x10000}, {0, 65535}});
    auto wide = guest::compare(c, bound, 65535) > 0;
    if (!wide) {
        block(c, draw.site + 16, 4);
        c.set({{3, 0xffff0000}, {0, std::uint32_t(-65535)}});
        wide = guest::compare(c, bound, -65535) < 0;
    }
    if (wide) {
        wide_initial_draw(draft, draw, bound);
        return;
    }

    block(c, draw.site + 0xf4, 10);
    const auto position = c.u32(c.r(26));
    c.set(3, position);
    guest::compare(c, bound, 0);
    const auto salt = c.u32(c.r(25));
    c.set(0, salt);
    const auto biased = c.u16(position) + salt;
    c.set(3, biased);
    const auto quotient = std::uint32_t(divide_signed<4>(biased, std::uint32_t(bound)));
    const auto remainder = biased - quotient * std::uint32_t(bound);
    c.set({{0, remainder}, {draw.result, 0u - remainder}});
    if (bound > 0) {
        block(c, draw.site + 0x11c, 1);
        c.set(draw.result, remainder);
    }
}

bool can_offer_initial_release(InitialContract& draft, std::uint32_t pc, int minimum) {
    InitialCpu& c = draft.c;
    block(c, pc, 3);
    const auto role = c.u8(draft.person + 0x62);
    c.set(0, role);
    if (guest::compare(c, role, 11) != 0) return false;
    block(c, pc + 12, 3);
    const auto reputation = std::int16_t(c.r(23));
    c.set(0, guest::extended(reputation));
    return guest::compare(c, reputation, minimum) > 0;
}

bool initial_release_competition(InitialContract& draft, std::uint32_t pc, unsigned method) {
    InitialCpu& c = draft.c;
    const bool split_branch = pc == 0x10372fc0;
    block(c, pc, split_branch ? 1 : 2);
    const auto has_club = guest::nonzero_logical(c, draft.club);
    if (split_branch) block(c, pc + 4, 1);
    if (!has_club) return false;
    block(c, pc + 8, 3);
    const auto slot = c.u32(draft.club + 0x30);
    c.set(3, slot);
    if (!guest::nonzero_logical(c, slot)) return false;
    block(c, pc + 20, 6);
    const auto id = c.u32(slot);
    c.set(0, id);
    const auto table = c.u32(c.r(13));
    c.set(3, table);
    c.set(0, id * 4);
    const auto competition = c.u32(table + id * 4);
    c.set(3, competition);
    if (!guest::nonzero_logical(c, competition)) return false;
    block(c, pc + 44, 3);
    const auto category = c.u8(competition + 0x43);
    c.set(0, category);
    if (guest::compare(c, category, 2) != 0) return false;
    const bool page_break = pc == 0x10372fc0;
    block(c, pc + 56, page_break ? 2 : 3);
    const auto methods = c.u32(competition);
    c.set(12, methods);
    c.set(12, c.u32(methods + method));
    if (page_break) block(c, pc + 64, 1);
    c.call(0x10781320, pc + 68);
    block(c, pc + 68, 3);
    c.set(2, c.u32(c.sp() + 0x14));
    return detail::record_nonzero(c, std::int8_t(c.r(3)));
}

bool first_initial_release_score(InitialContract& draft) {
    InitialCpu& c = draft.c;
    block(c, 0x10372f40, 32);
    const auto position = c.u32(c.r(26));
    c.set({{4, position}, {6, 0x51ec0000}, {3, 0x66660000}});
    const auto experience = c.u8(draft.person + 0x65);
    c.set(8, experience);
    const auto word = c.u16(position);
    const auto band = std::int8_t(std::uint32_t(draft.band));
    c.set({{5, word}, {4, std::uint32_t(band)}});
    const auto salt = c.u32(c.r(25));
    c.set(0, salt);
    const auto reputation = std::int16_t(c.r(23));
    const auto biased = std::int32_t(word + salt);
    c.set({{3, 0x66666667}, {6, 0x51eb851f}, {7, std::uint32_t(biased)}, {0, std::uint32_t(std::int8_t(experience))}});
    const auto professionalism = c.i8(draft.person + 0x63);
    const auto rating = reputation / 50;
    const auto high = (INT64_C(0x66666667) * biased) >> 32;
    const auto remaining = rating + biased % 20 - std::int8_t(experience) * 3;
    const auto threshold = band * 12;
    const auto score = professionalism * 2 + remaining;
    c.s.wr<1>(guest::reg::xer_ca, high < 0 && (high & 7));
    c.set({{0, std::uint32_t(threshold)}, {3, std::uint32_t(score)}, {5, std::uint32_t(remaining)},
           {6, std::uint32_t(std::int8_t(experience) * 3)}, {8, std::uint32_t(rating)}});
    return guest::compare(c, score, threshold) > 0;
}

bool alternate_initial_release_score(InitialContract& draft, bool third) {
    InitialCpu& c = draft.c;
    block(c, third ? 0x103734e4 : 0x103731ecu, third ? 32 : 28);
    const auto position = c.u32(c.r(26));
    c.set({{3, position}, {6, 0x66660000}, {5, 0x51ec0000}});
    const auto first_trait = c.u8(draft.person + (third ? 0x63 : 0x65));
    c.set(0, first_trait);
    const auto word = c.u16(position);
    const auto band = std::int8_t(std::uint32_t(draft.band));
    c.set({{10, word}, {third ? 3u : 4u, std::uint32_t(band)}});
    const auto salt = c.u32(c.r(25));
    c.set({{9, salt}, {third ? 4u : 7u, std::uint32_t(std::int8_t(first_trait))}, {8, 0x66666667}});
    const auto experience = third ? c.i8(draft.person + 0x65) : std::int8_t(first_trait);
    const auto reputation = std::int16_t(c.r(23));
    const auto biased = std::int32_t(word + salt);
    const auto rating_high = (INT64_C(0x51eb851f) * reputation) >> 32;
    const auto rating = reputation / 50;
    const auto remainder = biased % 20;
    const auto threshold = band * (third ? 10 : 14);
    const auto traits = third ? rating - experience * 2 + std::int8_t(first_trait) * 3 : rating - experience * 4;
    const auto score = traits - remainder;
    c.s.wr<1>(guest::reg::xer_ca, rating_high < 0 && (rating_high & 15));
    c.set({{0, std::uint32_t(threshold)}, {5, std::uint32_t(third ? rating : traits)}, {6, std::uint32_t(remainder)},
           {7, std::uint32_t(experience * (third ? 2 : 4))}, {8, std::uint32_t(biased / 20 * 20)}});
    if (third) c.set({{3, std::uint32_t(score)}, {4, std::uint32_t(traits)}});
    else c.set(4, std::uint32_t(score));
    return guest::compare(c, score, threshold) > 0;
}

bool initial_release_draw_bound(InitialContract& draft, std::uint32_t pc, bool half) {
    InitialCpu& c = draft.c;
    const auto valuation = std::uint32_t(draft.valuation);
    block(c, pc, half ? 4 : 3);
    const auto shift_input = std::int32_t(half ? valuation + (valuation >> 31) : valuation);
    const auto shift = half ? 1 : 2;
    const auto floor = shift_input >> shift;
    const auto carry = shift_input < 0 && (shift_input & ((1 << shift) - 1));
    c.s.wr<1>(guest::reg::xer_ca, carry);
    c.set(0, half ? std::uint32_t(shift_input) : std::uint32_t(floor));
    const auto bound = half ? floor : floor + int(carry);
    return detail::record_nonzero(c, bound, half ? 13 : 14);
}

void minimum_initial_release(InitialContract& draft, std::uint32_t pc, unsigned destination, std::uint32_t amount,
                             unsigned minimum) {
    InitialCpu& c = draft.c;
    block(c, pc, 5);
    c.set({{3, (minimum + (destination == 14 ? 0x2f70 : 0x3cb0))}, {destination, amount}, {0, minimum}});
    if (guest::compare(c, std::int32_t(amount), std::int32_t(minimum)) < 0) {
        block(c, pc + 20, 1);
        c.set(destination, minimum);
    }
}

void finish_initial_release(InitialContract& draft, std::uint32_t pc, std::int8_t option, unsigned amount) {
    initial_option(draft, pc, option, true);
    initial_release_amount(draft, pc + 12, draft.c.r(amount));
    block(draft.c, pc + 24, 1);
}

void apply_first_initial_release(InitialContract& draft) {
    InitialCpu& c = draft.c;
    if (!can_offer_initial_release(draft, 0x10372f00, 2750)) {
        initial_option(draft, 0x103731a0, 0, false);
        return;
    }
    advance_initial_random_pool(draft, 0x10372f18);
    if (!first_initial_release_score(draft) || !initial_release_competition(draft, 0x10372fc0, 0xa0)) {
        initial_option(draft, 0x103731a0, 0, false);
        return;
    }
    if (initial_release_draw_bound(draft, 0x10373010, false)) {
        advance_initial_random_pool(draft, 0x10373024);
        sample_initial_range(draft, {0x1037304c, 14, 15, 0x84});
    } else {
        block(c, 0x1037301c, 2);
        c.set(15, 0);
    }
    minimum_initial_release(draft, 0x1037316c, 14, draft.valuation + c.r(15), 250000);
    finish_initial_release(draft, 0x10373184, 0, 14);
}

std::uint32_t scale_second_initial_release(InitialContract& draft) {
    InitialCpu& c = draft.c;
    const auto frame = c.sp();
    block(c, 0x103733b4, 19);
    const auto amount = draft.valuation + c.r(15);
    const auto first_word = amount ^ 0x80000000;
    const auto doubled = amount * 2;
    const auto double_word = doubled ^ 0x80000000;
    c.set({{14, amount}, {3, double_header}, {0, first_word}});
    c.put32(frame + 0x84, double_header);
    c.set(4, doubled);
    const auto bias = draft.number(0xc0);
    c.set_f(3, bias);
    c.put32(frame + 0x88, first_word);
    c.set(0, double_word);
    c.set_f(4, draft.number(0x10));
    c.set_f(0, c.f64(frame + 0x84));
    c.put32(frame + 0x90, double_word);
    c.set_f(2, draft.fp.fsub(c.f(0), bias));
    c.set_f(1, draft.number(8));
    c.put32(frame + 0x8c, double_header);
    c.set_f(0, c.f64(frame + 0x8c));
    c.set_f(1, draft.fp.multiply_add(0x103733f0, c.f(4), c.f(2), c.f(1)));
    c.set_f(0, draft.fp.fsub(c.f(0), bias));
    draft.fp.fcmp(0, c.f(1), c.f(0));
    if (c.cr(0) & 4) block(c, 0x10373400, 1);
    else {
        block(c, 0x10373404, 4);
        c.put32(frame + 0x98, double_word);
        c.put32(frame + 0x94, double_header);
        c.set_f(0, c.f64(frame + 0x94));
        c.set_f(1, draft.fp.fsub(c.f(0), bias));
    }

    block(c, 0x10373414, 7);
    original_fctiwz(c.s, guest::reg::fpr(0), guest::reg::fpr(1));
    c.set({{3, 0xb70000}, {0, 12000000}});
    c.put64(frame + 0x7c, c.s.r<8>(guest::reg::fpr(0)));
    auto supplement = c.u32(frame + 0x80);
    c.set(4, supplement);
    if (guest::compare(c, std::int32_t(supplement), 12000000) > 0) {
        block(c, 0x10373430, 13);
        const auto encoded = supplement ^ 0x80000000;
        c.set({{3, encoded}, {0, double_header}});
        c.put32(frame + 0x78, encoded);
        c.set_f(2, draft.number(0xc0));
        c.put32(frame + 0x74, double_header);
        c.set_f(3, draft.number(0x20));
        c.set_f(1, c.f64(frame + 0x74));
        c.set_f(0, draft.number(0x18));
        c.set_f(1, draft.fp.fsub(c.f(1), c.f(2)));
        c.set_f(0, draft.fp.multiply_add(0x10373454, c.f(3), c.f(1), c.f(0)));
        c.put64(frame + 0x6c, draft.fp.fctiwz(0, c.f(0)));
        supplement = c.u32(frame + 0x70);
        c.set(4, supplement);
    }
    return amount + supplement;
}

void apply_second_initial_release(InitialContract& draft) {
    InitialCpu& c = draft.c;
    if (!can_offer_initial_release(draft, 0x103731ac, 6500)) {
        initial_option(draft, 0x10373498, 2, false);
        return;
    }
    advance_initial_random_pool(draft, 0x103731c4);
    if (!alternate_initial_release_score(draft, false)) {
        initial_option(draft, 0x10373498, 2, false);
        return;
    }
    if (initial_release_draw_bound(draft, 0x1037325c, false)) {
        advance_initial_random_pool(draft, 0x10373270, true);
        sample_initial_range(draft, {0x10373294, 14, 15, 0x84});
    } else {
        block(c, 0x10373268, 2);
        c.set(15, 0);
    }
    const auto amount = scale_second_initial_release(draft);
    minimum_initial_release(draft, 0x10373464, 14, amount, 250000);
    finish_initial_release(draft, 0x1037347c, 2, 14);
}

int read_release_trait(InitialContract& draft, std::uint32_t pc, unsigned source, unsigned reg, int threshold) {
    InitialCpu& c = draft.c;
    block(c, pc, 4);
    const auto raw = c.u8(draft.person + source);
    const auto trait = std::int8_t(raw);
    c.set({{reg, raw}, {0, std::uint32_t(trait)}});
    return guest::compare(c, trait, threshold);
}

void scale_third_initial_release(InitialContract& draft, std::uint32_t pc, unsigned factor, bool jump = true) {
    InitialCpu& c = draft.c;
    const auto frame = c.sp();
    block(c, pc, jump ? 13 : 12);
    const auto encoded = c.r(13) ^ 0x80000000;
    c.set({{3, encoded}, {0, double_header}});
    c.put32(frame + 0x70, encoded);
    c.set_f(1, draft.number(0xc0));
    c.put32(frame + 0x6c, double_header);
    c.set_f(2, draft.number(factor));
    c.set_f(0, c.f64(frame + 0x6c));
    c.set_f(0, draft.fp.fsub(c.f(0), c.f(1)));
    c.set_f(0, draft.fp.fmul(c.f(2), c.f(0)));
    c.put64(frame + 0x74, draft.fp.fctiwz(0, c.f(0)));
    c.set(13, c.u32(frame + 0x78));
}

void apply_third_initial_release(InitialContract& draft) {
    InitialCpu& c = draft.c;
    if (!can_offer_initial_release(draft, 0x103734a4, 1750)) {
        initial_option(draft, 0x1037381c, 3, false);
        return;
    }
    advance_initial_random_pool(draft, 0x103734bc);
    if (!alternate_initial_release_score(draft, true) || !initial_release_competition(draft, 0x10373564, 0xa4)) {
        initial_option(draft, 0x1037381c, 3, false);
        return;
    }
    if (initial_release_draw_bound(draft, 0x103735b4, true)) {
        advance_initial_random_pool(draft, 0x103735cc);
        sample_initial_range(draft, {0x103735f4, 13, 14, 0x6c});
    } else {
        block(c, 0x103735c4, 2);
        c.set(14, 0);
    }
    minimum_initial_release(draft, 0x10373714, 13, draft.valuation - c.r(14), 50000);
    if (read_release_trait(draft, 0x1037372c, 0x65, 3, 5) < 0
        && read_release_trait(draft, 0x1037373c, 0x63, 0, 10) > 0)
        scale_third_initial_release(draft, 0x1037374c, 0x28);
    else if (read_release_trait(draft, 0x10373780, 0x63, 0, 10) > 0)
        scale_third_initial_release(draft, 0x10373790, 0x30);
    else {
        block(c, 0x103737c4, 3);
        const auto experience = std::int8_t(c.r(3));
        c.set(0, std::uint32_t(experience));
        if (guest::compare(c, experience, 15) < 0)
            scale_third_initial_release(draft, 0x103737d0, 0x38, false);
    }
    finish_initial_release(draft, 0x10373800, 3, 13);
}

void apply_initial_profile_bonus(InitialContract& draft, std::uint32_t pc, std::int8_t bonus) {
    InitialCpu& c = draft.c;
    const auto wage = std::int32_t(c.r(22));
    const auto high = (INT64_C(0x66666667) * wage) >> 32;
    block(c, pc, 7);
    c.s.wr<1>(guest::reg::xer_ca, high < 0 && (high & 15));
    c.set({{0, std::uint32_t(high >> 4)}, {3, unsigned(high < 0)}});
    if (detail::record_nonzero(c, wage / 40, 13)) {
        advance_initial_random_pool(draft, pc + 0x24);
        sample_initial_range(draft, {pc + 0x4c, 13, 14, 0x6c, bonus == 2});
    } else {
        block(c, pc + 28, 2);
        c.set(14, 0);
    }

    block(c, pc + 0x16c, 11);
    const auto share = std::uint32_t(wage / 20);
    const auto amount = share - c.r(14) + 25;
    c.s.wr<1>(guest::reg::xer_ca, high < 0 && (high & 7));
    c.set({{3, draft.contract}, {4, std::uint32_t(bonus)}, {0, share}, {5, amount}});
    initial_bonus(draft, pc + 0x198, bonus, true, amount);
    block(c, pc + 0x198, 1);
}

void apply_initial_profile_bonuses(InitialContract& draft) {
    InitialCpu& c = draft.c;
    const auto profile = c.r(21);
    const auto experienced = [&](std::uint32_t pc, unsigned field) {
        block(c, pc, 4);
        const auto value = c.i8(profile + field);
        c.set(0, std::uint32_t(value));
        return guest::compare(c, value, 15) >= 0;
    };
    if (experienced(0x10373828, 0x15)) apply_initial_profile_bonus(draft, 0x10373838, 0);
    else initial_bonus(draft, 0x103739d4, 0, false);
    if (experienced(0x103739e0, 0x13) || experienced(0x103739f0, 0x14) || experienced(0x10373a00, 0x15))
        apply_initial_profile_bonus(draft, 0x10373a10, 1);
    else initial_bonus(draft, 0x10373bac, 1, false);

    block(c, 0x10373bb8, 3);
    const auto potential = c.u8(profile + 15);
    c.set(0, potential);
    if (guest::compare(c, potential, 20) == 0) apply_initial_profile_bonus(draft, 0x10373bc4, 2);
    else initial_bonus(draft, 0x10373d60, 2, false);
}

void apply_initial_contract_options(InitialContract& draft) {
    if (prepare_initial_options(draft)) {
        apply_first_initial_release(draft);
        apply_second_initial_release(draft);
        apply_third_initial_release(draft);
        apply_initial_profile_bonuses(draft);
    }
    clear_initial_options_without_profile(draft);
}

void run_initial_contract(Cpu& cpu, SemanticGuestObserver* observer) {
    InitialCpu c(cpu, observer);
    GuestNativeScope scope(c.s, initialize_entry);
    open_initial_contract_frame(c, scope);
    InitialContract draft(c, c.r(31), c.r(30), c.r(24));
    if (prepare_initial_contract(draft)) {
        choose_initial_contract_dates(draft);
        apply_initial_contract_options(draft);
        apply_initial_nation_limits(draft);
        finish_initial_contract(draft);
    } else {
        // Both early exits enter the restore from a different page.
        block(c, initialize_return, 6);
    }
    restore_initial_contract_frame(c);
    c.finish();
}
}

void initialize_staff_contract(Cpu& c) {
    run_initial_contract(c, contract_storage_diagnostics(std::as_const(c.s)).observer);
}
}
