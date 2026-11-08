// MATCH_MAN.CPP rating_c block (match_man_rating_c.hpp): 0x10211e70, the entry and its first phases.
#include "match_man_rating_c.hpp"
#include "match_man_adapter.hpp"
#include <cstdio>
#include <cstdlib>
#include "match_man_rating_c_detail.hpp"

namespace cm::aot::match_man {
using namespace placement;

Placement::Placement(Cpu& cpu, Fpu& f, Frame& fr)
    : c(cpu), fpu(f), frame(fr), relation(cpu, cpu.r(13)), adjust(cpu, cpu.r(14)), person(cpu, cpu.r(3)),
      data(cpu, cpu.r(4)), record(cpu, cpu.r(5)), flags(cpu, cpu.r(7)), club(cpu, cpu.r(8)), judge_arg(cpu, cpu.r(10)),
      second_word(cpu, cpu.r(21)), alt_role(cpu, cpu.r(22)), age(cpu, cpu.r(23)), total(cpu, 0), count(cpu, 0),
      staff(cpu, cpu.r(26)), second(cpu, 0), position(cpu, 0), worth(cpu, cpu.r(29)), modifier(cpu, 0),
      doubles(cpu, cpu.toc(-0x5278)) {}

i32 Placement::divw(i32 a, i32 b) const { return i32(u32(divide_signed<4>(reg(a), reg(b)))); }

bool Placement::in_people_table() const {
    if (!nonzero_logical(c, person)) return true;
    const u32 count_at = c.toc(-0x7be4);
    const i32 index = c.i32(person), people = c.i32(count_at);
    c.set({{3, count_at}, {4, reg(index)}, {0, reg(people)}});
    compare(c, index, people);
    return index < people;
}

// The running average (200 times the total over the count, or over 1 right after the first
// component) below the floor ends the routine with -10000.
bool Placement::floor_fails(bool by_count) {
    const i32 scaled = i32(reg(i32(u32(total))) * 200);
    c.set(3, reg(scaled));
    const i32 average = by_count ? divw(scaled, counted()) : scaled;  // the first check divides by a literal 1
    c.set(3, reg(average));
    compare(c, average, c.i32(at(at::floor)));
    return average < c.i32(at(at::floor));
}

std::int32_t rating_c(Cpu& c, std::uint32_t person_arg, std::uint32_t data_arg, std::uint32_t record_arg, std::uint32_t role_arg,
                      std::uint32_t flags_arg, std::uint32_t club_arg, std::uint32_t side_arg, std::uint32_t judge_in) {
    (void)person_arg; (void)data_arg; (void)record_arg; (void)flags_arg; (void)club_arg; (void)judge_in;
    c.set(0, c.lr());
    Frame frame(c, entry, 0x160, {.first_gpr = 13, .gpr_offset = -0x5c, .first_fpr = 31});
    Fpu fpu(c);
    Placement p(c, fpu, frame);
    if (const auto early = p.begin(role_arg, side_arg)) return *early;
    p.load_context();
    p.position_values();
    if (const auto early = p.averages()) return *early;
    p.trends();
    p.form();
    p.overall();
    p.place_bonuses();
    p.club_factor();
    if (const auto early = p.development()) return *early;
    if (const auto early = p.market()) return *early;
    p.staff_fit();
    p.side_and_nation();
    p.relations();
    p.ending();
    return p.finish();
}
}
