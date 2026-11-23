// The assertion module, block `split` (0x10739890): cases for the file-name splitter. A case writes the path (and the four output buffers'
// guards) into the scratch area, sets r3-r10 (distinct wherever the routine does not write them, so the full comparison of r3-r10 sees a
// register it should have kept) and runs the routine as the original and as the C++ from the snapshot's real stack. The checker compares
// the registers, the heap and every byte the call wrote, on the dead stack too (its 0x460-byte frame, the four 256-byte buffers and the string
// routines' frames below it), the call trace (strlen and strcpy: the order, the return addresses, the held registers and the arguments they
// read), and mutations of the bytes the original read.
//
// The cases cover: every call site of the routine (the first cases hit all ten return addresses, then each one absent), a null path (any
// outputs), each output pointer null or not, paths with no, one and many ':', a leading, a trailing and doubled ':', '.' in the directory part,
// the name and the extension, empty parts, high-bit bytes, the game's real file names, parts at and over the 256-byte buffers (the strings
// are copied with strcpy, so a long name or directory runs into the next buffer: the same on both sides), the four static arrays pre-filled
// with patterns (the directory buffer's terminator comes only from its static copy), and paths whose length wraps the halfword indices.
// Labels start with "split:".
#include "assertion_checks.hpp"
#include <string>

namespace cm::aot::assertion_checks {
namespace {
constexpr std::uint32_t split_entry = 0x10739890;
constexpr std::uint32_t path_at = scratch_base + 0x40000, volume_out = scratch_base + 0x1000, directory_out = scratch_base + 0x2000,
                        name_out = scratch_base + 0x3000, extension_out = scratch_base + 0x4000;
constexpr unsigned all_outputs = 0xf, volume_bit = 1, directory_bit = 2, name_bit = 4, extension_bit = 8;
constexpr std::uint32_t slot_arrays[4] = {-0x3e78 & 0xffffffff, -0x3e7c & 0xffffffff, -0x3e80 & 0xffffffff, -0x3e84 & 0xffffffff};

std::vector<std::pair<unsigned, std::uint32_t>> registers(std::uint32_t path, unsigned outputs) {
    std::vector<std::pair<unsigned, std::uint32_t>> all;
    for (unsigned n = 3; n <= 10; ++n) all.emplace_back(n, 0x7a000000u + n * 0x01010101u);
    all[0].second = path;
    all[1].second = outputs & volume_bit ? volume_out : 0;
    all[2].second = outputs & directory_bit ? directory_out : 0;
    all[3].second = outputs & name_bit ? name_out : 0;
    all[4].second = outputs & extension_bit ? extension_out : 0;
    return all;
}
std::vector<Write> text(std::uint32_t at, const std::string& value) {
    std::vector<Write> memory;
    for (std::size_t n = 0; n < value.size(); ++n) memory.push_back(w8(at + std::uint32_t(n), std::uint8_t(value[n])));
    memory.push_back(w8(at + std::uint32_t(value.size()), 0));
    return memory;
}
// The output buffers hold a guard pattern first, so a copy that does not happen (or goes too far) shows.
std::vector<Write> guards() {
    std::vector<Write> memory;
    for (const auto out : {volume_out, directory_out, name_out, extension_out})
        for (std::uint32_t n = 0; n < 0x400; n += 4) memory.push_back(w32(out + n, 0xe1e2e3e4u + out));
    return memory;
}
std::string repeated(const std::string& unit, std::size_t times) {
    std::string out;
    for (std::size_t n = 0; n < times; ++n) out += unit;
    return out;
}
std::string read_string(State& s, std::uint32_t at) {
    std::string out;
    for (std::uint32_t n = 0; n < 0x1000; ++n) {
        const auto byte = std::uint8_t(s.memory.read<1>(at + n));
        if (!byte) break;
        out.push_back(char(byte));
    }
    return out;
}
// The independent statement of what the splitter does for a well-formed path (no part reaches the buffers' size): the last ':' ends the
// directory part, the name is what follows it, cut at its last '.' (which starts the extension); of the directory part the text up to the
// first ':' is the volume (nothing when the path begins with ':' or the last ':' is its first character), the rest after that ':' the
// directory, with its last ':' cut. Written from the listing, not from the C++.
struct Parts { std::string volume, directory, name, extension; };
Parts expected_parts(const std::string& path) {
    Parts parts;
    const auto last = path.rfind(':');
    const std::string file = last == std::string::npos ? path : path.substr(last + 1);
    const auto dot = file.rfind('.');
    parts.name = dot == std::string::npos ? file : file.substr(0, dot);
    if (dot != std::string::npos) parts.extension = file.substr(dot);
    if (last != std::string::npos && last != 0 && path[0] != ':') {
        const auto first = path.find(':');
        parts.volume = path.substr(0, first);
        const auto from = first < last ? first + 1 : last;  // the first ':' is skipped when it comes before the last one
        parts.directory = path.substr(from, last - from);
        if (!parts.directory.empty() && parts.directory.back() == ':') parts.directory.pop_back();
    } else if (last != std::string::npos && last != 0) {
        // The path begins with ':': nothing is taken as the volume (its buffer keeps the static copy: empty) and the skipped ':' is the first.
        parts.directory = path.substr(1, last - 1);
        if (!parts.directory.empty() && parts.directory.back() == ':') parts.directory.pop_back();
    }
    return parts;
}
}

ASSERTION_CASES("split") {
    const auto arrays = [&] { std::array<std::uint32_t, 4> at{}; for (unsigned n = 0; n < 4; ++n) at[n] = scenario.toc(int(std::int32_t(slot_arrays[n]))); return at; }();
    const auto make = [&](std::string path, unsigned outputs, std::string label, std::vector<Write> extra = {}) {
        Case item{.entry = split_entry, .registers = registers(path_at, outputs), .label = "split: " + std::move(label)};
        append(item.memory, guards());
        append(item.memory, text(path_at, path));
        append(item.memory, extra);
        return item;
    };
    // A case that also states the result: the four outputs are what an independent reading of the listing says (typical paths only).
    const auto checked = [&](const std::string& path, unsigned outputs, std::string label) {
        auto item = make(path, outputs, std::move(label));
        const auto parts = expected_parts(path);
        item.verify = [parts, outputs](State& s) {
            return (!(outputs & volume_bit) || read_string(s, volume_out) == parts.volume) &&
                   (!(outputs & directory_bit) || read_string(s, directory_out) == parts.directory) &&
                   (!(outputs & name_bit) || read_string(s, name_out) == parts.name) &&
                   (!(outputs & extension_bit) || read_string(s, extension_out) == parts.extension);
        };
        return item;
    };

    // ---- Every call site first (the call trace pairs the first cases of an entry): the doubled ':' makes the directory end with one (the second
    // length call), the '.' the extension copy, and the outputs the four copies.
    cases.push_back(checked("Disk:Folder::FILE.CPP", all_outputs, "all ten call sites"));
    cases.push_back(checked("Disk:Folder:FILE.CPP", all_outputs, "no trailing colon in the directory"));
    cases.push_back(checked("Disk:Folder:FILE", all_outputs, "no extension"));
    cases.push_back(checked("Disk:Folder:FILE.CPP", 0, "no outputs"));
    cases.push_back(checked("Disk:Folder:FILE.CPP", volume_bit, "volume only"));
    cases.push_back(checked("Disk:Folder:FILE.CPP", directory_bit, "directory only"));
    cases.push_back(checked("Disk:Folder:FILE.CPP", name_bit, "name only"));
    cases.push_back(checked("Disk:Folder:FILE.CPP", extension_bit, "extension only"));
    cases.push_back(checked("FILE.CPP", all_outputs, "no colon"));
    cases.push_back(checked("Disk:FILE.CPP", all_outputs, "one colon"));
    for (unsigned outputs = 0; outputs < 16; ++outputs)
        cases.push_back(checked("Disk:Folder::FILE.CPP", outputs, "outputs " + hex(outputs)));

    // ---- A null path: the static copies are made and nothing else (the outputs are not touched, whatever they are).
    for (const unsigned outputs : {all_outputs, 0u, volume_bit | name_bit}) {
        auto item = make("", outputs, "null path, outputs " + hex(outputs));
        item.registers[0].second = 0;
        cases.push_back(item);
    }

    // ---- Typical and edge paths, all outputs and none.
    const std::vector<std::string> typical = {
        "DATE.CPP", "graph_carbon.cpp", "qsort8x.c", "MAIN.CPP", "UTILS.CPP", "CASH.CPP", "SIPRINT.CPP", "CUP_STAGE.CPP", "T2K.C",
        "Disk:Folder:Sub:Deep:FILE.CPP", "Macintosh HD:Projects:CM:SRC:FILE.CPP", "Disk:Folder:a.b.c", "FILE", "FILE.", ".CPP", "a.b.c",
        "Folder:a.b.c", "Disk:Folder.x:FILE.CPP", "Disk:Folder:FILE.A.B", "a", "ab:cd", "a:b:c", "A:B:C:D:E:F", "Disk:a.b:c.d", "x:.hidden",
        "a.b:c", "a.:b", ":Folder:FILE.CPP", ":Disk:Folder:FILE.CPP", "::FILE", ":FILE", "Disk::FILE", "Disk:Folder::", "Disk:Folder:", "Disk:",
        "Disk::", "a:", "x:y:z:", ":a.:", "Disk:Folder:::FILE.X", "Disk:::FILE", "Disk: :FILE", " : :a. ", "A.B:C.D:E.F"};
    for (const auto& path : typical) {
        cases.push_back(checked(path, all_outputs, "path " + path));
        cases.push_back(make(path, 0, "path " + path + ", no outputs"));
    }
    // The shortest and strangest paths (no verify: their parts are what the listing makes of them, which the original decides).
    for (const std::string path : {"", ":", "::", ":::", ".", "..", ":.", ".:", "...", "a..b", ":a", "a:."})
        for (const unsigned outputs : {all_outputs, 0u})
            cases.push_back(make(path, outputs, "short path '" + path + "', outputs " + hex(outputs)));
    // High-bit bytes (extsb: the sign extension must not make one of them a ':' or a '.').
    for (const std::string& path : {std::string("caf\xe9:x\xff.\x80"), std::string("\xba:\xba\xba.\xae"), std::string("\x80\xff:\xfe.\xff"), std::string("\xb3:\xae")})
        cases.push_back(make(path, all_outputs, "high-bit bytes"));

    // ---- Parts at and over the buffers' size (256): the strings run into the next buffer, up to the saved registers of the frame.
    for (const std::size_t size : {250u, 253u, 254u, 255u, 256u, 257u, 258u, 300u, 400u}) {
        const std::string longer(size, 'n');
        cases.push_back(make(longer, all_outputs, "name of " + std::to_string(size) + ", no extension"));
        cases.push_back(make(longer + ".EXT", all_outputs, "name of " + std::to_string(size) + " and an extension"));
        cases.push_back(make("V:D:" + std::string(size, 'e') , all_outputs, "name as a long run, volume and directory"));
        cases.push_back(make("Disk:" + std::string(size, 'd') + ":FILE.CPP", all_outputs, "directory of " + std::to_string(size)));
        // (A volume of 304 or more would smash the saved link register: the original then returns to the text's bytes, which no run can compare.)
        if (size < 304) cases.push_back(make(std::string(size, 'v') + ":D:FILE.CPP", all_outputs, "volume of " + std::to_string(size)));
        cases.push_back(make("N." + std::string(size, 'x'), all_outputs, "extension of " + std::to_string(size)));
    }
    for (const std::size_t times : {100u, 126u, 127u, 128u, 129u, 130u, 200u})
        cases.push_back(make(repeated("d:", times) + "FILE.CPP", all_outputs, std::to_string(times) + " one-letter directories"));
    // A volume long enough to run over the saved registers (past 0x100 + 0xc) and the directory with it, never as far as the link register.
    for (const std::size_t size : {266u, 270u, 285u, 296u})
        cases.push_back(make(std::string(size, 'v') + ":FILE.CPP", all_outputs, "volume of " + std::to_string(size) + " over the saved registers"));

    // ---- The static arrays pre-filled: patterns across the whole copy (a word copied to the wrong place shows), and a fill that is never
    // zero (the directory buffer's terminator then comes from nowhere: the length scan runs on into the next buffer).
    for (unsigned fill = 0; fill < 3; ++fill) {
        std::vector<Write> memory;
        for (unsigned n = 0; n < 4; ++n)
            for (std::uint32_t at = 0; at < 0x100; at += 4)
                memory.push_back(w32(arrays[n] + at, fill == 0 ? 0xaaaaaaaau : fill == 1 ? 0x41424344u + n * 0x01010101u + at * 0x10001u : 0x5b5b5b00u + (at >> 2)));
        for (const std::string path : {"Disk:Folder::FILE.CPP", "Disk:Folder:FILE", "FILE.CPP", "Disk:FILE.CPP", ":Folder:FILE.X", "Disk::FILE", ":", "", "a:b:"})
            cases.push_back(make(path, all_outputs, "static arrays fill " + std::to_string(fill) + ", path '" + path + "'", memory));
        auto item = make("", all_outputs, "static arrays fill " + std::to_string(fill) + ", null path", memory);
        item.registers[0].second = 0;
        cases.push_back(item);
    }

    // ---- Lengths that wrap the halfword indices: 0x8000 bytes is -0x8000 as a halfword, 0x8001 is -0x7fff (the colon scan ends at once and the name is
    // copied from before the path, which is zeros here).
    for (const std::size_t size : {0x8000u, 0x8001u}) {
        auto item = make("", all_outputs, "path of " + hex(std::uint32_t(size)) + " bytes");
        const auto at = path_at + 0x1000;
        item.registers[0].second = at;
        for (std::size_t n = 0; n < size; n += 4) append(item.memory, {w32(at + std::uint32_t(n), 0x61616161u)});
        append(item.memory, {w8(at + std::uint32_t(size), 0)});
        cases.push_back(item);
    }
}
}
