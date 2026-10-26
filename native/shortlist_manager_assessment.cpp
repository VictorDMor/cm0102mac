#include "shortlist_manager_detail.hpp"

// SHORTLIST_MANAGER.CPP: the position group a shortlist entry's position
// flags fall in (0x10359980) and a club's target reputation (0x10359ba0),
// both read by the contribution assessment (shortlist_manager_assessment_contribution.cpp).
namespace cm::aot::shortlist_manager {
using namespace detail;
std::int32_t position_group(Cpu& c, std::int16_t position) {
    // The compiler's binary search over the switch's cases, compare by
    // compare: cr0 is left by the last one. Flags with one bit (or the
    // pairs the game uses) map to the groups the contribution's jump
    // tables index; everything else is group 0xe.
    const std::int32_t p = position;
    const auto against = [&](std::int32_t value) { return compare(c, p, value); };
    const auto only = [&](std::int32_t value, std::int32_t group) { return against(value) == 0 ? group : 0xe; };
    if (const auto at_140 = against(0x140); at_140 == 0) return 0xe;
    else if (at_140 < 0) {
        if (const auto at_84 = against(0x84); at_84 == 0) return 2;
        else if (at_84 < 0) {
            if (const auto at_8 = against(0x8); at_8 == 0) return 0x13;
            else if (at_8 < 0) {
                if (const auto at_3 = against(0x3); at_3 == 0) return 0xe;
                else if (at_3 < 0) {
                    const auto at_1 = against(0x1);
                    return at_1 == 0 ? 0 : at_1 > 0 ? 1 : 0xe;
                }
                return against(0x5) >= 0 ? 0xe : 0x11;
            }
            if (const auto at_20 = against(0x20); at_20 == 0) return 0x15;
            else if (at_20 > 0) return only(0x40, 0x14);
            return only(0x10, 0x12);
        }
        if (const auto at_104 = against(0x104); at_104 == 0) return 4;
        else if (at_104 > 0) {
            if (const auto at_110 = against(0x110); at_110 == 0) return 9;
            else if (at_110 > 0) return only(0x120, 0xc);
            return only(0x108, 6);
        }
        if (const auto at_90 = against(0x90); at_90 == 0) return 8;
        else if (at_90 > 0) return only(0xa0, 0xb);
        return only(0x88, 5);
    }
    if (const auto at_410 = against(0x410); at_410 == 0) return 9;
    else if (at_410 < 0) {
        if (const auto at_220 = against(0x220); at_220 == 0) return 0xc;
        else if (at_220 < 0) {
            if (const auto at_208 = against(0x208); at_208 == 0) return 6;
            else if (at_208 > 0) return only(0x210, 9);
            return only(0x204, 4);
        }
        if (const auto at_404 = against(0x404); at_404 == 0) return 4;
        else if (at_404 > 0) return only(0x408, 6);
        return only(0x240, 0xe);
    }
    if (const auto at_808 = against(0x808); at_808 == 0) return 7;
    else if (at_808 < 0) {
        if (const auto at_440 = against(0x440); at_440 == 0) return 0xe;
        else if (at_440 > 0) return only(0x804, 3);
        return only(0x420, 0xc);
    }
    if (const auto at_820 = against(0x820); at_820 == 0) return 0xd;
    else if (at_820 > 0) return 0xe;
    return only(0x810, 0xa);
}

std::int32_t club_target_reputation(Cpu& c, std::uint32_t shortlists, std::uint32_t club_address) {
    Frame frame(c, 0x10359ba0, 0x80, {.first_gpr = 29});
    Fpu fpu(c);
    const auto sp = frame.base();
    const auto integer = [&](std::int32_t offset, std::int32_t value) {
        return fpu.fsub(stacked_integer(c, sp + std::uint32_t(offset), signed_word(value)), k::signed_bias);
    };
    const auto truncated = [&](std::int32_t offset, unsigned to, double value) {
        const auto bits = fpu.fctiwz(to, value);
        c.put64(sp + std::uint32_t(offset), bits);
        return std::int16_t(bits);
    };
    keep(c, {{31, c.toc(slot::constants)}});
    const Club club{c, club_address};
    const Kept<29> club_kept{c, club_address};
    // The club's entry in the manager's per-club table (12 bytes each);
    // +6 is the target reputation (inferred).
    const auto table = Field<std::uint32_t>{c, shortlists + 0xdc}.get();
    const auto offset = std::uint32_t(club.id()) * 12;
    const Kept<30> entry{c, table + offset};
    const Field<std::int16_t> target{c, entry + 6};
    // mulhw by 0x66666667: the division by 10 truncates toward zero.
    const std::int32_t tenth = club.attendance() / 10;
    target = std::int16_t(tenth + 4500);
    leave(c, {{8, std::uint32_t(tenth)}, {9, offset}, {10, table}});
    const auto human = Program{c}.call(0x1014c2e0, 0x10359c0c, {club_address, 1, 1, 0, 0});
    if (std::int8_t(human) == 0) {
        if (const auto manager = club.manager().get(); manager != 0) {
            // Quirk: the manager's staff record is not checked for null.
            const Person person{c, manager};
            const StaffRecord staff{c, person.non_player()};
            const std::int32_t standing = std::int32_t(person.ambition()) - std::int32_t(staff.standing());
            const std::int32_t start = target;
            c.set(4, 0x43300000);
            const double difference = integer(0x58, standing);
            const double scale = fpu.fdiv(difference, 80.0);
            const double from = integer(0x60, start);
            c.set_f(1, from);  // the original computes it into f1
            target = truncated(0x50, 0, fpu.fmul(from, fpu.fadd(1.0, scale)));
        }
    }
    const auto assistant = club.assistant().get();
    c.set(5, assistant);
    if (assistant != 0) {
        if (const auto record = Person{c, assistant}.non_player().get(); record != 0) {
            const Person person{c, assistant};
            const StaffRecord staff{c, record};
            const std::int32_t standing = std::int32_t(person.ambition()) - std::int32_t(staff.standing());
            const std::int32_t start = target;
            c.set(5, std::uint32_t(start));
            c.put32(sp + 0x40, 0x43300000);
            c.put32(sp + 0x48, 0x43300000);
            const double difference = integer(0x58, standing);
            const double scale = fpu.fdiv(difference, 60.0);
            const double from = integer(0x50, start);
            target = truncated(0x60, 1, fpu.fmul(from, fpu.fadd(1.0, scale)));
            const std::int32_t middle = target;
            const std::int32_t ambition = StaffRecord{c, Person{c, club.assistant()}.non_player()}.ambition();
            c.set(4, signed_word(middle));
            const double from_middle = integer(0x48, middle);
            const double drive = integer(0x40, ambition - 10);
            c.set_f(1, drive);  // the original computes it into f1
            target = truncated(0x38, 0, fpu.fmul(from_middle, fpu.fadd(1.0, fpu.fdiv(drive, 40.0))));
        }
    }
    // At least the club's reputation, at most 10000.
    const std::int16_t reputation = club.reputation();
    const std::int32_t value = target;
    if (compare(c, reputation, value) > 0) target = reputation;
    else target = std::int16_t(compare(c, value, 10000) > 0 ? 10000 : value);
    return result(c, std::uint32_t(std::int32_t(target.get())));
}
}
