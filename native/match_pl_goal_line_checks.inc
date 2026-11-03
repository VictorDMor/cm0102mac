namespace {
enum class GoalLinePath { miss, save, review, later_review, goal, missing_keeper };

struct GoalLineMilestoneRead final : MemoryObserver {
    State& state;
    MemoryObserver* previous;
    unsigned reads = 0;

    explicit GoalLineMilestoneRead(State& s) : state(s), previous(s.memory.date_observer) {
        s.memory.date_observer = this;
    }
    ~GoalLineMilestoneRead() override { state.memory.date_observer = previous; }
    void mapped(std::uint32_t at, std::uint64_t size) override { if (previous) previous->mapped(at, size); }
    void cleared(std::uint32_t at, std::uint64_t size) override { if (previous) previous->cleared(at, size); }
    void written(std::uint32_t at, const std::uint8_t* bytes, std::size_t size) override {
        if (previous) previous->written(at, bytes, size);
    }
    void read(std::uint32_t at, const std::uint8_t* bytes, unsigned size) override {
        if (previous) previous->read(at, bytes, size);
        // Increment, first-goal check, minimum check, then the division's fresh read.
        if (at == player + 0x46 && size == 2 && ++reads == 4)
            state.memory.write<2>(at, std::uint16_t(active.goal_line.milestone_reread));
    }
};

void initialize_goal_line(Cpu& c, GoalLinePath path, int variant, int phase, unsigned word) {
    c.set(3, player);
    c.set_f(1, 1);
    c.put8(match + 0x7920, 0);
    c.put8(match + 0x791f, phase);
    c.put32(c.r(2) - 0x7b90, data + 0x100); c.put32(data + 0x100, 0);
    c.put32(c.r(2) - 0x7b8c, data + 0x200);
    c.put32(c.r(2) - 0x7b88, data + 0x104); c.put32(data + 0x104, data + 0x23e);
    c.put32(c.r(2) - 0x7b84, data + 0x108); c.put32(data + 0x108, data + 0x200);
    for (unsigned slot = 0; slot < 32; ++slot) c.put16(data + 0x200 + slot * 2, slot);
    c.put16(data + 0x202, word); c.put16(data + 0x204, 0);
    c.put16(data + 0xf036, 200);
    c.put8(data + 0xf017, active.side ? -1 : 0);
    c.put8(data + 0xf027, 1 - active.side); c.put8(data + 0xf02f, 7);
    c.put32(data + 0xf06c, data + 0xc000);
    c.put8(data + 0xc064, variant == 1 ? -1 : 3);
    c.put8(data + 0xc066, variant == 3 ? 5 : 4);
    c.put8(data + 0xf00f, 7);
    active.first_role = 1;
    active.second_role = UINT32_MAX;
    active.strategy_low = phase == 4;

    switch (path) {
    case GoalLinePath::miss:
        active.last_is_player = variant == 2;
        active.first_role = variant != 3;
        active.argument = word;
        active.strategy_low = variant == 0;
        active.second_role = active.side ? UINT32_MAX : 0;
        for (unsigned slot = 3; slot < 8; ++slot) c.put16(data + 0x200 + slot * 2, 1);
        break;
    case GoalLinePath::save: {
        constexpr int ratings_at_310[]{-310, 330, 250, 170};
        constexpr int ratings_at_750[]{600, 366, 500, 410};
        constexpr unsigned offers[]{0, 0x219f, 0x21b0, 0x21b5};
        active.argument = std::uint32_t((word == 310 ? ratings_at_310 : ratings_at_750)[variant]);
        active.goal_line.offer = offers[variant];
        active.mutate_choice = variant == 3;
        active.strategy_low = phase == 0;
        for (unsigned slot = 3; slot < 8; ++slot) c.put16(data + 0x200 + slot * 2, 0);
        break;
    }
    case GoalLinePath::review: case GoalLinePath::later_review:
        active.argument = variant * 20;
        active.second_role = variant < 2 ? unsigned(active.side) : unsigned(!active.side);
        active.strategy_low = phase == 0;
        active.goal_line.score = variant == 0 ? 6 : variant == 1 ? 1 : variant == 2 ? 4 : 5;
        c.put16(data + 0x206, phase == 0 ? 40 : 50);
        c.put16(data + 0x208, phase == 0 ? 2 : 40);
        c.put16(data + 0x20a, 40);
        if (path == GoalLinePath::later_review) {
            c.put16(data + (phase == 0 ? 0x206 : 0x208), 3);
            c.put16(data + (phase == 0 ? 0x208 : 0x20a), variant == 3 ? 0 : 2);
            c.put8(player + 0xc, 3);
            c.put8(player + 0x31, phase);
        }
        break;
    case GoalLinePath::goal: {
        active.argument = (variant % 4) * 20;
        constexpr int scores[]{4, 3, 2, 2, 1, 0, 5, 2, 2, 5, 2, 2};
        constexpr unsigned minutes[]{72, 83, 86, 115, 60, 85, 78, 65, 0, 90, 70, 50};
        active.goal_line = {scores[variant], variant == 7, variant == 6, 0, 0, variant == 3};
        c.put8(match + 0x1c1, variant == 11);
        c.put8(player + 0xc, 0);
        c.put8(player + 0x31, 0);
        c.put16(match + 0x7948, minutes[variant] * 11);
        c.put16(match + 0x794c, variant == 6 ? 0 : 0x3de);
        c.put32(match + 0x3118, data + 0xa000);
        c.put32(data + 0xa01c, data + 0xb000); c.put32(data + 0xa020, data + 0xb100);
        c.put32(data + 0xb02c, data + 0xd000); c.put32(data + 0xb12c, data + 0xd000);
        c.put8(data + 0xd119, 8);
        c.put32(data + 0x104, data + (variant == 10 ? 0x206 : 0x23e));
        for (unsigned slot = 3; slot < 32; ++slot)
            c.put16(data + 0x200 + slot * 2, variant == 9 ? 0 : slot * 7 + (word == 310 ? 0 : 3));
        break;
    }
    case GoalLinePath::missing_keeper:
        active.null_player = true;
        c.set_f(1, variant == 0 ? 0 : -1);
        if (variant == 1) c.put32(data + 0x108, data + 0x23e);
        break;
    }
}

void require_goal_line_outcome(State& s, std::uint32_t entry, bool original) {
    reference_mode = original;
    Cpu c(s);
    auto run = [&] {
        if (original) {
            s.pc = entry;
            run_until(s, sentinel);
        } else {
            match_pl::GoalLineAction(c, entry == opposite_goal_line).run();
            c.finish();
        }
    };
    try {
        if (active.goal_line.milestone_reread == INT32_MIN) run();
        else {
            GoalLineMilestoneRead milestone(s);
            run();
            cm::semantic_io_checks::require(milestone.reads >= 4, "Milestone reread did not execute");
        }
    } catch (const GoalLineBoundary&) {
        const auto back = entry == opposite_goal_line ? 0x1026a37cu : 0x1026d820u;
        cm::semantic_io_checks::require(active.null_player && s.pc == 0x1076f7d0 &&
            c.sp() == stack - 0xd0 && c.lr() == back && c.r(3) == UINT32_MAX &&
            c.u8(c.toc(-0x7bac)) == 0, "Unexpected goal-line exit boundary");
        return;
    }
    cm::semantic_io_checks::require(!active.null_player, "Missing keeper did not exit");
    require_return(s);
}

void original_goal_line(State& s, std::uint32_t entry) { require_goal_line_outcome(s, entry, true); }

void rewritten_goal_line(State& s, std::uint32_t entry) {
    require_goal_line_outcome(s, entry, false);
#if defined(CM_MATCH_PL_COVERAGE)
    __llvm_profile_write_file();
#endif
}

auto compare_goal_line(State& state, std::uint32_t entry) {
    return compare_routine_call(state, entry, original_goal_line, rewritten_goal_line,
        RoutineChecker::result_r3 | RoutineChecker::result_f1);
}

int check_goal_line(const State& base, bool fatal) {
    unsigned checked = 0;
    for (auto path : {GoalLinePath::miss, GoalLinePath::save, GoalLinePath::review, GoalLinePath::later_review,
                      GoalLinePath::goal, GoalLinePath::missing_keeper}) {
        if (fatal != (path == GoalLinePath::missing_keeper)) continue;
        const auto thresholds = path == GoalLinePath::miss
            ? std::vector<unsigned>{0, 4, 25, 189, 190, 229, 230, 233, 234, 235, 239, 240}
            : std::vector<unsigned>{310, 750};
        for (const auto entry : {opposite_goal_line, own_goal_line})
        for (int side : {0, 1}) for (int phase : {0, 4})
        for (int variant = 0; variant < (path == GoalLinePath::goal ? 12 : fatal ? 2 : 4); ++variant)
        for (const auto threshold : thresholds) {
            active = {entry, side, 4, phase};
            auto state = cm::semantic_io_checks::clone(base);
            Cpu c(*state);
            initialize_case(c);
            initialize_goal_line(c, path, variant, phase, threshold);
            const auto differences = compare_goal_line(*state, entry);
            if (!differences.empty()) {
                std::cerr << hex_address(entry) << " path=" << unsigned(path) << " side=" << side <<
                    " phase=" << phase << " variant=" << variant << " threshold=" << threshold << '\n';
                for (const auto& line : differences) std::cerr << line << '\n';
                return 1;
            }
            ++checked;
        }
    }
    std::cout << checked << (fatal ? " goal-line exit boundaries match\n" : " original/C++ goal-line returns match\n");
    return 0;
}

int check_goal_line_gaps(const State& base) {
    using enum GoalLinePath;
    constexpr auto pool = data + 0x200, pool_end = data + 0x104, pool_cursor = data + 0x108;
    constexpr auto seed = data + 0x100, keeper = data + 0xf000;
    unsigned checked = 0, failed = 0;
    auto check = [&](std::string_view label, std::uint32_t entry, GoalLinePath path, int variant,
                     int phase, unsigned threshold, auto configure) {
        active = {entry, 0, 4, phase};
        auto state = cm::semantic_io_checks::clone(base);
        Cpu c(*state);
        initialize_case(c);
        initialize_goal_line(c, path, variant, phase, threshold);
        configure(c);
        const auto differences = compare_goal_line(*state, entry);
        ++checked;
        if (differences.empty()) return;
        ++failed;
        std::cerr << hex_address(entry) << ' ' << label << '\n';
        for (const auto& line : differences) std::cerr << line << '\n';
    };
    auto milestone = [](Cpu& c) {
        for (unsigned field = 0x3e; field <= 0x46; field += 2)
            c.put16(player + field, (field - 0x3e) / 2 + 2);
    };
    for (unsigned offer : {0u, 0x21b6u}) {
        check("debut", opposite_goal_line, goal, 1, 0, 310, [=](Cpu& c) {
            c.put8(player + 0x31, 1);
            active.goal_line.offer = offer;
        });
    }
    struct Milestone { unsigned field; int count; };
    constexpr Milestone counts[]{
        {0x3e, 1}, {0x40, 1}, {0x44, 1}, {0x40, 10}, {0x40, 11}, {0x3e, 50},
        {0x3e, 100}, {0x3e, 101}, {0x42, 50}, {0x42, 100}, {0x42, 101}, {0x44, 100},
        {0x44, 101}, {0x46, 1}, {0x46, 5}, {0x46, 6}
    };
    for (const auto [field, count] : counts) {
        check("scorer counter", opposite_goal_line, goal, 1, 0, 310, [=](Cpu& c) {
            milestone(c);
            c.put8(player + 0xc, 1);
            c.put8(player + 0x31, 1);
            c.put16(player + field, count - 1);
        });
    }
    for (int value : {-1, -3, -5}) {
        check("scorer counter changed between reads", opposite_goal_line, goal, 1, 0, 310, [=](Cpu& c) {
            milestone(c);
            active.goal_line.milestone_reread = value;
        });
    }
    for (auto entry : {opposite_goal_line, own_goal_line}) {
        for (unsigned threshold : {4u, 25u}) {
            check("phase draw zero", entry, miss, 1, 4, threshold,
                [](Cpu& c) { c.put16(pool + 6, 0); });
            check("phase draw refill", entry, miss, 1, 4, threshold,
                [](Cpu& c) { c.put32(pool_end, pool + 4); });
        }
        check("second initial refill", entry, miss, 0, 0, 4, [](Cpu& c) {
            c.put32(pool_cursor, pool + 0x3c);
            c.put16(pool + 0x3e, 5);
        });
        check("negative initial draws", entry, goal, 1, 0, 310, [](Cpu& c) {
            c.put32(seed, std::uint32_t(-1400));
            c.put16(pool + 2, 410);
            c.put16(pool + 4, 100);
        });
        check("keeper recovery refill", entry, miss, 0, 4, 235,
            [](Cpu& c) { c.put32(pool_end, pool + 4); });
        check("keeper near-goal role gate", entry, miss, 0, 4, 235,
            [](Cpu&) { active.first_role = 0; });
        check("mild keeper contact success", entry, save, 1, 4, 310,
            [](Cpu&) { active.mutate_choice = true; });
        check("negative mild keeper contact rejected", entry, save, 1, 4, 310, [](Cpu& c) {
            c.put8(keeper + 0x17, -8);
            c.put16(pool + 6, 3);
        });
        check("mild keeper contact role gate", entry, save, 1, 4, 310,
            [](Cpu&) { active.first_role = 0; });
        check("strong keeper contact role gate", entry, save, 3, 4, 750,
            [](Cpu&) { active.first_role = 0; });
        for (int sample : {-1, -6, -7}) {
            check("negative strong keeper contact", entry, save, 3, 4, 750, [=](Cpu& c) {
                active.goal_line.seed_after_tally = sample;
                c.put16(pool + 6, -sample);
            });
        }
        check("over-line keeper save with invalid side", entry, save, 2, 0, 310,
            [](Cpu& c) { c.put8(player + 0x27, 2); });
        for (int detail : {0, 2}) for (unsigned role : {0u, 1u}) {
            check("goal keeper detail and role", entry, goal, 1, 4, 310, [=](Cpu& c) {
                active.goal_line.keeper_detail = detail;
                active.first_role = role;
                c.put16(pool + 6, 0);
            });
        }
        if (entry == own_goal_line) {
            check("lower goal with self as keeper", entry, goal, 1, 0, 310, [](Cpu& c) {
                active.last_is_player = true;
                c.put8(stack - 0xd0 + 0x38, 1);
                c.put8(stack - 0xd0 + 0x39, 1);
                c.put32(player + 0x6c, data + 0xc000);
            });
        }
        check("negative keeper pressure with no recovery floor", entry, goal, 1, 0, 310, [](Cpu& c) {
            c.put8(data + 0xc064, -4);
            c.put8(player + 0xf, 0);
            c.put16(pool + 6, 22);
        });
        check("second keeper trait draw zero", entry, goal, 1, 0, 310,
            [](Cpu& c) { c.put8(data + 0xc064, 3); c.put16(pool + 8, 5); });
        check("initial review without keeper role", entry, goal, 1, 0, 310, [](Cpu& c) {
            active.first_role = 0;
            c.put16(pool + 6, 0);
        });
        check("initial review with keeper detail", entry, goal, 1, 0, 310, [](Cpu& c) {
            active.goal_line.keeper_detail = 2;
            c.put8(data + 0xc064, -1);
            c.put16(pool + 6, 0);
        });
        check("negative initial review draw with clear carry", entry, goal, 1, 0, 310, [](Cpu& c) {
            active.goal_line.seed_after_tally = -49;
            c.put16(pool + 6, 0);
        });
        check("pending restart negative margin bound", entry, review, 0, 0, 310,
            [](Cpu&) { active.goal_line.score = 8; });
        check("pending restart level score", entry, review, 2, 0, 310,
            [](Cpu&) { active.goal_line.score = 3; });
        for (const int ticks : {-1, -6, -17, -22}) {
            check("negative game clock", entry, goal, 3, 0, 310,
                [=](Cpu& c) { c.put16(match + 0x7948, ticks); });
        }
        check("extended clock thresholds", entry, goal, 0, 0, 310,
            [](Cpu& c) { c.put16(match + 0x794c, 0); });
        struct Trailing { int score, rival, minute; bool tied_leg; };
        constexpr Trailing trailing[]{
            {0, 4, 85, false}, {0, 3, 50, false}, {0, 3, 62, false}, {0, 3, 72, false},
            {0, 3, 86, false}, {0, 2, 62, false}, {1, 3, 72, false}, {1, 3, 90, true}
        };
        for (const auto [score, rival, minute, tied_leg] : trailing) {
            check("trailing side", entry, goal, 1, 0, 310, [=](Cpu& c) {
                active.goal_line.score = score;
                active.goal_line.rival_score = rival;
                active.goal_line.tied_leg = tied_leg;
                c.put16(match + 0x7948, minute * 11);
            });
        }
        check("early aggregate goal", entry, goal, 0, 0, 310, [](Cpu& c) {
            active.goal_line.tied_leg = true;
            c.put16(match + 0x7948, 65 * 11);
        });
        check("aggregate goal during extended time", entry, goal, 0, 0, 310, [](Cpu& c) {
            active.goal_line.tied_leg = true;
            c.put16(match + 0x7948, 95 * 11);
            c.put16(match + 0x794c, 0);
        });
        check("late goal without club commentary", entry, goal, 3, 0, 310, [=](Cpu& c) {
            active.first_role = 0;
            for (unsigned slot = 3; slot <= 8; ++slot) c.put16(pool + slot * 2, 0);
            c.put16(pool + (entry == opposite_goal_line ? 14 : 12), 50);
            c.put8(data + 0xd119, 7);
        });
        for (int status : {0, 7}) {
            check("celebration club ineligible", entry, goal, 9, 0, 310, [=](Cpu& c) {
                if (status) c.put8(data + 0xd119, status);
                else {
                    c.put32(data + 0xb02c, 0);
                    c.put32(data + 0xb12c, 0);
                }
            });
        }
    }
    std::cout << checked << " goal-line branch cases, " << failed << " with differences\n";
    return failed != 0;
}

int check_goal_line_production(const State& base) {
    constexpr unsigned results = RoutineChecker::result_r3 | RoutineChecker::result_f1;
    const auto production = +[](State& s, std::uint32_t entry) {
        reference_mode = false;
        s.pc = entry;
        s.budget = 64;
        run_until(s, sentinel);
        require_return(s);
    };
    const auto corrupted = +[](State& s, std::uint32_t entry) {
        reference_mode = false;
        s.pc = entry;
        s.budget = 64;
        run_until(s, sentinel);
        Cpu c(s);
        c.put8(match + 0x7928, c.u8(match + 0x7928) ^ 1);
        require_return(s);
    };
    for (const auto entry : {opposite_goal_line, own_goal_line}) {
        for (const auto path : {GoalLinePath::miss, GoalLinePath::goal}) {
            const bool goal = path == GoalLinePath::goal;
            active = {entry, 1, 4, goal ? 0 : 4};
            auto state = cm::semantic_io_checks::clone(base);
            Cpu c(*state);
            initialize_case(c);
            initialize_goal_line(c, path, goal ? 1 : 0, active.row, goal ? 310 : 239);
            const auto differences = compare_routine_call(*state, entry, original_goal_line, production, results);
            if (!differences.empty()) {
                std::cerr << hex_address(entry) << " production goal=" << goal << '\n';
                for (const auto& line : differences) std::cerr << line << '\n';
                return 1;
            }
            if (goal) {
                const auto control = compare_routine_call(*state, entry, original_goal_line, corrupted, results);
                cm::semantic_io_checks::require(std::ranges::any_of(control, [](const auto& line) {
                    return line.starts_with("byte 60017928 ");
                }), "Goal-line publication corruption escaped comparison");
            }
        }
    }
    std::cout << "4 goal-line production cases match; both publication corruption controls detected\n";
    return 0;
}
}
