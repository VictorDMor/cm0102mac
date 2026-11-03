struct BasicDrawInput {
    unsigned status = 2, flags = 8, draft_flags = 8, current_flags = 0, country = 0;
    unsigned preference = 0, mode = 0, role = 0, note = 0, person_id = 4, current_kind = 2;
    unsigned first = 0, second = 0, last = 0, option = 0;
    bool eligible = false, reason = false, profile = false, secondary = false, interested = false,
         same_club = false, link = false, compact = false, no_wage_widget = false, no_wage_grid = false,
         check_current = false, same_date = false, mixed_traits = false;
    int age = 27, rating = 7000, profile_rating = 6000, proposal_fee = 3000, proposal_wage = 900,
        draft_fee = 2500, draft_wage = 1000, current_fee = 1600, fee_cap = 3000, wage_cap = 6000,
        score = 180, valuation = 2000, draft_day = 15, draft_year = 1999, now = 18,
        begin_month = 2, begin_year = 1999, end_month = 4, end_year = 1999, date_month = 2, max_years = 3, note_year = 1999, club_id = 3;
};

struct BasicDrawFixture {
    static constexpr auto base = fixture + 0xb000, count = base + 0x10, finance = base + 0x14,
        locale = base + 0x18, region = base + 0x1c, calendar = base + 0x30, settings = base + 0x38,
        cache = base + 0x40, proposal_club = base + 0x100, other_club = base + 0x348,
        profile = base + 0x590, current = base + 0x600, other = base + 0x680, nation = base + 0x700,
        other_nation = base + 0x710, staff_holder = base + 0x720, score_scale = base + 0x730, month_name = base + 0x740;
    static constexpr unsigned club_id = 3, grid = 49, fee_cell = 60, wage_cell = 70, fee_grid = 74, wage_grid = 75;
};

BasicDrawInput basic_draw_input;
std::uint32_t basic_draw_fixture_layout = 0;

bool basic_draw_boundary(const Cpu& c) {
    return c.lr() >= 0x100981b0 && c.lr() <= 0x1009a418 && (c.s.pc < 0x10098130 || c.s.pc >= 0x1009a434);
}

void trace_basic_draw(State& s) {
    unsigned words = 0;
    switch (s.pc) {
    case 0x107198c0: words = 4; break;
    case 0x1074bcd0: case 0x10410e50: words = 1; break;
    case 0x107445a0: words = 2; break;
    case 0x107444e0: words = 4; break;
    case 0x10719740: words = 11; break;
    }
    // Keep this trace separate from the parent screen's scripted services.
    record_call(s, trace + 0x20000 + basic_draw_calls++ * trace_words * 4, words);
}

bool basic_draw_service(State& s) {
    using B = BasicDrawFixture;
    const auto& in = basic_draw_input;
    Cpu c(s);
    const auto back = c.lr();
    trace_basic_draw(s);
    switch (s.pc) {
    case 0x1073ef70: {
        std::uint32_t value;
        switch (c.r(4)) {
        case 3: value = in.link ? B::staff_holder : 0; break;
        case 7: value = in.option; break;
        case 8: value = in.role; break;
        case 9: value = in.begin_month; break;
        case 0xa: value = in.begin_year; break;
        case 0xb: value = in.end_month; break;
        case 0xc: value = in.end_year; break;
        case 0xd: value = in.mode; break;
        case 0x15: value = in.wage_cap; break;
        case 0x16: value = in.valuation; break;
        case 0x18: value = in.fee_cap; break;
        case 0x19: value = 0x6789; break;
        case 0x1a: value = in.score; break;
        case 0x1b: value = in.country; break;
        case 0x1c: value = in.preference; break;
        case 0x1d: value = in.note; break;
        default: throw std::runtime_error("unexpected Basic history slot");
        }
        c.set(3, value);
        break;
    }
    case 0x1073f1e0:
        if (c.r(4) < 0xf || c.r(4) > 0x11) throw std::runtime_error("unexpected Basic widget slot");
        c.put32(history_data + 4 * c.r(4), c.r(5));
        break;
    case 0x10082f20: {
        const auto contract = [](unsigned owner) { return owner == 0 ? 0 : owner == 1 ? B::current : B::other; };
        switch (back) {
        case 0x100983b0: c.set(3, contract(in.first)); break;
        case 0x10098d70: case 0x10098f18: c.set(3, contract(in.second)); break;
        case 0x1009a108: case 0x1009a120: case 0x1009a27c: case 0x1009a294: c.set(3, contract(in.last)); break;
        default: throw std::runtime_error("unexpected Basic contract query");
        }
        break;
    }
    case 0x10089ea0: c.set(3, in.eligible ? 0x1ff : 0x100); break;
    case 0x10410e50:
        c.put8(c.u32(c.sp() + 0x38), 0x42);
        c.set(3, in.reason ? 0x1ff : 0x100);
        break;
    case 0x100d9520: c.set(3, in.interested ? 0x1ff : 0x100); break;
    case 0x1076d470: c.set(3, 11); break;
    case 0x107198c0:
        if (c.r(3) != basic_draw_fixture_layout) throw std::runtime_error("unexpected Basic grid manager");
        c.set(3, back == 0x10098398 ? B::grid : back == 0x10099e38 ? B::fee_grid : in.no_wage_grid ? 0x1234ffff : B::wage_grid);
        break;
    case 0x107444e0: case 0x107445a0:
        if (c.r(4) != B::grid) throw std::runtime_error("unexpected Basic field grid");
        c.set(3, back == 0x100991f4 ? B::fee_cell : back == 0x1009949c ? in.no_wage_widget ? 0x1234ffff : B::wage_cell : 0x5678ffef);
        break;
    case 0x1074bcd0:
        c.put32(c.r(7), c.u32(c.r(7)) + 1);
        break;
    case 0x1000a060: {
        const auto address = c.r(3), amount = c.u32(address);
        c.put32(address, amount + 1000);
        c.set(3, amount + 1000);
        break;
    }
    case 0x10009f20: {
        const auto value = std::uint64_t(std::int64_t(c.i32(c.r(3))) * 1'000'000'000);
        c.set({{3, std::uint32_t(value >> 32)}, {4, std::uint32_t(value)}});
        break;
    }
    case 0x1000a330: {
        if (c.r(4) >= c.r(6)) throw std::runtime_error("unexpected Basic preset index");
        const auto value = c.u32(c.r(5) + c.r(4) * 4);
        c.put32(c.r(3), value);
        c.set(3, value);
        break;
    }
    case 0x1077dbd0: c.set(3, in.compact ? 9 : 8); break;
    case 0x10718360: c.set(3, 0x12340000 | std::uint8_t(in.date_month)); break;
    case 0x107188a0: c.set(3, B::month_name); break;
    case 0x10380710: c.set(3, in.check_current ? 14 : 0); break;
    case 0x10715880:
        c.put32(c.r(3), 0);
        c.put8(c.r(3) + 4, 0);
        break;
    case 0x103fb130:
        c.put16(c.r(4), in.same_date ? in.now : in.now + 1);
        c.put16(c.r(4) + 2, in.note_year);
        c.put16(c.r(5), 1);
        c.put16(c.r(5) + 2, 2000);
        break;
    case 0x103721f0: {
        const auto record = c.r(3);
        for (auto offset : {0x28, 0x30}) {
            c.put32(record + offset, 0);
            c.put8(record + offset + 4, 0);
        }
        for (auto offset : {0x8, 0x48, 0x50}) c.put32(record + offset, 0);
        c.put32(record + 4, 0xffffffff);
        c.put32(record + 0x58, 0xffffffff);
        c.put16(record + 0x42, 0);
        c.put8(record + 0x54, 0);
        c.put8(record + 0x5c, 0);
        break;
    }
    case 0x100caeb0: c.put8(c.r(4), 0); break;
    case 0x10727890: case 0x10719740: case 0x1074b6e0: case 0x1074b7c0: case 0x1074b830:
    case 0x1074b960: case 0x1074bab0: case 0x1074c570: case 0x10372590:
    case 0x1000b410: case 0x10374d80: case 0x10715e00: case 0x10716eb0:
        break;
    default:
        throw std::runtime_error("unexpected Basic draw service " + hex_address(s.pc) + " at " + hex_address(back));
    }
    c.finish();
    return true;
}

void basic_draw_case(Cpu& c, const BasicDrawInput& in, std::uint32_t draft = BonusFixture::draft,
                     std::uint32_t proposal = BonusFixture::proposal, std::uint32_t caller = stack,
                     std::uint32_t layout = BonusFixture::layout) {
    using B = BasicDrawFixture;
    if (caller == stack) {
        BonusFixture::setup(c);
        c.set({{3, draft}, {4, proposal}, {5, staff}, {6, 140}, {7, 545}});
        c.put32(draft + 8, in.draft_wage);
        c.put32(draft + 0xc, in.draft_fee);
        c.put16(draft + 0x28, in.draft_day);
        c.put16(draft + 0x2a, in.draft_year);
        c.put8(draft + 0x38, in.draft_flags);
        c.put32(proposal + 4, in.club_id);
        c.put32(proposal + 8, in.proposal_wage);
        c.put32(proposal + 0xc, in.proposal_fee);
        c.put8(proposal + 0x38, in.flags);
        c.put8(proposal + 0x60, in.status);
        c.put32(staff, in.person_id);
        c.put32(staff + 0x14, in.profile ? B::profile : 0);
        c.put32(staff + 0x1c, in.secondary ? B::profile : 0);
        c.put32(staff + 0x24, in.same_club ? B::proposal_club : 0);
        c.put16(staff + 0x5c, in.age);
        c.put8(staff + 0x65, in.mixed_traits ? 10 : in.age);
        c.put8(staff + 0x67, in.age + 1);
    }
    const auto set_toc = [&](int offset, std::uint32_t value) { c.put32(c.r(2) + std::uint32_t(offset), value); };
    set_toc(-0x7b50, B::count);
    set_toc(-0x7b40, B::finance);
    set_toc(-0x7ac8, B::locale);
    if (caller == stack) {
        set_toc(-0x7b80, B::calendar);
        set_toc(-0x7b78, B::cache);
        set_toc(-0x7be8, B::staff_holder);
        c.put32(B::staff_holder, B::proposal_club - B::club_id * 0x248);
    }
    set_toc(-0x7990, B::settings);
    set_toc(-0x56f4, B::score_scale);
    const auto calendar = c.toc(-0x7b80), club_id = c.u32(proposal + 4);
    const auto club = std::int32_t(club_id) > 0 && club_id < 64
        ? c.u32(c.toc(-0x7be8)) + club_id * 0x248 : B::proposal_club;
    c.put32(B::count, 64);
    c.put32(B::finance, fixture + 0xb900);
    c.put32(B::locale, B::region);
    c.put32(B::region + 4, in.country & 0x600000);
    c.put_f32(B::score_scale, 100);
    c.put32(B::month_name, 0x4a756e65);  // June
    c.put8(B::month_name + 4, 0);
    c.put16(calendar, in.now);
    c.put16(calendar + 2, 1999);
    c.put32(calendar + 4, 0);
    c.put8(B::settings, in.max_years);
    c.put32(club, club_id);
    c.put32(B::other_club, B::club_id + 1);
    c.put32(club + 0x80, B::profile);
    c.put32(club + 0x90, in.country & 1 ? B::nation : 0);
    c.put32(B::other_club + 0x90, in.country & 2 ? B::other_nation : 0);
    c.put32(B::nation, 22);
    c.put32(B::other_nation, 35);
    c.put16(club + 0x1ec, in.rating);
    c.put8(club + 0x242, 3);
    c.put16(B::profile + 0xa, in.profile_rating);
    c.put32(B::current + 4, B::club_id);
    c.put32(B::other + 4, B::club_id + 1);
    for (const auto record : {B::current, B::other}) {
        c.put32(record + 0xc, in.current_fee);
        c.put8(record + 0x38, in.current_kind * 4);
        c.put32(record + 0x50, in.current_flags);
    }
    basic_draw_input = in;
    basic_draw_fixture_layout = layout;
    if (caller == stack)
        expected_record_word = std::pair{draft + 0xc, std::uint32_t(std::min(in.fee_cap, in.draft_fee))};
}

constexpr std::array basic_draw_inputs{
    BasicDrawInput{},
    BasicDrawInput{.status = 1, .flags = 5, .country = 0x200000, .eligible = true, .compact = true, .age = 18, .rating = 8000},
    BasicDrawInput{.status = 1, .flags = 4, .eligible = true, .age = 18, .rating = 5000},
    BasicDrawInput{.status = 1, .flags = 9, .eligible = true, .age = 18, .rating = 4000},
    BasicDrawInput{.status = 2, .eligible = true, .age = 18, .rating = 8000, .proposal_wage = 0},
    BasicDrawInput{.status = 2, .person_id = 3, .eligible = true, .age = 18, .rating = 5000},
    BasicDrawInput{.status = 3, .eligible = true, .age = 17, .rating = 8000},
    BasicDrawInput{.status = 3, .eligible = true, .age = 8, .rating = 5000},
    BasicDrawInput{.status = 3, .eligible = true, .age = 22, .rating = 6300},
    BasicDrawInput{.status = 5, .eligible = true, .age = 20, .rating = 8000},
    BasicDrawInput{.status = 5, .eligible = true, .age = 20, .rating = 6500},
    BasicDrawInput{.status = 5, .eligible = true, .age = 20, .rating = 5300},
    BasicDrawInput{.status = 4, .role = 10, .eligible = true, .proposal_fee = 2500, .fee_cap = 35000, .score = 30},
    BasicDrawInput{.status = 6, .role = 9, .eligible = true, .proposal_fee = 2500, .fee_cap = 35000, .score = 30},
    BasicDrawInput{.status = 0, .role = 8, .eligible = true, .proposal_fee = 2500, .fee_cap = 35000, .score = 30},
    BasicDrawInput{.status = 0, .eligible = true, .proposal_fee = 2500, .fee_cap = 35000, .score = 35},
    BasicDrawInput{.status = 3, .reason = true},
    BasicDrawInput{.status = 4, .reason = true},
    BasicDrawInput{.status = 5, .reason = true},
    BasicDrawInput{.status = 6, .reason = true},
    BasicDrawInput{.status = 1, .reason = true},
    BasicDrawInput{.status = 3},
    BasicDrawInput{.status = 4},
    BasicDrawInput{.status = 5},
    BasicDrawInput{.status = 6},
    BasicDrawInput{.status = 0, .secondary = true},
    BasicDrawInput{.status = 0},
    BasicDrawInput{.status = 8, .first = 2, .current_kind = 5},
    BasicDrawInput{.status = 4, .country = 3, .first = 2, .reason = true, .valuation = 10000},
    BasicDrawInput{.status = 4, .flags = 9, .score = 8, .valuation = 10000},
    BasicDrawInput{.status = 4, .score = 8, .valuation = 10000},
    BasicDrawInput{.status = 4, .flags = 4, .score = 8, .valuation = 10000},
    BasicDrawInput{.status = 4, .score = 30, .valuation = 10000},
    BasicDrawInput{.current_flags = 2, .first = 1, .second = 1, .current_fee = 3500},
    BasicDrawInput{.current_flags = 4, .first = 1, .second = 1},
    BasicDrawInput{.current_flags = 2, .mode = 1, .first = 1, .second = 1},
    BasicDrawInput{.current_flags = 2, .first = 1, .second = 1},
    BasicDrawInput{.first = 1, .second = 1},
    BasicDrawInput{.first = 1},
    BasicDrawInput{.second = 1, .profile = true, .interested = true, .profile_rating = 7500},
    BasicDrawInput{.second = 2, .profile = true, .interested = true, .profile_rating = 7500},
    BasicDrawInput{.profile = true, .profile_rating = 7500},
    BasicDrawInput{.profile = true, .profile_rating = 5500},
    BasicDrawInput{.note = 1},
    BasicDrawInput{.note = 1, .last = 1},
    BasicDrawInput{.note = 1, .last = 1, .same_club = true, .same_date = true},
    BasicDrawInput{.preference = 3, .note = 1, .last = 1, .check_current = true},
    BasicDrawInput{.note = 1, .last = 1, .same_club = true, .check_current = true},
    BasicDrawInput{.country = 0x400000, .option = 1, .no_wage_widget = true, .draft_fee = 50, .draft_wage = 0, .fee_cap = 60},
    BasicDrawInput{.option = 1, .no_wage_grid = true, .draft_fee = 9000, .draft_wage = 60000, .wage_cap = 3500000},
    BasicDrawInput{.draft_flags = 5, .link = true, .wage_cap = 500, .draft_day = 20, .end_month = 8},
    BasicDrawInput{.draft_year = 2000, .begin_year = 2000, .end_year = 2002, .max_years = 2},
    BasicDrawInput{.begin_month = 1, .end_month = 3, .date_month = 2},
    BasicDrawInput{.begin_month = 1, .end_month = 0, .end_year = 2000, .date_month = 11},
    BasicDrawInput{.begin_month = 1, .end_month = 3, .end_year = 2000, .date_month = 1},
    BasicDrawInput{.begin_month = 1, .end_month = 3, .end_year = 2000, .date_month = 8},
    BasicDrawInput{.begin_month = 2, .end_month = 3},
    BasicDrawInput{.begin_year = 2000, .end_month = 3},
    BasicDrawInput{.eligible = true, .score = 400, .club_id = -1},
    BasicDrawInput{.eligible = true, .score = 400, .club_id = 64},
    BasicDrawInput{.eligible = true, .proposal_fee = 4600},
    BasicDrawInput{.country = 2, .first = 1, .eligible = true},
    BasicDrawInput{.first = 1, .eligible = true},
    BasicDrawInput{.role = 9, .eligible = true, .proposal_fee = 3400},
    BasicDrawInput{.eligible = true, .mixed_traits = true, .age = 20, .proposal_fee = 100},
    BasicDrawInput{.current_flags = 2, .first = 1, .second = 1, .proposal_fee = 25000, .current_fee = 3500},
    BasicDrawInput{.first = 2},
    BasicDrawInput{.flags = 8, .score = 320, .valuation = 10000},
    BasicDrawInput{.flags = 9, .score = 320, .valuation = 10000},
    BasicDrawInput{.flags = 4, .score = 320, .valuation = 10000},
    BasicDrawInput{.proposal_fee = 175000, .score = 320, .valuation = 10000},
    BasicDrawInput{.reason = true, .score = 100},
    BasicDrawInput{.wage_cap = 75000},
    BasicDrawInput{.wage_cap = 3500000},
    BasicDrawInput{.wage_cap = -75000},
    BasicDrawInput{.note = 1, .last = 1, .note_year = 2001},
};
