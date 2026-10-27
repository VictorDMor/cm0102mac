#include "contract_manager_detail.hpp"

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
std::int32_t review_age(Cpu& c, std::uint32_t person) {
    const auto age = std::int32_t(std::int8_t(c.i16(person + 0x5c)));
    c.set(0, std::uint32_t(age)); return age;
}
std::int32_t review_adaptability(Cpu& c, std::uint32_t person) {
    const auto adaptability = c.i8(person + 0x63); c.set(0, std::uint32_t(std::int32_t(adaptability))); return adaptability;
}
std::uint32_t ordinary_priority(Cpu& c, std::int32_t sentiment) {
    const auto bits = (0u - std::uint32_t(sentiment)) & ~std::uint32_t(sentiment);
    c.set(0, bits); c.s.wr<1>(guest::reg::xer_ca, std::int32_t(bits) < 0 && (bits & 0x7fffffffu));
    return result(c, sentiment > 0 ? 3 : 4);
}
}

std::uint32_t review_priority(Cpu& c, std::uint32_t person, std::uint32_t contract) {
    if (!nonzero_logical(c, person)) return result(c, 7);
    const auto attributes = c.u32(person + 0x14); c.set(6, attributes);
    if (!nonzero_logical(c, attributes)) return result(c, 7);
    if (!nonzero_logical(c, contract)) return result(c, 4);
    const auto flags = c.u8(contract + 0x38);
    const auto role = std::int32_t(std::int8_t(flags)) >> 2;
    c.set(0, std::uint32_t(role)); c.s.wr<1>(guest::reg::xer_ca, role < 0 && (flags & 2));
    if (compare(c, role, 5) == 0) return result(c, 8);
    const auto squad_role = c.u8(contract + 0x60) & 15u; c.set(0, squad_role);
    if (compare_logical(c, squad_role, 6) > 0) return result(c, 7);
    const auto rules = c.toc(-0x5730); c.set(3, rules); c.set(0, squad_role * 4);
    const auto rule = c.u32(rules + squad_role * 4); c.set(3, rule); c.s.wr<4>(guest::reg::ctr, rule);
    // The original dispatch table selects seven policy rules. Read its
    // actual target, as the original does, while expressing each rule with
    // semantic values and comparisons.
    switch (rule) {
    case 0x1007c1b0: {
        const auto score = c.i8(contract + 0x44); c.set(0, std::uint32_t(std::int32_t(score)));
        if (compare(c, score, 45) > 0) return result(c, 1);
        c.set(4, 25);
        const auto carry = std::uint32_t(25) >= std::uint32_t(std::int32_t(score));
        const auto difference = std::uint64_t(0xffffffffu) + (std::uint32_t(std::int32_t(score)) >> 31) + carry;
        c.set(0, 0); c.s.wr<1>(guest::reg::xer_ca, difference >> 32);
        return result(c, std::uint32_t(difference) + 3);
    }
    case 0x1007c1e4: {
        const auto raw = c.u8(contract + 0x44); const auto score = std::int8_t(raw); c.set(7, raw); c.set(0, std::uint32_t(std::int32_t(score)));
        if (compare(c, score, 75) > 0) {
            const auto loyalty = c.i8(person + 0x65), adaptability = c.i8(person + 0x63);
            c.set(3, std::uint32_t(std::int32_t(loyalty))); c.set(5, std::uint32_t(std::int32_t(loyalty) * 10));
            const auto ability = c.i16(attributes + 12);
            const auto adjusted = std::int32_t(ability) - std::int32_t(loyalty) * 10;
            c.set(3, std::uint32_t(adjusted)); c.set(0, std::uint32_t(adjusted + std::int32_t(adaptability) * 25));
            if (compare(c, adjusted + std::int32_t(adaptability) * 25, 6250) > 0) {
                const auto age = review_age(c, person);
                if (compare(c, age, 23) > 0 && compare(c, age, 34) < 0) return result(c, 1);
            }
        }
        c.set(3, std::uint32_t(std::int32_t(score)));
        if (compare(c, score, 25) > 0) return result(c, 2);
        return ordinary_priority(c, score);
    }
    case 0x1007c268: {
        const auto raw = c.u8(contract + 0x44); const auto score = std::int8_t(raw); c.set(3, raw); c.set(0, std::uint32_t(std::int32_t(score)));
        if (compare(c, score, 55) > 0) {
            const auto age = review_age(c, person);
            if (compare(c, age, 23) > 0 && compare(c, age, 34) < 0) return result(c, 2);
        }
        return ordinary_priority(c, score);
    }
    case 0x1007c2b0: {
        const auto raw = c.u8(contract + 0x44); const auto score = std::int8_t(raw); c.set(3, raw); c.set(0, std::uint32_t(std::int32_t(score)));
        if (compare(c, score, 65) > 0) {
            const auto age = review_age(c, person);
            if (compare(c, age, 23) > 0) {
                c.set(4, 34); const auto bits = std::uint32_t(34 ^ age);
                c.set(0, (std::uint32_t(std::int32_t(bits) >> 1) - (bits & 34u)));
                c.s.wr<1>(guest::reg::xer_ca, std::int32_t(bits) < 0 && (bits & 1));
                return result(c, age < 34 ? 3 : 2);
            }
        }
        c.set(0, std::uint32_t(std::int32_t(score)));
        if (compare(c, score, -30) > 0) return result(c, 4);
        const auto age = review_age(c, person);
        return result(c, compare(c, age, 24) >= 0 ? 7 : 5);
    }
    case 0x1007c320: {
        const auto age = review_age(c, person);
        if (compare(c, age, 24) > 0) {
            if (compare(c, age, 34) > 0) return result(c, 4);
            const auto score = c.i8(contract + 0x44); c.set(0, std::uint32_t(std::int32_t(score))); c.set(3, std::uint32_t(std::int32_t(score)));
            if (compare(c, score, 75) > 0) return result(c, 2);
            if (nonzero(c, score) && score > 0) return result(c, 3);
            return result(c, compare(c, score, -30) <= 0 ? 7 : 4);
        }
        if (compare(c, review_adaptability(c, person), 5) > 0) {
            const auto ability = c.i16(attributes + 10); c.set(0, extended(ability));
            if (compare(c, ability, 1250) > 0) return result(c, 5);
        }
        return result(c, 6);
    }
    case 0x1007c3a8: {
        const auto age = review_age(c, person);
        if (compare(c, age, 23) > 0) {
            const auto score = c.i8(contract + 0x44); c.set(0, std::uint32_t(std::int32_t(score))); c.set(3, std::uint32_t(std::int32_t(score)));
            if (compare(c, score, 75) > 0) return result(c, 1);
            if (compare(c, score, 30) > 0) return result(c, 2);
            if (compare(c, score) > 0) return result(c, 3);
            return result(c, compare(c, score, -50) <= 0 ? 7 : 4);
        }
        const auto ability = c.i16(attributes + 10); c.set(0, extended(ability));
        if (compare(c, ability, 1250) > 0 && compare(c, review_adaptability(c, person), 15) > 0) return result(c, 5);
        return result(c, 6);
    }
    case 0x1007c430: {
        const auto age = review_age(c, person);
        if (compare(c, age, 21) > 0) {
            const auto score = c.i8(contract + 0x44); c.set(0, std::uint32_t(std::int32_t(score)));
            if (compare(c, score, 75) > 0) return result(c, 1);
            if (compare(c, score, 40) >= 0) return result(c, 2);
            if (compare(c, score, 10) >= 0) return result(c, 3);
            return result(c, compare(c, score, -30) <= 0 ? 7 : 4);
        }
        const auto raw = c.u8(contract + 0x44); const auto score = std::int8_t(raw); c.set(3, raw); c.set(0, std::uint32_t(std::int32_t(score)));
        if (compare(c, score) > 0) return result(c, 5);
        c.set(0, std::uint32_t(std::int32_t(score)));
        if (compare(c, score, -30) > 0) return result(c, 6);
        return result(c, compare(c, review_adaptability(c, person), 5) <= 0 ? 7 : 6);
    }
    default: throw std::runtime_error("Unknown contract review policy rule");
    }
}
}
