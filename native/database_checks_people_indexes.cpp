// Fixtures for 0x100b44c0 (pointers to ids over twelve tables): every table with 0 to 3 rows (and counts that
// are negative as a signed number), rows whose words are null or point at records of random words in low and
// high memory, so every field's read offset is told apart by the stored ids.
#include "database_checks_people_detail.hpp"

namespace cm::aot::database_checks {
namespace {
using namespace people_checks;
struct Slots {
    std::int32_t base, count;
    std::uint32_t stride;
};
constexpr std::array<Slots, 12> tables{{{-0x7bbc, -0x7b64, 0x6c}, {-0x7be8, -0x7b50, 0x248}, {-0x7b98, -0x7b04, 0x128},
                                        {-0x7ab4, -0x7528, 0x2c}, {-0x7548, -0x7568, 0x50}, {-0x74bc, -0x74c0, 0x34},
                                        {-0x79f8, -0x79f4, 0x44}, {-0x7be0, -0x7be4, 0x70}, {-0x753c, -0x755c, 0x68},
                                        {-0x74c4, -0x74c8, 0x14}, {-0x7538, -0x7558, 0x38}, {-0x7a34, -0x7a38, 0x1c}}};
constexpr std::uint32_t cells = data + 0x100, count_cells = data + 0x180, areas = data + 0x2000, pointees = data + 0x10000;

std::uint32_t pointee(Mix& mix) {
    const auto k = mix.below(24);
    return k < 16 ? pointees + 0x100 * k : high + 0x100 * (k - 16);
}
void world(Cpu& c, std::uint32_t seed, const std::array<std::int32_t, 12>& counts, unsigned null_percent) {
    Mix mix{seed};
    for (std::uint32_t k = 0; k < 16; ++k) {
        for (std::uint32_t at = 0; at < 0x100; at += 4) {
            c.put32(pointees + 0x100 * k + at, mix.next() * 2654435761u);
            if (k < 8) c.put32(high + 0x100 * k + at, mix.next() * 40503u);
        }
    }
    for (unsigned t = 0; t < tables.size(); ++t) {
        const auto area = areas + 0x1000 * t;
        c.put32(c.r(2) + std::uint32_t(tables[t].base), cells + 4 * t);
        c.put32(c.r(2) + std::uint32_t(tables[t].count), count_cells + 4 * t);
        c.put32(cells + 4 * t, area);
        c.put32(count_cells + 4 * t, std::uint32_t(counts[t]));
        for (std::uint32_t at = 0; at < 3 * tables[t].stride; at += 4)
            c.put32(area + at, mix.below(100) < null_percent ? 0 : pointee(mix));
    }
}
}

std::vector<Fixture> people_indexes_fixtures() {
    std::vector<Fixture> out;
    Mix mix{0xb44c0};
    for (unsigned n = 0; n < 400; ++n) {
        std::array<std::int32_t, 12> counts{};
        const std::int32_t choices[] = {0, 1, 2, 3, -1, 3, 2, 1};
        for (auto& count : counts) count = n % 10 == 0 ? 3 : choices[mix.below(8)];
        if (n % 17 == 1) counts[mix.below(12)] = std::int32_t(0x80000000);
        if (n % 23 == 2) counts[11] = 0;
        if (n % 29 == 3) counts[7] = 0;
        const auto seed = mix.next();
        const auto nulls = n % 7 == 0 ? 100u : n % 5 == 0 ? 0u : 30u;
        out.push_back(make(0x100b44c0, "indexes counts " + std::to_string(counts[0]) + ',' + std::to_string(counts[1]) +
                                           ",.. nulls " + std::to_string(nulls) + "% #" + std::to_string(n),
                           Draws{}, [=](Cpu& c) { world(c, seed, counts, nulls); }, n % 3 == 0));
    }
    return out;
}
}
