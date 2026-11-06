#include "match_pl_shot_event_detail.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

void report_power_miss(Cpu& c, std::uint32_t actor, unsigned event, std::uint32_t back) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, event);
    c.set(10, c.u8(actor + 0xc));
    c.set(8, UINT32_MAX);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(9, UINT32_MAX);
    c.set(7, c.u8(actor + 0x27));
    c.set(6, c.u8(actor + 0x2f));
    c.set(5, c.u32(actor + 0x1a4));
    c.call(0x10201180, back);
}

void describe_power_miss(Cpu& c) {
    const auto actor = c.r(15);
    const auto power = c.i16(actor + 0x3a);
    c.set(0, std::uint32_t(power));
    c.compare(0, power, 350);
    if (power > 350) {
        const auto type = c.u8(actor + 0xc);
        c.set(0, type);
        c.compare_logical(0, type, 2);
        if (type == 2) {
            c.set(4, c.u32(actor + 0x1a4));
            c.set(3, 0x2e8ba2e9);
            const auto clock = c.i16(c.r(4) + 0x7948);
            const auto high = std::int32_t((std::int64_t(clock) * 0x2e8ba2e9) >> 32);
            const auto shifted = high >> 1;
            c.s.wr<1>(reg::xer_ca, high < 0 && (high & 1));
            c.set(3, shifted < 0);
            const auto minutes = shifted + std::int32_t(c.r(3));
            c.set(0, std::uint32_t(minutes));
            c.compare(0, minutes, 50);
            if (minutes > 50) {
                report_power_miss(c, actor, 0x1fdf, 0x1025e738);
                c.s.pc = 0x1025f858;
                return;
            }
        }
        report_power_miss(c, actor, 0x1fde, 0x1025e764);
        report_available_miss_message(c, actor, 0x1025e770);
        c.s.pc = 0x1025f858;
        return;
    }

    const auto current_power = c.i16(actor + 0x3a);
    c.set(0, std::uint32_t(current_power));
    c.compare(0, current_power, 250);
    if (current_power <= 250) {
        c.s.pc = 0x1025ec74;
        return;
    }
    report_power_miss(c, actor, 0x1fde, 0x1025e898);
    c.s.pc = report_available_miss_message(c, actor, 0x1025e8a4) ? 0x1025f858 : 0x1025e998;
}

static void report_miss_end_message(Cpu& c, std::uint32_t actor, unsigned event, std::uint32_t back) {
    c.set({{3, actor}, {4, event}});
    call_shot_owner(c, 0x10289d50, back);
    const auto result = std::int8_t(c.r(3));
    c.set(0, std::uint32_t(result));
    c.compare(0, result, 0);
    if (result) report_shot_event(c, actor, event, back + 0x30);
}

static void place_miss_restart(Cpu& c, std::uint32_t actor, std::uint32_t back) {
    const auto side = c.u8(actor + 0x27);
    c.set(0, side);
    c.compare(0, side, 1);
    if (side == 1) {
        c.set(3, c.u32(actor + 0x1a4));
        c.set(0, UINT32_MAX);
        c.put8(c.r(3) + 0x791e, 0xff);
    } else {
        const auto current_side = c.i8(actor + 0x27);
        c.set(0, std::uint32_t(current_side));
        c.compare(0, current_side, 0);
        if (!current_side) {
            c.set(3, c.u32(actor + 0x1a4));
            c.set(0, 12);
            c.put8(c.r(3) + 0x791e, 12);
        }
    }
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, c.u8(actor + 0x27));
    c.call(0x101f0c00, back);
    c.s.pc = 0x1026a190;
}

void finish_close_miss(Cpu& c) {
    const auto actor = c.r(15);
    report_miss_end_message(c, actor, 0x21a1, 0x1025f864);
    report_miss_end_message(c, actor, 0x21a2, 0x1025f8a0);
    place_miss_restart(c, actor, 0x1025f910);
}

void finish_on_target_miss(Cpu& c) {
    place_miss_restart(c, c.r(15), 0x10262194);
}
}
