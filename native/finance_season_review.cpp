// FINANCE.CPP, block season: the review of one club's finances (0x100fd210).
//
// Run for a club that the game runs, with the finance manager: a club whose owner is a rich man (a record at
// +0x3c) may be taken over, bought out or helped, a club in debt is scolded, and every club may be given a
// squad budget (names inferred from who calls the news routines). Every sum is booked on the club's account
// (two ledger copies and the balance) and most of them are announced with a news routine of the unit.
#include "finance_season.hpp"
#include "finance_detail.hpp"
#include "guest_fpu.hpp"
#include <bit>
#include <stdexcept>

namespace cm::aot::finance {
using guest::Fpu;
using guest::Frame;

namespace {
constexpr CheckSite account_missing{0x100fd28c, 0x40, 0x38, 0x1647};
constexpr std::uint32_t club_account = 0x10103510, announce_budget = 0x10102c90, announce_finance = 0x10102e00,
                        announce_state = 0x10102820, settle_debts = 0x100f9f40, human_managed = 0x1014c2e0,
                        lookup_record = 0x10459b50;
// Ledger lines these sums are booked on (word lines, both copies).
constexpr std::uint32_t line_8c = 0x8c, line_94 = 0x94, line_b4 = 0xb4;
constexpr std::int32_t flag_table = -0x79f0,    // the cell of the pointer to the table of 0x4c-byte rows with flag bytes at +2
                       record_table = -0x7b5c;  // the cell of the pointer to the table of record pointers
namespace field {  // the club record
constexpr std::uint32_t index = 0x00, competition = 0x30, owner = 0x3c, manager = 0x80, favourite = 0x90,
                        taken_over_by = 0x23e, kind = 0x23d, level = 0x240, wants_budget = 0x241;
}
namespace owner_field {  // the record at club +0x3c
constexpr std::uint32_t id = 0x00, money = 0x04, spent = 0x08, reserve = 0x0c, helped = 0x4c, rewarded = 0x4d;
}
namespace doubles_at {
constexpr std::uint32_t scale_a = 0xe0, scale_b = 0x118, ratio = 0x2f8;
}

std::int32_t divide(std::int32_t a, std::int32_t b) { return std::int32_t(std::int64_t(a) / b); }
std::int32_t remainder(std::int32_t a, std::int32_t b) {
    return std::int32_t(std::uint32_t(a) - std::uint32_t(divide(a, b)) * std::uint32_t(b));
}
std::int32_t sext16(std::uint32_t value) { return std::int32_t(std::int16_t(value)); }
std::int32_t sext8(std::uint32_t value) { return std::int32_t(std::int8_t(value)); }
std::uint32_t word(std::int32_t value) { return std::uint32_t(value); }

class Review {
public:
    // balance_low starts as the string pool pointer, which is what r22 holds until the account is found.
    Review(Cpu& c, std::uint32_t manager, std::uint32_t club)
        : c(c), fpu(c), manager(manager), club(club), doubles(c.toc(toc::doubles)),
          message_cell(c.toc(toc::message)), club_count_cell(c.toc(toc::club_count)), balance_low(c.toc(toc::pool)), flag_table_cell(c.toc(flag_table)),
          base_cell(c.toc(sample_table::base)), first(c.toc(sample_table::first)), last(c.toc(sample_table::last)),
          cursor(c.toc(sample_table::cursor)), account(c.r(28)) {}

    void run(const Frame& frame) {
        sync();
        if (std::int8_t(invoke(c, callee::club_is_inactive, 0x100fd25c, {club})) != 0) return;
        account = invoke_unit(c, club_account, 0x100fd270, {manager, club});
        if (account == 0) {
            sync();
            report_check(c, frame, account_missing);
            return;
        }
        balance_low = c.u32(account + 4);
        const auto owner = c.u32(club + field::owner);
        const bool taken = sext8(c.u8(club + field::taken_over_by)) != 0;
        if (owner != 0 && !taken && c.u32(club + field::competition) != 0) {
            find_rival(owner);
        } else if (owner != 0 && taken && c.u32(club + field::competition) != 0) {
            owned_club();
        }
        settle();
        budget();
    }

private:
    // ---- plumbing ----
    // The non-volatile registers as the original holds them at a call.
    void sync() {
        c.set({{18, message_cell}, {19, searching ? rival : club_count_cell}, {20, doubles}, {21, record_figure}, {22, balance_low}, {23, flag_table_cell}, {24, base_cell}, {25, first}, {26, last},
               {27, cursor}, {28, account}, {29, manager}, {30, club}, {31, flag_cleared}});
    }
    std::uint32_t drawn(std::uint32_t back) {
        sync();
        return draw(c, back);
    }
    std::int32_t reputation() const { return c.i16(club + club::reputation); }
    std::int64_t balance(std::uint32_t at) const { return balance_of(c, at); }
    std::uint32_t account_by_index() const { return c.u32(manager + manager::accounts) + c.u32(club + field::index) * account::size; }
    // A sum paid out: both copies of the line and the balance (the amount unsigned).
    void pay(std::uint32_t at, std::uint32_t line, std::uint32_t amount) {
        c.put32(at + line, c.u32(at + line) + amount);
        c.put32(at + line + account::second_copy, c.u32(at + line + account::second_copy) + amount);
        c.put64(at + account::balance, c.u64(at + account::balance) - std::uint64_t(amount));
    }
    void announce(std::uint32_t entry, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        sync();
        invoke_unit(c, entry, back, arguments);
    }
    // int to double through the magic word (the original's `xoris` and stack words).
    double to_double(std::int32_t value) {
        const auto bits = (std::uint64_t(0x43300000) << 32) | (std::uint32_t(value) ^ 0x80000000u);
        return fpu.fsub(std::bit_cast<double>(bits), c.f64(doubles + doubles::int_magic));
    }
    // ---- an owner without a club of his own (0x100fd2f8): a takeover, or a rival's compensation ----
    // The 20,000,000 the new club pays, with the first news kind.
    void takeover(std::uint32_t back) {
        c.put8(club + field::taken_over_by, 1);
        pay(account, line_8c, 20'000'000);
        announce(announce_finance, back, {manager, club, c.u32(c.u32(club + field::owner) + owner_field::id), 4});
    }
    void find_rival(std::uint32_t owner) {
        if (std::int32_t(balance_low) <= 35'000'000) return;
        const auto count = c.u32(club_count_cell);
        rival = c.u32(c.toc(toc::clubs));
        searching = true;
        bool found = false;
        if (std::int32_t(count) > 0) {
            for (std::uint32_t left = count; left != 0; --left) {
                if (rival != club && c.u32(rival + field::owner) == owner && sext8(c.u8(rival + field::taken_over_by)) != 0) {
                    found = true;
                    break;
                }
                rival += club_size;
            }
        }
        if (!found) return takeover(0x100fd3c4);
        // Dead (0x100fd3cc): a found rival was read at rival + 0x3c (0x100fd330), and the null page is unmapped, so rival is not 0.
        if (rival == 0) throw std::logic_error("0x100fd210: found rival at address 0 (dead at 0x100fd3cc)");
        sync();
        const auto rival_account = invoke_unit(c, club_account, 0x100fd3dc, {manager, rival});
        if (rival_account == 0) return takeover(0x100fd448);
        const auto kind = sext8(c.u8(rival_account + 0x165));
        if (kind != 1 && kind != 2) return;
        c.put8(club + field::taken_over_by, 1);
        c.put8(rival + field::taken_over_by, 0);
        pay(account, line_8c, 20'000'000);
        c.put32(rival_account + line_b4, c.u32(rival_account + line_b4) + 20'000'000u);
        c.put32(rival_account + line_b4 + account::second_copy, c.u32(rival_account + line_b4 + account::second_copy) + 20'000'000u);
        c.put64(rival_account + account::balance, c.u64(rival_account + account::balance) + 20'000'000u);
        announce(announce_finance, 0x100fd510, {manager, club, c.u32(c.u32(club + field::owner) + owner_field::id), 4});
        announce(announce_finance, 0x100fd528, {manager, rival, c.u32(c.u32(club + field::owner) + owner_field::id), 5});
    }

    // ---- a club that has an owner and is held by him (0x100fd52c) ----
    void owned_club() {
        // A rich club pays its chairman a leaving sum (the sum is 10 less 10000 a point of his ... unsigned: a wrapped
        // word, as the original books it).
        if (std::int32_t(balance_low) > 750'000 && sext8(c.u8(account + 0x166)) > 10) {
            const auto amount = 10u - word(sext8(c.u8(account + 0x166))) * 10000u;
            pay(account, line_8c, amount);
            balance_low -= amount;
        }
        raise_level();
        settle_flags();
        owner_rewards();
    }

    // The club's level (byte +0x240, at most 20) rises with its reputation and a draw, paid from its account.
    void raise_level() {
        const auto favourite = c.u32(club + field::favourite);
        if (!(favourite != 0 && reputation() != 0 && sext8(c.u8(club + field::level)) < 20 && c.u32(favourite + 0x1c) != 0)) return;
        sync();
        if (std::int8_t(invoke(c, human_managed, 0x100fd5f8, {club, 0, 1, 0, 0})) != 0) return;
        const auto rep = reputation();
        if (!(sext8(c.u8(club + field::level)) > sext8(word(rep / 500)) + 8)) return;
        if (!(std::int32_t(balance_low) > 0x225510)) return;
        const auto sample = std::int32_t(drawn(0x100fd664));
        const auto limit = sext8(c.u8(c.u32(c.u32(club + field::favourite) + 0x1c) + 0x42));
        if (!(limit > remainder(sample, 40))) return;
        const auto raw = c.u8(club + field::level);
        const auto level = sext8(raw);
        const auto step = word(level) * 50'000u + 75'000u;
        // The test at 0x100fd6c8 (level under 20) always holds: 0x100fd5cc already required it.
        c.put8(club + field::level, raw + 1u);
        pay(account_by_index(), line_8c, step);
        const auto lifted = [&](std::int32_t from) {
            auto amount = std::int32_t(word(20 - from) * 200'000u + 500'000u);
            if (amount < 1'000'000) amount = 1'000'000;
            c.put8(club + field::level, 20);
            pay(account_by_index(), line_8c, word(amount));
        };
        // The level was raised by one above (0x100fd6d8), so `now` is the old level plus one. The tests at
        // 0x100fd738 (reputation past 0x203a), 0x100fd894 (past 0x1482 with a level under 16) and 0x100fd8ac (past
        // 0x1c52) can never hold here: the old level had to exceed the reputation over 500 (a signed byte, so
        // at most 65) plus 8 (0x100fd62c), which with a level under 20 keeps the reputation under 5500.
        const auto now = sext8(c.u8(club + field::level));
        if (now < 20 && reputation() > 0x203a)
            throw std::logic_error("0x100fd210: reputation past 0x203a with the level test passed (dead at 0x100fd73c)");
        if (now < 20 && balance(account_by_index()) > 15'000'000) return lifted(now);
        if ((now < 16 && reputation() > 0x1482) || (now < 20 && reputation() > 0x1c52))
            throw std::logic_error("0x100fd210: reputation past 0x1482 with the level test passed (dead at 0x100fd898)");
    }

    // Bit 1 of the club's flag byte, when set and the club is human-managed (the branch separates human-managed from computer-managed clubs), is cleared and remembered in flag_cleared (the marker that the branch applied).
    void settle_flags() {
        const auto cell = c.u32(flag_table_cell);
        // The club is never null here: 0x100fd2d0 already read through it (the guard at 0x100fd91c).
        const bool flagged = (c.u8(c.u32(cell) + c.u32(club) * 0x4c + 2) & 2) != 0;
        if (!flagged) return;
        sync();
        if (std::int8_t(invoke(c, human_managed, 0x100fd96c, {club, 0, 1, 0, 0})) == 0) return;
        {
            const auto at = c.u32(c.u32(flag_table_cell)) + c.u32(club) * 0x4c + 2;
            c.put8(at, c.u8(at) & ~2u);
        }
        flag_cleared = 1;
    }

    // Rewards and help for the owner's record (+0x3c), each once, then the sums by the owner's means.
    void owner_rewards() {
        const auto owner = c.u32(club + field::owner);
        if (owner != 0 && std::int32_t(balance_low) > 0x10b0760 && reputation() > 0x1d4c && c.i32(owner + owner_field::money) > 0x61a8 &&
            sext8(c.u8(owner + owner_field::rewarded)) == 0) {
            c.put8(owner + owner_field::rewarded, 1);
            pay(account, line_8c, 1'500'000);
            announce(announce_finance, 0x100fda48, {manager, club, c.u32(c.u32(club + field::owner)), 6});
            return;
        }
        if (owner != 0 && std::int32_t(balance_low) > 0x47868c0 && reputation() > 0x2422 && c.i32(owner + owner_field::money) > 0xe09c &&
            c.u8(club + field::level) == 0x14 && sext8(c.u8(owner + owner_field::helped)) == 0) {
            c.put8(owner + owner_field::helped, 1);
            pay(account, line_8c, 7'500'000);
            return;
        }
        loans();
    }

    // ---- what the owner lends or gives (0x100fdaec) ----
    void loans() {
        const auto room = 0x501bd0;
        if (std::int32_t(balance_low) <= room) {
            sync();
            const bool ok = std::int8_t(invoke(c, human_managed, 0x100fdb14, {club, 0, 1, 0, 0})) != 0;
            if (!ok || std::int32_t(balance_low + word(reputation()) * 1000u) <= room) return reply_no();
        }
        const auto competition = c.u32(club + field::competition);
        if (competition != 0) {
            const auto row = c.u32(c.u32(c.toc(record_table)) + c.u32(competition) * 4);
            if (row != 0) {
                const auto record = c.u32(row + 8);
                if (record != 0) {
                    sync();
                    const auto entry = invoke(c, lookup_record, 0x100fdb74, {record, club});
                    if (entry != 0 && c.u8(entry + 5) != 0)
                        record_figure = word(sext16(word(divide(c.i32(entry + 8), std::int32_t(c.u8(entry + 5))))));
                }
            }
        }
        const auto owner = c.u32(club + field::owner);
        const auto money = c.i32(owner + owner_field::money);
        if (money > 0x1e848) return;
        bool special = false;
        if (flag_cleared == 0) {
            // The wish in thousands against the owner's money, by the ratio.
            const auto share = fpu.fmul(c.f64(doubles + doubles_at::ratio), to_double(money));
            const auto wish = to_double(sext16(record_figure));
            fpu.fcmp(0, wish, share);
            if (!(wish > share)) {
                const auto doubled = sext16(word(reputation()) << 1);
                special = doubled <= money || reputation() <= 0x109a;
            }
        }
        if (special) return special_help();
        // The sum by the owner's kind: 1 the small owner (scale a), otherwise scale b.
        const bool small_owner = c.u8(club + field::kind) == 1;
        const std::uint32_t scale = small_owner ? doubles_at::scale_a : doubles_at::scale_b;
        std::int32_t sum;
        if (std::int32_t(balance_low) > 20'000'000) {
            sum = small_owner ? 16000 : 21000;
        } else {
            const auto scaled = std::int32_t(fpu.fctiwz(0, fpu.fmul(c.f64(doubles + scale), to_double(std::int32_t(balance_low)))));
            sum = sext16(word(divide(scaled, small_owner ? 125 : 50)));
        }
        sum = sext16(word(sum));
        const auto have = c.u32(owner + owner_field::money), reserve = c.u32(owner + owner_field::reserve);
        auto cap = sext16(word(sext16(word(divide(sum, 500))) * 500));
        if (std::int32_t(word(cap) + have) > std::int32_t(reserve)) {
            const auto room_left = std::int32_t(reserve - have);
            cap = sext16(word(room_left < 0 ? 0 : room_left));
        }
        std::int32_t limit = 5000;
        const auto third = divide(std::int32_t(reserve), 3);
        if (third >= 5000) limit = third;
        if (cap < limit) limit = cap;
        const auto granted = sext16(word(limit));
        if (granted >= 1000) {
            c.put32(owner + owner_field::money, c.u32(owner + owner_field::money) + word(granted));
            if (small_owner) {
                const auto again = c.u32(club + field::owner);
                c.put32(again + owner_field::spent, c.u32(again + owner_field::spent) + word(granted));
            }
            const auto thousands = word(divide(granted, 1000) + 1);
            const auto amount = small_owner ? word(granted) * 125u + thousands * 750'000u : word(granted) * 50u + thousands * 50'000u;
            pay(account, line_8c, amount);
            announce(announce_finance, small_owner ? 0x100fdf88 : 0x100fdda0, {manager, club, word(granted), small_owner ? 1u : 2u});
            return;
        }
        if (money < 30000) {
            c.put32(owner + owner_field::reserve, c.u32(owner + owner_field::reserve) + 0x5dc);
            if (flag_cleared != 0) announce(announce_state, small_owner ? 0x100fdfbc : 0x100fddd4, {manager, club, 2, 2});
        } else if (flag_cleared != 0) {
            announce(announce_state, small_owner ? 0x100fdfdc : 0x100fddf4, {manager, club, 2, 2});
        }
    }
    void reply_no() {
        if (flag_cleared != 0) announce(announce_state, 0x100fe1e8, {manager, club, 2, 0xffffffffu});
    }
    // The owner helps in kind: a draw below the competition's size, then a sum by the balance.
    void special_help() {
        const auto sample = std::int32_t(drawn(0x100fe000));
        const auto size = c.i16(c.u32(club + field::competition) + 0x18);
        if (!(remainder(sample, 20) + 5 < size)) return;
        const auto owner = c.u32(club + field::owner);
        const auto spent = c.u32(owner + owner_field::spent), money = c.u32(owner + owner_field::money);
        if (std::int32_t(money - spent) <= 0) return;
        const auto scaled = std::int32_t(fpu.fctiwz(0, fpu.fmul(c.f64(doubles + doubles_at::scale_b), to_double(std::int32_t(balance_low)))));
        auto sum = std::int32_t(word(sext16(word(divide(sext16(word(divide(scaled, 75))), 500)))) * 500u);
        const auto left = std::int32_t(money - spent);
        const auto chosen = sum < left ? sum : left;
        const auto granted = sext16(word(chosen));
        if (granted >= 500) {
            c.put32(owner + owner_field::money, c.u32(owner + owner_field::money) + word(granted));
            const auto again = c.u32(club + field::owner);
            c.put32(again + owner_field::spent, c.u32(again + owner_field::spent) + word(granted));
            const auto amount = word(granted) * 0x4bu + word(divide(granted, 1000) + 1) * 50'000u;
            pay(account, line_8c, amount);
            announce(announce_finance, 0x100fe178, {manager, club, word(granted), 3});
            return;
        }
        if (c.i32(owner + owner_field::money) < 30000) {
            c.put32(owner + owner_field::reserve, c.u32(owner + owner_field::reserve) + 0x5dc);
            announce(announce_state, 0x100fe1a8, {manager, club, 2, 2});
        } else if (flag_cleared != 0) {
            // Dead (0x100fe1ac): this help is only entered with flag_cleared == 0 (0x100fdbac) and nothing sets it in between.
            throw std::logic_error("0x100fd210: the owner's help announced with the flag set (dead at 0x100fe1b0)");
        }
    }

    // ---- bit 0 of the flag byte: the club is in debt or has to settle (0x100fe1e8) ----
    void settle() {
        const auto cell = c.u32(flag_table_cell);
        const bool flagged = (c.u8(c.u32(cell) + c.u32(club) * 0x4c + 2) & 1) != 0;  // club not null: see settle_flags
        if (!flagged) return;
        const auto threshold = std::int32_t(250'000u - word(reputation()) * 500u);
        if (std::int32_t(balance_low) > threshold) {
            announce(settle_debts, 0x100fe248, {manager, club});
            announce(announce_finance, 0x100fe25c, {manager, club, 0xffffffffu, 8});
        } else {
            announce(announce_state, 0x100fe274, {manager, club, 8, 0xffffffffu});
        }
        {
            const auto at = c.u32(c.u32(flag_table_cell)) + c.u32(club) * 0x4c + 2;
            c.put8(at, c.u8(at) & ~1u);
        }
    }

    // ---- the squad budget (0x100fe2a0) ----
    void budget() {
        const auto rep = reputation();
        auto limit = std::int32_t(word(rep) * 8000u);
        if (limit < 500'000) limit = 500'000;
        const auto here = balance(account);
        if (here > limit && sext8(c.u8(club + field::wants_budget)) != 0) {
            std::uint32_t amount;
            if (rep < 4000) {
                const auto sample = std::int32_t(drawn(0x100fe318));
                amount = word(reputation()) * word(remainder(sample, 501) + 1500);
                if (std::int32_t(amount) < 50'000) amount = 50'000;
            } else {
                const auto sample = std::int32_t(drawn(0x100fe394));
                amount = word(reputation()) * word(remainder(sample, 501) + 2000);
            }
            return spend(amount, 0x100fe418);
        }
        const auto manager_record = c.u32(club + field::manager);
        std::int32_t wanted = rep;
        if (manager_record != 0 && c.u32(manager_record + 0x1c) != 0) {
            const auto skill = sext8(c.u8(c.u32(manager_record + 0x1c) + 0x40));
            const auto patience = sext8(c.u8(manager_record + 0x65));
            wanted = sext16(word(rep) + word(0x15 - skill) * 5u - word(patience) * 15u);
            if (wanted < 500) wanted = 500;
        }
        const auto scale = sext16(word(wanted));
        if (balance(account) > std::int64_t(std::int32_t(word(scale) * 6000u))) {
            std::uint32_t amount;
            if (rep < 4000) {
                const auto sample = std::int32_t(drawn(0x100fe4c0));
                amount = word(reputation()) * word(remainder(sample, 501) + 1000);
            } else {
                const auto sample = std::int32_t(drawn(0x100fe528));
                amount = word(reputation()) * word(remainder(sample, 501) + 1500);
            }
            return spend(amount, 0x100fe5ac);
        }
        if (!(balance(account) > std::int64_t(std::int32_t(word(scale) * 3000u)))) return;
        spend(rep < 4000 ? word(rep) * 500u : word(rep) * 750u, 0x100fe624);
    }
    void spend(std::uint32_t amount, std::uint32_t back) {
        c.put32(account + line_94, c.u32(account + line_94) + amount);
        c.put32(account + line_94 + account::second_copy, c.u32(account + line_94 + account::second_copy) + amount);
        c.put64(account + account::balance, c.u64(account + account::balance) - std::uint64_t(amount));
        announce(announce_budget, back, {manager, club, amount});
    }

    Cpu& c;
    Fpu fpu;
    std::uint32_t manager, club, doubles;
    std::uint32_t message_cell, club_count_cell, rival = 0, record_figure = 0, balance_low, flag_table_cell, base_cell, first, last, cursor,
        flag_cleared = 0, account;
    bool searching = false;  // r19 holds the club-count cell until the rival search, then the club it is on
};
}

void review_club(Cpu& c, std::uint32_t manager, std::uint32_t club) {
    Frame frame(c, 0x100fd210, 0xb0, {.first_gpr = 18});
    Review(c, manager, club).run(frame);
}
}
