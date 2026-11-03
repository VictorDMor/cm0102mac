struct DetailFixture {
    static constexpr std::uint32_t manager = fixture + 0x7050, draft = fixture + 0x7100, contract = fixture + 0x7200,
        contracts = fixture + 0x7300, offers = fixture + 0x7400, club = fixture + 0x7500, employer = fixture + 0x7800,
        instance = fixture + 0x7b00, calendar = fixture + 0x7c00, cache = fixture + 0x7d00, text = fixture + 0x7e00,
        colors = fixture + 0x7f00, layout = fixture + 0x5400, link = fixture + 0x8000, offer = fixture + 0x8100,
        list = fixture + 0x8200, node = fixture + 0x8300, other = fixture + 0x8400, participant = fixture + 0x8500;
    static constexpr unsigned person_id = 4, club_id = 2, offer_id = 0x101, revision = 0x102;
    std::uint32_t linked, employed, employer_club;

    DetailFixture(Cpu& c, bool has_link, bool has_contract, bool same_club, int kind)
        : linked(has_link ? link : 0), employed(has_contract ? contract : 0),
          employer_club(has_contract && kind != 5 ? (same_club ? club : employer) : 0) {
        const auto holder = [&](int slot, std::uint32_t address, std::uint32_t value) {
            c.put32(c.r(2) + slot, address);
            c.put32(address, value);
        };
        holder(-0x7a54, fixture + 0x60, manager);
        holder(-0x7b7c, fixture + 0x64, contracts);
        holder(-0x7b40, fixture + 0x68, offers);
        holder(-0x7be0, fixture + 0x6c, staff - person_id * 0x70);
        holder(-0x7be8, fixture + 0x70, (same_club ? club : employer) - club_id * 0x248);
        c.put32(c.r(2) - 0x7bb8, text);
        c.put32(c.r(2) - 0x7bc8, colors);
        c.put32(c.r(2) - 0x7b14, colors + 2);
        c.put32(c.r(2) - 0x7bc4, colors + 4);
        c.put32(c.r(2) - 0x7b80, calendar);
        c.put32(c.r(2) - 0x7b78, cache);
        c.put16(c.r(2) - 0x39f4, 0xa122);
        c.put16(colors, 20);
        c.put16(colors + 2, 21);
        c.put16(colors + 4, 22);
        c.put16(screens + 0x2c58, 2);
        c.put32(screens + 2 * 0x2c0 + 4, instance);
        c.put32(screens + 2 * 0x2c0 + 0x24, layout);
        c.put32(instance + 0x24, club);
        c.put32(staff, person_id);
        c.put32(staff + 0x14, 0);
        c.put32(staff + 0x24, 0);
        c.put32(cache, 0xfeedface);
        c.put32(draft, person_id);
        c.put8(draft + 0x44, 0x7d);
        c.put32(contract + 4, club_id);
        c.put8(contract + 0x38, std::uint8_t(kind * 4));
        c.put16(contract + 0x30, 1994);
        c.put16(contract + 0x32, 30);
        c.put8(contract + 0x34, 0x53);
        c.put16(calendar, 2001);
        c.put16(calendar + 2, 40);
        c.put32(calendar + 4, 0xc000ff00);
        c.put8(employer + 0x23b, 0x83);
        c.put32(link, offer);
        c.put32(offer + 4, list);
        c.put32(offer + 0xc, person_id);
        c.put8(offer + 0x10, 0);
        c.put32(offer + 0x2c, revision);
        c.put8(offer + 0x34, 0);
    }

    void read(std::uint32_t back, unsigned slot, std::uint32_t value) const {
        expect(read_history, back, {screens, slot}, value);
    }
    void primary(std::uint32_t back) const {
        expect(0x10082f20, back, {contracts, staff}, employed);
    }
};

struct OfferDetailDrawInput {
    bool missing = false, linked = false, employed = false, same_club = false, titled = false,
         same_day = true, same_year = true;
    int withdraw = 0, mode = 0, option = 0, kind = 0, height = 11, compensation = 0;
};

void offer_details_draw_case(Cpu& c, const OfferDetailDrawInput& in) {
    const DetailFixture data(c, in.linked, in.employed, in.same_club, in.kind);
    constexpr auto sp = stack - 0x540, box = sp + 0x88, date = sp + 0x80, note = sp + 0x308, name = sp + 0x1dc;
    constexpr unsigned header = 29, controls = 31;
    const auto strings = c.toc(-0x56cc);
    const auto caption = [&](std::uint32_t back, unsigned offset) {
        expect(0x10727890, back, {data.text, strings + offset});
    };
    const auto byte_value = [](int value) { return std::uint32_t(std::uint8_t(value)) | 0x1100; };
    const auto append_note = [&](std::uint32_t back) { expect(0x1077dc60, back, {data.text, note}); };
    c.put32(data.draft, in.missing ? 0xffffffff : data.person_id);
    c.put32(staff + 0x24, in.titled ? data.employer : 0);
    detail_date = std::pair{std::uint16_t(in.same_year ? 2001 : 2000), std::uint16_t(in.same_day ? 40 : 39)};
    expect(0x1074b6e0, 0x1009dc58, {box, 0});
    expect(0x10715880, 0x1009dc64, {date});
    expect(0x101487d0, 0x1009dc74, {data.manager, 0}, data.draft);
    if (in.missing) {
        const auto buffer = c.toc(-0x7ba8);
        expect(0x10739890, 0x1009dcb0, {strings + 0x87, 0, 0, sp + 0x90, sp + 0x78});
        expect(0x1077b720, 0x1009dcd0, {buffer, strings + 0x9c, c.toc(-0x5b20), sp + 0x90, sp + 0x78, 0x1197});
        expect(0x10769b60, 0x1009dce0, {strings + 0xa9, buffer, 0});
        expect(0x1074b7c0, 0x1009dcf8, {box, 0xffffffff});
        fixture_instruction_budget = 2;
        return;
    }
    data.primary(0x1009dd20);
    data.read(0x1009dd7c, 5, byte_value(in.withdraw));
    data.read(0x1009dd8c, 6, byte_value(in.option));
    data.read(0x1009dd9c, 7, byte_value(in.mode));
    expect(0x107136d0, 0x1009ddb0, {staff, 0, 0});
    expect(0x100c7a50, 0x1009ddc4, {data.club, 1, sp + 0x66, sp + 0x64});
    expect(0x100caeb0, 0x1009ddd8, {staff, name, 300, 1});
    if (in.titled) {
        expect(0x1077b720, 0x1009ddf8, {data.text, strings + 0x215, name, data.employer + 0x221});
        expect(0x1074b830, 0x1009de04, {box, data.text});
        expect(0x1074bab0, 0x1009de1c, {box, data.employer + 0x221, 0x7d5, data.club});
    } else {
        expect(0x1077dbf0, 0x1009de2c, {data.text, name});
        expect(0x1074b830, 0x1009de38, {box, data.text});
    }
    expect(0x107198c0, 0x1009de88, {data.layout, 100, 10, 790, 70, 1, 0, 1}, header);
    expected.back().stack_args = {0, 0x30, 13, 0xffffffff};
    expect(0x1074bcd0, 0x1009deb8, {box, header, 100, 790, sp + 0x6c, 7, 7, 1});
    expected.back().stack_args = {0};
    caption(in.withdraw ? 0x1009decc : 0x1009dedc, in.withdraw ? 0x127e : 0x21d);
    expect(0x107442f0, 0x1009dee8, {screens, data.text});
    expect(0x1076d470, 0x1009def0, {3}, std::uint32_t(in.height));
    int compensation = 0;
    if (data.employer_club == data.employer) {
        data.read(0x1009df18, 0, data.linked);
        if (!data.linked) {
            compensation = in.compensation;
            expect(0x103fac70, 0x1009df4c, {data.offers, staff, data.club, in.mode == 0 || in.option != 0},
                   std::uint32_t(compensation));
            if (compensation > 0) {
                expect(0x10009f20, 0x1009df64, {sp + 0x68, 6}, 0x12345678);
                expected.back().pointed_word = std::uint32_t(compensation);
                expected.back().second_result = 0x23456789;
                expect(0x1000b410, 0x1009df7c, {sp + 0x114, 200, 0x12345678, 0x23456789, 6});
            }
        }
    }
    expect(0x10727890, in.withdraw ? 0x1009df94 : 0x1009dfa8,
           {data.text, strings + (in.withdraw ? 0x1296 : 0x12dd), name});
    if (!in.withdraw && compensation != 0) {
        expect(0x10727890, 0x1009dfd0, {note, strings + 0x1317, 0xffffff83, 0xffffff83, data.employer + 0x221, sp + 0x114});
        append_note(0x1009dfdc);
    }
    if (in.mode && !in.option) {
        expect(0x103fb130, 0x1009dffc, {data.offers, date, 0});
        if (in.same_year && in.same_day) {
            expect(0x10727890, 0x1009e080, {note, strings + 0x13b4});
            append_note(0x1009e08c);
        } else {
            expect(0x10716eb0, 0x1009e054, {date, sp + 0xb0, 100, 255});
            expect(0x10727890, 0x1009e064, {note, strings + 0x1374, sp + 0xb0});
            append_note(0x1009e070);
        }
    } else if (in.mode && in.option == 3) {
        expect(0x10727890, 0x1009e0ac, {note, strings + 0x1406});
        append_note(0x1009e0b8);
    }
    const auto rows = std::uint32_t(std::int8_t(405 / in.height));
    expect(0x1074a350, compensation ? 0x1009e128 : 0x1009e178, {110, 140, 780, 545, 2, 0, rows, 3});
    expected.back().stack_args = {20, data.text, name, 0x7da, staff};
    if (compensation) expected.back().stack_args.insert(expected.back().stack_args.end(), {data.employer + 0x221, 0x7d5, data.employer});
    expected.back().stack_args.push_back(0);
    expect(0x102a1070, 0x1009e184, {4, 0});
    expect(0x107198c0, 0x1009e1d0, {data.layout, 100, 555, 790, 590, 2, sp + 0x60, 1}, controls);
    expected.back().stack_args = {0, 1, 13, 0xffffffff};
    caption(0x1009e1e0, 0x80);
    expect(0x107444e0, 0x1009e220, {screens, controls, 0, 0, 0x30, 22, 12, 3});
    expected.back().stack_args = {20, data.text, 0x15, 0};
    caption(0x1009e22c, 0x83);
    expect(0x107444e0, 0x1009e26c, {screens, controls, 1, 0, 0x30, 22, 12, 3});
    expected.back().stack_args = {20, data.text, 0x14, 0};
    expect(0x1074b7c0, 0x1009e278, {box, 0xffffffff});
}

struct OfferDetailCommandInput {
    unsigned action = 0x14;
    bool fallback = false, no_control = false, linked = false, listed_participant = false,
         stale = false, employed = false, same_club = false, profile = false, protected_player = false;
    int withdraw = 0, mode = 0, option = 0, role = 4, person_role = 4, kind = 0, status = 0, category = 0;
    std::uint32_t handled = 0, common = 0;
};

void offer_details_command_case(Cpu& c, const OfferDetailCommandInput& in) {
    const DetailFixture data(c, in.linked, in.employed, in.same_club, in.kind);
    constexpr auto sp = stack - 0xe0, proposal = sp + 0x40, date = sp + 0x38;
    constexpr unsigned control_index = 4, clause = 0xfd, submitted_id = 0x10123, submitted_revision = 0x20123;
    const auto control = in.no_control ? 0x3456ffffu : 0x34560000u | control_index;
    const auto widget = data.layout + 0xc0000 + control_index * 0x17c;
    c.put32(widget - 0x5794, in.fallback ? 0 : in.action | 0x110000);
    c.put16(screens + 0x260000 - 0x7e62, in.fallback ? in.action : 0);
    c.set(3, control);
    c.put8(staff + 0x62, in.person_role);
    c.put32(staff + 0x14, in.profile ? fixture + 0x8600 : 0);
    c.put8(data.draft + 0x60, 0xa0 | in.category);
    c.put32(data.offer + 0x2c, in.stale ? data.revision - 1 : data.revision);
    c.put8(data.offer + 0x34, in.status);
    if (in.action == 0x15) {
        expected_result = std::uint32_t(-9);
        return;
    }
    if (in.action != 0x14) {
        expect(0x102a54b0, 0x1009e83c, {control}, in.handled);
        if (!in.handled) expect(0x102a83a0, 0x1009e850, {control}, in.common);
        expected_result = in.handled ? in.handled : in.common;
        return;
    }
    const auto byte_value = [](int value) { return std::uint32_t(std::uint8_t(value)) | 0x1200; };
    expected_result = std::uint32_t(-10);
    expect(0x101487d0, 0x1009e32c, {data.manager, 0}, data.draft);
    data.read(0x1009e360, 4, byte_value(in.role));
    data.read(0x1009e370, 5, byte_value(in.withdraw));
    data.read(0x1009e380, 6, byte_value(in.option));
    data.read(0x1009e390, 7, byte_value(in.mode));
    if (in.withdraw) {
        expect(0x1007f840, 0x1009e3ac, {data.contracts, staff, data.club});
        expected_record_word = std::pair{data.draft, data.person_id};
        return;
    }
    data.read(0x1009e3c0, 0, data.linked);
    auto offer_id = 0xffffffffu, revision = 0xffffffffu;
    if (in.linked) {
        offer_id = data.offer_id;
        revision = data.revision;
        data.read(0x1009e3d8, 1, offer_id);
        data.read(0x1009e3ec, 2, revision);
        constexpr std::array clauses{0x7d, 0x7d, 75, 50, 0, 0, 0x7d};
        expected_record_byte = std::pair{data.draft + 0x44, std::uint8_t(clauses[std::min<unsigned>(in.category, 6)])};
        if (in.stale || in.status == 2 || in.status == 3) {
            expect(0x10411f10, 0x1009e46c, {data.offer, 1});
            expect(0x10422570, 0x1009e480, {data.offer, revision});
            expected_record_word = std::pair{data.link, 0u};
            expected_result = std::uint32_t(-11);
            return;
        }
    } else {
        data.primary(0x1009e498);
        if (data.employer_club != data.club) {
            expect(0x10411ea0, 0x1009e520, {proposal, staff, data.club, 0, 0xffffffff, 1});
            const bool player = in.person_role == 12;
            expect(0x100c7a20, 0x1009e52c, {std::uint32_t(in.person_role)}, player);
            unsigned category = 0;
            bool contract_date = false;
            if (player) {
                data.primary(0x1009e544);
                if (data.employer_club) {
                    if (in.mode && !in.option) {
                        expect(0x103fac70, 0x1009e5bc, {data.offers, staff, data.club, 0}, 7000);
                        category = 5;
                    } else {
                        data.primary(0x1009e5e0);
                        if (in.kind != 1) {
                            data.primary(0x1009e614);
                            contract_date = true;
                            category = 3;
                        }
                    }
                }
            }
            if (in.profile && category != 5 && (in.role == 11 || in.role == 13 || in.role == 15)) {
                data.primary(0x1009e678);
                expect(0x103f8820, 0x1009e6e0, {data.offers, data.club, date, staff, data.employer_club, 0, 0},
                       in.protected_player);
                if (!in.protected_player) category = category ? 4 : 1;
            }
            data.read(0x1009e724, 3, clause);
            expect(0x103e25d0, 0x1009e738, {data.offers, proposal, 0});
            detail_submission = DetailSubmission{std::uint8_t(category * 16 + 0xa), std::uint8_t(in.role), clause,
                std::uint16_t(contract_date ? 1994 : 2001), std::uint16_t(contract_date ? 30 : 40),
                std::uint8_t(contract_date ? 0x53 : 0xc0)};
            expect(0x10411f10, 0x1009e74c, {proposal, 0xffffffff});
            offer_id = submitted_id;
            revision = submitted_revision;
        }
    }
    expect(0x1007f200, 0x1009e770, {data.contracts, data.draft, offer_id, revision, std::uint32_t(in.role), 0, 1, 0});
    if (in.linked && in.listed_participant) {
        c.put32(data.offer + 0xc, 5);
        c.put32(data.list + 4, data.node);
        c.put32(data.list + 0xc, 0xbad);
        c.put32(data.node, data.other);
        c.put32(data.node + 8, data.node + 0x10);
        c.put32(data.node + 0x10, data.participant);
        c.put32(data.node + 0x18, 0);
        c.put32(data.other, 18);
        c.put8(data.other + 4, 0);
        c.put32(data.participant, data.person_id);
        c.put8(data.participant + 4, 0);
        expect(0x10738e00, 0x1009e7d0, {data.list}, data.other);
        expect(0x10738e00, 0x1009e7d0, {data.list}, data.participant);
        expect(0x10738e00, 0x1009e7d0, {data.list}, 0);
        expected_record_byte = std::pair{data.participant + 4, std::uint8_t(1)};
    } else if (in.linked) {
        expected_record_byte = std::pair{data.offer + 0x10, std::uint8_t(1)};
    }
    expected_record_word = std::pair{data.draft, 0xffffffffu};
}
