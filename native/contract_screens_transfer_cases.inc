constexpr auto random_table = fixture + 0xc400, random_salt = fixture + 0xc410,
               random_last = fixture + 0xc414, random_cursor = fixture + 0xc418;
constexpr auto entropy = 0x1077b770u;

class RandomInput {
public:
    RandomInput(Cpu& c, unsigned position, std::uint32_t salt, std::uint32_t seed)
        : position(position), salt(salt), seed(seed) {
        constexpr std::array<std::uint16_t, 4> samples{17, 40, 400, 65535};
        c.put32(c.r(2) - 0x7b90, random_salt);
        c.put32(c.r(2) - 0x7b8c, random_table);
        c.put32(c.r(2) - 0x7b88, random_last);
        c.put32(c.r(2) - 0x7b84, random_cursor);
        for (unsigned i = 0; i < samples.size(); ++i) c.put16(random_table + i * 2, samples[i]);
        c.put32(random_last, random_table + 6);
        c.put32(random_cursor, random_table + position * 2);
        c.put32(random_salt, salt);
        c.put32(c.r(2) - 0x34c4, seed);
    }
    void draw(std::uint32_t refill_back) {
        if (++position == 4) {
            position = 0;
            seed = seed * 1103515245u + 12345u;
            salt = (seed >> 16) & 0x7fff;
            expect(entropy, refill_back, {});
            ++refills;
        }
    }
    void finish() const {
        expected_random = RandomEffects{refills, seed, random_salt, salt, random_cursor, random_table + position * 2};
        fixture_instruction_budget = refills;  // One native RNG step per wrap.
    }
private:
    unsigned position;
    std::uint32_t salt, seed;
    unsigned refills = 0;
};

void sample_transfer_case(Cpu& c, std::int32_t bound, unsigned position, std::uint32_t salt, std::uint32_t seed) {
    RandomInput rng(c, position, salt, seed);
    c.set(3, std::uint32_t(bound));
    if (bound != 0) {
        rng.draw(0x100945b4);
        if ((bound > 65535 || bound < -65535) && std::int32_t(std::uint32_t(bound) * 10u) / 65535 != 0)
            rng.draw(0x100946a0);
    } else {
        expected_result = 0;
    }
    rng.finish();
}

struct TransferCommandInput {
    unsigned action = 0x21, position = 0;
    bool fallback = false, no_control = false, missing_contract = false, affected = false, listed = false,
         immediate = false, first_draw = true, second_draw = true;
    unsigned group = 0;
    std::uint32_t handled = 0, common = 0;
    int mood = 0, interest = 40, ambition = 0, persistence = 0, final_mood = 0;
    unsigned flags = 0x72;
    std::optional<int> staff_interest;
};

void transfer_command_case(Cpu& c, const TransferCommandInput& in) {
    constexpr unsigned pressed = 4, staff_id = 3, club_id = 2, revision = 0x133;
    constexpr auto club = fixture + 0x7500, contract = fixture + 0x7800, contracts = fixture + 0x7900,
                   offers = fixture + 0x8000, recruitment = fixture + 0x9000, item = fixture + 0x9100;
    const auto layout = fixture + 0x5400 + in.group * 0x40;
    c.put16(screens + 0x2c58, in.group);
    c.put32(screens + in.group * 0x2c0 + 0x24, layout);
    c.put32(layout + 0xc0000 + pressed * 0x17c - 0x5794, in.fallback ? 0 : 0xabcd0000u | in.action);
    c.put16(screens + 0x260000 - 0x7e62, in.fallback ? in.action : 0x22);
    const auto control = in.no_control ? 0xcafaffffu : 0xcafe0000u | pressed;
    c.set(3, control);
    RandomInput rng(c, in.position, 0, 1);
    rng.finish();

    expected_result = std::uint32_t(-9);
    if (in.action == 0x22) return;
    if (in.action != 0x20 && in.action != 0x21) {
        expect(0x102a54b0, 0x10094528, {control}, in.handled);
        expected_result = in.handled ? in.handled : in.common;
        if (in.handled == 0) expect(0x102a83a0, 0x1009453c, {control}, in.common);
        return;
    }
    const auto holder = [&](int slot, std::uint32_t pointer, std::uint32_t value) {
        c.put32(c.r(2) + slot, pointer);
        c.put32(pointer, value);
    };
    holder(-0x7b7c, fixture + 0x24, contracts);
    holder(-0x7b40, fixture + 0x28, offers);
    holder(-0x7be8, fixture + 0x2c, club - 0x248 * club_id);
    holder(-0x7be0, fixture + 0x30, staff - 0x70 * staff_id);
    c.put32(offers + 0x988, recruitment);
    c.put8(contract + 0x4c, std::uint8_t(in.mood));
    c.put8(contract + 0x5c, in.flags);
    c.put8(item + 0x65, std::uint8_t(in.interest));
    c.put8(staff + 0x65, std::uint8_t(in.staff_interest.value_or(in.interest)));
    c.put8(item + 0x67, std::uint8_t(in.ambition));
    c.put8(item + 0x69, std::uint8_t(in.persistence));
    const bool decline = in.action == 0x21;
    expect(read_history, decline ? 0x10093f4c : 0x100943f0, {screens, 0}, item);
    expect(read_history, decline ? 0x10093f60 : 0x10094404, {screens, 1}, revision);
    expect(read_history, decline ? 0x10093f74 : 0x10094418, {screens, 2}, club_id);
    expect(read_history, decline ? 0x10093f84 : 0x10094428, {screens, 3}, staff_id);
    expect(0x10082f90, decline ? 0x10093fa4 : 0x10094448, {contracts, staff, club}, in.missing_contract ? 0 : contract);
    if (in.missing_contract) return;

    expect(0x102cd0c0, decline ? 0x10093fd0 : 0x10094474, {fixture + 0x100, item, revision, 7, 1, 0});
    expect(0x102cd0c0, decline ? 0x10093fec : 0x10094490, {fixture + 0x100, item, revision, 8, decline ? 0u : 1u, 0});
    if (decline) {
        if (!in.immediate) {
            if (in.first_draw) rng.draw(0x10094094);
            if (in.second_draw) rng.draw(0x100941ec);
            rng.finish();
        }
        if (in.listed) expect(0x1037fa80, in.immediate ? 0x10094030 : 0x10094330, {contract, 0x10000, 0});
        expect(0x100d3570, 0x10094398, {staff, std::uint32_t(-12), 0});
        expected_record_byte = std::pair{contract + 0x4c, std::uint8_t(in.final_mood)};
    } else {
        expect(0x100d3230, 0x1009449c, {item, 0}, in.affected ? 0x1ff : 0x100);
        if (in.affected) expect(0x10365cc0, 0x100944b4, {recruitment, item});
        expect(0x100d3570, 0x100944e8, {staff, 12, 0});
        expect(0x10363ca0, 0x10094500, {recruitment, staff, 1});
        expect(0x1008cd20, 0x10094518, {contracts, staff, club});
        expected_record_byte = std::pair{contract + 0x5c, std::uint8_t((in.flags & ~2u) | 8u)};
    }
}
