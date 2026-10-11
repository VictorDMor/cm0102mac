#include "file_path_internal.hpp"
#include "file_controller_adapter.hpp"
#include "file_controller_fixture.hpp"
#include "semantic_io_checks.hpp"
#include "platform.hpp"
#include <fstream>
#include <unistd.h>

using namespace cm::semantic_io_checks;
namespace {
constexpr std::uint32_t text = fixture + 0x1000, output = fixture + 0x2000, leaf_output = fixture + 0x3000;
unsigned cases = 0, faults = 0, negatives = 0, native_cases = 0;
std::array<std::uint64_t, 7> witnesses{};
struct Temporary {
    std::filesystem::path root, game, user;
    Temporary() {
        auto pattern = (std::filesystem::temp_directory_path() / "cm-file-path-XXXXXX").string();
        require(mkdtemp(pattern.data()) != nullptr, "Private path fixture"); root = pattern; game = root / "Game"; user = root / "User";
        for (const auto& folder : {game, user}) {
            std::filesystem::create_directories(folder / "Folder");
            std::ofstream(folder / "live.dat") << "data"; std::ofstream(folder / "Folder/live.dat") << "nested";
            std::ofstream(folder / "Caf\xc3\xa9.dat") << "unicode";
        }
    }
    ~Temporary() { std::error_code error; std::filesystem::remove_all(root, error); }
};
void cstring(State& s, const std::string& value) { std::vector<std::uint8_t> bytes(value.begin(), value.end()); bytes.push_back(0); s.memory.copy_in(text, bytes); }
void args(State& s, unsigned i, std::initializer_list<std::uint32_t> values) {
    unsigned n = 3; for (auto value : values) s.wr<4>(n++ * 4, value);
    s.pc = file_path::entries[i]; s.wr<4>(4128, return_sentinel); s.budget = s.executed + 100000;
}
std::unique_ptr<State> setup(const char* program, unsigned i, bool modern = true) {
    auto s = std::make_unique<State>(); load_program(*s, program); s->memory.map(fixture, 0x10000); s->memory.copy_in(fixture, std::vector<std::uint8_t>(0x10000, 0xa5));
    s->memory.copy_in(stack - 8192, std::vector<std::uint8_t>(8256, 0xa7)); s->registers.fill(0x5b); s->unique.fill(0x6d);
    for (unsigned n = 0; n < 32; ++n) s->wr<4>(n * 4, 0xcafe0000 + n);
    s->wr<4>(4, stack); s->wr<4>(8, toc); s->wr<4>(4132, 0x31415926); s->wr<4>(2440, 0xbadc0ffe);
    s->wr<1>(2304, i % 2 ? 0xf9 : 0xa6); s->wr<1>(1024, i % 4); s->wr<1>(1027, (i / 2) % 2);
    s->memory.write<1>(toc - 0x3504, 1); s->memory.write<1>(toc - 0x3503, modern); s->memory.write<1>(toc - 0x34f4, 0);
    cstring(*s, "CM User Data:live.dat");
    if (i == 0 || i == 1 || i == 2 || i == 4) args(*s, i, i == 0 ? std::initializer_list<std::uint32_t>{output, text} : std::initializer_list<std::uint32_t>{text, output});
    else if (i == 3) args(*s, i, {});
    else if (i == 5) args(*s, i, {20, text, output});
    else args(*s, i, {text, output, leaf_output});
    return s;
}
struct PathModel : cm::file_controller_checks::FileModel {
    bool partial_conversion = false;
    std::optional<std::uint32_t> script_status;
    bool operator==(const PathModel&) const = default;
    bool dispatch(State& s, const char* name) {
        if (script_status && std::string_view(name) == "CarbonLib::UpgradeScriptInfoToTextEncoding") {
            if (!*script_status) s.memory.write<4>(arg(s, 4), 0x12345678);
            s.wr<4>(12, *script_status); s.pc = std::uint32_t(s.r<4>(4128)); return true;
        }
        if (partial_conversion && std::string_view(name) == "CarbonLib::ConvertFromTextToUnicode") {
            const auto stack_pointer = std::uint32_t(s.r<4>(4));
            const auto dest = std::uint32_t(s.memory.read<4>(stack_pointer + 68));
            s.memory.write<2>(dest, 0x1234);
            s.memory.write<4>(std::uint32_t(s.memory.read<4>(stack_pointer + 60)), 1);
            s.memory.write<4>(std::uint32_t(s.memory.read<4>(stack_pointer + 64)), 2);
            s.wr<4>(12, std::uint32_t(-54)); s.pc = std::uint32_t(s.r<4>(4128)); return true;
        }
        if (std::string_view(name) == "CarbonLib::ConvertFromTextToUnicode" && std::uint32_t(arg(s, 1)) > 255 && !errors.contains("ConvertFromTextToUnicode")) {
            s.wr<4>(12, std::uint32_t(-50)); s.pc = std::uint32_t(s.r<4>(4128)); return true;
        }
        return cm::file_controller_checks::FileModel::dispatch(s, name);
    }
};
struct Imports : FilePathBoundaryObserver {
    struct Record { std::string name; decltype(State::registers) registers; std::uint32_t pc; std::uint64_t calls; bool operator==(const Record&) const = default; };
    PathModel model; bool modeled = true; std::vector<Record> records;
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    void import_enter(const State& s, const char* name) override { records.push_back({name, s.registers, s.pc, s.native_calls}); }
    void import_leave(const State& s, const char* name) override { import_enter(s, name); }
    bool import_fixture(State& s, const char* name) override { return modeled && model.dispatch(s, name); }
};
using Expect = std::function<void(const State&)>;
void check(std::unique_ptr<State> original, unsigned i, const std::string& label, const PathModel& model = {},
           const Temporary* native = nullptr, const Expect& expected = {}, bool fault = false,
           const std::array<bool, 7>& selected_original = {}) {
    auto modern = clone(*original); Imports ia, ib; ia.model = ib.model = model;
    original->file_path_original = true; modern->file_path_entry_original = selected_original;
    if (native) {
        ia.modeled = ib.modeled = false;
        original->platform = std::make_shared<NativePlatform>(native->game, native->user, AudioMode::offline);
        modern->platform = std::make_shared<NativePlatform>(native->game, native->user, AudioMode::offline);
    }
    Effects ea, eb; ea.state = original.get(); eb.state = modern.get();
    original->file_path_observer = &ia; modern->file_path_observer = &ib;
    original->memory.date_observer = &ea; modern->memory.date_observer = &eb;
    std::string ax, bx;
    try { run(*original); } catch (const std::exception& error) { ax = error.what(); }
    try { run(*modern); } catch (const std::exception& error) { bx = error.what(); }
    original->file_path_observer = modern->file_path_observer = nullptr;
    original->memory.date_observer = modern->memory.date_observer = nullptr;
    require(ax == bx, label + " error original=" + ax + " semantic=" + bx);
    require(fault == !ax.empty(), label + " fault expectation " + ax);
    if (fault) require(ax.starts_with("Unmapped data access at "), label + " expected mapping fault");
    Image(*original).compare(*modern, label); require(ea.bytes == eb.bytes, label + " ordered effects");
    require(ia.records == ib.records && ia.model == ib.model, label + " imports/dependency history");
    if (!fault) for (const auto& [service, error] : model.errors) {
        (void)error;
        require(std::any_of(ib.records.begin(), ib.records.end(), [&](const auto& record) {
            return record.name == "CarbonLib::" + service;
        }), label + " unreached injected service " + service);
    }
    require(!modern->file_path_dispatch_escapes, label + " dispatch escape");
    for (unsigned n = 0; n < 7; ++n) {
        if (selected_original[n]) require(modern->file_path_original_calls[n] && !modern->file_path_calls[n], label + " selected original witness");
        else require(!modern->file_path_original_calls[n], label + " unexpected original call");
        if (!fault) witnesses[n] += modern->file_path_calls[n];
    }
    if (!fault && !selected_original[i]) require(modern->file_path_calls[i], label + " root production witness");
    if (expected) expected(*modern);
    ++cases; faults += fault; native_cases += native != nullptr;
}
struct Negative : Imports {
    struct Boundary { Image image; std::vector<std::uint8_t> effects; std::vector<Record> imports; PathModel model; };
    std::uint32_t target;
    std::vector<Boundary> reference;
    Effects* effects = nullptr;
    bool mutated = false;
    unsigned parent_returns = 0;
    explicit Negative(std::uint32_t entry) : target(entry) {}
    void leave(State& s, std::uint32_t entry, bool original) override {
        if (entry != target) { if (mutated) ++parent_returns; return; }
        if (original) { reference.push_back({Image(s), effects->bytes, records, model}); return; }
        require(!reference.empty(), "Missing path negative reference boundary");
        const auto& expected = reference.front();
        expected.image.compare(s, "Unmutated path boundary " + hex_address(entry));
        require(expected.effects == effects->bytes && expected.imports == records && expected.model == model,
                "Path boundary ordered effects/import/dependency prefix");
        s.wr<4>(12, s.r<4>(12) ^ 1); mutated = true;
        expected.image.compare(s, "Own path boundary " + hex_address(entry));
    }
};
void negative(std::unique_ptr<State> original, std::uint32_t target) {
    auto modern = clone(*original); original->file_path_original = true;
    Effects ea, eb; ea.state = original.get(); eb.state = modern.get();
    Negative observer(target); observer.effects = &ea;
    original->memory.date_observer = &ea; original->file_path_observer = &observer; run(*original);
    require(!observer.reference.empty(), "Unreached own/child path boundary");
    observer.records.clear(); observer.model = {}; observer.effects = &eb;
    modern->memory.date_observer = &eb; modern->file_path_observer = &observer;
    bool rejected = false; std::string error;
    try { run(*modern); } catch (const std::runtime_error& e) {
        error = e.what(); rejected = error.starts_with("Own path boundary " + hex_address(target));
    }
    original->file_path_observer = modern->file_path_observer = nullptr;
    original->memory.date_observer = modern->memory.date_observer = nullptr;
    require(rejected && observer.mutated && !observer.parent_returns,
            "Own/child path mutation escaped its boundary: " + error);
    ++negatives;
}
std::unique_ptr<State> sparse(const State& initial, std::uint64_t omitted = UINT64_MAX, const PathModel& model = {}) {
    auto probe = clone(initial); probe->file_path_original = true; Imports observer; observer.model = model;
    Effects effects; effects.state = probe.get(); probe->memory.date_observer = &effects; probe->file_path_observer = &observer;
    run(*probe); probe->memory.date_observer = nullptr; probe->file_path_observer = nullptr;
    auto result = std::make_unique<State>(); result->registers = initial.registers; result->unique = initial.unique;
    result->pc = initial.pc; result->budget = initial.budget; result->heap_end = initial.heap_end;
    result->allocated = initial.allocated; result->available = initial.available; result->memory_error = initial.memory_error;
    for (const auto& [begin, length] : effects.accesses) for (const auto& [address, size] : initial.memory.mapped_ranges()) {
        const auto low = std::max(std::uint64_t(begin), std::uint64_t(address));
        const auto high = std::min(std::uint64_t(begin) + length, std::uint64_t(address) + size);
        if (low >= high) continue;
        if (low <= omitted && omitted < high) {
            if (low < omitted) result->memory.map(std::uint32_t(low), omitted - low);
            if (omitted + 1 < high) result->memory.map(std::uint32_t(omitted + 1), high - omitted - 1);
        } else result->memory.map(std::uint32_t(low), high - low);
    }
    for (const auto& [address, size] : result->memory.mapped_ranges()) result->memory.copy_in(address, initial.memory.copy_out(address, size));
    return result;
}
}
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    try {
        for (bool modern : {false, true}) for (unsigned i = 0; i < 7; ++i) {
            check(setup(argv[1], i, modern), i, "Direct path " + std::to_string(modern) + "/" + std::to_string(i));
            auto s = setup(argv[1], i, modern); s->memory.write<4>(fixture, file_path_entries[i]); s->memory.write<4>(fixture + 4, toc);
            s->wr<4>(48, fixture); s->pc = 0x10781320;
            check(std::move(s), i, "Generated indirect path " + std::to_string(modern) + "/" + std::to_string(i));
        }
        for (unsigned i = 0; i < 7; ++i) negative(setup(argv[1], i), file_path_entries[i]);
        for (const auto [parent, child] : {std::pair{1u, 0u}, {2u, 4u}, {4u, 6u}, {6u, 5u}, {5u, 3u}}) {
            negative(setup(argv[1], parent), file_path_entries[child]);
            std::array<bool, 7> selected{}; selected[child] = true;
            check(setup(argv[1], parent), parent, "Mixed original child " + std::to_string(parent) + "/" + std::to_string(child), {}, nullptr, {}, false, selected);
            selected.fill(false); selected[parent] = true;
            check(setup(argv[1], parent), parent, "Mixed original parent " + std::to_string(parent) + "/" + std::to_string(child), {}, nullptr, {}, false, selected);
        }
        for (const std::string path : {"", "live.dat", ":live.dat", "CM User Data:live.dat", "CM User Data:missing.dat", "CM User Data:Folder:live.dat", ":Folder:live.dat", "CM User Data:", "CM User Data:Folder:", "A::B"})
            for (unsigned i : {0u, 1u, 2u, 4u, 6u}) { auto s = setup(argv[1], i); cstring(*s, path); check(std::move(s), i, "Path syntax " + std::to_string(i) + "/" + path); }
        for (const auto& [path, leaf] : std::array<std::pair<std::string, std::string>, 4>{{
                {":live.dat:", "live.dat"}, {"CM User Data:Folder:", "Folder"}, {"CM User Data:", ""}, {"live.dat", "live.dat"}}}) {
            auto s = setup(argv[1], 6); cstring(*s, path);
            check(std::move(s), 6, "Trailing-colon leaf contract " + path, {}, nullptr, [=](const State& result) {
                require(!result.r<4>(12) && result.memory.read<2>(leaf_output) == leaf.size(), "Trailing-colon retains the last converted leaf");
                for (unsigned n = 0; n < leaf.size(); ++n) require(result.memory.read<2>(leaf_output + 2 + n * 2) == std::uint8_t(leaf[n]), "Trailing-colon leaf bytes");
            });
        }
        for (unsigned i : {1u, 2u}) {
            auto s = setup(argv[1], i); s->wr<4>(12, 0); check(std::move(s), i, "Null guarded path " + std::to_string(i));
            for (unsigned n : {63u, 64u, 255u, 256u}) { auto t = setup(argv[1], i); cstring(*t, std::string(n, 'x')); check(std::move(t), i, "Guarded name length " + std::to_string(i) + "/" + std::to_string(n)); }
        }
        for (unsigned amount : {0u, 1u, 7u, 8u, 9u, 15u, 16u, 17u, 63u, 255u, 256u, 0xffffu, 0x10000u, 0xffff0001u})
            for (const std::string failed : {"", "CreateTextToUnicodeInfoByEncoding", "ConvertFromTextToUnicode", "DisposeTextToUnicodeInfo"}) {
                auto s = setup(argv[1], 5); s->wr<4>(12, amount); s->memory.copy_in(text, std::vector<std::uint8_t>(512, 0x8e));
                PathModel model; if (!failed.empty()) model.errors[failed] = -54;
                check(std::move(s), 5, "Unicode length/error " + std::to_string(amount) + "/" + failed, model, nullptr, [=](const State& result) {
                    require(result.memory.read<2>(output) == std::uint16_t(amount), "Unicode published low-word length");
                    if (std::int16_t(amount) > 0) for (unsigned n = 0; n < std::uint16_t(amount); ++n)
                        require(result.memory.read<2>(output + 2 + n * 2) ==
                                ((!failed.empty() && failed != "DisposeTextToUnicodeInfo") || std::uint16_t(amount) > 255 ? 0xff8e : 0x008e), "Unicode native/fallback byte contract");
                });
            }
        for (unsigned i = 0; i < 7; ++i) {
            auto s = setup(argv[1], i); constexpr auto alternate = fixture + 0x40000;
            s->memory.map(alternate - 0x9000, 0xb000);
            for (const auto& [address, size] : s->memory.mapped_ranges()) {
                const auto low = std::max(std::uint64_t(address), std::uint64_t(toc - 0x9000));
                const auto high = std::min(std::uint64_t(address) + size, std::uint64_t(toc + 0x2000));
                if (low < high) s->memory.copy_in(std::uint32_t(alternate + low - toc), s->memory.copy_out(low, high - low));
            }
            s->wr<4>(8, alternate);
            check(std::move(s), i, "Relocated live TOC " + std::to_string(i));
        }
        for (unsigned amount : {0u, 1u, 8u, 9u, 17u, 255u, 256u}) {
            auto s = setup(argv[1], 0); cstring(*s, std::string(amount, 'x'));
            check(std::move(s), 0, "Pascal unbounded copy/length byte " + std::to_string(amount), {}, nullptr, [=](const State& result) {
                require(result.memory.read<1>(output) == std::uint8_t(amount), "Pascal low-byte length");
                for (unsigned n = 0; n < amount; ++n) require(result.memory.read<1>(output + 1 + n) == 'x', "Pascal retains the whole copied payload");
                require(result.memory.read<1>(output + 1 + amount) == 0xa5, "Pascal does not copy the terminator");
            });
        }
        for (unsigned i : {0u, 5u}) for (int offset : {-2, -1, 0, 1, 2}) {
            auto s = setup(argv[1], i); s->wr<4>(i == 0 ? 12 : 20, text + offset);
            if (i == 5) { s->wr<4>(12, 17); PathModel model; model.errors["CreateTextToUnicodeInfoByEncoding"] = -54;
                check(std::move(s), i, "Live fallback alias " + std::to_string(offset), model);
            } else check(std::move(s), i, "Live Pascal alias " + std::to_string(offset));
        }
        for (const std::string failed : {"UpgradeScriptInfoToTextEncoding", "HGetVol", "FSMakeFSSpec", "FSpMakeFSRef", "FSMakeFSRefUnicode", "FSGetCatalogInfo", "PBGetCatInfoSync", "PBHGetVInfoSync"}) {
            const unsigned i = failed == "UpgradeScriptInfoToTextEncoding" ? 3 : failed.starts_with("PB") ? 1 : 2;
            PathModel model; model.errors[failed] = -54;
            auto s = setup(argv[1], i); if (failed == "FSMakeFSSpec") cstring(*s, "live.dat");
            check(std::move(s), i, "Native service failure " + failed, model, nullptr, [=](const State& result) {
                if (i == 3) require(!result.r<4>(12), "Failed script lookup falls back to encoding zero");
            });
        }
        for (std::uint32_t status : {0u, 0x10000u, 0xffff0000u, 0x80000000u}) {
            PathModel model; model.script_status = status;
            check(setup(argv[1], 3), 3, "Full-word script status " + std::to_string(status), model, nullptr, [=](const State& result) {
                require(result.r<4>(12) == (status ? 0 : 0x12345678), "Script result tests the full status word");
            });
        }
        {
            auto s = setup(argv[1], 5); s->wr<4>(12, 3); s->memory.copy_in(text, {'A', 0x8e, 'Z'});
            PathModel model; model.partial_conversion = true;
            check(std::move(s), 5, "Partial native result overwritten by legacy fallback", model, nullptr, [](const State& result) {
                require(result.memory.read<2>(output) == 3 && result.memory.read<2>(output + 2) == 'A' &&
                        result.memory.read<2>(output + 4) == 0xff8e && result.memory.read<2>(output + 6) == 'Z', "Partial conversion fallback content");
            });
        }
        for (unsigned i = 0; i < 7; ++i) {
            auto initial = setup(argv[1], i); check(sparse(*initial), i, "Minimal path mapping " + std::to_string(i));
            auto probe = clone(*initial); probe->file_path_original = true; Imports imports;
            Effects e; e.state = probe.get(); probe->memory.date_observer = &e; probe->file_path_observer = &imports;
            run(*probe); probe->memory.date_observer = nullptr; probe->file_path_observer = nullptr;
            std::vector<std::uint32_t> accessed;
            for (const auto& access : e.log) if (access.kind == 2 || access.kind == 4) accessed.push_back(access.address + unsigned(access.size) - 1);
            require(!accessed.empty(), "Missing actual path accesses");
            for (unsigned index : {0u, unsigned(accessed.size() / 2), unsigned(accessed.size() - 1)})
                check(sparse(*initial, accessed[index]), i, "Path fault prefix " + std::to_string(i) + "/" + std::to_string(index), {}, nullptr, {}, true);
        }
        Temporary native;
        for (bool modern : {false, true}) for (const std::string& path : std::array<std::string, 5>{"live.dat", "CM User Data:live.dat", "CM User Data:missing.dat", "CM User Data:Folder:live.dat", std::string("CM User Data:Caf") + char(0x8e) + ".dat"})
            for (unsigned i : {0u, 1u, 2u, 4u, 6u}) { auto s = setup(argv[1], i, modern); cstring(*s, path);
                check(std::move(s), i, "Actual native path " + std::to_string(modern) + "/" + std::to_string(i) + "/" + path, {}, &native, [=](const State& result) {
                    if (i == 1 || i == 2 || i == 4) require(std::int32_t(result.r<4>(12)) == (path.ends_with("missing.dat") ? -43 : 0), "Actual existing/missing path result");
                });
            }
        for (unsigned i : {1u, 2u}) {
            auto s = setup(argv[1], i); cstring(*s, "CM User Data:Folder");
            check(std::move(s), i, "Actual directory Classic/modern status " + std::to_string(i), {}, &native, [=](const State& result) {
                require(std::int32_t(result.r<4>(12)) == (i == 1 ? -1302 : 0), "Native directory preserves Classic rejection and modern catalog result");
            });
        }
        check(setup(argv[1], 3), 3, "Actual script encoding", {}, &native);
        {
            auto s = setup(argv[1], 5); s->wr<4>(12, 1); s->memory.write<1>(text, 0x8e);
            check(std::move(s), 5, "Actual MacRoman to Unicode name", {}, &native, [](const State& result) {
                require(result.memory.read<2>(output) == 1 && result.memory.read<2>(output + 2) == 0x00e9, "Actual native MacRoman character conversion");
            });
        }
        {
            auto s = setup(argv[1], 0); Imports observer; s->file_path_observer = &observer;
            s->file_path_original = true; s->file_path_entry_original.fill(true); s->file_path_calls.fill(7); s->file_path_original_calls.fill(9);
            s->file_path_original_resume = file_path_entries[0]; s->file_path_dispatch_escapes = 3; s->file_controller_calls[0] = 42;
            reset_file_path_diagnostics(*s);
            require(!s->file_path_original && !s->file_path_observer && !s->file_path_original_resume && !s->file_path_dispatch_escapes &&
                    s->file_path_entry_original == std::array<bool, 7>{} && s->file_path_calls == std::array<std::uint64_t, 7>{} &&
                    s->file_path_original_calls == std::array<std::uint64_t, 7>{} && s->file_controller_calls[0] == 42, "Independent path diagnostic reset");
        }
        for (unsigned i = 0; i < 7; ++i) { require(witnesses[i] != 0, "Missing production path witness"); std::cout << "file_path.entry=" << hex_address(file_path::entries[i]) << " normal=" << witnesses[i] << '\n'; }
        std::cout << cases << " path full-state/effect comparisons, " << faults << " expected mapping faults, " << negatives
                  << " rejected own/child negatives and " << native_cases << " actual native comparisons\n"; return 0;
    } catch (const std::exception& error) { std::cerr << "File path checks failed: " << error.what() << '\n'; return 1; }
}
