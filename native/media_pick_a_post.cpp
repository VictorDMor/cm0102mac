// Story 0xbd9: a request (player, club, second player, flag, kind) the caller built on its stack
// becomes an item with the persons around it, is posted, and (for an ordinary request) logged.
#include "media_pick_a.hpp"
#include "media_pick_a_detail.hpp"
#include "media_pick_b.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;
using namespace values;
using namespace detail::pick;

void post_item_bd9(Cpu& c, std::uint32_t self, std::uint32_t request_at, std::uint32_t involved) {
    Frame frame(c, 0x10655d60, 0x200, {.first_gpr = 16});
    const auto rng = Stream::world(c);
    const Request request{c, request_at};
    Program program{c};
    // r16 is the persons table until the story roll replaces it; r17 the stream's end until the loop index.
    Kept<16> persons(c, c.toc(-0x7be0));
    Kept<17> index(c, rng.end);
    Kept<27> second(c, 0);
    keep(c, {{18, 0}, {19, c.toc(-0x7be8)}, {20, c.toc(-0x7b40)}, {21, c.toc(-0x7570)}, {22, rng.start}, {23, rng.seed},
             {24, c.toc(-0x7bf0)}, {25, self}, {26, rng.cursor}, {29, involved}, {31, request_at}});
    const Counter counter{c, frame.at(0x38)};
    c.put8(counter.at, 0);
    const auto today = frame.at(0x3c);
    program.call(0x10715880, 0x10655db8, {today});
    if (!nonzero_logical(c, request_at)) return;
    const auto player_at = [&] { return c.u32(persons) + request.player().get() * player_size; };
    const auto profile = [&] { return Person{c, player_at()}.player(); };
    c.set(3, player_at());
    if (!nonzero_logical(c, profile())) return;
    c.set(3, profile());
    if (compare(c, Person{c, player_at()}.player_record().reputation(), 0xdac) < 0) return;
    if (compare(c, request.club().get()) < 0) return;
    c.set({{3, c.toc(-0x7b50)}, {4, std::uint32_t(request.club().get())}});
    if (compare(c, request.club().get(), c.i32(c.toc(-0x7b50))) > 0) return;
    const Club club{c, c.u32(c.r(19)) + std::uint32_t(request.club().get()) * club_size};
    c.set(3, c.u32(c.r(19)));
    Kept<30> club_at(c, club.at);
    if (!nonzero_logical(c, club.nation())) return;
    Kept<28> player(c, player_at());
    c.set(3, request.player().get() * player_size);
    c.set(4, c.u32(persons));
    if (compare(c, request.second().get(), -1) > 0) second = c.u32(persons) + std::uint32_t(request.second().get()) * player_size;
    const std::uint32_t kind = std::int32_t(request.kind().get());
    const bool plain = compare(c, std::int32_t(request.flag().get())) == 0;
    if (plain) {
        const auto known = program.module_call(0x1069e700, 0x10655e9c, {self, player, club.at, 0xbd9, kind, 0});
        if (compare(c, byte_word(known)) != 0) return;
    }
    if (!plain && !nonzero_logical(c, second)) return;
    const Item item{c, frame.at(0xd8)};
    const Fields fields{item, counter};
    item.begin(0xbd9, 0x10655ec8);
    fields.put(request.player().get(), 0x10655ee0);
    // The fields the original loads after stepping the counter: a null link faults with the byte stepped.
    put_names_loaded(c, fields, player, {0x10655efc, 0x10655f18, 0x10655f34});
    fields.put(std::uint32_t(request.club().get()), 0x10655f4c);
    fields.put(std::uint32_t(std::int32_t(request.amount().get())), 0x10655f68);
    fields.put(kind, 0x10655f80);
    rng.advance(0x10655fa0);
    fields.put_loaded([&] { return std::uint32_t(std::int32_t(rng.sample()) % 2); }, 0x10655fdc);
    fields.put(std::uint32_t(std::int32_t(request.flag().get())), 0x10655ff8);
    fields.put(0x1e, 0x10656010);
    const auto accepts = [&](std::uint32_t back) { return byte_word(program.call(0x1014c2e0, back, {club.at, 0, 1, 0, 0})); };
    if (compare(c, accepts(0x10656028)) == 0 && compare(c, std::int32_t(request.flag().get())) == 0) {
        const auto verdict = program.module_call(0x106584d0, 0x10656050, {self, club.manager(), player, kind});
        fields.put(verdict, 0x10656068);
    } else {
        fields.put(0, 0x10656084);
    }
    if (compare(c, request.second().get(), -1) > 0) {
        fields.put(std::uint32_t(request.second().get()), 0x106560a8);
        put_names_loaded(c, fields, second, {0x106560c4, 0x106560e0, 0x106560fc});
    } else {
        put_no_person(fields, {0x10656118, 0x10656130, 0x10656148, 0x10656160});
    }
    if (nonzero_logical(c, involved)) {
        fields.put(1, 0x10656180);
        program.call(0x1030d4a0, 0x10656190, {involved, club.at, today});
        program.call(0x1030dcd0, 0x106561a0, {today, item.at, counter.at});
        // The involved person's club changed: 1 when it is this club, 2 when another.
        put_result(c, fields, Fixture{c, involved}, club.at, {0x106561c8, 0x10656208, 0x10656224, 0x10656264, 0x10656280, 0});
    } else {
        fields.put(0, 0x1065629c);
    }
    persons = program.module_call(0x1069eb10, 0x106562a4, {player});
    fields.put(persons, 0x106562c0);
    std::uint32_t reaction;
    if (nonzero_logical(c, club.nation()))
        reaction = program.module_call(0x106a95c0, 0x106562e4, {c.u32(c.r(21)), club.at, std::uint32_t(byte_word(persons)), c.u32(club.nation() + 0x68)});
    else
        reaction = program.module_call(0x106a95c0, 0x106562fc, {c.u32(c.r(21)), club.at, std::uint32_t(byte_word(persons)), none});
    fields.put(reaction, 0x10656314);
    if (compare(c, std::int32_t(request.flag().get())) != 0) {
        item.post(club.at, 0x1065633c);
    } else {
        std::uint32_t count = 0;
        const auto list = frame.at(0x60);
        if (nonzero_logical(c, c.u32(c.r(20)))) count = program.call(0x103fc710, 0x10656364, {c.u32(c.r(20)), player, 0x1a, list, 0x1e});
        else c.set(3, 0);
        persons = count;
        for (index = 0; compare(c, byte_word(index), byte_word(persons)) < 0; index = index + 1)
            item.post(c.u32(list + std::uint32_t(byte_word(index)) * 4), 0x10656394);
    }
    if (compare(c, std::int32_t(request.flag().get())) == 0) {
        enter(c, 0x106563d0, {self, kind, player, club.at, 0, none});
        weigh_story_targets(c, kind, player, club.at, 0, none);
        program.module_call(0x1069e190, 0x106563f0, {self, player, club.at, 0xbd9, std::uint32_t(byte_word(kind)), 0});
    }
    if (compare(c, accepts(0x10656408)) == 0 && compare(c, std::int32_t(request.flag().get())) == 0)
        program.module_call(0x10677d60, 0x10656434, {self, player, club.at, kind, std::uint32_t(byte_word(c.r(18)))});
}
}
