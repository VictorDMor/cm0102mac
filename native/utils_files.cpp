#include "utils_detail.hpp"
#include <array>

namespace cm::aot::utils {
using namespace detail;
namespace {
// Pool offsets of the environment variables and their default folders.
struct DirectoryDefault { std::uint16_t variable, folder; };
constexpr std::array<DirectoryDefault, 12> directory_defaults{{
    {0x28, 0x30},  // SI_DATA: data
    {0x35, 0x30},  // CM3_DATA: data
    {0x3e, 0x47},  // CM3_TEMP: temp
    {0x4c, 0x56},  // CM3_DEBUG: debug
    {0x5c, 0x66},  // CM3_PREFS: prefs
    {0x6c, 0x30},  // CM3_TACTICS: data, as the original
    {0x78, 0x85},  // CM3_TRAINING: training
    {0x8e, 0x9a},  // CM3_HISTORY: history
    {0xa2, 0xaf},  // CM3_PICTURES: pictures
    {0xb8, 0xc3},  // CM3_SOUNDS: sounds
    {0xca, 0xd3},  // CM3_HELP: help
    {0xd8, 0xe3},  // CM3_QSTART: quickstarts
}};
constexpr std::uint32_t ignore_environment = 0x1d, temp_variable = 0x3e, read_mode = 0x254, write_mode = 0x257;
// Last word of a data file; see DB_FILES.CPP in docs/cpp-rewrites.md.
constexpr std::uint32_t marker_absent = 0xf3fffff3, marker_alternate = 0xf2fffff2, marker_mac = 0xf1fffff1;
constexpr std::uint32_t seek_set = 0, seek_end = 2;
constexpr std::int32_t temp_listing_size = 100, temp_name_size = 0x1f;
}

std::uint32_t environment_directory(Cpu& c, std::uint32_t variable) {
    Frame frame(c, 0x107460c0, 0x50, {.first_gpr = 29});
    const auto strings = c.toc(strings_slot);
    keep(c, {{31, strings}, {29, variable}});
    Kept<30> directory(c, 0);
    if (!nonzero_logical(c, call(c, string_find, 0x107460f0, {c.toc(-0x7410), strings + ignore_environment})))
        directory = call(c, 0x1077f6d0, 0x10746100, {variable});
    if (nonzero_logical(c, directory)) return result(c, directory);
    auto back = 0x10746118u;
    for (const auto& known : directory_defaults) {
        if (compare(c, std::int32_t(call(c, string_compare_nocase, back, {variable, strings + known.variable}))) == 0) {
            directory = strings + known.folder;
            break;
        }
        back += 0x1c;
    }
    return result(c, directory);
}

std::uint32_t data_path(Cpu& c, std::uint32_t variable, std::uint32_t file) {
    Frame frame(c, 0x10746280, 0x60, {.first_gpr = 30});
    const auto path = c.toc(path_slot), name = frame.at(0x38);
    keep(c, {{31, path}, {30, file}});
    c.put8(path, 0);
    enter(c, 0x107462a8, {variable});
    const auto directory = environment_directory(c, variable);
    keep(c, {{30, directory}});
    enter(c, 0x107462bc, {file, name});
    short_name(c, file, name);
    if (nonzero_logical(c, directory)) call(c, folder_location, 0x107462d8, {path, directory, name, 0});
    call(c, string_append, 0x107462e4, {path, name});
    return result(c, path);
}

std::uint32_t folder_path(Cpu& c, std::uint32_t folder, std::uint32_t path) {
    Frame frame(c, 0x10746300, 0x40, {});
    call(c, folder_location, 0x10746328, {path, folder, c.toc(strings_slot) + no_text, 1});
    return result(c, 1);
}

std::uint32_t usable_path(Cpu& c, std::uint32_t path) {
    Frame frame(c, 0x10746340, 0xa0, {.first_gpr = 28});
    keep(c, {{31, c.toc(strings_slot)}, {30, c.toc(message_slot)}, {29, c.toc(-0x7bac)}, {28, path}});
    if (!nonzero(c, std::int32_t(path))) { report(c, 0x10746384, frame, 0x68, 0x40, 0x299); return result(c, 0); }
    if (!nonzero_logical(c, call(c, string_length, 0x107463cc))) { report(c, 0x107463e8, frame, 0x48, 0x38, 0x2a0); return result(c, 0); }
    // The original then asks for a handle, which is never null here (this
    // routine, a stub, always returns 0x1e), and frees it with another stub.
    compare_logical(c, call(c, 0x10739870, 0x10746434, {path}), 0);
    call(c, 0x10739880, 0x10746440);
    return result(c, 1);
}

void path_with_extension(Cpu& c, std::uint32_t source, std::uint32_t path, std::uint32_t extension) {
    Frame frame(c, 0x10746470, 0x260, {.first_gpr = 30});
    const auto volume = frame.at(0x38), directory = frame.at(0x158), name = frame.at(0x58);
    keep(c, {{31, extension}, {30, path}});
    call(c, split_path, 0x107464a0, {source, volume, directory, name, 0});
    call(c, make_path, 0x107464b8, {path, volume, directory, name, extension});
}

std::uint32_t read_file_marker(Cpu& c, std::uint32_t stream) {
    Frame frame(c, 0x107464d0, 0x50, {.first_gpr = 30});
    const auto marker = frame.at(0x38);
    keep(c, {{30, stream}});
    const auto position = call(c, file_position, 0x107464ec);
    Kept<31> found(c, position);
    call(c, seek_file, 0x10746504, {stream, std::uint32_t(-4), seek_end});
    const auto read = call(c, read_file, 0x10746518, {marker, 4, 1, stream});
    found = read == 1;
    call(c, seek_file, 0x10746538, {stream, position, seek_set});
    if (!nonzero(c, std::int32_t(found))) return result(c, marker_absent);
    const auto value = c.u32(marker);
    c.set(4, value);
    const auto against_alternate = compare(c, std::int32_t(value), std::int32_t(marker_alternate));
    if (against_alternate == 0 || (against_alternate < 0 && compare(c, std::int32_t(value), std::int32_t(marker_mac)) == 0))
        return result(c, value);
    return result(c, marker_absent);
}

std::uint32_t write_file_marker(Cpu& c, std::uint32_t stream, std::uint32_t marker) {
    Frame frame(c, 0x107465a0, 0x40, {.first_gpr = 31});
    keep(c, {{31, stream}});
    // The argument's slot in the caller's frame.
    const auto word = frame.at(0x5c);
    c.put32(word, marker);
    call(c, seek_file, 0x107465c4, {stream, 0, seek_end});
    return call(c, write_file, 0x107465d8, {word, 4, 1, stream});
}

std::uint32_t read_line(Cpu& c, std::uint32_t stream, std::uint32_t line) {
    Frame frame(c, 0x10746f80, 0x50, {.first_gpr = 28});
    keep(c, {{29, line}, {28, stream}});
    Kept<30> count(c, 0);
    Kept<31> cursor(c, line);
    const auto next = [&](std::uint32_t back) {
        const auto character = std::int32_t(std::int8_t(call(c, read_character, back, {stream})));
        c.set(4, std::uint32_t(character));
        return character;
    };
    const auto append = [&](std::int32_t character) {
        c.put8(cursor, std::uint32_t(character));
        count = count + 1;
        cursor = cursor + 1;
    };
    while (true) {
        auto character = next(0x10747030);
        if (compare(c, character, -1) == 0) break;
        // The original's jump table covers 9 to '#'; anything else is text.
        if (compare_logical(c, std::uint32_t(character) - 9, 0x1a) > 0) { append(character); continue; }
        switch (character) {
        case '#':
            // A comment runs to the end of its line, wherever it starts.
            while (compare(c, character, '\n') != 0 && compare(c, character, -1) != 0) character = next(0x10746fe0);
            break;
        case '\n':
            if (nonzero(c, std::int32_t(count))) {
                c.put8(line + count, 0);
                return result(c, 1);
            }
            break;
        case '\t': case '\r': case ' ':
            // Skipped at the start of a line.
            if (nonzero(c, std::int32_t(count))) append(character);
            break;
        default:
            append(character);
        }
    }
    c.put8(line + count, 0);
    return result(c, count != 0);
}

std::uint32_t copy_file(Cpu& c, std::uint32_t source, std::uint32_t destination) {
    Frame frame(c, 0x107470b0, 0x150, {.first_gpr = 26});
    const auto strings = c.toc(strings_slot);
    keep(c, {{31, strings}, {27, destination}, {26, 0}});
    const auto input = call(c, open_file, 0x107470d4, {source, strings + read_mode});
    keep(c, {{30, input}});
    if (!nonzero(c, std::int32_t(input))) return result(c, 0);
    const auto output = call(c, open_file, 0x107470e8, {destination, strings + write_mode});
    keep(c, {{29, output}});
    if (!nonzero(c, std::int32_t(output))) {
        call(c, close_file, 0x107471b4, {input});
        return result(c, 0);
    }
    call(c, seek_file, 0x10747100, {input, 0, seek_end});
    const auto size = call(c, file_position, 0x10747108, {input});
    Kept<26> chunk(c, size);
    Kept<27> remaining(c, size);
    call(c, seek_file, 0x10747124, {input, 0, seek_set});
    Kept<31> copied(c, 2);
    // Halves down to 256 bytes on a failed allocation, and retries that for
    // as long as it fails, as the original does.
    Kept<28> buffer(c, c.r(28));
    while (true) {
        buffer = call(c, allocate, 0x10747144, {chunk});
        if (nonzero(c, std::int32_t(buffer))) break;
        chunk = chunk / 2;
        if (compare_logical(c, chunk, 0x100) < 0) chunk = 0x100;
    }
    // A read that returns nothing before `size` bytes spins here in the original too.
    while (nonzero_logical(c, remaining)) {
        copied = call(c, read_file, 0x10747164, {buffer, 1, chunk, input});
        call(c, write_file, 0x10747180, {buffer, 1, copied, output});
        remaining = remaining - copied;
    }
    // A stack buffer of the source; this build always allocates.
    if (compare_logical(c, buffer, frame.at(0x38)) != 0) call(c, release, 0x107471a0, {buffer});
    call(c, close_file, 0x107471a8, {output});
    keep(c, {{26, 1}});
    call(c, close_file, 0x107471b4, {input});
    return result(c, 1);
}

void delete_temporary_files(Cpu& c, std::uint32_t pattern) {
    Frame frame(c, 0x10747260, 0x11a0, {.first_gpr = 28});
    const auto strings = c.toc(strings_slot), path = c.toc(path_slot);
    const auto name = frame.at(0x38), directory = frame.at(0x58), whole = frame.at(0x158),
               listing = frame.at(0x258), search = frame.at(0xe74);
    keep(c, {{31, strings}, {30, path}, {28, pattern}});
    c.put8(path, 0);
    enter(c, 0x10747298, {strings + temp_variable});
    const auto folder = environment_directory(c, strings + temp_variable);
    keep(c, {{29, folder}});
    enter(c, 0x107472a8, {strings + no_text, name});
    short_name(c, strings + no_text, name);
    if (nonzero_logical(c, folder)) call(c, folder_location, 0x107472c4, {path, folder, name, 0});
    call(c, string_append, 0x107472d0, {path, name});
    call(c, string_copy, 0x107472dc, {directory, path});
    Kept<30> count(c, path);
    Kept<31> at(c, strings);
    Kept<29> offset(c, folder);
    do {
        // A hundred names at a time, until a search finds fewer.
        call(c, make_path, 0x107472f4, {whole, 0, directory, pattern, 0});
        count = 0;
        at = 0;
        c.set({{3, whole}, {4, search}, {5, 0}});
        c.call(0x10739690, 0x1074730c);
        while (compare(c, std::int8_t(c.r(3))) == 0) {
            const auto entry = listing + at;
            count = count + 1;
            at = at + std::uint32_t(temp_name_size);
            call(c, string_copy, 0x10747328, {entry, search});
            if (compare(c, std::int32_t(count), temp_listing_size) == 0) c.set(3, 1);
            else call(c, 0x107397d0, 0x10747340, {search});
        }
        call(c, 0x107156a0, 0x10747350, {search});
        at = 0;
        offset = 0;
        while (compare(c, std::int32_t(at), std::int32_t(count)) < 0) {
            call(c, make_path, 0x10747378, {whole, 0, directory, listing + offset, 0});
            call(c, remove_file, 0x10747380, {whole});
            offset = offset + std::uint32_t(temp_name_size);
            at = at + 1;
        }
    } while (compare(c, std::int32_t(count), temp_listing_size) == 0);
}
}
