#include "actor_construction_random.hpp"

namespace cm::aot::actor_construction {
namespace {
// The branch condition has already been established from the original signed
// team byte. The separate loads are intentional: callbacks/aliases can change
// fixture pointers between them.
void select_fixture_club(ConstructionCall& a, std::uint32_t branch, unsigned destination) {
    a.at(branch);
    if (!a.equal()) { a.load(3, 3, 0x3118); a.load(destination, 3, 0x20); a.jump(branch + 0x18); }
    else { a.at(branch + 0x10); a.load(3, 3, 0x3118); a.load(destination, 3, 0x1c); }
    a.at(branch + 0x18);
}
void begin_actor(ConstructionCall& a) {
    a.link(); a.floating_store(31, 1, -8); a.floating_store(30, 1, -16); a.multiple(13, false, -16);
    a.move(16, 5); a.move(18, 8); a.load(23, 2, -0x7b3c); a.load(14, 2, -0x7b80);
    a.move(15, 3); a.load(24, 2, -0x7b5c); a.move(17, 6); a.load(13, 2, -0x7ba0);
    a.value(20, 0); a.load(25, 2, -0x7be4); a.value(19, 0); a.load(26, 2, -0x51e8);
    a.load(27, 2, -0x7b90); a.load(28, 2, -0x7b8c); a.load(29, 2, -0x7b88);
    a.load(30, 2, -0x7b84); a.load(31, 2, -0x51e0); a.save_link(); a.frame(0xd0);
    a.store(3, 0x1a4, 4); a.store(3, 0x6c, 16); a.store<1>(3, 0, 7); a.store<1>(3, 0x2f, 18);
}
void choose_player_record(ConstructionCall& a) {
    a.load(0, 5, 0x14); a.compare(0, 0, false);
    if (!a.equal()) { a.load(0, 16, 0x14); a.store(15, 0x70, 0); a.jump(0x10223d08); }
    else {
        a.at(0x10223b44); a.sign8(0, 17, true); a.load(3, 15, 0x1a4); select_fixture_club(a, 0x10223b4c, 5);
        a.offset(8, 5, 0x1ee); a.load(3, 2, -0x7bb8); a.load(4, 2, -0x51e4); a.load(5, 16, 0);
        a.load(6, 16, 0x28); a.load(7, 16, 0x2c); a.call(0x1077b720); a.load(3, 2, -0x74d4); a.load(21, 3, 0);
        draw_uniform<21,22,0x5c,0x54,0x4c,true>(a, 0x10223b88);
        a.load(3, 2, -0x7b38); a.multiply(0, 22, 0x48); a.load(3, 3, 0); a.add(0, 3, 0); a.store(15, 0x70, 0);
    }
    a.at(0x10223d08); a.load(0, 15, 0x6c); a.compare(0, 0, false);
    bool synthetic_staff = false;
    if (!a.equal()) { a.load(3, 15, 0x6c); a.load(0, 25, 0); a.load(3, 3, 0); a.compare_registers(3, 0); synthetic_staff = !a.less(); }
    if (synthetic_staff) { a.value(0, 10); a.store<1>(15, 0x17, 0); a.jump(0x10223d40); }
    else { a.at(0x10223d34); a.load(3, 15, 0x70); a.load<1>(0, 3, 0x45); a.store<1>(15, 0x17, 0); }
    a.at(0x10223d40); a.compare(16, 0, false);
    bool known_staff = true;
    if (!a.equal()) { a.load(3, 16, 0); a.load(0, 25, 0); a.compare_registers(3, 0); known_staff = !a.at_least(); }
    if (known_staff) {
        a.at(0x10223d58); a.load<1>(0, 15, 0x27); a.load(3, 15, 0x1a4); a.sign8(0, 0, true);
        select_fixture_club(a, 0x10223d64, 5); a.load(3, 2, -0x7b7c); a.move(4, 16); a.load(3, 3, 0); a.call(0x10082f90); a.move(20, 3);
    }
    a.at(0x10223d90);
}
void copy_attributes_and_clubs(ConstructionCall& a) {
    a.load(3, 15, 0x70); a.sign8(0, 17, true); a.load<2>(0, 3, 10, true); a.store<2>(15, 0x48, 0);
    a.load(3, 15, 0x70); a.load<2>(0, 3, 8, true); a.store<2>(15, 0x4a, 0);
    a.load(3, 15, 0x70); a.load<2>(0, 3, 12, true); a.store<2>(15, 0x4c, 0);
    if (!a.equal()) {
        a.load(3, 15, 0x1a4); a.load(3, 3, 0x3118); a.load(0, 3, 0x20); a.store(15, 0xfc, 0);
        a.load(3, 15, 0x1a4); a.load(3, 3, 0x3118); a.load(21, 3, 0x1c); a.jump(0x10223df8);
    } else {
        a.at(0x10223ddc); a.load(3, 15, 0x1a4); a.load(3, 3, 0x3118); a.load(0, 3, 0x1c); a.store(15, 0xfc, 0);
        a.load(3, 15, 0x1a4); a.load(3, 3, 0x3118); a.load(21, 3, 0x20);
    }
    a.at(0x10223df8);
}
void initialize_fitness_and_position(ConstructionCall& a) {
    a.load(3, 13, 0); a.move(4, 16); a.value(5, 0); a.call(0x101878d0);
    a.value(0, 0x43300000); a.sign16(3, 3); a.store(1, 0x4c, 0); a.exclusive_flags(0, 3, 0x80000000);
    a.floating_load(1, 26, 0x80); a.move(4, 16); a.store(1, 0x50, 0); a.value(5, 0); a.load(3, 2, -0x7a7c);
    a.floating_load(0, 1, 0x4c); a.floating_subtract(0, 0, 1, true); a.floating_store_single(0, 15, 0x50);
    a.floating_load(0, 15, 0x50, true); a.floating_store_single(0, 15, 0x54); a.load(3, 3, 0); a.call(0x103d0990);
    a.floating_load(0, 31, 0, true); a.move(4, 16); a.floating_store_single(0, 15, 0x5c); a.load(3, 13, 0); a.call(0x10187680);
    a.sign8(0, 3); a.compare(0, 2);
    if (a.equal()) { a.value(0, 0xfffffffe); a.store<1>(15, 0x19, 0); a.jump(0x10223eb4); }
    else {
        a.at(0x10223e74); a.sign8(0, 18, true); bool starting = !a.less();
        if (starting) { a.sign8(0, 18); a.compare(0, 10); starting = !a.positive(); }
        if (starting) { a.store<1>(15, 0x19, 18); a.move(3, 15); a.call(0x10289560); a.store<2>(15, 0x1c2, 3); a.jump(0x10223eb4); }
        else {
            a.at(0x10223e9c); a.value(0, 0xffffffff); a.value(3, 0x1900); a.store<1>(15, 0x19, 0);
            a.value(0, 0); a.store<2>(15, 0x36, 3); a.store<2>(15, 0x1c2, 0);
        }
    }
    a.at(0x10223eb4);
}
bool opponent_lists_club(ConstructionCall& a, std::uint32_t start) {
    a.at(start); a.load(3, 15, 0xfc); a.load(0, 21, 0x74); a.compare_registers(3, 0, false);
    if (a.equal()) return true;
    a.load(3, 15, 0xfc); a.load(0, 21, 0x78); a.compare_registers(3, 0, false);
    if (a.equal()) return true;
    a.load(3, 15, 0xfc); a.load(0, 21, 0x7c); a.compare_registers(3, 0, false);
    return a.equal();
}
void initialize_affiliation(ConstructionCall& a) {
    a.value(0, 0xffffffff); a.value(3, 0x43300000); a.store<1>(15, 0x1f, 0); a.value(0, 0xffffffff);
    a.value(6, 0xffffffff); a.value(5, 0xffffffff); a.store<1>(15, 0x22, 0); a.value(0, 0);
    a.floating_load(1, 26, 0x80); a.move(4, 21); a.store<1>(15, 0x20, 6); a.store<1>(15, 0x21, 5);
    a.load(5, 15, 0x6c); a.store(1, 0x4c, 3); a.load<1>(3, 5, 0x69); a.sign8(3, 3);
    a.mask(3, 3, 0xfffffffe, false, 1); a.subtract_from(3, 3, 50); a.exclusive_flags(3, 3, 0x80000000);
    a.store(1, 0x50, 3); a.floating_load(0, 1, 0x4c); a.floating_subtract(0, 0, 1, true);
    a.floating_store_single(0, 15, 0x1a8); a.store<2>(15, 0x32, 0); a.load(3, 15, 0x6c); a.call(0x100d9480); a.sign8(0, 3, true);
    if (!a.equal()) { a.load<2>(3, 15, 0x32, true); a.offset(0, 3, 10); a.store<2>(15, 0x32, 0); a.jump(0x10223f54); }
    else {
        a.at(0x10223f34); a.load(3, 15, 0x6c); a.move(4, 21); a.call(0x100d94d0); a.sign8(0, 3, true);
        if (!a.equal()) { a.load<2>(3, 15, 0x32, true); a.offset(0, 3, -20); a.store<2>(15, 0x32, 0); }
    }
    if (opponent_lists_club(a, 0x10223f54)) { a.at(0x10223f84); a.load<2>(3, 15, 0x32, true); a.offset(0, 3, -10); a.store<2>(15, 0x32, 0); }
    a.at(0x10223f90); a.value(0, 10); a.store<2>(15, 0x34, 0); a.load(3, 15, 0x6c); a.load(4, 15, 0xfc); a.call(0x100d9480); a.sign8(0, 3, true);
    if (!a.equal()) { a.load<2>(3, 15, 0x34, true); a.offset(0, 3, 30); a.store<2>(15, 0x34, 0); a.jump(0x10223fdc); }
    else {
        a.at(0x10223fbc); a.load(3, 15, 0x6c); a.load(4, 15, 0xfc); a.call(0x100d94d0); a.sign8(0, 3, true);
        if (!a.equal()) { a.load<2>(3, 15, 0x34, true); a.offset(0, 3, -10); a.store<2>(15, 0x34, 0); }
    }
    if (opponent_lists_club(a, 0x10223fdc)) { a.at(0x1022400c); a.load<2>(3, 15, 0x34, true); a.offset(0, 3, 5); a.store<2>(15, 0x34, 0); }
    a.at(0x10224018); a.value(0, 0); a.value(4, 0); a.store<1>(15, 0x1d, 0); a.value(3, 0); a.value(0, 0);
    a.store<1>(15, 0x1e, 4); a.store<2>(15, 0x3a, 3); a.store<1>(15, 0x30, 0);
}
void unknown_eligibility(ConstructionCall& a, std::uint32_t start, bool jump = true) {
    a.at(start); a.value(3, 0xfffffc18); a.value(0, 0xfffffc18); a.store<2>(15, 0x3e, 3);
    a.value(5, 0xfffffc18); a.value(4, 0xfffffc18); a.value(3, 0xfffffc18); a.store<2>(15, 0x40, 0);
    a.value(0, 0); a.store<2>(15, 0x42, 5); a.store<2>(15, 0x44, 4); a.store<2>(15, 0x46, 3); a.store<1>(15, 0x31, 0);
    if (jump) a.jump(0x102243a4);
}
void special_club_eligibility(ConstructionCall& a, std::uint32_t start) {
    a.at(start); a.load<1>(0, 16, 0x5e); a.value(5, 0xfffffc18); a.value(4, 0xfffffc18); a.value(3, 0xfffffc18);
    a.leading_zeroes(6, 0); a.value(0, 0xfffffc18); a.mask(6, 6, 0x07ffffff, false, 27); a.store<1>(15, 0x31, 6);
    a.store<2>(15, 0x3e, 5); a.load<1>(5, 16, 0x5f); a.store<2>(15, 0x40, 5); a.store<2>(15, 0x44, 4);
    a.store<2>(15, 0x46, 3); a.store<2>(15, 0x42, 0); a.jump(0x102243a4);
}
bool eligibility_requires_history(ConstructionCall& a) {
    a.load(3, 15, 0x1a4); a.load<1>(0, 3, 0x7931); a.sign8(0, 0, true); if (a.equal()) return false;
    a.sign8(0, 17, true);
    if (a.equal()) {
        a.load(3, 15, 0x1a4); a.load(3, 3, 0x3118); a.load<1>(0, 3, 0x40); a.sign8(0, 0, true); if (a.equal()) return false;
    }
    a.at(0x10224064); a.sign8(0, 17, true);
    if (!a.equal()) { a.load(3, 15, 0x1a4); a.load(3, 3, 0x3118); a.load<1>(0, 3, 0x41); a.sign8(0, 0, true); if (a.equal()) return false; }
    a.at(0x10224080); a.compare(16, 0, false);
    if (!a.equal()) { a.load(3, 16, 0); a.load(0, 25, 0); a.compare_registers(3, 0); if (a.at_least()) return false; }
    a.at(0x10224098); a.load(3, 15, 0x1a4); a.load(3, 3, 0x3118); a.load<1>(0, 3, 0x3a); a.compare(0, 1);
    return !a.equal();
}
bool competition_history_available(ConstructionCall& a) {
    a.at(0x1022413c); a.load(3, 15, 0xfc); a.load(3, 3, 0x2c); a.load<1>(0, 3, 0x120); a.sign8(0, 0, true); if (a.equal()) return false;
    a.load(3, 15, 0xfc); a.load(0, 3, 0x30); a.compare(0, 0, false); if (a.equal()) return false;
    a.load(3, 15, 0xfc); a.load(4, 24, 0); a.load(3, 3, 0x30); a.load(0, 3, 0); a.mask(0, 0, 0xfffffffc, false, 2);
    a.indexed_load(0, 4, 0); a.compare(0, 0, false); if (a.equal()) return false;
    a.load(4, 15, 0xfc); a.load(3, 2, -0x7a24); a.load(4, 4, 0x2c); a.load(0, 3, 0); a.load(3, 4, 0x68); a.compare_registers(3, 0);
    if (!a.equal()) return true;
    a.load<2>(0, 14, 2); a.compare(0, 2002, false); return a.at_least();
}
void ordinary_eligibility(ConstructionCall& a) {
    a.at(0x102241dc); a.value(0, 0xfffffc18); a.store<2>(15, 0x40, 0); a.load(3, 15, 0x1a4); a.load(3, 3, 0x3118);
    a.load(0, 3, 0); a.compare(0, -1);
    bool use_attributes = !a.nonpositive();
    if (use_attributes) {
        a.load(3, 15, 0x1a4); a.load(3, 3, 0x3118); a.load(3, 3, 0); a.call(0x104646d0); a.sign8(0, 3, true); use_attributes = !a.equal();
    }
    if (use_attributes) {
        a.load(3, 23, 0); a.value(5, 2); a.load(4, 15, 0x70); a.call(0x10306a80); a.floating_integer(0, 1);
        a.move(4, 16); a.floating_store(0, 1, 0x4c); a.load(0, 1, 0x50); a.store<2>(15, 0x42, 0);
        a.load(3, 23, 0); a.call(0x10305ae0); a.store<2>(15, 0x44, 3); a.jump(0x10224254);
    } else {
        a.at(0x10224244); a.value(3, 0xfffffc18); a.value(0, 0xfffffc18); a.store<2>(15, 0x42, 3); a.store<2>(15, 0x44, 0);
    }
    a.at(0x10224254); a.load(3, 23, 0); a.move(4, 16); a.load(5, 15, 0xfc); a.call(0x10305700); a.sign16(0, 3);
    a.move(4, 16); a.leading_zeroes(0, 0); a.mask(0, 0, 0x07ffffff, false, 27); a.store<1>(15, 0x31, 0);
    a.load(3, 23, 0); a.load(5, 15, 0xfc); a.call(0x103058f0); a.store<2>(15, 0x3e, 3); a.value(0, 0xfffffc18); a.store<2>(15, 0x46, 0);
    a.load(0, 16, 0x24); a.compare(0, 0, false); if (a.equal()) return;
    a.load(3, 16, 0x24); a.move(4, 14); a.call(0x100d9440); a.sign8(0, 3, true); if (a.equal()) return;
    a.load(3, 23, 0); a.value(5, 6); a.load(4, 15, 0x70); a.value(6, 1); a.floating_load(1, 31, 4, true); a.call(0x10303560);
    a.floating_load(0, 26, 8); a.floating_compare(1, 0); a.compare_greater_or_equal_bit(); if (!a.equal()) return;
    a.load(3, 23, 0); a.value(5, 6); a.load(4, 15, 0x70); a.value(6, 2); a.floating_load(1, 31, 4, true); a.call(0x10303560);
    a.floating_integer(0, 1); a.floating_store(0, 1, 0x4c); a.load(0, 1, 0x50); a.store<2>(15, 0x46, 0); a.jump(0x102243a4);
}
void initialize_eligibility(ConstructionCall& a) {
    if (eligibility_requires_history(a)) {
        a.load(3, 15, 0xfc); a.call(0x100ce250); a.sign8(0, 3, true);
        if (!a.equal()) unknown_eligibility(a, 0x102240bc);
        else {
            a.at(0x102240f0); a.load(3, 15, 0xfc); a.call(0x100c7d00); a.sign8(0, 3, true);
            if (!a.equal()) special_club_eligibility(a, 0x10224100);
            else if (competition_history_available(a)) ordinary_eligibility(a);
            else unknown_eligibility(a, 0x102241a8);
        }
    } else {
        a.at(0x10224304); a.load(3, 15, 0xfc); a.call(0x100c7d00); a.sign8(0, 3, true); bool special = !a.equal();
        if (special) { a.load(3, 15, 0xfc); a.call(0x100ce250); a.sign8(0, 3, true); special = a.equal(); }
        if (special) { a.load(3, 15, 0x1a4); a.load(3, 3, 0x3118); a.load<1>(0, 3, 0x40); a.sign8(0, 0, true); special = !a.equal(); }
        if (special) special_club_eligibility(a, 0x10224338); else unknown_eligibility(a, 0x10224374, false);
    }
    a.at(0x102243a4);
}
void copy_preferences_and_names(ConstructionCall& a) {
    a.load(4, 15, 0x70); a.load(3, 15, 0x70); a.load<1>(4, 4, 0x30); a.load<1>(0, 3, 0x3b);
    a.sign8(3, 4); a.sign8(0, 0); a.compare_registers(3, 0);
    if (!a.nonpositive()) {
        a.value(0, 0); a.store<1>(15, 0x2d, 0); a.load(3, 15, 0x70); a.load<1>(0, 3, 0x3b);
        a.subtract_from(0, 0, 20); a.store<1>(15, 0x2e, 0); a.jump(0x102243f8);
    } else {
        a.at(0x102243e0); a.value(0, 1); a.store<1>(15, 0x2d, 0); a.load(3, 15, 0x70); a.load<1>(0, 3, 0x30);
        a.subtract_from(0, 0, 20); a.store<1>(15, 0x2e, 0);
    }
    for (auto field : {std::pair{0x102243f8u,0x28}, {0x1022441cu,0x2c}, {0x10224440u,0x30}}) {
        const auto [start, source] = field; const auto destination = 0x60 + source - 0x28;
        a.at(start); a.load(0, 16, source); a.compare(0, 0, false);
        if (!a.equal()) { a.load(3, 16, source); a.load(0, 3, 0x20); a.store(15, destination, 0); a.jump(start + 0x24); }
        else { a.at(start + 0x1c); a.value(0, 0xffffffff); a.store(15, destination, 0); }
    }
    a.at(0x10224464); a.store<1>(15, 0x27, 17);
}
void inspect_special_club_context(ConstructionCall& a) {
    a.load(0, 15, 0xfc); a.compare(0, 0, false); if (a.equal()) return;
    a.load(3, 15, 0xfc); a.load(0, 3, 0x2c); a.compare(0, 0, false); if (a.equal()) return;
    a.load(3, 15, 0xfc); a.load<2>(0, 3, 0x1ec, true); a.compare(0, 5750); if (a.nonpositive()) return;
    a.load<1>(0, 15, 0x27); a.load(4, 15, 0x1a4); a.sign8(0, 0); a.multiply(3, 0, 0xd8);
    a.offset(0, 3, 0x7aac); a.add_record(0, 4, 0); if (!a.equal()) return;
    a.load<1>(0, 15, 0x27); a.load(3, 15, 0x1a4); a.sign8(0, 0, true); select_fixture_club(a, 0x102244bc, 3);
    a.value(4, 0); a.value(5, 1); a.value(6, 0); a.value(7, 0); a.call(0x1014c2e0); a.sign8(0, 3, true); if (!a.equal()) return;
    a.load(3, 15, 0xfc); a.load(3, 3, 0x2c); a.load<1>(0, 3, 0x120); a.sign8(0, 0, true); if (!a.equal()) return;
    a.value(19, 1);
}
void initialize_random_preferences(ConstructionCall& a) {
    draw_small(a, 0x10224508, 0x66660000, 0x6667, 3, 20); a.store<1>(15, 0x11b, 0);
    a.load(3, 15, 0x70); a.load<1>(0, 3, 0xf); a.compare(0, 20);
    if (a.equal()) {
        a.load(3, 15, 0x70); a.load<1>(0, 3, 0x21); a.sign8(0, 0); a.subtract_from(14, 0, 35);
        draw_uniform<14,13,0x4c,0x54,0x5c>(a, 0x10224584); a.store<1>(15, 0x11c, 13); a.jump(0x1022487c);
    } else {
        a.at(0x102246f8); a.load(3, 15, 0x70); a.load<1>(0, 3, 0x21); a.sign8(0, 0); a.mask(0, 0, 0xfffffffe, false, 1);
        a.subtract_from(14, 0, 60); draw_uniform<14,13,0x4c,0x54,0x5c>(a, 0x1022470c); a.store<1>(15, 0x11c, 13);
    }
    a.at(0x1022487c);
}
void choose_affiliation_factor(ConstructionCall& a) {
    a.at(0x102248b4); a.load<1>(0, 15, 0x27); a.load(3, 15, 0x1a4); a.sign8(0, 0, true);
    select_fixture_club(a, 0x102248c0, 3); a.call(0x100c7d00); a.sign8(0, 3, true);
    if (!a.equal()) { a.floating_load(30, 26, 0x18); a.jump(0x1022497c); }
    else {
        a.at(0x102248ec); a.compare(20, 0, false);
        if (a.equal()) { a.at(0x10224978); a.floating_load(30, 26, 0x10); }
        else {
            a.load<1>(0, 20, 0x60); a.mask(0, 0, 15); a.compare(0, 2);
            if (a.equal()) { a.at(0x10224928); a.floating_load(30, 26, 0x28); a.jump(0x1022493c); }
            else if (!a.at_least()) {
                a.compare(0, 1);
                if (a.at_least()) { a.at(0x10224920); a.floating_load(30, 26, 0x20); a.jump(0x1022493c); }
                else { a.jump(0x10224938); a.floating_load(30, 26, 8); }
            } else {
                a.at(0x10224914); a.compare(0, 4);
                if (a.at_least()) { a.at(0x10224938); a.floating_load(30, 26, 8); }
                else { a.jump(0x10224930); a.floating_load(30, 26, 0x30); a.jump(0x1022493c); }
            }
            a.at(0x1022493c); a.load<1>(3, 20, 0x4c); a.value(0, 0x43300000); a.store(1, 0x4c, 0);
            a.sign8(0, 3); a.floating_load(2, 26, 0x80); a.exclusive_flags(0, 0, 0x80000000); a.floating_load(0, 26, 0x38);
            a.store(1, 0x50, 0); a.floating_load(3, 26, 0x10); a.floating_load(1, 1, 0x4c); a.floating_subtract(1, 1, 2);
            a.floating_difference(0, 1, 0, true); a.floating_sum(0, 3, 0); a.floating_product(30, 30, 0); a.jump(0x1022497c);
        }
    }
    a.at(0x1022497c);
}
void derive_fixture_reputation(ConstructionCall& a) {
    a.load(3, 15, 0x1a4); a.load(3, 3, 0x3118); a.load<1>(0, 3, 0x40); a.sign8(0, 0, true);
    if (!a.equal()) {
        a.load<1>(0, 15, 0x27); a.load(3, 15, 0x1a4); a.sign8(0, 0); a.leading_zeroes(0, 0);
        a.mask(0, 0, 0x07ffffff, false, 27); a.sign8(0, 0, true); select_fixture_club(a, 0x102249a8, 4);
        a.value(3, 0x10620000); a.load<2>(0, 4, 0x1ec, true); a.offset(3, 3, 0x4dd3); a.load(4, 15, 0x1a4);
        a.multiply_high(0, 3, 0); a.load(3, 4, 0x3118); a.load(4, 3, 0x14); a.shift_right(0, 0, 5);
        a.load<2>(4, 4, 0x18, true); a.mask(3, 0, 1, false, 1); a.add(0, 0, 3); a.add(3, 4, 0);
        a.mask(0, 3, 1, false, 1); a.add(0, 0, 3); a.shift_right(0, 0, 1); a.sign8(13, 0); a.jump(0x10224a18);
    } else {
        a.at(0x10224a04); a.load(3, 15, 0x1a4); a.load(3, 3, 0x3118); a.load(3, 3, 0x14); a.load<2>(0, 3, 0x18, true); a.sign8(13, 0);
    }
    for (auto item : {std::pair{0x10224a18u,0x74}, {0x10224a78u,0x78}, {0x10224ad8u,0x7c}}) {
        const auto [start, offset] = item;
        a.at(start); a.load<1>(0, 15, 0x27); a.load(3, 15, 0x1a4); a.sign8(0, 0); a.leading_zeroes(0, 0);
        a.mask(0, 0, 0x07ffffff, false, 27); a.sign8(0, 0, true); select_fixture_club(a, start + 0x18, 4);
        a.load<1>(0, 15, 0x27); a.load(3, 15, 0x1a4); a.sign8(0, 0, true); select_fixture_club(a, start + 0x3c, 3);
        a.load(0, 3, offset); a.compare_registers(0, 4, false);
        if (a.equal()) { a.at(0x10224b38); a.offset(13, 13, 10); break; }
    }
    a.at(0x10224b3c);
}
void repair_missing_competition_descriptor(ConstructionCall& a) {
    a.load(3, 15, 0x1a4); a.load(3, 3, 0x3118); a.load(0, 3, 0); a.compare(0, -1); if (a.nonpositive()) return;
    a.load(4, 15, 0x1a4); a.load(3, 24, 0); a.load(4, 4, 0x3118); a.load(0, 4, 0); a.mask(0, 0, 0xfffffffc, false, 2);
    a.indexed_load(0, 3, 0); a.compare(0, 0, false); if (!a.equal()) return;
    a.load(3, 2, -0x7a50); a.load(4, 15, 0x1a4); a.load(0, 3, 0); a.load(3, 4, 0x3118); a.load(4, 2, -0x7bbc);
    a.store(3, 0, 0); a.load(3, 2, -0x7a50); a.load(5, 15, 0x1a4); a.load(0, 3, 0); a.load(3, 4, 0);
    a.multiply(0, 0, 0x6c); a.load(4, 5, 0x3118); a.add(0, 3, 0); a.store(4, 0x14, 0);
}
void derive_personality_factor(ConstructionCall& a) {
    a.at(0x10224ba8); a.load(3, 15, 0x1a4); a.load(3, 3, 0x3118); a.load(0, 3, 0); a.compare(0, -1);
    if (!a.nonpositive()) {
        a.load<1>(0, 15, 0x27); a.load(3, 15, 0x1a4); a.sign8(0, 0, true); select_fixture_club(a, 0x10224bc8, 4);
        a.load(5, 15, 0x1a4); a.load(3, 24, 0); a.load(5, 5, 0x3118); a.load(0, 5, 0); a.mask(0, 0, 0xfffffffc, false, 2);
        a.indexed_load(3, 3, 0); a.call(0x10452ed0); a.sign8(0, 3, true); if (a.equal()) a.offset(13, 13, -10);
    }
    a.at(0x10224c08); a.sign8(3, 13); a.value(0, 0x43300000); a.offset(3, 3, -10); a.store(1, 0x4c, 0);
    a.exclusive_flags(0, 3, 0x80000000); a.floating_load(2, 26, 0x80); a.store(1, 0x50, 0); a.floating_load(0, 26, 0x40);
    a.floating_load(1, 1, 0x4c); a.floating_load(3, 26, 0x10); a.floating_subtract(1, 1, 2); a.floating_load(2, 26, 0x48);
    a.floating_difference(0, 1, 0, true); a.floating_sum(31, 3, 0); a.floating_move(1, 31); a.call(0x10780fc0);
    a.load(2, 1, 0x14); a.load<1>(0, 15, 0x27); a.floating_move(31, 1); a.sign8(0, 0, true);
}
void derive_personality_bound(ConstructionCall& a) {
    bool home = a.equal();
    if (home) {
        a.load<1>(0, 15, 0x27); a.load(3, 15, 0x1a4); a.sign8(0, 0); a.add(3, 3, 0);
        a.load<1>(0, 3, 0x1c6); a.sign8(0, 0, true); home = a.equal();
    }
    if (home) {
        a.load(5, 15, 0x1a4); a.load(4, 15, 0x1a4); a.load(3, 5, 0x3118); a.load(4, 4, 0x7c5c);
        a.load(0, 3, 0x18); a.compare(0, 0, false); bool capacity = !a.equal();
        if (capacity) { a.load(3, 5, 0x3118); a.load(3, 3, 0x18); a.load(0, 3, 4); a.compare(0, 0); capacity = !a.nonpositive(); }
        if (capacity) { a.load(3, 5, 0x3118); a.load(3, 3, 0x18); a.load(8, 3, 4); a.jump(0x10224cbc); }
        else { a.at(0x10224cb8); a.value(8, 1500); }
        a.at(0x10224cbc); a.multiply(3, 4, 2000); a.value(0, 0x43300000); a.load<1>(5, 16, 0x67); a.value(4, 0x43300000);
        a.load<1>(7, 16, 0x67); a.value(6, 0x43300000); a.exclusive_flags(3, 3, 0x80000000); a.store(1, 0x5c, 0);
        a.sign8(0, 5); a.sign8(7, 7); a.store(1, 0x60, 3); a.exclusive_flags(5, 0, 0x80000000); a.floating_load(1, 26, 0x80);
        a.exclusive_flags(7, 7, 0x80000000); a.floating_load(0, 1, 0x5c); a.exclusive_flags(3, 8, 0x80000000); a.value(0, 0x43300000);
        a.store(1, 0x58, 5); a.floating_subtract(0, 0, 1); a.floating_load(2, 26, 0x80); a.store(1, 0x54, 4); a.floating_load(4, 26, 0x80);
        a.floating_product(0, 31, 0); a.floating_load(1, 1, 0x54); a.store(1, 0x50, 7); a.floating_subtract(2, 1, 2);
        a.floating_load(1, 26, 0x80); a.store(1, 0x4c, 6); a.floating_product(0, 30, 0); a.floating_load(3, 1, 0x4c);
        a.store(1, 0x48, 3); a.floating_subtract(3, 3, 4); a.store(1, 0x44, 0); a.floating_product(2, 2, 0); a.floating_load(0, 1, 0x44);
        a.floating_subtract(0, 0, 1); a.floating_product(1, 3, 2); a.floating_difference(0, 1, 0, true); a.floating_integer(0, 0);
        a.floating_store(0, 1, 0x3c); a.load(14, 1, 0x40); a.jump(0x10224e64);
    } else {
        a.at(0x10224d64); a.load(5, 15, 0x1a4); a.load(4, 15, 0x1a4); a.load(3, 5, 0x3118); a.load(0, 4, 0x7c5c);
        a.load(3, 3, 0x18); a.compare(3, 0, false); bool capacity = !a.equal();
        if (capacity) { a.load(3, 5, 0x3118); a.load(3, 3, 0x18); a.load(3, 3, 4); a.compare(3, 0); capacity = !a.nonpositive(); }
        if (capacity) { a.load(3, 5, 0x3118); a.load(3, 3, 0x18); a.load(8, 3, 4); a.jump(0x10224da8); }
        else { a.at(0x10224da4); a.value(8, 1500); }
        a.at(0x10224da8); a.load(3, 15, 0x70); a.value(5, 0x43300000); a.load<1>(4, 16, 0x67); a.value(7, 0x43300000);
        a.load<1>(3, 3, 0x2c); a.value(9, 0x43300000); a.sign8(6, 4); a.load<1>(10, 16, 0x67); a.sign8(3, 3);
        a.exclusive_flags(4, 8, 0x80000000); a.offset(3, 3, -10); a.exclusive_flags(8, 6, 0x80000000); a.multiply(6, 3, 100);
        a.sign8(10, 10); a.value(3, 0x43300000); a.store(1, 0x4c, 5); a.exclusive_flags(10, 10, 0x80000000); a.floating_load(2, 26, 0x80);
        a.offset(5, 6, 2000); a.store(1, 0x48, 8); a.product(0, 5, 0); a.floating_load(1, 26, 0x80); a.store(1, 0x44, 7);
        a.floating_load(5, 26, 0x80); a.floating_load(0, 1, 0x44); a.store(1, 0x40, 10); a.exclusive_flags(0, 0, 0x80000000);
        a.store(1, 0x3c, 9); a.floating_subtract(3, 0, 1); a.floating_load(1, 26, 0x80); a.store(1, 0x50, 0);
        a.floating_load(4, 1, 0x3c); a.floating_load(0, 1, 0x4c); a.store(1, 0x58, 4); a.floating_subtract(4, 4, 5);
        a.floating_subtract(0, 0, 2); a.store(1, 0x54, 3); a.floating_product(2, 31, 0); a.floating_load(0, 1, 0x54);
        a.floating_subtract(0, 0, 1); a.floating_product(1, 30, 2); a.floating_product(1, 3, 1); a.floating_product(1, 4, 1);
        a.floating_difference(0, 1, 0, true); a.floating_integer(0, 0); a.floating_store(0, 1, 0x5c); a.load(14, 1, 0x60);
    }
    draw_uniform<14,13,0x3c,0x44,0x4c>(a, 0x10224e64);
}
void choose_base_personality(ConstructionCall& a) {
    a.load(3, 15, 0x1a4); a.call(0x101fce40); a.sign8(0, 3, true); bool skip = !a.equal();
    if (!skip) {
        a.compare(16, 0, false);
        if (!a.equal()) { a.load(3, 16, 0); a.load(0, 25, 0); a.compare_registers(3, 0); skip = !a.less(); }
    }
    if (skip) { a.at(0x102248a4); a.value(0, 0); a.floating_load(31, 26, 0x10); a.store<1>(15, 0x103, 0); a.jump(0x1022546c); return; }
    choose_affiliation_factor(a); derive_fixture_reputation(a); repair_missing_competition_descriptor(a);
    derive_personality_factor(a); derive_personality_bound(a);
    draw_probability_threshold(a, 0x10224fd0, 0x131, 0x50, 0x1f0000, -0x7b81, true);
    a.compare_registers(14, 13);
    if (!a.nonpositive()) {
        draw_small(a, 0x102250b0, 0x66660000, 0x6667, 2, 10); a.store<1>(15, 0x103, 0); a.jump(0x1022546c); return;
    }
    draw_probability_threshold(a, 0x10225110, 0x98, 0x58, 0xf0000, 0x423f);
    a.compare_registers(14, 13);
    if (!a.nonpositive()) {
        draw_small(a, 0x102251ec, 0x66660000, 0x6667, 1, 5); a.store<1>(15, 0x103, 0); a.jump(0x1022546c); return;
    }
    a.at(0x1022524c); a.move(3, 15); a.call(0x10289560); a.sign16(0, 3); a.compare(0, 1);
    if (a.equal()) { a.value(0, 0); a.store<1>(15, 0x103, 0); a.jump(0x1022546c); return; }
    draw_probability_threshold(a, 0x1022526c, 15, 0x60, 0x20000, -0x7961);
    a.compare_registers(14, 13);
    if (!a.at_least()) {
        draw_small(a, 0x10225348, 0x999a0000, -0x6667, 1, 0xfffffffb, true); a.store<1>(15, 0x103, 0); a.jump(0x1022546c); return;
    }
    draw_small(a, 0x102253ac, 0x68dc0000, -0x7453, 12, 10000); a.compare_registers(14, 0);
    if (!a.nonpositive()) { draw_small(a, 0x1022540c, 0x999a0000, -0x6667, 2, 0xfffffff6, true); a.store<1>(15, 0x103, 0); }
}
void apply_personality_attributes(ConstructionCall& a) {
    a.at(0x1022546c); a.floating_load(0, 26, 0x10); a.floating_compare(31, 0);
    if (!a.nonpositive()) {
        a.floating_load(0, 26, 0x70); a.floating_load(1, 26, 0x68); a.floating_difference(0, 31, 0, true); a.floating_sum(31, 1, 0);
    }
    a.at(0x10225488); a.load(3, 15, 0x70); a.value(0, 0x43300000); a.store(1, 0x3c, 0); a.load<1>(0, 3, 0x2c);
    a.floating_load(1, 26, 0x80); a.sign8(3, 0); a.offset(0, 3, -10); a.exclusive_flags(0, 0, 0x80000000);
    a.store(1, 0x40, 0); a.floating_load(0, 1, 0x3c); a.floating_subtract(0, 0, 1); a.floating_product(0, 0, 31);
    a.floating_integer(0, 0); a.floating_store(0, 1, 0x44); a.load(14, 1, 0x48);
    draw_uniform<14,13,0x3c,0x44,0x4c>(a, 0x102254c4);
    a.load<1>(5, 15, 0x103); a.sign8(3, 13); a.value(0, 0x43300000); a.value(4, 0x43300000); a.add(3, 5, 3);
    a.store(1, 0x44, 0); a.floating_load(2, 26, 0x80); a.store<1>(15, 0x103, 3); a.floating_load(0, 26, 0x78);
    a.load(3, 15, 0x70); a.load<1>(5, 15, 0x103); a.load<2>(0, 3, 4, true); a.sign8(3, 5); a.store(1, 0x3c, 4);
    a.exclusive_flags(0, 0, 0x80000000); a.floating_load(4, 26, 0x80); a.store(1, 0x48, 0); a.exclusive_flags(0, 3, 0x80000000);
    a.floating_load(3, 26, 0x10); a.floating_load(1, 1, 0x44); a.store(1, 0x40, 0); a.floating_subtract(1, 1, 2);
    a.floating_load(2, 1, 0x3c); a.floating_difference(0, 1, 0, true); a.floating_subtract(1, 2, 4); a.floating_subtract(0, 3, 0);
    a.floating_product(0, 1, 0); a.floating_integer(0, 0); a.floating_store(0, 1, 0x4c); a.load(0, 1, 0x50); a.store<1>(15, 0x103, 0);
    a.load(3, 15, 0x1a4); a.load(3, 3, 0x3118); a.load<1>(0, 3, 0x40); a.sign8(0, 0, true);
    if (!a.equal()) {
        a.load<1>(0, 15, 0x27); a.load(3, 15, 0x1a4); a.sign8(0, 0); a.leading_zeroes(0, 0); a.mask(0, 0, 0x07ffffff, false, 27);
        a.sign8(0, 0, true); select_fixture_club(a, 0x102256d8, 3); a.load(4, 15, 0x6c); a.load(3, 3, 0x90); a.call(0x100d9580); a.sign8(0, 3, true);
        if (!a.equal()) { a.load<1>(3, 15, 0x103); a.offset(0, 3, 10); a.store<1>(15, 0x103, 0); }
    }
    a.at(0x10225710);
}
bool apply_coach_influence(ConstructionCall& a) {
    a.load<1>(0, 15, 0x27); a.load(3, 15, 0x1a4); a.sign8(0, 0, true); select_fixture_club(a, 0x1022571c, 3);
    a.value(4, 0); a.value(5, 1); a.value(6, 0); a.value(7, 0); a.call(0x1014c2e0); a.sign8(0, 3, true); if (!a.equal()) return false;
    a.load<1>(0, 15, 0x27); a.load(4, 15, 0x1a4); a.sign8(0, 0); a.multiply(3, 0, 0xd8); a.offset(3, 3, 0x7aac); a.add(3, 4, 3);
    a.load(0, 3, 0x20); a.compare(0, 0, false); if (a.equal()) return false;
    a.load<1>(0, 15, 0x27); a.load(4, 15, 0x1a4); a.sign8(0, 0); a.multiply(3, 0, 0xd8); a.offset(3, 3, 0x7aac); a.add(3, 4, 3);
    a.load(3, 3, 0x20); a.load(0, 3, 0x1c); a.compare(0, 0, false); if (a.equal()) return false;
    a.load<1>(0, 15, 0x27); a.load<1>(4, 15, 0x27); a.sign8(0, 0); a.load(6, 15, 0x1a4); a.multiply(3, 0, 0xd8);
    a.load(5, 15, 0x1a4); a.sign8(0, 4); a.offset(7, 3, 0x7aac); a.multiply(4, 0, 0xd8); a.add(7, 6, 7);
    a.offset(3, 4, 0x7aac); a.load(4, 7, 0x20); a.add(3, 5, 3); a.load(3, 3, 0x20); a.load(4, 4, 0x1c);
    a.load(3, 3, 0x1c); a.load<1>(4, 4, 0x3b); a.call(0x100d2f50); a.sign8(3, 3); a.offset_record(14, 3, -10);
    draw_uniform<14,13,0x3c,0x44,0x4c>(a, 0x102257e8, true);
    a.load<1>(3, 15, 0x103); a.sign8(0, 13); a.add(0, 3, 0); a.store<1>(15, 0x103, 0); a.jump(0x10225a34); return true;
}
void apply_fallback_influence(ConstructionCall& a) {
    a.at(0x10225968); a.sign8(0, 19, true);
    if (!a.equal()) {
        advance_pool(a, 0x10225970); a.load(3, 30, 0); a.load(0, 27, 0); a.load<2>(4, 3, 0); a.load<1>(3, 15, 0x103); a.add(4, 4, 0);
        a.mask(0, 4, 0xe0000000, false, 29); a.mask(4, 4, 1, false, 1); a.subtract(0, 4, 0); a.mask(0, 0, 0xffffffff, false, 3);
        a.add(0, 0, 4); a.sign8(0, 0); a.add(0, 3, 0); a.store<1>(15, 0x103, 0); a.jump(0x10225a34);
    } else {
        advance_pool(a, 0x102259d0); a.load(3, 30, 0); a.value(4, 0x2aab0000); a.load(0, 27, 0); a.offset(4, 4, -0x5555);
        a.load<2>(5, 3, 0); a.load<1>(3, 15, 0x103); a.add(5, 5, 0); a.multiply_high(4, 4, 5); a.mask(0, 4, 1, false, 1);
        a.add(0, 4, 0); a.multiply(0, 0, 6); a.subtract(0, 0, 5); a.sign8(0, 0); a.add(0, 3, 0); a.store<1>(15, 0x103, 0);
    }
    a.at(0x10225a34);
}
bool own_competition_has_history(ConstructionCall& a) {
    a.load(0, 15, 0xfc); a.compare(0, 0, false); if (a.equal()) return false;
    a.load(3, 15, 0xfc); a.load(0, 3, 0x30); a.compare(0, 0, false); if (a.equal()) return false;
    a.compare(20, 0, false); if (a.equal()) return false;
    a.load(4, 15, 0x1a4); a.load(3, 15, 0xfc); a.load(4, 4, 0x3118); a.load(0, 3, 0x30); a.load(3, 4, 0x14); a.compare_registers(3, 0, false);
    if (!a.equal()) return false;
    a.load<1>(0, 20, 0x44); a.sign8(0, 0); a.compare(0, 25); if (a.nonpositive()) return false;
    a.load(3, 15, 0xfc); a.load(4, 24, 0); a.load(3, 3, 0x30); a.load(0, 3, 0); a.mask(0, 0, 0xfffffffc, false, 2);
    a.indexed_load(0, 4, 0); a.compare(0, 0, false); if (a.equal()) return false;
    a.load(3, 15, 0xfc); a.load(4, 24, 0); a.load(3, 3, 0x30); a.load(0, 3, 0); a.mask(0, 0, 0xfffffffc, false, 2);
    a.indexed_load(3, 4, 0); a.load(0, 3, 8); a.compare(0, 0, false); return !a.equal();
}
void decrement_personality(ConstructionCall& a, std::uint32_t start, unsigned destination) {
    a.at(start); a.load<1>(destination, 15, 0x103); a.offset(0, destination, -1); a.store<1>(15, 0x103, 0);
}
bool history_exceeds_limits(ConstructionCall& a, std::uint32_t start, int standing, int points) {
    a.at(start); a.load(0, 3, 0x1c); a.compare(0, standing);
    if (a.positive()) return true;
    a.load(0, 3, 0x44); a.compare(0, points); return !a.nonpositive();
}
void apply_standings_pressure(ConstructionCall& a) {
    a.load(3, 15, 0xfc); a.load(5, 24, 0); a.load(3, 3, 0x30); a.load(4, 15, 0xfc); a.load(0, 3, 0);
    a.mask(0, 0, 0xfffffffc, false, 2); a.indexed_load(3, 5, 0); a.load(3, 3, 8); a.call(0x10459b50); a.compare(3, 0, false);
    if (a.equal()) return;
    if (history_exceeds_limits(a, 0x10225af4, 5, 9)) {
        a.at(0x10225b0c); a.load(4, 3, 0x1c); a.load<1>(5, 16, 0x67); a.offset(0, 4, 10); a.sign8(4, 5); a.sign8(0, 0); a.compare_registers(4, 0);
        if (!a.at_least()) { decrement_personality(a, 0x10225b28, 4); a.jump(0x10225b78); }
        else {
            if (history_exceeds_limits(a, 0x10225b38, 3, 7)) {
                a.at(0x10225b50); a.load(4, 3, 0x1c); a.load<1>(5, 16, 0x67); a.offset(0, 4, 6); a.sign8(4, 5); a.sign8(0, 0); a.compare_registers(4, 0);
                if (!a.at_least()) decrement_personality(a, 0x10225b6c, 4);
            }
        }
    } else {
        if (history_exceeds_limits(a, 0x10225b38, 3, 7)) {
            a.at(0x10225b50); a.load(4, 3, 0x1c); a.load<1>(5, 16, 0x67); a.offset(0, 4, 6); a.sign8(4, 5); a.sign8(0, 0); a.compare_registers(4, 0);
            if (!a.at_least()) decrement_personality(a, 0x10225b6c, 4);
        }
    }
    if (history_exceeds_limits(a, 0x10225b78, 3, 5)) {
        a.at(0x10225b90); a.load<1>(14, 16, 0x67); a.sign8(14, 14, true);
        draw_uniform<14,13,0x3c,0x44,0x4c>(a, 0x10225b94, true); a.compare(13, 0);
        bool reduce = a.equal();
        if (!reduce) {
            a.load<1>(14, 16, 0x66); a.sign8(14, 14, true); draw_uniform<14,13,0x3c,0x44,0x4c>(a, 0x10225d0c, true);
            a.compare(13, 0); reduce = a.equal();
        }
        if (reduce) decrement_personality(a, 0x10225e80, 3);
    }
}
void apply_club_history_pressure(ConstructionCall& a) {
    a.at(0x10225e8c); a.load(3, 15, 0xfc); a.load(0, 3, 0x90); a.compare(0, 0, false); if (a.equal()) return;
    a.load(3, 2, -0x7b28); a.load(4, 15, 0xfc); a.load(3, 3, 0); a.load(4, 4, 0x90); a.call(0x102de3c0); a.sign8(0, 3, true); if (a.equal()) return;
    a.load<1>(14, 16, 0x67); a.sign8(14, 14, true); draw_uniform<14,13,0x3c,0x44,0x4c>(a, 0x10225ebc, true);
    a.compare(13, 0); bool reduce = a.equal();
    if (!reduce) {
        a.load<1>(14, 16, 0x65); a.sign8(14, 14, true); draw_uniform<14,13,0x3c,0x44,0x4c>(a, 0x10226034, true);
        a.compare(13, 0); reduce = a.equal();
    }
    if (reduce) decrement_personality(a, 0x102261a8, 3);
}
void apply_goalkeeper_club_context(ConstructionCall& a) {
    a.at(0x102261b4); a.load(3, 15, 0xfc); a.value(4, 0); a.value(5, 1); a.value(6, 0); a.value(7, 0); a.call(0x1014c2e0); a.sign8(0, 3, true);
    if (!a.equal()) {
        a.move(3, 15); a.call(0x10289560); a.sign16(0, 3); a.compare(0, 1);
        if (a.equal()) { a.load<1>(3, 15, 0x103); a.offset(0, 3, 1); a.store<1>(15, 0x103, 0); }
    }
    a.at(0x102261f4);
}
void apply_staff_context(ConstructionCall& a) {
    a.load(0, 15, 0xfc); a.compare(0, 0, false); if (a.equal()) return;
    a.compare(20, 0, false); if (a.equal()) return;
    a.load(0, 24, 0); a.compare(0, 0, false); if (a.equal()) return;
    a.load(3, 15, 0xfc); a.load(0, 3, 0x30); a.compare(0, 0, false); if (a.equal()) return;
    a.load(3, 15, 0xfc); a.load(4, 24, 0); a.load(3, 3, 0x30); a.load(0, 3, 0); a.mask(0, 0, 0xfffffffc, false, 2);
    a.indexed_load(0, 4, 0); a.compare(0, 0, false); if (a.equal()) return;
    a.load(3, 15, 0xfc); a.value(4, 0); a.value(5, 1); a.value(6, 0); a.value(7, 0); a.call(0x1014c2e0); a.sign8(0, 3, true);
    if (a.equal()) { a.load<1>(0, 15, 0x103); a.sign8(0, 0); a.compare(0, 5); if (a.at_least()) return; }
    a.at(0x10226274); a.load(0, 20, 0x50); a.compare(0, 0); if (!a.equal()) return;
    a.load<1>(0, 20, 0x4c); a.sign8(0, 0); a.compare(0, 25); if (a.nonpositive()) return;
    a.move(3, 16); a.value(4, 0); a.call(0x100d3150); a.sign8(0, 3, true); if (!a.equal()) return;
    a.load(3, 15, 0xfc); a.load(4, 2, -0x7984); a.load(0, 3, 0); a.load(3, 4, 0); a.mask(0, 0, 0xfffffff8, false, 3);
    a.add(3, 3, 0); a.load<1>(0, 3, 5); a.sign8(0, 0, true); if (!a.equal()) return;
    a.load<1>(0, 16, 0x67); a.sign8(0, 0); a.compare(0, 14); if (a.nonpositive()) return;
    a.load<1>(13, 16, 0x65); a.sign8(13, 13, true); draw_uniform<13,14,0x3c,0x44,0x4c>(a, 0x102262dc, true);
    a.compare(14, 0); if (a.equal()) return;
    a.load<1>(3, 15, 0x103); a.offset(0, 3, 1); a.store<1>(15, 0x103, 0);
}
void finish_actor_defaults(ConstructionCall& a) {
    a.at(0x1022645c); a.load<1>(0, 15, 0x103); a.load(4, 15, 0x70); a.sign8(3, 0); a.mask(0, 3, 1, false, 1);
    a.load<1>(4, 4, 0x25); a.add(0, 0, 3); a.sign8(3, 4); a.shift_right(0, 0, 1); a.add(3, 3, 0); a.offset(0, 3, 10);
    a.shift_right(0, 0, 2); a.add_carry(0, 0); a.store<1>(15, 0x102, 0); a.load<1>(0, 15, 0x27); a.load(3, 15, 0x1a4); a.sign8(0, 0, true);
    if (a.equal()) { a.load<1>(0, 3, 0x77c5); a.jump(0x102264ac); }
    else { a.at(0x102264a8); a.load<1>(0, 3, 0x77c6); }
    a.at(0x102264ac); a.load(3, 15, 0xfc); a.mask(4, 0, 255); a.offset(5, 1, 0x3a); a.offset(6, 1, 0x38); a.call(0x100c7a50);
    a.load<2>(3, 1, 0x38); a.sign8(0, 17, true); a.store<2>(15, 0x1c0, 3);
    if (!a.equal()) { a.value(0, 180); a.store<2>(15, 0x19e, 0); a.jump(0x102264e4); }
    else { a.at(0x102264dc); a.value(0, 0); a.store<2>(15, 0x19e, 0); }
    a.at(0x102264e4); a.value(0, 0); a.value(4, 0); a.store<1>(15, 0x2c, 0); a.value(3, 0); a.value(0, 101);
    a.store<1>(15, 0x105, 4); a.store<1>(15, 0x106, 3); a.store<2>(15, 0x19c, 0);
    a.load(0, 1, 0xd8); a.offset(1, 1, 0xd0); a.floating_load(31, 1, -8); a.floating_load(30, 1, -16);
    a.restore_link(); a.multiple(13, true, -16); a.finish();
}
}

void initialize_actor(State& state, DependencyDispatch* dependencies) {
    ConstructionCall a(state, entries[1], dependencies);
    begin_actor(a); choose_player_record(a); copy_attributes_and_clubs(a);
    initialize_fitness_and_position(a); initialize_affiliation(a); initialize_eligibility(a);
    copy_preferences_and_names(a); inspect_special_club_context(a);
    initialize_random_preferences(a); choose_base_personality(a); apply_personality_attributes(a);
    if (!apply_coach_influence(a)) apply_fallback_influence(a);
    if (own_competition_has_history(a)) { apply_standings_pressure(a); apply_club_history_pressure(a); }
    apply_goalkeeper_club_context(a); apply_staff_context(a); finish_actor_defaults(a);
}
}
