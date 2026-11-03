#include "match_pl_goal_line.hpp"

namespace cm::aot::match_pl {
using namespace guest;

void GoalLineAction::announce_non_goal(int event) {
    if (opposite) {
        constexpr std::uint32_t returns[]{0x1026a624, 0x1026a650, 0x1026a67c, 0x1026a6a8, 0x1026a6d4, 0x1026a700};
        if (compare(c, event, 11) != 0) {
            if (event < 11) {
                if (compare(c, event, 9) < 0 && compare(c, event, 8) < 0) return;
            } else if (compare(c, event, 13) > 0) return;
        }
        announce(0x1fc6 + unsigned(event - 8), actor.address, false, returns[event - 8]);
    } else {
        if (compare(c, event, 13) == 0) announce(0x1fcb, actor.address, true, 0x1026dad8);
        else if (event < 13 && compare(c, event, 12) >= 0)
            announce(0x1fca, actor.address, true, 0x1026da9c);
    }
}

void GoalLineAction::keeper_recovery(std::uint32_t refill_back, std::uint32_t recovery_back, unsigned extra) {
    c.set(3, c.u32(c.r(29)));
    c.set(0, c.u32(c.r(end_register())));
    c.set(3, c.r(3) + 2);
    const bool wrap = compare_logical(c, c.r(3), c.r(0)) > 0;
    c.put32(c.r(29), c.r(3));
    if (wrap) replace_seed(refill_back);
    c.set(3, c.u32(c.r(29)));
    c.set(4, 0x55550000);
    c.set(0, c.u32(c.r(27)));
    c.set(4, 0x55555556);
    c.set(5, c.u16(c.r(3)));
    c.set(3, c.r(30));
    c.set(5, c.r(5) + c.r(0));
    const auto high = std::int32_t((std::int64_t(std::int32_t(c.r(5))) * 0x55555556) >> 32);
    c.set(0, c.r(5) - 3 * std::uint32_t(high + (high < 0)) + extra);
    c.set(4, std::uint32_t(std::int8_t(c.r(0))));
    c.call(0x10285ed0, recovery_back);
}

void GoalLineAction::finish_near_goal() {
    if (stack_byte(0x38) && !stack_byte(0x39) && keeper_role(resume(0x1026a738, 0x1026db10)))
        announce(0x20d5, c.r(30), false, resume(0x1026a76c, 0x1026db44));
    announce(resume(0x211c, 0x211b), actor.address, false, resume(0x1026a794, 0x1026db6c));
    c.set(0, std::uint32_t(std::int8_t(c.r(28))));
    const bool normal_restart = compare(c, std::int32_t(c.r(0)), 12) == 0;
    c.set({{4, actor.match()}, {6, 2}, {3, actor.match()}, {8, 1}});
    c.set({{5, c.u8(c.r(4) + 0x791e)}, {9, 1}, {4, c.u8(c.r(3) + 0x791d)},
           {10, 0}, {3, actor.match()}, {7, actor.side()}});
    c.call(0x101f1100, normal_restart ? resume(0x1026a7cc, 0x1026dba4) : resume(0x1026a7fc, 0x1026dbd4));
    if (compare_logical(c, c.r(30)) != 0 && keeper_role(resume(0x1026a80c, 0x1026dbe4)))
        keeper_recovery(resume(0x1026a838, 0x1026dc10), resume(0x1026a878, 0x1026dc50));
    c.set(3, actor.match());
    c.set(0, 1);
    c.put8(c.r(3) + 0x1e6, c.r(0));
}

void GoalLineAction::keeper_contact(bool stronger) {
    const auto bound_register = opposite ? 20u : 19u;
    const auto draw_register = first_register();
    c.set(3, std::uint32_t(c.i16(c.r(30) + 0x36)));
    c.set(0, c.r(3) + (stronger ? 600 : 350));
    c.put16(c.r(30) + 0x36, c.r(0));
    c.set(0, std::uint32_t(c.i8(c.r(30) + 0x17)));
    c.set(3, c.r(0) * 20);
    const unsigned added = stronger ? 20 : 50;
    c.s.wr<1>(reg::xer_ca, c.r(3) > UINT32_MAX - added);
    const auto bound = std::int32_t(c.r(3) + added);
    c.set(bound_register, std::uint32_t(bound));
    if (compare(c, bound) == 0) c.set(draw_register, 0);
    else {
        refill_pool(stronger ? resume(0x1026ab48, 0x1026df20) : resume(0x1026a930, 0x1026dd08));
        // Sign-extended byte × 20 + 20 or 50 cannot enter the wide sampler.
        c.set(3, std::uint32_t(-65535));
        compare(c, bound, -65535);
        c.set(3, c.u32(c.r(29)));
        compare(c, bound);
        c.set(0, c.u32(c.r(27)));
        c.set(3, c.u16(c.r(3)) + c.r(0));
        const auto remainder = std::uint32_t(std::int64_t(std::int32_t(c.r(3))) % bound);
        c.set(draw_register, remainder);
        c.set(0, bound > 0 ? remainder : 0u - remainder);
        c.set(draw_register, c.r(0));
    }
    if (compare(c, std::int32_t(c.r(draw_register))) != 0) return;
    refill_pool(stronger ? resume(0x1026acac, 0x1026e084) : resume(0x1026aa94, 0x1026de6c));
    if (stronger) {
        c.set(3, c.u32(c.r(29)));
        c.set(4, 0x92490000);
        c.set(0, c.u32(c.r(27)));
        c.set(4, 0x92492493);
        c.set(5, c.u16(c.r(3)));
        c.set(3, c.r(30));
        c.set(5, c.r(5) + c.r(0));
        const auto product = std::int64_t(std::int32_t(c.r(5))) * std::int32_t(0x92492493u);
        const auto high = std::uint32_t(product >> 32) + c.r(5);
        c.s.wr<1>(reg::xer_ca, std::int32_t(high) < 0 && (high & 3));
        const auto quotient = std::int32_t(high) >> 2;
        c.set(0, c.r(5) - 7 * std::uint32_t(quotient + (quotient < 0)));
    } else {
        c.set(4, c.u32(c.r(29)));
        c.set(3, c.r(30));
        c.set(0, c.u32(c.r(27)));
        const auto word = c.u16(c.r(4)) + c.r(0);
        c.set(0, std::uint32_t(std::int32_t(word) % 4));
    }
    c.set(4, std::uint32_t(std::int8_t(c.r(0))));
    c.call(0x10288f30, stronger ? resume(0x1026acf4, 0x1026e0cc) : resume(0x1026aacc, 0x1026dea4));
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0))) != 0) {
        c.set({{3, c.r(30)}, {4, 0}});
        c.call(0x1022ba10, stronger ? resume(0x1026ad08, 0x1026e0e0) : resume(0x1026aae0, 0x1026deb8));
    }
}

void GoalLineAction::explain_keeper(const KeeperMessages& sequence) {
    c.set(0, std::uint32_t(std::int8_t(c.r(opposite ? 23 : 22))));
    if (compare(c, std::int32_t(c.r(0))) != 0) {
        announce(sequence.strong_message, c.r(30), false, sequence.strong_back);
        constexpr unsigned offers[]{0x219f, 0x21a0, 0x21b0, 0x21b3, 0x21b4, 0x21b5};
        for (unsigned i = 0; i < std::size(offers); ++i) {
            c.set({{3, c.r(30)}, {4, offers[i]}});
            c.call(0x10289d50, sequence.first_offer_back + 0x40 * i);
            c.set(0, std::uint32_t(std::int8_t(c.r(3))));
            if (compare(c, std::int32_t(c.r(0))) != 0) {
                announce(offers[i], c.r(30), false, sequence.first_message_back + 0x40 * i);
                return;
            }
        }
        return;
    }
    c.set(0, std::uint32_t(std::int8_t(c.r(opposite ? 26 : 24))));
    if (compare(c, std::int32_t(c.r(0))) != 0)
        announce(sequence.mild_message, c.r(30), false, sequence.mild_back);
    else announce(sequence.default_message, c.r(30), false, sequence.default_back);
}

void GoalLineAction::queue_goal_line_restart() {
    if (opposite) c.set(0, actor.side());
    c.set({{6, 0}, {4, actor.match()}, {8, 0}, {3, actor.match()}});
    if (opposite) c.set(0, std::int8_t(c.r(0)) == 0);
    c.set({{5, c.u8(c.r(4) + 0x791e)}, {4, c.u8(c.r(3) + 0x791d)}, {3, actor.match()},
           {9, 1}, {10, 0}, {7, opposite ? c.r(0) : actor.side()}});
    c.call(0x101f1100, resume(0x1026d0c8, 0x1026fd38));
}

void GoalLineAction::finish_close_save() {
    explain_keeper({0x2111, resume(0x1026ce18, 0x1026fac0),
                    resume(0x1026ce24, 0x1026facc), resume(0x1026ce54, 0x1026fafc),
                    0x210f, resume(0x1026cfc8, 0x1026fc70), 0x1fe6, resume(0x1026cff4, 0x1026fc9c)});
    if (opposite) announce(0x211c, actor.address, false, 0x1026d01c);
    c.set(3, std::uint32_t(c.i16(c.r(30) + 0x36)));
    c.set(0, c.r(3) + 140);
    c.put16(c.r(30) + 0x36, c.r(0));
    keeper_recovery(resume(0x1026d048, 0x1026fcc8), resume(0x1026d08c, 0x1026fd0c), 1);
    queue_goal_line_restart();
    if (!opposite) announce(0x211b, actor.address, false, 0x1026fd60);
    c.set(3, actor.match());
    c.set(0, 1);
    c.put8(c.r(3) + 0x1e6, c.r(0));
}

void GoalLineAction::finish_over_line() {
    explain_keeper({0x2111, resume(0x1026d114, 0x1026fdac),
                    resume(0x1026d120, 0x1026fdb8), resume(0x1026d150, 0x1026fde8),
                    0x210f, resume(0x1026d2c4, 0x1026ff5c), 0x1fe4, resume(0x1026d2f0, 0x1026ff88)});
    c.set(0, actor.side());
    if (compare(c, std::int32_t(c.r(0)), 1) == 0) {
        c.set(3, actor.match());
        c.set(0, UINT32_MAX);
        c.put8(c.r(3) + 0x791e, c.r(0));
    } else {
        c.set(0, std::uint32_t(std::int8_t(actor.side())));
        if (compare(c, std::int32_t(c.r(0))) == 0) {
            c.set(3, actor.match());
            c.set(0, 12);
            c.put8(c.r(3) + 0x791e, c.r(0));
        }
    }
    c.set(0, 0);
    if (opposite) {
        c.set({{5, actor.side()}, {3, actor.match()}});
        c.put8(c.r(3) + 0x7924, c.r(5));
    } else {
        c.set({{3, actor.side()}, {5, actor.match()}});
        c.set(3, std::int8_t(c.r(3)) == 0);
        c.put8(c.r(5) + 0x7924, c.r(3));
    }
    c.set(3, actor.match() + 0x10000);
    c.put32(c.r(3) - 0x2000, c.r(0));
    announce(resume(0x211c, 0x211b), actor.address, false, resume(0x1026d368, 0x1027000c));
    c.set(3, actor.match());
    c.set(0, 3);
    c.put8(c.r(3) + 0x7928, c.r(0));
    c.set({{4, 0x1f75}, {6, UINT32_MAX}, {8, UINT32_MAX}, {9, UINT32_MAX}, {10, 0}});
    if (!opposite) c.set(0, actor.side());
    c.set(3, actor.match());
    if (opposite) c.set(7, actor.side());
    else c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    if (!opposite) c.set(5, actor.match());
    c.set(3, c.u32(c.r(3) + 0x79f4));
    if (opposite) c.set(5, actor.match());
    else {
        c.set(0, c.r(0) == 0);
        c.set(7, c.r(0));
    }
    c.call(0x10201180, resume(0x1026d39c, 0x10270050));
}

void GoalLineAction::finish_keeper_possession() {
    keeper_recovery(resume(0x1026d3c0, 0x10270074), resume(0x1026d400, 0x102700b4));
    c.set(3, std::uint32_t(c.i16(c.r(30) + 0x36)));
    c.set(0, c.r(3) + 200);
    c.put16(c.r(30) + 0x36, c.r(0));
    explain_keeper({0x2112, resume(0x1026d43c, 0x102700f0),
                    resume(0x1026d448, 0x102700fc), resume(0x1026d478, 0x1027012c),
                    0x2110, resume(0x1026d5ec, 0x102702a0), 0x1fe5, resume(0x1026d618, 0x102702cc)});
    announce(resume(0x211c, 0x211b), actor.address, false, resume(0x1026d640, 0x102702f4));
    c.set(3, c.r(30));
    c.call(0x1022e3a0, resume(0x1026d648, 0x102702fc));
}

// Returns false only when the original enters its separate goal branch.
bool GoalLineAction::finish_non_goal() {
    const auto event = std::int32_t(std::int8_t(c.r(28)));
    c.set(0, std::uint32_t(event));
    if (compare(c, event, 1) == 0) return false;
    announce_non_goal(event);
    c.set(0, std::uint32_t(std::int8_t(c.r(28))));
    const auto current = std::int32_t(c.r(0));
    if (compare(c, current, 14) < 0 && compare(c, current, 12) >= 0) finish_near_goal();
    return true;
}

void GoalLineAction::prepare_goal_effects() {
    const auto rating = std::int32_t(std::int16_t(c.r(opposite ? 20 : 26)));
    const auto threshold = std::int32_t(std::int16_t(c.r(clock_register())));
    c.set(3, std::uint32_t(rating));
    const auto sum = rating + threshold;
    c.set(0, std::uint32_t(sum));
    if (compare(c, sum) == 0) c.set(28, 0);
    else {
        c.set(3, std::uint32_t(threshold));
        c.set(4, std::uint32_t(threshold * 100));
        c.set(0, std::uint32_t(threshold * 100 / sum));
        c.set(28, std::uint32_t(std::int16_t(c.r(0))));
    }
    c.set(0, std::uint32_t(std::int16_t(c.r(28))));
    if (compare(c, std::int32_t(c.r(0)), 71) > 0) return;
    c.set(0, std::uint32_t(std::int8_t(c.r(opposite ? 26 : 24))));
    if (compare(c, std::int32_t(c.r(0))) != 0 && keeper_role(resume(0x1026a8dc, 0x1026dcb4))) {
        keeper_contact(false);
        return;
    }
    c.set(0, std::uint32_t(std::int8_t(c.r(opposite ? 23 : 22))));
    if (compare(c, std::int32_t(c.r(0))) != 0 && keeper_role(resume(0x1026aaf4, 0x1026decc)))
        keeper_contact(true);
}

// The lower ratios share the saved and over-line outcome families.
bool GoalLineAction::finish_saved_goal() {
    c.set(0, std::uint32_t(std::int16_t(c.r(28))));
    if (compare(c, std::int32_t(c.r(0)), 71) > 0) return false;
    if (compare(c, std::int32_t(c.r(0)), 61) > 0) finish_close_save();
    else if (compare(c, std::int32_t(c.r(0)), 51) > 0) finish_over_line();
    else finish_keeper_possession();
    return true;
}
}
