struct NegotiationCommandInput {
    unsigned action = 1, group = 0;
    bool fallback = false, no_control = false, linked = false, populated = false,
         review = false, stale = false, details_open = true;
    int status = 0, interval = 20;
    unsigned handler = 4;
    std::uint32_t common = 0;
};

void negotiation_command_case(Cpu& c, const NegotiationCommandInput& in) {
    constexpr auto draft_manager = fixture + 0x7500, draft = fixture + 0x7600, link_address = fixture + 0x7700,
                   offer_record = fixture + 0x7800, draft_offer = fixture + 0x7900;
    constexpr unsigned control_index = 8, value = 0x1234abcd, revision = 0x121, offer_id = 0x122,
                       year = 1999, day = 23, flags = 0x43;
    const auto layout = fixture + 0x5400 + in.group * 0x40;
    const auto control = in.no_control ? 0x9876ffffu : 0x12340000u | control_index;
    const auto widget = layout + 0xc0000 + control_index * 0x17c;
    const auto link = in.linked ? link_address : 0;
    const auto offer = in.populated ? offer_record : 0;
    c.set(3, control);
    c.put16(screens + 0x2c58, in.group);
    c.put32(screens + in.group * 0x2c0 + 0x24, layout);
    c.put32(widget - 0x5794, in.fallback ? 0 : 0xabc10000u | in.action);
    c.put32(widget - 0x5754, value);
    c.put16(screens + 0x260000 - 0x7e62, in.fallback ? in.action : 0);
    c.put32(c.r(2) - 0x7a54, fixture + 0x58);
    c.put32(fixture + 0x58, draft_manager);
    c.put32(draft, draft_offer);
    c.put32(link_address, offer);
    c.put32(offer_record + 0x28, offer_id);
    c.put32(offer_record + 0x2c, in.stale ? revision + 1 : revision);
    c.put8(offer_record + 0x34, std::uint8_t(in.status));
    c.put8(offer_record + 0x35, 0xfe);

    const auto read = [&](std::uint32_t back, unsigned slot, std::uint32_t result) {
        expect(read_history, back, {screens, slot}, result);
    };
    const auto active = [&](std::uint32_t back) {
        expect(0x101487d0, back, {draft_manager, 0}, draft);
    };
    const auto details = [&](bool confirm) {
        expect(append, 0x1009daf8, {screens, c.toc(-0x5708), c.toc(-0x570c), 1, 0, 0}, in.details_open ? 1 : 0x100);
        if (!in.details_open) return;
        history(0x1009db14, 0, link);
        history(0x1009db28, 3, confirm ? std::uint32_t(-2) : 0);
        history(0x1009db3c, 4, confirm ? std::uint32_t(-3) : 0xb);
        history(0x1009db50, 5, confirm ? 0 : 1);
        history(0x1009db64, 6, confirm ? std::uint32_t(-4) : 3);
        history(0x1009db78, 7, confirm ? std::uint32_t(-5) : 0);
        history(offer ? 0x1009dba0 : 0x1009dbd0, 1, offer ? offer_id : 0xffffffff);
        history(offer ? 0x1009dbb8 : 0x1009dbe4, 2, offer ? revision : 0xffffffff);
    };
    const bool refresh = offer && (in.stale || in.status == 2 || in.status == 3);
    expected_result = std::uint32_t(-4);
    switch (in.action) {
    case 0xc:
        history(0x10097c14, 1, in.no_control ? 0 : value);
        expected_result = std::uint32_t(-11);
        break;
    case 3:
        history(0x10097c94, 0, in.no_control ? 0 : value);
        history(0x10097ca8, 2, 0);
        expected_result = std::uint32_t(-11);
        break;
    case 0x31:
        expect(0x106ff550, 0x10097d00, {0, 0});
        break;
    case 2:
        read(0x10097d58, 3, link);
        read(0x10097d74, 6, in.review ? 0x1ff : 0x100);
        if (in.review) {
            read(0x10097d88, 5, revision);
            if (refresh) {
                expect(0x10411f10, 0x10097dc4, {offer, 1});
                expect(0x10422570, 0x10097dd8, {offer, revision});
                expected_record_word = std::pair{link_address, 0u};
                expected_result = std::uint32_t(-11);
            } else {
                active(0x10097dec);
                expect(0x10422ac0, 0x10097dfc, {link, draft_offer});
            }
        } else if (link) {
            expect(0x1041ea60, 0x10097e1c, {link, 2, link});
        } else {
            details(false);
        }
        break;
    case 0x1b:
        active(0x10097e98);
        expected_record_word = std::pair{draft, 0xffffffffu};
        expected_result = std::uint32_t(-10);
        break;
    case 1:
        active(0x10097ef8);
        read(0x10097f08, 3, link);
        if (!link) read(0x10097f2c, 7, 0x1fe);
        read(0x10097f3c, 8, 0x1fd);
        read(0x10097f4c, 0x1c, 0x1fc);
        read(0x10097f5c, 0x1d, 0x1fb);
        read(0x10097f6c, 0xb, 0x180);
        read(0x10097f7c, 0xc, 0xabc0ffff);
        expect(0x107182f0, 0x10097f88, {draft + 0x28}, 0x12340000u | std::uint16_t(in.interval));
        if (in.interval <= 28)
            expect(0x107182f0, 0x10097fa4, {draft + 0x28}, 0x56780000u | std::uint16_t(in.interval));
        expect(0x107156f0, 0x10097fc0, {stack - 0x70 + 0x38, std::uint32_t(std::min(in.interval, 28)), 0xffffff80, 0xffff, 0xffffffff});
        expected_record_word = std::pair{draft + 0x30, (year << 16) | day};
        expected_record_byte = std::pair{draft + 0x34, std::uint8_t(flags)};
        read(0x10097fe4, 5, revision);
        if (refresh) {
            expect(0x10422570, 0x1009801c, {offer, revision});
            expected_result = std::uint32_t(-11);
        } else {
            details(true);
        }
        break;
    default: {
        constexpr std::array handlers{std::pair{0x1009a440u, 0x10098050u}, std::pair{0x1009c020u, 0x10098064u},
                                      std::pair{0x1009d410u, 0x10098078u}, std::pair{0x102a54b0u, 0x1009808cu}};
        for (unsigned index = 0; index < handlers.size(); ++index) {
            const auto [target, back] = handlers[index];
            const auto result = index == in.handler ? 0xfffffff6u : 0;
            expect(target, back, {control}, result);
            if (result) {
                expected_result = result;
                return;
            }
        }
        expect(0x102a83a0, 0x100980a0, {control}, in.common);
        expected_result = in.common;
        break;
    }
    }
}
