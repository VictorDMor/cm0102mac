// FINANCE.CPP, block board: the draws, the held registers and the income booking the board's routines share
// (finance_board_engine.hpp).
#include "finance_board_engine.hpp"

namespace cm::aot::finance::board {
namespace {
constexpr std::uint32_t draw_helper = 0x100f2100, refresh = 0x100f5770, news = 0x10101ff0;
constexpr LedgerLine donations{0xb4, false, true};

std::int32_t divide(std::int32_t a, std::int32_t b) { return std::int32_t(std::int64_t(a) / b); }
// The compiler's signed remainder (a divw, mullw, subf); a divide by 0 leaves the dividend (the product with the
// undefined quotient is 0).
std::int32_t remainder(std::int32_t a, std::int32_t b) {
    return b == 0 ? a : std::int32_t(std::uint32_t(a) - std::uint32_t(divide(a, b)) * std::uint32_t(b));
}

// `real` reads the limit through fctiwz of `value` each time the original does; otherwise it is `fixed`.
std::int32_t below(Held& h, const Below& site, bool real, double value, std::int32_t fixed) {
    Cpu& c = h.c;
    guest::Fpu fpu{c};
    const auto sample_word = std::int32_t(sample(h, site.redraw_back));
    const auto fetch = [&]() -> std::int32_t { return real ? i(std::uint32_t(fpu.fctiwz(0, value))) : fixed; };
    auto limit = fetch();
    bool small = false;
    if (limit <= 0xffff) {
        limit = fetch();
        small = limit >= -0xffff;
    }
    if (small) {
        const auto rest = remainder(sample_word, limit);
        const auto result = limit > 0 ? rest : i(0u - u(rest));
        h.set(site.dest, u(result));
        return result;
    }
    if (site.helper_back == 0) throw std::logic_error("finance board: a draw below a limit past 0xffff at a site that cannot reach it");
    limit = fetch();
    const auto scale = fpu.fdiv(to_double(fpu, c, limit), pool_double(c, pool::scale));
    const auto place = remainder(sample_word, 0xffff);
    const auto share = i(std::uint32_t(fpu.fctiwz(0, fpu.fmul(scale, to_double(fpu, c, place)))));
    const auto tenth = divide(i(u(limit) * 10u), 0xffff);
    h.set(site.dest, u(share));
    h.sync();
    const auto inner = i(invoke_unit(c, draw_helper, site.helper_back, {u(tenth)}));
    limit = fetch();
    const auto top = i(u(limit) - 1u);
    auto result = i(u(share) + u(divide(i(u(inner) + 5u), 10)));
    if (result > top) result = top;
    h.set(site.dest, u(result));
    return result;
}
}

std::uint32_t sample(Held& h, std::uint32_t back) {
    Cpu& c = h.c;
    const auto cursor_cell = c.toc(sample_table::cursor), base_cell = c.toc(sample_table::base);
    const auto next = c.u32(cursor_cell) + 2;
    c.put32(cursor_cell, next);
    if (next > c.u32(c.toc(sample_table::last))) {
        c.put32(cursor_cell, c.toc(sample_table::first));
        h.sync();
        c.call(sample_table::redraw, back);
        c.put32(base_cell, c.r(3) & 0xffff);
    }
    return std::uint32_t(c.u16(c.u32(cursor_cell))) + c.u32(base_cell);
}

bool multiple_of(Held& h, std::uint32_t back, std::int32_t modulus) { return std::int32_t(sample(h, back)) % modulus == 0; }

std::int32_t draw_below(Held& h, const Below& site, std::int32_t limit) { return below(h, site, false, 0.0, limit); }
std::int32_t draw_below_real(Held& h, guest::Fpu&, const Below& site, double limit) { return below(h, site, true, limit, 0); }

void book_donation(Cpu& c, std::uint32_t account, std::uint32_t amount) {
    book(c, account, donations, std::uint64_t(std::int64_t(i(amount))));
}

void announce(Held& h, std::uint32_t manager, std::uint32_t club, std::uint32_t amount, std::uint32_t refreshed,
              std::uint32_t kind, std::uint32_t back) {
    h.sync();
    invoke_unit(h.c, news, back, {manager, club, amount, refreshed, kind});
}

std::uint32_t refresh_after_booking(Held& h, std::uint32_t account, std::uint32_t back) {
    h.sync();
    invoke_unit(h.c, refresh, back, {account, 0, 0, 0, 1, 0});
    return h.c.r(4);
}
}
