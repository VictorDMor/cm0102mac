#include "staff_contracts.hpp"
#include "staff_contracts_detail.hpp"

namespace cm::aot::staff_contracts {
namespace {
using detail::block;
using detail::call_owned;
using detail::ContractRecord;
using guest::Cpu;

constexpr std::uint32_t new_draft_entry = 0x10372260, primary_draft_entry = 0x103723b0;
constexpr std::uint32_t reset_review_entry = 0x1037f050;
constexpr unsigned wage_field = 0x0c, initial_wage = 0x3c, adjustment_flag = 0x40;

template<unsigned Register>
void copy_date(Cpu& c, std::uint32_t source, std::uint32_t destination) {
    const auto year = c.u16(source + 2);
    c.set(Register, year);
    c.put16(destination + 2, year);
    const auto day = c.i16(source);
    c.set(Register, guest::extended(day));
    c.put16(destination, std::uint16_t(day));
    const auto flags = c.u8(source + 4);
    c.set(Register, flags);
    c.put8(destination + 4, flags);
}

template<unsigned Register>
void clear_draft_clauses(Cpu& c, const guest::Kept<Register>& draft, std::uint32_t first_return) {
    call_owned(c, 0x10374b60, first_return, clear_option, std::uint32_t(draft), std::int8_t(0));
    auto pc = first_return;
    for (std::int8_t option = 1; option < 5; ++option, pc += 12) {
        block(c, pc, 3);
        c.set({{3, draft}, {4, std::uint32_t(option)}});
        call_owned(c, 0x10374b60, pc + 12, clear_option, std::uint32_t(draft), option);
    }
    for (std::int8_t bonus = 0; bonus < 3; ++bonus, pc += 12) {
        block(c, pc, 3);
        c.set({{3, draft}, {4, std::uint32_t(bonus)}});
        call_owned(c, 0x10374b80, pc + 12, clear_bonus, std::uint32_t(draft), bonus);
    }
    block(c, pc, 3);
    c.set({{3, draft}, {4, 0}});
    call_owned(c, 0x10380700, pc + 12, set_release_amount, std::uint32_t(draft), 0u);
}

void report_missing_contract(Cpu& c, const guest::Kept<24>& source, const guest::Kept<23>& diagnostic) {
    const auto frame = c.sp();
    block(c, 0x103723fc, 6);
    c.set({{3, source}, {6, frame + 0x40}, {7, frame + 0x38}, {4, 0}, {5, 0}});
    c.call(0x10739890, 0x10372414);
    block(c, 0x10372414, 1);
    c.link(0x10372418);
    c.set(3, c.toc(-0x5b20));
    c.s.charge(2);
    block(c, 0x10372418, 7);
    c.set({{5, c.r(3)}, {3, diagnostic}, {4, source + 0x14}, {6, frame + 0x40}, {7, frame + 0x38}, {8, 0xf5}});
    c.call(0x1077b720, 0x10372434);
    block(c, 0x10372434, 4);
    c.set({{4, diagnostic}, {3, source + 0x21}, {5, 0}});
    c.call(0x10769b60, 0x10372444);
    block(c, 0x10372444, 4);
    const auto active = c.toc(-0x7bac);
    c.set({{3, active}, {0, 0}});
    c.put8(active, 0);
}

std::uint32_t scale_wage(Cpu& c, std::uint32_t wage, std::int8_t percentage) {
    const auto product = std::int32_t(wage * std::uint32_t(percentage));
    // The low-word product wraps before division. Keep the carry and the
    // sign adjustment that the original leaves live at the review call.
    constexpr std::int64_t reciprocal = 0x51eb851f;
    const auto high_word = (std::int64_t(product) * reciprocal) >> 32;
    c.s.wr<1>(guest::reg::xer_ca, high_word < 0 && (high_word & 31));
    c.set(8, product < 0 ? 1 : 0);
    return std::uint32_t(product / 100);
}
}

void prepare_new_draft(Cpu& c, std::uint32_t contract, std::uint32_t staff, std::uint32_t club,
                       std::uint8_t duration) {
    block(c, new_draft_entry, 27);
    GuestNativeScope scope(c.s, new_draft_entry);
    const auto stack = c.sp(), link = c.lr();
    c.set(0, link);
    c.put32(stack - 4, c.r(31));
    const guest::Kept<31> draft(c, contract);
    c.set(7, 0);
    c.put32(stack - 8, c.r(30));
    const guest::Kept<30> employer(c, club);
    c.put32(stack - 12, c.r(29));
    const guest::Kept<29> person(c, staff);
    c.put32(stack + 8, link);
    const auto frame = stack - 0x50;
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);

    const auto staff_id = c.u32(staff);
    c.set(0, staff_id);
    const auto start = c.toc(-0x7b80);
    c.set(4, start);
    c.put32(contract, staff_id);
    const auto end = frame + 0x38;
    c.set({{0, 200}, {3, end}});
    const auto club_id = c.u32(club);
    c.set({{8, club_id}, {5, duration}});
    c.put32(draft + ContractRecord::club_id, club_id);
    c.put32(draft + ContractRecord::draft_auxiliary, 0);
    c.put32(draft + wage_field, 200);
    copy_date<0>(c, start, draft + ContractRecord::start_date);
    c.call(0x10717a10, 0x103722cc);

    block(c, 0x103722cc, 18);
    const auto end_year = c.u16(end + 2);
    c.set({{3, end_year}, {8, 5}, {6, 200}, {0, 0}});
    c.put16(draft + ContractRecord::end_date + 2, end_year);
    c.set({{3, draft}, {4, person}, {5, employer}});
    const auto end_day = c.i16(end);
    c.set(7, guest::extended(end_day));
    c.put16(draft + ContractRecord::end_date, std::uint16_t(end_day));
    const auto end_flags = c.u8(end + 4);
    c.set(7, end_flags);
    c.put8(draft + ContractRecord::end_date + 4, end_flags);
    const auto kind = (c.u8(draft + ContractRecord::packed_kind) & 3) | (5 << 2);
    c.set(7, kind);
    c.put8(draft + ContractRecord::packed_kind, kind);
    c.put32(draft + initial_wage, 200);
    c.put8(draft + adjustment_flag, 0);
    call_owned(c, reset_review_entry, 0x10372314, reset_review_state,
               std::uint32_t(draft), std::uint32_t(person), std::uint32_t(employer));

    block(c, 0x10372314, 8);
    c.set({{5, 0}, {0, UINT32_MAX}});
    c.put8(draft + ContractRecord::terms_flag, 0);
    c.set({{3, draft}, {4, 0}});
    c.put32(draft + ContractRecord::terms_marker, UINT32_MAX);
    c.put16(draft + ContractRecord::review_marker, 0);
    clear_draft_clauses(c, draft, 0x10372334);

    block(c, 0x10372394, 7);
    const auto restored = c.sp() + 0x50;
    const auto caller = c.u32(restored + 8);
    c.set({{0, caller}, {1, restored}});
    c.set_lr(caller);
    c.set(31, c.u32(restored - 4));
    c.set(30, c.u32(restored - 8));
    c.set(29, c.u32(restored - 12));
}

void prepare_primary_draft(Cpu& c, std::uint32_t contract, std::uint32_t staff, std::uint32_t club,
                           std::uint32_t end, std::uint32_t percentage, std::uint32_t first_clause,
                           std::uint32_t second_clause) {
    block(c, primary_draft_entry, 17);
    GuestNativeScope scope(c.s, primary_draft_entry);
    const auto stack = c.sp(), link = c.lr();
    c.set(0, link);
    detail::transfer_saved_registers<22>(c, stack);
    const auto manager_slot = c.toc(-0x7b7c);
    c.set(10, manager_slot);
    const guest::Kept<25> draft(c, contract);
    const guest::Kept<31> start(c, c.toc(-0x7b80));
    const guest::Kept<26> person(c, staff);
    const guest::Kept<23> diagnostic(c, c.toc(-0x7ba8));
    const guest::Kept<27> employer(c, club);
    const guest::Kept<24> source(c, c.toc(-0x4da4));
    const guest::Kept<28> end_date(c, end);
    const guest::Kept<22> factor(c, percentage);
    const guest::Kept<29> first(c, first_clause);
    const guest::Kept<30> second(c, second_clause);
    c.put32(stack + 8, link);
    const auto frame = stack - 0x90;
    c.put32(frame, stack);
    c.set(1, frame);
    scope.frame(frame);
    c.set(3, c.u32(manager_slot));
    c.call(0x10082f20, 0x103723f4);

    block(c, 0x103723f4, 2);
    const auto primary = c.r(3);
    if (!guest::nonzero_logical(c, primary)) {
        report_missing_contract(c, source, diagnostic);
    } else {
        block(c, 0x10372454, 42);
        const auto staff_id = c.u32(person);
        c.set({{0, staff_id}, {4, 0x51ec0000}, {9, 0x51eb851f}, {10, 0}});
        c.put32(draft, staff_id);
        const auto percent = std::int8_t(std::uint32_t(factor));
        c.set({{6, std::uint32_t(percent)}, {7, 6}, {0, UINT32_MAX}});
        const auto club_id = c.u32(employer);
        c.set({{8, club_id}, {4, person}, {5, employer}});
        c.put32(draft + ContractRecord::club_id, club_id);
        c.put32(draft + ContractRecord::draft_auxiliary, 0);
        const auto original_wage = c.u32(primary + wage_field);
        c.set({{8, original_wage}, {3, draft}});
        const auto wage = scale_wage(c, original_wage, percent);
        c.set(6, wage);
        c.put32(draft + wage_field, wage);
        copy_date<6>(c, start, draft + ContractRecord::start_date);
        copy_date<6>(c, end_date, draft + ContractRecord::end_date);
        const auto kind = (c.u8(draft + ContractRecord::packed_kind) & 3) | (6 << 2);
        c.set(6, kind);
        c.put8(draft + ContractRecord::packed_kind, kind);
        const auto updated_wage = c.u32(draft + wage_field);
        c.set(6, updated_wage);
        c.put32(draft + initial_wage, updated_wage);
        c.put8(draft + adjustment_flag, 0);
        c.put8(draft + ContractRecord::terms_flag, 0);
        c.put32(draft + ContractRecord::terms_marker, UINT32_MAX);
        call_owned(c, reset_review_entry, 0x103724fc, reset_review_state,
                   std::uint32_t(draft), std::uint32_t(person), std::uint32_t(employer));

        block(c, 0x103724fc, 5);
        c.set({{0, 0}, {3, draft}});
        c.put16(draft + ContractRecord::review_marker, 0);
        c.set(4, 0);
        clear_draft_clauses(c, draft, 0x10372510);
        block(c, 0x10372570, 2);
        c.put8(draft + ContractRecord::option, first);
        c.put8(draft + ContractRecord::option + 1, second);
    }
    block(c, 0x10372578, 5);
    const auto restored = c.sp() + 0x90;
    const auto caller = c.u32(restored + 8);
    c.set({{0, caller}, {1, restored}});
    c.set_lr(caller);
    detail::transfer_saved_registers<22, true>(c, restored);
}
}
