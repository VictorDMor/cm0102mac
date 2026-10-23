#pragma once
// Tactics routines short enough that generated callers run them in line
// (tools/compile_recomp.py CPP_LEAVES): at a direct call site the page
// calls tactics_leaf_<entry> instead of the original instructions; calls
// through the dispatcher reach the same function through the routine's hook.
// Each takes its arguments from the registers and sets its results. They
// write no stack and leave r4-r10 as the original does. Each
// returns false, doing nothing, while the tactics routines decline, and the
// call site then runs the original instructions.
#include "guest_abi.hpp"

namespace cm::aot {
// True while every tactics routine runs its original body: in a checker's
// reference child, and for the whole run with CM_TACTICS_ORIGINAL=1.
extern bool tactics_declining;
// 0x10113e10: a player's instruction word (r3 high, r4 low): formation +0x1888 + 8 * player.
inline bool tactics_leaf_10113e10(State& s) {
    if (tactics_declining) return false;
    guest::Cpu c(s);
    const auto at = c.r(3) + std::uint32_t(std::int32_t(std::int8_t(c.r(4)))) * 8;
    c.set(3, c.u32(at + 0x1888)); c.set(4, c.u32(at + 0x188c));
    return true;
}
// 0x101149f0: a player slot's role byte: formation +0x5b8 + slot.
inline bool tactics_leaf_101149f0(State& s) {
    if (tactics_declining) return false;
    guest::Cpu c(s);
    c.set(3, c.u8(c.r(3) + std::uint32_t(std::int32_t(std::int8_t(c.r(4)))) + 0x5b8));
    return true;
}
// 0x103b3c40: screen byte +0xec0.
inline bool tactics_leaf_103b3c40(State& s) {
    if (tactics_declining) return false;
    guest::Cpu c(s);
    c.set(3, c.u8(c.r(3) + 0xec0));
    return true;
}
// 0x103baae0: copies two halfwords from r4 to r3.
inline bool tactics_leaf_103baae0(State& s) {
    if (tactics_declining) return false;
    guest::Cpu c(s);
    c.put16(c.r(3), c.u16(c.r(4)));
    c.put16(c.r(3) + 2, c.u16(c.r(4) + 2));
    return true;
}
// 0x103bacd0: byte +0x10.
inline bool tactics_leaf_103bacd0(State& s) {
    if (tactics_declining) return false;
    guest::Cpu c(s);
    c.set(3, c.u8(c.r(3) + 0x10));
    return true;
}
// 0x103bace0: the screen's formation record (+0xec4).
inline bool tactics_leaf_103bace0(State& s) {
    if (tactics_declining) return false;
    guest::Cpu c(s);
    c.set(3, c.r(3) + 0xec4);
    return true;
}
// 0x103bacf0: screen byte +0x27a4.
inline bool tactics_leaf_103bacf0(State& s) {
    if (tactics_declining) return false;
    guest::Cpu c(s);
    c.set(3, c.u8(c.r(3) + 0x27a4));
    return true;
}
// 0x103bad00: screen byte +0x27ae.
inline bool tactics_leaf_103bad00(State& s) {
    if (tactics_declining) return false;
    guest::Cpu c(s);
    c.set(3, c.u8(c.r(3) + 0x27ae));
    return true;
}
// 0x103badd0: screen byte +0xc6b1.
inline bool tactics_leaf_103badd0(State& s) {
    if (tactics_declining) return false;
    guest::Cpu c(s);
    c.set(4, c.r(3) + 0x10000);  // addis r4,r3,1
    c.set(3, c.u8(c.r(4) - 0x394f));
    return true;
}
// 0x103bae40: formation byte +0x1886.
inline bool tactics_leaf_103bae40(State& s) {
    if (tactics_declining) return false;
    guest::Cpu c(s);
    c.set(3, c.u8(c.r(3) + 0x1886));
    return true;
}
// 0x103bae80: formation saved flag (+0x1884).
inline bool tactics_leaf_103bae80(State& s) {
    if (tactics_declining) return false;
    guest::Cpu c(s);
    c.set(3, c.u8(c.r(3) + 0x1884));
    return true;
}
// 0x103be410: byte +0x4b53.
inline bool tactics_leaf_103be410(State& s) {
    if (tactics_declining) return false;
    guest::Cpu c(s);
    c.set(3, c.u8(c.r(3) + 0x4b53));
    return true;
}
// 0x103be550: word +0x1c.
inline bool tactics_leaf_103be550(State& s) {
    if (tactics_declining) return false;
    guest::Cpu c(s);
    c.set(3, c.u32(c.r(3) + 0x1c));
    return true;
}
// 0x103be620: word +0x18.
inline bool tactics_leaf_103be620(State& s) {
    if (tactics_declining) return false;
    guest::Cpu c(s);
    c.set(3, c.u32(c.r(3) + 0x18));
    return true;
}
// 0x103be6f0: word +0x14.
inline bool tactics_leaf_103be6f0(State& s) {
    if (tactics_declining) return false;
    guest::Cpu c(s);
    c.set(3, c.u32(c.r(3) + 0x14));
    return true;
}
// 0x103be7c0: word +0x10.
inline bool tactics_leaf_103be7c0(State& s) {
    if (tactics_declining) return false;
    guest::Cpu c(s);
    c.set(3, c.u32(c.r(3) + 0x10));
    return true;
}
// 0x103be980: signed halfword +0x76.
inline bool tactics_leaf_103be980(State& s) {
    if (tactics_declining) return false;
    guest::Cpu c(s);
    c.set(3, std::uint32_t(std::int32_t(c.i16(c.r(3) + 0x76))));
    return true;
}
// 0x103be990: sets screen byte +0x27ae.
inline bool tactics_leaf_103be990(State& s) {
    if (tactics_declining) return false;
    guest::Cpu c(s);
    c.put8(c.r(3) + 0x27ae, c.r(4));
    return true;
}
// 0x103be9a0: halfword +0x2.
inline bool tactics_leaf_103be9a0(State& s) {
    if (tactics_declining) return false;
    guest::Cpu c(s);
    c.set(3, c.u16(c.r(3) + 2));
    return true;
}
// 0x103be9b0: signed halfword +0x0.
inline bool tactics_leaf_103be9b0(State& s) {
    if (tactics_declining) return false;
    guest::Cpu c(s);
    c.set(3, std::uint32_t(std::int32_t(c.i16(c.r(3)))));
    return true;
}
// 0x103be9f0: formation byte +0x1885.
inline bool tactics_leaf_103be9f0(State& s) {
    if (tactics_declining) return false;
    guest::Cpu c(s);
    c.set(3, c.u8(c.r(3) + 0x1885));
    return true;
}
// 0x103c1f20: clears screen byte +0xc6b1.
inline bool tactics_leaf_103c1f20(State& s) {
    if (tactics_declining) return false;
    guest::Cpu c(s);
    c.set(4, c.r(3) + 0x10000);  // addis r4,r3,1
    c.put8(c.r(4) - 0x394f, 0);
    return true;
}
// 0x103c5940: formation record `index` of the screen's array at +0x7c6c (0x18e0 bytes each).
inline bool tactics_leaf_103c5940(State& s) {
    if (tactics_declining) return false;
    guest::Cpu c(s);
    c.set(5, std::uint32_t(std::int32_t(std::int8_t(c.r(4)))) * 0x18e0);  // mulli r5
    c.set(3, c.r(3) + c.r(5) + 0x7c6c);
    return true;
}
// 0x103c9960: screen byte +0x7929.
inline bool tactics_leaf_103c9960(State& s) {
    if (tactics_declining) return false;
    guest::Cpu c(s);
    c.set(3, c.u8(c.r(3) + 0x7929));
    return true;
}
// 0x103cba60: decrements byte +0x19.
inline bool tactics_leaf_103cba60(State& s) {
    if (tactics_declining) return false;
    guest::Cpu c(s);
    c.set(4, c.u8(c.r(3) + 0x19));  // lbz r4
    c.put8(c.r(3) + 0x19, c.r(4) - 1);
    return true;
}
}
