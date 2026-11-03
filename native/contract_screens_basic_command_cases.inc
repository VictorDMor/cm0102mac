struct BasicCommandInput {
    unsigned action = 0x70, link = 0, value = 1234, flags = 8, status = 1, age = 23;
    bool fallback = false, no_control = false, missing = false, compact = false, profile = false;
    int fee = 700, wage = 100, adjusted = 120, next = 130, cap = 150;
    int role = 2, first_employer = -1, second_employer = -1, salary = 520;
    int start_month = 5, start_year = 1999, month = 6, year = 1999, years_allowed = 4;
    int proposal_month = 7, proposal_year = 2001, draft_month = 5, draft_year = 1999;
};

struct BasicCommandFixture {
    using F = BonusFixture;
    static constexpr auto base = fixture + 0xb000, calendar = base, instance = base + 0x10,
        link = base + 0x50, offer = base + 0x60, current = base + 0xa0,
        different = base + 0xb0, profile = base + 0xc0, table = base + 0xe0,
        cached = base + 0xe4, settings = base + 0xe8;
    static constexpr auto sp = stack - 0xf0, wage_widget = 0xffffff86u, duration_widget = 0xffffff70u, fee_widget = 0x82u;
    static constexpr auto proposed_fee = 400u, proposed_wage = 180u;

    static void configure(std::uint32_t back, std::uint32_t widget, unsigned state, unsigned style, unsigned action,
                          std::uint32_t state_call = 0x10740620) {
        expect(state_call, back, {screens, widget, state});
        expect(0x10741450, back + 0x10, {screens, widget, style});
        expect(0x10742100, back + 0x20, {screens, widget, action});
    }

    static void format_wage(std::uint32_t back, std::uint32_t widget, std::uint32_t local, int amount, bool compact) {
        const auto number = [&](std::uint32_t site, unsigned digits) {
            expect(0x10009f20, site, {local, digits}, 0x12345678);
            expected.back().pointed_word = amount;
            expected.back().second_result = 0x56780000;
            expect(0x1000b410, site + 0x18, {F::text, 200, 0x12345678, 0x56780000, digits});
        };
        number(back, 6);
        expect(0x1077dbd0, back + 0x20, {F::text}, compact ? 9 : 8);
        if (compact) number(back + 0x34, 5);
        expect(0x10741da0, back + 0x5c, {screens, widget + 1, F::text});
    }
};

void basic_command_case(Cpu& c, const BasicCommandInput& in) {
    using B = BasicCommandFixture;
    using F = BonusFixture;
    constexpr unsigned offer_id = 0xf3, proposal_flags = 0x16;
    F::setup(c);
    c.put32(c.r(2) - 0x7b80, B::calendar);
    c.put32(c.r(2) - 0x7990, B::settings);
    c.put32(c.r(2) - 0x7b78, B::cached);
    c.put32(screens + 0x2c0 + 4, B::instance);
    c.put32(B::instance + 0x24, F::club);
    c.put32(B::link, in.link > 1 ? B::offer : 0);
    c.put32(B::offer + 0x28, offer_id);
    c.put32(staff + 0x14, in.profile ? B::profile : 0);
    c.put16(staff + 0x5c, in.age);
    c.put16(B::calendar + 2, in.start_year);
    c.put8(B::settings, in.years_allowed);
    c.put32(B::current + 4, F::club_id);
    c.put32(B::different + 4, F::club_id + 1);
    c.put32(B::cached, 0x12345678);
    c.put32(F::draft + 8, in.wage);
    c.put32(F::draft + 0xc, in.fee);
    c.put16(F::draft + 0x2a, in.draft_year);
    c.put8(F::draft + 0x38, in.flags);
    c.put8(F::draft + 0x60, 0xb0 | in.status);
    c.put32(F::proposal + 8, B::proposed_wage);
    c.put32(F::proposal + 0xc, B::proposed_fee);
    c.put16(F::proposal + 0x32, in.proposal_year);
    c.put8(F::proposal + 0x38, proposal_flags);
    const auto control = in.no_control ? 0x4567ffffu : 0x45670003u;
    const auto cell = F::layout + 0xc0000 + 3 * 0x17c;
    c.set(3, control);
    c.put32(cell - 0x5794, in.fallback ? 0u : 0x34ff0000u | in.action);
    c.put32(cell - 0x5754, in.value);
    c.put16(screens + 0x260000 - 0x7e62, in.fallback ? in.action : 0);
    const auto strings = c.toc(-0x56cc);
    expect(0x101487d0, 0x1009a488, {F::drafts, 0}, in.missing ? 0 : F::draft);
    if (in.missing) {
        expect(0x10739890, 0x1009a4a8, {strings + 0x87, 0, 0, B::sp + 0x60, B::sp + 0x58});
        expect(0x1077b720, 0x1009a4c8, {c.toc(-0x7ba8), strings + 0x9c, c.toc(-0x5b20), B::sp + 0x60, B::sp + 0x58, 0xb2b});
        expect(0x10769b60, 0x1009a4d8, {strings + 0xa9, c.toc(-0x7ba8), 0});
        expect(0x1076f7d0, 0x1009a4ec, {0xffffffff});
        fixture_instruction_budget = 2;
        return;
    }
    const auto read = [](std::uint32_t back, unsigned slot, std::uint32_t value) {
        expect(read_history, back, {screens, slot}, value);
    };
    const auto refresh = [](std::uint32_t back) { expect(0x10742ec0, back, {screens, 0, 0}); };
    const auto money = [](std::uint32_t target, std::uint32_t back, unsigned local, unsigned limit, int before, int after) {
        expect(target, back, {B::sp + local, limit}, after);
        expected.back().pointed_word = before;
        expected.back().pointed_result = after;
    };
    const auto date = [&](std::uint32_t month_back, std::uint32_t caption, std::uint32_t widget, int month, int year) {
        expect(0x107188a0, month_back, {std::uint32_t(month)}, 0x11223344);
        expect(0x10727890, caption, {F::text, strings + 0xe66, 0x11223344, std::uint32_t(year)});
        expect(0x10741da0, widget, {screens, B::duration_widget + 1, F::text});
    };
    const auto write_date = [&](std::uint32_t month_back, std::uint32_t year_back, int month, int year) {
        history(month_back, 0xb, std::uint32_t(std::int8_t(month)));
        history(year_back, 0xc, std::uint16_t(year));
    };
    const auto fee_change = [&](std::uint32_t back, bool enabled, bool increase) {
        B::configure(back, increase ? B::fee_widget + 2 : B::fee_widget, enabled ? 2 : 6, enabled ? 12 : 44,
                     enabled ? (increase ? 4 : 5) : 0, 0x10742100);
    };
    read(0x1009a4f8, 0, F::person_id);
    read(0x1009a52c, 2, F::proposal);
    read(0x1009a540, 0x15, in.cap);
    read(0x1009a560, 3, in.link ? B::link : 0);
    read(0x1009a584, 9, 0x98760000 | std::uint8_t(in.start_month));
    read(0x1009a594, 0xa, 0x87650000 | std::uint16_t(in.start_year));
    read(0x1009a5a4, 0xb, 0x76540000 | std::uint8_t(in.month));
    read(0x1009a5b4, 0xc, 0x65430000 | std::uint16_t(in.year));
    expected_result = 0;
    constexpr auto redraw = std::uint32_t(-11);
    const auto selected = in.no_control ? 0u : in.value;
    const auto fee_word = [&](unsigned value) { expected_record_word = std::pair{F::draft + 0xc, value}; };
    const auto wage_word = [&](unsigned value) { expected_record_word = std::pair{F::draft + 8, value}; };
    const auto length_byte = [&](unsigned value) { expected_record_byte = std::pair{F::draft + 0x38, std::uint8_t(value)}; };
    const auto kind = std::int8_t(in.flags) >> 2;
    switch (in.action) {
    case 4: {
        read(0x1009a660, 8, in.role);
        expect(0x10380560, 0x1009a668, {F::draft}, in.adjusted);
        read(0x1009a674, 0x18, in.cap);
        read(0x1009a684, 0xf, 0x55660000 | B::fee_widget);
        B::configure(0x1009a698, B::fee_widget, 2, 12, 5);
        expect(0x10372590, 0x1009a6d4, {F::draft, F::text, 0, 2});
        expect(0x10741da0, 0x1009a6e4, {screens, B::fee_widget + 1, F::text});
        money(0x1000a060, 0x1009a6f0, 0x40, 8, in.adjusted, in.next);
        const auto parse = [](std::uint32_t back, unsigned local, int value) {
            const auto expanded = std::uint64_t(std::int64_t(value) * 1'000'000'000);
            expect(0x10009f20, back, {B::sp + local, 8}, expanded >> 32);
            expected.back().pointed_word = value;
            expected.back().second_result = std::uint32_t(expanded);
        };
        parse(0x1009a6fc, 0x44, in.cap);
        parse(0x1009a710, 0x40, in.next);
        fee_change(in.next <= in.cap ? 0x1009a770 : 0x1009a73c, in.next <= in.cap, true);
        refresh(0x1009a7a0);
        fee_word(in.adjusted);
        break;
    }
    case 5:
        expect(0x10380630, 0x1009a7b0, {F::draft}, in.adjusted);
        read(0x1009a7bc, 0xf, 0x77660000 | B::fee_widget);
        fee_change(in.adjusted > 100 ? 0x1009a810 : 0x1009a7dc, in.adjusted > 100, false);
        expect(0x10372590, 0x1009a844, {F::draft, F::text, 0, 2});
        expect(0x10741da0, 0x1009a854, {screens, B::fee_widget + 1, F::text});
        B::configure(0x1009a864, B::fee_widget + 2, 2, 12, 4);
        refresh(0x1009a894);
        fee_word(in.adjusted);
        break;
    case 6:
    case 7:
    case 8:
        fee_word(in.action == 6 ? B::proposed_fee : selected);
        expected_result = redraw;
        break;
    case 9: {
        auto month = in.month, year = in.year;
        if (kind != 1) {
            if (year > in.start_year + 1 || (year == in.start_year + 1 && month >= in.start_month)) ++year;
            else if (++month > 11) { month = 0; ++year; }
            write_date(0x1009ae18, 0x1009ae2c, month, year);
        }
        read(0x1009ae38, 0x11, B::duration_widget);
        B::configure(0x1009ae4c, B::duration_widget, 2, 12, 0xa);
        date(0x1009ae74, 0x1009ae8c, 0x1009ae9c, month, year);
        const bool enabled = year - in.years_allowed < in.start_year;
        B::configure(enabled ? 0x1009aefc : 0x1009aec8, B::duration_widget + 2, enabled ? 2 : 6, enabled ? 12 : 44, enabled ? 9 : 0);
        refresh(0x1009af2c);
        length_byte((in.flags & 3) | 8);
        break;
    }
    case 0xa: {
        auto month = in.month, year = in.year;
        if (year == in.start_year + 1) expect(0x10718360, 0x1009af50, {B::calendar}, std::uint32_t(in.start_month));
        if (year > in.start_year + 1 || (year == in.start_year + 1 && month > in.start_month)) --year;
        else if (--month < 0) { month = 11; --year; }
        write_date(0x1009af9c, 0x1009afb0, month, year);
        read(0x1009afbc, 0x11, B::duration_widget);
        auto prior_month = month, prior_year = year;
        if (year == in.draft_year + 1)
            expect(0x10718360, 0x1009afe4, {F::draft + 0x28}, std::uint32_t(in.draft_month));
        if (year > in.draft_year + 1 || (year == in.draft_year + 1 && month > in.draft_month)) --prior_year;
        else if (--prior_month < 0) { prior_month = 11; --prior_year; }
        if (prior_year < in.start_year || (prior_year == in.start_year && prior_month <= in.start_month)) {
            B::configure(0x1009b054, B::duration_widget, 6, 44, 0);
        } else {
            bool toggle = false;
            if (year == in.draft_year) {
                expect(0x10718360, 0x1009b090, {F::draft + 0x28}, std::uint32_t(in.draft_month));
                toggle = month == in.draft_month + 1;
            } else if (year == in.draft_year + 1 && month == 0) {
                expect(0x10718360, 0x1009b0c4, {F::draft + 0x28}, std::uint32_t(in.draft_month));
                toggle = in.draft_month == 11;
            }
            if (toggle) B::configure(0x1009b0e0, B::duration_widget, 2, 12, 0x1d);
            else B::configure(kind == 1 ? 0x1009b12c : 0x1009b160, B::duration_widget, kind == 1 ? 6 : 2,
                              kind == 1 ? 44 : 12, kind == 1 ? 0 : 0xa);
        }
        date(0x1009b188, 0x1009b19c, 0x1009b1ac, month, year);
        B::configure(0x1009b1bc, B::duration_widget + 2, 2, 12, 9);
        refresh(0x1009b1ec);
        break;
    }
    case 0xb:
        expect(0x10718360, 0x1009b1fc, {F::proposal + 0x30}, std::uint32_t(in.proposal_month));
        write_date(0x1009b210, 0x1009b224, in.proposal_month, in.proposal_year);
        length_byte((in.flags & 3) | (proposal_flags & 0xfc));
        expected_result = redraw;
        break;
    case 0x16:
    case 0x19:
        wage_word(std::min(in.action == 0x16 ? int(B::proposed_wage) : int(selected), in.cap));
        expected_result = redraw;
        break;
    case 0x17:
        money(0x1000a060, 0x1009a948, 0x48, 4, in.wage, in.adjusted);
        read(0x1009a95c, 0x10, B::wage_widget);
        B::configure(0x1009a970, B::wage_widget, 2, 12, 0x18);
        B::format_wage(0x1009a9a4, B::wage_widget, B::sp + 0x3c, in.adjusted, in.compact);
        money(0x1000a060, 0x1009aa0c, 0x3c, 4, in.adjusted, in.next);
        B::configure(in.next <= in.cap ? 0x1009aa58 : 0x1009aa24, B::wage_widget + 2,
                     in.next <= in.cap ? 2 : 6, in.next <= in.cap ? 12 : 44, in.next <= in.cap ? 0x17 : 0, 0x10742100);
        refresh(0x1009aa88);
        wage_word(in.adjusted);
        break;
    case 0x18:
        money(0x1000a170, 0x1009aa9c, 0x48, 4, in.wage, in.adjusted);
        read(0x1009aab0, 0x10, B::wage_widget);
        B::configure(in.adjusted ? 0x1009ab04 : 0x1009aad0, B::wage_widget, in.adjusted ? 2 : 6,
                     in.adjusted ? 12 : 44, in.adjusted ? 0x18 : 0, 0x10742100);
        B::format_wage(0x1009ab38, B::wage_widget, B::sp + 0x38, in.adjusted, in.compact);
        B::configure(0x1009aba4, B::wage_widget + 2, 2, 12, 0x17);
        refresh(0x1009abd4);
        wage_word(in.adjusted);
        break;
    case 0x1a:
    case 0x30:
        history(in.action == 0x1a ? 0x1009b278 : 0x1009b2b4, in.action == 0x1a ? 7 : 0x1c, selected);
        expected_result = redraw;
        break;
    case 0x1c: {
        read(0x1009b2c8, 8, std::uint32_t(in.role));
        auto role = in.role;
        if (in.profile) role = role == 11 ? 15 : 11;
        else {
            do {
                if (++role == 11) role = 0;
                expect(0x1007e4f0, 0x1009b324, {F::contracts, std::uint32_t(role), staff}, role == 3 ? 0x100 : 0x1ff);
            } while (role == 3);
            const bool special = role == 11 || role == 12 || role == 13 || role == 15;
            expect(0x10103510, special ? 0x1009b458 : 0x1009b35c, {F::bank, F::club}, F::budget);
            auto maximum = in.cap;
            if (special) expect(0x100f2680, 0x1009b464, {F::budget, staff, 0xffffffff}, std::uint32_t(in.cap));
            else {
                expect(0x100f4790, 0x1009b364, {F::budget, std::uint32_t(role)}, std::uint32_t(in.cap));
                const auto contract = [](int club) { return club < 0 ? 0u : club == 0 ? B::current : B::different; };
                expect(0x10082f20, 0x1009b390, {F::contracts, staff}, contract(in.first_employer));
                if (in.first_employer >= 0) {
                    expect(0x10082f20, 0x1009b3d0, {F::contracts, staff}, contract(in.second_employer));
                    if (in.second_employer == 0) {
                        expect(0x10084b60, 0x1009b410, {F::contracts, staff}, std::uint32_t(in.salary));
                        maximum = std::max(maximum, in.salary);
                    }
                }
            }
            history(0x1009b4a0, 0x18, std::uint32_t(maximum));
        }
        history(0x1009b4b4, 8, std::uint32_t(role));
        expected_result = redraw;
        break;
    }
    case 0x1d: {
        read(0x1009abe8, 0x11, B::duration_widget);
        if (kind == 1) {
            B::configure(0x1009ac20, B::duration_widget, 2, 12, 0xa);
            date(0x1009ac48, 0x1009ac5c, 0x1009ac6c, in.month, in.year);
            const bool enabled = in.year - in.years_allowed < in.start_year;
            B::configure(enabled ? 0x1009accc : 0x1009ac98, B::duration_widget + 2,
                         enabled ? 2 : 6, enabled ? 12 : 44, enabled ? 9 : 0);
        } else {
            B::configure(0x1009ad0c, B::duration_widget, 3, 44, 0);
            expect(0x10727890, 0x1009ad38, {F::text, strings + 0xe57});
            expect(0x10741da0, 0x1009ad48, {screens, B::duration_widget + 1, F::text});
            B::configure(0x1009ad58, B::duration_widget + 2, 2, 12, 9);
        }
        length_byte((in.flags & 3) | (kind == 1 ? 8 : 4));
        refresh(0x1009ad88);
        break;
    }
    case 0x1e:
        length_byte(in.flags ^ 1);
        expected_result = redraw;
        break;
    case 0x1f: {
        unsigned next = in.status;
        if (in.status == 1) next = std::int8_t(in.age) < 24 ? 5 : 4;
        else if (in.status >= 2 && in.status <= 5) --next;
        expect(0x10374f50, 0x1009b5b0, {F::proposal, staff, F::club, in.link > 1 ? offer_id : 0xffffffff,
                                       next, 0xb, B::sp + 0x54, B::sp + 0x50});
        history(0x1009b5c4, 0x19, 337);
        history(0x1009b5f0, 0x1a, std::uint32_t(std::int32_t(2 * c.f64(c.toc(-0x56d0) + 0x30))));
        expected_record_byte = std::pair{F::draft + 0x60, std::uint8_t(0xb0 | next)};
        expected_result = redraw;
        break;
    }
    }
}

constexpr std::array basic_command_inputs{
    BasicCommandInput{},
    BasicCommandInput{.action = 0, .fallback = true},
    BasicCommandInput{.action = 0x21, .link = 1},
    BasicCommandInput{.action = 4, .next = 150},
    BasicCommandInput{.action = 4, .next = 151},
    BasicCommandInput{.action = 4, .next = -10, .cap = -1},
    BasicCommandInput{.action = 5},
    BasicCommandInput{.action = 5, .adjusted = 100},
    BasicCommandInput{.action = 6},
    BasicCommandInput{.action = 7, .value = 0xfedc3456},
    BasicCommandInput{.action = 8, .fallback = true, .no_control = true},
    BasicCommandInput{.action = 9, .flags = 4},
    BasicCommandInput{.action = 9, .month = 11},
    BasicCommandInput{.action = 9, .month = 7, .year = 2000},
    BasicCommandInput{.action = 9, .month = 4, .year = 2000},
    BasicCommandInput{.action = 9, .month = 1, .year = 2002, .years_allowed = 1},
    BasicCommandInput{.action = 0xa},
    BasicCommandInput{.action = 0xa, .flags = 4, .month = 10},
    BasicCommandInput{.action = 0xa, .start_month = 1, .month = 7},
    BasicCommandInput{.action = 0xa, .start_month = 1, .month = 1, .year = 2000, .draft_month = 11},
    BasicCommandInput{.action = 0xa, .start_month = 1, .month = 8, .year = 2000},
    BasicCommandInput{.action = 0xa, .start_month = 1, .month = 0, .year = 2002, .draft_year = 2000},
    BasicCommandInput{.action = 0xa, .month = 0, .year = 1999},
    BasicCommandInput{.action = 0xa, .start_month = 1, .month = 4, .year = 2002, .draft_year = 2000},
    BasicCommandInput{.action = 0xb, .flags = 0xff},
    BasicCommandInput{.action = 0x16},
    BasicCommandInput{.action = 0x16, .cap = 200},
    BasicCommandInput{.action = 0x17, .compact = true},
    BasicCommandInput{.action = 0x17, .cap = 125},
    BasicCommandInput{.action = 0x18, .adjusted = 0},
    BasicCommandInput{.action = 0x18, .compact = true},
    BasicCommandInput{.action = 0x19, .value = 0x12345678},
    BasicCommandInput{.action = 0x19, .fallback = true, .no_control = true},
    BasicCommandInput{.action = 0x1a, .value = 0xabcdef12},
    BasicCommandInput{.action = 0x1a, .fallback = true, .no_control = true},
    BasicCommandInput{.action = 0x1c},
    BasicCommandInput{.action = 0x1c, .first_employer = 0, .second_employer = 0},
    BasicCommandInput{.action = 0x1c, .first_employer = 0, .second_employer = 0, .salary = 80},
    BasicCommandInput{.action = 0x1c, .first_employer = 0, .second_employer = 1},
    BasicCommandInput{.action = 0x1c, .first_employer = 0},
    BasicCommandInput{.action = 0x1c, .role = 10},
    BasicCommandInput{.action = 0x1c, .role = 11},
    BasicCommandInput{.action = 0x1c, .role = 12},
    BasicCommandInput{.action = 0x1c, .role = 14},
    BasicCommandInput{.action = 0x1c, .profile = true, .role = 11},
    BasicCommandInput{.action = 0x1c, .profile = true, .role = 15},
    BasicCommandInput{.action = 0x1d},
    BasicCommandInput{.action = 0x1d, .flags = 4},
    BasicCommandInput{.action = 0x1d, .flags = 4, .year = 2004},
    BasicCommandInput{.action = 0x1e, .flags = 0x80},
    BasicCommandInput{.action = 0x1e, .flags = 0x83},
    BasicCommandInput{.action = 0x1f, .link = 2},
    BasicCommandInput{.action = 0x1f, .age = 24},
    BasicCommandInput{.action = 0x1f, .status = 0},
    BasicCommandInput{.action = 0x1f, .status = 2},
    BasicCommandInput{.action = 0x1f, .status = 3},
    BasicCommandInput{.action = 0x1f, .status = 4},
    BasicCommandInput{.action = 0x1f, .status = 5},
    BasicCommandInput{.action = 0x1f, .status = 6},
    BasicCommandInput{.action = 0x30},
    BasicCommandInput{.action = 0x30, .fallback = true, .no_control = true},
    BasicCommandInput{.missing = true},
};
