#include "inter_amer_cup_retirement.hpp"
#include "guest_abi.hpp"

namespace cm::aot {
namespace {
// This is an initialization-call context, never persistent game state. A load
// must restore the old object and its statistics through the original reader.
thread_local State* creating_career = nullptr;
struct CreationScope {
    State* previous = creating_career;
    explicit CreationScope(State& s) { creating_career = &s; }
    ~CreationScope() { creating_career = previous; }
};
}
bool inter_amer_cup_retired(State& s) {
    guest::Cpu c(s);
    return c.i16(c.toc(-0x7b80) + 2) > 1998;
}
std::uint32_t inter_amer_cup_creation_id(State& s, std::uint32_t configured_id) {
    // The ruling-body factory already supports an absent competition (-1).
    // Both its count and creation checks use this value; the database ID stays
    // intact for historical lookups and binary save compatibility.
    return creating_career == &s && inter_amer_cup_retired(s) ? 0xffffffffu : configured_id;
}
void inter_amer_cup_initialize_new_career(State& s) {
    CreationScope scope(s);
    guest::Cpu(s).call(0x10191cd0, 0x1034a8e8);
}
bool inter_amer_cup_retired_object(State& s, std::uint32_t object) {
    guest::Cpu c(s);
    return object && c.u32(object) == c.toc(-0x5cc0) && inter_amer_cup_retired(s);
}
bool inter_amer_cup_skip_daily(State& s) {
    guest::Cpu c(s);
    if (!inter_amer_cup_retired_object(s, c.r(3))) return false;
    c.set(3, 0); // The original CUP daily routine's ordinary return value.
    c.finish();
    return true;
}
bool inter_amer_cup_cancel_pending(State& s, std::uint32_t fixture) {
    guest::Cpu c(s);
    const auto id = c.u32(c.toc(-0x6af8));
    // Serialized FIX_MAN rows are 0x50 bytes. A score of -1 is the original
    // pending-game marker; played results and pre-retirement editions survive.
    return id != 0xffffffffu && c.u32(fixture) == id &&
           c.i16(fixture + 0x28) > 1998 && c.i8(fixture + 0x43) == -1;
}
bool inter_amer_cup_skip_host_announcement(State& s, std::uint32_t row) {
    guest::Cpu c(s);
    const auto id = c.u32(c.toc(-0x6af8));
    return id != 0xffffffffu && c.u32(row) == id && c.u16(row + 4) > 1998;
}
}
