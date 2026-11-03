struct NegotiationDrawInput {
    bool person = true, linked = false, populated = false, resume = true, reuse_draft = false,
         same_club = false, current = false, previous = false, employer_text = false,
         menu_enabled = false, menu_requested = false;
    int tab = 1, role = 4, person_role = 4, previous_kind = 0, current_kind = 0, previous_status = 0;
    unsigned group = 0;
    bool without_club = false, different_contract_club = false, different_draft_club = false;
    std::uint32_t limit = 500, wage = 600;
};

void negotiation_draw_case(Cpu& c, const NegotiationDrawInput& in, std::uint32_t draft = fixture + 0x7100,
                           std::uint32_t proposal = fixture + 0x7200) {
    constexpr auto sp = stack - 0xba0, box = sp + 0xa8, draft_manager = fixture + 0x7050,
                   contract_manager = fixture + 0x7300,
                   previous = fixture + 0x7400, current = fixture + 0x7500, link = fixture + 0x7600,
                   offer = fixture + 0x7700, club = fixture + 0x7800, other_club = fixture + 0x7b00,
                   instance = fixture + 0x7e00, calendar = fixture + 0x7f00, candidate = fixture + 0x8000,
                   layout = fixture + 0x8100, text = fixture + 0x8200, ink = fixture + 0x8300,
                   cached_contract = fixture + 0x8400, enabled = fixture + 0x8500;
    constexpr unsigned staff_id = 4, club_id = 7, offer_id = 9, review_limit = 337, header_grid = 53,
                       comparison_grid = 51, comparison_icon = 52, footer_grid = 54;
    const auto strings = c.toc(-0x56cc);
    const auto staff_address = in.person ? staff : 0, club_address = in.person && !in.without_club ? club : 0;
    const auto linked = in.linked ? link : 0;
    const auto proposal_id = in.resume ? proposal : 0;
    const auto primary = in.current ? current : 0, pending = in.previous ? previous : 0;
    const auto holder = [&](int slot, std::uint32_t address, std::uint32_t value) {
        c.put32(c.r(2) + slot, address);
        c.put32(address, value);
    };
    holder(-0x7a54, fixture + 0x60, draft_manager);
    holder(-0x7b7c, fixture + 0x64, contract_manager);
    holder(-0x7be0, fixture + 0x68, staff_address - staff_id * 0x70);
    holder(-0x7be8, fixture + 0x6c, (in.same_club ? club : other_club) - club_id * 0x248);
    c.put32(c.r(2) - 0x7bb8, text);
    c.put32(c.r(2) - 0x7bc8, ink);
    c.put32(c.r(2) - 0x7bcc, ink + 2);
    c.put32(c.r(2) - 0x7bd0, ink + 4);
    c.put32(c.r(2) - 0x7bd4, ink + 6);
    c.put32(c.r(2) - 0x7bdc, ink + 8);
    c.put32(c.r(2) - 0x7bc4, ink + 10);
    c.put32(c.r(2) - 0x7b14, ink + 12);
    c.put32(c.r(2) - 0x7b78, cached_contract);
    c.put32(c.r(2) - 0x7a68, enabled);
    c.put32(c.r(2) - 0x7b80, calendar);
    c.put16(c.r(2) - 0x3a00, 0x42);
    c.put8(c.r(2) - 0x39fe, 1);
    c.put16(c.r(2) - 0x39fc, 0x19);
    c.put8(enabled, in.menu_enabled);
    for (unsigned i = 0; i < 7; ++i) c.put16(ink + 2 * i, 30 + i);
    c.put16(screens + 0x2c58, in.group);
    c.put32(screens + in.group * 0x2c0 + 4, instance);
    c.put32(screens + in.group * 0x2c0 + 0x24, layout);
    c.put32(instance + 0x24, club_address);
    c.put32(staff, staff_id);
    c.put32(staff + 0x24, in.employer_text ? club : 0);
    c.put8(staff + 0x62, in.person_role);
    c.put32(club, club_id);
    c.put32(link, in.populated ? offer : 0);
    c.put32(offer + 0x28, offer_id);
    c.put32(cached_contract, 0xfeedface);
    c.put32(current + 4, club_id + in.different_contract_club);
    c.put8(current + 0x38, std::uint8_t(4 * in.current_kind));
    c.put16(current + 0x32, 37);
    for (const auto source : {draft, previous, proposal}) {
        c.put32(source, source == draft && !in.reuse_draft ? staff_id + 1 : staff_id);
        c.put32(source + 4, club_id + (source == draft && in.different_draft_club));
        c.put32(source + 8, source == draft ? 1 : in.wage);
        for (unsigned offset = 0xc; offset <= 0x60; ++offset)
            c.put8(source + offset, std::uint8_t((source >> 8) + offset));
    }
    c.put16(proposal + 0x32, 38);
    c.put8(previous + 0x38, std::uint8_t(4 * in.previous_kind));
    c.put8(previous + 0x64, in.previous_status);
    c.put8(previous + 0x78, 12);
    for (unsigned i = 0; i < 50; ++i) c.put32(club + 0x98 + 4 * i, 0);
    c.put32(club + 0x98, staff);
    c.put32(club + 0xa0, candidate);
    c.put32(candidate, 43);

    const auto read = [&](std::uint32_t back, unsigned slot, std::uint32_t value) {
        expect(read_history, back, {screens, slot}, value);
    };
    const auto caption = [&](std::uint32_t back, unsigned offset) {
        expect(0x10727890, back, {text, strings + offset});
    };
    const auto club_style = [&](std::uint32_t back) {
        expect(0x100c7a50, back, {club_address, 1, sp + 0x6e, sp + 0x6c});
    };
    const auto assert_at = [&](std::uint32_t at, unsigned name, unsigned extension, unsigned line) {
        const auto buffer = c.toc(-0x7ba8);
        expect(0x10739890, at + 4, {strings + 0x87, 0, 0, sp + name, sp + extension});
        expect(0x1077b720, at + 0x24, {buffer, strings + 0x9c, c.toc(-0x5b20), sp + name, sp + extension, line});
        expect(0x10769b60, at + 0x34, {strings + 0xa9, buffer, 0});
        fixture_instruction_budget += 2;  // The real version getter.
    };
    expect(0x1074b6e0, 0x10096cd8, {box, 0});
    read(0x10096ce8, 0, staff_id);
    read(0x10096d04, 3, linked);
    read(0x10096d28, 8, std::uint32_t(std::uint8_t(in.role)) | 0xff00);
    if (!in.person) {
        assert_at(0x10096d4c, 0xd0, 0xa0, 0x567);
        expect(0x1074b7c0, 0x10096d98, {box, 0xffffffff});
        return;
    }
    read(0x10096da8, 2, proposal_id);
    expect(0x101487d0, 0x10096dd4, {draft_manager, 0}, draft);

    const bool reuse = in.previous && in.previous_kind != 5 && in.previous_kind != 6;
    if (!in.resume) {
        read(0x10096dec, 0xd, 0x1fc);
        expect(0x10773520, 0x10096df8, {100}, proposal);
        expect(0x103721f0, 0x10096e04, {proposal});
        expect(0x10374f50, 0x10096e28, {proposal, staff, club_address, in.populated ? offer_id : 0xffffffff,
                                       std::uint32_t(-4), std::uint8_t(in.role), sp + 0x78, sp + 0x74});
        history(0x10096e3c, 0x19, review_limit);
        const auto converted = c.f64(c.toc(-0x56d0) + 0x30) * 2.0;
        history(0x10096e68, 0x1a, std::uint32_t(std::int32_t(converted)));
        expect(0x1007f720, 0x10096e7c, {contract_manager, staff_id, club_id}, pending);
        if (reuse || !in.reuse_draft || in.different_draft_club) {
            const auto source = reuse ? previous : proposal;
            expect(0x10771110, reuse ? 0x10096ef4 : 0x10097048, {draft + 0x1c, source + 0x1c, 5});
            if (!reuse) expect(0x1008a160, 0x10097100, {contract_manager, proposal});
            read(reuse ? 0x10096fa8 : 0x1009710c, 0x15, in.limit);
            expected_record_word = std::pair{draft + 8, std::min(in.wage, in.limit)};
            if (reuse) history(0x10096fd0, 8, 12);
        }
        expect(0x10082f20, 0x1009712c, {contract_manager, staff}, primary);
        if (in.current && !in.different_contract_club) {
            expect(0x10718360, 0x1009714c, {current + 0x30}, 0x1ff);
            history(0x10097160, 9, 0xffffffff);
            history(0x10097174, 0xa, 37);
        } else {
            expect(0x10717b20, 0x10097188, {sp + 0x90, calendar, 32});
            expect(0x10718360, 0x10097190, {sp + 0x90}, 0x1ff);
            history(0x100971a4, 9, 0xffffffff);
            expect(0x10717b20, 0x100971b4, {sp + 0x88, calendar, 32});
            history(0x100971c8, 0xa, 42);
        }
        history(0x100971dc, 2, proposal);
        expect(0x10718360, 0x100971e4, {proposal + 0x30}, 0x180);
        history(0x100971f8, 0xb, 0xffffff80);
        history(0x1009720c, 0xc, 38);
    }

    read(0x10097218, 1, std::uint32_t(std::uint8_t(in.tab)) | 0xff00);
    expect(0x107136d0, 0x1009722c, {staff, 0, 0});
    expect(0x10082f20, 0x10097240, {contract_manager, staff}, primary);
    if (in.current && in.current_kind != 5 && in.same_club && !in.different_contract_club) {
        expect(0x100c7a20, 0x100972a8, {std::uint32_t(in.person_role)}, in.person_role == 12);
        if (in.person_role == 12) {
            club_style(0x100972c4);
            expect(0x10744430, 0x10097308, {screens, 105, 15, 120, 35, 0x30, 13, 12}, comparison_icon);
            expected.back().stack_args = {1, 7, 0, 0, 0};
            expect(0x107198c0, 0x10097354, {layout, 0, 0, 0, 0, 0, 0, 0}, comparison_grid);
            expected.back().stack_args = {0, 0x30, 31, comparison_icon};
            expect(0x10768300, 0x1009736c, {club + 0x98, 50, 4, c.toc(-0x7988)});
            expect(0x10768300, 0x10097380, {club + 0x98, 20, 4, c.toc(-0x798c)});
            expect(0x100caeb0, 0x100973b4, {candidate, text, 2000, 4});
            expect(0x10719740, 0x10097430, {layout, 2, 0, 0, 0, 0, 0, 0});
            expected.back().stack_args = {16, 31, 32, 1, 1, 33, text, 0, 3, 43, comparison_grid};
        }
    }

    if (club_address) club_style(0x10097474);
    expect(0x100caeb0, 0x10097488, {staff, sp + 0x36c, 2000, 1});
    if (in.employer_text) {
        expect(0x1077b720, 0x100974a8, {text, strings + 0x215, sp + 0x36c, club + 0x221});
        expect(0x1074b830, 0x100974b4, {box, text});
        expect(0x1074bab0, 0x100974c8, {box, club + 0x221, 0x7d5});
    } else {
        expect(0x1077dbf0, 0x100974d8, {text, sp + 0x36c});
        expect(0x1074b830, 0x100974e4, {box, text});
    }
    expect(0x107198c0, 0x10097534, {layout, 100, 10, 790, 70, 1, 0, 1}, header_grid);
    expected.back().stack_args = {0, 0x30, club_address ? 13u : 30u, 0xffffffff};
    expect(0x1074bcd0, 0x10097564, {box, header_grid, 100, 790, sp + 0x70, 7, club_address ? 7u : 36u, 1});
    expected.back().stack_args = {0};
    caption(0x10097570, 0x21d);
    expect(0x107442f0, 0x1009757c, {screens, text});
    if (in.menu_enabled) {
        read(0x10097598, 0x1d, in.menu_requested);
        if (in.menu_requested) {
            expect(0x10727890, 0x100975b0, {text, strings + 0x22c, strings + 0x214});
            expect(0x10744430, 0x100975fc, {screens, 730, 85, 785, 105, 0x30, 34, 12});
            expected.back().stack_args = {1, 30, text, 0x31, 0};
        }
    }

    const auto display_role = !in.resume && reuse ? 12 : in.role;
    expect(0x100c7a20, 0x10097614, {std::uint32_t(display_role)}, display_role == 12);
    if (display_role == 12) {
        expect(0x10727890, 0x1009762c, {sp + 0xf4, strings + 0x267});
        expect(0x10727890, 0x10097660, {sp + 0x1c8, strings + 0x26d});
        expect(0x10727890, 0x1009768c, {sp + 0x29c, strings + 0x275});
        expect(0x1074aa90, 0x100976bc, {sp + 0xf0, 3, std::uint32_t(in.tab), sp + 0x80, sp + 0x7c, 1});
    }
    switch (in.tab) {
    case 1: basic_draw_case(c, {}, draft, proposal, sp, layout); break;
    case 2: bonuses_draw_case(c, {}, draft, proposal, sp, layout); break;
    case 3: clauses_draw_case(c, {}, draft, proposal, sp, layout); break;
    default: assert_at(0x10097748, 0xb0, 0x98, 0x659); break;
    }

    expect(0x102a1070, 0x10097794, {4, 0});
    if (!in.linked) expect(0x1007f720, 0x100978f4, {contract_manager, staff_id, club_id}, pending);
    const bool review = in.previous && in.previous_status != 1 && in.previous_status != 4;
    const auto full = in.linked || review;
    expect(0x107198c0, in.linked ? 0x100977e8 : review ? 0x10097960 : 0x10097aa4,
           {layout, 100, 555, 790, 590, full ? 3u : 2u, sp + (full ? 0x84 : 0x68), 1}, footer_grid);
    expected.back().stack_args = {0, 1, club_address ? 13u : 30u, 0xffffffff};
    const auto button = [&](std::uint32_t title, std::uint32_t back, unsigned label, unsigned column, unsigned command) {
        caption(title, label);
        expect(0x107444e0, back, {screens, footer_grid, column, 0, 0x30, 35, 12, 3});
        expected.back().stack_args = {30, text, command, 0};
    };
    if (in.linked) {
        button(0x100977f8, 0x1009783c, 0x7b, 0, 0x1b);
        button(0x10097848, 0x1009788c, 0x27d, 1, 2);
        button(0x10097898, 0x100978dc, 0x284, 2, 1);
    } else if (review) {
        button(0x10097970, 0x100979b4, 0x7b, 0, 0x1b);
        button(0x100979c0, 0x10097a04, 0x27d, 1, 2);
        button(0x10097a10, 0x10097a54, 0x284, 2, 1);
    } else {
        button(0x10097ab4, 0x10097af8, 0x27d, 0, 0x1b);
        button(0x10097b04, 0x10097b48, 0x284, 1, 1);
    }
    expect(0x1074b7c0, 0x10097b54, {box, 0xffffffff});
}
