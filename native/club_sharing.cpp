#include "club_sharing.hpp"
#include "allocator_adapter.hpp"
#include "aot_runtime.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <iomanip>
#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>

namespace cm::aot {
namespace {
constexpr std::uint32_t stack_low = 0x70000000, stack_high = 0x70100000;
constexpr std::uint32_t data_low = 0x108b3a90, data_high = 0x10bf7e30, heap_low = 0x20000000, heap_high = 0x30000000;
constexpr std::uint32_t refresh = 0x10357cb0, approach = 0x1036d6e0;
constexpr std::uint32_t rng_slots = 0x108b3f00, crt_rng = 0x108b85cc;
std::string hex_text(std::uint32_t value) { std::ostringstream text; text << "0x" << std::hex << std::setw(8) << std::setfill('0') << value; return text.str(); }
}

struct ClubSharing::Impl final : MemoryObserver, AllocatorBoundaryObserver {
    State& s;
    struct Shadow {
        std::uint32_t low, words;
        // Per 4-byte word: the club span that last wrote and last read it (span serial), the code that did,
        // and the span whose call allocated the block the word is in.
        std::uint32_t *writer, *writer_pc, *reader, *reader_pc, *block_owner;
        Shadow(std::uint32_t l, std::uint32_t h) : low(l), words((h - l + 3) / 4) {
            for (auto** table : {&writer, &writer_pc, &reader, &reader_pc, &block_owner}) *table = static_cast<std::uint32_t*>(std::calloc(words, 4));
        }
        ~Shadow() { for (auto* table : {writer, writer_pc, reader, reader_pc, block_owner}) std::free(table); }
    };
    Shadow data{data_low, data_high}, heap{heap_low, heap_high};
    // A span: one call for a club. Its identity for conflicts is the club on a day: two calls for one club
    // (refresh then approach) do not conflict with each other.
    struct Span { std::uint32_t entry, sp, club; bool below; };
    std::vector<Span> spans;
    struct Club { std::uint32_t address, day; };
    std::vector<Club> clubs{{0, 0}};                    // index 0: nobody
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint32_t> club_today;   // (address, day) -> index
    std::uint64_t days = 1, calls[2]{}, draws_inside = 0, draws_outside = 0, accesses = 0, allocator_calls = 0;
    unsigned allocator_depth = 0;
    std::uint64_t pending_size = 0;
    std::uint32_t pending_link = 0;
    struct Live { std::uint32_t size, owner, link; std::uint64_t judge; };
    std::map<std::uint32_t, Live> live;
    struct Conflict { std::uint64_t accesses = 0; std::uint32_t sample = 0; std::unordered_set<std::uint32_t> words; std::string block; };
    std::map<std::tuple<int, std::uint32_t, std::uint32_t>, Conflict> conflicts;
    // Inside the candidate scan: what judging one list entry writes outside the stack, by the code that writes.
    bool judging = false;
    std::uint64_t judge_serial = 0, judge_blocks_total = 0, judge_survivors = 0, judge_foreign_frees = 0;
    std::vector<std::uint32_t> judge_blocks;
    std::map<std::uint32_t, std::uint64_t> survivor_sizes, survivor_links;
    // The block a word lies in was allocated by the judging that is running: its own, not shared.
    bool own_block(std::uint32_t address) {
        auto after = live.upper_bound(address);
        if (after == live.begin()) return false;
        const auto& [start, block] = *std::prev(after);
        return address - start < block.size && block.judge == judge_serial;
    }
    std::uint64_t scans = 0, judged = 0, most_judged = 0, judged_in_scan = 0, judge_draws = 0, judge_allocations = 0, scan_draws = 0;
    bool scanning = false;
    struct Written { std::uint64_t accesses = 0; std::unordered_set<std::uint32_t> words; std::uint32_t sample = 0; std::string block; };
    std::map<std::uint32_t, Written> judge_writes;
    void candidate(int phase) {
        if (phase == 0) { scanning = true; ++scans; judged_in_scan = 0; }
        else if (phase == 3) { scanning = false; most_judged = std::max(most_judged, judged_in_scan); }
        else if (phase == 1) { judging = true; ++judged; ++judged_in_scan; ++judge_serial; }
        else {
            judging = false;
            // Blocks this judging allocated and did not release: they outlive it.
            for (const auto block : judge_blocks) if (live.count(block)) { ++judge_survivors; ++survivor_sizes[live[block].size]; ++survivor_links[live[block].link]; }
            judge_blocks.clear();
        }
    }
    std::array<std::uint32_t, 4> rng_words{};
    std::uint32_t rng_cursor = 0, date_word = 0, today = 0;

    explicit Impl(State& state) : s(state) {
        for (unsigned n = 0; n < 4; ++n) rng_words[n] = std::uint32_t(state.memory.read<4>(rng_slots + n * 4)) & ~3u;
        rng_cursor = rng_words[3];
        date_word = std::uint32_t(state.memory.read<4>(0x108b3f10));
        today = std::uint32_t(state.memory.read<2>(date_word));
    }
    std::uint32_t sp() const { return std::uint32_t(s.r<4>(4)); }
    std::uint32_t current() const { return spans.empty() ? 0 : spans.back().club; }
    void track() {
        const auto now = sp();
        while (!spans.empty()) {
            auto& top = spans.back();
            if (now < top.sp) { top.below = true; return; }
            if (!top.below) return;
            spans.pop_back();
        }
    }
    void probe() { track(); open(s.pc, sp()); }
    // A call is seen by the dispatcher's probe or, where the world loop calls the C++ owner in place, by the
    // callee's prologue saving one of the loop's return addresses in the caller's frame.
    void open(std::uint32_t pc, std::uint32_t at) {
        if (pc != refresh && pc != approach) return;
        if (!spans.empty() && spans.back().entry == pc && spans.back().sp == at) return;
        ++calls[pc == approach];
        const auto address = std::uint32_t(s.r<4>(16));
        auto [found, fresh] = club_today.try_emplace({address, std::uint32_t(days)}, std::uint32_t(clubs.size()));
        if (fresh) clubs.push_back({address, std::uint32_t(days)});
        spans.push_back({pc, at, found->second, false});
    }
    Shadow* shadow(std::uint32_t address) {
        if (address >= heap_low && address < heap_high) return &heap;
        if (address >= data_low && address < data_high) return &data;
        return nullptr;
    }
    bool rng(std::uint32_t address) const {
        return std::find(rng_words.begin(), rng_words.end(), address & ~3u) != rng_words.end() || (address & ~3u) == crt_rng;
    }
    std::string block_of(std::uint32_t address) {
        auto after = live.upper_bound(address);
        if (after == live.begin() || address - std::prev(after)->first >= std::prev(after)->second.size) return address >= heap_low ? "heap, no dated block" : "static data";
        const auto& [start, block] = *std::prev(after);
        return "block+" + hex_text(address - start) + " of " + std::to_string(block.size) + " bytes allocated " +
               (block.owner ? "by a club span" : "outside the spans") + " at " + hex_text(block.link);
    }
    void conflict(int kind, std::uint32_t other_pc, std::uint32_t address) {
        auto& entry = conflicts[{kind, other_pc, s.pc}];
        ++entry.accesses;
        if (!entry.sample) { entry.sample = address; entry.block = block_of(address); }
        if (entry.words.size() < 200000) entry.words.insert(address);
    }
    void access(std::uint32_t start, std::uint64_t size, bool write) {
        track();
        const auto club = current();
        if (write && start <= rng_cursor && start + size > rng_cursor) {
            ++(club ? draws_inside : draws_outside);
            if (judging) ++judge_draws; else if (scanning) ++scan_draws;
        }
        if (judging && write && !(start >= stack_low && start < stack_high) && !rng(start)) {
            if (allocator_depth) ++judge_allocations;
            else if (!own_block(start)) {
                auto& entry = judge_writes[s.pc];
                ++entry.accesses;
                if (!entry.sample) { entry.sample = start; entry.block = block_of(start); }
                if (entry.words.size() < 100000) entry.words.insert(start & ~3u);
            }
        }
        if (start >= stack_low && start < stack_high) return;
        auto* map = shadow(start);
        if (!map) return;
        const auto other_today = [&](std::uint32_t owner) { return owner && owner != club && clubs[owner].day == days; };
        for (auto word = (start - map->low) / 4, last = std::uint32_t((start + std::max<std::uint64_t>(size, 1) - 1 - map->low) / 4);
             word <= last && word < map->words; ++word) {
            const auto address = map->low + word * 4;
            if (!club) {
                // Code between the clubs' calls: what it writes is everyone's again.
                if (write) { map->writer[word] = 0; map->reader[word] = 0; }
                continue;
            }
            if (allocator_depth || rng(address) || (map->block_owner[word] && clubs[map->block_owner[word]].address == clubs[club].address &&
                                                  clubs[map->block_owner[word]].day == days)) continue;
            ++accesses;
            if (write) {
                if (other_today(map->writer[word])) conflict(1, map->writer_pc[word], address);
                else if (map->writer[word] != club && other_today(map->reader[word])) conflict(2, map->reader_pc[word], address);
                map->writer[word] = club; map->writer_pc[word] = s.pc;
            } else {
                if (other_today(map->writer[word])) conflict(0, map->writer_pc[word], address);
                if (map->reader[word] != club) { map->reader[word] = club; map->reader_pc[word] = s.pc; }
            }
        }
    }
    void mapped(std::uint32_t, std::uint64_t) override {}
    void cleared(std::uint32_t start, std::uint64_t size) override { access(start, size, true); }
    void written(std::uint32_t start, const std::uint8_t* bytes, std::size_t size) override {
        if (size == 4 && start >= stack_low && start < stack_high && start - 8 == sp()) {
            const auto value = std::uint32_t(read_be<4>(bytes));
            if (value == 0x10119c80 || value == 0x10119d58) { track(); open(refresh, start - 8); }
            else if (value == 0x10119dac || value == 0x10119ddc || value == 0x10119e18) { track(); open(approach, start - 8); }
        }
        if (start == date_word && size >= 2) {
            const auto day = std::uint32_t(bytes[0]) << 8 | bytes[1];
            if (day != today) { today = day; ++days; }
        }
        access(start, size, true);
    }
    void read(std::uint32_t start, const std::uint8_t*, unsigned size) override { access(start, size, false); }
    void enter(const State& state, std::uint32_t entry, bool) override {
        if (!allocator_depth++) {
            track();
            pending_size = entry == 0x10775130 ? state.r<4>(12) : entry == 0x107751b0 ? state.r<4>(16)
                         : entry == 0x10775200 ? state.r<4>(12) * state.r<4>(16) : 0;
            if (entry == 0x10775170 || entry == 0x107751b0) {
                const auto found = live.find(std::uint32_t(state.r<4>(12)));
                if (found != live.end()) {
                    if (judging && found->second.judge != judge_serial) ++judge_foreign_frees;
                    live.erase(found);
                }
            }
            pending_link = std::uint32_t(state.r<4>(4128));
            if (current()) ++allocator_calls;
        }
    }
    void leave(State& state, std::uint32_t, bool) override {
        if (!allocator_depth || --allocator_depth || !pending_size) return;
        const auto block = std::uint32_t(state.r<4>(12));
        if (block >= heap_low && block < heap_high && pending_size < 0x4000000) {
            live[block] = {std::uint32_t(pending_size), current(), pending_link, judging ? judge_serial : 0};
            if (judging) { ++judge_blocks_total; judge_blocks.push_back(block); }
            for (auto word = (block - heap_low) / 4, last = std::uint32_t((block + pending_size - 1 - heap_low) / 4); word <= last && word < heap.words; ++word)
                heap.block_owner[word] = current();
        }
        pending_size = 0;
    }
};

ClubSharing::ClubSharing(State& state) : impl(std::make_unique<Impl>(state)) { state.allocator_observer = impl.get(); }
ClubSharing::~ClubSharing() { if (impl->s.allocator_observer == impl.get()) impl->s.allocator_observer = nullptr; }
MemoryObserver* ClubSharing::observer() { return impl.get(); }
void ClubSharing::probe(State&) { impl->probe(); }
void ClubSharing::candidate(int phase) { impl->candidate(phase); }

void ClubSharing::report(std::ostream& out) const {
    const auto& i = *impl;
    out << "club sharing: " << i.days << " game days, " << i.calls[0] << " refresh calls, " << i.calls[1] << " approach calls, "
        << i.clubs.size() - 1 << " (club, day) pairs\n"
        << "RNG draws inside the clubs' calls: " << i.draws_inside << ", outside: " << i.draws_outside
        << "; allocator calls inside: " << i.allocator_calls << "\n"
        << "accesses of the clubs' calls outside stack, own blocks, RNG and allocator: " << i.accesses << '\n';
    out << "candidate scans: " << i.scans << ", entries judged: " << i.judged << " (most in one scan: " << i.most_judged
        << "); RNG draws while judging: " << i.judge_draws << ", in the scans outside judging: " << i.scan_draws
        << "; allocator writes while judging: " << i.judge_allocations << '\n'
        << "blocks allocated while judging: " << i.judge_blocks_total << ", still allocated when that judging ends: " << i.judge_survivors
        << "; blocks from elsewhere released while judging: " << i.judge_foreign_frees << '\n';
    for (const auto& [size, count] : i.survivor_sizes) out << "  surviving block of " << size << " bytes x" << count << '\n';
    for (const auto& [link, count] : i.survivor_links) out << "  surviving block allocated at " << hex_text(link) << " x" << count << '\n';
    out << "";
    {
        std::vector<std::pair<std::uint32_t, const Impl::Written*>> rows;
        std::uint64_t accesses = 0;
        for (const auto& [pc, value] : i.judge_writes) { rows.push_back({pc, &value}); accesses += value.accesses; }
        std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) { return a.second->accesses > b.second->accesses; });
        out << "\n-- judging writes, outside the stack while one candidate is judged: " << rows.size() << " distinct words, " << accesses << " accesses, " << rows.size() << " PCs --\n";
        for (std::size_t n = 0; n < std::min<std::size_t>(rows.size(), 400); ++n)
            out << "  writer " << hex_text(rows[n].first) << " -> writer " << hex_text(rows[n].first) << ": words=" << rows[n].second->words.size()
                << " accesses=" << rows[n].second->accesses << " sample=" << hex_text(rows[n].second->sample) << " in " << rows[n].second->block << ", this match 0\n";
    }
    static const char* const names[] = {"club conflicts, a club reads what another club of the day wrote",
        "club conflicts, two clubs of the day write", "club conflicts, a club writes what another club of the day read"};
    for (int kind = 0; kind < 3; ++kind) {
        std::vector<std::pair<std::tuple<int, std::uint32_t, std::uint32_t>, const Impl::Conflict*>> rows;
        std::uint64_t words = 0, accesses = 0;
        for (const auto& [key, value] : i.conflicts)
            if (std::get<0>(key) == kind) { rows.push_back({key, &value}); words += value.words.size(); accesses += value.accesses; }
        std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) { return a.second->words.size() > b.second->words.size(); });
        out << "\n-- " << names[kind] << ": " << words << " distinct words, " << accesses << " accesses, " << rows.size() << " PC pairs --\n";
        for (std::size_t n = 0; n < std::min<std::size_t>(rows.size(), 800); ++n)
            out << "  writer " << hex_text(std::get<1>(rows[n].first)) << " -> " << (kind == 0 ? "reader " : "writer ") << hex_text(std::get<2>(rows[n].first))
                << ": words=" << rows[n].second->words.size() << " accesses=" << rows[n].second->accesses << " sample=" << hex_text(rows[n].second->sample)
                << " in " << rows[n].second->block << ", this match 0\n";
    }
}
}
