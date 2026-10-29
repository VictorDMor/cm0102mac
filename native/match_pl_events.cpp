#include "match_pl_detail.hpp"

namespace cm::aot::match_pl {
using namespace guest;

std::uint32_t allows_event(Cpu& c, std::uint32_t player, std::uint32_t event_word) {
    Frame frame(c, 0x10289d50, 0x80, {.first_gpr = 24});
    const auto calendar = c.toc(-0x7b80), records = c.toc(-0x7b94), seed = c.toc(-0x7b90),
               begin = c.toc(-0x7b8c), end = c.toc(-0x7b88), cursor = c.toc(-0x7b84);
    keep(c, {{24, event_word}, {25, calendar}, {26, end}, {27, begin}, {28, records},
             {29, seed}, {30, cursor}, {31, player}});
    const Player actor{c, player};
    const auto copy_date = [&](unsigned from, unsigned to) {
        c.put16(frame.at(to), c.u16(frame.at(from)));
        c.put16(frame.at(to + 2), c.u16(frame.at(from + 2)));
        c.put8(frame.at(to + 4), c.u8(frame.at(from + 4)));
    };
    leave(c, {{3, frame.at(0x40)}, {4, calendar}, {5, 7}}); c.call(0x10717b20, 0x10289d94);
    c.set(7, extended(c.i16(frame.at(0x40)))); c.set(6, c.u16(frame.at(0x42))); copy_date(0x40, 0x50);
    leave(c, {{3, frame.at(0x38)}, {4, calendar}, {5, 30}}); c.call(0x10717b20, 0x10289dc0);
    c.set(5, extended(c.i16(frame.at(0x38)))); c.set(4, c.u16(frame.at(0x3a))); copy_date(0x38, 0x48);
    const auto event = std::int16_t(event_word);
    bool within = compare(c, event, 8000) >= 0;
    if (within) within = compare(c, event, 8676) <= 0;
    c.set(3, within); if (compare(c, c.r(3)) == 0) return result(c, 0);
    c.set(4, actor.match());
    c.set(3, actor.match() + std::uint32_t(std::int8_t(actor.side()) * 1000 + event));
    if (compare(c, c.i8(c.r(3) - 7432)) != 0) return result(c, 0);
    if (compare_logical(c, c.u32(player + 0x6c)) != 0) {
        c.set(3, c.toc(-0x7be4)); c.set(4, c.u32(c.u32(player + 0x6c)));
        if (compare(c, std::int32_t(c.r(4)), c.i32(c.r(3))) >= 0) return result(c, 0);
    }
    const auto index = std::uint32_t(std::int32_t(event) - 0x219e);
    if (compare_logical(c, index, 26) > 0) return result(c, 1);
    const auto high_product = [](std::uint32_t value, std::int32_t multiplier) {
        return std::int32_t(std::uint64_t(std::int64_t(std::int32_t(value)) * multiplier) >> 32);
    };
    const auto shifted_sign = [&](std::int32_t high, unsigned bits) {
        c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & ((1u << bits) - 1)));
        return std::uint32_t(high < 0);
    };
    const auto time_units = [&](std::int16_t value) {
        shifted_sign(high_product(extended(value), 0x2e8ba2e9), 1);
        return std::int32_t(value) / 11;
    };
    const auto relative_time = [&] {
        const auto time = c.i16(actor.match() + 0x7948); const auto status = c.i8(player + 0x1f);
        leave(c, {{4, std::uint32_t(std::int32_t(status))}, {5, extended(time)}});
        return time_units(time) - status;
    };
    const auto common_status = [&] {
        return compare_logical(c, c.u8(player + 0xc)) == 0 && compare_logical(c, c.u8(player + 0x10)) == 0 &&
               compare(c, c.i8(player + 0x17), 8) <= 0 && compare_logical(c, c.u8(player + 3), 2) <= 0;
    };
    const auto condition_within_limit = [&] {
        auto limit = relative_time() * 33 + 4000;
        if (compare(c, limit, 6500) > 0) limit = 6500;
        c.set(3, std::uint32_t(limit));
        return compare(c, c.i16(player + 0x36), limit) <= 0;
    };
    const auto query = [&](unsigned criterion, unsigned date, std::uint32_t back) {
        leave(c, {{3, c.u32(records)}, {4, c.u32(player + 0x6c)}, {5, c.u32(player + 0xfc)},
                  {6, UINT32_MAX}, {7, std::uint32_t(std::int32_t(std::int8_t(criterion)))}, {8, frame.at(date)}});
        c.call(0x1069e700, back);
        return compare(c, std::int8_t(c.r(3))) != 0;
    };
    const auto criteria = [&](unsigned first, unsigned last, std::uint32_t back) {
        c.set(24, first);
        while (compare(c, std::int8_t(c.r(24)), last) <= 0) {
            if (query(c.r(24), 0x50, back)) return true;
            c.set(24, c.r(24) + 1);
        }
        return false;
    };
    const auto advance = [&](std::uint32_t back) {
        const auto next = c.u32(cursor) + 2; c.set(3, next);
        const auto order = compare_logical(c, next, c.u32(end)); c.put32(cursor, next);
        if (order > 0) { c.put32(cursor, begin); c.call(0x1077b770, back); c.put32(seed, c.r(3) & 0xffff); }
    };
    const auto sample = [&] { return std::uint32_t(c.u16(c.u32(cursor))) + c.u32(seed); };
    const auto random_condition = [&](int base, int divisor, unsigned shift, std::uint32_t back) {
        advance(back); const auto value = sample();
        shifted_sign(high_product(value, 0x10624dd3), shift);
        const auto residual = std::int32_t(value) % divisor;
        leave(c, {{3, extended(c.i16(player + 0x36))}, {4, std::uint32_t(residual)}, {5, value}});
        return compare(c, std::int32_t(c.r(3)), base + residual);
    };
    switch (event) {
    case 0x219e:
        if (!common_status() || !condition_within_limit()) return result(c, 0);
        return result(c, criteria(0, 29, 0x10289f64) || criteria(30, 39, 0x10289fa8));
    case 0x21b1:
    case 0x21b2:
        if (!common_status() || !condition_within_limit()) return result(c, 0);
        return result(c, event == 0x21b1 ? query(0x34, 0x50, 0x1028a0b0) : query(0x33, 0x48, 0x1028a1a4));
    case 0x21b7:
        if (compare(c, c.i8(player + 0x31)) == 0 || !common_status()) return result(c, 0);
        if (compare(c, relative_time(), 55) < 0 || !condition_within_limit()) return result(c, 0);
        return result(c, 1);
    case 0x219f:
        if (random_condition(7000, 2000, 7, 0x1028a2e8) < 0) return result(c, 0);
        return result(c, criteria(0, 29, 0x1028a35c));
    case 0x21a0: {
        const auto staff = c.u32(player + 0x6c);
        leave(c, {{3, calendar}, {4, c.u32(staff + 0x4c)}, {5, c.u32(staff + 0x50)}}); c.call(0x10718130, 0x1028a394);
        if (compare(c, std::int32_t(c.r(3)), 90) < 0 || random_condition(7000, 2000, 7, 0x1028a3c4) <= 0) return result(c, 0);
        leave(c, {{3, c.u32(records)}, {4, c.u32(player + 0x6c)}}); c.call(0x10658370, 0x1028a418);
        return result(c, compare(c, std::int8_t(c.r(3))) != 0);
    }
    case 0x21a1:
    case 0x21a2: {
        c.set(3, actor.match() + 0x10000);
        if (compare_logical(c, player, c.u32(c.r(3) - 0x1fd8)) != 0) return result(c, 0);
        c.set(3, actor.match() + 0x10000);
        if (compare_logical(c, c.u32(c.r(3) - 0x1fdc)) == 0) return result(c, 0);
        if (event == 0x21a1) {
            if (compare_logical(c, c.u8(player + 0xa), 4) <= 0 || compare(c, c.i16(player + 0x3a), 250) <= 0) return result(c, 0);
        } else if (compare_logical(c, c.u8(player + 0xa), 3) >= 0 || compare_logical(c, c.u8(player + 0xa)) == 0) return result(c, 0);
        c.set(4, actor.match()); c.set(24, extended(c.i16(c.r(4) + 0x7948)));
        advance(event == 0x21a1 ? 0x1028a490 : 0x1028a594);
        const auto value = sample(); const auto time = time_units(std::int16_t(c.r(24)));
        if (event == 0x21a1) {
            const auto status = c.i8(player + 0x1f); const auto divisor = c.u8(player + 0xa);
            const auto ratio = (time - status) / divisor, residual = std::int32_t(value) % 6;
            leave(c, {{3, std::uint32_t(ratio)}, {4, std::uint32_t(residual)}, {5, std::uint32_t(std::int32_t(status))},
                      {6, 0x2aaaaaab}, {7, value}});
            if (compare(c, ratio, residual + 12) >= 0) return result(c, 0);
            return result(c, query(0x52, 0x50, 0x1028a51c));
        }
        const auto high = high_product(value, 0x51eb851f), residual = std::int32_t(value) % 25;
        leave(c, {{3, std::uint32_t(time)}, {4, std::uint32_t(residual)}, {5, std::uint32_t(high < 0)}, {6, value}});
        if (compare(c, time, residual + 50) <= 0) return result(c, 0);
        const auto elapsed = relative_time(); c.set(5, c.r(4));
        c.set(4, std::uint32_t(high_product(extended(c.i16(actor.match() + 0x7948)), 0x2e8ba2e9) < 0));
        c.set(3, std::uint32_t(elapsed));
        if (compare(c, elapsed / c.u8(player + 0xa), 30) <= 0) return result(c, 0);
        return result(c, query(0x52, 0x50, 0x1028a64c));
    }
    case 0x21b0:
    case 0x21b3:
    case 0x21b5: {
        const auto back = event == 0x21b0 ? 0x1028a67cu : event == 0x21b3 ? 0x1028a710u : 0x1028a85cu;
        if (random_condition(8000, 1000, 6, back) <= 0) return result(c, 0);
        if (event == 0x21b5) return result(c, compare(c, c.i8(player + 0x31)) != 0);
        return result(c, event == 0x21b0 ? query(0x34, 0x50, 0x1028a6e0) : query(0x33, 0x48, 0x1028a774));
    }
    case 0x21b4:
        if (random_condition(7000, 2000, 7, 0x1028a7a4) < 0) return result(c, 0);
        return result(c, criteria(40, 59, 0x1028a818));
    case 0x21b6:
        if (random_condition(7250, 2000, 7, 0x1028a8d8) <= 0 || compare_logical(c, c.u8(player + 0xc), 1) != 0 ||
            compare(c, relative_time(), 65) <= 0 || compare(c, c.i8(player + 0x31)) == 0) return result(c, 0);
        c.set(4, actor.match() + std::uint32_t(std::int8_t(actor.side()) * 1000)); c.set(5, 1);
        c.put8(c.r(4) + 0x4ad, 1); return result(c, 1);
    case 0x21b8: {
        if (compare(c, c.i16(player + 0x36), 6500) >= 0) return result(c, 0);
        advance(0x1028a9c0); auto value = sample(); auto high = high_product(value, 0x55555556);
        leave(c, {{3, std::uint32_t(std::int32_t(value) % 3)}, {4, std::uint32_t(high)}, {5, value}});
        if (compare(c, c.u8(player + 0x14), std::int32_t(c.r(3))) <= 0) return result(c, 0);
        if (compare_logical(c, actor.match()) != 0) {
            leave(c, {{3, actor.match()}, {4, player}}); c.call(0x101e3d50, 0x1028aa18);
        } else c.set(3, 0);
        if (compare(c, c.r(3) & 4) == 0) return result(c, 0);
        advance(0x1028aa48); value = sample();
        const auto trait = c.i8(c.u32(player + 0x70) + 0x11); const auto residual = std::int32_t(value) % 4;
        leave(c, {{3, std::uint32_t(std::int32_t(trait))}, {4, std::uint32_t(residual)}, {5, (value & 3) << 30}});
        if (compare(c, trait, residual + 15) <= 0 || compare_logical(c, c.u8(player + 0xc)) != 0) return result(c, 0);
        advance(0x1028aab8); value = sample(); high = high_product(value, 0x2aaaaaab);
        leave(c, {{3, std::uint32_t(std::int32_t(value) % 6)}, {4, std::uint32_t(high)}, {5, value}});
        if (compare(c, c.i8(player + 0x17), std::int32_t(c.r(3))) >= 0) return result(c, 0);
        c.set(4, actor.match()); c.set(24, extended(c.i16(c.r(4) + 0x7948))); advance(0x1028ab24);
        const auto word = c.u16(c.u32(cursor)); value = std::uint32_t(word) + c.u32(seed);
        shifted_sign(high_product(value, 0x51eb851f), 4);
        const auto status = c.i8(player + 0x1f); const auto elapsed = time_units(std::int16_t(c.r(24))) - status;
        leave(c, {{3, std::uint32_t(elapsed)}, {4, std::uint32_t(std::int32_t(status))},
                  {5, std::uint32_t(std::int32_t(value) % 50)}, {6, value}, {7, word}});
        return result(c, compare(c, elapsed, std::int32_t(value) % 50 + 30) > 0);
    }
    default: return result(c, 1);
    }
}
}
