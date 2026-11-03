// Included by contract_screens_checks.cpp. The case selects the expected
// opener path explicitly and scripts only services outside CONTRACT_SCREENS.
enum class OfferPath { missing_instance, missing_club, pending, sole_role, reuse, list };

struct OfferChoiceInput {
    OfferPath path = OfferPath::list;
    bool link = false, blank_offer = false, with_offer = false, restricted = true,
         same_day = false, same_year = true, profile = false, terms_apply = false;
    unsigned available = 2, preferred = 1, opened = 1, permitted_categories = 0, rated_categories = 0;
    int pending_category = -1, club_category = -1, club_kind = 0, primary_kind = -1,
        age = 25, reputation = 4000, club_reputation = 1000, preferred_category = 0,
        reputation_factor = 0, interest_factor = 0, ambition_factor = 0;
};

void offer_choice_case(Cpu& c, const OfferChoiceInput& in, std::uint32_t outer = offer_choice) {
    constexpr std::uint32_t club = fixture + 0x7500, contracts = fixture + 0x8000, instance = fixture + 0x8200,
        offer = fixture + 0x8500, profile = fixture + 0x8600, cache = fixture + 0x8700,
        club_terms = fixture + 0x8900, pending_record = fixture + 0x8a00, primary = fixture + 0x8b00,
        today = fixture + 0x8c00, offers = fixture + 0x9000, recruitment = fixture + 0xa000,
        alert_buffer = fixture + 0xa100, succeeded = fixture + 0xa200;
    const auto link = in.link ? object : 0;
    const bool with_offer = in.link || in.with_offer;
    const bool direct = in.path == OfferPath::pending || in.path == OfferPath::sole_role || in.path == OfferPath::reuse;
    const auto choice_stack = outer == offer_choice ? stack : stack - 0x130;
    std::vector<Call> negotiation;
    if (direct) {
        const auto category = in.path == OfferPath::reuse && in.pending_category == -1
            ? std::uint32_t(in.club_category) : std::uint32_t(in.pending_category);
        negotiation_case(c, {.existing = with_offer, .profile = in.profile, .link = in.link,
                             .opened = in.blank_offer ? 0u : 1u, .kind = in.preferred},
                         category, 1, choice_stack - 0xf0);
        negotiation = std::move(expected);
        expected.clear();
    }

    const auto manager = [&](int slot, std::uint32_t holder, std::uint32_t value) {
        c.put32(c.r(2) + std::uint32_t(slot), holder);
        c.put32(holder, value);
    };
    manager(-0x7b7c, fixture + 0x24, contracts);
    manager(-0x7b40, fixture + 0x2c, offers);
    manager(-0x7be0, fixture + 0x30, staff - 3 * 0x70);
    c.put32(c.r(2) - 0x7b78, cache);
    c.put32(c.r(2) - 0x7b80, today);
    c.put32(c.r(2) - 0x7bac, succeeded);
    c.put32(c.r(2) - 0x7ba8, alert_buffer);
    c.put8(succeeded, 1);
    c.put32(offers + 0x988, recruitment);
    c.put16(screens + 0x2c58, 0);
    c.put32(screens + 4, in.path == OfferPath::missing_instance ? 0 : instance);
    c.put32(instance + 0x24, in.path == OfferPath::missing_club ? 0 : club);
    c.put16(club + 0x1ec, std::uint16_t(in.club_reputation));
    c.put32(staff, 37);
    c.put32(staff + 0x14, in.profile ? profile : 0);
    c.put16(staff + 0x5c, std::uint16_t(in.age));
    c.put8(staff + 0x63, std::uint8_t(in.reputation_factor));
    c.put8(staff + 0x65, std::uint8_t(in.interest_factor));
    c.put8(staff + 0x67, std::uint8_t(in.ambition_factor));
    c.put16(profile + 0xa, std::uint16_t(in.reputation));
    c.put8(club_terms + 0x38, std::uint8_t(in.club_kind * 4));
    c.put8(club_terms + 0x60, std::uint8_t(in.club_category));
    c.put8(primary + 0x38, std::uint8_t(in.primary_kind * 4));
    c.put16(today, 2001);
    c.put16(today + 2, 40);
    c.put32(today + 4, 0xaabbccdd);
    c.put16(pending_record + 0xc, in.same_year ? 2001 : 2000);
    c.put16(pending_record + 0xe, in.same_day ? 40 : 39);
    c.put8(pending_record + 0x14, std::uint8_t(in.pending_category));
    c.put32(object, in.blank_offer ? 0 : offer);
    c.put32(offer + 0xc, 37);
    c.put32(offer + 0x20, 120'000);
    c.put32(offer + 0x28, 0x6b9);
    c.put32(offer + 0x2c, 0x37);
    c.put8(offer + 0x35, 0xfe);
    c.set({{3, staff}, {4, link}, {5, in.with_offer}, {6, in.restricted}});

    if (outer != offer_choice) {
        constexpr std::uint32_t item = 0x21, revision = 0x35, offer_id = 0xa4;
        const auto news_stack = stack - 0x130 + 0x38;
        const bool restricted = outer == restricted_news;
        offer_news_payload = OfferNewsPayload{restricted || !with_offer ? 0xfa3u : 0xfa4u, 3, offer_id};
        c.set({{3, item}, {4, revision}});
        expect(init_news, restricted ? 0x10093a7c : 0x1009ed54, {news_stack, 0, 0});
        expect(fetch_news, restricted ? 0x10093a94 : 0x1009ed6c, {fixture + 0x100, item, revision, news_stack});
        expect(news_payload, restricted ? 0x10093aa0 : 0x1009ed78, {news_stack, restricted ? 2u : 0u});
        if (outer == offer_news && with_offer) {
            expect(news_payload, 0x1009ed94, {news_stack, 7});
            expect(0x103eb440, 0x1009eda4, {offers, offer_id, fixture + 0x2c}, link);
        }
    }

    if (in.path == OfferPath::missing_instance || in.path == OfferPath::missing_club) {
        const bool no_instance = in.path == OfferPath::missing_instance;
        const auto back = no_instance ? 0x10094bb4u : 0x10094c14u;
        const auto local = choice_stack - 0xf0;
        const auto name = local + (no_instance ? 0x74 : 0x54), extension = local + (no_instance ? 0x4c : 0x44);
        const auto strings = c.toc(-0x56cc);
        expect(0x10739890, back, {strings + 0x87, 0, 0, name, extension});
        expect(0x1077b720, back + 0x20, {alert_buffer, strings + 0x9c, c.toc(-0x5b20), name, extension,
                                       no_instance ? 0x193u : 0x19bu});
        expect(0x10769b60, back + 0x30, {strings + 0xa9, alert_buffer, 0});
        fixture_instruction_budget = 2;  // The actual two-instruction version getter.
        return;
    }

    for (unsigned role = 0; role < 16; ++role)
        expect(0x1007e4f0, 0x10094c70, {contracts, role, staff}, role < in.available);
    const auto current = in.club_category >= 0 ? club_terms : 0;
    const auto pending = in.pending_category >= 0 ? pending_record : 0;
    expect(0x10082f90, 0x10094ca0, {contracts, staff, club}, current);
    expect(0x10089cc0, 0x10094cb4, {contracts, staff}, in.preferred);
    expect(0x1007bf20, 0x10094cd0, {contracts, staff, instance, club}, pending);
    if (in.path != OfferPath::pending && in.path != OfferPath::sole_role &&
        !in.restricted && in.available < 2 && in.club_category > 0 && in.club_category < 7) {
        expect(0x10082f20, 0x10094e10, {contracts, staff}, in.primary_kind >= 0 ? primary : 0);
    }
    if (direct) {
        expected.insert(expected.end(), negotiation.begin(), negotiation.end());
        return;
    }

    expect(append, 0x10094ed8, {screens, c.toc(-0x56d4), c.toc(-0x56d8), 1, 0, 0}, in.opened);
    if (std::uint8_t(in.opened) == 0) return;
    history(0x10094ef4, 0, staff);
    if (current) expect(0x1037e3e0, 0x10094f0c, {current, staff}, in.terms_apply);
    if (current && !in.terms_apply) {
        if (in.club_category != 7 && in.club_category != 8 && in.profile)
            expect(0x1036f920, 0x10094f50, {recruitment, staff, std::uint32_t(in.club_category)},
                   (in.permitted_categories >> in.club_category) & 1);
    } else if (in.profile) {
        const auto young_rating = in.reputation + 10 * in.reputation_factor + 20 * in.ambition_factor;
        const auto adult_rating = in.reputation + 10 * in.reputation_factor + 25 * in.ambition_factor;
        for (unsigned category = 1; category <= 6; ++category) {
            if (current && category != 1) {
                expect(0x1037e3e0, 0x10094f94, {current, staff}, 1);
                if (std::uint32_t(in.club_category) <= category) continue;
            }
            if (in.age < 24) {
                if (category == 1 && (in.reputation < 3250 || young_rating < in.club_reputation)) continue;
                if (category == 2 && young_rating + 750 < in.club_reputation) continue;
                if (category == 3 && young_rating + 1250 < in.club_reputation) continue;
            }
            if (category == 1 && adult_rating + 1750 < in.club_reputation) continue;
            const auto allowed = (in.permitted_categories >> category) & 1;
            expect(0x1036f920, 0x1009512c, {recruitment, staff, category}, allowed);
            if (allowed) {
                constexpr double scores[] = {0, 9, 2, 5, 6, 7, 7.25};
                expect(0x10369080, 0x10095158, {recruitment, staff, club, 1, 0, category, choice_stack - 0xf0 + 0x38},
                       {}, scores[category]);
                expected.back().candidate_flags = (in.rated_categories >> category) & 1 ? 0x8000 : 0;
            }
        }
    }
    if (in.pending_category >= 0 && in.pending_category < 128) {
        history(0x100951b8, 1, std::uint32_t(in.pending_category));
        history(0x100951cc, 6, std::uint32_t(in.pending_category));
    } else {
        history(0x100951e4, 1, 0);
        history(0x100951f8, 6, std::uint32_t(in.preferred_category));
    }
    history(0x1009520c, 2, link);
    history(0x10095220, 3, with_offer);
    history(0x10095234, 4, in.preferred);
    history(0x10095248, 5, pending);
    history(in.link && !in.blank_offer ? 0x10095270 : 0x10095288, 7, in.link && !in.blank_offer ? 0x37 : 0);
}
