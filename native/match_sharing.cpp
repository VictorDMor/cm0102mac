#include "match_sharing.hpp"
#include "allocator_adapter.hpp"
#include "aot_runtime.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace cm::aot {
namespace {
constexpr std::uint32_t stack_low = 0x70000000, stack_high = 0x70100000;
constexpr std::uint32_t data_low = 0x108b3a90, data_high = 0x10bf7e30, heap_low = 0x20000000, heap_high = 0x30000000;
// Match routines (MATCH_DAY/MATCH_ENG): the day's driver, the two callers
// that own a match around the engine, and the match object's own routines.
constexpr std::uint32_t day_driver = 0x101cb9d0, scheduled = 0x101cd5c0, watched = 0x101ccdb0, statistics = 0x10370e10,
                        construct = 0x101d16a0, set_up = 0x101d1760, destroy = 0x101d3e50, advance = 0x101d3f60;
constexpr std::uint32_t match_object_size = 0xdfc;
// The match RNG's words are reached through four TOC slots (seed, table begin, table end, cursor); the C runtime's state is fixed.
constexpr std::uint32_t rng_slots = 0x108b3f00, crt_rng = 0x108b85cc;
// Return addresses of the calls of those routines (the instruction after each `bl`), sorted.
constexpr std::array<std::pair<std::uint32_t, std::uint32_t>, 34> call_sites{{
    {0x10119170, day_driver}, {0x101196b8, day_driver}, {0x10119940, statistics}, {0x10131df8, watched},
    {0x101cb838, destroy}, {0x101cbfac, advance}, {0x101cc008, destroy}, {0x101cc1a8, scheduled}, {0x101cc1e4, construct},
    {0x101cc268, set_up}, {0x101cc284, advance}, {0x101cc368, destroy}, {0x101cc620, scheduled}, {0x101ccb28, construct},
    {0x101ccba4, set_up}, {0x101ccbc0, advance}, {0x101cccac, destroy}, {0x101ccdd8, scheduled}, {0x101ccf1c, advance},
    {0x101cd1ec, scheduled}, {0x101cd4d8, construct}, {0x101cd55c, set_up}, {0x101cd6fc, advance}, {0x101cd748, advance},
    {0x101cd838, destroy}, {0x101cebc0, construct}, {0x101cec38, set_up}, {0x101cec4c, advance}, {0x101ced58, destroy},
    {0x1028df54, watched}, {0x10371bfc, construct}, {0x10371c74, set_up}, {0x10371c88, advance}, {0x10371d10, destroy}}};
// Owners of an access. Matches start at first_match.
enum : std::uint32_t { nobody = 0, other = 1, driver = 2, statistics_caller = 3, scheduled_caller = 4, watched_caller = 5, first_match = 8 };
// A write by other code made after a match finished on the same day carries that match.
constexpr std::uint32_t after_match = 0x40000000;
const char* owner_name(std::uint32_t owner) {
    switch (owner) {
        case nobody: return "before-run"; case other: return "other-code"; case driver: return "day-driver";
        case statistics_caller: return "statistics-caller"; case scheduled_caller: return "scheduled-caller";
        case watched_caller: return "watched-caller";
    }
    return owner & after_match ? "code-after-match" : "match";
}
struct Hex { std::uint32_t value; };
std::ostream& operator<<(std::ostream& out, Hex h);
std::string hex_text(std::uint32_t value) { std::ostringstream text; text << Hex{value}; return text.str(); }
std::ostream& operator<<(std::ostream& out, Hex h) {
    const auto flags = out.flags();
    out << "0x" << std::hex << h.value;
    out.flags(flags);
    return out;
}
}

struct MatchSharing::Impl final : MemoryObserver, AllocatorBoundaryObserver {
    State& s;
    struct Shadow {
        std::uint32_t low, words;
        std::uint32_t *writer, *writer_pc, *reader, *birth;   // per 4-byte word; calloc keeps untouched pages unmapped
        Shadow(std::uint32_t l, std::uint32_t h) : low(l), words((h - l + 3) / 4),
            writer(static_cast<std::uint32_t*>(std::calloc(words, 4))), writer_pc(static_cast<std::uint32_t*>(std::calloc(words, 4))),
            reader(static_cast<std::uint32_t*>(std::calloc(words, 4))), birth(static_cast<std::uint32_t*>(std::calloc(words, 4))) {}
        ~Shadow() { std::free(writer); std::free(writer_pc); std::free(reader); std::free(birth); }
    };
    Shadow data{data_low, data_high}, heap{heap_low, heap_high};
    struct Span { std::uint32_t entry, owner, sp; bool below; std::uint64_t executed; };
    std::vector<Span> spans;
    struct Match {
        std::uint32_t object = 0, caller = other, advances = 0;
        bool advanced = false, finished = false;
        std::uint64_t instructions = 0, stack_reads = 0, stack_writes = 0, own_reads = 0, own_writes = 0;
        std::uint64_t reads = 0, writes = 0;                       // outside the stack and the match object
        std::uint64_t new_words_written = 0;                       // words outside it this match had not written last
        std::uint64_t draws = 0, crt_draws = 0, allocator_calls = 0;
        std::uint64_t reads_from_match = 0, reads_from_later_driver = 0;   // distinct words another match (or the driver after one) wrote
        std::uint64_t overwrites_of_match = 0;                     // distinct words another match wrote, overwritten unread
    };
    std::vector<Match> matches;
    std::unordered_map<std::uint32_t, std::uint32_t> current;     // match object -> owner
    std::uint32_t last_finished = 0;                              // on the current day
    std::uint32_t last_finished_ever = first_match - 1;          // for the birth of allocated blocks
    std::uint64_t pending_size = 0, sized_allocations = 0, unsized_allocator_calls = 0, days = 0;
    unsigned allocator_depth = 0;
    std::uint64_t driver_calls = 0, events_logged = 0;
    std::map<std::uint32_t, std::uint64_t> entries, draws_by_owner_class, allocator_by_owner_class;
    // Flows between owners: (class of flow, writer PC, reader PC) -> uses.
    struct Flow { std::uint64_t accesses = 0, words = 0; std::uint32_t sample = 0; std::map<std::uint32_t, std::uint64_t> allocations; };
    std::map<std::tuple<int, bool, std::uint32_t, std::uint32_t>, Flow> flows;   // class: kind + 10 (through other code) + 100 (block born after the writer's match)
    // What other owners read of what matches wrote (the matches' outputs), distinct words by reader class.
    std::map<std::uint32_t, std::uint64_t> outputs_read_by;
    std::vector<std::string> log;
    // Blocks allocated while a match's advance runs: where each one is freed. A play phase on its own thread
    // must hand the survivors over to the code that frees them.
    struct Born { std::uint32_t owner, size; };
    std::unordered_map<std::uint32_t, Born> born;
    std::uint64_t born_total = 0, freed_same_advance = 0, freed_same_match = 0, freed_other_match = 0, freed_outside = 0,
                  foreign_frees = 0, born_bytes = 0, surviving_bytes = 0;
    std::map<std::uint32_t, std::uint64_t> survivor_sizes, survivor_freers;
    bool in_advance() const { return !spans.empty() && spans.back().entry == advance; }
    std::uint32_t pending_serial = 0;
    void freed(std::uint32_t block) {
        const auto found = born.find(block);
        if (found == born.end()) { if (in_advance() && block >= heap_low) ++foreign_frees; return; }
        const auto o = owner();
        if (o == found->second.owner && in_advance() && advance_serial == found->second.size >> 24) ++freed_same_advance;
        else {
            if (o == found->second.owner) ++freed_same_match; else if (match_of(o)) ++freed_other_match; else ++freed_outside;
            surviving_bytes += found->second.size & 0xffffff;
            ++survivor_sizes[found->second.size & 0xffffff];
            ++survivor_freers[s.pc];
        }
        born.erase(found);
    }
    std::uint32_t advance_serial = 0;   // low 8 bits tag the advance call a block was born in
    std::uint64_t unowned_draws = 0, draw_samples = 0;
    std::map<std::uint32_t, std::uint64_t> draw_callers;
    std::vector<std::uint8_t> stack_copy;
    const std::uint8_t* stack_bytes(std::uint32_t address) { stack_copy = s.memory.copy_out(address, 4); return stack_copy.data(); }

    std::array<std::uint32_t, 4> rng_words{};
    std::uint32_t rng_cursor = 0, date_word = 0, today = 0;
    explicit Impl(State& state) : s(state) {
        for (unsigned n = 0; n < 4; ++n) rng_words[n] = std::uint32_t(state.memory.read<4>(rng_slots + n * 4)) & ~3u;
        rng_cursor = rng_words[3];
        date_word = std::uint32_t(state.memory.read<4>(0x108b3f10));
        today = std::uint32_t(state.memory.read<2>(date_word));
    }
    std::uint32_t sp() const { return std::uint32_t(s.r<4>(4)); }
    std::uint32_t owner() const { return spans.empty() ? other : spans.back().owner; }
    Match* match_of(std::uint32_t o) { return o >= first_match && !(o & after_match) ? &matches[o - first_match] : nullptr; }
    static std::uint32_t owner_class(std::uint32_t o) { return o & after_match ? driver : std::min(o, std::uint32_t(first_match)); }

    // Spans whose stack pointer is back at (or above) its entry value have returned.
    void track() {
        const auto now = sp();
        while (!spans.empty()) {
            auto& top = spans.back();
            if (now < top.sp) { top.below = true; return; }
            if (!top.below) return;
            close(top);
            spans.pop_back();
        }
    }
    void close(const Span& span) {
        if (auto* match = match_of(span.owner)) {
            match->instructions += s.executed - span.executed;
            if (span.entry == advance) { match->advanced = true; last_finished = last_finished_ever = span.owner; }
            if (span.entry == destroy) match->finished = true;
        }
    }
    std::uint32_t match_for(std::uint32_t object, bool fresh_allowed) {
        const auto found = current.find(object);
        if (found != current.end()) {
            const auto& match = matches[found->second - first_match];
            if (!fresh_allowed || !(match.advanced || match.finished)) return found->second;
        }
        matches.push_back({});
        matches.back().object = object;
        for (auto span = spans.rbegin(); span != spans.rend(); ++span)
            if (span->owner < first_match) { matches.back().caller = span->owner; break; }
        return current[object] = first_match + std::uint32_t(matches.size() - 1);
    }
    void probe() { track(); open(s.pc, sp()); }
    // A call of a match routine: seen by the dispatcher's probe, or (the app calls C++ owners in place, past the
    // probe) by the callee's prologue saving one of the known return addresses in its caller's frame.
    void open(std::uint32_t pc, std::uint32_t at) {
        if (!spans.empty() && spans.back().entry == pc && spans.back().sp == at) return;
        std::uint32_t o;
        switch (pc) {
            case day_driver: o = driver; ++driver_calls; break;
            case scheduled: o = scheduled_caller; break;
            case watched: o = watched_caller; break;
            case statistics: o = statistics_caller; break;
            case construct: case set_up: o = match_for(std::uint32_t(s.r<4>(12)), true); break;
            case destroy: o = match_for(std::uint32_t(s.r<4>(12)), false); break;
            case advance: {
                // The day's driver reuses one object: an advance after its destroy starts the next match.
                const auto found = current.find(std::uint32_t(s.r<4>(12)));
                o = match_for(std::uint32_t(s.r<4>(12)), found != current.end() && matches[found->second - first_match].finished);
                break;
            }
            default: return;
        }
        ++entries[pc];
        if (pc == advance) { ++matches[o - first_match].advances; advance_serial = (advance_serial + 1) & 0xff; }
        if (events_logged++ < 400) {
            std::ostringstream line;
            line << Hex{pc} << " r3=" << Hex{std::uint32_t(s.r<4>(12))} << " r4=" << Hex{std::uint32_t(s.r<4>(16))} << " sp=" << Hex{at}
                 << " depth=" << spans.size() << " owner=" << (o >= first_match ? "match " + std::to_string(o - first_match) : owner_name(o));
            log.push_back(line.str());
        }
        spans.push_back({pc, o, at, false, s.executed});
    }

    Shadow* shadow(std::uint32_t address) {
        if (address >= heap_low && address < heap_high) return &heap;
        if (address >= data_low && address < data_high) return &data;
        return nullptr;
    }
    // 0: RNG words, 1: allocator (inside an allocator call), 2: everything else.
    int kind(std::uint32_t address) const {
        if (std::find(rng_words.begin(), rng_words.end(), address & ~3u) != rng_words.end() || (address & ~3u) == crt_rng) return 0;
        return allocator_depth ? 1 : 2;
    }
    void note(int flow, bool overwrite, std::uint32_t writer_pc, std::uint32_t address, bool distinct, std::uint32_t from) {
        if (address >= heap_low && heap.birth[(address - heap_low) / 4] >= from) flow += 100;
        auto& entry = flows[{flow, overwrite, writer_pc, s.pc}];
        ++entry.accesses;
        if (!distinct) return;
        ++entry.words;
        if (!entry.sample) entry.sample = address;
        if (address >= heap_low) {
            auto allocation = s.allocated.upper_bound(address);
            if (allocation != s.allocated.begin() && address - std::prev(allocation)->first < std::prev(allocation)->second)
                ++entry.allocations[std::prev(allocation)->second];
            else ++entry.allocations[0];
        }
    }
    void access(std::uint32_t start, std::uint64_t size, bool write) {
        track();
        const auto o = owner();
        auto* match = match_of(o);
        if (start >= stack_low && start < stack_high) {
            if (match) ++(write ? match->stack_writes : match->stack_reads);
            return;
        }
        if (write && start <= rng_cursor && start + size > rng_cursor) {
            ++draws_by_owner_class[owner_class(o)];
            // Where the draws outside any span come from: one in 1,024, every return address on the guest stack.
            if (!match && !(++unowned_draws & 1023)) {
                ++draw_samples;
                std::uint32_t frame = sp();
                for (unsigned depth = 0; depth < 96 && frame >= stack_low && frame + 12 <= stack_high; ++depth) {
                    const auto next = std::uint32_t(read_be<4>(stack_bytes(frame))), link = std::uint32_t(read_be<4>(stack_bytes(frame + 8)));
                    if (link >= 0x10000000 && link < 0x108b3a84) ++draw_callers[link];
                    if (next <= frame) break;
                    frame = next;
                }
            }
            if (match) ++match->draws;
        }
        if (write && start <= crt_rng && start + size > crt_rng && match) ++match->crt_draws;
        const bool own = match && start >= match->object && start < match->object + match_object_size;
        if (match) ++(own ? (write ? match->own_writes : match->own_reads) : (write ? match->writes : match->reads));
        auto* map = shadow(start);
        if (!map) return;
        // The driver's writes after a match carry that match, so a later match's read of them shows.
        const auto as_writer = !match && last_finished ? after_match | last_finished : o;
        for (auto word = (start - map->low) / 4, last = std::uint32_t((start + std::max<std::uint64_t>(size, 1) - 1 - map->low) / 4);
             word <= last && word < map->words; ++word) {
            const auto address = map->low + word * 4, previous = map->writer[word];
            const bool seen = map->reader[word] == o;
            if (write) {
                if (match && !own && previous != o) {
                    ++match->new_words_written;
                    if (match_of(previous) && !seen) { ++match->overwrites_of_match; note(kind(address), true, map->writer_pc[word], address, true, previous); }
                }
                map->writer[word] = as_writer;
                map->writer_pc[word] = s.pc;
                map->reader[word] = o;      // its own value from here on
                continue;
            }
            const auto from = previous & after_match ? (previous & ~after_match) : previous;
            if (match && previous != o && (match_of(previous) || (previous & after_match)) && from != o) {
                if (!seen) ++(previous & after_match ? match->reads_from_later_driver : match->reads_from_match);
                note(kind(address) + (previous & after_match ? 10 : 0), false, map->writer_pc[word], address, !seen, from);
            } else if (!match && match_of(previous) && !seen) {
                ++outputs_read_by[owner_class(o)];
            }
            map->reader[word] = o;
        }
    }
    void mapped(std::uint32_t, std::uint64_t) override {}
    void cleared(std::uint32_t start, std::uint64_t size) override { access(start, size, true); }
    void written(std::uint32_t start, const std::uint8_t* bytes, std::size_t size) override {
        // A new game date ends the day: matches of different days stay in order anyway.
        if (size == 4 && start >= stack_low && start < stack_high) {
            const auto value = std::uint32_t(read_be<4>(bytes));
            if (value >= 0x10119170 && value <= 0x10371d10)
                if (const auto found = std::lower_bound(call_sites.begin(), call_sites.end(), std::pair<std::uint32_t, std::uint32_t>{value, 0});
                    found != call_sites.end() && found->first == value && start - 8 == sp()) { track(); open(found->second, start - 8); }
        }
        if (start == date_word && size >= 2) {
            const auto day = std::uint32_t(bytes[0]) << 8 | bytes[1];
            if (day != today) { today = day; ++days; last_finished = 0; }
        }
        access(start, size, true);
    }
    void read(std::uint32_t start, const std::uint8_t*, unsigned size) override { access(start, size, false); }
    void enter(const State& state, std::uint32_t entry, bool) override {
        if (!allocator_depth++) {
            track();
            // The C facades give the size; blocks taken straight from a pool or arena routine are not dated.
            pending_size = entry == 0x10775130 ? state.r<4>(12) : entry == 0x107751b0 ? state.r<4>(16)
                         : entry == 0x10775200 ? state.r<4>(12) * state.r<4>(16) : 0;
            if (!pending_size && entry != 0x10775170) ++unsized_allocator_calls;
            if (entry == 0x10775170 || entry == 0x107751b0) freed(std::uint32_t(state.r<4>(12)));
            ++allocator_by_owner_class[owner_class(owner())];
            if (auto* match = match_of(owner())) ++match->allocator_calls;
        }
    }
    void leave(State& state, std::uint32_t, bool) override {
        if (!allocator_depth || --allocator_depth || !pending_size) return;
        const auto block = std::uint32_t(state.r<4>(12));
        if (block >= heap_low && block < heap_high && pending_size < 0x4000000) {
            ++sized_allocations;
            if (in_advance()) { ++born_total; born_bytes += pending_size; born[block] = {owner(), std::uint32_t(std::min<std::uint64_t>(pending_size, 0xffffff)) | advance_serial << 24}; }
            for (auto word = (block - heap_low) / 4, last = std::uint32_t((block + pending_size - 1 - heap_low) / 4); word <= last && word < heap.words; ++word)
                heap.birth[word] = last_finished_ever;
        }
        pending_size = 0;
    }
};

MatchSharing::MatchSharing(State& state) : impl(std::make_unique<Impl>(state)) { state.allocator_observer = impl.get(); }
MatchSharing::~MatchSharing() { if (impl->s.allocator_observer == impl.get()) impl->s.allocator_observer = nullptr; }
MemoryObserver* MatchSharing::observer() { return impl.get(); }
void MatchSharing::probe(State&) { impl->probe(); }

void MatchSharing::report(std::ostream& out) const {
    const auto& i = *impl;
    out << "== match sharing ==\nentries seen by the probe:";
    for (const auto& [pc, count] : i.entries) out << ' ' << Hex{pc} << '=' << count;
    out << "\ndays: " << i.days << "; dated allocations: " << i.sized_allocations << "; allocator calls without a size: " << i.unsized_allocator_calls;
    out << "\nmatches: " << i.matches.size() << "; open spans at the end: " << i.spans.size() << '\n';
    out << "first events:\n";
    for (std::size_t n = 0; n < std::min<std::size_t>(i.log.size(), 60); ++n) out << "  " << i.log[n] << '\n';
    out << "RNG draws (cursor writes) by owner:";
    for (const auto& [o, count] : i.draws_by_owner_class) out << ' ' << owner_name(o) << '=' << count;
    out << "\nreturn addresses under draws outside any match span (" << i.draw_samples << " samples):";
    {
        std::vector<std::pair<std::uint64_t, std::uint32_t>> rows;
        for (const auto& [link, count] : i.draw_callers) rows.push_back({count, link});
        std::sort(rows.rbegin(), rows.rend());
        for (std::size_t n = 0; n < std::min<std::size_t>(rows.size(), 70); ++n) out << ' ' << Hex{rows[n].second} << '=' << rows[n].first;
    }
    out << "\nblocks allocated inside advance: " << i.born_total << " (" << i.born_bytes << " bytes); freed in the same advance call "
        << i.freed_same_advance << ", later by the same match " << i.freed_same_match << ", by another match " << i.freed_other_match
        << ", by code outside matches " << i.freed_outside << ", never freed in the run " << i.born.size() << "; bytes of those freed later "
        << i.surviving_bytes << "; frees inside advance of blocks born elsewhere " << i.foreign_frees;
    out << "\n  sizes of blocks freed later:";
    { std::size_t n = 0; for (const auto& [size, count] : i.survivor_sizes) if (n++ < 24) out << ' ' << size << 'x' << count; }
    out << "\n  PCs freeing them:";
    { std::size_t n = 0; for (const auto& [pc, count] : i.survivor_freers) if (n++ < 24) out << ' ' << Hex{pc} << 'x' << count; }
    {
        std::map<std::uint32_t, std::uint64_t> sizes;
        for (const auto& [block, b] : i.born) ++sizes[b.size & 0xffffff];
        out << "\n  sizes of blocks never freed:";
        std::size_t n = 0; for (const auto& [size, count] : sizes) if (n++ < 24) out << ' ' << size << 'x' << count;
    }
    out << "\nallocator calls by owner:";
    for (const auto& [o, count] : i.allocator_by_owner_class) out << ' ' << owner_name(o) << '=' << count;
    out << "\nwords matches wrote that others then read, by reader:";
    for (const auto& [o, count] : i.outputs_read_by) out << ' ' << owner_name(o) << '=' << count;
    Impl::Match total;
    std::map<std::uint32_t, std::uint64_t> by_caller, unadvanced;
    for (const auto& m : i.matches) {
        ++by_caller[m.caller];
        if (!m.advanced) ++unadvanced[m.caller];
        total.instructions += m.instructions; total.stack_reads += m.stack_reads; total.stack_writes += m.stack_writes;
        total.own_reads += m.own_reads; total.own_writes += m.own_writes; total.reads += m.reads; total.writes += m.writes;
        total.new_words_written += m.new_words_written; total.draws += m.draws; total.crt_draws += m.crt_draws;
        total.allocator_calls += m.allocator_calls; total.reads_from_match += m.reads_from_match;
        total.reads_from_later_driver += m.reads_from_later_driver; total.overwrites_of_match += m.overwrites_of_match;
        total.advances += m.advances;
    }
    out << "\nmatches by caller:";
    for (const auto& [o, count] : by_caller) out << ' ' << owner_name(o) << '=' << count << " (never advanced: " << unadvanced[o] << ')';
    const auto row = [&](const char* name, const Impl::Match& m) {
        out << name << " instr=" << m.instructions << " advances=" << m.advances << " stack r/w=" << m.stack_reads << '/' << m.stack_writes
            << " object r/w=" << m.own_reads << '/' << m.own_writes << " outside r/w=" << m.reads << '/' << m.writes
            << " outside-words-written=" << m.new_words_written << " draws=" << m.draws << " crt-draws=" << m.crt_draws
            << " allocator=" << m.allocator_calls << " words-from-other-match=" << m.reads_from_match
            << " words-from-code-after-match=" << m.reads_from_later_driver << " overwrites-of-other-match=" << m.overwrites_of_match << '\n';
    };
    // Full matches (minute by minute) against quick ones, by caller: a quick one draws about 20,000 numbers.
    for (const auto& [o, count] : by_caller) {
        std::uint64_t full = 0, full_draws = 0, quick_draws = 0, slices = 0, full_allocator = 0;
        for (const auto& m : i.matches)
            if (m.caller == o) {
                if (m.draws >= 100000) { ++full; full_draws += m.draws; slices += m.advances; full_allocator += m.allocator_calls; }
                else quick_draws += m.draws;
            }
        out << "\n  " << owner_name(o) << ": full=" << full << " (draws " << full_draws << ", advance calls " << slices << ", allocator calls "
            << full_allocator << ") quick=" << count - full << " (draws " << quick_draws << ")";
    }
    out << "\n";
    row("TOTAL", total);
    for (std::size_t n = 0; n < i.matches.size(); ++n)
        if (n < 12 || n + 3 >= i.matches.size()) {
            const auto name = "match " + std::to_string(n) + " (" + owner_name(i.matches[n].caller) + ", object " +
                              hex_text(i.matches[n].object) + ")";
            row(name.c_str(), i.matches[n]);
        }
    static const char* const kinds[] = {"RNG words", "allocator", "game data"};
    for (const int flow : {2, 102, 12, 112, 1, 101, 11, 111, 0, 10})
        for (const bool overwrite : {false, true}) {
            std::vector<std::pair<std::tuple<int, bool, std::uint32_t, std::uint32_t>, const Impl::Flow*>> rows;
            std::uint64_t words = 0, accesses = 0;
            for (const auto& [key, value] : i.flows)
                if (std::get<0>(key) == flow && std::get<1>(key) == overwrite) { rows.push_back({key, &value}); words += value.words; accesses += value.accesses; }
            if (rows.empty()) continue;
            std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) { return a.second->words > b.second->words; });
            out << "\n-- " << kinds[flow % 10] << ": a match " << (overwrite ? "overwrites unread" : "reads") << " what "
                << (flow % 100 >= 10 ? "other code wrote after another match" : "another match wrote")
                << (flow >= 100 ? ", in a block allocated after that match" : flow % 10 ? "" : "") << ": " << words << " distinct words, "
                << accesses << " accesses, " << rows.size() << " (writer PC, reader PC) pairs --\n";
            for (std::size_t n = 0; n < std::min<std::size_t>(rows.size(), flow == 12 || flow == 2 ? 400 : 12); ++n) {
                const auto& [key, value] = rows[n];
                out << "  writer " << Hex{std::get<2>(key)} << " -> " << (overwrite ? "writer " : "reader ") << Hex{std::get<3>(key)} << ": words="
                    << value->words << " accesses=" << value->accesses << " sample=" << Hex{value->sample} << " allocation sizes:";
                std::size_t shown = 0;
                for (const auto& [size, count] : value->allocations) if (shown++ < 4) out << ' ' << size << "x" << count;
                out << '\n';
            }
        }
}
}
