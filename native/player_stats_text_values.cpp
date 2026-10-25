// The stats screens' values as text: one season record's (0x10304190) and a
// player's totals' (0x10304b10). Both format with the module's sprintf
// texts and have the same shape: the integer columns print "%d", with
// " / appearances" or the bracketed figure on request; the ratios and
// percentages go through the compiler's int-to-double idiom, whose stack
// words (and sprintf's double argument at sp+0x20) are part of the stack
// image, so each case writes the slots its branch of the original writes.
#include "player_stats_detail.hpp"

namespace cm::aot::player_stats {
using namespace detail;
namespace {
// The stats columns (season_value's kinds 1-17; column_title's titles).
enum Column : std::int32_t {
    appearances = 1, goals, conceded, penalties, assists, team_goals, team_conceded, games_won, games_lost,
    yellow_cards, red_cards, man_of_match, pass_completion, tackles, dribbles, shots_on_target, average_rating
};

// The stack slots one percentage uses: its two int-to-double words and
// fctiwz's result (sp offsets).
struct PercentSlots {
    std::int32_t part, whole, result;
};
// The two int-to-double slots of a per-appearance ratio.
struct RatioSlots {
    std::int32_t value, appearances;
};

// One value being formatted into `buffer`: what both routines share.
class ValueText {
public:
    ValueText(Cpu& c, const Frame& frame, std::uint32_t buffer, std::uint32_t pool, std::uint32_t detail,
              std::uint32_t per_appearance, std::uint32_t zeros)
        : c(c), frame(frame), buffer(buffer), pool(pool), constants(c.toc(slot::constants)),
          constants2(c.toc(slot::constants2)), detail_(detail), per_appearance_(per_appearance), zeros_(zeros) {}

    // The char flags, tested as the original tests them (extsb., into cr0).
    bool detailed() { return nonzero(c, std::int8_t(detail_)); }
    bool per_appearance() { return nonzero(c, std::int8_t(per_appearance_)); }
    bool zeros() { return nonzero(c, std::int8_t(zeros_)); }

    // sprintf(buffer, format, r5...): the values are already in r5-r7, where
    // the original loads them (varargs: sprintf stores r3-r10 in the frame).
    void print(std::uint32_t back, std::uint32_t format) {
        c.set(3, buffer);
        c.set(4, pool + format);
        c.call(routine::sprintf, back);
    }
    void copy(std::uint32_t back, std::uint32_t text) { Program{c}.strcpy(buffer, pool + text, back); }

    // Appearances (case label `label`): "starts (substitute)" with the
    // detail flag, else the appearances; nothing for none unless `zeros`.
    void appearance_count(std::uint32_t label, std::uint32_t total, std::uint32_t substitute) {
        c.set(6, substitute);
        if (nonzero_logical(c, substitute) && detailed()) {
            c.set(5, total - substitute);
            print(label + 0x28, text::count_of);
            return;
        }
        c.set(5, total);
        if (nonzero_logical(c, total) || zeros()) print(label + 0x4c, text::count);
    }
    // A tally (goals, conceded, assists ... man of the match): "%d", or
    // "%d / appearances"; nothing for a player without appearances unless `zeros`.
    void tally(std::uint32_t label, std::uint32_t games, std::uint32_t value) {
        c.set(6, games);
        if (!nonzero_logical(c, games) && !zeros()) return;
        c.set(5, value);
        if (per_appearance()) print(label + 0x2c, text::count_per);
        else print(label + 0x40, text::count);
    }
    // Penalties: "%d (%d)" with the detail flag (and " / appearances"),
    // else as a tally. Quirk: without the detail flag (or with no
    // penalties) the season text prints the first figure, the totals text
    // (`totals`) the bracketed one.
    void penalty_count(std::uint32_t label, std::uint32_t games, std::uint32_t first, std::uint32_t bracketed, bool totals) {
        c.set(5, first);
        if (nonzero_logical(c, first) && detailed()) {
            c.set(6, bracketed);
            if (per_appearance()) {
                c.set(7, games);
                print(label + 0x30, text::count_of_per);
            } else print(label + 0x44, text::count_of);
            return;
        }
        c.set(6, games);
        if (!nonzero_logical(c, games) && !zeros()) return;
        if (totals) c.set(5, bracketed);
        if (per_appearance()) print(label + (totals ? 0x74 : 0x70), text::count_per);
        else print(label + (totals ? 0x88 : 0x80), text::count);
    }
    // A percentage, 100 * part / whole truncated, "%d%%" (with " / %d", r6
    // the appearances, on request). The caller has loaded `whole` into r6.
    void percentage(std::int32_t part, std::int32_t whole, std::uint32_t games, PercentSlots per,
                    std::uint32_t back_per, PercentSlots plain, std::uint32_t back_plain) {
        if (per_appearance()) {
            c.set(5, percent(part, whole, per));
            c.set(6, games);
            print(back_per, text::percent_per);
        } else {
            c.set(5, percent(part, whole, plain));
            print(back_plain, text::percent);
        }
    }
    // value / appearances as a double (format_per with " / %d", r7 the
    // appearances); for none, nothing, or `none_text` when `none_back` is set.
    void per_game(std::int32_t value, std::uint32_t games, std::uint32_t format_per, RatioSlots per,
                  std::uint32_t back_per, std::uint32_t format, RatioSlots plain, std::uint32_t back,
                  std::uint32_t none_back = 0) {
        c.set(7, games);
        if (!nonzero_logical(c, games)) {
            if (none_back) copy(none_back, text::no_value);
            return;
        }
        if (per_appearance()) {
            ratio(value, games, per);
            print(back_per, format_per);
        } else {
            ratio(value, games, plain);
            print(back, format);
        }
    }
    // 90 * value / minutes as "%3.1f" (" / %d" with the appearances in r7).
    void per_ninety(std::int32_t value, std::int32_t minutes, std::uint32_t games, std::uint32_t back_per,
                    std::uint32_t back) {
        if (per_appearance()) {
            ninety(value, minutes);
            c.set(7, games);
            print(back_per, text::ratio1_per);
        } else {
            ninety(value, minutes);
            print(back, text::ratio1);
        }
    }

private:
    // The compiler's int-to-double: the words 0x43300000 and `low` at
    // sp+slot, read back as a double (the bias is subtracted by the caller).
    double stacked(std::int32_t slot, std::uint32_t low) {
        return stacked_integer(c, frame.at(slot), low);
    }
    double signed_bias() const { return c.f64(constants + 0x20); }    // 2^52 + 2^31
    double unsigned_bias() const { return c.f64(constants + 0x30); }  // 2^52
    // A double varargs argument: stored at sp+0x20, passed as r5:r6.
    void as_words(double value) {
        c.put_f64(frame.at(0x20), value);
        c.set(5, c.u32(frame.at(0x20)));
        c.set(6, c.u32(frame.at(0x24)));
    }
    // fctiwz(100 * part / whole) in single precision; leaves f0-f3 as the original.
    std::uint32_t percent(std::int32_t part, std::int32_t whole, PercentSlots slots) {
        Fpu fpu{c};
        const double bias = signed_bias(), hundred = c.f32(constants2 + 0x14);
        const double scaled = fpu.fmuls(hundred, fpu.fsubs(stacked(slots.part, std::uint32_t(part) ^ 0x80000000u), bias));
        const double quotient = fpu.fdivs(scaled, fpu.fsubs(stacked(slots.whole, std::uint32_t(whole) ^ 0x80000000u), bias));
        const auto bits = fpu.fctiwz(0, quotient);
        c.put64(frame.at(slots.result), bits);
        c.set_f(1, scaled);
        c.set_f(2, bias);
        c.set_f(3, hundred);
        return std::uint32_t(bits);
    }
    // value / games (games unsigned) in single precision, into r5:r6.
    void ratio(std::int32_t value, std::uint32_t games, RatioSlots slots) {
        Fpu fpu{c};
        const double bias = signed_bias(), games_bias = unsigned_bias();
        const double numerator = fpu.fsubs(stacked(slots.value, std::uint32_t(value) ^ 0x80000000u), bias);
        const double quotient = fpu.fdivs(numerator, fpu.fsubs(stacked(slots.appearances, games), games_bias));
        c.set_f(0, quotient);
        c.set_f(1, games_bias);
        c.set_f(2, numerator);
        as_words(quotient);
    }
    // 90 * value / minutes in single precision, into r5:r6 (slots 0xb0, 0xb8).
    void ninety(std::int32_t value, std::int32_t minutes) {
        Fpu fpu{c};
        const double bias = signed_bias(), factor = c.f32(constants2 + 0x1c);
        const double scaled = fpu.fmuls(factor, fpu.fsubs(stacked(0xb0, std::uint32_t(value) ^ 0x80000000u), bias));
        const double quotient = fpu.fdivs(scaled, fpu.fsubs(stacked(0xb8, std::uint32_t(minutes) ^ 0x80000000u), bias));
        c.set_f(0, quotient);
        c.set_f(1, scaled);
        c.set_f(2, bias);
        c.set_f(3, factor);
        as_words(quotient);
    }

    Cpu& c;
    const Frame& frame;
    std::uint32_t buffer, pool, constants, constants2;
    std::uint32_t detail_, per_appearance_, zeros_;
};

// The case labels of each routine's jump table (columns 1-17).
constexpr std::uint32_t season_labels[18] = {0,          0x103042d0, 0x10304324, 0x1030436c, 0x103043b4, 0x1030443c,
                                             0x10304484, 0x103044cc, 0x10304514, 0x1030455c, 0x103045a4, 0x103045ec,
                                             0x10304634, 0x10304754, 0x10304830, 0x103048f8, 0x103049c0, 0x1030467c};
constexpr std::uint32_t totals_labels[18] = {0,          0x10304c50, 0x10304ca4, 0x10304cec, 0x10304d34, 0x10304dc4,
                                             0x10304e0c, 0x10304e54, 0x10304e9c, 0x10304ee4, 0x10304f2c, 0x10304f74,
                                             0x10304fbc, 0x103050dc, 0x103051b8, 0x10305294, 0x10305370, 0x10305004};
// Columns the jump table covers (1-17; 0 and the rest assert).
bool listed_column(Cpu& c, std::int8_t column) {
    return compare_logical(c, std::uint32_t(std::int32_t(column)), average_rating) <= 0 && column != 0;
}
}

std::uint32_t season_value_text(Cpu& c, std::uint32_t record, std::int8_t column, std::uint32_t buffer,
                                std::int32_t size, std::uint32_t detail, std::uint32_t per_appearance,
                                std::uint32_t zeros) {
    Frame frame(c, 0x10304190, 0xf0, {.first_gpr = 22});
    const bool has_buffer = nonzero(c, std::int32_t(buffer));  // or. r31,r6,r6
    const auto pool = c.toc(slot::pool);
    keep(c, {{22, detail}, {24, c.toc(slot::success)}, {25, c.toc(slot::message)}, {26, c.toc(slot::constants2)},
             {27, zeros}, {28, per_appearance}, {29, pool}, {30, record}, {31, buffer}});
    if (!has_buffer || compare(c, size, minimum_text_buffer) < 0) {
        report(c, pool, 0x103041e4, frame.at(0x90), frame.at(0x48), 0x7ff, false);
        return 0;
    }
    keep(c, {{23, std::uint32_t(std::int32_t(column))}});
    ValueText value{c, frame, buffer, pool, detail, per_appearance, zeros};
    if (compare(c, column, average_rating) == 0) value.copy(0x10304244, text::no_value);
    else value.copy(0x10304254, text::dash);
    if (!nonzero_logical(c, record)) {
        report(c, pool, 0x10304270, frame.at(0x70), frame.at(0x40), 0x811, false);
        return 0;
    }
    if (!listed_column(c, column)) {
        report(c, pool, 0x10304ab0, frame.at(0x50), frame.at(0x38), 0x91b, false);
        return 0;
    }
    const SeasonRecord season{c, record};
    const std::uint32_t games = season.appearances();
    const auto label = season_labels[column];
    switch (Column(column)) {
    case appearances: value.appearance_count(label, games, season.substitute_appearances()); break;
    case penalties: value.penalty_count(label, games, season.tally(penalties), season.penalties_bracketed(), false); break;
    case pass_completion: {
        const std::int16_t passes = season.passes();
        c.set(6, extended(passes));
        if (compare(c, passes) > 0)
            value.percentage(season.passes_completed(), passes, games, {0xb8, 0xc0, 0xb0}, 0x103047c8, {0xb0, 0xb8, 0xc0},
                             0x10304828);
        break;
    }
    case shots_on_target: {
        const std::int16_t shots = season.shots();
        c.set(6, extended(shots));
        if (compare(c, shots) > 0)
            value.percentage(season.shots_on_target(), shots, games, {0xb0, 0xb8, 0xc0}, 0x10304a34, {0xb0, 0xb8, 0xc0},
                             0x10304a94);
        break;
    }
    case tackles:
        value.per_game(season.tackles(), games, text::ratio1_per, {0xb0, 0xb8}, 0x10304898, text::ratio1, {0xb0, 0xb8},
                       0x103048f0);
        break;
    case dribbles:
        value.per_game(season.dribbles(), games, text::ratio1_per, {0xb0, 0xb8}, 0x10304960, text::ratio1, {0xb0, 0xb8},
                       0x103049b8);
        break;
    case average_rating:
        value.per_game(season.rating_total(), games, text::ratio2_per, {0xc0, 0xb8}, 0x103046e4, text::ratio2,
                       {0xb8, 0xc0}, 0x1030473c, 0x1030474c);
        break;
    default: value.tally(label, games, season.tally(column)); break;  // goals ... man of the match
    }
    return 1;
}

std::uint32_t totals_value_text(Cpu& c, std::uint32_t totals, std::int8_t column, std::uint32_t buffer,
                                std::int32_t size, std::uint32_t detail, std::uint32_t per_appearance,
                                std::uint32_t zeros) {
    Frame frame(c, 0x10304b10, 0xf0, {.first_gpr = 22});
    const bool has_buffer = nonzero(c, std::int32_t(buffer));  // or. r30,r6,r6
    const auto pool = c.toc(slot::pool);
    keep(c, {{22, detail}, {24, c.toc(slot::success)}, {25, c.toc(slot::message)}, {26, c.toc(slot::constants2)},
             {27, zeros}, {28, per_appearance}, {29, totals}, {30, buffer}, {31, pool}});
    if (!has_buffer || compare(c, size, minimum_text_buffer) < 0) {
        report(c, pool, 0x10304b64, frame.at(0x90), frame.at(0x48), 0x932, false);
        return 0;
    }
    keep(c, {{23, std::uint32_t(std::int32_t(column))}});
    ValueText value{c, frame, buffer, pool, detail, per_appearance, zeros};
    if (compare(c, column, average_rating) == 0) value.copy(0x10304bc4, text::no_value);
    else value.copy(0x10304bd4, text::dash);
    if (!nonzero_logical(c, totals)) {
        report(c, pool, 0x10304bf0, frame.at(0x70), frame.at(0x40), 0x944, false);
        return 0;
    }
    if (!listed_column(c, column)) {
        report(c, pool, 0x1030548c, frame.at(0x50), frame.at(0x38), 0xa50, false);
        return 0;
    }
    const SeasonTotals sum{c, totals};
    const std::uint32_t games = sum.appearances();
    const auto label = totals_labels[column];
    switch (Column(column)) {
    case appearances: value.appearance_count(label, games, sum.substitute_appearances()); break;
    case penalties: value.penalty_count(label, games, sum.tally(penalties), sum.penalties_bracketed(), true); break;
    case pass_completion: {
        // Shown from 20 passes on.
        const std::int16_t passes = sum.passes();
        c.set(6, extended(passes));
        if (compare(c, passes, 19) > 0)
            value.percentage(sum.passes_completed(), passes, games, {0xb8, 0xc0, 0xb0}, 0x10305150, {0xb0, 0xb8, 0xc0},
                             0x103051b0);
        break;
    }
    case tackles:
    case dribbles: {
        // Per 90 minutes, from 150 minutes on.
        const std::int32_t minutes = sum.minutes();
        c.set(6, std::uint32_t(minutes));
        if (compare(c, minutes, 150) < 0) break;
        if (column == tackles) value.per_ninety(sum.tackles(), minutes, games, 0x1030522c, 0x1030528c);
        else value.per_ninety(sum.dribbles(), minutes, games, 0x10305308, 0x10305368);
        break;
    }
    case shots_on_target: {
        // Shown for over 24 shots, over 4 at no more than 45 minutes a
        // shot, or over 10 in over 540 minutes.
        const std::int16_t shots = sum.shots();
        const std::int32_t minutes = sum.minutes();
        c.set(6, extended(shots));
        const auto shown = [&] {
            if (compare(c, shots, 24) > 0) return true;
            if (compare(c, shots, 4) > 0 && compare(c, minutes / shots, 45) <= 0) return true;
            return compare(c, minutes, 540) > 0 && compare(c, shots, 10) > 0;
        };
        if (shown())
            value.percentage(sum.shots_on_target(), shots, games, {0xb0, 0xb8, 0xc0}, 0x10305410, {0xb0, 0xb8, 0xc0},
                             0x10305470);
        break;
    }
    case average_rating:
        value.per_game(sum.rating_total(), games, text::ratio2_per, {0xc0, 0xb8}, 0x1030506c, text::ratio2, {0xb8, 0xc0},
                       0x103050c4, 0x103050d4);
        break;
    default: value.tally(label, games, sum.tally(column)); break;  // goals ... man of the match
    }
    return 1;
}
}
