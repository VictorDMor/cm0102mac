#include "transfer_manager_installments.hpp"
#include "guest_fpu.hpp"
#include "guest_values.hpp"
#include "transfer_manager_adapter.hpp"
#include "transfer_manager_fees.hpp"

namespace cm::aot::transfer_manager {
namespace {
using namespace guest;
constexpr std::uint32_t installment_list = 0x9a4, staff_flags = 0x9a0, club_size = 0x248, person_size = 0x70;
constexpr auto constants_slot = -0x4ba0, finances_slot = -0x7ab8, clubs_slot = -0x7be8;
constexpr std::uint32_t club_finance = 0x10103510, record_transfer = 0x100132f0, primary_lookup = 0x10082f20,
    debit_account = 0x100f70e0, credit_account = 0x100f7050, append_list = 0x10738d10, next_list = 0x10738e00,
    remove_list = 0x10738e60, allocate_flags = 0x10775200, release_flags = 0x10775170;

struct OfferTerms {
    Cpu& c;
    std::uint32_t address;
    std::uint32_t buyer() const { return c.u32(address + 8); }
    std::uint32_t person() const { return c.u32(address + 0xc); }
    std::int32_t fee() const { return c.i32(address + 0x20); }
    std::int8_t installment_count() const { return c.i8(address + 0x51); }
    std::int8_t sell_on_percent() const { return c.i8(address + 0x50); }
};

struct Installment {
    Cpu& c;
    std::uint32_t address;
    std::uint32_t amount() const { return c.u32(address); }
    std::uint32_t buyer() const { return c.u32(address + 4); }
    std::uint32_t seller() const { return c.u32(address + 8); }
    Field<std::int8_t> remaining() const { return {c, address + 0xc}; }
};

std::uint32_t call(Cpu& c, std::uint32_t entry, std::uint32_t back,
                   std::initializer_list<std::uint32_t> args = {}) {
    unsigned r = 3;
    for (auto arg : args) c.set(r++, arg);
    c.call(entry, back);
    return c.r(3);
}

std::uint32_t sign_word(Cpu& c, std::uint32_t amount) {
    const auto sign = amount >> 31;
    c.s.wr<1>(reg::xer_ca, sign && (amount & 0x7fffffffu));
    return 0u - sign;
}

void append_fee(Cpu& c, FeeKind kind, std::uint32_t manager, std::uint32_t row,
                std::uint32_t entry, std::uint32_t back) {
    c.set({{3, manager}, {4, row}});
    if (!transfer_manager_direct_calls_enabled()) {
        c.call(entry, back);
        return;
    }
    c.link(back);
    c.s.pc = entry;
    append_transfer_fee(c, kind, manager, row);
    c.finish();
}

void publish_status(Cpu& c, std::uint32_t manager, const OfferTerms& offer, std::uint32_t bit) {
    const auto offset = offer.person() * 10, status = c.u32(manager + staff_flags);
    const auto updated = c.u8(status + offset) | bit;
    c.set({{0, updated}, {3, offset}, {4, status}});
    c.put8(status + offset, updated);
}

void append_appearance(Cpu& c, const OfferTerms& offer, std::uint32_t manager, std::uint32_t seller,
                       std::uint32_t row, FeeKind kind) {
    const bool international = kind == FeeKind::international_appearance;
    const auto field = offer.address + (international ? 0x48 : 0x40);
    const auto amount = c.i32(field);
    c.set(5, std::uint32_t(amount));
    if (compare(c, amount) <= 0) return;
    c.put32(row, offer.person());
    c.put32(row + 4, std::uint32_t(amount));
    c.put8(row + 8, c.u8(field + 4));
    const auto club = c.u32(seller);
    c.set(0, club);
    c.put32(row + 12, club);
    append_fee(c, kind, manager, row, international ? 0x103f2100 : 0x103f1fb0,
               international ? 0x103f1e2c : 0x103f1de0);
    publish_status(c, manager, offer, international ? 4 : 2);
}

// The two words of the biased integer load remain in the guest frame.
void write_biased_amount(Cpu& c, std::uint32_t scratch, std::uint32_t amount) {
    c.put32(scratch, 0x43300000);
    c.put32(scratch + 4, amount ^ 0x80000000u);
}

void add_initial_reserve(Cpu& c, std::uint32_t account, std::uint32_t amount, std::uint32_t constants,
                         std::uint32_t biased, std::uint32_t converted) {
    write_biased_amount(c, biased, amount);
    const auto bias = c.f64(constants + 8), divisor = c.f64(constants + 0x130);
    const auto old = c.u32(account + 0x1c);
    c.set(4, old);
    c.set_f(2, bias);
    Fpu fpu(c);
    const auto signed_amount = fpu.fsub(c.f64(biased), bias);
    c.set_f(1, signed_amount);
    c.put64(converted, fpu.fctiwz(0, fpu.fdiv(signed_amount, divisor)));
    const auto next = old + c.u32(converted + 4);
    c.set(0, next);
    c.put32(account + 0x1c, next);
}

void add_recurring_reserves(Cpu& c, const Frame& frame, const Installment& row, std::uint32_t buyer,
                            std::uint32_t seller, std::uint32_t constants, bool final_year) {
    const auto buyer_input = frame.at(final_year ? 0x50 : 0x38);
    const auto buyer_result = frame.at(final_year ? 0x48 : 0x40);
    const auto seller_input = frame.at(final_year ? 0x40 : 0x48);
    const auto seller_result = frame.at(final_year ? 0x38 : 0x50);
    write_biased_amount(c, buyer_input, row.amount());
    const auto bias = c.f64(constants + 8), rate = c.f64(constants + (final_year ? 0x138 : 0xb0));
    c.set_f(1, bias);
    c.set_f(2, rate);
    const auto buyer_old = c.u32(buyer + 0x1c);
    c.set(3, buyer_old);
    Fpu fpu(c);
    const auto buyer_amount = fpu.fsub(c.f64(buyer_input), bias);
    c.put32(seller_input, 0x43300000);
    c.put64(buyer_result, fpu.fctiwz(0, fpu.fmul(rate, buyer_amount)));
    const auto buyer_next = buyer_old + c.u32(buyer_result + 4);
    c.set(0, buyer_next);
    c.put32(buyer + 0x1c, buyer_next);
    const auto amount = row.amount(), seller_old = c.u32(seller + 0x1c);
    c.set(3, seller_old);
    c.put32(seller_input + 4, (0u - amount) ^ 0x80000000u);
    const auto seller_amount = fpu.fsub(c.f64(seller_input), bias);
    c.put64(seller_result, fpu.fctiwz(0, fpu.fmul(rate, seller_amount)));
    const auto seller_next = seller_old + c.u32(seller_result + 4);
    c.set(0, seller_next);
    c.put32(seller + 0x1c, seller_next);
}

void clear_inactive_reserve(Cpu& c, std::uint32_t account, std::uint32_t sellers, std::uint32_t buyers) {
    const auto seller_flag = c.i8(sellers);
    c.set({{0, std::uint32_t(seller_flag)}, {4, std::uint8_t(seller_flag)}});
    if (!nonzero(c, seller_flag)) {
        const auto buyer_flag = c.i8(buyers);
        c.set(0, std::uint32_t(buyer_flag));
        if (!nonzero(c, buyer_flag)) {
            c.put32(account + 0x1c, 0);
            return;
        }
    }
    const auto buyer_flag = c.i8(buyers);
    c.set({{0, std::uint32_t(buyer_flag)}, {5, std::uint8_t(buyer_flag)}});
    if (nonzero(c, buyer_flag) && !nonzero(c, seller_flag)) {
        const auto reserve = c.i32(account + 0x1c);
        if (compare(c, reserve) > 0) c.put32(account + 0x1c, 0);
    } else if (nonzero(c, seller_flag) && !nonzero(c, buyer_flag)) {
        const auto reserve = c.i32(account + 0x1c);
        if (compare(c, reserve) < 0) c.put32(account + 0x1c, 0);
    }
}
}

void register_transfer_consideration(Cpu& c, std::uint32_t manager, std::uint32_t offer_address) {
    Frame frame(c, 0x103f1b30, 0xb0, {.first_gpr = 22});
    const Kept<25> transfers(c, manager);
    const Kept<26> offer_register(c, offer_address);
    const auto clubs = c.toc(clubs_slot);
    Kept<31> seller(c, clubs);
    Kept<24> sign(c, 0);
    const auto people = c.toc(-0x7be0), ledger = c.toc(-0x7b7c);
    const Kept<29> constants(c, c.toc(constants_slot));
    const Kept<30> finances(c, c.toc(finances_slot));
    Kept<23> buyer_account(c, c.toc(-0x7b78));
    const OfferTerms offer{c, offer_register};
    const auto buyer_offset = offer.buyer() * club_size, staff_offset = offer.person() * person_size;
    const auto club_base = c.u32(clubs);
    const Kept<27> buyer(c, club_base + buyer_offset);
    c.set({{0, staff_offset}, {5, ledger}, {6, buyer_offset}, {7, club_base}});
    const auto contract = call(c, primary_lookup, 0x103f1b8c, {c.u32(ledger), c.u32(people) + staff_offset});
    const bool found = nonzero_logical(c, contract);
    c.put32(buyer_account, contract);
    if (found) {
        const auto flags = c.i8(contract + 0x38);
        c.s.wr<1>(reg::xer_ca, flags < 0 && (flags & 2));
        const auto kind = std::int32_t(flags) >> 2;
        c.set(0, std::uint32_t(kind));
        if (compare(c, kind, 5) != 0) sign = 1;
    }
    if (nonzero(c, sign)) {
        const auto current_contract = c.u32(buyer_account);
        const auto offset = c.u32(current_contract + 4) * club_size;
        c.set(0, offset);
        seller = c.u32(clubs) + offset;
    } else {
        seller = 0;
    }
    buyer_account = call(c, club_finance, 0x103f1be4, {c.u32(finances), buyer});
    // The original retains the caller's r28 when there is no selling club.
    Kept<28> seller_account(c, c.r(28));
    if (nonzero_logical(c, seller)) seller_account = call(c, club_finance, 0x103f1bfc, {c.u32(finances), seller});

    const auto months = offer.installment_count();
    c.set({{0, std::uint32_t(months)}, {4, std::uint8_t(months)}});
    const bool scheduled = compare(c, months) > 0;
    const auto amount = offer.fee();
    const auto payment = std::uint32_t(scheduled ? amount / months : amount);
    const Kept<22> fee(c, payment);
    if (scheduled) c.set(0, payment);
    sign = sign_word(c, payment);
    call(c, record_transfer, 0x103f1c44, {c.u32(c.toc(-0x7afc)), std::uint32_t(amount), buyer, seller});
    call(c, debit_account, 0x103f1c54, {buyer_account, sign, fee});
    if (nonzero_logical(c, seller_account)) call(c, credit_account, 0x103f1c6c, {seller_account, sign, fee});

    const auto current_months = offer.installment_count();
    c.set({{0, std::uint32_t(current_months)}, {3, std::uint8_t(current_months)}});
    if (compare(c, current_months) > 0) {
        const auto installment = frame.at(0x68);
        const auto scheduled_amount = std::uint32_t(offer.fee() / current_months);
        c.put32(installment, scheduled_amount);
        c.put32(installment + 4, c.u32(buyer));
        const auto seller_id = c.u32(seller);
        c.set({{0, std::uint32_t(current_months - 1)}, {5, seller_id}});
        c.put32(installment + 8, seller_id);
        c.put8(installment + 12, std::uint32_t(current_months - 1));
        seller_account = call(c, club_finance, 0x103f1cac, {c.u32(finances), buyer});
        const auto recipient = call(c, club_finance, 0x103f1cbc, {c.u32(finances), seller});
        if (nonzero_logical(c, seller_account))
            add_initial_reserve(c, seller_account, 0u - c.u32(installment), constants, frame.at(0x80), frame.at(0x78));
        if (nonzero_logical(c, recipient))
            add_initial_reserve(c, recipient, c.u32(installment), constants, frame.at(0x78), frame.at(0x80));
        call(c, append_list, 0x103f1d60, {transfers + installment_list, installment, 16});
    }

    const auto percent = offer.sell_on_percent();
    c.set({{0, std::uint32_t(percent)}, {3, std::uint8_t(percent)}});
    if (compare(c, percent) > 0) {
        const auto row = frame.at(0x58);
        c.put8(row, std::uint8_t(percent));
        c.put32(row + 4, c.u32(buyer));
        c.put32(row + 8, c.u32(seller));
        const auto staff_id = offer.person();
        c.set(0, staff_id);
        c.put32(row + 12, staff_id);
        append_fee(c, FeeKind::sell_on, transfers, row, 0x103f1e60, 0x103f1d94);
        publish_status(c, transfers, offer, 8);
    }
    append_appearance(c, offer, transfers, seller, frame.at(0x48), FeeKind::first_team_appearance);
    append_appearance(c, offer, transfers, seller, frame.at(0x38), FeeKind::international_appearance);
}

void settle_transfer_installments(Cpu& c, std::uint32_t manager) {
    Frame frame(c, 0x103f2250, 0x80, {.first_gpr = 22});
    c.set(0, c.lr());
    const Kept<30> club_count(c, c.toc(-0x7b50));
    Kept<22> transfers(c, manager);
    Kept<27> constants(c, c.toc(constants_slot));
    const Kept<28> finances(c, c.toc(finances_slot));
    const Kept<29> clubs(c, c.toc(clubs_slot));
    const Kept<24> seller_flags(c, call(c, allocate_flags, 0x103f2280, {c.u32(club_count), 1}));
    c.set(0, seller_flags);
    const Kept<23> buyer_flags(c, call(c, allocate_flags, 0x103f2294, {c.u32(club_count), 1}));
    const auto cursor = c.u32(transfers + 0x9a8);
    c.set(0, cursor);
    c.put32(transfers + 0x9b0, cursor);
    Kept<26> row_address(c, c.r(26));
    Kept<25> recipient(c, c.r(25));
    Kept<31> payer(c, c.r(31));
    for (;;) {
        row_address = call(c, next_list, 0x103f2464, {transfers + installment_list});
        if (!nonzero(c, row_address)) break;
        const Installment row{c, row_address};
        const auto finance_at = [&](std::uint32_t club_index, std::uint32_t back) {
            const auto offset = club_index * club_size;
            c.set(0, offset);
            return call(c, club_finance, back, {c.u32(finances), c.u32(clubs) + offset});
        };
        recipient = finance_at(row.seller(), 0x103f22c0);
        payer = finance_at(row.buyer(), 0x103f22dc);
        const auto debit = row.amount();
        call(c, debit_account, 0x103f22ec, {payer, sign_word(c, debit), debit});
        const auto credit = row.amount();
        call(c, credit_account, 0x103f22fc, {recipient, sign_word(c, credit), credit});
        const auto before = std::uint8_t(row.remaining().get());
        c.set(3, before);
        row.remaining() = std::int8_t(before - 1);
        const auto after = row.remaining().get();
        c.set(0, std::uint32_t(after));
        if (compare(c, after, 4) < 0) add_recurring_reserves(c, frame, row, payer, recipient, constants, true);
        else if (compare(c, after, 8) < 0) add_recurring_reserves(c, frame, row, payer, recipient, constants, false);
        const auto remaining = row.remaining().get();
        c.set(0, std::uint32_t(remaining));
        if (!nonzero(c, remaining)) {
            call(c, remove_list, 0x103f242c, {transfers + installment_list, row_address});
        } else {
            const auto seller_base = c.u32(clubs), seller_id = c.u32(seller_base + row.seller() * club_size);
            c.set({{0, seller_id}, {3, seller_base}, {4, 1}});
            c.put8(seller_flags + seller_id, 1);
            const auto buyer_base = c.u32(clubs), buyer_id = c.u32(buyer_base + row.buyer() * club_size);
            c.set({{0, buyer_id}, {3, buyer_base}});
            c.put8(buyer_flags + buyer_id, 1);
        }
    }
    row_address = seller_flags;
    transfers = buyer_flags;
    recipient = 0;
    constants = 0;
    for (;;) {
        const auto count = c.u32(club_count);
        c.set(0, count);
        if (compare(c, recipient, count) >= 0) break;
        const auto base = c.u32(clubs);
        c.set(0, base);
        const auto account = call(c, club_finance, 0x103f2490, {c.u32(finances), base + constants});
        if (nonzero_logical(c, account)) clear_inactive_reserve(c, account, row_address, transfers);
        constants = constants + club_size;
        recipient = recipient + 1;
        transfers = transfers + 1;
        row_address = row_address + 1;
    }
    if (nonzero_logical(c, seller_flags)) call(c, release_flags, 0x103f2538, {seller_flags});
    if (nonzero_logical(c, buyer_flags)) call(c, release_flags, 0x103f2548, {buyer_flags});
}
}
