#pragma once
// DATABASE.CPP's loading block: TOC slots and layouts (names inferred from the uses).
#include "guest_abi.hpp"
#include <array>
#include <cstdint>

namespace cm::aot::database::loading {
// Return addresses and callees outside DATABASE.CPP.
namespace callee {
constexpr std::uint32_t release = 0x10775170, allocate = 0x10775130, resize = 0x107751b0,
                        operator_new = 0x10773520, operator_delete = 0x107735a0, data_folder_path = 0x10746280,
                        open_stream = 0x10739de0, close_stream = 0x107766f0, read_block = 0x10775fe0,
                        read_file_marker = 0x107464d0, file_exists = 0x10739d60, delete_temporary_files = 0x10747260,
                        seek_stream = 0x10777f10, show_progress = 0x10743120, copy_path = 0x10727890,
                        copy_text = 0x1077dbf0, compare_text = 0x1077dc90, text_length = 0x1077dbd0,
                        copy_limited = 0x1077dc10, service_release = 0x106a8f60, service_construct = 0x106a8a50,
                        split_name = 0x10739890, version_text = 0x10768810, format_text = 0x1077b720,
                        show_error = 0x10769b60;
}
// TOC words (offsets from r2) the language object uses.
namespace toc {
constexpr std::int32_t language_byte = -0x7ab0;     // the language number's address; 0 and 1 use inline text
constexpr std::int32_t language_object = -0x39e4;   // the shared language object, in the TOC itself
constexpr std::int32_t string_pool = -0x5618;       // base of the module's strings
constexpr std::int32_t success_flag = -0x7bac;      // the shared "success" byte's address
constexpr std::int32_t message_buffer = -0x7ba8;
constexpr std::int32_t release_flag = -0x39d0;      // a byte: which side of the free the original takes
}
// The language object: eight arrays of fixed-size records. The object's first eight words are the arrays' record
// counts (the language file's header), the next eight their addresses. Names are by the table each array is merged
// into (a table of rows of that size, TOC slots in load_language_file); meanings are inferred.
namespace object {
constexpr std::uint32_t size = 0x40;
struct Table {
    std::uint32_t array, count, record_size;  // offsets of the array's address and count words; bytes per record
};
constexpr Table clubs_4f{0x20, 0x00, 0x4f}, rows_ba_to_c8{0x24, 0x04, 0xba}, people_53_appended{0x28, 0x08, 0x53},
    rows_6d_to_128{0x2c, 0x0c, 0x6d}, rows_34_to_38{0x30, 0x10, 0x34}, rows_34_to_50{0x34, 0x18, 0x34},
    people_53{0x38, 0x14, 0x53}, rows_4f_to_68{0x3c, 0x1c, 0x4f};
// By address (the order in which the release frees them) and in the order the language file holds them.
constexpr std::array<Table, 8> by_address{clubs_4f, rows_ba_to_c8, people_53_appended, rows_6d_to_128,
                                          rows_34_to_38, rows_34_to_50, people_53, rows_4f_to_68};
constexpr std::array<Table, 8> in_file_order{clubs_4f, rows_ba_to_c8, people_53_appended, rows_6d_to_128,
                                             rows_34_to_50, rows_34_to_38, rows_4f_to_68, people_53};
}
// A language object in guest memory.
class LanguageObject {
public:
    LanguageObject(guest::Cpu& cpu, std::uint32_t at) : c(cpu), address(at) {}
    std::uint32_t array_word(const object::Table& t) const { return address + t.array; }
    std::uint32_t array(const object::Table& t) const { return c.u32(array_word(t)); }
    std::uint32_t count(const object::Table& t) const { return c.u32(address + t.count); }
    void set_array(const object::Table& t, std::uint32_t value) const { c.put32(array_word(t), value); }
    void clear_arrays() const {
        for (const auto& t : object::by_address) set_array(t, 0);
    }

private:
    guest::Cpu& c;
    std::uint32_t address;
};
// The string pool's offsets.
namespace pool {
constexpr std::uint32_t folder = 0xc7, mode_read = 0x892;
}
}
