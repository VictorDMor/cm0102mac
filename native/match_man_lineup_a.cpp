// MATCH_MAN.CPP lineup_a block (match_man_lineup_a.hpp): the entry, the setup
// and the check whether the manager should swap the lineup for a weaker rival.
#include "match_man_lineup_a.hpp"
#include "match_man_lineup_a_detail.hpp"

namespace cm::aot::match_man {
using namespace guest;

std::uint32_t computer_lineup(Cpu& c, std::uint32_t side, std::uint32_t team, std::uint32_t mode) {
    c.set(0, c.lr());
    ComputerLineup lineup(c, side, team, mode);
    return lineup.run();
}

ComputerLineup::ComputerLineup(Cpu& cpu, std::uint32_t side_record, std::uint32_t team_record, std::uint32_t mode_value)
    : c(cpu),
      frame(cpu, 0x10206ea0, 0x960, {.first_gpr = 13}),
      fpu(cpu),
      r13(cpu, cpu.toc(-0x7be4)),   // the people count's address
      r14(cpu, cpu.toc(-0x7b7c)),
      r15(cpu, cpu.r(15)),
      r16(cpu, cpu.r(16)),
      r17(cpu, cpu.r(17)),
      r18(cpu, cpu.toc(-0x7b90)),   // the random seed word
      r19(cpu, cpu.toc(-0x7b84)),   // the random table cursor
      r20(cpu, cpu.toc(-0x7a6c)),   // the squad registry's global
      r21(cpu, cpu.toc(-0x7bac)),   // the shared success flag
      r22(cpu, cpu.toc(-0x7ba8)),   // the message buffer
      r23(cpu, cpu.toc(-0x5274)),   // the module's literal pool
      r24(cpu, side_record),
      r25(cpu, mode_value),
      r26(cpu, cpu.r(26)),
      r27(cpu, cpu.toc(-0x7a4c)),
      r28(cpu, cpu.toc(-0x7a2c)),
      r29(cpu, cpu.r(29)),
      r30(cpu, cpu.toc(-0x7b78)),
      r31(cpu, cpu.toc(-0x7aa8)) {
    c.put32(at(0x97c), team_record);  // stw r4,0x97c(r1): the caller's parameter slot
}

std::uint32_t ComputerLineup::run() {
    begin();
    assess_replacement();
    first_pass();
    second_scan();
    choose_flags();
    reorder_candidates();
    fill_slots();
    second_arrangement();
    check_duplicates();
    rotate_lists();
    final_picks();
    tail_flags();
    // The destructor of the object at sp+0x130, then the count at sp+0x906.
    c.set({{3, at(0x130)}, {4, 0xffffffff}});
    c.call(0x10738c20, 0x10209cf8);
    return result(c, c.u8(at(0x906)));
}

// The assertion report and the exit(-1) that follows it at the three sites
// that exit: `at_address` is the first call of the report.
void ComputerLineup::assert_that(bool condition, std::uint32_t at_address, std::uint32_t line, std::int32_t name,
                                 std::int32_t extension) {
    if (condition) return;
    report(at_address, line, name, extension);
    c.set(3, 0xffffffff);
    c.call(0x1076f7d0, at_address + 0x44);  // exit(-1); where it returns, the original goes on
}

// The report alone (the duplicate check at 0x102091e8 does not exit).
void ComputerLineup::report(std::uint32_t at_address, std::uint32_t line, std::int32_t name, std::int32_t extension) {
    const std::uint32_t pool_address = r23;
    report_assertion(c, {.at = at_address, .file = pool_address, .format = pool_address + 0xe, .title = pool_address + 0x1b,
                         .name = at(name), .extension = at(extension), .line = line});
}

// The setup: the stack objects, the squad size and the first swap inputs.
void ComputerLineup::begin() {
    c.put8(at(0x907), 0);
    c.put8(at(0x906), 0);
    c.set(3, at(0x130));
    c.call(0x10738c00, 0x10206f04);
    r26 = 0;
    c.set(3, at(0x120));
    c.call(0x10715880, 0x10206f10);
    c.set({{4, c.toc(-0x7b80)}, {3, at(0xe0)}, {5, 2}});
    c.call(0x10717890, 0x10206f20);
    // A five-byte record (halfword, halfword, byte) copied from the result.
    c.put16(at(0x118), c.u16(at(0xe0)));
    c.put16(at(0x11a), c.u16(at(0xe2)));
    c.put8(at(0x11c), c.u8(at(0xe4)));
    c.put8(at(0x905), 0);
    c.put8(at(0x904), 0);
    // The opposing record may not be missing.
    assert_that(compare_logical(c, team()) != 0, 0x10206f68, 0x174, 0x1a0, 0x110);

    c.set({{3, at(0x3d8)}, {4, 0}, {5, 0x64}});
    c.call(0x10778300, 0x10206fbc);
    c.set({{3, at(0x2ac)}, {4, 0}, {5, 0x64}});
    c.call(0x10778300, 0x10206fcc);
    c.set({{3, at(0x310)}, {4, 0xffffffff}, {5, 0x64}});
    c.call(0x10778300, 0x10206fdc);
    c.set({{3, at(0x694)}, {4, 0}, {5, 0xc8}});
    c.call(0x10778300, 0x10206fec);
    c.set({{3, at(0x1c0)}, {4, 0}, {5, 0x14}});
    c.call(0x10778300, 0x10206ffc);

    // The squad considered: 11 plus the high nibble of the settings' byte +0x4e.
    const auto settings = c.u32(c.u32(side() + lineup_side::match) + 0x3118);
    const auto bench = (c.u8(settings + 0x4e) >> 4) & 0xf;
    r29 = std::uint32_t(s8(bench + 0xb));
    c.set({{3, club()}, {4, 0}, {5, 1}});
    c.call(0x100c9070, 0x10207024);
    r15 = c.r(3);
    c.put8(side() + lineup_side::changed, 0);
}

// The club's four-way test: `person` is one of the three words at club+0x74.
bool ComputerLineup::is_club_slot(std::uint32_t club_record, std::uint32_t person) {
    for (const std::uint32_t offset : {0x74u, 0x78u, 0x7cu})
        if (compare_logical(c, person, c.u32(club_record + offset)) == 0) return true;
    return false;
}

// The int-to-double idiom (words 0x43300000 and value ^ 0x80000000 at the
// address, loaded as a double, less the pool's 2^52 + 2^31 with fsub).
double ComputerLineup::convert(std::uint32_t at_address, std::int32_t value) {
    c.put32(at_address, 0x43300000);
    c.put32(at_address + 4, std::uint32_t(value) ^ 0x80000000u);
    return fpu.fsub(c.f64(at_address), c.f64(pool() + lineup_pool::conversion));
}

// value * factor (a pool double), truncated by fctiwz and stored (stfd) at
// `store_at`, whose low word is read back. Leaves f1 as the original does.
std::uint32_t ComputerLineup::scaled(std::uint32_t value, std::uint32_t factor, std::uint32_t convert_at,
                                     std::uint32_t store_at) {
    const double raw = convert(at(convert_at), std::int32_t(value));
    const double product = fpu.fmul(c.f64(pool() + factor), raw);
    c.set_f(1, c.f64(pool() + lineup_pool::conversion));
    return truncated_to_stack(c, fpu, at(store_at), product);
}

// A club's strength as the swap check scores it: a third of the club's rating
// (+0x1ec) plus 0x14d times a rating of `row` (+0x18).
std::int32_t ComputerLineup::rating_of(std::uint32_t club_record, std::uint32_t row) const {
    const std::int32_t third = std::int32_t(c.i16(club_record + 0x1ec)) / 3;  // mulhw by 0x55555556
    return std::int32_t(std::uint32_t(third) + std::uint32_t(std::int32_t(c.i16(row + 0x18)) * 0x14d));
}
}
