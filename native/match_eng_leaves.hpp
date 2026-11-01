#pragma once
// MATCH_ENG.CPP routines short enough that generated callers run them in line
// (tools/compile_recomp.py CPP_LEAVES): a direct call site calls
// match_eng_leaf_<entry> instead of the original instructions, and run_until
// does too for calls from C++ owners. Each takes its arguments from the
// registers and sets its results; each returns false, doing nothing, while the
// module declines, and the call site then runs the original instructions.
#include "match_eng_fields.hpp"

namespace cm::aot {
extern bool match_eng_declining;
inline bool match_eng_leaf_101d3ac0(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::signed_byte_46(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101d3b40(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::word_50(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101d3b50(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::position(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101d3c40(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::set_byte_0(c,c.r(3),c.r(4));
    return true;
}
inline bool match_eng_leaf_101d3c80(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::absolute(c,c.f(1));
    return true;
}
inline bool match_eng_leaf_101d5660(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::trait_10a(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101d5670(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::secondary_rating(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101d5680(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::column(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101d5690(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::row(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101d56a0(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::action_code(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101d56b0(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::primary_rating(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101d56c0(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::marking_target(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101d56d0(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::ball_row(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101d56e0(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::ball_column(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101d56f0(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::tactical_flags(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101d5730(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::set_marking_target(c,c.r(3),c.r(4));
    return true;
}
inline bool match_eng_leaf_101d5740(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::clear_marking_list(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101d5750(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::set_byte_18(c,c.r(3),c.r(4));
    return true;
}
inline bool match_eng_leaf_101d57c0(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::byte_7931(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101d5870(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::word_79f4(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101d6690(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::set_word_e000(c,c.r(3),c.r(4));
    return true;
}
inline bool match_eng_leaf_101d66a0(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::set_possession_side(c,c.r(3),c.r(4));
    return true;
}
inline bool match_eng_leaf_101d66b0(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::set_byte_7928(c,c.r(3),c.r(4));
    return true;
}
inline bool match_eng_leaf_101d66c0(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::set_ball_column(c,c.r(3),c.r(4));
    return true;
}
inline bool match_eng_leaf_101d66d0(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::set_ball_row(c,c.r(3),c.r(4));
    return true;
}
inline bool match_eng_leaf_101d66e0(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::increment_byte(c,c.r(3)+0x13);
    return true;
}
inline bool match_eng_leaf_101d6770(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::byte_2f(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101d7d90(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::trait_10c(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101d7da0(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::trait_10d(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101d7db0(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::marking_count(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101d8100(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::pressure_adjustment(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101d8110(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::position(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101d8d60(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::adjustment_byte(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101d93a0(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::fitness(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101da750(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::word_20(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101db320(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::set_action_19e(c,c.r(3),c.r(4));
    return true;
}
inline bool match_eng_leaf_101db330(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::byte_0(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101e3f00(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::previous_position(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101e5070(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::word_6c(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101e50b0(State& s) {
    static_cast<void>(s);
    if (match_eng_declining) return false;
    return true;
}
inline bool match_eng_leaf_101e88f0(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::word_24(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101e8900(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::minute_clock(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101e8910(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::status_110(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101e8920(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::match_clock(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101e8930(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::clear_word_310c(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101e8940(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::clear_word_3108(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101eaf50(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::trait_10e(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101ec7a0(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::word_1ac(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101ee510(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::trait_10f(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101ee520(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::set_action_code(c,c.r(3),c.r(4));
    return true;
}
inline bool match_eng_leaf_101f0bf0(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::increment_byte(c,c.r(3)+0x12);
    return true;
}
inline bool match_eng_leaf_101f1520(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::set_default_turn(c,c.r(3),c.r(4));
    return true;
}
inline bool match_eng_leaf_101f1860(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::position(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101f1870(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::set_position(c,c.r(3),c.r(4));
    return true;
}
inline bool match_eng_leaf_101f1930(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::set_column(c,c.r(3),c.r(4));
    return true;
}
inline bool match_eng_leaf_101f1940(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::set_row(c,c.r(3),c.r(4));
    return true;
}
inline bool match_eng_leaf_101f1dc0(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::marking_member(c,c.r(3),c.r(4));
    return true;
}
inline bool match_eng_leaf_101f21c0(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::set_previous_position(c,c.r(3),c.r(4));
    return true;
}
inline bool match_eng_leaf_101f93d0(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::side_stats(c,c.r(3),c.r(4));
    return true;
}
inline bool match_eng_leaf_101f93f0(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::fitness_1a8(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101f9400(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::byte_44(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101f9410(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::fixture(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101f9420(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::players(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101f9430(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::halfword_0(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101f9460(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::reset_list_cursor(c,c.r(3));
    return true;
}
inline bool match_eng_leaf_101fb570(State& s) {
    if (match_eng_declining) return false;
    guest::Cpu c(s);
    match_eng::low_nibble_60(c,c.r(3));
    return true;
}
}
