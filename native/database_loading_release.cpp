// DATABASE.CPP's loading block, part "release": 0x100a4e50 frees and clears every loaded table of the
// database, then the language object (database_loading.hpp).
//
// One frame of 0x90 bytes saves r13-r31, and the original keeps TOC words in them for the whole routine:
// each is the address of a table's base word. Registers it holds at each call are what the callee's prologue
// saves into the stack image, so they stay in the register file. The "flagged" blocks free through one of two
// identical calls by the byte at TOC -0x39d0, read again each time.
#include "database_loading.hpp"
#include "database_loading_detail.hpp"
#include "guest_values.hpp"
#include <array>

namespace cm::aot::database {
namespace {
using guest::compare;
using guest::compare_logical;
namespace callee = loading::callee;
namespace toc = loading::toc;

// r13-r31 as the prologue loads them: the TOC slot of each table's base word.
struct Held {
    unsigned reg;
    std::int32_t slot;
};
constexpr Held held[]{{24, -0x7520}, {14, -0x7548}, {15, -0x7544}, {16, -0x7540}, {17, -0x7bbc}, {18, -0x753c},
                      {19, -0x7538}, {20, -0x7534}, {21, -0x7530}, {22, -0x752c}, {23, -0x7ab4}, {25, -0x7518},
                      {26, -0x74f8}, {13, -0x7a34}, {27, -0x7b98}, {28, -0x74cc}, {29, -0x74c4}, {30, -0x79f8},
                      {31, -0x7b38}};

class Release {
public:
    explicit Release(Cpu& cpu) : c(cpu) {}

    // "if (*word) { free(*word) ...": tests the word's content (cmplwi), then calls the allocator's release
    // with the word's content. `site` is the address of the call (the flagged site where there are two).
    bool occupied(std::uint32_t word) {
        const auto block = c.u32(word);
        c.set(0, block);
        return compare_logical(c, block) != 0;
    }
    void release_at(std::uint32_t word, std::uint32_t back) {
        c.set(3, c.u32(word));
        c.call(callee::release, back);
    }
    // The same through the flag's two call sites: `site` is the first call; the second is 0xc further.
    void release_flagged(std::uint32_t word, std::uint32_t site) { release_at(word, flag_set() ? site + 4 : site + 0x10); }
    bool flag_set() {
        const auto flag = std::uint32_t(std::int32_t(std::int8_t(c.u8(c.r(2) + std::uint32_t(toc::release_flag)))));
        c.set(0, flag);
        return compare(c, std::int32_t(flag)) != 0;
    }
    // A base word reached through a TOC slot that points at it: tested (r3 the slot's word), then released and
    // cleared with its count word (through the TOC slot `mirror`).
    bool indirect_occupied(std::int32_t slot) {
        c.set(3, c.toc(slot));
        const auto block = c.u32(c.r(3));
        c.set(0, block);
        return compare_logical(c, block) != 0;
    }
    void release_indirect(std::int32_t slot, std::int32_t mirror, std::uint32_t back) {
        if (!indirect_occupied(slot)) return;
        c.set(3, c.toc(slot));
        c.set(3, c.u32(c.r(3)));
        c.call(callee::release, back);
        c.set({{4, c.toc(mirror)}, {0, 0}, {3, c.toc(slot)}, {5, 0}});
        c.put32(c.r(4), c.r(0));
        c.put32(c.r(3), c.r(5));
    }
    // The zeroing after a release in the first part: the count word's address in r3 (and r4 = 0).
    void cleared_with_mirror(unsigned reg, std::int32_t mirror) {
        c.set(3, c.toc(mirror));
        c.set({{4, 0}, {0, 0}});
        c.put32(c.r(reg), c.r(4));
        c.put32(c.r(3), c.r(0));
    }

private:
    Cpu& c;
};
}

void release_all_tables(Cpu& c) {
    constexpr std::uint32_t entry = 0x100a4e50;
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x90, {.first_gpr = 13});
    for (const auto& [reg, slot] : held) c.set(reg, c.toc(slot));
    Release release(c);
    const auto word_of = [&](unsigned reg) { return c.r(reg); };

    // The first three tables free through the flagged call sites.
    if (release.occupied(word_of(24))) {
        release.release_flagged(word_of(24), 0x100a4ec8);
        release.cleared_with_mirror(24, -0x7524);
    }
    if (release.occupied(word_of(29))) {
        release.release_flagged(word_of(29), 0x100a4f08);
        release.cleared_with_mirror(29, -0x74c8);
    }
    if (release.occupied(word_of(28))) {
        release.release_flagged(word_of(28), 0x100a4f48);
        release.cleared_with_mirror(28, -0x74d0);
    }
    if (release.occupied(word_of(13))) {
        release.release_at(word_of(13), 0x100a4f80);
        release.cleared_with_mirror(13, -0x7a38);
    }
    release.release_indirect(-0x74bc, -0x74c0, 0x100a4fb0);
    {
        // The word in the TOC itself (-0x39e0) with its length beside it (-0x39dc).
        const auto word = c.r(2) + std::uint32_t(toc::language_object + 4);
        if (release.occupied(word)) {
            release.release_at(word, 0x100a4fdc);
            c.set({{3, 0}, {0, 0}});
            c.put32(word, c.r(3));
            c.put32(word + 4, c.r(0));
        }
    }
    release.release_indirect(-0x7be0, -0x7be4, 0x100a5008);
    if (release.occupied(word_of(31))) {
        release.release_at(word_of(31), 0x100a5034);
        c.set({{3, 0}, {0, 0}});
        c.put32(word_of(31), c.r(3));
        c.set(3, c.toc(-0x74d4));
        c.put32(c.r(3), c.r(0));
    }
    if (release.occupied(word_of(30))) {
        release.release_at(word_of(30), 0x100a505c);
        c.set({{3, 0}, {0, 0}});
        c.put32(word_of(30), c.r(3));
        c.set(3, c.toc(-0x79f4));
        c.put32(c.r(3), c.r(0));
    }
    if (release.occupied(word_of(23))) {
        release.release_at(word_of(23), 0x100a5084);
        release.cleared_with_mirror(23, -0x7528);
    }
    if (release.indirect_occupied(-0x7be8)) {
        c.set(3, c.toc(-0x7be8));
        c.set(3, c.u32(c.r(3)));
        c.call(callee::release, 0x100a50b4);
        c.set({{7, c.toc(-0x7b50)}, {8, 0}, {5, c.toc(-0x74fc)}, {6, 0}, {4, c.toc(-0x7500)}, {0, 0}});
        c.set({{3, c.toc(-0x7be8)}, {9, 0}});
        c.put32(c.r(7), c.r(8));
        c.put32(c.r(3), c.r(9));
        c.put32(c.r(5), c.r(6));
        c.put32(c.r(4), c.r(0));
    }

    if (release.flag_set()) {
        // The flag set: nothing is freed here; the words are cleared through registers that hold zeros.
        c.set({{4, 0}, {28, 0}});
        c.put32(word_of(31), c.r(4));
        c.set({{24, 0}, {4, c.toc(-0x754c)}, {3, 0}, {23, 0}, {13, 0}});
        c.put32(c.r(4), c.r(3));
        c.set({{0, 0}, {3, c.toc(-0x7550)}, {12, 0}, {29, 0}, {11, 0}});
        c.put32(c.r(3), c.r(0));
        c.set({{10, 0}, {3, c.toc(-0x7554)}, {0, 0}, {9, 0}, {8, 0}});
        c.put32(c.r(3), c.r(29));
        c.set({{7, 0}, {3, c.toc(-0x7558)}, {6, 0}, {5, 0}});
        c.put32(word_of(22), c.r(28));
        c.put32(c.r(3), c.r(0));
        // Each following pair: r0 = 0, r3 = the count word's address, a stored register, then 0 into r3's word.
        struct Pair {
            std::int32_t slot;
            unsigned word, value;
        };
        for (const auto& [slot, word, value] : std::array<Pair, 8>{{{-0x755c, 21, 24}, {-0x7b64, 20, 23}, {-0x7560, 19, 13},
                                                                  {-0x751c, 18, 12}, {-0x7564, 17, 11}, {-0x7b04, 16, 10},
                                                                  {-0x7568, 25, 9},  {-0x79f4, 15, 8}}}) {
            c.set({{0, 0}, {3, c.toc(slot)}});
            c.put32(word_of(word), c.r(value));
            c.put32(c.r(3), c.r(0));
        }
        c.set({{0, 0}, {3, c.toc(-0x74d4)}});
        c.put32(word_of(27), c.r(7));
        c.put32(word_of(14), c.r(6));
        c.put32(word_of(30), c.r(5));
        c.put32(c.r(3), c.r(0));
    } else {
        struct Block {
            unsigned word;
            std::int32_t mirror;
            std::uint32_t site;
        };
        for (const auto& [word, mirror, site] : std::array<Block, 13>{{{22, -0x754c, 0x100a5214}, {21, -0x7550, 0x100a5254},
                                                                       {20, -0x7554, 0x100a5294}, {19, -0x7558, 0x100a52d4},
                                                                       {18, -0x755c, 0x100a5314}, {17, -0x7b64, 0x100a5354},
                                                                       {16, -0x7560, 0x100a5394}, {25, -0x751c, 0x100a53d4},
                                                                       {15, -0x7564, 0x100a5414}, {27, -0x7b04, 0x100a5454},
                                                                       {14, -0x7568, 0x100a5494}, {30, -0x79f4, 0x100a54d4},
                                                                       {31, -0x74d4, 0x100a5514}}}) {
            if (!release.occupied(word_of(word))) continue;
            release.release_flagged(word_of(word), site);
            c.set({{3, 0}, {0, 0}});
            c.put32(word_of(word), c.r(3));
            c.set(3, c.toc(mirror));
            c.put32(c.r(3), c.r(0));
        }
    }

    // The tail: one more plain word, then the language object, then the language byte back to -1.
    if (release.occupied(word_of(26))) {
        release.release_at(word_of(26), 0x100a554c);
        c.set(0, 0);
        c.put32(word_of(26), c.r(0));
    }
    const auto object_slot = c.r(2) + std::uint32_t(toc::language_object);
    const auto object = c.u32(object_slot);
    c.set(0, object);
    if (compare_logical(c, object) != 0) {
        c.set(3, c.u32(object_slot));
        guest::enter(c, 0x100a556c, {c.r(3), 1});
        release_language_object(c, c.r(3), 1);
        c.set(0, 0);
        c.put32(object_slot, c.r(0));
    }
    const auto language_byte = c.toc(toc::language_byte);
    c.set({{4, language_byte}, {0, 0xffffffff}, {3, c.toc(toc::string_pool)}});
    c.put8(c.r(4), c.r(0));
    c.set(3, c.r(3) + 0xc1);
    c.call(callee::delete_temporary_files, 0x100a558c);
}
}
