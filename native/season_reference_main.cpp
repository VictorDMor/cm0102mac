#include "season_reference.hpp"
#include "match_replay.hpp"
#include "calendar_adapter.hpp"
#include <iostream>
#include <string_view>

int main(int argc, char** argv) {
    try {
        if (argc < 2) throw std::runtime_error("Usage: cm-season-reference <fixture> [--modern|--calendar-baseline|--date-ordering-baseline|--date-before-baseline|--date-month-baseline|--date-day-of-month-baseline|--date-weekday-baseline] [--perturb-rng|--perturb-calendar|--perturb-difference|--perturb-ordering|--perturb-before|--perturb-month|--perturb-day-of-month|--perturb-weekday] [--calendar-record|--calendar-check|--calendar-record-v2|--calendar-check-v2 <private-edges.gz>] [--month-record|--month-check <private-month.gz>] [--day-of-month-record|--day-of-month-check <private-day.gz>] [--weekday-record|--weekday-check <private-weekday.gz>]");
        bool modern = false, baseline = false, perturb_rng = false, perturb_calendar = false, record = false;
        bool perturb_difference = false;
        bool ordering_baseline = false, perturb_ordering = false;
        bool before_baseline = false, perturb_before = false, include_before = false;
        bool month_baseline = false, perturb_month = false, month_record = false;
        bool day_baseline = false, perturb_day = false, day_record = false;
        bool weekday_baseline = false, perturb_weekday = false, weekday_record = false;
        std::filesystem::path edges, month_edges, day_edges, weekday_edges;
        for (int i = 2; i < argc; ++i) {
            const std::string_view option(argv[i]);
            if (option == "--modern") modern = true;
            else if (option == "--calendar-baseline") { modern = true; baseline = true; }
            else if (option == "--date-ordering-baseline") { modern = true; ordering_baseline = true; }
            else if (option == "--date-before-baseline") { modern = true; before_baseline = true; }
            else if (option == "--date-month-baseline") { modern = true; month_baseline = true; }
            else if (option == "--date-day-of-month-baseline") { modern = true; day_baseline = true; }
            else if (option == "--date-weekday-baseline") { modern = true; weekday_baseline = true; }
            else if (option == "--perturb-weekday") perturb_weekday = true;
            else if ((option == "--weekday-record" || option == "--weekday-check") && i + 1 < argc) {
                weekday_record = option == "--weekday-record"; weekday_edges = argv[++i];
            }
            else if (option == "--perturb-day-of-month") perturb_day = true;
            else if ((option == "--day-of-month-record" || option == "--day-of-month-check") && i + 1 < argc) {
                day_record = option == "--day-of-month-record"; day_edges = argv[++i];
            }
            else if (option == "--perturb-month") perturb_month = true;
            else if ((option == "--month-record" || option == "--month-check") && i + 1 < argc) {
                month_record = option == "--month-record"; month_edges = argv[++i];
            }
            else if (option == "--perturb-before") perturb_before = true;
            else if (option == "--perturb-rng") perturb_rng = true;
            else if (option == "--perturb-calendar") perturb_calendar = true;
            else if (option == "--perturb-difference") perturb_difference = true;
            else if (option == "--perturb-ordering") perturb_ordering = true;
            else if ((option == "--calendar-record" || option == "--calendar-check" ||
                      option == "--calendar-record-v2" || option == "--calendar-check-v2") && i + 1 < argc) {
                include_before = option == "--calendar-record-v2" || option == "--calendar-check-v2";
                record = option == "--calendar-record" || option == "--calendar-record-v2"; edges = argv[++i];
            } else throw std::runtime_error("Invalid season replay option");
        }
        if ((int(baseline) + int(ordering_baseline) + int(before_baseline) + int(month_baseline) + int(day_baseline) + int(weekday_baseline) > 1) ||
            (weekday_record && modern) ||
            (perturb_weekday && (!modern || weekday_record || weekday_edges.empty() || baseline || ordering_baseline || before_baseline || month_baseline || day_baseline || weekday_baseline)) ||
            (day_record && modern) ||
            (day_baseline && (baseline || ordering_baseline || before_baseline || month_baseline)) ||
            (perturb_day && (!modern || day_record || day_edges.empty() || day_baseline || baseline || ordering_baseline || before_baseline || month_baseline)) ||
            (month_record && modern) || (perturb_month && (!modern || month_edges.empty() || month_baseline || baseline || ordering_baseline || before_baseline)) ||
            (month_baseline && (baseline || ordering_baseline || before_baseline)) || (record && modern) || ((perturb_calendar || perturb_difference || perturb_ordering || perturb_before) && (!modern || edges.empty())) ||
            (perturb_difference && baseline) || (perturb_ordering && (baseline || ordering_baseline)) ||
            ((baseline && ordering_baseline) || (before_baseline && (baseline || ordering_baseline))) ||
            (perturb_before && (!include_before || baseline || ordering_baseline || before_baseline)))
            throw std::runtime_error("Calendar references require original execution; calendar perturbation requires a modern edge check");
        const std::filesystem::path directory(argv[1]);
        auto state = std::make_unique<cm::aot::State>();
        auto expected = std::make_unique<cm::aot::State>();
        cm::aot::load_match_snapshot(*state, directory / "initial.snapshot");
        cm::aot::load_match_snapshot(*expected, directory / "expected.snapshot");
        state->calendar_original = !modern;
        state->date_difference_original = baseline;
        state->date_ordering_original = baseline || ordering_baseline;
        state->date_before_original = baseline || ordering_baseline || before_baseline;
        state->date_month_original = baseline || ordering_baseline || before_baseline || month_baseline;
        state->date_day_of_month_original = baseline || ordering_baseline || before_baseline || month_baseline || day_baseline;
        state->date_weekday_original = baseline || ordering_baseline || before_baseline || month_baseline || day_baseline || weekday_baseline;
        if (expected->executed <= state->executed)
            throw std::runtime_error("Private fixture has no bounded execution interval");
        // A real finite upper bound remains independent of the replacement's
        // reduced accounting. End-of-transcript plus final morning terminates.
        state->budget = expected->executed;
        if (perturb_rng) {
            const auto seed = state->memory.read<4>(0x108b3f00);
            state->memory.write<4>(seed, state->memory.read<4>(seed) ^ 1);
        }
        if (!edges.empty()) cm::aot::open_calendar_audit(edges, record, perturb_calendar, perturb_difference, perturb_ordering, include_before, perturb_before);
        if (!month_edges.empty()) cm::aot::open_month_audit(month_edges, month_record, perturb_month);
        if (!day_edges.empty()) cm::aot::open_day_of_month_audit(day_edges, day_record, perturb_day);
        if (!weekday_edges.empty()) cm::aot::open_weekday_audit(weekday_edges, weekday_record, perturb_weekday);
        cm::aot::open_season_replay(directory, modern);
        bool complete = false;
        try { cm::aot::run(*state); }
        catch (const cm::aot::SeasonReplayComplete&) { complete = true; }
        cm::aot::close_season_replay();
        cm::aot::close_calendar_audit(true);
        cm::aot::close_month_audit(true);
        cm::aot::close_day_of_month_audit(true);
        cm::aot::close_weekday_audit(true);
        if (!complete) throw std::runtime_error("Replay returned before the final full-day boundary");
        const auto difference = cm::aot::compare_season_states(*expected, *state, modern);
        if (!difference.empty()) throw std::runtime_error(difference);
        std::cout << (modern ? "PASS: semantic calendar bounded simulation agreement\n" : "PASS: exact bounded simulation agreement\n")
                  << cm::aot::season_state_summary(*state) << '\n'
                  << cm::aot::calendar_call_summary(*state) << '\n';
        if (modern && (!state->calendar_calls[0] || !state->calendar_calls[1] || state->calendar_exceptional_calls))
            throw std::runtime_error("Real fixture did not exercise both modern calendar routines exclusively");
        if (modern && !baseline && !state->date_difference_calls)
            throw std::runtime_error("Real fixture did not exercise semantic date difference");
        if (baseline && state->date_difference_calls)
            throw std::runtime_error("Delivered calendar baseline unexpectedly used modern date difference");
        if ((baseline || ordering_baseline) && state->date_ordering_calls)
            throw std::runtime_error("Original ordering baseline unexpectedly used modern date ordering");
        if ((baseline || ordering_baseline || before_baseline) && state->date_before_calls)
            throw std::runtime_error("Original before baseline unexpectedly used modern date before");
        if ((baseline || ordering_baseline || before_baseline || month_baseline) && state->date_month_calls)
            throw std::runtime_error("Original month baseline unexpectedly used modern date month");
        if (state->date_day_of_month_original && state->date_day_of_month_calls)
            throw std::runtime_error("Original day-of-month baseline unexpectedly used modern day-of-month");
        if (state->date_weekday_original && state->date_weekday_calls)
            throw std::runtime_error("Original weekday baseline unexpectedly used modern weekday");
        // Existing immutable fixtures may never call this independent entry.
        // Report zero honestly; the checker requires separate exercised
        // coverage before accepting an ordering negative control.
        return 0;
    } catch (const std::exception& error) {
        cm::aot::close_season_replay();
        try { cm::aot::close_calendar_audit(); } catch (...) {}
        try { cm::aot::close_month_audit(); } catch (...) {}
        try { cm::aot::close_day_of_month_audit(); } catch (...) {}
        try { cm::aot::close_weekday_audit(); } catch (...) {}
        std::cerr << error.what() << '\n';
        return 1;
    }
}
