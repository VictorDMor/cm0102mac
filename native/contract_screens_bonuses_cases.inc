struct BonusFixture {
    static constexpr std::uint32_t bank = fixture + 0xb400, budget = fixture + 0xb500,
        club = fixture + 0xb600, contracts = fixture + 0xb900, drafts = fixture + 0xba00,
        draft = fixture + 0xbb00, proposal = fixture + 0xbc00, colors = fixture + 0xbd00,
        text = fixture + 0xbe00, layout = fixture + 0x5400;
    static constexpr unsigned person_id = 2, club_id = 3;

    static void bank_holder(Cpu& c) {
        c.put32(c.r(2) - 0x7ab8, fixture + 0x90);
        c.put32(fixture + 0x90, bank);
    }
    static void setup(Cpu& c) {
        bank_holder(c);
        const auto holder = [&](int slot, unsigned offset, std::uint32_t value) {
            c.put32(c.r(2) + slot, fixture + offset);
            c.put32(fixture + offset, value);
        };
        holder(-0x7be8, 0x94, club - club_id * 0x248);
        holder(-0x7be0, 0x98, staff - person_id * 0x70);
        holder(-0x7b7c, 0x9c, contracts);
        holder(-0x7a54, 0xa0, drafts);
        c.put32(c.r(2) - 0x7bb8, text);
        const std::array slots{-0x7bc8, -0x7bdc, -0x7bf4, -0x7bcc, -0x7bd0, -0x7bd4, -0x7bc4};
        for (unsigned i = 0; i < slots.size(); ++i) {
            c.put32(c.r(2) + std::uint32_t(slots[i]), colors + i * 2);
            c.put16(colors + i * 2, 30 + i);
        }
        c.put16(screens + 0x2c58, 1);
        c.put32(screens + 0x2c0 + 0x24, layout);
        c.put32(proposal, person_id);
        c.put32(proposal + 4, club_id);
        c.put32(proposal + 0xc, 660);
        c.put32(draft + 0xc, 550);
    }
};

struct BonusMoney {
    std::uint32_t parse, render, length = 0, short_parse = 0, short_render = 0;
};

void bonus_money(std::uint32_t text, std::uint32_t local, int value, bool compact, BonusMoney sites) {
    const auto line = [&](unsigned precision, std::uint32_t parse, std::uint32_t render) {
        expect(0x10009f20, parse, {local, precision}, 0x11223344);
        expected.back().pointed_word = std::uint32_t(value);
        expected.back().second_result = 0x33445566;
        expect(0x1000b410, render, {text, 2000, 0x11223344, 0x33445566, precision});
    };
    line(10, sites.parse, sites.render);
    if (sites.length) expect(0x1077dbd0, sites.length, {text}, compact ? 9 : 8);
    if (compact && sites.length) line(9, sites.short_parse, sites.short_render);
}

struct BonusesDrawInput {
    std::array<int, 3> draft{-1, -1, -1}, proposal{-1, -1, -1};
    unsigned supported = 0;
    bool eligible = false, compact = false;
    int height = 11, cap = 100;
};

void bonuses_draw_case(Cpu& c, const BonusesDrawInput& in, std::uint32_t draft = BonusFixture::draft,
                       std::uint32_t proposal = BonusFixture::proposal, std::uint32_t caller = stack,
                       std::optional<std::uint32_t> layout_override = {}) {
    if (caller == stack) BonusFixture::setup(c);
    else BonusFixture::bank_holder(c);
    const auto sp = caller - 0x130, text = c.toc(-0x7bb8), strings = c.toc(-0x56cc), box = sp + 0x84;
    const auto color = [&](int slot) { return c.u16(c.toc(slot)); };
    const auto layout = layout_override.value_or(BonusFixture::layout);
    const auto proposal_club = c.u32(c.toc(-0x7be8)) + c.u32(proposal + 4) * 0x248;
    const auto contracts = c.u32(c.toc(-0x7b7c));
    constexpr unsigned top = 140, bottom = 545, grid = 51;
    if (caller == stack) {
        c.set({{3, draft}, {4, proposal}, {5, staff}, {6, top}, {7, bottom}});
        for (unsigned i = 0; i < 3; ++i) {
            c.put32(draft + 0x10 + 4 * i, std::uint32_t(in.draft[i]));
            c.put32(proposal + 0x10 + 4 * i, std::uint32_t(in.proposal[i]));
        }
    }
    const auto cells = [&](std::uint32_t back, unsigned column, unsigned row, unsigned type,
                           unsigned decoration, unsigned style, unsigned ink, std::uint32_t caption,
                           unsigned action = 0, unsigned index = 0, bool clickable = true, unsigned value = 0) {
        expect(clickable ? 0x107444e0 : 0x107445a0, back, {screens, grid, column, row, type, decoration, style, 3}, value);
        expected.back().stack_args = {ink, caption};
        if (clickable) expected.back().stack_args.insert(expected.back().stack_args.end(), {action, index});
    };
    const auto heading = [&](std::uint32_t localize, unsigned label, std::uint32_t assign, std::uint32_t render,
                             unsigned ink) {
        expect(0x10727890, localize, {text, strings + label});
        expect(0x1074b830, assign, {box, text});
        expect(0x1074bcd0, render, {box, grid, 120, 770, sp + 0x78, 3, ink, 0});
        expected.back().stack_args = {0};
    };
    const auto ink = color(-0x7bc8), enabled = color(-0x7bdc), accent = color(-0x7bf4);
    expect(0x1074b6e0, 0x1009b698, {box, 0});
    expect(0x10778300, 0x1009b6b0, {sp + 0x7c, 0xffffffff, 6});
    expect(0x10089ea0, 0x1009b6c0, {contracts, proposal}, in.eligible ? 1 : 0x100);
    expect(0x1076d470, 0x1009b6cc, {3}, std::uint32_t(in.height));
    expect(0x107198c0, 0x1009b730, {layout, 110, top, 780, bottom, 9, sp + 0x8c,
                                   std::uint8_t((bottom - top) / in.height)}, grid);
    expected.back().stack_args = {0, 2, ink, 0xffffffff};
    heading(0x1009b740, 0x11e6, 0x1009b74c, 0x1009b778, ink);
    expect(0x10103510, 0x1009b7a4, {BonusFixture::bank, proposal_club}, BonusFixture::budget);
    expect(0x100f2680, 0x1009b7b0, {BonusFixture::budget, staff, 0xffffffff}, std::uint32_t(in.cap * 10));
    unsigned row = 3;
    std::array<unsigned, 3> grids{};
    for (unsigned i = 0; i < 3; ++i) {
        const bool supported = (in.supported >> i) & 1;
        expect(0x103803b0, 0x1009b7e4, {staff, i}, supported ? 0xff : 0x100);
        if (!supported) continue;
        const auto amount = std::min(in.draft[i], in.cap), offer = in.proposal[i];
        const bool note = in.eligible && offer >= 0 && offer <= in.cap;
        if (note) {
            cells(0x1009b858, 1, row, 1, 0, 12, accent, strings + 0x120b, 0, 0, false);
            bonus_money(text, sp + 0x70, offer, in.compact, {0x1009b86c, 0x1009b884, 0x1009b88c, 0x1009b8a0, 0x1009b8b8});
            expect(0x1077b720, 0x1009b8c8, {sp + 0xac, strings + 0x120d, text});
        }
        if (amount >= 0) {
            expect(0x10380270, 0x1009b8f0, {i, text});
            cells(0x1009b930, 2, row, 1, 0, 1, ink, text, 0xe, i);
            const bool reduce = amount > 5, raise = amount < in.cap;
            cells(reduce ? 0x1009b9b8 : 0x1009b970, 3, row, 0x30, enabled, reduce ? 12 : 44, ink, strings + 0xe1d,
                  0x10, i, reduce, 0x66 + i);
            history(0x1009b9d4, 0x12 + i, 0x66 + i);
            bonus_money(text, sp + 0x6c, amount, in.compact, {0x1009b9e0, 0x1009b9f8, 0x1009ba00, 0x1009ba14, 0x1009ba2c});
            cells(0x1009ba68, 4, row, 1, 0, 12, ink, text, 0, 0, true, 0x56 + i);
            grids[i] = 0x60 + i;
            expect(0x107198c0, 0x1009bab4, {layout, 0, 0, 0, 0, 0, 0, 0}, grids[i]);
            expected.back().stack_args = {0, 0x130, color(-0x7bcc), 0x56 + i};
            cells(raise ? 0x1009bb40 : 0x1009bafc, 5, row, 0x30, enabled, raise ? 12 : 44, ink, strings + 0xe1f,
                  0xf, i, raise);
            cells(0x1009bb80, 6, row, 0x30, enabled, 12, ink, strings + 0xe21, 0x11, i);
            if (note) cells(0x1009bbcc, 8, row, 1, 0, 1, accent, sp + 0xac, 0x11, i);
        } else {
            expect(0x10380270, 0x1009bbdc, {i, text});
            cells(0x1009bc20, 2, row, 1, 0, 1, color(-0x7bc4), text, 0xd, i);
            cells(0x1009bc60, 3, row, 0x30, enabled, 44, ink, strings + 0xe1d, 0xd, i);
            if (note) expect(0x1077dbf0, 0x1009bc78, {text, sp + 0xac});
            else bonus_money(text, sp + 0x68, 55, in.compact, {0x1009bca8, 0x1009bcc0, 0x1009bcc8, 0x1009bcdc, 0x1009bcf4});
            cells(0x1009bd30, 4, row, 1, 0, 44, ink, text, 0xd, i);
            cells(0x1009bd70, 5, row, 0x30, enabled, 44, ink, strings + 0xe1f, 0xd, i);
            cells(0x1009bdb0, 6, row, 0x30, enabled, 44, ink, strings + 0xe21, 0xd, i);
            if (note) cells(0x1009bdfc, 8, row, 1, 0, 33, accent, sp + 0xac, 0xd, i);
        }
        ++row;
    }
    if (in.eligible) heading(0x1009be44, 0x1212, 0x1009be50, 0x1009be7c, accent);
    for (unsigned i = 0; i < 3; ++i) {
        if (!grids[i]) continue;
        for (unsigned item = 0; item < 5; ++item) {
            const auto amount = in.cap * int(item + 1) / 5;
            expect(0x1000a330, 0x1009bf34, {sp + 0x74, item, sp + 0x98, 5});
            bonus_money(text, sp + 0x74, amount, false, {0x1009bf40, 0x1009bf58});
            expect(0x10719740, 0x1009bfd4, {layout, 2, 0, 0, 0, 0, 0, 0});
            expected.back().stack_args = {16, color(-0x7bcc), color(-0x7bd0), 1, 1, color(-0x7bd4), text, 0,
                                          i + 0x2a, std::uint32_t(amount), grids[i]};
        }
    }
    expect(0x1074b7c0, 0x1009c004, {box, 0xffffffff});
}

struct BonusesCommandInput {
    unsigned action = 0x10, bonus = 1;
    bool fallback = false, no_control = false, missing_draft = false, compact = false;
    int previous = 50, adjusted = 45, offer = 90, cap = 100;
};

void bonuses_command_case(Cpu& c, const BonusesCommandInput& in) {
    using F = BonusFixture;
    F::setup(c);
    const auto sp = stack - 0x90;
    const auto control = in.no_control ? 0x7654ffffu : 0x76540003u;
    const auto widget = F::layout + 0xc0000 + 3 * 0x17c;
    const auto selected = in.no_control ? 0 : in.bonus;
    const auto offset = selected * 4 + 0x10;
    constexpr auto increment = 0x3456u;
    c.set(3, control);
    c.put32(widget - 0x5794, in.fallback ? 0 : 0x77ab0000u | in.action);
    c.put32(widget - 0x5754, in.action >= 0x2a && in.action <= 0x2d ? increment : 0x12340000u | in.bonus);
    c.put16(screens + 0x260000 - 0x7e62, in.fallback ? in.action : 0);
    c.put32(F::draft + offset, std::uint32_t(in.previous));
    c.put32(F::proposal + offset, std::uint32_t(in.offer));
    expect(read_history, 0x1009c05c, {screens, 2}, F::proposal);
    expect(0x101487d0, 0x1009c070, {F::drafts, 0}, in.missing_draft ? 0 : F::draft);
    if (in.missing_draft) {
        const auto strings = c.toc(-0x56cc), buffer = c.toc(-0x7ba8);
        expect(0x10739890, 0x1009c090, {strings + 0x87, 0, 0, sp + 0x48, sp + 0x40});
        expect(0x1077b720, 0x1009c0b0, {buffer, strings + 0x9c, c.toc(-0x5b20), sp + 0x48, sp + 0x40, 0xec2});
        expect(0x10769b60, 0x1009c0c0, {strings + 0xa9, buffer, 0});
        expect(0x1076f7d0, 0x1009c0d4, {0xffffffff});
        fixture_instruction_budget = 2;
        return;
    }
    const auto limits = [&](std::uint32_t club, std::uint32_t salary) {
        expect(0x10103510, club, {F::bank, F::club}, F::budget);
        expect(0x100f2680, salary, {F::budget, staff, 0xffffffff}, std::uint32_t(in.cap * 10));
    };
    const auto configure = [&](std::uint32_t back, unsigned control, bool enabled, unsigned command) {
        expect(0x10740620, back, {screens, control, enabled ? 2u : 6u});
        expect(0x10741450, back + 0x10, {screens, control, enabled ? 12u : 44u});
        expect(0x10742100, back + 0x20, {screens, control, enabled ? command : 0u});
        expect(0x10742420, back + (back == 0x1009c480 ? 0x34 : 0x30), {screens, control, enabled ? selected : 0u});
    };
    expected_result = 0;
    if (in.action >= 0x2a && in.action <= 0x2d) {
        expected_record_word = std::pair{F::draft + 0x10 + 4 * (in.action - 0x2a), in.no_control ? 0u : increment};
        expected_result = std::uint32_t(-11);
    } else if (in.action == 0x10 || in.action == 0xf) {
        const bool raise = in.action == 0xf;
        const auto amount = sp + (raise ? 0x38 : 0x3c);
        constexpr unsigned selected_widget = 0x45;
        expect(raise ? 0x1000a060 : 0x1000a170, raise ? 0x1009c458 : 0x1009c268, {amount, 8}, std::uint32_t(in.adjusted));
        expected.back().pointed_result = std::uint32_t(in.adjusted);
        expected.back().pointed_word = std::uint32_t(in.previous);
        expect(read_history, raise ? 0x1009c46c : 0x1009c27c, {screens, selected + 0x12}, 0x12340000 | selected_widget);
        configure(raise ? 0x1009c480 : in.adjusted > 5 ? 0x1009c2e0 : 0x1009c29c,
                  selected_widget, raise || in.adjusted > 5, 0x10);
        bonus_money(F::text, amount, in.adjusted, in.compact,
                    raise ? BonusMoney{0x1009c4c0, 0x1009c4d8, 0x1009c4e0, 0x1009c4f4, 0x1009c50c}
                          : BonusMoney{0x1009c31c, 0x1009c334, 0x1009c33c, 0x1009c350, 0x1009c368});
        expect(0x10741da0, raise ? 0x1009c51c : 0x1009c378, {screens, selected_widget + 1, F::text});
        if (raise) limits(0x1009c534, 0x1009c54c);
        configure(raise ? (in.adjusted < in.cap ? 0x1009c5c4 : 0x1009c580) : 0x1009c388,
                  selected_widget + 2, !raise || in.adjusted < in.cap, 0xf);
        expect(0x10742ec0, raise ? 0x1009c604 : 0x1009c3c8, {screens, 0, 0});
        expected_record_word = std::pair{F::draft + offset, std::uint32_t(in.adjusted)};
    } else if (in.action == 0x11) {
        limits(0x1009c6d4, 0x1009c6ec);
        expected_record_word = std::pair{F::draft + offset, std::uint32_t(std::min(in.offer >= 0 ? in.offer : 66, in.cap))};
        expected_result = std::uint32_t(-11);
    } else if (in.action == 0xd || in.action == 0xe) {
        const auto value = in.action == 0xd ? (in.offer >= 0 ? std::uint32_t(in.offer) : 55u) : 0xffffffffu;
        expected_record_word = std::pair{F::draft + offset, value};
        // The original and rewrite both execute the actual leaf.
        fixture_instruction_budget = in.action == 0xd ? 5 : 6;
        expected_result = std::uint32_t(-11);
    }
}
