// 0x103baee0: the tactics screen's handler. The UI framework calls it with
// the index of the control the event concerns (r3). It first takes the
// answers of the dialogs the screen opened (file dialogs, confirmations),
// then tests the control's action, one branch per action, in the original's
// order; anything else goes to the generic handlers 0x102a54b0 / 0x102a83a0.
//
// The original keeps temporaries in non-volatile registers that differ from
// branch to branch; its callees save them into the stack image, so each
// branch sets them as the original does before its calls. Every exit stores
// its code in a stack slot of its own and destroys the stack object at
// sp+0x240 (0x1074b7c0).
#include "tactics_screens_main.hpp"
#include "tactics_leaves.hpp"
#include <array>
#include <bit>
#include <cmath>
#include <optional>

namespace cm::aot {
void original_fctiwz(State& state, unsigned destination, unsigned source);
}

namespace cm::aot::tactics_screens::main {
using guest::Cpu;
using guest::Frame;
namespace {
std::uint32_t sext8(std::uint32_t value) { return std::uint32_t(std::int32_t(std::int8_t(value))); }
std::uint32_t sext16(std::uint32_t value) { return std::uint32_t(std::int32_t(std::int16_t(value))); }
std::int32_t byte(std::uint32_t value) { return std::int8_t(value); }
std::int32_t half(std::uint32_t value) { return std::int16_t(value); }
// mulhw: the high word of a signed 64-bit product.
std::int32_t mulhw(std::int32_t a, std::int32_t b) { return std::int32_t((std::int64_t(a) * b) >> 32); }

class Handler {
public:
    Handler(Cpu& cpu, Frame& frame, std::uint32_t event)
        : c(cpu), f(frame), event(event), root(cpu.toc(-0x7bb0)), pool(cpu.toc(-0x4c90)), text(cpu.toc(-0x7bb8)),
          managers(cpu.toc(-0x7a54)), positions(cpu.toc(-0x7474)) {}
    std::uint32_t run();

private:
    Cpu& c;
    Frame& f;
    const std::uint32_t event, root, pool, text, managers, positions;
    std::uint32_t screen = 0;

    std::uint32_t at(std::int32_t offset) const { return f.at(offset); }
    // srawi: the arithmetic shift, with XER CA as the original leaves it
    // (set when a negative value loses one bits).
    std::uint32_t sra(std::uint32_t value, unsigned shift) {
        const auto lost = value & ((1u << shift) - 1);
        c.s.wr<1>(guest::reg::xer_ca, std::int32_t(value) < 0 && lost != 0);
        return std::uint32_t(std::int32_t(value) >> shift);
    }
    // CodeWarrior's signed division by a constant, its srawi setting CA.
    std::int32_t divide_ca(std::int32_t magic, unsigned shift, std::int32_t x) {
        const auto q = std::int32_t(sra(std::uint32_t(mulhw(magic, x)), shift));
        return q + std::int32_t(std::uint32_t(q) >> 31);
    }
    void put(std::int32_t offset, std::uint32_t value) { c.put32(at(offset), value); }
    std::uint32_t word(std::int32_t offset) const { return c.u32(at(offset)); }
    // The position the club's handler left at sp+0x64 / sp+0x62 for dialogs.
    std::uint32_t x() const { return sext16(c.u16(at(0x64))); }
    std::uint32_t y() const { return sext16(c.u16(at(0x62))); }
    std::uint32_t call(std::uint32_t target, std::uint32_t back) {
        c.call(target, back);
        return c.r(3);
    }

    // The root's dialog answer: its id (0x10329e00), taking it (0x10329df0),
    // its value (0x10329de0) and clearing that (0x10329dd0).
    std::uint32_t answer_id(std::uint32_t back) {
        c.set(3, root);
        return call(0x10329e00, back);
    }
    void take_answer(std::uint32_t back) {
        c.set({{3, root}, {4, 0}});
        call(0x10329df0, back);
    }
    std::uint32_t answer(std::uint32_t back) {
        c.set(3, root);
        return call(0x10329de0, back);
    }
    void clear_answer(std::uint32_t back) {
        c.set({{3, root}, {4, 0}});
        call(0x10329dd0, back);
    }
    // A control's action (0x10329bf0) and value (0x1031acc0).
    std::uint32_t action_of(std::uint32_t control, std::uint32_t back) {
        c.set({{3, root}, {4, control}});
        return call(0x10329bf0, back);
    }
    std::uint32_t action(std::uint32_t back) { return action_of(event, back); }
    std::uint32_t value_of(std::uint32_t control, std::uint32_t back) {
        c.set({{3, root}, {4, control}});
        return call(0x1031acc0, back);
    }
    std::uint32_t value(std::uint32_t back) { return value_of(event, back); }
    // The screen's page parameters (0x1073ef70 / 0x1073f1e0).
    std::uint32_t parameter(std::uint32_t n, std::uint32_t back) {
        c.set({{3, root}, {4, n}});
        return call(0x1073ef70, back);
    }
    void set_parameter(std::uint32_t n, std::uint32_t value, std::uint32_t back) {
        c.set({{3, root}, {4, n}, {5, value}, {6, 0}});
        call(0x1073f1e0, back);
    }
    // sprintf (0x10727890) of a pool string into `into`; other arguments as set.
    std::uint32_t print(std::uint32_t into, std::uint32_t format, std::uint32_t back) {
        c.set({{3, into}, {4, pool + format}});
        return call(0x10727890, back);
    }
    // The human manager's formation folder (0x10147150) and name (0x10148eb0).
    std::uint32_t folder(std::uint32_t back) {
        c.set({{3, c.u32(managers)}, {4, 0}, {5, 1}});
        return call(0x10147150, back);
    }
    std::uint32_t manager_name(std::uint32_t back) {
        c.set({{3, c.u32(managers)}, {4, 0}});
        return call(0x10148eb0, back);
    }
    // Reads a file's lock (0x10117fa0): its creator at sp+0x258 (empty when
    // not locked), the file's name at sp+0x48a. True when locked by someone
    // other than the manager.
    bool locked_by_other(std::uint32_t path, std::uint32_t lock_back, std::uint32_t name_back,
                         std::uint32_t compare_back, auto&& after_lock, auto&& after_name, auto&& after_compare) {
        c.set({{3, path}, {4, at(0x258)}, {5, at(0x48a)}, {6, 0}});
        call(0x10117fa0, lock_back);
        after_lock();
        if (!c.i8(at(0x258))) return false;
        const auto name = manager_name(name_back);
        after_name(name);
        c.set({{3, name}, {4, at(0x258)}});
        call(0x1077dc90, compare_back);
        after_compare();
        return c.r(3) != 0;
    }
    void release(std::uint32_t path, std::uint32_t back) {
        c.set(3, path);
        call(0x10773610, back);
    }
    // The file dialog's arguments past the eighth (sp+0x38): the y position,
    // the answer to post, four zeros.
    void dialog_arguments(std::uint32_t answer_code) {
        put(0x38, y());
        put(0x3c, answer_code);
        for (const std::int32_t offset : {0x40, 0x44, 0x48, 0x4c}) put(offset, 0);
    }
    // An exit: the code in its stack slot, the stack object destroyed. `temp`
    // is the register the original loads the code into first.
    std::uint32_t leave(std::int32_t code, std::int32_t slot, unsigned temp, std::uint32_t back) {
        c.set(temp, std::uint32_t(code));
        put(slot, std::uint32_t(code));
        c.set({{3, at(0x240)}, {4, 0xffffffffu}});
        call(0x1074b7c0, back);
        // Some exits keep 0x1074b7c0's result in a volatile register.
        switch (back) {
        case 0x103bc884: case 0x103bd540: case 0x103be360: c.set(4, c.r(3)); break;
        case 0x103bcbbc: case 0x103bcc34: case 0x103be3f4: c.set(5, c.r(3)); break;
        case 0x103bb554: case 0x103bb608: case 0x103bd230: c.set(9, c.r(3)); break;
        case 0x103bb6f0: case 0x103bd2b8: case 0x103bd3ac: c.set(10, c.r(3)); break;
        default: break;
        }
        return std::uint32_t(code);
    }
    // A direct call of a routine of this group (its result in r3, as the original's).
    template <class Result>
    Result direct(std::uint32_t back, auto&& routine) {
        c.link(back);
        const Result result = routine();
        c.set(0, c.lr());  // the callee's epilogue reloads LR through r0
        if constexpr (sizeof(Result) == 8) c.set({{3, std::uint32_t(result >> 32)}, {4, std::uint32_t(result)}});
        else c.set(3, result);
        return result;
    }

    std::optional<std::uint32_t> answers();
    std::uint32_t actions();
    std::optional<std::uint32_t> files_actions();
    std::optional<std::uint32_t> opposition();
    std::optional<std::uint32_t> edit_actions();
    std::optional<std::uint32_t> drags();
    std::optional<std::uint32_t> drag_on_pitch();
    std::optional<std::uint32_t> drag_with_ball();
    std::uint32_t remaining();
    // Distance helpers of the drags.
    std::int32_t distance(std::uint32_t squared, std::int32_t words, std::int32_t result, std::uint32_t back);
    void double_subtract_status(double result, double a, double b);
    std::uint32_t pitch(std::int32_t marker, std::uint32_t mark_back, std::array<std::int32_t, 4> box,
                        std::array<std::uint32_t, 4> backs, std::array<unsigned, 4> keep);
};

// The confirmation and file dialogs post their answers back to the root.
std::optional<std::uint32_t> Handler::answers() {
    // 0x34: "Set To Default" confirmed: the default formation chosen before.
    c.set(5, answer_id(0x103bb040));
    if (c.r(3) == 0x34) {
        take_answer(0x103bb058);
        c.set(14, answer(0x103bb060));
        if (c.r(3) == 2) {
            clear_answer(0x103bb078);
            const auto index = sext16(parameter(9, 0x103bb084));
            c.set({{13, index}, {3, screen}, {4, index}});
            c.set(15, direct<std::uint32_t>(0x103bb098, [&] { return set_default(c, screen, index); }));
        }
        clear_answer(0x103bb0a8);
    }
    // 0x2c: "Load another formation" confirmed: the load dialog.
    if (answer_id(0x103bb0b0) == 0x2c) {
        take_answer(0x103bb0c8);
        c.set(8, answer(0x103bb0d0));
        if (c.r(3) == 2) {
            clear_answer(0x103bb0e8);
            put(0x704, folder(0x103bb0f8));
            put(0x708, print(at(0x552), 0x714, 0x103bb108));
            print(at(0x61a), 0x705, 0x103bb118);
            dialog_arguments(0x2d);
            c.set({{4, word(0x708)}, {5, 0}, {6, word(0x704)}, {7, pool + 0x720}, {8, 0}, {9, c.r(19)}, {10, x()}});
            call(0x100edf20, 0x103bb170);
            return leave(-4, 0x17c, 4, 0x103bb184);
        }
        clear_answer(0x103bb198);
    }
    // 0x2d: a file chosen to load (the answer is the path, freed after).
    if (answer_id(0x103bb1a0) == 0x2d) {
        take_answer(0x103bb1b4);
        const auto path = answer(0x103bb1bc);
        c.set(25, path);
        if (path) {
            clear_answer(0x103bb1d4);
            const bool locked = locked_by_other(path, 0x103bb1e8, 0x103bb208, 0x103bb218,
                [&] { c.set(5, c.r(3)); }, [&](std::uint32_t name) { c.set(13, name); }, [&] { c.set(14, c.r(3)); });
            if (locked) {
                c.set({{5, path}, {6, at(0x48a)}});
                c.set(15, print(text, 0x734, 0x103bb238));
                c.set(13, print(at(0x552), 0x725, 0x103bb248));
                c.set(14, print(at(0x61a), 0x705, 0x103bb258));
                const auto px = c.u16(at(0x64)), py = c.u16(at(0x62));
                c.set({{3, c.r(14)}, {4, c.r(13)}, {5, c.r(15)}, {6, 0}, {15, px}, {7, sext16(px)}, {13, py},
                       {8, sext16(py)}, {9, 0}});
                call(0x100f05a0, 0x103bb284);
                release(path, 0x103bb28c);
                return leave(-4, 0x178, 0, 0x103bb2a0);
            }
            c.set({{3, screen}, {4, path}, {5, 0}});
            const auto loaded = direct<std::uint32_t>(0x103bb2bc, [&] { return load(c, screen, path, 0); });
            c.set({{20, loaded}, {13, sext8(loaded)}});
            if (!byte(loaded)) {
                c.set(5, path);
                c.set(20, print(text, 0x773, 0x103bb2dc));
                c.set(16, print(at(0x552), 0x725, 0x103bb2ec));
                c.set(14, print(at(0x61a), 0x705, 0x103bb2fc));
                const auto px = c.u16(at(0x64));
                c.set({{3, c.r(14)}, {4, c.r(16)}, {5, c.r(20)}, {6, 0}, {13, px}, {7, sext16(px)}, {8, y()}, {9, 0}});
                call(0x100f05a0, 0x103bb328);
                release(path, 0x103bb330);
                return leave(-4, 0x174, 0, 0x103bb344);
            }
            release(path, 0x103bb358);
        }
    }
    // 0x36: a preset formation chosen.
    if (answer_id(0x103bb360) == 0x36) {
        take_answer(0x103bb374);
        const auto path = answer(0x103bb37c);
        c.set(25, path);
        if (path) {
            clear_answer(0x103bb394);
            c.set({{3, screen}, {4, path}, {5, 1}});
            const auto loaded = direct<std::uint32_t>(0x103bb3a4, [&] { return load(c, screen, path, 1); });
            c.set(20, loaded);
            if (!byte(loaded)) {
                c.set(5, path);
                c.set(14, print(text, 0x7ba, 0x103bb3c4));
                c.set(13, print(at(0x552), 0x725, 0x103bb3d4));
                print(at(0x61a), 0x7a4, 0x103bb3e4);
                put(0x70c, c.u16(at(0x64)));
                const auto py = c.u16(at(0x62));
                c.set({{4, c.r(13)}, {5, c.r(14)}, {6, 0}, {7, sext16(word(0x70c))}, {15, py}, {8, sext16(py)}, {9, 0}});
                call(0x100f05a0, 0x103bb410);
                release(path, 0x103bb418);
                put(0x710, 0xfffffffcu);
                return leave(-4, 0x170, 0, 0x103bb434);
            }
            release(path, 0x103bb448);
        }
    }
    // 0x2e: a file chosen to save to.
    c.set(7, answer_id(0x103bb450));
    if (c.r(3) == 0x2e) {
        take_answer(0x103bb468);
        const auto path = answer(0x103bb470);
        c.set(25, path);
        if (path) {
            clear_answer(0x103bb488);
            const bool locked = locked_by_other(path, 0x103bb49c, 0x103bb4bc, 0x103bb4cc,
                [&] { c.set({{14, c.u8(at(0x258))}, {7, sext8(c.u8(at(0x258)))}}); }, [](std::uint32_t) {}, [] {});
            if (locked) {
                c.set({{5, path}, {6, at(0x48a)}});
                c.set(14, print(text, 0x810, 0x103bb4ec));
                c.set(15, print(at(0x552), 0x801, 0x103bb4fc));
                put(0x714, print(at(0x61a), 0x7f2, 0x103bb50c));
                const auto px = c.u16(at(0x64)), py = c.u16(at(0x62));
                c.set({{3, word(0x714)}, {4, c.r(15)}, {5, c.r(14)}, {6, 0}, {7, sext16(px)}, {8, sext16(py)}, {9, 0}});
                call(0x100f05a0, 0x103bb538);
                release(path, 0x103bb540);
                return leave(-4, 0x16c, 9, 0x103bb554);
            }
            const auto named = parameter(5, 0x103bb56c);
            c.set({{3, screen}, {4, path}, {5, named}});
            const auto saved = direct<std::uint32_t>(0x103bb580, [&] { return save(c, screen, path, named); });
            c.set({{20, saved}, {9, sext8(saved)}});
            if (!byte(saved)) {
                c.set(5, path);
                c.set(13, print(text, 0x885, 0x103bb5a0));
                c.set(14, print(at(0x552), 0x801, 0x103bb5b0));
                print(at(0x61a), 0x7f2, 0x103bb5c0);
                const auto py = c.u16(at(0x62));
                c.set({{4, c.r(14)}, {5, c.r(13)}, {6, 0}, {7, x()}, {10, py}, {8, sext16(py)}, {9, 0}});
                call(0x100f05a0, 0x103bb5ec);
                release(path, 0x103bb5f4);
                return leave(-4, 0x168, 0, 0x103bb608);
            }
            release(path, 0x103bb61c);
        }
    }
    // 0x35: a file chosen to save as a preset.
    c.set(13, answer_id(0x103bb624));
    if (c.r(3) == 0x35) {
        take_answer(0x103bb63c);
        const auto path = answer(0x103bb644);
        c.set(25, path);
        if (path) {
            clear_answer(0x103bb65c);
            c.set({{3, screen}, {4, path}});
            const auto saved = direct<std::uint32_t>(0x103bb668, [&] { return save_preset(c, screen, path); });
            c.set({{20, saved}, {9, sext8(saved)}});
            if (!byte(saved)) {
                c.set(5, path);
                c.set(14, print(text, 0x8ce, 0x103bb688));
                c.set(15, print(at(0x552), 0x801, 0x103bb698));
                print(at(0x61a), 0x8b5, 0x103bb6a8);
                c.set({{4, c.r(15)}, {5, c.r(14)}, {6, 0}, {7, x()}, {8, y()}, {9, 0}});
                call(0x100f05a0, 0x103bb6d4);
                release(path, 0x103bb6dc);
                return leave(-4, 0x164, 0, 0x103bb6f0);
            }
            release(path, 0x103bb704);
        }
    }
    // 0x2f: a file chosen to delete.
    if (answer_id(0x103bb70c) == 0x2f) {
        take_answer(0x103bb724);
        const auto path = answer(0x103bb72c);
        c.set(25, path);
        if (path) {
            clear_answer(0x103bb744);
            const bool locked = locked_by_other(path, 0x103bb758, 0x103bb778, 0x103bb788,
                [&] { c.set({{11, c.r(3)}, {10, c.u8(at(0x258))}}); },
                [&](std::uint32_t name) { put(0x718, name); }, [&] { c.set(13, c.r(3)); });
            if (locked) {
                c.set({{5, path}, {6, at(0x48a)}});
                c.set(15, print(text, 0x734, 0x103bb7a8));
                c.set(14, print(at(0x552), 0x919, 0x103bb7b8));
                print(at(0x61a), 0x908, 0x103bb7c8);
                c.set({{4, c.r(14)}, {5, c.r(15)}, {6, 0}, {7, x()}, {8, y()}, {9, 0}});
                call(0x100f05a0, 0x103bb7ec);
                release(path, 0x103bb7f4);
                return leave(-4, 0x160, 3, 0x103bb808);
            }
            c.set(3, path);
            put(0x1fc, call(0x10776690, 0x103bb818));  // deletes the file; nonzero on failure
            if (word(0x1fc)) {
                c.set(5, path);
                c.set(15, print(text, 0x92a, 0x103bb838));
                c.set(14, print(at(0x552), 0x919, 0x103bb848));
                c.set(13, print(at(0x61a), 0x908, 0x103bb858));
                c.set({{3, c.r(13)}, {4, c.r(14)}, {5, c.r(15)}, {6, 0}, {7, x()}, {8, y()}, {9, 0}});
                call(0x100f05a0, 0x103bb884);
                release(path, 0x103bb88c);
                return leave(-4, 0x15c, 3, 0x103bb8a0);
            }
            release(path, 0x103bb8b0);
        }
    }
    // 0x30: "Load another formation" confirmed for a recent file (page parameter 6).
    if (answer_id(0x103bb8b8) == 0x30) {
        take_answer(0x103bb8cc);
        if (answer(0x103bb8d4) == 2) {
            clear_answer(0x103bb8e8);
            const auto recent = sext8(parameter(6, 0x103bb8f4));
            const auto path = at(0x38a);
            c.set({{4, recent}, {3, c.u32(managers)}, {5, path}, {6, 0x100}, {7, 0}, {8, 0}, {9, 1}});
            call(0x10147310, 0x103bb914);  // the recent file's path
            const bool locked = locked_by_other(path, 0x103bb92c, 0x103bb948, 0x103bb958, [] {},
                [&](std::uint32_t name) { c.set(15, name); }, [&] { c.set(23, c.r(3)); });
            if (locked) {
                c.set({{5, path}, {6, at(0x48a)}});
                c.set(13, print(text, 0x734, 0x103bb978));
                c.set(14, print(at(0x552), 0x725, 0x103bb988));
                print(at(0x61a), 0x705, 0x103bb998);
                c.set({{4, c.r(14)}, {5, c.r(13)}, {6, 0}, {7, x()}, {8, y()}, {9, 0}});
                call(0x100f05a0, 0x103bb9bc);
                return leave(-4, 0x158, 3, 0x103bb9d0);
            }
            c.set({{3, screen}, {4, path}, {5, 0}});
            const auto loaded = direct<std::uint32_t>(0x103bb9e8, [&] { return load(c, screen, path, 0); });
            c.set(20, loaded);
            if (!byte(loaded)) {
                c.set(5, path);
                c.set(13, print(text, 0x773, 0x103bba08));
                c.set(14, print(at(0x552), 0x725, 0x103bba18));
                print(at(0x61a), 0x705, 0x103bba28);
                c.set({{4, c.r(14)}, {5, c.r(13)}, {6, 0}, {7, x()}, {8, y()}, {9, 0}});
                call(0x100f05a0, 0x103bba4c);
                return leave(-4, 0x154, 3, 0x103bba60);
            }
        }
        clear_answer(0x103bba78);
    }
    // 0x31: "Return to the match ?" answered yes: the screen closes.
    c.set(23, answer_id(0x103bba80));
    if (c.r(3) == 0x31) {
        take_answer(0x103bba98);
        if (answer(0x103bbaa0) == 2) {
            clear_answer(0x103bbab8);
            c.set({{3, screen}, {4, 1}});
            c.set(10, call(0x103b3b50, 0x103bbac4));
            return leave(-9, 0x150, 0, 0x103bbadc);
        }
        clear_answer(0x103bbaf0);
    }
    // 0x32: the match changes confirmed: a captain chosen when missing, then close.
    if (answer_id(0x103bbaf8) == 0x32) {
        take_answer(0x103bbb0c);
        if (answer(0x103bbb14) == 2) {
            clear_answer(0x103bbb28);
            c.set(3, screen);
            const auto selected = direct<std::uint32_t>(0x103bbb30, [&] { return captain_selected(c, screen); });
            c.set(4, selected);
            if (!byte(selected)) {
                c.set(3, screen);
                c.set(5, direct<std::uint32_t>(0x103bbb48, [&] { return choose_captain(c, screen); }));
            }
            c.set(3, screen);
            c.set(5, call(0x103c5b70, 0x103bbb54));  // keeps the match changes
            c.set({{3, screen}, {4, 1}});
            call(0x103b3b50, 0x103bbb64);
            return leave(-9, 0x14c, 13, 0x103bbb7c);
        }
        clear_answer(0x103bbb94);
    }
    // 0x33: "Save the formation ?" answered yes: the save dialog.
    if (answer_id(0x103bbb9c) == 0x33) {
        take_answer(0x103bbbb4);
        c.set(23, answer(0x103bbbbc));
        if (c.r(3) == 2) {
            clear_answer(0x103bbbd4);
            c.set(3, screen);
            c.set(8, call(0x103bae10, 0x103bbbdc));  // the formation's flag +0x1886: named after itself
            if (byte(c.r(3))) {
                put(0x71c, screen + field::formation);
                put(0x720, folder(0x103bbc08));
                c.set(13, print(at(0x552), 0x95c, 0x103bbc18));
                print(at(0x61a), 0x7f2, 0x103bbc28);
                c.set({{14, 0}, {15, 0}});
                put(0x724, 0);
                dialog_arguments(0x2e);
                c.set({{4, c.r(13)}, {5, 1}, {6, word(0x720)}, {7, pool + 0x720}, {8, word(0x71c)}, {9, c.r(19)},
                       {10, x()}});
                call(0x100edf20, 0x103bbc88);
            } else {
                c.set(14, folder(0x103bbc9c));
                c.set(13, print(at(0x552), 0x95c, 0x103bbcac));
                print(at(0x61a), 0x7f2, 0x103bbcbc);
                dialog_arguments(0x2e);
                c.set({{4, c.r(13)}, {5, 1}, {6, c.r(14)}, {7, pool + 0x720}, {8, pool + 0x22b}, {9, c.r(19)},
                       {10, x()}});
                call(0x100edf20, 0x103bbd14);
            }
            return leave(-4, 0x148, 13, 0x103bbd28);
        }
        clear_answer(0x103bbd40);
    }
    return std::nullopt;
}

// Actions 1 .. 0xd: the screen's menus and buttons.
std::optional<std::uint32_t> Handler::files_actions() {
    // 1: team instructions.
    if (action(0x103bbd4c) == 1) {
        c.set(3, screen);
        c.link(0x103bbd5c);
        open_team_instructions(c, screen);
        return leave(-4, 0x144, 0, 0x103bbd70);
    }
    // 0x22: the club's screen (0x1001d410), with page parameters 1 and 3.
    if (action(0x103bbd88) == 0x22) {
        put(0x1f4, parameter(1, 0x103bbd9c));
        c.put8(at(0x56), sext8(parameter(3, 0x103bbdac)));
        c.set({{3, word(0x1f4)}, {4, c.u8(at(0x56))}, {5, 0}, {6, 0}, {7, 1}});
        call(0x1001d410, 0x103bbdcc);
        return leave(-4, 0x140, 13, 0x103bbde0);
    }
    // 2: a default formation chosen (parameter 9); asks first when not saved.
    if (action(0x103bbdf8) == 2) {
        set_parameter(9, value(0x103bbe0c), 0x103bbe24);
        c.set(3, screen);
        if (!byte(call(0x103bae50, 0x103bbe2c))) {  // the formation's saved byte
            c.set(5, screen + field::formation);
            c.set(13, print(text, 0x980, 0x103bbe54));
            c.set(14, print(at(0x552), 0x96c, 0x103bbe64));
            print(at(0x61a), 0x50, 0x103bbe74);
            c.set({{4, c.r(14)}, {5, c.r(13)}, {6, 1}, {7, x()}, {8, y()}, {9, 0x34}});
            call(0x100f05a0, 0x103bbe98);
            return leave(-4, 0x13c, 5, 0x103bbeac);
        }
        const auto index = sext16(parameter(9, 0x103bbec0));
        c.set({{4, index}, {3, screen}});
        direct<std::uint32_t>(0x103bbecc, [&] { return set_default(c, screen, index); });
        return leave(-11, 0x138, 0, 0x103bbee0);
    }
    // 3: load a formation; asks first when not saved.
    if (action(0x103bbef4) == 3) {
        c.set(3, screen);
        if (!byte(call(0x103bae50, 0x103bbf04))) {
            c.set(5, screen + field::formation);
            c.set(13, print(text, 0x9d1, 0x103bbf28));
            put(0x6f4, print(at(0x552), 0x96c, 0x103bbf38));
            print(at(0x61a), 0x705, 0x103bbf48);
            put(0x6f8, c.u16(at(0x64)));
            const auto py = c.u16(at(0x62));
            c.set({{4, word(0x6f4)}, {5, c.r(13)}, {6, 1}, {7, sext16(word(0x6f8))}, {13, py}, {8, sext16(py)},
                   {9, 0x2c}});
            call(0x100f05a0, 0x103bbf74);
            return leave(-4, 0x134, 14, 0x103bbf88);
        }
        put(0x6fc, folder(0x103bbfa4));
        put(0x728, print(at(0x552), 0x714, 0x103bbfb4));
        put(0x700, print(at(0x61a), 0x705, 0x103bbfc4));
        c.set(14, 0);
        dialog_arguments(0x2d);
        c.set({{3, word(0x700)}, {4, word(0x728)}, {5, 0}, {6, word(0x6fc)}, {7, pool + 0x720}, {8, 0}, {9, c.r(19)},
               {10, x()}});
        call(0x100edf20, 0x103bc024);
        return leave(-4, 0x130, 13, 0x103bc038);
    }
    // 4: load a preset formation (folder "CM3_TACTICS", files .pct).
    if (action(0x103bc050) == 4) {
        c.set({{3, pool + 0xa1a}, {4, pool + 0x22b}});
        const auto presets = call(0x10746280, 0x103bc068);
        c.set({{8, presets}, {3, c.toc(-0x4cb4)}, {4, presets}});
        put(0x704, call(0x1077dbf0, 0x103bc078));
        put(0x708, print(at(0x552), 0x714, 0x103bc088));
        print(at(0x61a), 0x7a4, 0x103bc098);
        dialog_arguments(0x36);
        c.set({{4, word(0x708)}, {5, 0}, {6, c.toc(-0x4cb4)}, {7, pool + 0xa26}, {8, pool + 0x22b}, {9, c.toc(-0x622c)},
               {10, x()}});
        call(0x100edf20, 0x103bc0f0);
        return leave(-4, 0x12c, 4, 0x103bc104);
    }
    // 5: save under the formation's own name.
    if (action(0x103bc118) == 5) {
        const auto name = screen + field::formation;
        c.set(25, name);
        const auto directory = folder(0x103bc13c);
        const auto path = at(0x28a);
        c.set({{5, directory}, {3, path}, {4, pool + 0x22b}, {6, name}, {7, pool + 0x720}});
        call(0x10739b40, 0x103bc154);  // the file's path
        const bool locked = locked_by_other(path, 0x103bc168, 0x103bc184, 0x103bc194,
            [&] { c.set(13, sext8(c.u8(at(0x258)))); }, [&](std::uint32_t manager) { c.set(14, manager); },
            [&] { c.set(15, c.r(3)); });
        if (locked) {
            c.set({{5, path}, {6, at(0x48a)}});
            c.set(13, print(text, 0x810, 0x103bc1b4));
            c.set(14, print(at(0x552), 0x801, 0x103bc1c4));
            c.set(15, print(at(0x61a), 0x7f2, 0x103bc1d4));
            const auto px = c.u16(at(0x64));
            c.set({{3, c.r(15)}, {4, c.r(14)}, {5, c.r(13)}, {6, 0}, {13, px}, {7, sext16(px)}, {8, y()}, {9, 0}});
            call(0x100f05a0, 0x103bc200);
            return leave(-4, 0x128, 0, 0x103bc214);
        }
        const auto named = parameter(5, 0x103bc22c);
        c.set({{13, named}, {3, screen}, {4, path}, {5, named}});
        const auto saved = direct<std::uint32_t>(0x103bc240, [&] { return save(c, screen, path, named); });
        c.set({{20, saved}, {16, sext8(saved)}});
        if (byte(saved)) return leave(-11, 0x120, 3, 0x103bc2dc);
        c.set(5, path);
        c.set(14, print(text, 0x885, 0x103bc260));
        c.set(13, print(at(0x552), 0x801, 0x103bc270));
        print(at(0x61a), 0x7f2, 0x103bc280);
        const auto py = c.u16(at(0x62));
        c.set({{4, c.r(13)}, {5, c.r(14)}, {6, 0}, {7, x()}, {21, py}, {8, sext16(py)}, {9, 0}});
        call(0x100f05a0, 0x103bc2ac);
        return leave(-4, 0x124, 3, 0x103bc2c0);
    }
    // 6: save as: the save dialog (under the formation's name when flagged).
    c.set(14, action(0x103bc2f0));
    if (c.r(3) == 6) {
        set_parameter(5, 0, 0x103bc310);
        c.set(3, screen);
        c.set(13, call(0x103bae10, 0x103bc318));
        if (byte(c.r(3))) {
            put(0x70c, screen + field::formation);
            c.set(15, folder(0x103bc344));
            put(0x710, print(at(0x552), 0x95c, 0x103bc354));
            c.set(13, print(at(0x61a), 0x7f2, 0x103bc364));
            c.set({{25, y()}, {14, 0}});
            dialog_arguments(0x2e);
            const auto px = c.u16(at(0x64));
            c.set({{3, c.r(13)}, {4, word(0x710)}, {5, 1}, {6, c.r(15)}, {7, pool + 0x720}, {8, word(0x70c)},
                   {9, c.r(19)}, {14, px}, {10, sext16(px)}});
            call(0x100edf20, 0x103bc3c4);
        } else {
            c.set(15, folder(0x103bc3d8));
            put(0x714, print(at(0x552), 0x95c, 0x103bc3e8));
            print(at(0x61a), 0x7f2, 0x103bc3f8);
            c.set({{20, 0}, {13, 0}});
            dialog_arguments(0x2e);
            const auto px = c.u16(at(0x64));
            c.set({{4, word(0x714)}, {5, 1}, {6, c.r(15)}, {7, pool + 0x720}, {8, pool + 0x22b}, {9, c.r(19)},
                   {14, px}, {10, sext16(px)}});
            call(0x100edf20, 0x103bc458);
        }
        return leave(-4, 0x11c, 0, 0x103bc46c);
    }
    // 7: save as a preset: the save dialog in the presets folder.
    c.set(10, action(0x103bc484));
    if (c.r(3) == 7) {
        set_parameter(5, 0, 0x103bc4a4);
        c.set({{3, pool + 0xa1a}, {4, pool + 0x22b}});
        const auto presets = call(0x10746280, 0x103bc4b0);
        c.set({{3, c.toc(-0x4cb8)}, {4, presets}});
        c.set(9, call(0x1077dbf0, 0x103bc4c0));
        c.set(13, print(at(0x552), 0x95c, 0x103bc4d0));
        c.set(25, print(at(0x61a), 0x8b5, 0x103bc4e0));
        c.set({{20, c.u16(at(0x62))}, {14, 0x35}, {15, 0}});
        dialog_arguments(0x35);
        c.set({{3, c.r(25)}, {4, c.r(13)}, {5, 1}, {6, c.toc(-0x4cb8)}, {7, pool + 0xa26}, {8, pool + 0x22b},
               {9, c.toc(-0x622c)}, {10, x()}});
        call(0x100edf20, 0x103bc540);
        return leave(-4, 0x118, 10, 0x103bc554);
    }
    // 8: save as, with page parameter 5 set to the control's value.
    c.set(25, action(0x103bc56c));
    if (c.r(3) == 8) {
        const auto named = value(0x103bc584);
        c.set(11, named);
        set_parameter(5, named, 0x103bc59c);
        c.set(3, screen);
        c.set(10, call(0x103bae10, 0x103bc5a4));
        if (byte(c.r(3))) {
            put(0x718, screen + field::formation);
            c.set(13, folder(0x103bc5d0));
            c.set(15, print(at(0x552), 0x95c, 0x103bc5e0));
            c.set(14, print(at(0x61a), 0x7f2, 0x103bc5f0));
            dialog_arguments(0x2e);
            const auto px = c.u16(at(0x64));
            c.set({{3, c.r(14)}, {4, c.r(15)}, {5, 1}, {6, c.r(13)}, {7, pool + 0x720}, {8, word(0x718)}, {9, c.r(19)},
                   {15, px}, {10, sext16(px)}});
            call(0x100edf20, 0x103bc650);
        } else {
            c.set(14, folder(0x103bc664));
            c.set(13, print(at(0x552), 0x95c, 0x103bc674));
            print(at(0x61a), 0x7f2, 0x103bc684);
            dialog_arguments(0x2e);
            c.set({{4, c.r(13)}, {5, 1}, {6, c.r(14)}, {7, pool + 0x720}, {8, pool + 0x22b}, {9, c.r(19)}, {10, x()}});
            call(0x100edf20, 0x103bc6e4);
        }
        return leave(-4, 0x114, 3, 0x103bc6f8);
    }
    // 9: delete a formation file: the file dialog in delete mode.
    if (action(0x103bc70c) == 9) {
        c.set(15, folder(0x103bc724));
        c.set(23, print(at(0x552), 0x714, 0x103bc734));
        c.set(13, print(at(0x61a), 0x908, 0x103bc744));
        c.set(14, c.u16(at(0x62)));
        dialog_arguments(0x2f);
        c.set({{3, c.r(13)}, {4, c.r(23)}, {5, 2}, {6, c.r(15)}, {7, pool + 0x720}, {8, 0}, {9, c.r(19)}, {10, x()}});
        call(0x100edf20, 0x103bc7a4);
        return leave(-4, 0x110, 13, 0x103bc7b8);
    }
    // 0xa: unlock; asks to save when the formation has changed.
    if (action(0x103bc7d0) == 0xa) {
        c.set(3, screen);
        direct<std::uint32_t>(0x103bc7e0, [&] { return unlock(c, screen); });
        c.set(3, screen);
        if (!byte(direct<std::uint32_t>(0x103bc7ec, [&] { return formation_changed(c, screen); })))
            return leave(-11, 0x108, 3, 0x103bc884);
        c.set(13, print(text, 0xa3c, 0x103bc808));
        c.set(23, print(at(0x552), 0x96c, 0x103bc818));
        print(at(0x61a), 0xa2b, 0x103bc828);
        const auto px = c.u16(at(0x64));
        c.set({{4, c.r(23)}, {5, c.r(13)}, {6, 1}, {10, px}, {7, sext16(px)}, {8, y()}, {9, 0x33}});
        call(0x100f05a0, 0x103bc854);
        return leave(-4, 0x10c, 3, 0x103bc868);
    }
    // 0xb: lock; asks to save unless the formation has changed.
    if (action(0x103bc89c) == 0xb) {
        c.set(3, screen);
        c.set(5, direct<std::uint32_t>(0x103bc8b0, [&] { return lock(c, screen); }));
        c.set(3, screen);
        c.set(5, direct<std::uint32_t>(0x103bc8bc, [&] { return formation_changed(c, screen); }));
        if (byte(c.r(3))) {
            put(0x720, 0xfffffff5u);
            return leave(-11, 0x100, 0, 0x103bc960);
        }
        c.set(13, print(text, 0xa9f, 0x103bc8d8));
        c.set(14, print(at(0x552), 0x96c, 0x103bc8e8));
        print(at(0x61a), 0xa90, 0x103bc8f8);
        const auto px = c.u16(at(0x64)), py = c.u16(at(0x62));
        c.set({{4, c.r(14)}, {5, c.r(13)}, {6, 1}, {23, px}, {7, sext16(px)}, {8, sext16(py)}, {9, 0x33}});
        call(0x100f05a0, 0x103bc924);
        const auto code = leave(-4, 0x104, 0, 0x103bc938);
        put(0x71c, c.r(3));
        return code;
    }
    // 0xc: load a recent file (page parameter 6); asks first when not saved.
    if (action(0x103bc978) == 0xc) {
        c.set({{5, value(0x103bc98c)}, {3, root}, {4, 6}, {6, 0}});
        call(0x1073f1e0, 0x103bc9a0);
        c.set(3, screen);
        c.set(4, call(0x103bae50, 0x103bc9a8));
        if (!byte(c.r(3))) {
            const auto name = screen + field::formation;
            c.set({{14, name}, {5, name}});
            c.set(15, print(text, 0x9d1, 0x103bc9d4));
            put(0x724, print(at(0x552), 0x96c, 0x103bc9e4));
            print(at(0x61a), 0x705, 0x103bc9f4);
            const auto py = c.u16(at(0x62));
            c.set({{4, word(0x724)}, {5, c.r(15)}, {6, 1}, {7, x()}, {14, py}, {8, sext16(py)}, {9, 0x30}});
            call(0x100f05a0, 0x103bca20);
            return leave(-4, 0xfc, 13, 0x103bca34);
        }
        const auto recent = sext8(parameter(6, 0x103bca48));
        const auto path = at(0x38a);
        c.set({{4, recent}, {3, c.u32(managers)}, {5, path}, {6, 0x100}, {7, 0}, {8, 0}, {9, 1}});
        call(0x10147310, 0x103bca6c);
        const bool locked = locked_by_other(path, 0x103bca84, 0x103bcaa4, 0x103bcab4, [&] { c.set(4, c.r(3)); },
            [](std::uint32_t) {}, [] {});
        if (locked) {
            c.set({{5, path}, {6, at(0x48a)}});
            c.set(13, print(text, 0x734, 0x103bcad4));
            c.set(14, print(at(0x552), 0x725, 0x103bcae4));
            print(at(0x61a), 0x705, 0x103bcaf4);
            c.set({{4, c.r(14)}, {5, c.r(13)}, {6, 0}, {7, x()}, {8, y()}, {9, 0}});
            call(0x100f05a0, 0x103bcb18);
            return leave(-4, 0xf8, 3, 0x103bcb2c);
        }
        c.set({{3, screen}, {4, path}, {5, 0}});
        const auto loaded = direct<std::uint32_t>(0x103bcb44, [&] { return load(c, screen, path, 0); });
        c.set(20, loaded);
        if (byte(loaded)) return leave(-11, 0xf0, 13, 0x103bcbdc);
        c.set(5, path);
        c.set(13, print(text, 0x773, 0x103bcb64));
        c.set(14, print(at(0x552), 0x725, 0x103bcb74));
        print(at(0x61a), 0x705, 0x103bcb84);
        c.set({{4, c.r(14)}, {5, c.r(13)}, {6, 0}, {7, x()}, {8, y()}, {9, 0}});
        call(0x100f05a0, 0x103bcba8);
        return leave(-4, 0xf4, 3, 0x103bcbbc);
    }
    // 0xd: page parameter 10 set to the control's value.
    if (action(0x103bcbf4) == 0xd) {
        set_parameter(10, value(0x103bcc08), 0x103bcc20);
        return leave(-11, 0xec, 0, 0x103bcc34);
    }
    return std::nullopt;
}

// 0xe: the opposition's tactics: the club (page parameter 1) next fixture's
// opponent, opened on its own tactics screen when its staff can be shown.
std::optional<std::uint32_t> Handler::opposition() {
    if (action(0x103bcc4c) != 0xe) return std::nullopt;
    const auto list = at(0x248);
    c.set(3, list);
    call(0x10738c00, 0x103bcc5c);  // constructs a fixture list
    const auto club = parameter(1, 0x103bcc68);
    c.set({{4, club}, {24, club}});
    c.put8(at(0x55), sext8(parameter(3, 0x103bcc7c)));
    c.set(3, club);
    const auto dates = c.r(18);
    // The fixtures between two dates (0x10717cb0 moves a date by days).
    const auto shifted = [&](std::int32_t offset, std::uint32_t days, std::uint32_t back) {
        c.set({{3, at(offset)}, {4, dates}, {5, days & 0xff}});
        call(0x10717cb0, back);
    };
    if (byte(call(0x100c7d00, 0x103bcc8c))) {
        const auto national = c.i8(at(0x55));
        if (national) {
            c.set(5, 0);
            put(0x6c, 0);
        } else {
            c.set(13, 1);
            put(0x6c, 1);
        }
        put(0x6f4, c.u16(dates + 2));
        put(0x6f8, std::uint32_t(c.u16(dates)));
        shifted(0x210, 0x18, 0x103bccec);
        c.set(13, c.u16(at(0x212)));
        shifted(0x218, 0x18, 0x103bcd0c);
        const auto day = sext16(c.u16(at(0x218)));
        c.set({{14, day}, {15, day & 0xffff}});
        put(0x6fc, 0xfffffffeu);
        put(0x38, 0xfffffffeu);
        put(0x728, word(0x6c));
        put(0x700, sext8(word(0x728)));
        put(0x3c, word(0x700));
        c.set({{3, c.u32(c.toc(-0x7a60))}, {4, c.r(15)}, {5, c.r(13)}, {6, word(0x6f8)}, {7, word(0x6f4)},
               {8, c.u32(club)}, {9, list}, {10, 0xffffffffu}});
        call(0x1048b180, 0x103bcd70);
    } else {
        const auto national = c.u8(at(0x55));
        if (std::int8_t(national)) {
            c.set(5, national);
            put(0x68, 0);
        } else {
            c.set(5, 1);
            put(0x68, 1);
        }
        c.set(14, c.u16(dates + 2));
        c.set(13, std::uint32_t(c.u16(dates)));
        shifted(0x200, 8, 0x103bcdc8);
        c.set(15, c.u16(at(0x202)));
        shifted(0x208, 8, 0x103bcde8);
        const auto day = std::uint32_t(c.u16(at(0x208)));
        put(0x704, 0xfffffffeu);
        put(0x38, 0xfffffffeu);
        put(0x708, word(0x68));
        put(0x3c, sext8(word(0x708)));
        c.set({{3, c.u32(c.toc(-0x7a60))}, {4, day}, {5, c.r(15)}, {6, c.r(13)}, {7, c.r(14)}, {8, c.u32(club)},
               {9, list}, {10, 0xffffffffu}});
        call(0x1048b180, 0x103bce44);
        c.set(4, c.r(3));
    }
    // Opens the tactics of the first fixture's opponent the game can show.
    const auto finish = [&](std::int32_t slot, unsigned temp, std::uint32_t list_back, std::uint32_t back) {
        c.set(temp, 0xfffffffcu);
        put(slot, 0xfffffffcu);
        c.set({{3, list}, {4, 0xffffffffu}});
        call(0x10738c20, list_back);
        return std::make_pair(0xfffffffcu, back);
    };
    c.set(3, list);
    c.set(4, sext16(call(0x101f9430, 0x103bce50)));  // the number of fixtures
    if (c.r(4) != 0) {
        c.set(3, list);
        call(0x10323570, 0x103bce68);  // to the first
        for (;;) {
            c.set(3, list);
            const auto fixture = call(0x10738e30, 0x103bd070);  // the next one
            c.set(21, fixture);
            if (!fixture) break;
            if (c.i8(fixture + 0x43) <= -1) continue;
            c.set({{3, c.u32(c.r(17))}, {4, c.u32(fixture + 8)}, {5, c.u32(fixture + 0xc)}, {6, c.u32(fixture + 0x10)},
                   {7, c.u16(fixture + 0x2a)}, {8, c.u16(fixture + 0x28)}});
            c.set(4, call(0x1029eb20, 0x103bce98));
            if (byte(c.r(3))) {
                const auto opponent = c.u32(fixture + 8);
                c.set({{3, club}, {4, c.u8(at(0x55))}, {5, 0}, {6, 0}, {7, opponent}, {8, 0}});
                direct<std::uint32_t>(0x103bcec4, [&] { return open(c, club, c.u8(at(0x55)), 0, 0, opponent, 0); });
                const auto [code, back] = finish(0xe8, 4, 0x103bced8, 0x103bcee4);
                c.set({{3, at(0x240)}, {4, 0xffffffffu}});
                call(0x1074b7c0, back);
                return code;
            }
            // Its staff cannot be shown: say why.
            const auto national = c.u8(at(0x55));
            c.set({{25, national}, {5, sext8(national)}});
            const auto rating = [&](std::uint32_t format, std::uint32_t back) {
                c.set({{3, at(0x61a)}, {4, pool + format}, {5, sext8(c.u8(club + 0x23b))}, {6, sext8(c.u8(club + 0x23b))},
                       {7, club + 0x221}});
                return call(0x10727890, back);
            };
            if (std::int8_t(national)) {
                c.set(3, club);
                if (byte(call(0x100c7d00, 0x103bcf04))) {
                    c.set(13, print(text, 0xb25, 0x103bcf1c));
                    c.set(14, print(at(0x552), 0xb10, 0x103bcf2c));
                    c.set(15, rating(0xaf1, 0x103bcf50));
                    const auto px = c.u16(at(0x64)), py = c.u16(at(0x62));
                    c.set({{3, c.r(15)}, {4, c.r(14)}, {5, c.r(13)}, {6, 0}, {13, px}, {7, sext16(px)}, {14, py},
                           {8, sext16(py)}, {9, 0}});
                    call(0x100f05a0, 0x103bcf7c);
                } else {
                    c.set(15, print(text, 0xb7c, 0x103bcf8c));
                    c.set(13, print(at(0x552), 0xb10, 0x103bcf9c));
                    rating(0xb5e, 0x103bcfc0);
                    const auto py = c.u16(at(0x62));
                    c.set({{4, c.r(13)}, {5, c.r(15)}, {6, 0}, {7, x()}, {20, py}, {8, sext16(py)}, {9, 0}});
                    call(0x100f05a0, 0x103bcfec);
                }
            } else {
                c.set(13, print(text, 0xbb1, 0x103bcffc));
                c.set(20, print(at(0x552), 0xb10, 0x103bd00c));
                const auto px = c.u16(at(0x64)), py = c.u16(at(0x62));
                c.set({{3, club + 0x1ee}, {4, c.r(20)}, {5, c.r(13)}, {6, 0}, {16, px}, {7, sext16(px)}, {14, py},
                       {8, sext16(py)}, {9, 0}});
                call(0x100f05a0, 0x103bd038);
            }
            const auto [code, back] = finish(0xe4, 13, 0x103bd04c, 0x103bd05c);
            c.set({{3, at(0x240)}, {4, 0xffffffffu}});
            call(0x1074b7c0, back);
            return code;
        }
    }
    // No fixture to show.
    const auto rating = [&](std::uint32_t format, std::uint32_t back) {
        c.set({{3, at(0x61a)}, {4, pool + format}, {5, sext8(c.u8(club + 0x23b))}, {6, sext8(c.u8(club + 0x23b))},
               {7, club + 0x221}});
        return call(0x10727890, back);
    };
    c.set(3, sext8(c.u8(at(0x55))));
    if (c.i8(at(0x55))) {
        c.set(3, club);
        c.set(3, sext8(call(0x100c7d00, 0x103bd094)));
        if (byte(c.r(3))) {
            c.set(14, print(text, 0xbf0, 0x103bd0ac));
            c.set(13, print(at(0x552), 0xbde, 0x103bd0bc));
            rating(0xaf1, 0x103bd0e0);
            put(0x70c, c.u16(at(0x64)));
            const auto py = c.u16(at(0x62));
            c.set({{4, c.r(13)}, {5, c.r(14)}, {6, 0}, {7, sext16(word(0x70c))}, {15, py}, {8, sext16(py)}, {9, 0}});
            call(0x100f05a0, 0x103bd10c);
        } else {
            put(0x710, print(text, 0xc26, 0x103bd11c));
            c.set(13, print(at(0x552), 0xbde, 0x103bd12c));
            rating(0xb5e, 0x103bd150);
            const auto px = c.u16(at(0x64));
            c.set({{4, c.r(13)}, {5, word(0x710)}, {6, 0}, {25, px}, {7, sext16(px)}, {8, y()}, {9, 0}});
            call(0x100f05a0, 0x103bd17c);
        }
    } else {
        c.set(14, print(text, 0xc52, 0x103bd18c));
        const auto title = print(at(0x552), 0xbde, 0x103bd19c);
        c.set({{3, club + 0x1ee}, {4, title}, {5, c.r(14)}, {6, 0}, {7, x()}, {8, y()}, {9, 0}});
        call(0x100f05a0, 0x103bd1c8);
    }
    const auto [code, back] = finish(0xe0, 14, 0x103bd1dc, 0x103bd1ec);
    c.set(15, c.r(3));
    c.set({{3, at(0x240)}, {4, 0xffffffffu}});
    call(0x1074b7c0, back);
    put(0x714, c.r(3));
    return code;
}

// Actions 0xf .. 0x1b: edits of the formation and of the match squad.
std::optional<std::uint32_t> Handler::edit_actions() {
    // 0xf undo, 0x10 copy the cell, 0x11 copy every cell, 0x12 paste, 0x13 mirror.
    c.set(8, action(0x103bd204));
    if (c.r(3) == 0xf) {
        c.set(3, screen);
        c.set(9, direct<std::uint32_t>(0x103bd218, [&] { return undo(c, screen); }));
        return leave(-11, 0xdc, 9, 0x103bd230);
    }
    if (action(0x103bd248) == 0x10) {
        c.set(3, screen);
        c.set(20, direct<std::uint32_t>(0x103bd25c, [&] { return copy_cell(c, screen); }));
        return leave(-11, 0xd8, 9, 0x103bd274);
    }
    c.set(14, action(0x103bd28c));
    if (c.r(3) == 0x11) {
        c.set(3, screen);
        direct<std::uint32_t>(0x103bd2a0, [&] { return copy_all_cells(c, screen); });
        return leave(-11, 0xd4, 0, 0x103bd2b8);
    }
    if (action(0x103bd2d0) == 0x12) {
        c.set(3, screen);
        c.set(9, direct<std::uint32_t>(0x103bd2e4, [&] { return paste(c, screen); }));
        return leave(-11, 0xd0, 13, 0x103bd2fc);
    }
    c.set(20, action(0x103bd314));
    if (c.r(3) == 0x13) {
        c.set(3, screen);
        c.set(9, direct<std::uint32_t>(0x103bd328, [&] { return mirror_cell(c, screen); }));
        return leave(-11, 0xcc, 14, 0x103bd340);
    }
    // 0x14: toggles the free-kick side byte (+0x27ae, leaves 0x103bad00 / 0x103be990).
    if (action(0x103bd358) == 0x14) {
        c.set(3, screen);
        tactics_leaf_103bad00(c.s);  // reads the byte
        const bool set = byte(c.r(3)) != 0;
        c.compare(0, byte(c.r(3)), 0);
        c.set({{3, screen}, {4, set ? 0u : 1u}});
        tactics_leaf_103be990(c.s);  // stores it
        return leave(-11, 0xc8, 0, 0x103bd3ac);
    }
    // 0x15: the view (1..3) from the control's value.
    if (action(0x103bd3c4) == 0x15) {
        const auto chosen = value(0x103bd3dc);
        const auto view = sext8(chosen);
        c.set({{25, chosen}, {11, view}, {3, screen}, {4, view}});
        c.set(10, direct<std::uint32_t>(0x103bd3f0, [&] { return set_view(c, screen, view); }));
        const auto code = leave(-11, 0xc4, 0, 0x103bd408);
        put(0x718, c.r(3));
        return code;
    }
    // 0x16: a match entry's instructions, when it is selectable.
    c.set(13, action(0x103bd420));
    if (c.r(3) == 0x16) {
        const auto chosen = value(0x103bd438);
        c.set({{15, chosen}, {14, sext8(chosen)}});
        c.put8(at(0x5a), sext8(chosen));
        c.set({{3, screen}, {4, c.u8(at(0x5a))}, {5, 0}, {6, 0}});
        if (byte(call(0x103c9810, 0x103bd458))) {
            const auto entry = c.u8(at(0x5a));
            c.set({{3, screen}, {4, entry}});
            c.link(0x103bd470);
            open_player_instructions(c, screen, entry);
            return leave(-4, 0xc0, 0, 0x103bd484);
        }
    }
    return drags();
}

// Drops: a dragged control (event) on the control it was dropped on (+0x76).
std::optional<std::uint32_t> Handler::drags() {
    const auto target = [&](std::uint32_t back) {
        c.set({{3, root}, {4, event}});
        const auto dropped = direct<std::uint32_t>(back, [&] { return control_76(c, root, event); });
        c.set(23, dropped);
        return dropped;
    };
    // 0x17 on 0x16: makes that match entry captain.
    if (action(0x103bd498) == 0x17) {
        const auto dropped = target(0x103bd4ac);
        if (half(dropped) != -1) {
            c.set(15, action_of(dropped, 0x103bd4c8));
            if (c.r(3) == 0x16) {
                c.set(3, screen);
                const auto current = call(0x103c6180, 0x103bd4dc);
                c.set({{14, current}, {13, sext8(current)}});
                const auto chosen = sext8(value_of(dropped, 0x103bd4f0));
                c.set(8, chosen);
                if (chosen != sext8(current)) {
                    c.put8(at(0x54), sext8(value_of(dropped, 0x103bd50c)));
                    c.set({{3, screen}, {4, c.u8(at(0x54))}, {5, 0}});
                    if (call(0x103ca4e0, 0x103bd524) == 0) return leave(0, 0xbc, 3, 0x103bd540);
                    c.set({{3, screen}, {4, c.u8(at(0x54))}, {5, 0}});
                    const auto staff = call(0x103ca4e0, 0x103bd55c);
                    c.set({{4, staff}, {3, screen}});
                    direct<std::uint32_t>(0x103bd56c, [&] { return set_captain(c, screen, staff); });
                    return leave(-11, 0xb8, 3, 0x103bd580);
                }
            }
        }
    }
    // 0x18 on 0x16: puts the match entry in that formation place.
    c.set(15, action(0x103bd594));
    if (c.r(3) == 0x18) {
        const auto dropped = target(0x103bd5ac);
        c.set(13, sext16(dropped));
        if (half(dropped) != -1) {
            c.set(14, action_of(dropped, 0x103bd5c8));
            if (c.r(3) == 0x16) {
                c.put8(at(0x59), sext8(value(0x103bd5e0)));
                c.put8(at(0x5a), sext8(value_of(dropped, 0x103bd5f4)));
                c.set({{3, screen}, {4, c.u8(at(0x59))}});
                const auto held = sext8(call(0x103c72c0, 0x103bd60c));
                c.set(13, sext8(c.u8(at(0x5a))));
                if (sext8(c.u8(at(0x5a))) != held) {
                    c.set({{3, screen}, {4, c.u8(at(0x5a))}, {5, 0}, {6, 0}});
                    c.set(14, call(0x103c9810, 0x103bd634));
                    if (byte(c.r(3))) {
                        const auto place = c.u8(at(0x59)), entry = c.u8(at(0x5a));
                        c.set({{3, screen}, {4, place}, {5, entry}});
                        direct<std::uint32_t>(0x103bd654, [&] { return set_lineup(c, screen, place, entry); });
                        return leave(-11, 0xb4, 0, 0x103bd66c);
                    }
                }
            }
        }
    }
    // 0x18 on 0x18: swaps two formation places.
    c.set(13, action(0x103bd680));
    if (c.r(3) == 0x18) {
        const auto dropped = target(0x103bd698);
        if (half(dropped) != -1) {
            c.set(10, action_of(dropped, 0x103bd6b4));
            if (c.r(3) == 0x18) {
                c.put8(at(0x58), sext8(value(0x103bd6cc)));
                c.put8(at(0x57), sext8(value_of(dropped, 0x103bd6e4)));
                const auto first = c.u8(at(0x58)), second = c.u8(at(0x57));
                c.set({{4, first}, {5, sext8(second)}});
                if (sext8(first) != sext8(second)) {
                    c.set({{3, screen}, {4, first}, {5, second}});
                    direct<std::uint32_t>(0x103bd714, [&] { return swap_lineup(c, screen, first, second); });
                    return leave(-11, 0xb0, 13, 0x103bd72c);
                }
            }
        }
    }
    // 0x19 on 0x19: swaps two match entries.
    if (action(0x103bd744) == 0x19) {
        const auto dropped = target(0x103bd75c);
        c.set(8, sext16(dropped));
        if (half(dropped) != -1 && action_of(dropped, 0x103bd778) == 0x19) {
            put(0x71c, value_of(dropped, 0x103bd790));
            put(0x720, sext8(word(0x71c)));
            const auto chosen = sext8(value(0x103bd7ac));
            c.set(5, chosen);
            if (chosen != word(0x720)) {
                const auto second = sext8(value_of(dropped, 0x103bd7c8));
                c.set(14, second);
                const auto first_value = value(0x103bd7dc);
                c.set(15, first_value);
                put(0x724, sext8(first_value));
                c.set({{3, screen}, {4, word(0x724)}, {5, second}, {6, 0}});
                direct<std::uint32_t>(0x103bd7fc, [&] { return swap_entries(c, screen, word(0x724), second, 0); });
                c.set(4, c.r(3));
                return leave(-11, 0xac, 0, 0x103bd814);
            }
        }
    }
    // 0x1a / 0x1b: a player's instruction bit (the value: player in bits
    // 6..10, bit index in bits 0..5), toggled (0x103c82a0) or set (0x103c8c70).
    c.set(13, action(0x103bd82c));
    if (c.r(3) == 0x1a) {
        c.put8(at(0x53), sext8(sra(sext16(value(0x103bd844)) & 0x7c1, 6)));
        const auto index = sext8(sext16(value(0x103bd864)) & 0x3f);
        c.set(3, index);
        instruction_bit_registers(c);  // 0x103cf110, frameless
        const auto bit = std::uint64_t(c.r(3)) << 32 | c.r(4);
        c.set({{13, std::uint32_t(bit)}, {14, std::uint32_t(bit >> 32)}});
        put(0x23c, std::uint32_t(bit));
        put(0x238, std::uint32_t(bit >> 32));
        c.set({{3, screen}, {4, c.u8(at(0x53))}, {5, word(0x238)}, {6, word(0x23c)}});
        call(0x103c82a0, 0x103bd8a0);
        return leave(-11, 0xa8, 0, 0x103bd8b4);
    }
    if (action(0x103bd8cc) == 0x1b) {
        const auto player = sra(sext16(value(0x103bd8e0)) & 0x7c1, 6);
        c.set({{13, player}, {14, sext8(player)}});
        c.put8(at(0x52), sext8(player));
        const auto index = sext8(sext16(value(0x103bd900)) & 0x3f);
        c.set(3, index);
        instruction_bit_registers(c);  // 0x103cf110, frameless
        const auto bit = std::uint64_t(c.r(3)) << 32 | c.r(4);
        c.set({{5, std::uint32_t(bit)}, {13, std::uint32_t(bit >> 32)}});
        put(0x234, std::uint32_t(bit));
        put(0x230, std::uint32_t(bit >> 32));
        c.set({{3, screen}, {4, c.u8(at(0x52))}, {14, word(0x230)}, {5, word(0x234)}});
        call(0x103c8c70, 0x103bd934);
        return leave(-11, 0xa4, 0, 0x103bd948);
    }
    if (const auto code = drag_on_pitch()) return code;
    if (const auto code = drag_with_ball()) return code;
    return std::nullopt;
}

// The generated code's FPSCR bits for a double subtraction (fsub): result
// class, the sticky invalid bit for NaN operands, and the summaries.
void Handler::double_subtract_status(double r, double a, double b) {
    auto& s = c.s;
    const auto get = [&](std::size_t bit) { return std::uint8_t(s.r<1>(bit)); };
    const auto set = [&](std::size_t bit, std::uint32_t value) { s.wr<1>(bit, value); };
    set(1296, r < 0);
    set(1297, 0 < r);
    set(1298, r == 0);
    set(1299, std::isnan(r));
    set(1287, get(1287) | std::isnan(a) | std::isnan(b));
    std::uint8_t invalid = 0;
    for (const std::size_t bit : {1287u, 1288u, 1289u, 1290u, 1291u, 1292u, 1301u, 1302u, 1303u}) invalid |= get(bit);
    set(1282, invalid);
    set(1280, get(1280) | get(1283) | get(1284) | get(1285) | get(1286));
    std::uint8_t enabled = 0;
    for (const std::size_t bit : {1282u, 1283u, 1284u, 1285u, 1286u}) enabled ^= get(bit) & get(bit + 22);
    set(1281, enabled);
}

// The distance from a squared length, as the original computes it: the
// integer as a double through the stack words at sp+words (the int-to-double
// magic at TOC -0x4cb0 + 8), sqrt (0x10780fd8, an import), then fctiwz
// stored at sp+result; its low word is the distance.
std::int32_t Handler::distance(std::uint32_t squared, std::int32_t words, std::int32_t result, std::uint32_t back) {
    const auto magic = c.f64(c.toc(-0x4cb0) + 8);
    c.set_f(1, magic);
    put(words + 4, squared ^ 0x80000000u);
    put(words, 0x43300000);
    c.set(0, 0x43300000);
    const auto value = c.f64(at(words));
    const double difference = value - magic;
    c.set_f(0, value);
    c.set_f(1, difference);
    double_subtract_status(difference, value, magic);
    call(0x10780fd8, back);
    c.set(2, word(0x14));
    original_fctiwz(c.s, guest::reg::fpr(0), guest::reg::fpr(1));
    c.put64(at(result), c.s.r<8>(guest::reg::fpr(0)));
    return std::int32_t(word(result + 4));
}

// The dragged control's rectangle copied to sp+marker (0x103be7d0) and the
// pitch control's (left, top, right, bottom at sp+box .. box-0xc); returns the
// pitch control.
std::uint32_t Handler::pitch(std::int32_t marker, std::uint32_t mark_back, std::array<std::int32_t, 4> box,
                             std::array<std::uint32_t, 4> backs, std::array<unsigned, 4> keep) {
    c.set({{3, root}, {4, event}, {5, at(marker)}});
    direct<std::uint32_t>(mark_back, [&] { return set_control_60(c, root, event, at(marker)); });
    const auto dropped = c.r(23);
    using Reader = std::uint32_t (*)(Cpu&, std::uint32_t, std::uint32_t);
    constexpr Reader readers[] = {control_10, control_14, control_18, control_1c};
    for (std::size_t n = 0; n < 4; ++n) {
        c.set({{3, root}, {4, dropped}});
        const auto value = direct<std::uint32_t>(backs[n], [&] { return readers[n](c, root, dropped); });
        if (keep[n]) c.set(keep[n], value);
        put(box[n], value);
    }
    return dropped;
}

// 0x1d on 0x1c: a player dragged on the pitch. In the positions view, to the
// nearest position marker (closer than 800): a swap with the player holding
// it, else his new position. In the other views, to the cell of a 27 x 36
// grid over the pitch the dragged marker overlaps most: a swap of spots with
// the player standing there, else his spot moves there.
std::optional<std::uint32_t> Handler::drag_on_pitch() {
    c.set(5, action(0x103bd960));
    if (c.r(3) != 0x1d) return std::nullopt;
    c.set({{3, root}, {4, event}});
    const auto dropped = direct<std::uint32_t>(0x103bd978, [&] { return control_76(c, root, event); });
    c.set(23, dropped);
    if (half(dropped) == -1) return std::nullopt;
    c.set(4, action_of(dropped, 0x103bd994));
    if (c.r(3) != 0x1c) return std::nullopt;
    c.put8(at(0x51), sext8(value(0x103bd9ac)));
    pitch(0x228, 0x103bd9c4, {0x1f0, 0x1ec, 0x1e8, 0x1e4}, {0x103bd9d0, 0x103bd9e0, 0x103bd9f0, 0x103bda00}, {0, 0, 0, 5});
    const auto player = [&] { return c.u8(at(0x51)); };
    c.set(3, screen);
    tactics_leaf_103bacf0(c.s);  // the view
    const auto view = c.r(3);
    c.set(13, view);
    put(0x6f4, sext8(view));
    if (word(0x6f4) == 1) {
        put(0x1e0, 800);
        c.set(3, 800);
        put(0x6f8, 0);
        put(0x1dc, 0);
        c.set(0, 0);
        // Scratch registers are set as the original's instructions leave
        // them: they reach the callees and the exit.
        for (;;) {
            const auto offset = word(0x1dc) << 1;
            const auto code = sext16(c.u16(positions + offset));
            c.set({{0, word(0x1dc)}, {20, offset}, {13, code}});
            c.compare(0, half(code), -1);
            if (half(code) == -1) break;
            const auto index = word(0x1dc);
            c.set({{3, screen}, {13, index}, {14, index << 1}, {4, sext16(c.u16(positions + (index << 1)))}});
            const auto central = call(0x103bab40, 0x103bda58);
            c.set(15, central);
            put(0x6fc, word(0x1dc));
            put(0x728, word(0x6fc) << 1);
            c.set(0, word(0x728));
            // The position's marker (0x103ce330) at sp+0x1d4 (x), sp+0x1d8 (y).
            c.set({{3, sext16(c.u16(positions + word(0x728)))}, {4, word(0x1f0)}, {5, word(0x1ec)}, {6, word(0x1e8)},
                   {7, word(0x1e4)}, {8, at(0x1d8)}, {9, at(0x1d4)}, {10, central}});
            put(0x700, call(0x103ce330, 0x103bda98));
            const auto left = sext16(c.u16(at(0x228)));
            const auto width = sext16(c.u16(at(0x22c))) - left;
            const auto rounded = (width >> 31) + width;
            const auto centre_x = left + std::uint32_t(std::int32_t(rounded) >> 1);
            const auto dx = centre_x - word(0x1d4);
            c.set({{14, rounded}, {13, centre_x}, {15, dx}});
            put(0x1d0, dx);
            const auto top = sext16(c.u16(at(0x22a)));
            put(0x704, top);
            put(0x708, sext16(c.u16(at(0x22e))));
            const auto height = word(0x708) - word(0x704);
            const auto dy = top + sra((height >> 31) + height, 1) - word(0x1d8);
            put(0x1cc, dy);
            const auto squared = word(0x1cc) * word(0x1cc) + word(0x1d0) * word(0x1d0);
            c.set({{25, squared}, {4, word(0x1d0)}, {8, top}, {5, c.toc(-0x4cb0)}, {3, squared ^ 0x80000000u}});
            const auto length = std::uint32_t(distance(squared, 0x6ec, 0x6e4, 0x103bdb44));
            c.set(13, length);
            put(0x1c8, length);
            c.set(14, word(0x1e0));
            c.compare(0, std::int32_t(length), std::int32_t(word(0x1e0)));
            if (std::int32_t(length) < std::int32_t(word(0x1e0))) {
                const auto nearest = word(0x1dc);
                c.set({{15, nearest}, {13, nearest << 1}, {14, sext16(c.u16(positions + (nearest << 1)))}});
                c.put16(at(0x60), c.r(14));
                c.set(15, word(0x1c8));
                put(0x1e0, word(0x1c8));
            }
            c.set({{13, word(0x1dc)}, {0, word(0x1dc) + 1}});
            put(0x1dc, word(0x1dc) + 1);
        }
        c.set(20, word(0x1e0));
        c.compare(0, std::int32_t(word(0x1e0)), 800);
        if (std::int32_t(word(0x1e0)) >= 800) return std::nullopt;
        c.set({{3, screen}, {4, sext16(c.u16(at(0x60)))}});
        const auto holder = call(0x103bab00, 0x103bdbb4);  // the player at that position
        c.set({{16, holder}, {14, sext8(holder)}});
        if (byte(holder) > -1) {
            c.set(13, sext8(holder));
            if (byte(holder) != c.i8(at(0x51))) {
                c.set({{3, screen}, {4, player()}, {5, holder}});
                c.set(21, direct<std::uint32_t>(0x103bdbe8, [&] { return swap_players(c, screen, player(), holder); }));
                return leave(-11, 0xa0, 3, 0x103bdc00);
            }
        }
        if (byte(holder) != -1) return std::nullopt;
        const auto position = sext16(c.u16(at(0x60)));
        c.set({{3, screen}, {4, player()}, {5, position}, {6, 0}});
        direct<std::uint32_t>(0x103bdc28, [&] { return set_position(c, screen, player(), position, 0); });
        return leave(-11, 0x9c, 14, 0x103bdc3c);
    }
    // The grid: 27 columns, 36 rows over the pitch control. 0x10769920
    // (frameless, no stores) is the overlap of two rectangles; it runs here,
    // and the registers are left as the original's last iteration leaves them.
    put(0x1c4, 0);
    put(0x70c, 0);
    put(0x1c0, 0);
    std::uint32_t r[12] = {}, r13 = c.r(13), r14 = c.r(14), r15 = c.r(15), r20 = c.r(20), r25 = c.r(25);
    while (std::int32_t(word(0x1c0)) < 27) {
        r15 = 0;
        put(0x1bc, 0);
        while (std::int32_t(word(0x1bc)) < 36) {
            const auto marker_top = sext16(c.u16(at(0x22a))), marker_left = sext16(c.u16(at(0x228)));
            const auto marker_bottom = sext16(c.u16(at(0x22e))), marker_right = sext16(c.u16(at(0x22c)));
            put(0x710, word(0x1f0));
            const auto column = word(0x1c0), row = word(0x1bc);
            const auto left = word(0x1f0), top = word(0x1ec), right = word(0x1e8), bottom = word(0x1e4);
            r14 = column * (right - left);
            r13 = std::uint32_t(mulhw(0x4bda12f7, std::int32_t(r14)));  // / 27
            r15 = std::uint32_t(std::int32_t(r13) >> 3);
            r25 = r15 >> 31;
            const auto x1 = word(0x710) + r15 + r25;
            r20 = top;
            r13 = bottom - top;
            r14 = (row + 1) * r13;
            put(0x714, std::uint32_t(mulhw(0x38e38e39, std::int32_t(r14))));  // / 36
            const auto q1 = std::uint32_t(std::int32_t(word(0x714)) >> 3);
            const auto y1 = bottom - (q1 + (q1 >> 31));
            r20 = column;
            r14 = left;
            r15 = right;
            const auto q2 = std::uint32_t(mulhw(0x4bda12f7, std::int32_t((column + 1) * (right - left))) >> 3);
            r13 = q2 >> 31;
            r25 = q2 + r13;
            const auto x2 = left + r25;
            put(0x718, bottom - top);
            r13 = row * word(0x718);
            r15 = std::uint32_t(mulhw(0x38e38e39, std::int32_t(r13)) >> 3);
            r25 = r15 + (r15 >> 31);
            r14 = bottom;
            const auto y2 = bottom - r25;
            // 0x10769920(marker_top, marker_left, marker_bottom, marker_right, x1, y1, x2, y2).
            const auto smaller = [](std::uint32_t a, std::uint32_t b) { return std::int32_t(a) < std::int32_t(b) ? a : b; };
            const auto larger = [](std::uint32_t a, std::uint32_t b) { return std::int32_t(a) > std::int32_t(b) ? a : b; };
            r[7] = larger(marker_top, x1);
            r[9] = smaller(marker_bottom, x2);
            r[5] = r[9] - r[7];
            r[8] = larger(marker_left, y1);
            r[10] = smaller(marker_right, y2);
            r[4] = r[10] - r[8];
            const auto positive = [](std::uint32_t v) { return std::int32_t(v) > 0 ? v : 0u; };
            const auto area = positive(r[5]) * positive(r[4]);
            // Its last srawi (by 31, of -w & ~w) leaves CA.
            const auto mask = (0u - r[5]) & ~r[5];
            c.s.wr<1>(guest::reg::xer_ca, std::int32_t(mask) < 0 && (mask & 0x7fffffff) != 0);
            r[6] = marker_right;
            put(0x1b8, area);
            if (std::int32_t(area) > std::int32_t(word(0x1c4))) {
                c.put16(at(0x1b4), word(0x1c0));
                c.put16(at(0x1b6), word(0x1bc));
                r15 = sext16(word(0x1bc)) & 0xffff;
                r14 = word(0x1b8);
                put(0x1c4, r14);
            }
            r13 = word(0x1bc);
            put(0x1bc, r13 + 1);
        }
        put(0x1c0, word(0x1c0) + 1);
    }
    c.set({{0, 36}, {3, word(0x1c4)}, {4, r[4]}, {5, r[5]}, {6, r[6]}, {7, r[7]}, {8, 36}, {9, r[9]}, {10, r[10]},
           {11, 35}, {13, r13}, {14, r14}, {15, r15}, {20, r20}, {25, r25}});
    if (!word(0x1c4)) return std::nullopt;
    const auto spot = word(0x1b4);  // row << 16 | column of the best cell
    c.set({{3, screen}, {4, spot}});
    const auto holder = direct<std::uint32_t>(0x103bddf4, [&] { return player_at_spot(c, screen, spot); });
    c.set({{16, holder}, {4, sext8(holder)}});
    if (byte(holder) > -1) {
        c.set({{0, sext8(holder)}, {3, sext8(c.u8(at(0x51)))}});
        if (byte(holder) != c.i8(at(0x51))) {
            c.set({{3, screen}, {4, player()}, {5, holder}});
            direct<std::uint32_t>(0x103bde28, [&] { return swap_spots(c, screen, player(), holder); });
            return leave(-11, 0x98, 15, 0x103bde3c);
        }
    }
    c.set(13, sext8(holder));
    if (byte(holder) != -1) return std::nullopt;
    c.set({{3, screen}, {4, player()}, {5, word(0x1b4)}});
    c.set(14, direct<std::uint32_t>(0x103bde64, [&] { return move_spot(c, screen, player(), word(0x1b4)); }));
    return leave(-11, 0x94, 3, 0x103bde7c);
}

// 0x1e on 0x1c: a player dragged on the pitch in the with-the-ball view: to
// the nearest position marker (closer than 800), his position with the ball.
std::optional<std::uint32_t> Handler::drag_with_ball() {
    if (action(0x103bde94) != 0x1e) return std::nullopt;
    c.set({{3, root}, {4, event}});
    const auto dropped = direct<std::uint32_t>(0x103bdeac, [&] { return control_76(c, root, event); });
    c.set(23, dropped);
    if (half(dropped) == -1) return std::nullopt;
    if (action_of(dropped, 0x103bdec8) != 0x1c) return std::nullopt;
    const auto chosen = sext8(value(0x103bdedc));
    c.set(13, chosen);
    c.put8(at(0x50), chosen);
    pitch(0x220, 0x103bdef8, {0x1b0, 0x1ac, 0x1a8, 0x1a4}, {0x103bdf04, 0x103bdf18, 0x103bdf28, 0x103bdf3c}, {14, 0, 0, 0});
    put(0x1a0, 800);
    c.set(3, 800);
    c.set(13, 0);
    put(0x19c, 0);
    // Scratch registers are set as the original's instructions leave them.
    for (;;) {
        const auto code = sext16(c.u16(positions + (word(0x19c) << 1)));
        c.set({{0, word(0x19c) << 1}, {3, code}});
        c.compare(0, half(code), -1);
        if (half(code) == -1) break;
        const auto index = word(0x19c);
        c.set({{3, screen}, {23, index}, {0, index << 1}, {4, sext16(c.u16(positions + (index << 1)))}});
        const auto central = call(0x103bab40, 0x103bdf6c);
        c.set({{10, central}, {0, word(0x19c)}, {3, sext16(c.u16(positions + (word(0x19c) << 1)))}, {4, word(0x1b0)},
               {5, word(0x1ac)}, {6, word(0x1a8)}, {7, word(0x1a4)}, {8, at(0x198)}, {9, at(0x194)}});
        call(0x103ce330, 0x103bdf98);
        const auto left = sext16(c.u16(at(0x220)));
        const auto width = sext16(c.u16(at(0x224))) - left;
        const auto rounded = (width >> 31) + width;
        const auto half_width = sra(rounded, 1);
        const auto dx = left + half_width - word(0x194);
        c.set({{13, rounded}, {14, half_width}, {23, dx}});
        put(0x190, dx);
        const auto top = sext16(c.u16(at(0x222)));
        put(0x71c, top);
        put(0x720, sext16(c.u16(at(0x226))));
        const auto height = word(0x720) - word(0x71c);
        const auto rounded_height = (height >> 31) + height;
        const auto dy = top + sra(rounded_height, 1) - word(0x198);
        c.set({{13, height}, {14, dy}, {5, rounded_height}, {8, word(0x198)}});
        put(0x18c, dy);
        c.set(15, word(0x18c));
        put(0x724, word(0x18c));
        const auto dx_squared = word(0x190) * word(0x190);
        c.set({{13, dx_squared}, {14, word(0x190)}});
        const auto squared = word(0x18c) * word(0x724) + dx_squared;
        c.set({{3, squared}, {4, squared ^ 0x80000000u}});
        const auto length = std::uint32_t(distance(squared, 0x6e4, 0x6ec, 0x103be048));
        c.set({{4, length}, {0, word(0x1a0)}});
        put(0x188, length);
        c.compare(0, std::int32_t(length), std::int32_t(word(0x1a0)));
        if (std::int32_t(length) < std::int32_t(word(0x1a0))) {
            c.put16(at(0x5e), c.u16(positions + (word(0x19c) << 1)));
            c.set({{0, sext16(c.u16(positions + (word(0x19c) << 1)))}, {13, word(0x188)}});
            put(0x1a0, word(0x188));
        }
        c.set({{14, word(0x19c)}, {3, word(0x19c) + 1}});
        put(0x19c, word(0x19c) + 1);
    }
    c.set(3, word(0x1a0));
    c.compare(0, std::int32_t(word(0x1a0)), 800);
    if (std::int32_t(word(0x1a0)) >= 800) return std::nullopt;
    const auto player = c.u8(at(0x50));
    const auto position = sext16(c.u16(at(0x5e)));
    c.set({{3, screen}, {4, player}, {5, position}, {6, 1}});
    direct<std::uint32_t>(0x103be0c0, [&] { return set_position(c, screen, player, position, 1); });
    return leave(-11, 0x90, 0, 0x103be0d4);
}

// 0x1f: a pitch cell clicked (value: row * 100 + column); 0x20: close;
// 0x21: back to the match; else the generic handlers.
std::uint32_t Handler::remaining() {
    c.set(14, action(0x103be0ec));
    if (c.r(3) == 0x1f) {
        c.put16(at(0x184), c.u16(c.r(2) - 0x3826));
        c.put16(at(0x186), c.u16(c.r(2) - 0x3824));
        const auto chosen = value(0x103be114);
        c.set(14, chosen);
        const auto row = std::uint32_t(divide_ca(0x51eb851f, 5, std::int32_t(chosen)));  // / 100
        c.set(13, row);
        c.put16(at(0x184), row);
        const auto again = value(0x103be148);
        const auto column = again - std::uint32_t(divide_ca(0x51eb851f, 5, std::int32_t(again))) * 100;
        c.put16(at(0x186), column);
        c.put16(at(0x5c), column);
        const auto cell = word(0x184);
        c.set({{3, screen}, {4, cell}});
        c.set(5, direct<std::uint32_t>(0x103be188, [&] { return set_cell(c, screen, cell); }));
        const auto code = leave(-11, 0x8c, 13, 0x103be1a0);
        put(0x6f4, c.r(3));
        return code;
    }
    if (action(0x103be1b8) == 0x20) {
        c.set(3, screen);
        tactics_leaf_103be410(c.s);  // the changed byte
        put(0x6f8, c.r(3));
        c.set(13, sext8(word(0x6f8)));
        if (!byte(word(0x6f8))) {
            c.set({{3, screen}, {4, 1}});
            c.set(5, call(0x103b3b50, 0x103be26c));
            return leave(-9, 0x84, 0, 0x103be284);
        }
        c.set(14, print(text, 0xc83, 0x103be1e8));
        c.set(15, print(at(0x552), 0xc76, 0x103be1f8));
        put(0x6fc, print(at(0x61a), 0x27, 0x103be208));
        put(0x728, c.u16(at(0x64)));
        put(0x700, c.u16(at(0x62)));
        c.set({{3, word(0x6fc)}, {4, c.r(15)}, {5, c.r(14)}, {6, 1}, {7, sext16(word(0x728))}, {8, sext16(word(0x700))},
               {9, 0x31}});
        call(0x100f05a0, 0x103be244);
        return leave(-4, 0x88, 4, 0x103be258);
    }
    c.set(5, action(0x103be29c));
    if (c.r(3) == 0x21) {
        c.set({{3, screen}, {4, at(0x240)}});
        const auto shown = direct<std::uint32_t>(0x103be2b4, [&] { return match_changes(c, screen, at(0x240)); });
        c.set(14, shown);
        if (!byte(shown)) {
            c.set(3, screen);
            call(0x103c5b70, 0x103be33c);
            c.set({{3, screen}, {4, 1}});
            call(0x103b3b50, 0x103be348);
            return leave(-9, 0x7c, 4, 0x103be360);
        }
        c.set(13, print(at(0x552), 0xcc7, 0x103be2d0));
        c.set(15, print(at(0x61a), 0x27, 0x103be2e0));
        c.set({{3, c.r(15)}, {4, c.r(13)}, {5, at(0x240)}, {6, 1}, {7, x()}, {8, y()}, {9, 0x32}});
        call(0x100f0720, 0x103be30c);
        put(0x704, 0xfffffffcu);
        const auto code = leave(-4, 0x80, 0, 0x103be328);
        put(0x708, c.r(3));
        return code;
    }
    c.set(3, event);
    const auto first = call(0x102a54b0, 0x103be374);
    c.set(4, first);
    put(0x180, first);
    if (first) {
        put(0x78, first);
        c.set({{3, at(0x240)}, {4, 0xffffffffu}});
        call(0x1074b7c0, 0x103be39c);
        c.set(4, c.r(3));
        return first;
    }
    c.set(3, event);
    const auto second = call(0x102a83a0, 0x103be3b0);
    put(0x180, second);
    c.set(4, second);
    if (second) {
        put(0x74, second);
        c.set({{3, at(0x240)}, {4, 0xffffffffu}});
        call(0x1074b7c0, 0x103be3d8);
        return second;
    }
    return leave(0, 0x70, 25, 0x103be3f4);
}

std::uint32_t Handler::actions() {
    if (const auto code = files_actions()) return *code;
    if (const auto code = opposition()) return *code;
    if (const auto code = edit_actions()) return *code;
    return remaining();
}

std::uint32_t Handler::run() {
    c.set({{27, event}, {28, positions}, {17, c.toc(-0x7b4c)}, {18, c.toc(-0x7b80)}, {19, c.toc(-0x6228)},
           {22, managers}, {26, text}, {30, root}, {31, pool}});
    c.set({{3, at(0x240)}, {4, 0}});
    call(0x1074b6e0, 0x103baf20);  // the stack object every exit destroys
    c.set({{5, c.r(3)}, {3, root}, {4, 0}});
    screen = call(0x1073ef70, 0x103baf30);  // page parameter 0: the state object
    c.set({{13, screen}, {29, screen}});
    if (!screen) {
        // First event: the state object from the page parameters.
        c.set(3, field::size);
        const auto created = call(0x10773520, 0x103baf4c);
        put(0x6f4, created);
        put(0x1f8, created);
        if (created) {
            put(0x6f8, sext8(parameter(8, 0x103baf70)));
            c.set(13, parameter(7, 0x103baf84));
            const auto in_match = parameter(4, 0x103baf94);
            c.set({{14, in_match}, {15, sext8(in_match)}});
            put(0x6fc, parameter(3, 0x103bafa8));
            put(0x728, sext8(word(0x6fc)));
            put(0x700, parameter(2, 0x103bafc4));
            const auto club = parameter(1, 0x103bafd4);
            c.set({{4, club}, {3, word(0x1f8)}, {5, word(0x700)}, {6, word(0x728)}, {7, c.r(15)}, {8, c.r(13)},
                   {9, word(0x6f8)}});
            call(0x103c4b10, 0x103baff4);
        }
        screen = word(0x1f8);
        c.set(29, screen);
        set_parameter(0, screen, 0x103bb00c);
    }
    // The club's handler sees the event first; it leaves the dialog position at sp+0x64 / sp+0x62.
    c.set({{3, screen}, {4, at(0x64)}, {5, at(0x62)}});
    direct<std::uint64_t>(0x103bb01c, [&] { return club_event(c, screen, at(0x64), at(0x62)); });
    c.set({{5, c.r(3)}, {3, root}});
    if (byte(call(0x10329e10, 0x103bb028)))  // a dialog answered
        if (const auto code = answers()) return *code;
    return actions();
}
}

std::uint32_t handle(Cpu& c, std::uint32_t event) {
    Frame frame(c, 0x103baee0, 0x780, {.first_gpr = 13});
    Handler handler(c, frame, event);
    return handler.run();
}
}
