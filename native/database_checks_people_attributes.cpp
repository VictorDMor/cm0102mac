// Fixtures of the people block, part "attributes" (database_checks_people.cpp): 0x100c2800, 0x100c2cb0 and
// 0x100c2520. The real attribute copies (0x100c5ab0, 0x100c65d0, 0x100c6150, 0x100c63c0), the balance
// (0x100c3870) and the abs leaf run; the hook 0x103d3f70 is stubbed (it records its arguments and the
// preserved registers). Worlds come from a seeded mixer: person rows of 0x70 bytes, player records and
// second records with attribute bytes drawn from profiles (centre and spread, so slots land within and beyond
// the limit), the player's kind byte, the 16-bit values at +0x4/+0x24, the hook word set or not, the pool of
// floating-point constants as the game has it or with extreme weights, and null rows/records.
#include "database_checks_people_detail.hpp"

namespace cm::aot::database_checks {
namespace {
using namespace people_checks;
constexpr std::uint32_t hook_cell = data + 0x100, table_cell = data + 0x110, count_cell = data + 0x120,
                        hook_word = data + 0x104,
                        people = data + 0x1000, players = data + 0x4000, records = data + 0x6000,
                        pool_copy = data + 0x8000;
constexpr std::uint32_t person_stride = 0x70, player_stride = 0x200, record_stride = 0x100;
constexpr std::uint32_t settle = 0x100c2800, weighted = 0x100c2cb0, rescale = 0x100c2520, hook = 0x103d3f70;

void put_toc(Cpu& c, std::int32_t offset, std::uint32_t value) { c.put32(c.r(2) + std::uint32_t(offset), value); }

struct Profile {
    std::int32_t centre, spread;
};
constexpr std::array<Profile, 10> profiles{{{0, 0}, {0, 20}, {0, 60}, {30, 60}, {-30, 60}, {60, 60}, {0, 127}, {100, 27},
                                           {-100, 27}, {12, 6}}};

struct Shape {
    Profile profile{0, 60};
    std::uint32_t kind = 5;
    std::int32_t limit = 0, record_limit = 0, record_second = 0;  // 16-bit values at player +4, record +4 and +0x24
    bool hook_set = false;
    bool null_player = false, null_record = false, null_person = false;
    unsigned pool = 0;  // 0: the game's constants; others: a copy with changed weights
};

void fill(Cpu& c, Mix& mix, std::uint32_t at, std::uint32_t size, const Profile& p) {
    for (std::uint32_t i = 0; i < size; ++i) {
        // Spread 0 is the bimodal profile: mostly -126 (within a wide limit) and some 127, so a shift by a positive
        // delta pushes the high ones over the clamp.
        if (p.spread == 0) {
            c.put8(at + i, mix.below(3) == 0 ? 127u - mix.below(3) : std::uint32_t(-126 + std::int32_t(mix.below(3))));
            continue;
        }
        const auto value = std::clamp(p.centre + std::int32_t(mix.below(2 * p.spread + 1)) - p.spread, -126, 127);  // -127 is the slot terminator: the second record's loops would not end
        c.put8(at + i, std::uint32_t(value));
    }
}

void put_double(Cpu& c, std::uint32_t at, double value) { c.put64(at, std::bit_cast<std::uint64_t>(value)); }

// The constants the routines load at TOC -0x5614: the game's, or a copy whose weights are odd (zero, large so
// the truncation saturates, negative, not a number) to reach the floating-point status paths.
void pool_world(Cpu& c, unsigned variant) {
    if (variant == 0) return;
    const auto real = c.toc(-0x5614);
    for (std::uint32_t i = 0; i < 0x100; i += 4) c.put32(pool_copy + i, c.u32(real + i));
    switch (variant) {
    case 1: for (const std::uint32_t at : {0x70u, 0x48u, 0x88u, 0x90u, 0x50u, 0x80u, 0xa0u}) put_double(c, pool_copy + at, 1e12); break;
    case 2: for (const std::uint32_t at : {0x70u, 0x48u, 0x88u, 0x90u, 0x50u, 0x80u, 0xa0u}) put_double(c, pool_copy + at, -3.7); break;
    case 3: put_double(c, pool_copy + 0x98, 0.0); put_double(c, pool_copy + 0x80, 0.0); put_double(c, pool_copy + 0xa0, 0.0); break;
    case 4: put_double(c, pool_copy + 0x98, std::bit_cast<double>(0x7ff0000000000001ULL)); put_double(c, pool_copy + 0x80, std::numeric_limits<double>::quiet_NaN()); break;
    case 5: put_double(c, pool_copy + 0x70, 0.3333); put_double(c, pool_copy + 0x98, 0.25); put_double(c, pool_copy + 0x80, 0.7); put_double(c, pool_copy + 0xa0, 0.7); break;
    }
    put_toc(c, -0x5614, pool_copy);
}

// A person row with its records. `k` selects the addresses.
std::uint32_t person_world(Cpu& c, Mix& mix, std::uint32_t k, const Shape& s) {
    const auto person = people + k * person_stride, player = players + k * player_stride, record = records + k * record_stride;
    fill(c, mix, person, person_stride, {0, 127});
    fill(c, mix, player, player_stride, s.profile);
    fill(c, mix, record, record_stride, s.profile);
    c.put8(player + 0xf, s.kind);
    c.put16(player + 4, std::uint32_t(s.limit));
    c.put16(record + 4, std::uint32_t(s.record_limit));
    c.put16(record + 0x24, std::uint32_t(s.record_second));
    c.put32(person + 0x14, s.null_player ? 0 : player);
    c.put32(person + 0x1c, s.null_record ? 0 : record);
    return person;
}

void common_world(Cpu& c, const Shape& s) {
    put_toc(c, -0x7a7c, hook_cell);
    c.put32(hook_cell, s.hook_set ? hook_word : 0);
    pool_world(c, s.pool);
}

Shape random_shape(Mix& mix) {
    Shape s;
    s.profile = profiles[mix.below(profiles.size())];
    s.kind = mix.below(2) ? mix.pick<std::uint32_t>({0, 1, 5, 14, 15, 16, 20, 20, 127, 128, 200, 255}) : mix.below(256);
    constexpr std::array<std::int32_t, 12> limits{0, 1, -1, 30, -30, 100, 1000, -1000, 5000, 32767, -32768, 255};
    s.limit = mix.below(3) ? limits[mix.below(limits.size())] : std::int32_t(mix.below(2000)) - 1000;
    s.record_limit = limits[mix.below(limits.size())];
    s.record_second = limits[mix.below(limits.size())];
    s.hook_set = mix.below(2);
    s.null_person = mix.below(25) == 0;
    s.null_player = mix.below(25) == 1;
    s.pool = mix.below(4) == 0 ? 1 + mix.below(5) : 0;
    return s;
}

std::string describe(const Shape& s) {
    return " profile " + std::to_string(s.profile.centre) + '/' + std::to_string(s.profile.spread) + " kind " +
           std::to_string(s.kind) + " limit " + std::to_string(s.limit) + (s.hook_set ? " hook" : "") +
           (s.null_person ? " null person" : "") + (s.null_player ? " null player" : "") +
           (s.pool ? " pool " + std::to_string(s.pool) : "");
}
const std::vector<Answer> hook_answer{{hook, {0}, 3}};
}

std::vector<Fixture> people_attributes_fixtures() {
    std::vector<Fixture> out;
    for (const auto entry : {settle, weighted}) {
        Mix mix{entry == settle ? 0xa771u : 0xb33fu};
        for (unsigned n = 0; n < 220; ++n) {
            auto shape = random_shape(mix);
            // The weighted routine's base depends on the player's 16-bit +0x4 against the weighted bytes: sweep it.
            if (entry == weighted && n % 3 == 0) shape.limit = std::int32_t(mix.below(65536)) - 32768;
            const auto world = mix.next();
            out.push_back(make(entry, "attributes" + describe(shape), Draws{}, [=](Cpu& c) {
                Mix local{world};
                common_world(c, shape);
                const auto person = person_world(c, local, 0, shape);
                c.set(3, shape.null_person ? 0 : person);
            }, n % 5 == 0, hook_answer));
        }
    }
    // The whole table: 0-4 rows, each with its own shape.
    Mix mix{0xc0ffeeu};
    for (unsigned n = 0; n < 90; ++n) {
        const std::uint32_t rows = n % 5;
        std::vector<Shape> shapes;
        for (std::uint32_t k = 0; k < rows; ++k) {
            auto s = random_shape(mix);
            s.null_person = false;
            s.null_record = mix.below(3) == 0;
            s.null_player = mix.below(4) == 0;
            shapes.push_back(s);
        }
        auto first = shapes.empty() ? Shape{} : shapes.front();
        const std::int32_t count = n % 17 == 3 ? -2 : n % 17 == 4 ? 0 : std::int32_t(rows);
        const auto world = mix.next();
        std::string text = "table of " + std::to_string(rows) + " count " + std::to_string(count);
        for (const auto& s : shapes) text += " [" + describe(s) + (s.null_record ? " null record" : "") + " " + std::to_string(s.record_limit) + "/" + std::to_string(s.record_second) + "]";
        out.push_back(make(rescale, text, Draws{},
                           [=](Cpu& c) {
            Mix local{world};
            common_world(c, first);
            put_toc(c, -0x7be0, table_cell);
            c.put32(table_cell, people);
            put_toc(c, -0x7be4, count_cell);
            c.put32(count_cell, std::uint32_t(count));
            for (std::uint32_t k = 0; k < shapes.size(); ++k) person_world(c, local, k, shapes[k]);
        }, n % 5 == 0, hook_answer));
    }
    return out;
}
}
