#include "match_pl_shot_event_detail.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

namespace {
constexpr std::uint32_t restart = 0x10262154;

struct Narration {
    std::uint32_t sample_refill;
    std::uint32_t single_recipient;
    std::uint32_t double_refill;
    std::uint32_t general_message;
};

constexpr Narration pressured{0x10260308, 0x102604ec, 0x10260728, 0x10260bd4};
constexpr Narration keeper{0x10260f64, 0x10261148, 0x10261384, 0x10261830};
constexpr Narration ordinary{0x10261888, 0x10261a6c, 0x10261ca8, 0x10262154};

int compare_narration(Cpu& c, std::int32_t value, std::int32_t bound = 0) {
    c.set(0, std::uint32_t(value));
    c.compare(0, value, bound);
    return value < bound ? -1 : value > bound ? 1 : 0;
}

void describe_contextual_attempt(Cpu& c, std::uint32_t actor, std::uint32_t stack, const Narration& narration) {
    if (compare_narration(c, c.i16(stack + 0x12c), -135) >= 0 ||
        compare_narration(c, c.u8(stack + 0x1cf), 0x3a) == 0) return;

    draw_miss_context(c, narration.sample_refill);
    const auto outcome = std::int32_t(c.r(13));
    c.compare(0, outcome, 0);
    if (!outcome) return;

    const auto recipient = std::int8_t(c.r(18));
    const auto rank = compare_narration(c, recipient, 2);
    if (rank == 0) {
        report_double_shot_context(c, narration.double_refill);
        return;
    }
    if (rank < 0) {
        c.compare(0, recipient, 1);
        if (recipient >= 1) {
            report_single_shot_context(c, narration.single_recipient);
            return;
        }
    }
    report_shot_event(c, actor, 0x2132, narration.general_message);
}

bool is_special_power_attempt(Cpu& c, std::uint32_t actor, std::uint32_t stack) {
    if (compare_narration(c, c.i16(actor + 0x3a), 350) > 0 &&
        compare_narration(c, c.u8(stack + 0x1cf), 0x3a) != 0) return true;
    return compare_narration(c, c.i16(actor + 0x3a), 175) > 0 &&
           compare_narration(c, c.u8(stack + 0x1cf), 0x3a) == 0;
}

void report_special_power_attempt(Cpu& c, std::uint32_t actor) {
    const auto type = c.u8(actor + 0xc);
    c.set({{0, type}, {3, 0x1fdd}});
    c.set(10, c.u8(actor + 0xc));
    c.compare_logical(0, type, 2);
    c.set(7, c.u8(actor + 0x27));
    c.set(6, c.u8(actor + 0x2f));
    c.set(5, c.u32(actor + 0x1a4));
    if (type == 2) c.set(3, 0x1fdf);
    c.set(8, c.u32(actor + 0x1a4));
    c.set({{4, c.r(3)}, {9, UINT32_MAX}});
    c.set(3, c.u32(c.r(8) + 0x79f4));
    c.set(8, UINT32_MAX);
    c.call(0x10201180, 0x10260c6c);
}

bool describes_keeper_attempt(Cpu& c, std::uint32_t stack) {
    if (compare_narration(c, c.i8(stack + 0x45)) == 0 ||
        compare_narration(c, c.i8(stack + 0x46)) != 0) return false;
    c.set(3, c.r(24));
    call_shot_owner(c, 0x10289560, 0x10260ecc);
    if (compare_narration(c, std::int16_t(c.r(3)), 1) != 0) return false;
    const auto action = std::uint8_t(c.r(13) - 5);
    c.set(0, action);
    c.compare_logical(0, action, 2);
    return action > 2;
}

void report_keeper_attempt(Cpu& c, std::uint32_t actor) {
    const auto recipient = c.r(24);
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, 0x20d5);
    c.set(7, c.u8(recipient + 0x27));
    c.set(8, UINT32_MAX);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(9, UINT32_MAX);
    c.set(6, c.u8(recipient + 0x2f));
    c.set(10, 0);
    c.set(5, c.u32(actor + 0x1a4));
    c.call(0x10201180, 0x10260f10);
}
}

void describe_on_target_miss(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp();
    if (has_shot_pressure(c, actor, stack)) {
        describe_contextual_attempt(c, actor, stack, pressured);
        report_shot_event(c, actor, 0x201f, 0x10260bfc);
    } else if (is_special_power_attempt(c, actor, stack)) {
        report_special_power_attempt(c, actor);
        report_available_miss_message(c, actor, 0x10260c78);
    } else if (compare_narration(c, c.i16(actor + 0x3a), 250) > 0 ||
               compare_narration(c, c.u8(stack + 0x1cf), 0x3a) == 0) {
        report_power_miss(c, actor, 0x1fdd, 0x10260dac);
        report_available_miss_message(c, actor, 0x10260db8);
    } else if (describes_keeper_attempt(c, stack)) {
        report_keeper_attempt(c, actor);
        describe_contextual_attempt(c, actor, stack, keeper);
    } else {
        describe_contextual_attempt(c, actor, stack, ordinary);
    }
    c.s.pc = restart;
}
}
