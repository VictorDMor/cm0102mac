#pragma once
#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl::detail {
// One activation covers the contest, flight and publication phases. Stack
// out-parameters and kept values remain live across commentary callbacks.
class BallAction {
public:
    BallAction(Cpu&, std::uint32_t player, std::uint32_t from_column,
               std::uint32_t from_row, std::uint32_t kind, std::uint32_t control,
               std::uint32_t receiver, std::uint32_t to_column, std::uint32_t to_row);
    std::uint32_t run();

private:
    Cpu& c;
    guest::Frame frame;
    guest::Fpu fp;
    Player actor;
    const std::uint32_t doubles, seed, begin, end, cursor, singles;
    std::uint32_t at(unsigned offset) const { return frame.at(offset); }
    std::uint32_t player() const { return actor.address; }
    std::uint32_t match() const { return actor.match(); }
    std::uint32_t receiver() const { return c.r(25); }
    std::uint32_t opponent() const { return c.r(13); }
    std::int8_t kind() const { return std::int8_t(c.r(24)); }
    std::int8_t from_column() const { return c.i8(at(0x16f)); }
    std::int8_t from_row() const { return c.i8(at(0x173)); }
    std::int8_t to_column() const { return std::int8_t(c.r(26)); }
    std::int8_t to_row() const { return c.i8(at(0x187)); }
    std::int8_t byte(unsigned offset) const { return c.i8(at(offset)); }
    double metric() const { return c.f(31); }
    std::int16_t score() const { return std::int16_t(c.r(31)); }
    void set_score(std::int32_t value) { c.set(31, std::uint32_t(std::int32_t(std::int16_t(value)))); }
    static std::int32_t high(std::uint32_t value, std::int32_t multiplier);
    std::uint32_t shifted_sign(std::int32_t value, unsigned bits);
    std::uint32_t sample() const { return std::uint32_t(c.u16(c.u32(cursor))) + c.u32(seed); }
    void advance(std::uint32_t back);
    std::int32_t modulo(unsigned divisor);
    std::int32_t binary(unsigned divisor = 2);
    void range(std::int32_t bound, unsigned destination, std::uint32_t back,
               std::uint32_t correction, bool narrow_only = false, bool encoded_bound = false);
    void condition_delta(std::uint32_t address, int amount);
    void player_message(unsigned id, std::uint32_t back, bool duration = false);
    void receiver_message(unsigned id, std::uint32_t back, bool duration = false);
    void target_message(unsigned id, std::uint32_t address, std::uint32_t back);
    void style_message(bool lost);
    void paired_message(unsigned id, std::uint32_t back, bool reverse = false);
    void side_message(unsigned id, std::uint32_t side, std::uint32_t back);
    bool opposite_area(std::uint32_t address);
    bool defensive_area(std::uint32_t address);
    bool protected_action(std::uint32_t back);

    void initial_condition_draw();
    void initial_distance_draw();
    void calculate_quality();
    void apply_weather();
    void query_opponent();
    bool contest();
    void foul();
    void disciplinary_queue();
    void lose();
    void delivery();
    void accuracy();
    void exertion();
    void contextual_commentary();
    void direction_flags();
    void action_commentary();
    void flight_phase();
    std::uint32_t publish();
};
}
