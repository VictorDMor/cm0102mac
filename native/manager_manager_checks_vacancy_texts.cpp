// MANAGER_MANAGER.CPP checker cases for the vacancy block's three news texts (manager_manager_vacancy.hpp): 0x101bede0
// (message 0x29), 0x101bf070 (0x1789) and 0x101bf460 (0x2b). The message record is a scratch array of words (0x102cb570
// reads the word at +8+4n); the clubs, persons and rows are the career's, the formatters and name routines run for real.
// The output object's routines are answered in one family of cases and run for real over a scratch object in the other
// (0x1074bab0's row holds the register save area of the callees below it).
#include "manager_manager_checks_tail.hpp"
#include <string>

namespace cm::aot::manager_manager_checks {
namespace {
using tail::Rng;
constexpr std::uint32_t kVacancy = 0x101bede0, kAnswer = 0x101bf070, kOffer = 0x101bf460;
constexpr std::uint32_t to_object = 0x1074b830, name_to_object = 0x1074bab0;
constexpr std::uint32_t record = scratch_base + 0x9000, out_object = scratch_base + 0xa000;

Case make(std::uint32_t entry, std::uint32_t flag, const std::uint32_t (&f)[6], bool answered, std::string label) {
    Case item{.entry = entry, .registers = {{4, record}, {5, flag}, {6, out_object}}, .label = "vacancy text " + std::move(label)};
    for (unsigned i = 0; i < 6; ++i) item.memory.push_back({4, record + 8 + 4 * i, f[i]});
    if (answered) item.answers = {{to_object, 0}, {name_to_object, 0}};
    return item;
}
}

MANAGER_MANAGER_CASES("vacancy_texts") {
    if (scenario.match) return;
    Rng r{0x5eed};
    unsigned n = 0;
    for (const std::uint32_t flag : {1u, 0u, 0x100u, 0xffu, 0x80u, 0x101u})
        for (unsigned i = 0; i < 12; ++i) {
            std::uint32_t f[6] = {r.below(30), r.below(30), r.below(6), r.below(30), r.below(6), r.below(6)};
            f[4] = r.pick<std::uint32_t>({0u, 1u, 2u, 0xffu, 0x80u, 0x1ffu});
            cases.push_back(make(kVacancy, flag, f, r.chance(40), "vacancy " + std::to_string(n++)));
        }
    // The answer: first byte (1, 2, 0, other), second byte (the cases of the chain), the job's number.
    for (const std::uint32_t flag : {1u, 0u, 0x100u, 0xffu})
        for (const std::uint32_t first : {1u, 2u, 0u, 3u, 0xffu, 0x101u, 0x80u})
            for (const std::uint32_t second : {0u, 1u, 2u, 4u, 6u, 10u, 12u, 0xffu, 0x80u, 5u, 0x10cu})
                for (const std::uint32_t number : {0u, 4u, 5u, 6u, 9u, 0xffffffffu, 0x80000000u}) {
                    if (flag != 1 && (second != 0 || number != 0) && r.chance(80)) continue;
                    std::uint32_t f[6] = {r.below(30), first, second, r.below(30), number, 0};
                    cases.push_back(make(kAnswer, flag, f, r.chance(40), "answer " + std::to_string(n++)));
                }
    for (const std::uint32_t flag : {1u, 0u, 0x100u, 0xffu, 0x80u})
        for (unsigned i = 0; i < 80; ++i) {
            std::uint32_t f[6] = {r.below(6), r.below(30), r.below(30), r.below(40), 0, 0};
            auto item = make(kOffer, flag, f, r.chance(40), "offer " + std::to_string(n++));
            if (r.chance(50)) {  // the record the current-person cell names is the message's person
                const auto cell = scenario.toc(-0x7bb0);
                const auto current = cell + std::uint32_t(std::int32_t(std::int16_t(scenario.s.memory.read<2>(cell + 0x2c58)))) * 0x2c0;
                item.memory.push_back({4, current + 4, scenario.person(f[3])});
            }
            cases.push_back(std::move(item));
        }
}
}
