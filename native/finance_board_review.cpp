// FINANCE.CPP, block board: a club's board review by its wealth band (0x10101400).
//
// Each wealth band has its own line of action: a stage kept in the account (byte +0x165: 1 means the board was
// told, 2 that it was calmed) and a patience counter (byte +0x166), changed on a day the board may act (`acted`),
// then the gifts and bail-outs in the order the band prefers (0x100fefa0, 0x100fe640, 0x100ffec0, 0x101006a0). A
// club in trouble is told once (news 0x10102a70, 0x1014c2e0, and its people's contract wishes cut by 0x10084be0).
// Then, when the patience counter is high, the board may spend on the club (a tenth of its funds in news kind 1/0)
// or pay off the debt of a board member that has been waiting, and the manager's patience byte (+0x240) wears down.
// Names are inferred from what the routines touch.
#include "finance_board.hpp"
#include "finance_board_engine.hpp"

namespace cm::aot::finance {
using guest::Fpu;
using guest::Frame;
using namespace board;

namespace {
constexpr std::uint32_t make_account = 0x10103510, wealth_of_account = 0x100f55b0, clear_news = 0x10103270,
                        trouble_news = 0x10102a70, spend_news = 0x10102590, calm_news = 0x10102e00, donation = 0x100fe640;
namespace club_field {
constexpr std::uint32_t league = 0x30, board = 0x3c, rep = 0x1ec, works = 0x23e, patience = 0x240, refused = 0x241;
}
namespace account_field {
constexpr std::uint32_t stage = 0x165, counter = 0x166;
}

class Review {
public:
    Review(Cpu& cpu, std::uint32_t manager, std::uint32_t club, std::uint32_t acted_word)
        : c(cpu), h(cpu), fpu(cpu), manager(manager), club(club), acted(std::int8_t(acted_word) != 0) {
        h.set(20, manager);
        h.set(21, acted_word);
        h.set(23, c.toc(-0x7b78));
        h.set(24, c.toc(-0x7b7c));
        h.set(25, c.toc(-0x7be8));
        h.set(26, c.toc(sample_table::base));
        h.set(27, c.toc(sample_table::first));
        h.set(28, c.toc(sample_table::last));
        h.set(29, c.toc(sample_table::cursor));
        h.set(30, c.toc(toc::doubles));
        h.set(31, club);
    }

    void run() {
        h.sync();
        account = invoke_unit(c, make_account, 0x10101440, {manager, club});
        h.set(22, account);
        h.sync();
        const auto band = std::int8_t(invoke_unit(c, wealth_of_account, 0x10101448, {account}));
        // 0x100f55b0 answers only -2..2 (finance_ledger.cpp wealth: returns 2, 1, 0, -1, -2), so every test of a band beyond them is dead.
        if (band < -2 || band > 2) throw std::logic_error("finance board: a wealth band outside -2..2");
        if (band == 0) return generic();
        if (band > 0) {
            if (band == 2) return rich();
            return comfortable();
        }
        if (band == -2) return in_trouble();
        short_of_money();
    }

private:
    double k(std::uint32_t offset) const { return pool_double(c, offset); }
    std::int32_t stage() const { return c.i8(account + account_field::stage); }
    std::uint32_t patience_byte() const { return c.u8(club + club_field::patience); }
    void set_counter(std::uint32_t value) { c.put8(account + account_field::counter, value); }

    // The board is told once the club's trouble is over: stage 1 sends the news that clears it.
    void clear(std::uint32_t back) {
        h.sync();
        invoke_unit(c, clear_news, back, {manager, c.u32(club)});
    }
    // A call of a routine of this block that answers a byte: the original tests only its low byte.
    template <class F> bool own(std::uint32_t back, F&& call) {
        h.sync();
        c.link(back);
        return std::int8_t(call()) != 0;
    }
    bool donate(std::uint32_t back) {
        h.sync();
        return std::int8_t(invoke_unit(c, donation, back, {manager, club, account})) != 0;
    }

    // The manager's patience byte wears down one in ten, while it is 20 and the club's reputation is low.
    void wear(std::uint32_t back) {
        if (patience_byte() != 0x14 || c.i16(club + club_field::rep) >= 0x1c52) return;
        if (std::int32_t(sample(h, back)) % 10 == 0) c.put8(club + club_field::patience, patience_byte() - 1u);
    }

    // The stage goes back to 0 (from 1 the board's news clears first).
    void calm_down(std::uint32_t back) {
        const auto before = stage();
        if (before == 1 || before == 2) {
            if (before == 1) clear(back);
            c.put8(account + account_field::stage, 0);
        }
    }

    void comfortable() {
        calm_down(0x101017c8);
        if (!acted) return;
        if (const auto counter = c.i8(account + account_field::counter); counter > 0) set_counter(u(counter) - 1u);
        if (!own(0x101017fc, [&] { return benefactor_gift(c, manager, club, account); }) && !donate(0x10101814))
            own(0x1010182c, [&] { return board_cover(c, manager, club, account); });
        tail();
    }
    void rich() {
        calm_down(0x10101860);
        if (!acted) return;
        if (const auto counter = c.i8(account + account_field::counter); counter > 5) set_counter(u(counter) - 1u);
        tail();
    }
    void generic() {
        calm_down(0x101018bc);
        if (!acted) return;
        if (const auto counter = c.i8(account + account_field::counter); counter > 10) set_counter(u(counter) - 1u);
        if (!own(0x101018f4, [&] { return benefactor_gift(c, manager, club, account); }) && !donate(0x1010190c))
            own(0x10101924, [&] { return board_cover(c, manager, club, account); });
        tail();
    }
    void short_of_money() {
        if (stage() != 2) {
            if (stage() == 1) clear(0x10101498);
            c.put8(account + account_field::stage, 2);
        }
        if (!acted) return;
        if (c.i8(club + club_field::works) != 0) set_counter(c.u8(account + account_field::counter) + 1u);
        wear(0x101014f8);
        if (!donate(0x10101550) && !own(0x10101568, [&] { return board_cover(c, manager, club, account); }))
            own(0x10101580, [&] { return owner_bailout(c, manager, club, account); });
        tail();
    }

    // The club in trouble: told once (stage 1) with its people's wishes cut; afterwards the bail-outs.
    void in_trouble() {
        if (c.u8(account + account_field::stage) != 1) return told();
        if (!acted) return;
        if (!own(0x101016cc, [&] { return owner_bailout(c, manager, club, account); }) &&
            !own(0x101016e4, [&] { return board_cover(c, manager, club, account); }))
            own(0x101016fc, [&] { return benefactor_gift(c, manager, club, account); });
        wear(0x10101734);
        if (c.i8(club + club_field::works) != 0) set_counter(c.u8(account + account_field::counter) + 3u);
        tail();
    }
    void told() {
        h.sync();
        invoke_unit(c, trouble_news, 0x1010159c, {manager, club});
        if (c.i8(club + club_field::refused) != 0) c.put8(club + club_field::refused, 0);
        c.put8(account + account_field::stage, 1);
        h.sync();
        if (std::int8_t(invoke(c, 0x1014c2e0, 0x101015d0, {club, 1, 1, 0, 0})) == 0) return;  // only a human-managed club (club_control.hpp)
        const auto list = c.sp() + 0x38;
        h.sync();
        invoke(c, 0x100d06f0, 0x101015e8, {club, list, 0});
        const auto game_cell = c.toc(-0x7b7c), wish_cell = c.toc(-0x7b78);
        std::uint32_t at = list;
        h.set(20, at);
        for (std::int32_t counter = 0; std::int8_t(counter) < 0x32; ++counter, at += 4) {
            h.set(20, at);
            h.set(27, u(counter));
            const auto person = c.u32(at);
            h.set(26, person);
            if (person == 0 || c.u32(person + 0x14) == 0) continue;
            h.sync();
            const auto answer = invoke(c, 0x10082f20, 0x1010161c, {c.u32(game_cell), person});
            c.put32(wish_cell, answer);
            const auto other_club = answer != 0 ? club_record(c, c.u32(answer + 4)) : 0u;
            h.set(22, other_club);
            h.sync();
            const auto record = invoke(c, 0x10082f90, 0x10101650, {c.u32(game_cell), person, other_club});
            h.set(21, record);
            if (other_club != club || record == 0) continue;
            h.sync();
            const auto cost = invoke(c, 0x10084be0, 0x10101680, {c.u32(game_cell), person, other_club, 0, 0xffffffffu});
            if (i(cost) < c.i32(person + 8)) {
                c.put8(record + 0x5c, c.u8(record + 0x5c) | 4u);
                c.put32(person + 8, cost);
            }
        }
    }

    // After the band's action: with a league table row that is in use and a high patience counter, the board spends.
    void tail() {
        // 0x10101924..0x10101928: every path here tested the same register (the byte `acted`) before, and returned when it was 0.
        if (!acted) throw std::logic_error("0x10101400: the tail is reached without a day to act");
        const auto league = c.u32(club + club_field::league);
        if (league == 0) return;
        const auto table = c.u32(c.toc(-0x7b5c));
        if (c.u32(table + (c.u32(league) << 2)) == 0) return;
        if (c.i8(account + account_field::counter) <= 0x32) return;
        const auto board = c.u32(club + club_field::board);
        if (board != 0 && c.i8(board + 0x4d) != 0 && c.i16(club + club_field::rep) < 0x109a) return calm_the_board();
        spend();
    }

    // The board's flag is cleared with news, and the manager's patience byte wears down.
    void calm_the_board() {
        h.sync();
        invoke_unit(c, calm_news, 0x1010199c, {manager, club, 0, 7});
        h.set(24, 0);
        c.put8(c.u32(club + club_field::board) + 0x4d, 0);
        const auto patience = std::int8_t(patience_byte());
        if (patience == 0x14) {
            const auto wear_by = std::int8_t(std::int32_t(sample(h, 0x101019d8)) % 4 + 1);
            c.put8(club + club_field::patience, patience_byte() - u(wear_by));
            return;
        }
        if (patience <= 1) return;
        h.set(23, u(patience));
        std::int32_t drawn = 0;
        if (patience != 0) drawn = draw_below(h, {24, 0x10101a50, 0}, patience);
        if (drawn == 0) return;
        c.put8(club + club_field::patience, patience_byte() - 1u);
    }

    // The board spends a tenth of its funds on the club (or on its debts), when the draw below the reputation allows.
    void spend() {
        const auto board = c.u32(club + club_field::board);
        if (board == 0) return;
        if (c.i32(board + 4) <= 0xabe) return;
        const auto rep = std::int32_t(c.i16(club + club_field::rep));
        h.set(23, u(rep));
        std::int32_t drawn = 0;
        if (rep != 0) drawn = draw_below(h, {24, 0x10101bd8, 0}, rep);
        else h.set(24, 0);
        const auto funds = c.u32(club + club_field::board);
        const auto funds_word = c.i32(funds + 4);
        if (drawn >= funds_word) return;
        const auto tenth = fpu.fctiwz(0, fpu.fmul(k(0xe0), to_double(fpu, c, funds_word)));
        const auto halfword = std::int32_t(std::int16_t(std::uint32_t(tenth)));
        const auto amount = std::int32_t(std::int16_t(halfword / 10 * 10));
        h.set(21, u(amount));
        const auto owed = c.i32(funds + 8);
        h.sync();
        if (owed > amount) {
            c.put32(funds + 8, u(owed) - u(amount));  // wraps in a word
            invoke_unit(c, spend_news, 0x10101d8c, {manager, club, u(amount), 1});
        } else {
            invoke_unit(c, spend_news, 0x10101da4, {manager, club, u(amount), 0});
        }
        auto row = c.u32(club + club_field::board);
        c.put32(row + 4, c.u32(row + 4) - u(amount));
        row = c.u32(club + club_field::board);
        c.put32(row + 0xc, c.u32(row + 0xc) + u(amount));
        c.put8(account + account_field::counter, 0x14);
        const auto patience = std::int8_t(patience_byte());
        if (patience > 0xf) {
            const auto wear_by = std::int8_t(std::int32_t(sample(h, 0x10101dfc)) % 5 + 1);
            c.put8(club + club_field::patience, patience_byte() - u(wear_by));
            return;
        }
        if (patience > 7) {
            c.put8(club + club_field::patience, u(patience) - 1u);
            return;
        }
        if (patience <= 1) return;
        h.set(23, u(patience));
        std::int32_t woke = 0;
        if (patience != 0) woke = draw_below(h, {20, 0x10101e98, 0}, patience);
        else h.set(20, 0);
        if (woke != 0) c.put8(club + club_field::patience, patience_byte() - 1u);
    }

    Cpu& c;
    Held h;
    Fpu fpu;
    std::uint32_t manager, club, account = 0;
    bool acted;
};
}

void board_review(Cpu& c, std::uint32_t manager, std::uint32_t club, std::uint32_t acted) {
    Frame frame(c, 0x10101400, 0x150, {.first_gpr = 20});
    Review(c, manager, club, acted).run();
}
}
