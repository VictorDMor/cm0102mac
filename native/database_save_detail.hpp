#pragma once
// DATABASE.CPP's save block: the table of file sections the database is written to and read from (0x100abfb0).
// The routine is one pass over 22 sections (a directory entry id, the record table it describes, a callee that
// writes or reads that table), each with the same steps; this header holds what differs per section.
#include <array>
#include <cstdint>

namespace cm::aot::database::save {
// Call sites (the address of the `bl`) the routine reaches in a section; the callee saves the address after it as
// its return address. Members that a section does not have are 0.
struct Sites {
    std::uint32_t memcpy, say, assertion;  // directory entry copy; the "table valid, but no entry" report
    std::uint32_t rename_compare = 0, rename_copy = 0;  // sections that may rename their file
    std::uint32_t open_write, open_read, open_fail_resolve;
    std::uint32_t size_write = 0, size_read = 0;  // sections that record the stream's size
    std::uint32_t callee;                          // the table's own writer/reader
    std::uint32_t fail_close_write, fail_close_read, fail_resolve;
    std::uint32_t callee2 = 0, fail2_close_write = 0, fail2_close_read = 0, fail2_resolve = 0;
    std::uint32_t ok_close_write, ok_close_read;
    std::uint32_t progress;
};

// One section. Offsets are TOC offsets (the word there is the address of a global) unless noted.
struct Section {
    std::uint32_t id;      // directory entry id looked up at entry +0x34
    std::int32_t table;    // the global holding the table's first record (0: the table does not exist)
    std::int32_t count;    // the global holding the record count written to the directory
    std::int32_t minus;    // 0, or a second count subtracted from it (the table is split between two files)
    bool minus_global;     // also subtract the loose word at TOC -0x39d4
    std::uint32_t type;    // stored in the directory entry at +0x40 (1 or 2)
    std::uint32_t callee;  // the writer/reader of the table (stream, direction, first record, count)
    std::uint32_t format;  // offset in the string pool of "... table valid, but no entry in %s"
    std::uint32_t name, extension, line;  // the assertion's two stack buffers (frame offsets) and line number
    std::uint32_t fail_slot, fail2_slot, ok_slot;  // frame slots that hold the stream handle for each close
    Sites sites;
    std::uint32_t also_id = 0;          // a second id that selects this section's entry
    bool writes_id = false;             // stores 0x16 in the entry's id after opening
    bool marks_people = false;          // remembers the entry's position (the people section)
    std::uint32_t rename_format = 0;    // offset of the file name that replaces one equal to the people section's
    bool queries_size = false;          // records the stream's size in the entry (+0x3c)
    bool second_pass = false;           // writes a second part of the table when a loose count is positive
    bool last = false;                  // the last section: no entry advance, progress count not advanced
};

inline constexpr std::array sections{
    Section{.id = 0x3, .table = -0x7520, .count = -0x7524, .minus = 0x0, .minus_global = false, .type = 1, .callee = 0x105d76e0,
            .format = 0x1f6, .name = 0x4ac, .extension = 0x204, .line = 0x101d, .fail_slot = 0x154, .fail2_slot = 0, .ok_slot = 0x150,
            .sites = {.memcpy=0x100ac090, .say=0x100ac0c8, .assertion=0x100ac0e0, .open_write=0x100ac154, .open_read=0x100ac16c, .open_fail_resolve=0x100ac1b0, .callee=0x100ac208, .fail_close_write=0x100ac22c, .fail_close_read=0x100ac238, .fail_resolve=0x100ac24c, .ok_close_write=0x100ac268, .ok_close_read=0x100ac274, .progress=0x100ac2a0}},
    Section{.id = 0x7, .table = -0x7ab4, .count = -0x7528, .minus = 0x0, .minus_global = false, .type = 1, .callee = 0x105dfa10,
            .format = 0x221, .name = 0x48c, .extension = 0x1fc, .line = 0x104e, .fail_slot = 0x148, .fail2_slot = 0, .ok_slot = 0x144,
            .sites = {.memcpy=0x100ac2d8, .say=0x100ac310, .assertion=0x100ac328, .open_write=0x100ac39c, .open_read=0x100ac3b4, .open_fail_resolve=0x100ac3f8, .callee=0x100ac450, .fail_close_write=0x100ac474, .fail_close_read=0x100ac480, .fail_resolve=0x100ac494, .ok_close_write=0x100ac4b0, .ok_close_read=0x100ac4bc, .progress=0x100ac4e8}},
    Section{.id = 0xd, .table = -0x752c, .count = -0x754c, .minus = 0x0, .minus_global = false, .type = 1, .callee = 0x105e01d0,
            .format = 0x24b, .name = 0x46c, .extension = 0x1f4, .line = 0x1080, .fail_slot = 0x13c, .fail2_slot = 0, .ok_slot = 0x138,
            .sites = {.memcpy=0x100ac520, .say=0x100ac558, .assertion=0x100ac570, .open_write=0x100ac5e4, .open_read=0x100ac5fc, .open_fail_resolve=0x100ac640, .callee=0x100ac698, .fail_close_write=0x100ac6bc, .fail_close_read=0x100ac6c8, .fail_resolve=0x100ac6dc, .ok_close_write=0x100ac6f8, .ok_close_read=0x100ac704, .progress=0x100ac730}},
    Section{.id = 0xe, .table = -0x7530, .count = -0x7550, .minus = 0x0, .minus_global = false, .type = 1, .callee = 0x105e01d0,
            .format = 0x277, .name = 0x44c, .extension = 0x1ec, .line = 0x10b2, .fail_slot = 0x130, .fail2_slot = 0, .ok_slot = 0x12c,
            .sites = {.memcpy=0x100ac768, .say=0x100ac7a0, .assertion=0x100ac7b8, .open_write=0x100ac82c, .open_read=0x100ac844, .open_fail_resolve=0x100ac888, .callee=0x100ac8e0, .fail_close_write=0x100ac904, .fail_close_read=0x100ac910, .fail_resolve=0x100ac924, .ok_close_write=0x100ac940, .ok_close_read=0x100ac94c, .progress=0x100ac978}},
    Section{.id = 0xf, .table = -0x7534, .count = -0x7554, .minus = 0x0, .minus_global = false, .type = 1, .callee = 0x105e01d0,
            .format = 0x2a4, .name = 0x42c, .extension = 0x1e4, .line = 0x10e4, .fail_slot = 0x124, .fail2_slot = 0, .ok_slot = 0x120,
            .sites = {.memcpy=0x100ac9b0, .say=0x100ac9ec, .assertion=0x100aca04, .open_write=0x100aca7c, .open_read=0x100aca94, .open_fail_resolve=0x100acad8, .callee=0x100acb34, .fail_close_write=0x100acb58, .fail_close_read=0x100acb64, .fail_resolve=0x100acb78, .ok_close_write=0x100acb94, .ok_close_read=0x100acba0, .progress=0x100acbcc}},
    Section{.id = 0x15, .table = -0x7538, .count = -0x7558, .minus = 0x0, .minus_global = false, .type = 2, .callee = 0x105d8ad0,
            .format = 0x2d1, .name = 0x40c, .extension = 0x1dc, .line = 0x1116, .fail_slot = 0x118, .fail2_slot = 0, .ok_slot = 0x114,
            .sites = {.memcpy=0x100acc04, .say=0x100acc40, .assertion=0x100acc58, .open_write=0x100accd0, .open_read=0x100acce8, .open_fail_resolve=0x100acd2c, .callee=0x100acd88, .fail_close_write=0x100acdac, .fail_close_read=0x100acdb8, .fail_resolve=0x100acdcc, .ok_close_write=0x100acde8, .ok_close_read=0x100acdf4, .progress=0x100ace20}},
    Section{.id = 0x0, .table = -0x7be8, .count = -0x7b50, .minus = -0x7500, .minus_global = false, .type = 2, .callee = 0x105da670,
            .format = 0x2f8, .name = 0x3ec, .extension = 0x1d4, .line = 0x1148, .fail_slot = 0x10c, .fail2_slot = 0, .ok_slot = 0x108,
            .sites = {.memcpy=0x100ace58, .say=0x100ace94, .assertion=0x100aceac, .open_write=0x100acf24, .open_read=0x100acf3c, .open_fail_resolve=0x100acf80, .callee=0x100acfe8, .fail_close_write=0x100ad00c, .fail_close_read=0x100ad018, .fail_resolve=0x100ad02c, .ok_close_write=0x100ad048, .ok_close_read=0x100ad054, .progress=0x100ad080}},
    Section{.id = 0x1, .table = -0x74fc, .count = -0x7500, .minus = 0x0, .minus_global = false, .type = 2, .callee = 0x105da670,
            .format = 0x31e, .name = 0x3cc, .extension = 0x1cc, .line = 0x117a, .fail_slot = 0x100, .fail2_slot = 0, .ok_slot = 0xfc,
            .sites = {.memcpy=0x100ad0b8, .say=0x100ad0f4, .assertion=0x100ad10c, .open_write=0x100ad184, .open_read=0x100ad19c, .open_fail_resolve=0x100ad1e0, .callee=0x100ad23c, .fail_close_write=0x100ad260, .fail_close_read=0x100ad26c, .fail_resolve=0x100ad280, .ok_close_write=0x100ad29c, .ok_close_read=0x100ad2a8, .progress=0x100ad2d4}},
    Section{.id = 0xb, .table = -0x753c, .count = -0x755c, .minus = 0x0, .minus_global = false, .type = 2, .callee = 0x105df1a0,
            .format = 0x34b, .name = 0x3ac, .extension = 0x1c4, .line = 0x11ac, .fail_slot = 0xf4, .fail2_slot = 0, .ok_slot = 0xf0,
            .sites = {.memcpy=0x100ad30c, .say=0x100ad348, .assertion=0x100ad360, .open_write=0x100ad3d8, .open_read=0x100ad3f0, .open_fail_resolve=0x100ad434, .callee=0x100ad490, .fail_close_write=0x100ad4b4, .fail_close_read=0x100ad4c0, .fail_resolve=0x100ad4d4, .ok_close_write=0x100ad4f0, .ok_close_read=0x100ad4fc, .progress=0x100ad528}},
    Section{.id = 0xc, .table = -0x7bbc, .count = -0x7b64, .minus = -0x7560, .minus_global = true, .type = 2, .callee = 0x105de9b0,
            .format = 0x377, .name = 0x38c, .extension = 0x1bc, .line = 0x11de, .fail_slot = 0xe8, .fail2_slot = 0, .ok_slot = 0xe4,
            .sites = {.memcpy=0x100ad560, .say=0x100ad59c, .assertion=0x100ad5b4, .open_write=0x100ad62c, .open_read=0x100ad644, .open_fail_resolve=0x100ad688, .callee=0x100ad6f8, .fail_close_write=0x100ad71c, .fail_close_read=0x100ad728, .fail_resolve=0x100ad73c, .ok_close_write=0x100ad758, .ok_close_read=0x100ad764, .progress=0x100ad790}},
    Section{.id = 0x10, .table = -0x7540, .count = -0x7560, .minus = 0x0, .minus_global = false, .type = 2, .callee = 0x105de9b0,
            .format = 0x3a2, .name = 0x36c, .extension = 0x1b4, .line = 0x1210, .fail_slot = 0xdc, .fail2_slot = 0xd8, .ok_slot = 0xd4,
            .sites = {.memcpy=0x100ad7c8, .say=0x100ad804, .assertion=0x100ad81c, .open_write=0x100ad894, .open_read=0x100ad8ac, .open_fail_resolve=0x100ad8f0, .callee=0x100ad94c, .fail_close_write=0x100ad970, .fail_close_read=0x100ad97c, .fail_resolve=0x100ad990, .callee2=0x100ad9d4, .fail2_close_write=0x100ad9f8, .fail2_close_read=0x100ada04, .fail2_resolve=0x100ada18, .ok_close_write=0x100ada34, .ok_close_read=0x100ada40, .progress=0x100ada6c}, .second_pass = true},
    Section{.id = 0x12, .table = -0x74cc, .count = -0x74d0, .minus = 0x0, .minus_global = false, .type = 1, .callee = 0x105df5c0,
            .format = 0x3cf, .name = 0x34c, .extension = 0x1ac, .line = 0x124a, .fail_slot = 0xcc, .fail2_slot = 0, .ok_slot = 0xc8,
            .sites = {.memcpy=0x100adaa4, .say=0x100adae0, .assertion=0x100adaf8, .open_write=0x100adb70, .open_read=0x100adb88, .open_fail_resolve=0x100adbcc, .callee=0x100adc28, .fail_close_write=0x100adc4c, .fail_close_read=0x100adc58, .fail_resolve=0x100adc6c, .ok_close_write=0x100adc88, .ok_close_read=0x100adc94, .progress=0x100adcc0}},
    Section{.id = 0x13, .table = -0x7a34, .count = -0x7a38, .minus = -0x751c, .minus_global = false, .type = 1, .callee = 0x105ded70,
            .format = 0x402, .name = 0x32c, .extension = 0x1a4, .line = 0x127c, .fail_slot = 0xc0, .fail2_slot = 0, .ok_slot = 0xbc,
            .sites = {.memcpy=0x100adcf8, .say=0x100add34, .assertion=0x100add4c, .open_write=0x100addc4, .open_read=0x100adddc, .open_fail_resolve=0x100ade20, .callee=0x100ade88, .fail_close_write=0x100adeac, .fail_close_read=0x100adeb8, .fail_resolve=0x100adecc, .ok_close_write=0x100adee8, .ok_close_read=0x100adef4, .progress=0x100adf20}},
    Section{.id = 0x14, .table = -0x7518, .count = -0x751c, .minus = 0x0, .minus_global = false, .type = 1, .callee = 0x105ded70,
            .format = 0x434, .name = 0x30c, .extension = 0x19c, .line = 0x12ae, .fail_slot = 0xb4, .fail2_slot = 0, .ok_slot = 0xb0,
            .sites = {.memcpy=0x100adf58, .say=0x100adf94, .assertion=0x100adfac, .open_write=0x100ae024, .open_read=0x100ae03c, .open_fail_resolve=0x100ae080, .callee=0x100ae0dc, .fail_close_write=0x100ae100, .fail_close_read=0x100ae10c, .fail_resolve=0x100ae120, .ok_close_write=0x100ae13c, .ok_close_read=0x100ae148, .progress=0x100ae174}},
    Section{.id = 0x2, .table = -0x7544, .count = -0x7564, .minus = 0x0, .minus_global = false, .type = 1, .callee = 0x105dfdf0,
            .format = 0x468, .name = 0x2ec, .extension = 0x194, .line = 0x12e0, .fail_slot = 0xa8, .fail2_slot = 0, .ok_slot = 0xa4,
            .sites = {.memcpy=0x100ae1ac, .say=0x100ae1e8, .assertion=0x100ae200, .open_write=0x100ae278, .open_read=0x100ae290, .open_fail_resolve=0x100ae2d4, .callee=0x100ae330, .fail_close_write=0x100ae354, .fail_close_read=0x100ae360, .fail_resolve=0x100ae374, .ok_close_write=0x100ae390, .ok_close_read=0x100ae39c, .progress=0x100ae3c8}},
    Section{.id = 0x4, .table = -0x7b98, .count = -0x7b04, .minus = 0x0, .minus_global = false, .type = 2, .callee = 0x105d8560,
            .format = 0x490, .name = 0x2cc, .extension = 0x18c, .line = 0x1312, .fail_slot = 0x9c, .fail2_slot = 0, .ok_slot = 0x98,
            .sites = {.memcpy=0x100ae400, .say=0x100ae43c, .assertion=0x100ae454, .open_write=0x100ae4cc, .open_read=0x100ae4e4, .open_fail_resolve=0x100ae528, .callee=0x100ae584, .fail_close_write=0x100ae5a8, .fail_close_read=0x100ae5b4, .fail_resolve=0x100ae5c8, .ok_close_write=0x100ae5e4, .ok_close_read=0x100ae5f0, .progress=0x100ae61c}},
    Section{.id = 0x5, .table = -0x7548, .count = -0x7568, .minus = 0x0, .minus_global = false, .type = 1, .callee = 0x105d8ef0,
            .format = 0x1f6, .name = 0x2ac, .extension = 0x184, .line = 0x1344, .fail_slot = 0x90, .fail2_slot = 0, .ok_slot = 0x8c,
            .sites = {.memcpy=0x100ae654, .say=0x100ae690, .assertion=0x100ae6a8, .open_write=0x100ae720, .open_read=0x100ae738, .open_fail_resolve=0x100ae77c, .callee=0x100ae7d8, .fail_close_write=0x100ae7fc, .fail_close_read=0x100ae808, .fail_resolve=0x100ae81c, .ok_close_write=0x100ae838, .ok_close_read=0x100ae844, .progress=0x100ae870}},
    Section{.id = 0x6, .table = -0x7be0, .count = -0x7be4, .minus = 0x0, .minus_global = false, .type = 2, .callee = 0x105dac10,
            .format = 0x4b8, .name = 0x28c, .extension = 0x17c, .line = 0x1376, .fail_slot = 0x84, .fail2_slot = 0, .ok_slot = 0x80,
            .sites = {.memcpy=0x100ae8a8, .say=0x100ae8e8, .assertion=0x100ae900, .open_write=0x100ae978, .open_read=0x100ae990, .open_fail_resolve=0x100ae9d4, .callee=0x100aea30, .fail_close_write=0x100aea54, .fail_close_read=0x100aea60, .fail_resolve=0x100aea74, .ok_close_write=0x100aea90, .ok_close_read=0x100aea9c, .progress=0x100aeac8}, .marks_people = true},
    Section{.id = 0x8, .table = -0x74bc, .count = -0x74c0, .minus = 0x0, .minus_global = false, .type = 1, .callee = 0x105ddd20,
            .format = 0x4de, .name = 0x26c, .extension = 0x174, .line = 0x13a8, .fail_slot = 0x78, .fail2_slot = 0, .ok_slot = 0x74,
            .sites = {.memcpy=0x100aeb14, .say=0x100aeb50, .assertion=0x100aeb68, .rename_compare=0x100aebd4, .rename_copy=0x100aebf0, .open_write=0x100aec18, .open_read=0x100aec30, .open_fail_resolve=0x100aec74, .size_write=0x100aec9c, .size_read=0x100aeca8, .callee=0x100aecf8, .fail_close_write=0x100aed1c, .fail_close_read=0x100aed28, .fail_resolve=0x100aed3c, .ok_close_write=0x100aed58, .ok_close_read=0x100aed64, .progress=0x100aed90}, .also_id = 0x16, .writes_id = true, .rename_format = 0x510, .queries_size = true},
    Section{.id = 0x9, .table = -0x79f8, .count = -0x79f4, .minus = 0x0, .minus_global = false, .type = 2, .callee = 0x105dc160,
            .format = 0x520, .name = 0x24c, .extension = 0x16c, .line = 0x13e1, .fail_slot = 0x6c, .fail2_slot = 0, .ok_slot = 0x68,
            .sites = {.memcpy=0x100aedc8, .say=0x100aee04, .assertion=0x100aee1c, .rename_compare=0x100aee88, .rename_copy=0x100aeea4, .open_write=0x100aeecc, .open_read=0x100aeee4, .open_fail_resolve=0x100aef28, .size_write=0x100aef40, .size_read=0x100aef4c, .callee=0x100aef9c, .fail_close_write=0x100aefc0, .fail_close_read=0x100aefcc, .fail_resolve=0x100aefe0, .ok_close_write=0x100aeffc, .ok_close_read=0x100af008, .progress=0x100af034}, .rename_format = 0x54c, .queries_size = true},
    Section{.id = 0xa, .table = -0x7b38, .count = -0x74d4, .minus = 0x0, .minus_global = false, .type = 2, .callee = 0x105dd910,
            .format = 0x55a, .name = 0x22c, .extension = 0x164, .line = 0x1419, .fail_slot = 0x60, .fail2_slot = 0, .ok_slot = 0x5c,
            .sites = {.memcpy=0x100af06c, .say=0x100af0a8, .assertion=0x100af0c0, .rename_compare=0x100af12c, .rename_copy=0x100af148, .open_write=0x100af170, .open_read=0x100af188, .open_fail_resolve=0x100af1cc, .size_write=0x100af1e4, .size_read=0x100af1f0, .callee=0x100af240, .fail_close_write=0x100af264, .fail_close_read=0x100af270, .fail_resolve=0x100af284, .ok_close_write=0x100af2a0, .ok_close_read=0x100af2ac, .progress=0x100af2d8}, .rename_format = 0x582, .queries_size = true},
    Section{.id = 0x11, .table = -0x74c4, .count = -0x74c8, .minus = 0x0, .minus_global = false, .type = 1, .callee = 0x105de0e0,
            .format = 0x58d, .name = 0x20c, .extension = 0x15c, .line = 0x1452, .fail_slot = 0x54, .fail2_slot = 0, .ok_slot = 0x50,
            .sites = {.memcpy=0x100af310, .say=0x100af34c, .assertion=0x100af364, .open_write=0x100af3dc, .open_read=0x100af3f4, .open_fail_resolve=0x100af438, .callee=0x100af494, .fail_close_write=0x100af4b8, .fail_close_read=0x100af4c4, .fail_resolve=0x100af4d8, .ok_close_write=0x100af4f4, .ok_close_read=0x100af500, .progress=0x100af51c}, .last = true},
};
}
