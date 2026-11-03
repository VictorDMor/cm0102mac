struct ClausesDrawInput {
    unsigned supported = 0;
    std::array<int, 5> draft{}, proposal{};
    bool eligible = false, compact = false, missing_club = false, no_presets = false;
    int amount = 20, floor = 30;
};

void clauses_draw_case(Cpu& c, const ClausesDrawInput& in, std::uint32_t draft = BonusFixture::draft,
                       std::uint32_t proposal = BonusFixture::proposal, std::uint32_t caller = stack,
                       std::uint32_t layout = BonusFixture::layout) {
    using F = BonusFixture;
    constexpr unsigned top = 140, bottom = 545, height = 11, rows = (bottom - top) / height,
                       flags_grid = 51, amount_grid = 52, lower = 61;
    const auto presets_grid = in.no_presets ? 0x1234ffffu : 53;
    constexpr auto preset_data = fixture + 0xb000;
    constexpr std::array values{5, 10, 20, 50, 100, 500, 1000, 2000, 50'000'000};
    if (caller == stack) {
        F::setup(c);
        c.set({{3, draft}, {4, proposal}, {5, staff}, {6, top}, {7, bottom}});
        c.put32(draft + 4, in.missing_club ? 0xffffffff : F::club_id);
        c.put32(draft + 0x24, in.amount);
        for (unsigned i = 0; i < 5; ++i) {
            c.put8(draft + 0x1c + i, in.draft[i]);
            c.put8(proposal + 0x1c + i, in.proposal[i]);
        }
        c.put32(c.r(2) - 0x5704, preset_data);
        for (unsigned i = 0; i < values.size(); ++i) c.put32(preset_data + 4 * i, values[i]);
    }
    const auto sp = caller - 0x110, text = c.toc(-0x7bb8), strings = c.toc(-0x56cc), box = sp + 0x7c;
    const auto color = [&](int slot) { return c.u16(c.toc(slot)); };
    const auto ink = color(-0x7bc8), muted = color(-0x7bc4), accent = color(-0x7bf4);
    const auto action_ink = color(-0x7bdc), secondary = color(-0x7bcc);
    const auto club = c.u32(c.toc(-0x7be8)) + c.u32(draft + 4) * 0x248;
    const auto amount = std::int32_t(c.u32(draft + 0x24));
    unsigned row = 0;
    const auto cell = [&](std::uint32_t back, unsigned grid, unsigned column, unsigned type, unsigned decoration,
                          unsigned style, unsigned color, std::uint32_t caption, unsigned action = 0,
                          unsigned value = 0, bool clickable = false, unsigned handle = 0) {
        expect(clickable ? 0x107444e0 : 0x107445a0, back, {screens, grid, column, row, type, decoration, style, 3}, handle);
        expected.back().stack_args = {color, caption};
        if (clickable) expected.back().stack_args.insert(expected.back().stack_args.end(), {action, value});
    };
    const auto heading = [&](std::uint32_t back, unsigned caption, unsigned color) {
        expect(0x10727890, back, {text, strings + caption});
        expect(0x1074b830, back + 0xc, {box, text});
        expect(0x1074bcd0, back + 0x38, {box, flags_grid, 120, 770, sp + 0x78, 3, color, 0});
        expected.back().stack_args = {0};
        ++row;
    };
    const auto draw_flag = [&](unsigned i) {
        if (in.missing_club) return;
        const bool extra = i == 4, available = (in.supported >> i) & 1;
        expect(0x10380400, extra ? 0x1009d0ec : 0x1009cac4, {staff, club, i}, available ? 0xff : 0x100);
        if (!available) return;
        if (in.proposal[i] && in.eligible)
            cell(extra ? 0x1009d144 : 0x1009cb1c, flags_grid, 1, 1, 0, 12, accent, strings + 0x120b);
        const bool selected = in.draft[i] == 1;
        expect(0x10380300, extra ? (selected ? 0x1009d164 : 0x1009d1b4) : (selected ? 0x1009cb3c : 0x1009cb90), {i, text});
        cell(extra ? (selected ? 0x1009d1a4 : 0x1009d1f4) : (selected ? 0x1009cb7c : 0x1009cbd0),
             flags_grid, 2, 1, 0, 1, selected ? ink : muted, text, selected ? 0x13 : 0x12, i, true);
        ++row;
    };
    expect(0x1074b6e0, 0x1009c8f4, {box, 0});
    expect(0x10089ea0, 0x1009c964, {c.u32(c.toc(-0x7b7c)), proposal}, in.eligible ? 1 : 0x100);
    expect(0x1076d470, 0x1009c970, {3}, height);
    expect(0x107198c0, 0x1009c9e0, {layout, 110, top, 780, bottom, 3, sp + 0x74, rows}, flags_grid);
    expected.back().stack_args = {0, 2, ink, 0xffffffff};
    expect(0x107198c0, 0x1009ca34, {layout, 110, top, 780, bottom, 9, sp + 0x84, rows}, amount_grid);
    expected.back().stack_args = {0, 1, ink, 0xffffffff};
    heading(0x1009ca44, 0x122c, ink);
    row += 2;
    bool active = false;
    for (unsigned i = 0; i < 4; ++i) {
        draw_flag(i);
        active |= !in.missing_club && ((in.supported >> i) & 1) && in.draft[i] == 1;
    }
    ++row;
    expect(0x10727890, active ? 0x1009cc14 : 0x1009ceec, {text, strings + 0x1251});
    cell(active ? 0x1009cc44 : 0x1009cf1c, flags_grid, 2, 1, 0, 1, active ? ink : muted, text);
    const auto minimum = in.draft[1] == 1 ? in.floor : 5;
    if (active) {
        if (in.draft[1] == 1) expect(read_history, 0x1009cc5c, {screens, 0x17}, minimum);
        const bool decrease = amount > minimum, increase = amount < 50'000'000;
        cell(decrease ? 0x1009ccfc : 0x1009ccac, amount_grid, 3, 0x30, action_ink, decrease ? 12 : 44,
             ink, strings + 0xe1d, 0x28, 0, decrease, lower);
        history(0x1009cd14, 0xe, lower);
        bonus_money(text, sp + 0x6c, amount, in.compact, {0x1009cd20, 0x1009cd38, 0x1009cd40, 0x1009cd54, 0x1009cd6c}, 6);
        cell(0x1009cda8, amount_grid, 4, 1, 0, 12, ink, text, 0, 0, true, lower + 1);
        expect(0x107198c0, 0x1009cdf8, {layout, 0, 0, 0, 0, 0, 0, 0}, presets_grid);
        expected.back().stack_args = {0, 0x130, secondary, lower + 1};
        cell(increase ? 0x1009ce94 : 0x1009ce48, amount_grid, 5, 0x30, action_ink, increase ? 12 : 44,
             ink, strings + 0xe1f, 0x27, 0, increase);
        cell(0x1009cedc, amount_grid, 6, 0x30, action_ink, 12, ink, strings + 0xe21, 0x29, 0, true);
    } else {
        cell(0x1009cf54, amount_grid, 3, 0x30, action_ink, 44, ink, strings + 0xe1d);
        bonus_money(text, sp + 0x68, amount, in.compact, {0x1009cf68, 0x1009cf80, 0x1009cf88, 0x1009cf9c, 0x1009cfb4}, 6);
        cell(0x1009cfe4, amount_grid, 4, 1, 0, 44, ink, text);
        cell(0x1009d01c, amount_grid, 5, 0x30, action_ink, 44, ink, strings + 0xe1f);
        cell(0x1009d054, amount_grid, 6, 0x30, action_ink, 44, ink, strings + 0xe21);
    }
    row += 2;
    heading(0x1009d06c, 0x1264, ink);
    row += 2;
    draw_flag(4);
    if (in.eligible) {
        row = rows - 1;
        heading(0x1009d23c, 0x1212, accent);
    }
    if (active && !in.no_presets) {
        for (unsigned i = 0; i < values.size(); ++i) {
            const bool available = values[i] >= minimum;
            expect(0x1000a330, 0x1009d2a0, {sp + 0x70, i, sp + 0x90, 9});
            bonus_money(text, sp + 0x70, values[i], false, {0x1009d2ac, 0x1009d2c4}, 5);
            expect(0x10719740, available ? 0x1009d3d4 : 0x1009d350, {layout, 2, 0, 0, 0, 0, 0, 0});
            expected.back().stack_args = {16, secondary, color(-0x7bd0), available ? 1u : 33u, 1,
                color(-0x7bd4), text, 0, available ? 0x26u : 0u, available ? std::uint32_t(values[i]) : 0u, presets_grid};
        }
    }
    expect(0x1074b7c0, 0x1009d3f0, {box, 0xffffffff});
}

struct ClausesCommandInput {
    unsigned action = 0x26, value = 0x00345678, flag = 0;
    bool fallback = false, no_control = false, missing_draft = false, compact = false;
    int before = 20, after = 5, offer = 15, minimum = 30, second_minimum = 31;
};

void clauses_command_case(Cpu& c, const ClausesCommandInput& in) {
    using F = BonusFixture;
    F::setup(c);
    const auto sp = stack - 0x90;
    const auto control = in.no_control ? 0x7654ffffu : 0x76540003u;
    const auto widget = F::layout + 0xc0000 + 3 * 0x17c;
    const auto value = in.no_control ? 0u : in.value;
    constexpr auto amount = F::draft + 0x24;
    c.set(3, control);
    c.put32(widget - 0x5794, in.fallback ? 0 : 0x77ab0000u | in.action);
    c.put32(widget - 0x5754, in.value);
    c.put16(screens + 0x260000 - 0x7e62, in.fallback ? in.action : 0);
    c.put32(amount, in.before);
    c.put32(F::proposal + 0x24, in.offer);
    c.put32(F::draft + 0x1c, 0x02030201);
    c.put8(F::draft + 0x1d, in.flag);
    c.put8(F::draft + 0x20, 2);
    expect(read_history, 0x1009d440, {screens, 2}, F::proposal);
    expect(0x101487d0, 0x1009d454, {F::drafts, 0}, in.missing_draft ? 0 : F::draft);
    if (in.missing_draft) {
        const auto strings = c.toc(-0x56cc), buffer = c.toc(-0x7ba8);
        expect(0x10739890, 0x1009d474, {strings + 0x87, 0, 0, sp + 0x48, sp + 0x40});
        expect(0x1077b720, 0x1009d494, {buffer, strings + 0x9c, c.toc(-0x5b20), sp + 0x48, sp + 0x40, 0x1098});
        expect(0x10769b60, 0x1009d4a4, {strings + 0xa9, buffer, 0});
        expect(0x1076f7d0, 0x1009d4b8, {0xffffffff});
        fixture_instruction_budget = 2;
        return;
    }
    expected_result = 0;
    const auto store = [&](int result) { expected_record_word = std::pair{amount, std::uint32_t(result)}; };
    constexpr auto redraw = std::uint32_t(-11);
    switch (in.action) {
    case 0x26:
        store(value);
        expected_result = redraw;
        break;
    case 0x12: {
        const auto index = std::int8_t(value);
        if (index == 1) expect(read_history, 0x1009d5bc, {screens, 0x17}, in.minimum);
        store(index == 1 ? std::max(in.before, in.minimum) : in.before);
        expected_record_byte = std::pair{F::draft + 0x1c + index, std::uint8_t(1)};
        fixture_instruction_budget = index == 1 ? 15 : 10;
        expected_result = redraw;
        break;
    }
    case 0x13:
        expected_record_byte = std::pair{F::draft + 0x1c + std::int8_t(value), std::uint8_t(0)};
        fixture_instruction_budget = 5;
        expected_result = redraw;
        break;
    case 0x29: {
        auto restored = in.offer;
        if (in.flag == 1) {
            expect(read_history, 0x1009da68, {screens, 0x17}, in.minimum);
            if (restored < in.minimum) {
                expect(read_history, 0x1009da80, {screens, 0x17}, in.second_minimum);
                restored = in.second_minimum;
            }
        }
        store(restored);
        expected_result = redraw;
        break;
    }
    case 0x27:
    case 0x28: {
        const bool increase = in.action == 0x27;
        const auto local = sp + (increase ? 0x38 : 0x3c);
        constexpr auto id = 0xffffff80u;
        expect(increase ? 0x1000a060 : 0x1000a170, increase ? 0x1009d894 : 0x1009d6b4, {local, 4}, in.after);
        expected.back().pointed_result = in.after;
        expected.back().pointed_word = in.before;
        expect(read_history, increase ? 0x1009d8a8 : 0x1009d6c8, {screens, 0xe}, 0x1234ff80);
        if (!increase && in.flag == 1) expect(read_history, 0x1009d6e4, {screens, 0x17}, in.minimum);
        const auto configure = [&](std::uint32_t back, unsigned control, bool enabled, unsigned action) {
            expect(0x10740620, back, {screens, control, enabled ? 2u : 6u});
            expect(0x10741450, back + 0x10, {screens, control, enabled ? 12u : 44u});
            expect(0x10742100, back + 0x20, {screens, control, enabled ? action : 0u});
            expect(0x10742420, back + 0x30, {screens, control, 0});
        };
        const bool reduce = increase || in.after > (in.flag == 1 ? in.minimum : 5);
        configure(increase ? 0x1009d8bc : reduce ? 0x1009d74c : 0x1009d708, id, reduce, 0x28);
        bonus_money(F::text, local, in.after, in.compact,
                    increase ? BonusMoney{0x1009d8f8, 0x1009d910, 0x1009d918, 0x1009d92c, 0x1009d944}
                             : BonusMoney{0x1009d788, 0x1009d7a0, 0x1009d7a8, 0x1009d7bc, 0x1009d7d4}, 6);
        expect(0x10741da0, increase ? 0x1009d954 : 0x1009d7e4, {screens, id + 1, F::text});
        const bool raise = !increase || in.after < 50'000'000;
        configure(increase ? (raise ? 0x1009d9bc : 0x1009d978) : 0x1009d7f4, id + 2, raise, 0x27);
        expect(0x10742ec0, increase ? 0x1009d9fc : 0x1009d834, {screens, 0, 0});
        store(in.after);
        break;
    }
    }
}

constexpr std::array clause_draw_inputs{
    ClausesDrawInput{},
    ClausesDrawInput{.supported = 0x1b, .draft = {1, 1, 0, 2, 1}, .proposal = {1, 0, 1, 1, 1},
                     .eligible = true, .compact = true},
    ClausesDrawInput{.supported = 0x15, .draft = {0, 0, 1, 0, 0}, .proposal = {1, 1, 1, 1, 1}, .amount = 50'000'000},
    ClausesDrawInput{.supported = 0x13, .draft = {1, 0, 0, 0, 1}, .eligible = true, .no_presets = true},
    ClausesDrawInput{.supported = 0x10, .draft = {0, 0, 0, 0, 1}, .proposal = {0, 0, 0, 0, 1},
                     .eligible = true, .compact = true},
    ClausesDrawInput{.supported = 0x10, .proposal = {0, 0, 0, 0, 1}, .eligible = true},
    ClausesDrawInput{.missing_club = true},
};
constexpr std::array clause_command_inputs{
    ClausesCommandInput{},
    ClausesCommandInput{.fallback = true},
    ClausesCommandInput{.fallback = true, .no_control = true},
    ClausesCommandInput{.action = 0x12, .value = 1},
    ClausesCommandInput{.action = 0x12, .value = 0xaaaa0001, .before = 45},
    ClausesCommandInput{.action = 0x12, .value = 2, .flag = 1},
    ClausesCommandInput{.action = 0x13, .value = 4},
    ClausesCommandInput{.action = 0x13, .fallback = true, .no_control = true},
    ClausesCommandInput{.action = 0x28},
    ClausesCommandInput{.action = 0x28, .flag = 2, .after = 6},
    ClausesCommandInput{.action = 0x28, .flag = 1, .compact = true, .after = 31},
    ClausesCommandInput{.action = 0x28, .flag = 1, .after = 29},
    ClausesCommandInput{.action = 0x27, .after = 49'999'999},
    ClausesCommandInput{.action = 0x27, .compact = true, .after = 50'000'001},
    ClausesCommandInput{.action = 0x29},
    ClausesCommandInput{.action = 0x29, .flag = 1},
    ClausesCommandInput{.action = 0x29, .flag = 1, .offer = 35},
    ClausesCommandInput{.action = 0x70},
    ClausesCommandInput{.missing_draft = true},
};
