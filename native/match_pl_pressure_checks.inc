namespace {
void original_pressure(State& s, std::uint32_t entry) {
    reference_mode = true;
    s.pc = entry;
    run_until(s, sentinel);
    require_return(s);
    if (active.pressure.actual_acquisition)
        cm::semantic_io_checks::require(s.memory.read<4>(match + 0xe000) == player,
            "Original pressure verdict did not acquire the ball");
    if (active.pressure.expected_recipient_route)
        cm::semantic_io_checks::require(s.memory.read<1>(trace_at + 0x4000) == active.pressure.expected_recipient_route,
            "Original pressure verdict missed the expected restart recipient route");
}

void rewritten_pressure(State& s, std::uint32_t) {
    reference_mode = false;
    Cpu c(s);
    { match_pl::CellPressure pressure(c); pressure.run(); }
    c.finish();
    require_return(s);
    if (active.pressure.expected_recipient_route)
        cm::semantic_io_checks::require(c.u8(trace_at + 0x4000) == active.pressure.expected_recipient_route,
            "C++ pressure verdict missed the expected restart recipient route");
#if defined(CM_MATCH_PL_COVERAGE)
    __llvm_profile_write_file();
#endif
}

int check_pressure_matrix(const State& base, std::string_view mode_name) {
    const bool unsettled = mode_name == "--pressure-unsettled-return";
    const bool matrix = mode_name == "--pressure-matrix";
    const bool published = mode_name == "--pressure-published-return";
    unsigned matched = 0, failed = 0;
    for (int side : {0, 1}) for (int mode : {-1, 0, 1, 2, 3, 4, 5, 6})
    for (int scenario : {0, 1, 2, 3, 4, 5, 6, 7})
    for (int edge : {0, 1, 2}) {
        if (unsettled && (mode != 0 && mode != 4)) continue;
        if (published && (mode != 0 || scenario < 3)) continue;
        active = {pressure_entry, side, 4, side ? 11 : 0};
        active.first_role = scenario >= 4;
        active.second_role = scenario != 2;
        active.argument = scenario == 7 ? 100000 : edge == 1 ? std::uint32_t(-17) : 45;
        auto state = cm::semantic_io_checks::clone(base);
        Cpu c(*state);
        initialize_case(c);
        c.set({{3, player}, {4, 300}, {5, std::uint32_t(mode)}, {7, 0}});
        c.set_f(1, edge == 2 ? 0.0 : 4.0);
        c.put32(match + 0xdff4, scenario == 0 ? player : 0);
        c.put8(match + 0x7924, scenario < 4 ? 1 - side : side);
        c.put8(player + 0x110, scenario == 1 ? 10 : 0);
        if (scenario == 4) c.put8(player + 0x106, 5);
        constexpr float keeper_power[]{-1000000, 0, 1000000};
        c.put_f32(player + 0xa8, scenario < 6 ? 17 : keeper_power[edge]);
        c.put32(c.r(2) - 0x7b90, data + 0x100);
        c.put32(data + 0x100, edge == 1 ? std::uint32_t(-7000) : 0);
        c.put32(c.r(2) - 0x7b8c, data + 0x200);
        c.put32(c.r(2) - 0x7b88, data + 0x104);
        c.put32(data + 0x104, data + 0x206);
        c.put32(c.r(2) - 0x7b84, data + 0x108);
        c.put32(data + 0x108, data + (edge == 2 ? 0x206 : 0x200));
        for (unsigned slot = 0; slot < 4; ++slot)
            c.put16(data + 0x200 + slot * 2, scenario == 7 ? 65000 : 17 + slot * 2);
        if (published) {
            constexpr unsigned codes[]{0, 2, 3, 4, 6, 7, 9, 0xff};
            constexpr unsigned words[]{441, 443, 440};
            active.context_flags = codes[scenario];
            active.last_is_player = scenario == 5 && edge == 1;
            active.first_role = 0;
            active.strategy_low = edge == 0;
            active.mutate_choice = true;
            active.null_player = edge == 0;
            active.second_role = scenario != 4;
            active.strategy_high = scenario != 6;
            active.feedback_alias = edge != 0;
            active.no_feedback = scenario == 7 && edge == 0;
            c.set(4, 0);
            c.set_f(1, 4);
            c.put8(match + 0x7924, side);
            c.put8(match + 0x7921, 1 - side);
            c.put8(match + 0x7920, edge == 0 ? 0 : edge == 1 ? -4 : 3);
            c.put32(match + 0x3108, 345);
            c.put8(player + 0x106, (scenario == 4 || scenario == 6) ? 5 : side ? 11 : 0);
            c.put32(player + 0x6c, data + 0xc000);
            c.put8(data + 0xc066, edge == 0 ? 0 : edge == 1 ? -3 : 3);
            c.put8(data + 0xc068, edge == 0 ? -3 : 1);
            c.put32(match + 0x3118, data + 0xa000);
            c.put32(data + 0xa01c, data + 0xb000);
            c.put32(data + 0xa020, data + 0xb000);
            c.put32(data + 0xb02c, edge == 0 ? 0 : data + 0xd000);
            c.put8(data + 0xd119, edge == 1 ? 7 : 8);
            c.put8(data + 0xf02f, 17);
            c.put8(data + 0xf105, 4);
            c.put8(data + 0xf106, edge == 1 ? 5 : side ? 11 : 0);
            c.put32(data + 0x108, data + 0x200);
            c.put32(data + 0x104, data + 0x22e);
            c.put32(data + 0x100, 0);
            for (unsigned slot = 0; slot < 24; ++slot)
                c.put16(data + 0x200 + slot * 2, slot < 3 ? slot : words[edge]);
            for (unsigned index : {1u, 21u, 36u, 37u, 41u})
                c.put16(player + 0x11e + index * 2, 1000);
        }
        if (matrix) {
            c.put32(player + 0x6c, data + 0xc000);
            c.put8(data + 0xc068, edge == 0 ? -3 : 1);
            c.put8(match + 0x7921, scenario % 2 ? side : 1 - side);
            c.put8(match + 0x7920, edge == 0 ? 0 : edge == 1 ? -4 : 3);
            c.put8(match + 0x791f, 5);
            c.put16(match + 0x7948, 24);
            c.put32(match + 0x3108, 345);
            for (unsigned index : {1u, 21u, 36u, 37u, 41u})
                c.put16(player + 0x11e + index * 2, index * 3);
        }
        if (unsettled) {
            active.pressure.override_contact_draws = true;
            active.pressure.keeper_after_contact = scenario >= 4;
            active.pressure.clock = edge == 0 ? 0 : edge == 1 ? -4 : 3;
            active.pressure.words = {0, 0, 0, 0, 0};
            if (scenario == 4) active.pressure.words[0] = 11;
            if (scenario == 5) active.pressure.words[1] = 201;
            if (scenario == 6) active.pressure.words[1] = 200;
            active.context_flags = 0;
            active.first_role = 0;
            c.set(4, scenario == 0 || scenario >= 7 ? 0 : 400);
            c.set(7, scenario & 1);
            c.set_f(1, edge == 0 ? 1.0 : edge == 1 ? 4.0 : 8.0);
            c.put8(match + 0x7924, side);
            c.put8(match + 0x7921, 1 - side);
            c.put8(match + 0x7920, 0);
            c.put8(match + 0x791f, scenario == 7 ? 4 : 3);
            c.put16(match + 0x7948, scenario == 2 ? -17 : 90 * 11);
            c.put8(match + 0xe03d, 0);
            c.put8(match + 0xe073, 0);
            c.put32(match + 0xdff4, 0);
            c.put32(match + 0x3108, 345);
            c.put32(player + 0x6c, data + 0xc000);
            c.put8(player + 0x106, scenario < 3 ? 5 : side ? 11 : 0);
            c.put8(data + 0xc068, 1);
            c.put16(player + 0x36, 8000);
            c.put8(player + 4, scenario == 3 ? 0 : 12);
            c.put8(player + 5, 11);
            c.put8(player + 6, 8);
            c.put8(player + 7, scenario == 3 ? 0 : 11);
            c.put8(player + 8, 10);
            c.put8(player + 9, 7);
            c.put8(player + 0x100, 0);
            for (unsigned index : {1u, 21u, 36u, 37u, 41u})
                c.put16(player + 0x11e + index * 2, 1700);
            c.put32(data + 0x100, 0);
            c.put32(data + 0x104, data + 0x23e);
            c.put32(data + 0x108, data + 0x200);
            c.put16(data + 0x202, 0);
            c.put16(data + 0x204, 2);
            c.put16(data + 0x206, scenario == 0 || scenario == 7 ? 1300 : 480);
        }
        const auto differences = compare_routine_call(*state, pressure_entry, original_pressure,
            rewritten_pressure, RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
        if (differences.empty()) ++matched;
        else {
            if (++failed <= 8) {
                std::cerr << "side=" << side << " mode=" << mode << " scenario=" << scenario << " edge=" << edge << '\n';
                for (const auto& line : differences) std::cerr << line << '\n';
            }
        }
    }
    std::cout << matched << " pressure returns match, " << failed << " with differences\n";
    return failed != 0;
}
void production_pressure(State& s, std::uint32_t entry) {
    reference_mode = false;
    s.pc = entry;
    s.budget = active.pressure.actual_acquisition ? 192 : 0;
    run_until(s, sentinel);
    require_return(s);
    if (active.pressure.actual_acquisition)
        cm::semantic_io_checks::require(s.memory.read<4>(match + 0xe000) == player,
            "C++ pressure verdict did not acquire the ball");
}

int check_pressure_gaps(const State& base, std::string_view mode) {
    const bool production = mode == "--pressure-production";
    const bool receiver_retry = mode == "--pressure-recipient-fallback";
    constexpr auto rng = data + 0x200;
    constexpr auto end = data + 0x104, cursor = data + 0x108, seed = data + 0x100;
    unsigned checked = 0, failed = 0;
    auto check = [&](std::string_view label, int side, auto arrange) {
        active = {pressure_entry, side, 4, side ? 11 : 0};
        active.first_role = 0;
        active.second_role = 1;
        auto state = cm::semantic_io_checks::clone(base);
        Cpu c(*state);
        initialize_case(c);
        c.set({{3, player}, {4, 0}, {5, 0}, {7, 1}});
        c.set_f(1, 4);
        c.put32(match + 0xdff4, 0);
        c.put32(match + 0x3108, 345);
        c.put8(match + 0x7924, side);
        c.put8(match + 0x7921, 1 - side);
        c.put8(match + 0x7920, 0);
        c.put8(match + 0x791f, 0);
        c.put16(match + 0x7948, 90 * 11);
        c.put8(match + 0xe03d, 0);
        c.put8(match + 0xe073, 0);
        c.put32(match + 0x3118, data + 0xa000);
        c.put32(data + 0xa01c, data + 0xb000); c.put32(data + 0xa020, data + 0xb000);
        c.put32(data + 0xb02c, data + 0xd000); c.put8(data + 0xd119, 8);
        c.put32(player + 0x6c, data + 0xc000);
        c.put8(data + 0xc066, 0); c.put8(data + 0xc068, 1);
        c.put_f32(player + 0xa8, 17);
        c.put8(player + 0x110, 0);
        c.put8(player + 4, 10); c.put8(player + 5, 9); c.put8(player + 6, 8);
        c.put8(player + 7, 10); c.put8(player + 8, 9); c.put8(player + 9, 8);
        c.put16(player + 0x36, 8000);
        c.put8(player + 0x100, 0);
        c.put8(data + 0xf105, 4); c.put8(data + 0xf106, 5);
        c.put32(c.r(2) - 0x7b90, seed); c.put32(seed, 0);
        c.put32(c.r(2) - 0x7b8c, rng);
        c.put32(c.r(2) - 0x7b88, end); c.put32(end, rng + 0x3e);
        c.put32(c.r(2) - 0x7b84, cursor); c.put32(cursor, rng);
        for (unsigned slot = 0; slot < 32; ++slot) c.put16(rng + slot * 2, slot < 3 ? slot : 441);
        for (unsigned index : {1u, 21u, 36u, 37u, 41u})
            c.put16(player + 0x11e + index * 2, 1700);
        arrange(c);
        constexpr unsigned results = RoutineChecker::result_r3 | RoutineChecker::result_f1;
        const auto differences = compare_routine_call(*state, pressure_entry, original_pressure,
            production ? production_pressure : rewritten_pressure, results, false, true);
        if (production && active.pressure.actual_acquisition && differences.empty()) {
            auto forced_original = cm::semantic_io_checks::clone(*state);
            forced_original->budget = 192;
            reference_mode = false;
            bool exhausted = false;
            try { match_pl_run_original(*forced_original, pressure_entry); }
            catch (const std::runtime_error& error) {
                if (std::string_view(error.what()) != "Native instruction budget exhausted") throw;
                exhausted = true;
            }
            cm::semantic_io_checks::require(exhausted, "Original pressure body fit the production budget");
            const auto corrupt_publication = +[](State& s, std::uint32_t entry) {
                production_pressure(s, entry);
                Cpu c(s);
                c.put16(player + 0x19c, c.u16(player + 0x19c) ^ 1);
            };
            const auto control = compare_routine_call(*state, pressure_entry, original_pressure, corrupt_publication, results);
            cm::semantic_io_checks::require(std::ranges::any_of(control, [](const auto& line) {
                return line.starts_with("byte 6000119d ");
            }), "Pressure publication corruption escaped comparison");
            const auto corrupt_trace = +[](State& s, std::uint32_t entry) {
                production_pressure(s, entry);
                Cpu c(s);
                c.put32(trace_at + 12, c.u32(trace_at + 12) ^ 1);
            };
            const auto trace_control = compare_routine_call(*state, pressure_entry, original_pressure, corrupt_trace, results);
            cm::semantic_io_checks::require(std::ranges::any_of(trace_control, [](const auto& line) {
                return line.starts_with("byte 6003800f ");
            }), "Pressure call-argument corruption escaped comparison");
        }
        ++checked;
        if (differences.empty()) return;
        ++failed;
        std::cerr << label << " side=" << side << '\n';
        for (const auto& line : differences) std::cerr << line << '\n';
    };
    for (int side : {0, 1}) {
        if (production) {
            check("normal entry rejects active target", side,
                [](Cpu& c) { c.put32(match + 0xdff4, player); });
            check("normal entry uses real ball acquisition", side, [](Cpu& c) {
                active.pressure.actual_acquisition = true;
                active.pressure.override_contact_draws = true;
                c.set_f(1, 8);
                c.put32(match + 0xe000, 0);
                c.put16(rng + 2, 0); c.put16(rng + 6, 1300);
            });
            continue;
        }
        if (receiver_retry) {
            for (bool attacking_band : {false, true}) {
                check("restart receiver attacking-band result", side, [=](Cpu& c) {
                    active.context_flags = 6;
                    active.strategy_high = 1;
                    active.second_role = attacking_band;
                    active.feedback_alias = true;
                    active.pressure.effects_second_draw = 8;
                    active.pressure.expected_recipient_route = attacking_band ? 2 : 1;
                    c.put8(trace_at + 0x4000, 0);
                    c.put8(player + 0x105, 0);
                    c.put8(player + 0x106, 5);
                    c.put8(data + 0xf027, 0);
                    c.put8(data + 0xf106, attacking_band ? 9 : 5);
                });
            }
            continue;
        }
        for (bool allow : {false, true}) {
            check("eligibility refill and row service", side, [=](Cpu& c) {
                c.put8(match + 0x7924, 1 - side);
                c.put8(player + 0x110, -1);
                c.put32(cursor, rng + 0x3e);
                active.second_role = allow;
            });
        }
        for (int column : {-1, 8}) {
            check("keeper outside goal columns", side, [=](Cpu& c) {
                active.first_role = 1;
                c.put8(player + 0x105, column);
            });
        }
        for (int bound : {0, 45}) {
            check("keeper narrow bound", side, [=](Cpu& c) {
                active.first_role = 1;
                const auto floats = c.toc(-0x51e0);
                c.put_f32(floats + 0x80, 1);
                c.put_f32(floats + 0xd8, 0);
                c.put_f32(player + 0xa8, bound);
            });
        }
        check("expansive draw cap", side, [](Cpu& c) {
            c.set(5, 5);
            active.argument = 2147483642;
        });
        for (int clock : {-4, 0, 4}) {
            check("same-side pressure restart", side, [=](Cpu& c) {
                c.put8(match + 0x7921, side);
                c.put8(match + 0x7920, clock);
                c.put16(rng + 2, 0); c.put16(rng + 4, 0);
            });
        }
        check("zero restart bound", side, [=](Cpu& c) {
            c.put8(match + 0x7921, side);
            c.set_f(1, -10);
            c.put32(seed, std::uint32_t(-1000));
            c.put16(rng + 2, 400);
        });
        for (int strength : {-1700, 0, 1700}) for (int context : {-301, -300, 0}) {
            check("signed strength and context", side, [=](Cpu& c) {
                for (unsigned index : {1u, 21u, 36u, 37u, 41u})
                    c.put16(player + 0x11e + index * 2, strength);
                c.set(4, std::uint32_t(context));
                c.put16(rng + 6, 47);
            });
        }
        for (unsigned sanction : {2u, 3u, 7u, 9u, 0xffu}) for (unsigned word : {0u, 1u, 2u, 9u, 400u}) {
            check("sanction decision and announcements", side, [=](Cpu& c) {
                active.context_flags = sanction;
                active.no_feedback = word == 2;
                active.strategy_low = word == 9;
                active.strategy_high = word == 1;
                active.second_role = word != 2;
                c.put8(data + 0xf106, word == 1 ? (side ? 11 : 0) : 5);
                c.put8(data + 0xc066, word == 2 ? 7 : 0);
                active.pressure.override_contact_draws = word != 400;
                active.pressure.clock = word == 2 ? -4 : 0;
                active.pressure.words = {word, word, word, word, word};
                if (word == 9) active.pressure.words = {9, 0, 0, 9, 2};
                if (word == 400) {
                    active.mutate_choice = true;
                    active.feedback_alias = true;
                }
                c.put8(player + 0x106, word == 2 ? 5 : side ? 11 : 0);
            });
        }
        for (int clock : {-4, 20}) {
            check("forceful contact deflection and distance gate", side, [=](Cpu& c) {
                c.set(5, 3);
                c.put16(rng + 2, 0);
                for (unsigned index : {1u, 21u, 36u, 37u, 41u})
                    c.put16(player + 0x11e + index * 2, 12000);
                c.put16(rng + 6, clock < 0 ? 155 : 9500);
                c.put8(player + 0x106, 5);
                active.pressure.override_contact_draws = true;
                active.pressure.clock = clock;
                active.pressure.words = {11, 13, 15, 17, 19};
            });
        }
        for (int choice : {-1, 0, 1, 2}) {
            check("unsigned option without a named sanction", side, [=](Cpu&) {
                active.context_flags = 9;
                active.pressure.wait = 0;
                active.pressure.override_contact_draws = true;
                const auto base = choice < 0 ? 100u : 0u;
                active.pressure.seed_after_contact = -int(base);
                active.pressure.words = {base + 11, base + 12, base + choice, base + 9, base + 2};
            });
        }
        for (int condition : {0, 1, 2, 3, 4, 5, 6}) for (int mode : {0, 4}) {
            check("recovery gates and deflection", side, [=](Cpu& c) {
                c.set(5, mode);
                c.set_f(1, condition == 6 ? -10.0 : 8.0);
                active.pressure.override_contact_draws = true;
                active.pressure.keeper_after_contact = condition == 5;
                active.pressure.clock = condition == 5 ? -4 : 0;
                if (condition == 0) c.put8(match + 0xe03d + (side == 0 ? 0x36 : 0), 1);
                if (condition == 1) {
                    c.put8(player + 6, 0); c.put8(player + 8, 3);
                }
                if (condition == 2) c.put8(player + 9, 0);
                if (condition == 3) c.put16(player + 0x36, 20);
                if (condition == 4) c.put8(player + 0x100, 1);
                if (condition == 5) c.put16(rng + 6, 180);
                if (condition == 6) c.put16(rng + 6, 1200);
            });
        }
    }
    std::cout << checked << (production ? " pressure production cases" : " pressure branch cases")
              << ", " << failed << " with differences\n";
    if (production && !failed)
        std::cout << "Both original budget controls and four publication/argument corruption controls passed\n";
    return failed != 0;
}

}
