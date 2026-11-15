// FINANCE.CPP: a club's account, its ledger and its wealth (finance_ledger.hpp).
#include "finance_ledger.hpp"
#include "finance_detail.hpp"
#include "guest_fpu.hpp"
#include <bit>

namespace cm::aot::finance {
using guest::Frame;

namespace {
constexpr CheckSite account_without_club{0x10103554, 0x44, 0x3c, 0x206b};
constexpr std::uint32_t attendance_factor = 0x170;  // in the double pool
constexpr std::uint32_t refresh_account = 0x100f4f80;
}

void book(Cpu& c, std::uint32_t account, const LedgerLine& line, std::uint64_t amount) {
    for (const auto at : {account + line.offset, account + line.offset + account::second_copy}) {
        if (line.wide) c.put64(at, c.u64(at) + amount);
        else c.put32(at, c.u32(at) + std::uint32_t(amount));
    }
    const auto balance = c.u64(account + account::balance);
    c.put64(account + account::balance, line.income ? balance + amount : balance - amount);
}

std::uint32_t account_of_club(Cpu& c, std::uint32_t manager_object, std::uint32_t club) {
    Frame frame(c, 0x10103510, 0x80, {.first_gpr = 28});
    c.set({{31, c.toc(toc::pool)}, {30, c.toc(toc::message)}, {29, club}, {28, manager_object}});
    if (club == 0) {
        report_check(c, frame, account_without_club);
        return 0;
    }
    // A club record that stands for another club (a reserve side) uses that club's account.
    const auto stands_for = invoke(c, callee::club_of_code, 0x101035ac, {club, frame.at(0x38), 1});
    if (stands_for != 0 && c.i8(frame.at(0x38)) == 0) {
        club = stands_for;
        c.set(29, club);
    }
    return c.u32(manager_object + manager::accounts) + c.u32(club) * account::size;
}

std::int32_t wealth(Cpu& c, std::uint32_t account) {
    Frame frame(c, 0x100f55b0, 0x40, {.first_gpr = 30});
    const auto club = club_record(c, c.u32(account + account::club));
    c.set({{31, club}, {30, account}});
    if (std::int8_t(invoke(c, callee::club_is_inactive, 0x100f55e4, {club})) != 0) return 0;
    const std::uint32_t reputation = std::uint32_t(std::int32_t(c.i16(club + club::reputation)));
    const auto balance = balance_of(c, account);
    // Each limit is computed in a word, wrapping, and widened signed.
    const auto limit = [](std::uint32_t word) { return std::int64_t(std::int32_t(word)); };
    if (balance > limit(reputation * 2500u + 5'000'000u)) return 2;
    if (balance > limit(reputation * 1000u + 3'500'000u)) return 1;
    if (balance < limit(0u - 2'500'000u - reputation * 1500u)) return -2;
    const auto shortfall = std::int32_t(reputation) < 5000 ? 0u - reputation * 250u
                                                           : 0u - ((reputation - 5000u) * 1000u + 1'250'000u);
    return balance < limit(shortfall) ? -1 : 0;
}

void scale_attendance(Cpu& c, std::uint32_t account, std::int8_t percent) {
    guest::Fpu fpu{c};
    const auto pool = c.toc(toc::doubles);
    const auto sp = c.sp();
    // A leaf without a frame: its int-to-double words and the converted result lie below the stack pointer.
    const auto to_double = [&](std::uint32_t slot, std::uint32_t value) {
        c.put32(sp - slot, 0x43300000);
        c.put32(sp - slot + 4, value ^ 0x80000000u);
        return fpu.fsub(c.f64(sp - slot), c.f64(pool + doubles::int_magic));
    };
    const auto share = fpu.fmul(c.f64(pool + attendance_factor),
                                to_double(0x10, std::uint32_t(std::int32_t(percent)) + 100u));
    const auto scaled = fpu.fmul(to_double(8, c.u32(account + account::attendance)), share);
    const auto bits = fpu.fctiwz(0, scaled);
    c.put64(sp - 0x18, bits);
    c.put32(account + account::attendance, std::uint32_t(bits));
}

std::uint32_t destroy_account(Cpu& c, std::uint32_t account, std::int16_t deleting) {
    Frame frame(c, 0x100f1010, 0x40, {.first_gpr = 31});
    c.set(31, account);
    if (account != 0 && deleting > 0) invoke(c, callee::free_object, 0x100f1034, {account});
    return account;
}

std::uint32_t destroy_manager(Cpu& c, std::uint32_t manager_object, std::uint32_t deleting_word) {
    const auto deleting = std::int16_t(deleting_word);
    Frame frame(c, 0x100f8720, 0x40, {.first_gpr = 30});
    c.set({{31, deleting_word}, {30, manager_object}});
    if (manager_object == 0) return 0;
    if (const auto accounts = c.u32(manager_object + manager::accounts); accounts != 0)
        invoke(c, callee::destroy_array, 0x100f8754, {accounts, c.toc(toc::element_destructor)});
    if (deleting > 0) invoke(c, callee::free_object, 0x100f8764, {manager_object});
    return manager_object;
}

void refresh_leading_clubs(Cpu& c, std::uint32_t manager_object) {
    Frame frame(c, 0x100fd1a0, 0x50, {.first_gpr = 27});
    const auto count_cell = c.toc(toc::club_count), clubs_cell = c.toc(toc::clubs);
    auto account = c.u32(manager_object + manager::accounts);
    c.set({{30, count_cell}, {31, clubs_cell}});
    for (std::uint32_t index = 0; std::int32_t(index) < c.i32(count_cell); ++index, account += account::size) {
        c.set({{27, index}, {29, index * club_size}, {28, account}});
        // The pass ends at the first inactive club: the clubs after it are not refreshed.
        if (std::int8_t(invoke(c, callee::club_is_inactive, 0x100fd1d4, {c.u32(clubs_cell) + index * club_size})) != 0) break;
        invoke_unit(c, refresh_account, 0x100fd1e4, {account});
    }
}
}
