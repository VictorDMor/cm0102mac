#include "match_pl_movement_detail.hpp"

namespace cm::aot::match_pl {
using namespace guest;

PlayerMovement::PlayerMovement(Cpu& cpu)
    : c(cpu), frame(c), fp(c), actor{c, c.r(3)} {
    c.set({{16, c.r(4)}, {15, actor.address}, {4, 0}, {31, c.toc(-0x51e0)}, {17, c.r(5)},
           {26, c.toc(-0x51e8)}, {18, c.r(6)}, {27, c.toc(-0x7b90)}, {19, c.r(7)}, {28, c.toc(-0x7b8c)},
           {20, c.r(8)}, {29, c.toc(-0x7b88)}, {13, 0}, {30, c.toc(-0x7b84)}, {14, c.toc(-0x51e4)}});
    frame.enter();
}
}

namespace cm::aot::match_pl {
namespace {
struct MovementTailReturn {};

void run_movement(Cpu& c) {
    constexpr std::uint32_t entry = 0x1022e7e0;
    constexpr std::uint32_t noncarrier = 0x10234e40;
    constexpr std::uint32_t contact = 0x102303fc;
    constexpr std::uint32_t repeated = 0x102338f8;
    constexpr std::uint32_t ratings = 0x10234a64;
    constexpr std::uint32_t epilogue = 0x10235d88;
    const auto caller = c.lr();

    c.s.step(entry);
    PlayerMovement movement(c);
    const auto path = movement.prepare();
    if (path == MovementPhase::returned) return;

    auto phase = path == MovementPhase::carrier ? 0x1022ec48u : noncarrier;

    for (;;) {
        if (phase == caller) {
            // A live table can target the caller directly, bypassing the epilogue.
            c.s.pc = phase;
            throw MovementTailReturn{};
        }
        switch (phase) {
        case noncarrier:
            movement.move_without_ball();
            movement.consume_move_condition();
            movement.finish_without_ball();
            return;
        case epilogue:
            return;
        case 0x1022ec48:
            movement.sample_carrier();
            [[fallthrough]];
        case 0x1022f514:
            movement.sample_carrier_environment();
            [[fallthrough]];
        case 0x1022fa58:
            movement.prepare_carrier_interaction();
            [[fallthrough]];
        case 0x1022ffe0:
            phase = movement.decide_carrier_contact() ? contact : 0x10231768;
            break;
        case 0x102316c8: case 0x10231708:
            movement.resume_carrier_contact_outcome(phase == 0x102316c8);
            [[fallthrough]];
        case 0x10231768:
            if (!movement.finish_carrier_possession()) {
                phase = noncarrier;
                break;
            }
            [[fallthrough]];
        case 0x10231c04:
            if (!movement.prepare_carrier_play()) {
                phase = repeated;
                break;
            }
            [[fallthrough]];
        case 0x10231e60:
            phase = movement.choose_carrier_play();
            break;
        case contact:
        case 0x10230420: case 0x1023044c: case 0x10230478: case 0x102304a4:
        case 0x102304d0: case 0x102304fc: case 0x10230528: case 0x10230554:
        case 0x10230580: case 0x102305ac: case 0x102305d8: case 0x10230604:
            phase = movement.choose_contact_action(phase);
            break;
        case 0x10230a20:
            movement.follow_up_carrier_contact();
            [[fallthrough]];
        case 0x102311b4:
            movement.apply_carrier_restart();
            return;
        case 0x10232354:
            if (movement.continue_kind17_contact()) movement.continue_kind17_clear();
            phase = noncarrier;
            break;
        case 0x102327b4:
            movement.continue_kind17_clear();
            phase = noncarrier;
            break;
        case 0x10232e30:
            movement.continue_kind18();
            phase = noncarrier;
            break;
        case 0x10233830: case 0x10233890:
            movement.continue_simple_play(phase);
            phase = noncarrier;
            break;
        case repeated:
        case 0x10233b1c: case 0x10233b48: case 0x10233b74: case 0x10233ba0:
        case 0x10233bcc: case 0x10233bf8: case 0x10233c24: case 0x10233c50:
        case 0x10233c7c: case 0x10233ca8: case 0x10233cd4: case 0x10233d00:
        case 0x10233d9c: case 0x10233dd8: case 0x10233e14:
            phase = movement.continue_repeated_contact(phase);
            break;
        case 0x10233f7c:
            phase = movement.continue_carrier_transfer();
            break;
        case 0x102342c4: case 0x102346e4:
            movement.continue_carrier_distance(phase);
            phase = ratings;
            break;
        case 0x102346f8:
            movement.continue_transfer_statistics();
            [[fallthrough]];
        case ratings:
            movement.continue_transfer_rating();
            return;
        case 0x10234dec:
            movement.continue_null_contact();
            phase = noncarrier;
            break;
        default:
            if (phase > entry && phase < 0x10235da4)
                throw std::runtime_error("Unsupported movement continuation " + hex_address(phase));
            // Branch through the live table without changing the link register.
            // Only the external destination executes through the dispatcher.
            const auto continuation = c.lr();
            c.s.pc = phase;
            run_until(c.s, continuation);
            phase = c.s.pc;
            break;
        }
    }
}
}

void move_player(Cpu& c) {
    try {
        run_movement(c);
    } catch (const MovementTailReturn&) {
        return;
    }
    c.finish();
}
}
