// The fixture specifies which control each category creates independently
// from the scores and statuses passed to the original and C++ implementations.
enum class CategoryControl { unavailable_role, pending_selected, pending_other,
                             rejected, selected, selectable };
struct ChoiceDrawInput {
    bool profile = true, secondary = false, player = true, offer = false, pending = false;
    bool same_club = false;
    unsigned roles = 0, group = 0;
    int category = 1, role = 12, proposal = 0, age = 23, height = 11, rating = 33000;
    std::array<CategoryControl, 6> controls{CategoryControl::selected, CategoryControl::selectable,
        CategoryControl::selectable, CategoryControl::selectable, CategoryControl::selectable, CategoryControl::selectable};
    unsigned warning = 0;
};

void choice_draw_case(Cpu& c, const ChoiceDrawInput& in) {
    constexpr auto local = stack - 0x3a0;
    constexpr auto text = fixture + 0x7000, style = fixture + 0x7100, colors = fixture + 0x7200,
                   contracts = fixture + 0x7300, offers = fixture + 0x8000, recruitment = fixture + 0x9200,
                   instance = fixture + 0x9300, club = fixture + 0x9400, other = fixture + 0x9700,
                   pending = fixture + 0x9800, link = fixture + 0x9900, linked_offer = fixture + 0x9a00,
                   profile = fixture + 0x9b00, secondary = fixture + 0x9c00, role_name = fixture + 0x9d00;
    constexpr unsigned selected_ink = 10, disabled_ink = 11, normal_ink = 12, action_grid = 33;
    constexpr unsigned notes = 22, option_grid = 23;
    const auto strings = c.toc(-0x56cc), layout = fixture + 0xb000 + in.group * 0x100;
    const auto linked = in.offer ? link : 0;
    const auto proposal = in.pending ? pending : 0;
    c.put32(c.r(2) - 0x56dc, style);
    c.put32(c.r(2) - 0x7bb8, text);
    c.put32(c.r(2) - 0x7bd0, colors);
    c.put32(c.r(2) - 0x7bc4, colors + 2);
    c.put32(c.r(2) - 0x7bc8, colors + 4);
    c.put32(c.r(2) - 0x7b7c, fixture + 0x70);
    c.put32(c.r(2) - 0x7b40, fixture + 0x74);
    c.put32(fixture + 0x70, contracts);
    c.put32(fixture + 0x74, offers);
    c.put32(offers + 0x988, recruitment);
    c.put32(style, 0x22334455);
    c.put16(style + 4, 0x6677);
    c.put8(style + 6, 0x88);
    c.put16(c.r(2) - 0x3a08, 0x77);
    c.put16(c.r(2) - 0x3a04, 0x88);
    c.put16(colors, selected_ink);
    c.put16(colors + 2, disabled_ink);
    c.put16(colors + 4, normal_ink);
    c.put16(screens + 0x2c58, in.group);
    c.put32(screens + in.group * 0x2c0 + 4, instance);
    c.put32(screens + in.group * 0x2c0 + 0x24, layout);
    c.put32(instance + 0x24, club);
    c.put16(club + 0x1ec, 100);
    c.put32(club + 0x2c, 0);
    c.put32(staff + 0x14, in.profile ? profile : 0);
    c.put32(staff + 0x1c, in.secondary ? secondary : 0);
    c.put32(staff + 0x24, in.same_club ? club : other);
    c.put16(staff + 0x5c, std::uint16_t(in.age));
    c.put32(link, linked_offer);

    const auto caption = [&](std::uint32_t back, unsigned offset) {
        expect(0x10727890, back, {text, strings + offset});
    };
    const auto row = [&](std::uint32_t back, unsigned grid, unsigned at, unsigned ink,
                         unsigned command = 0, unsigned value = 0, bool label = false) {
        expect(label ? 0x107445a0 : 0x107444e0, back, {screens, grid, 1, at, 1, 0, 1, 3});
        expected.back().stack_args = {ink, text};
        if (!label) expected.back().stack_args.insert(expected.back().stack_args.end(), {command, value});
    };
    const auto has_role = [&](std::uint32_t back, unsigned role) {
        expect(0x1007e4f0, back, {contracts, role, staff}, (in.roles >> role) & 1);
    };

    expect(read_history, 0x10095334, {screens, 0}, staff);
    expect(read_history, 0x10095348, {screens, 1}, std::uint32_t(std::uint8_t(in.category)) | 0xff00);
    expect(read_history, 0x10095358, {screens, 2}, linked);
    expect(read_history, 0x10095368, {screens, 4}, std::uint32_t(std::uint8_t(in.role)) | 0xff00);
    expect(read_history, 0x10095378, {screens, 5}, proposal);
    expect(read_history, 0x10095388, {screens, 6}, std::uint32_t(std::uint8_t(in.proposal)) | 0xff00);
    if (in.offer) expect(0x10416710, 0x100953ac, {linked_offer, 0, 0, 1, 1}, std::uint32_t(in.rating));
    for (unsigned role = 0; role < 16; ++role) has_role(0x100953d0, role);

    expect(0x107136d0, 0x10095420, {staff, 0, 0});
    expect(0x100c7a50, 0x10095434, {club, 1, local + 0x72, local + 0x70});
    expect(0x100caeb0, 0x10095448, {staff, local + 0x144, 500, 1});
    expect(0x10744250, 0x1009545c, {screens, 13, 7, local + 0x144});
    caption(0x10095468, 0xaf);
    expect(0x107442f0, 0x10095474, {screens, text});
    expect(0x10719740, 0x100954f4, {layout, 6, 125, 10, 790, 70, 0, 0});
    expected.back().stack_args = {1, 0, 0, 12, 7, 0, 0, 0, 0x7d7, staff, 0xffffffff};
    expect(0x1076d470, 0x100954fc, {3}, std::uint32_t(in.height));
    const auto lines = std::uint8_t(405 / in.height);
    expect(0x107198c0, 0x10095558, {layout, 110, 140, 780, 545, 2, local + 0x74, lines},
           notes);
    expected.back().stack_args = {0, 2, 0, 0xffffffff};
    unsigned y = 0;
    if (in.profile) {
        caption(0x10095574, 0xc0);
        row(0x100955b0, notes, y, normal_ink);
        y = 2;
    }
    expect(0x107198c0, 0x10095604, {layout, 110, 140, 780, 545, 2, local + 0x68, lines}, option_grid);
    expected.back().stack_args = {0, 1, 0, 0xffffffff};
    if (in.profile) {
        const auto name = local + 0xe0;
        // The original limit helper returns -1 for categories one through
        // three and scales club reputation for four through six.
        constexpr std::array limits{-1, -1, -1, 33000, 55000, 11000};
        for (unsigned cat = 1; cat <= 6; ++cat, ++y) {
            expect(0x1036f7e0, 0x10095634, {recruitment, cat, name, 2000});
            if (int(cat) == in.proposal)
                expect(0x1077b720, 0x10095650, {text, strings + 0xcd, name});
            else
                expect(0x1077dbf0, 0x10095668, {text, name});
            expect(0x1036f670, 0x10095680, {recruitment, club, staff, cat}, std::uint32_t(limits[cat - 1]));
            expect(0x100c7a20, 0x1009568c, {std::uint32_t(in.role)}, in.player);
            const auto control = in.controls[cat - 1];
            switch (control) {
            case CategoryControl::unavailable_role:
                row(0x100956c4, option_grid, y, disabled_ink, 0, 0, true);
                break;
            case CategoryControl::pending_selected:
                row(0x10095718, option_grid, y, selected_ink, 0, 0, true);
                break;
            case CategoryControl::pending_other:
                row(0x1009574c, option_grid, y, disabled_ink, 0, 0, true);
                break;
            case CategoryControl::rejected:
            case CategoryControl::selected:
            case CategoryControl::selectable:
                expect(0x1036f920, 0x10095764, {recruitment, staff, cat}, cat < 5 || std::int8_t(in.age) < 24);
                if (control == CategoryControl::rejected) {
                    expect(0x1077b720, 0x100957a0, {text, strings + 0xd2, name});
                    row(0x10095804, option_grid, y, disabled_ink, 0, 0, true);
                } else if (control == CategoryControl::selected) {
                    row(in.proposal ? 0x10095858 : 0x100958e4, option_grid, y, selected_ink, 3, cat);
                } else {
                    row(in.proposal ? 0x10095898 : 0x10095924, option_grid, y, normal_ink, 3, cat);
                }
                break;
            }
        }
    }
    ++y;
    if (std::popcount(in.roles) > 1) {
        caption(0x10095960, 0xd7);
        row(0x1009599c, notes, y, normal_ink);
        y += 2;
        for (unsigned role = 0; role < 16; ++role) {
            has_role(0x100959c0, role);
            if (((in.roles >> role) & 1) == 0 || (role == 10 && in.secondary && in.profile)) continue;
            expect(0x100c6bf0, 0x10095a00, {role, 0, 1, 0}, role_name);
            expect(0x1077dbf0, 0x10095a0c, {text, role_name});
            const bool selected = int(role) == in.role;
            row(selected ? 0x10095a54 : 0x10095a94, option_grid, y++, selected ? selected_ink : normal_ink, 0x2f, role);
        }
    }

    switch (in.warning) {
    case 0xe7:
        expect(0x10717270, 0x10095afc, {proposal + 0xc, local + 0x7c, 100, 255});
        expect(0x10727890, 0x10095b10, {text, strings + 0xe7, 100, local + 0x7c});
        row(0x10095b50, notes, ++y, normal_ink, 0, 0, true);
        break;
    case 0x129:
        caption(0x10095b20, 0x129);
        row(0x10095b50, notes, ++y, normal_ink, 0, 0, true);
        break;
    case 0x142:
    case 0x174:
        caption(in.warning == 0x142 ? 0x10095b90 : 0x10095ba0, in.warning);
        row(0x10095bd0, notes, ++y, normal_ink, 0, 0, true);
        break;
    }
    if (in.profile && !in.secondary && std::int8_t(in.age) > 30) {
        caption(0x10095c04, 0x1ad);
        row(0x10095c34, notes, y + 1, normal_ink, 0, 0, true);
    }
    expect(0x102a1070, 0x10095c40, {4, 0});
    expect(0x107198c0, 0x10095c94, {layout, 100, 555, 790, 590, 2, local + 0x6c, 1}, action_grid);
    expected.back().stack_args = {0, 1, 13, 0xffffffff};
    caption(0x10095ca4, 0x7b);
    expect(0x107444e0, 0x10095ce4, {screens, action_grid, 0, 0, 0x30, disabled_ink, 12, 3});
    expected.back().stack_args = {normal_ink, text, 1, 0};
    expect(0x10727890, 0x10095cf4, {text, strings + 0x1ef, strings + 0x214});
    if ((in.category <= 0 || in.category >= 7) && in.profile) {
        expect(0x100c7a20, 0x10095d18, {std::uint32_t(in.role)}, in.player);
        if (in.player) {
            expect(0x107445a0, 0x10095d94, {screens, action_grid, 1, 0, 0x30, disabled_ink, 0x2c, 3});
            expected.back().stack_args = {normal_ink, text};
            return;
        }
    }
    expect(0x107444e0, 0x10095d60, {screens, action_grid, 1, 0, 0x30, disabled_ink, 12, 3});
    expected.back().stack_args = {normal_ink, text, 2, 0};
}
