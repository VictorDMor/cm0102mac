#include "match_pl_shot_event_detail.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

static void record_flight_value(Cpu& c, std::int32_t value, std::int32_t limit = 0) {
    c.set(0, std::uint32_t(value));
    c.compare(0, value, limit);
}

static void sample_strong_shot(Cpu& c, std::int16_t power) {
    record_flight_value(c, power);
    if (c.cr(0) & 2) {
        c.set(13, 0);
        return;
    }
    advance_shot_stream(c, 0x1025b1cc);
    // Both wide guards are dominated by the signed halfword conversion.
    for (const auto limit : {65535, -65535}) {
        c.set(3, std::uint32_t(limit));
        record_flight_value(c, power, limit);
    }
    c.set(5, c.u32(c.r(31)));
    c.set(3, std::uint32_t(power));
    c.set(4, c.u32(c.r(28)));
    record_flight_value(c, power);
    c.set(0, c.u16(c.r(5)));
    c.set(4, c.r(0) + c.r(4));
    const auto quotient = std::uint32_t(divide_signed<4>(c.r(4), c.r(3)));
    const auto remainder = c.r(4) - quotient * c.r(3);
    c.set(13, remainder);
    c.set(0, power > 0 ? remainder : 0u - remainder);
    c.set(13, c.r(0));
}

static bool random_flight_variant(Cpu& c, std::uint32_t back) {
    advance_shot_stream(c, back);
    c.set(3, c.u32(c.r(31)));
    c.set(0, c.u32(c.r(28)));
    const auto sample = c.u16(c.r(3)) + c.r(0);
    const auto sign = sample >> 31;
    c.set(3, sign);
    record_flight_value(c, std::int32_t(((sample + sign * 3) & 3) - sign * 3));
    return (c.cr(0) & 2) != 0;
}

static unsigned classify_weaker_shot(Cpu& c, std::int16_t power) {
    record_flight_value(c, power, 10);
    const auto variant = [&] {
        record_flight_value(c, std::int8_t(c.r(21)));
        return (c.cr(0) & 2) == 0;
    };
    if (c.cr(0) & 8) {
        if (!variant()) return 2;
        return random_flight_variant(c, 0x1025b380) ? 4 : 3;
    }
    const auto test_gap = [&](int limit) {
        c.set(3, std::uint32_t(power));
        record_flight_value(c, std::int16_t(c.r(13)) - power, limit);
        return (c.cr(0) & 4) != 0;
    };
    if (test_gap(60)) {
        if (!variant()) return 5;
        return random_flight_variant(c, 0x1025b404) ? 7 : 6;
    }
    if (test_gap(25)) return variant() ? 9 : 8;
    if (test_gap(14)) return variant() ? 11 : 10;
    return variant() ? 13 : 12;
}

void classify_shot_flight(Cpu& c) {
    const auto power = std::int16_t(c.r(16));
    const auto required = std::int16_t(c.r(13));
    c.set(3, std::uint32_t(power));
    record_flight_value(c, required);
    c.compare(0, power, required);
    if (!(c.cr(0) & 4)) c.set(13, classify_weaker_shot(c, power));
    else {
        sample_strong_shot(c, power);
        c.compare(0, std::int32_t(c.r(13)), 10);
        if (c.cr(0) & 8) {
            record_flight_value(c, std::int8_t(c.r(21)));
            c.set(13, (c.cr(0) & 2) ? 14 : 15);
        } else c.set(13, 1);
    }
    c.s.pc = 0x1025b4b8;
}
}
