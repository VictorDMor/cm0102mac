// Sorted insertion in the add-link owner against the original sort routine
// (0x10768300 with the two pair comparators). Wherever insertion applies, all
// memory except the sort's dead stack frames must equal the original's;
// wherever it declines, it must leave the index untouched. Cases: random sorted indexes with a new pair at every
// position, ties with the new pair, -1 wildcards, negative and extreme ids,
// unsorted indexes and single-pair indexes.
#include "shortlist_links.hpp"
#include "semantic_io_checks.hpp"
#include <iostream>
#include <optional>
#include <algorithm>
#include <random>
#include <set>

using namespace cm::aot;
namespace {
constexpr std::uint32_t array = 0x38000000, toc = 0x108bba90;
using Pair = std::array<std::int32_t, 2>;
void require(bool condition, const std::string& message) { if (!condition) throw std::runtime_error(message); }
void write(State& s, const std::vector<Pair>& pairs) {
    for (std::size_t n = 0; n < pairs.size(); ++n) {
        s.memory.write<4>(array + n * 8, std::uint32_t(pairs[n][0]));
        s.memory.write<4>(array + n * 8 + 4, std::uint32_t(pairs[n][1]));
    }
}
std::vector<std::uint8_t> read(const State& s, std::size_t count) { return s.memory.copy_out(array, count * 8); }
// First address where two runs' memory differs, ignoring the sort's dead
// frames below the caller's SP and the caller's 24-byte linkage area, where
// every callee saves its return address and which no caller reads.
std::optional<std::uint32_t> difference(const State& a, const State& b) {
    const auto stack = std::uint32_t(a.r<4>(4)) + 24;
    for (const auto& [start, size] : a.memory.mapped_ranges()) {
        const auto x = a.memory.copy_out(start, size), y = b.memory.copy_out(start, size);
        for (std::size_t n = 0; n < size; ++n)
            if (x[n] != y[n] && (start + n < 0x70000000 || start + n >= stack)) return std::uint32_t(start + n);
    }
    return std::nullopt;
}
struct Outcome { bool applied; std::unique_ptr<State> original, inserted; };
// Runs the original sort and the insertion on the same index.
Outcome run_both(const State& program, const std::vector<Pair>& pairs, bool club_first) {
    Outcome outcome{false, cm::semantic_io_checks::clone(program), cm::semantic_io_checks::clone(program)};
    auto& original = *outcome.original;
    write(original, pairs); write(*outcome.inserted, pairs);
    original.budget = UINT64_MAX;
    original.pc = 0x10768300;
    original.wr<4>(12, array); original.wr<4>(16, std::uint32_t(pairs.size())); original.wr<4>(20, 8);
    original.wr<4>(24, std::uint32_t(original.memory.read<4>(toc + (club_first ? -0x6298 : -0x62a8))));
    original.wr<4>(4128, return_sentinel);
    run(original);
    outcome.applied = shortlist_links::insert_last_pair(*outcome.inserted, array, std::uint32_t(pairs.size()), club_first);
    return outcome;
}
// Returns whether insertion applied; either way the outcome must be right:
// applied, all memory equals the original's; declined, the index is untouched.
bool check(const State& program, const std::vector<Pair>& pairs, bool club_first, const std::string& label) {
    auto before = cm::semantic_io_checks::clone(program);
    write(*before, pairs);
    const auto outcome = run_both(program, pairs, club_first);
    if (outcome.applied) {
        if (const auto at = difference(*outcome.original, *outcome.inserted))
            throw std::runtime_error(label + ": insertion differs from the original sort at " + hex_address(*at));
    } else {
        require(read(*outcome.inserted, pairs.size()) == read(*before, pairs.size()), label + ": a declined insertion changed the index");
    }
    return outcome.applied;
}
std::vector<Pair> sorted(std::vector<Pair> pairs, bool club_first) {
    std::ranges::sort(pairs, [&](const Pair& a, const Pair& b) {
        return club_first ? std::pair{a[1], a[0]} < std::pair{b[1], b[0]} : a < b;
    });
    return pairs;
}
}

int main(int argc, char** argv) {
    try {
        require(argc == 2, "Usage: cm-shortlist-links-insert-checks <original-segments>");
        auto program = std::make_unique<State>();
        load_program(*program, argv[1]);
        program->memory.map(array, 0x10000);
        std::mt19937 random(20261021);
        unsigned applied = 0, declined = 0;
        const auto count = [&](bool used) { ++(used ? applied : declined); };
        for (const bool club_first : {false, true}) {
            for (unsigned round = 0; round < 300; ++round) {
                // Unique non-negative pairs, sorted, then one new unique pair.
                const auto range = 1 + random() % 40, size = 1 + random() % std::min(120u, range * range);
                std::set<Pair> unique;
                while (unique.size() < size) unique.insert({std::int32_t(random() % range), std::int32_t(random() % range)});
                std::vector<Pair> pairs(unique.begin(), unique.end());
                const auto added = pairs[random() % pairs.size()];
                pairs.erase(std::ranges::find(pairs, added));
                pairs = sorted(pairs, club_first);
                pairs.push_back(added);
                require(check(*program, pairs, club_first, "unique pair"), "Insertion declined a unique pair");
                ++applied;
                // The same index with a tie: the new pair repeats an existing one.
                if (pairs.size() > 1) {
                    auto tie = pairs; tie.back() = tie[random() % (tie.size() - 1)];
                    require(!check(*program, tie, club_first, "tie"), "Insertion accepted a tie");
                    ++declined;
                }
            }
            // A -1 wildcard anywhere, negative ids, an unsorted index.
            count(check(*program, {{1, 2}, {3, -1}, {2, 2}}, club_first, "wildcard in index"));
            count(check(*program, {{1, 2}, {3, 4}, {2, -1}}, club_first, "wildcard added"));
            count(check(*program, {{-5, 2}, {3, 4}, {2, 2}}, club_first, "negative id"));
            count(check(*program, {{3, 4}, {1, 2}, {2, 2}}, club_first, "unsorted index"));
            // Extreme non-negative ids, where the comparators' subtraction is widest.
            count(check(*program, sorted({{0x7fffffff, 0}, {0, 0x7fffffff}}, club_first), club_first, "extreme ids sorted"));
            auto extreme = sorted({{0x7fffffff, 0x7fffffff}, {0, 0}, {0x40000000, 1}}, club_first);
            extreme.push_back({0x7ffffffe, 0x7fffffff});
            require(check(*program, extreme, club_first, "extreme ids"), "Insertion declined extreme ids");
            ++applied;
            require(check(*program, {{7, 7}}, club_first, "single pair"), "Insertion declined a single pair");
            ++applied;
        }
        // The comparison itself must catch a wrong index: two pairs swapped.
        {
            auto outcome = run_both(*program, {{1, 1}, {2, 2}, {4, 4}, {3, 3}}, false);
            require(outcome.applied && !difference(*outcome.original, *outcome.inserted), "Control insertion");
            write(*outcome.inserted, {{2, 2}, {1, 1}, {3, 3}, {4, 4}});
            const auto at = difference(*outcome.original, *outcome.inserted);
            require(at && *at >= array && *at < array + 32, "A swapped index was not detected");
        }
        std::cout << "Link index insertion matches the original sort: " << applied << " applied, " << declined
                  << " declined and left to the sort\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
