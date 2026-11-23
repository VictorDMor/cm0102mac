// FONT.CPP, block `record` (0x1074f390, 0x1074f410, 0x10750200, 0x1074f470): cases. The snapshot holds the game's eight font records (the
// array at TOC -0x3bd8: slots 0-3 loaded with their engine objects, 4-7 constructed and empty), so the release and the destructor run on real
// records (real engine deletes, real allocator), with fields zeroed in memory to take each branch. The loader runs for real with its callees'
// answers scripted: the Toolbox imports by name (Case::imports), the stream I/O and the allocator by entry (Case::answers / nth_answers, the
// owners run as originals so that the answers reach both sides). Labels start with "record:".
#include "font_cpp_checks.hpp"
#include "font_cpp_files_detail.hpp"
#include <cstdlib>
#include <random>

namespace cm::aot::font_cpp_checks {
namespace {
using namespace cm::aot::font_cpp;
constexpr std::uint32_t ctor_entry = 0x1074f390, dtor_entry = 0x1074f410, release_entry = 0x10750200, loader_entry = 0x1074f470;
constexpr std::uint32_t slot_at = scratch_base + 0x10000, record_at = scratch_base + 0x10100, rows_at = scratch_base + 0x11000, names_at = scratch_base + 0x11400,
                        handle_at = scratch_base + 0x14000, data_at = scratch_base + 0x20000, other_slot_at = scratch_base + 0x10040;
constexpr std::uint32_t any_file = 0x61234567, engine_allocate = 0x10775130;

std::vector<Write> fill(std::uint32_t at, std::uint32_t size, std::uint8_t byte) {
    std::vector<Write> memory;
    for (std::uint32_t n = 0; n < size; ++n) memory.push_back(w8(at + n, byte));
    return memory;
}
std::vector<Write> text(std::uint32_t at, const std::string& value) {
    std::vector<Write> memory;
    for (std::size_t n = 0; n < value.size(); ++n) memory.push_back(w8(at + std::uint32_t(n), std::uint8_t(value[n])));
    memory.push_back(w8(at + std::uint32_t(value.size()), 0));
    return memory;
}
}

FONT_CPP_CASES("record") {
    const auto array = scenario.toc(-0x3bd8);
    const auto built = scenario.toc(toc::list_built), count = scenario.toc(toc::list_count), table = scenario.toc(toc::list_table),
               selection = scenario.toc(toc::selection);
    const auto real = [&](unsigned k) { return scenario.r32(array + 4 * k); };
    const auto slot = [&](unsigned k) { return array + 4 * k; };
    if (std::getenv("CM_FONT_CPP_SHOW_STATE"))
        for (unsigned k = 0; k < 8; ++k) {
            std::fprintf(stderr, "record state: slot %u at %x rec %x:", k, slot(k), real(k));
            if (real(k)) for (unsigned n = 0; n < 0x24; n += 4) std::fprintf(stderr, " %08x", scenario.r32(real(k) + n));
            std::fprintf(stderr, "\n");
        }
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> builder_answers{{0x10739690, 1}, {0x10739b40, 0}, {0x107156a0, 0}};

    // ---- 0x1074f390: the constructor (the list is built first: the flag set, or not built with the folder walk ending at once).
    for (const std::uint32_t target : {slot(4), slot(0), slot_at}) {
        Case item{.entry = ctor_entry, .registers = {{3, target}}, .memory = {w8(built, 1), w32(slot_at, 0x5a5a5a5a)}, .label = "record: constructor slot " + hex(target)};
        cases.push_back(item);
    }
    cases.push_back({.entry = ctor_entry, .registers = {{3, slot(5)}}, .memory = {w8(built, 0), w16(count, 0)}, .label = "record: constructor, list not built (walk ends at once)",
                     .answers = builder_answers});
    cases.push_back({.entry = ctor_entry, .registers = {{3, slot_at}}, .memory = {w8(built, 1), w32(slot_at, 0x5a5a5a5a)},
                     .label = "record: constructor, allocation fails", .answers = {{lib::new_object, 0}}});

    // ---- 0x1074f410: the destructor: a record of the array held by a slot (the scratch slot with a flag that does not free it; with a positive flag the
    // slot is a real heap block, made by the constructor, that holds the record's pointer), an empty record, no slot.
    for (const unsigned k : {0u, 1u, 2u, 3u, 4u})
        for (const std::uint32_t flag : {0u, 1u, 0xffffu, 0x8000u, 0x10000u, 0x18001u}) {
            Case item{.entry = dtor_entry, .registers = {{3, slot_at}, {4, flag}}, .memory = {w32(slot_at, real(k)), w8(built, 1)},
                      .label = "record: destructor record " + std::to_string(k) + " flag " + hex(flag)};
            if (std::int16_t(flag) > 0) {
                const auto record = real(k);
                item.prepare = [record, flag](State& s) {
                    guest::Cpu c(s);
                    c.set(3, slot_at);
                    font_cpp_run_original(s, ctor_entry);
                    const auto heap_slot = c.u32(slot_at);
                    c.put32(heap_slot, record);
                    c.set({{3, heap_slot}, {4, flag}});
                };
            }
            cases.push_back(item);
        }
    for (const std::uint32_t flag : {0u, 1u, 0xffffffffu})
        cases.push_back({.entry = dtor_entry, .registers = {{3, 0}, {4, flag}}, .label = "record: destructor no slot flag " + hex(flag)});

    // ---- 0x10750200: the release of a record, field by field.
    const auto glyph = [&](unsigned k, unsigned n) { return scenario.r32(scenario.r32(real(k) + font_record::cache) + 4 * n); };
    for (const unsigned k : {0u, 1u, 3u, 2u, 4u, 7u})
        cases.push_back({.entry = release_entry, .registers = {{3, slot(k)}}, .label = "record: release slot " + std::to_string(k)});
    for (const unsigned k : {0u, 1u}) {
        const auto rec = real(k);
        const std::pair<const char*, std::uint32_t> fields[] = {{"copy", font_data::copy}, {"memory", font_record::sfnt_memory}, {"stream", font_record::stream},
                                                                {"sfnt", font_record::sfnt}, {"scaler", font_record::scaler}};
        for (const auto& [name, offset] : fields)
            cases.push_back({.entry = release_entry, .registers = {{3, slot(k)}}, .memory = {w32(rec + offset, 0)},
                             .label = std::string("record: release slot ") + std::to_string(k) + " without " + name});
        cases.push_back({.entry = release_entry, .registers = {{3, slot(k)}}, .memory = {w32(rec + font_record::cache, 0), w32(rec + font_record::cache_count, 0)},
                         .label = "record: release slot " + std::to_string(k) + " without a cache"});
        cases.push_back({.entry = release_entry, .registers = {{3, slot(k)}}, .memory = {w32(rec + font_record::cache_count, 0)},
                         .label = "record: release slot " + std::to_string(k) + " cache array only"});
        cases.push_back({.entry = release_entry, .registers = {{3, slot(k)}}, .memory = {w32(rec + font_record::cache_count, 1)},
                         .label = "record: release slot " + std::to_string(k) + " one glyph"});
        cases.push_back({.entry = release_entry, .registers = {{3, slot(k)}}, .memory = {w32(glyph(k, 0) + glyph_cache::bitmap, 0)},
                         .label = "record: release slot " + std::to_string(k) + " first glyph without a bitmap"});
        cases.push_back({.entry = release_entry, .registers = {{3, slot(k)}},
                         .memory = {w32(rec + font_record::scaler, 0), w32(rec + font_record::sfnt, 0), w32(rec + font_record::stream, 0), w32(rec + font_record::sfnt_memory, 0)},
                         .label = "record: release slot " + std::to_string(k) + " cache and copy only"});
        cases.push_back({.entry = release_entry, .registers = {{3, slot(k)}},
                         .memory = {w32(rec + font_data::copy, 0), w32(rec + font_record::cache, 0), w32(rec + font_record::cache_count, 0), w32(rec + font_record::scaler, 0),
                                    w32(rec + font_record::sfnt, 0), w32(rec + font_record::stream, 0), w32(rec + font_record::sfnt_memory, 0)},
                         .label = "record: release slot " + std::to_string(k) + " emptied"});
    }
    // A slot whose record pointer is null: the assertion (line 0x3cd), then the read at address 0x20 faults.
    cases.push_back({.entry = release_entry, .registers = {{3, slot_at}}, .memory = {w32(slot_at, 0)}, .label = "record: release a null record"});
    // ---- 0x1074f470: the loader.
    std::vector<Write> rows{w32(table, rows_at), w16(count, 2), w32(rows_at, names_at), w16(rows_at + 8, 0), w32(rows_at + 12, names_at + 0x40), w16(rows_at + 12 + 8, 1)};
    append(rows, text(names_at, "first.ttf"));
    append(rows, text(names_at + 0x40, "second.ttf"));
    std::vector<Write> pattern;
    for (std::uint32_t n = 0; n < 0x400; ++n) pattern.push_back(w8(data_at + n, std::uint8_t(n * 7 + 3)));
    pattern.push_back(w32(handle_at, data_at));
    const auto resource_imports = [&](std::uint32_t handle_size, std::uint32_t second_size, std::uint32_t resources, std::uint32_t refnum) {
        std::vector<Case::Import> imports{{"CarbonLib::FSMakeFSSpec", 0, 0}, {"CarbonLib::ResolveAliasFile", 0, 0}, {"CarbonLib::CurResFile", 0, 0x11112222},
                                          {"CarbonLib::FSpOpenResFile", 0, refnum}, {"CarbonLib::UseResFile", 0, 0}, {"CarbonLib::CloseResFile", 0, 0},
                                          {"CarbonLib::Count1Resources", 0, resources}, {"CarbonLib::Get1IndResource", 0, handle_at}, {"CarbonLib::DetachResource", 0, 0},
                                          {"CarbonLib::GetHandleSize", 1, handle_size}, {"CarbonLib::GetHandleSize", 0, second_size}, {"CarbonLib::DisposeHandle", 0, 0}};
        return imports;
    };
    const auto loader = [&](const std::string& label, std::uint32_t target, std::uint32_t kind, std::int32_t selected, std::vector<Case::Import> imports) {
        std::vector<Write> memory = rows;
        append(memory, pattern);
        memory.push_back(w32(selection, std::uint32_t(selected)));
        memory.push_back(w32(other_slot_at, record_at));
        append(memory, fill(record_at, 0x24, 0));
        Case item{.entry = loader_entry, .registers = {{3, target}, {4, kind}}, .memory = memory,
                  .label = "record: load kind " + hex(kind) + " selection " + std::to_string(selected) + " " + label};
        item.imports = std::move(imports);
        return item;
    };
    // A record in the scratch area is loaded (fields checked by the case itself): the loader reports 1, sets the flag, the copy, the memory
    // object and the stream.
    const auto loaded_ok = [](State& s) {
        guest::Cpu c(s);
        return c.r(3) == 1 && c.u8(record_at) == 1 && c.u32(record_at + font_data::copy) != 0 && c.u32(record_at + font_record::sfnt_memory) != 0 &&
               c.u32(record_at + font_record::stream) != 0;
    };
    // The file route: no usable resource (FSMakeFSSpec fails), the file opened (fopen), measured (fseek/ftell) and read (fseek/fread).
    const auto file_imports = [&] { return std::vector<Case::Import>{{"CarbonLib::FSMakeFSSpec", 0, 0xffffffe4}}; };
    const auto file_answers = [&](std::uint32_t opened, std::uint32_t seek, std::uint32_t size, std::uint32_t read) {
        return std::vector<std::pair<std::uint32_t, std::uint32_t>>{{lib::fopen, opened}, {lib::fseek, seek}, {lib::ftell, size}, {lib::fread, read}, {lib::fclose, 0}};
    };

    // Rare paths first (the call trace pairs the first cases): the first allocation (the copy of the data) fails: the assertion (line 0x25a), after
    // which the copy goes to a null pointer. (Failures of the engine's own allocations cannot be scripted: an nth answer reaches the loader's and the
    // engine's calls unevenly between the two sides.)
    for (const char* source : {"resource", "file"}) {
        auto item = loader(std::string(source) + ", the copy's allocation fails", other_slot_at, 0, -2, std::string(source) == "file" ? file_imports() : resource_imports(0x400, 0x400, 1, 7));
        item.answers = {{lib::allocate, 0}, {lib::fclose, 0}};
        if (std::string(source) == "file") {
            item.answers = file_answers(any_file, 0, 0x100, 1);
            item.answers.push_back({lib::allocate, 0});
            item.original_owners = {lib::fread};
        }
        cases.push_back(item);
    }
    // The engine's own allocations fail (the memory object, its table, the stream's block: the n-th call of the allocator's new[] 0x10775130, which the
    // engine's malloc is, counted with the owner's original body dispatched on both sides): the error words set, the assertions at lines 0x268 / 0x26e.
    for (const unsigned nth : {1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u}) {
        auto item = loader("resource, engine allocation " + std::to_string(nth) + " fails", other_slot_at, 0, -2, resource_imports(0x400, 0x400, 1, 7));
        item.nth_answers = {{engine_allocate, nth, 0}};
        item.original_owners = {engine_allocate};
        cases.push_back(item);
    }
    {
        auto item = loader("resource", other_slot_at, 0, -2, resource_imports(0x400, 0x400, 1, 7));
        item.verify = loaded_ok;
        cases.push_back(item);
        item = loader("resource", other_slot_at, 3, 0, resource_imports(0x400, 0x400, 1, 7));
        item.verify = loaded_ok;
        cases.push_back(item);
    }
    // The file route: no usable resource (FSMakeFSSpec fails), the file opened (fopen), measured (fseek/ftell) and read (fseek/fread), in each state.
    for (const std::uint32_t kind : {0u, 3u, 2u})
        for (const std::int32_t selected : {-2, 0, 1})
            for (const auto& answers : {file_answers(any_file, 0, 0x100, 1), file_answers(any_file, 1, 0x100, 1), file_answers(any_file, 0, 0, 1), file_answers(any_file, 0, 0x100, 0),
                                        file_answers(any_file, 0, 0x100, 2), file_answers(0, 0, 0x100, 1)}) {
                auto item = loader("file " + hex(answers[0].second) + " seek " + hex(answers[1].second) + " size " + hex(answers[2].second) + " read " + hex(answers[3].second),
                                   other_slot_at, kind, selected, file_imports());
                item.answers = answers;
                item.original_owners = {lib::fread};
                cases.push_back(item);
            }
    // Every kind and selection through a resource.
    for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 4u, 0x100u, 0x102u, 0x203u, 0xffu, 0x1ffu})
        for (const std::int32_t selected : {-2, -1, 0, 1})
            cases.push_back(loader("resource", slot(4), kind, selected, resource_imports(0x400, 0x400, 1, 7)));
    cases.push_back(loader("resource, selection 5 past the rows", slot(5), 2, 5, resource_imports(0x400, 0x400, 1, 7)));
    // A slot whose record pointer is null: the first assertion (line 0x20b) ends the run (the reporter exits).
    {
        auto item = loader("null record", other_slot_at, 0, -2, resource_imports(0x400, 0x400, 1, 7));
        item.memory.push_back(w32(other_slot_at, 0));
        cases.push_back(item);
    }
    // The target: a real loaded record (released first, real engine deletes), an empty one, the scratch record.
    for (const auto target : {slot(0), slot(1), slot(3), slot(2), slot(6), other_slot_at}) {
        auto item = loader("resource, slot " + hex(target), target, 0, -2, resource_imports(0x400, 0x400, 1, 7));
        cases.push_back(item);
    }
    // The size of the data: the handle's first answer (the size) and its second (the copy's check).
    for (const std::uint32_t first : {0u, 1u, 0x20u, 0x400u, 0x1000u, 0xffffffffu})
        for (const std::uint32_t second : {0u, 1u, 0x20u, 0x3ffu, 0x400u, 0xfffu, 0x1000u}) {
            if (first == 0 && second != 0) continue;
            cases.push_back(loader("resource, size " + hex(first) + "/" + hex(second), other_slot_at, 1, -2, resource_imports(first, second, 1, 7)));
        }
    // No resource (count zero or too small for the font number), no usable resource file: the file, in each state.
    {
        auto item = loader("resource, refnum -1 (the file)", other_slot_at, 2, 0, resource_imports(0x400, 0x400, 1, 0xffffffff));
        item.answers = file_answers(any_file, 0, 0x100, 1);
        item.original_owners = {lib::fread};
        cases.push_back(item);
        item = loader("resource, count zero (the file)", other_slot_at, 2, 0, resource_imports(0x400, 0x400, 0, 7));
        item.answers = file_answers(any_file, 0, 0x100, 1);
        item.original_owners = {lib::fread};
        cases.push_back(item);
    }
    {
        auto item = loader("file", other_slot_at, 1, -2, file_imports());
        item.answers = file_answers(any_file, 0, 0x100, 1);
        item.original_owners = {lib::fread};
        item.verify = loaded_ok;
        cases.push_back(item);
    }
}
}
