#include <CoreServices/CoreServices.h>
// The SDK calling-convention macro conflicts with the fixture's string reader.
#undef pascal
#include "file_controller_internal.hpp"
#include "file_controller_adapter.hpp"
#include "file_controller_fixture.hpp"
#include "semantic_io_checks.hpp"
#include "stream_io_adapter.hpp"
#include "platform.hpp"
#include <map>
#include <fstream>
#include <unistd.h>

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

using namespace cm::semantic_io_checks;
namespace {
constexpr std::uint32_t stream = fixture + 0x1000, buffer = fixture + 0x2000,
    text = fixture + 0x3000, output = fixture + 0x4000, vectors = fixture + 0x5000,
    node = fixture + 0x6000, second = fixture + 0x6100, alternate_toc = fixture + 0xd000;
constexpr std::array state_entries{
    0x10776a40u, 0x10776bc0u, 0x10776cd0u, 0x10776d10u, 0x10776d20u, 0x10776d60u,
    0x10777c40u, 0x10777cc0u, 0x10777ce0u, 0x10777d20u, 0x10777f10u, 0x10777f30u,
    0x107784a0u, 0x107784b0u};
unsigned cases = 0, faults = 0, negatives = 0;
std::array<std::uint64_t, 26> witnesses{};
struct TemporaryRoots {
    std::filesystem::path root, game, user;
    TemporaryRoots() {
        auto name = (std::filesystem::temp_directory_path() / "cm-file-controller-XXXXXX").string();
        if (!mkdtemp(name.data())) throw std::runtime_error("Cannot create private file-controller fixture");
        root = name; game = root / "Game"; user = root / "User";
        std::filesystem::create_directories(game); std::filesystem::create_directories(user);
    }
    ~TemporaryRoots() { std::error_code error; std::filesystem::remove_all(root, error); }
};
std::uint32_t reg(const State& s, unsigned n) { return std::uint32_t(s.r<4>(n * 4)); }
void args(State& s, std::uint32_t entry, std::initializer_list<std::uint32_t> values) {
    unsigned n = 3; for (auto value : values) s.wr<4>(n++ * 4, value);
    s.wr<4>(4128, return_sentinel); s.pc = entry; s.budget = s.executed + 100000;
}
void descriptor(State& s, unsigned cls = 1, unsigned state = 2, unsigned mode = 1) {
    s.memory.clear(stream, 84); s.memory.write<4>(stream, 0x1234);
    s.memory.write<2>(stream + 4, (cls << 6) | (mode << 11) | 8);
    s.memory.write<1>(stream + 8, state << 5); s.memory.write<1>(stream + 12, 1);
    s.memory.write<4>(stream + 28, 20); s.memory.write<4>(stream + 32, buffer);
    s.memory.write<4>(stream + 36, 16); s.memory.write<4>(stream + 40, buffer + 4);
    s.memory.write<4>(stream + 44, 16); s.memory.write<4>(stream + 56, 4);
    s.memory.write<4>(stream + 60, vectors); s.memory.write<4>(stream + 68, vectors);
    s.memory.write<4>(stream + 76, 0xabcdef01);
}
std::unique_ptr<State> base(const char* program, unsigned variation = 0) {
    auto s = std::make_unique<State>(); load_program(*s, program); s->memory.map(fixture, 0x10000);
    s->memory.copy_in(fixture, std::vector<std::uint8_t>(0x10000, 0xa5));
    s->memory.copy_in(stack - 8192, std::vector<std::uint8_t>(8256, 0xa7));
    s->registers.fill(0x5b); s->unique.fill(0x6d);
    for (unsigned n = 0; n < 32; ++n) s->wr<4>(n * 4, 0xcafe0000 + n);
    s->wr<4>(4, stack); s->wr<4>(8, toc); s->wr<4>(4128, return_sentinel);
    s->wr<4>(4132, 0x31415926); s->wr<4>(2440, 0xbadc0ffe);
    s->wr<1>(2304, variation % 2 ? 0xf9 : 0xa6); s->wr<1>(1024, variation % 4); s->wr<1>(1027, (variation / 2) % 2);
    descriptor(*s);
    s->memory.write<4>(toc - 0x5a58, output + 0x100); s->memory.write<4>(toc - 0x5a5c, output + 0x110);
    s->memory.write<4>(toc - 0x5a60, output + 0x114); s->memory.write<4>(toc - 0x5a80, output + 0x118);
    s->memory.write<4>(toc - 0x5a54, 0x12345678); s->memory.write<4>(output + 0x110, 0); s->memory.write<4>(output + 0x114, 0);
    s->memory.write<1>(toc - 0x3504, 1); s->memory.write<1>(toc - 0x3503, 1);
    s->memory.write<4>(toc - 0x3508, node); s->memory.write<2>(node, 0xff83);
    s->memory.write<4>(node + 152, second); s->memory.write<2>(second, 123); s->memory.write<4>(second + 152, 0);
    s->memory.copy_in(text, {'r', 'b', '+', 0}); s->memory.write<4>(output, 6);
    s->memory.write<4>(vectors, 0x10775fd0); s->memory.write<4>(vectors + 4, alternate_toc);
    s->memory.write<4>(vectors + 8, 0x10777f90); s->memory.write<4>(vectors + 12, alternate_toc);
    return s;
}
void state_body(State& s) {
    run(s); require(!s.file_controller_dispatch_escapes, "File-controller dispatch escape");
}
struct Imports : FileControllerBoundaryObserver {
    struct Record {
        std::string name; decltype(State::registers) registers; std::uint32_t pc;
        std::uint64_t calls; std::uint32_t heap; std::int16_t error;
        decltype(State::allocated) allocated, available;
        std::vector<std::pair<std::uint32_t, std::uint64_t>> ranges;
        bool operator==(const Record&) const = default;
    };
    std::vector<Record> records;
    cm::file_controller_checks::FileModel model;
    bool fixture_imports = false;
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    void capture(const State& s, const char* n) { records.push_back({n, s.registers, s.pc, s.native_calls, s.heap_end, s.memory_error, s.allocated, s.available, s.memory.mapped_ranges()}); }
    void import_enter(const State& s, const char* n) override { capture(s, n); }
    void import_leave(const State& s, const char* n) override { capture(s, n); }
    bool import_fixture(State& s, const char* n) override { return fixture_imports && model.dispatch(s, n); }
};
struct Negative : Imports {
    struct Boundary {
        Image image;
        std::vector<std::uint8_t> effects;
        std::vector<Record> imports;
        cm::file_controller_checks::FileModel model;
    };
    std::uint32_t target;
    std::vector<Boundary> expected;
    Effects* effects = nullptr;
    bool mutated = false;
    unsigned parent_returns = 0;
    explicit Negative(std::uint32_t entry) : target(entry) {}
    void leave(State& s, std::uint32_t entry, bool original) override {
        if (entry != target) { if (mutated) ++parent_returns; return; }
        if (original) { expected.push_back({Image(s), effects->bytes, records, model}); return; }
        require(!expected.empty(), "Missing file-controller reference boundary");
        const auto& boundary = expected.front();
        boundary.image.compare(s, "Unmutated file-controller boundary " + hex_address(entry));
        require(boundary.effects == effects->bytes && boundary.imports == records && boundary.model == model,
                "File-controller boundary effects/import/dependency prefix " + hex_address(entry));
        s.wr<4>(12, s.r<4>(12) ^ 1); mutated = true;
        boundary.image.compare(s, "Own file-controller boundary " + hex_address(entry));
    }
};
void negative(std::unique_ptr<State> original, std::uint32_t target) {
    auto semantic = clone(*original); original->file_controller_original = true;
    Effects ea, eb; ea.state = original.get(); eb.state = semantic.get();
    Negative observer(target); observer.fixture_imports = true; observer.effects = &ea;
    original->memory.date_observer = &ea; original->file_controller_observer = &observer; run(*original);
    require(!observer.expected.empty(), "Negative fixture did not reach controller boundary " + hex_address(target));
    observer.records.clear(); observer.model = {}; observer.effects = &eb;
    semantic->memory.date_observer = &eb; semantic->file_controller_observer = &observer;
    bool rejected = false; std::string error;
    try { run(*semantic); } catch (const std::runtime_error& e) {
        error = e.what(); rejected = error.starts_with("Own file-controller boundary " + hex_address(target));
    }
    original->memory.date_observer = semantic->memory.date_observer = nullptr;
    original->file_controller_observer = semantic->file_controller_observer = nullptr;
    require(rejected && observer.mutated && !observer.parent_returns, "Mutation escaped file-controller own/child boundary " + hex_address(target) + ": " + error);
    ++negatives;
}
using Expect = std::function<void(const State&)>;
using Configure = std::function<void(Effects&)>;
using ModelExpect = std::function<void(const cm::file_controller_checks::FileModel&)>;
void check(std::unique_ptr<State> original, const std::string& label, const Expect& expected = {},
           bool fault = false, const Configure& configure = {}, const std::function<void()>& restore_host = {},
           const cm::file_controller_checks::FileModel* model = nullptr, const ModelExpect& expected_model = {},
           const std::array<bool, 26>& selected_original = {}) {
    auto semantic = clone(*original); semantic->platform = original->platform;
    semantic->file_controller_entry_original = selected_original;
    original->file_controller_original = true;
    Effects ea, eb; ea.state = original.get(); eb.state = semantic.get();
    if (configure) { configure(ea); configure(eb); }
    Imports ia, ib;
    if (model) { ia.fixture_imports = ib.fixture_imports = true; ia.model = ib.model = *model; }
    if (model && model->pending_async) {
        const auto attach = [](Effects& effects, Imports& observer) {
            effects.on_read = [&observer](State& s, Effects&, std::uint32_t address, unsigned) {
                if (s.pc == 0x10781320 && address == vectors) observer.model.complete_pending(s);
            };
        };
        attach(ea, ia); attach(eb, ib);
    }
    original->memory.date_observer = &ea; semantic->memory.date_observer = &eb;
    original->file_controller_observer = &ia; semantic->file_controller_observer = &ib;
    std::string ax, bx;
    try { run(*original); } catch (const std::exception& e) { ax = e.what(); }
    if (restore_host) restore_host();
    try { state_body(*semantic); } catch (const std::exception& e) { bx = e.what(); }
    original->memory.date_observer = semantic->memory.date_observer = nullptr;
    original->file_controller_observer = semantic->file_controller_observer = nullptr;
    require(ax == bx, label + " error original=" + ax + " semantic=" + bx);
    require(fault == !ax.empty(), label + " fault expectation " + ax);
    if (fault) require(ax.starts_with("Unmapped data access at "), label + " expected mapping fault");
    const auto import_names = [](const Imports& observer) {
        std::string names;
        for (unsigned i = 0; i < observer.records.size(); i += 2) names += " " + observer.records[i].name;
        return names;
    };
    require(original->native_calls == semantic->native_calls, label + " native call count original=" + std::to_string(original->native_calls) + " semantic=" + std::to_string(semantic->native_calls) +
            " original imports:" + import_names(ia) + " semantic imports:" + import_names(ib));
    Image(*original).compare(*semantic, label);
    require(ea.bytes == eb.bytes, label + " ordered memory effects");
    require(ea.callbacks == eb.callbacks && ia.records == ib.records, label + " callback/import ABI");
    require(!semantic->file_controller_dispatch_escapes, label + " dispatch escape");
    for (unsigned i = 0; i < selected_original.size(); ++i) {
        if (selected_original[i]) require(semantic->file_controller_original_calls[i] > 0 && !semantic->file_controller_calls[i], label + " selected original witness");
        else require(!semantic->file_controller_original_calls[i], label + " unselected original path");
    }
    if (model) require(ia.model == ib.model, label + " file dependency history");
    if (expected_model) expected_model(ib.model);
    if (!fault) for (unsigned i = 0; i < witnesses.size(); ++i) witnesses[i] += semantic->file_controller_calls[i];
    if (expected) expected(*semantic); ++cases; if (fault) ++faults;
}
void cstring(State& s, std::uint32_t at, const std::string& value) {
    std::vector<std::uint8_t> bytes(value.begin(), value.end()); bytes.push_back(0); s.memory.copy_in(at, bytes);
}
std::unique_ptr<State> full_setup(const char* program, unsigned i, bool modern = true) {
    auto s = base(program, i); const auto entry = file_controller_entries[i];
    cstring(*s, text, "CM User Data:live.dat"); cstring(*s, text + 128, "CM User Data:next.dat"); cstring(*s, text + 256, "rb");
    s->memory.write<1>(toc - 0x3503, modern); s->memory.write<4>(toc - 0x3508, 0);
    const auto root = fixture + 0x7000; s->memory.clear(root, 252); s->memory.write<4>(toc - 0x5a68, root);
    descriptor(*s, 0, 0); s->memory.write<4>(output, 4);
    switch (i) {
    case 0: case 16: args(*s, entry, {text}); break;
    case 1: case 17: args(*s, entry, {text, text + 128}); break;
    case 2: args(*s, entry, {text, text + 256}); break;
    case 3: args(*s, entry, {text, text + 256, stream}); break;
    case 4: args(*s, entry, {text + 256, output}); break;
    case 5: case 8: case 25: args(*s, entry, {}); break;
    case 6: cm::file_controller_checks::FileModel::spec(*s, node, "live.dat"); args(*s, entry, {node, 0}); break;
    case 7: s->memory.write<4>(toc - 0x3508, node); s->memory.write<2>(node, 121); args(*s, entry, {121}); break;
    case 9: case 10: args(*s, entry, {0}); break;
    case 11: args(*s, entry, {text, 0x08480000, stream}); break;
    case 12: case 13: args(*s, entry, {121, buffer, output, 0}); break;
    case 14: args(*s, entry, {121, output, 0, 0}); break;
    case 15: args(*s, entry, {121}); break;
    case 18: case 19: descriptor(*s); args(*s, entry, {stream}); break;
    case 20: case 23: descriptor(*s); args(*s, entry, {stream, output}); break;
    case 21: case 22: descriptor(*s); args(*s, entry, {stream, 6, 1}); break;
    case 24: args(*s, entry, {stream}); break;
    }
    return s;
}
std::unique_ptr<State> setup(const char* program, unsigned i) {
    auto s = base(program, i); const auto entry = state_entries[i];
    if (i == 0) args(*s, entry, {text, output});
    else if (i == 2) args(*s, entry, {123});
    else if (i == 4 || i == 5) args(*s, entry, {0});
    else if (i == 8) args(*s, entry, {stream, output});
    else if (i == 9 || i == 10) args(*s, entry, {stream, 6, 0});
    else if (i == 11) args(*s, entry, {stream, output});
    else args(*s, entry, {stream});
    return s;
}
std::unique_ptr<State> sparse(const State& initial, std::uint64_t omitted = UINT64_MAX,
                              const cm::file_controller_checks::FileModel* model = nullptr) {
    auto probe = clone(initial); probe->platform = initial.platform; probe->file_controller_original = true;
    Imports observer;
    if (model) { observer.model = *model; observer.fixture_imports = true; }
    Effects e; e.state = probe.get(); probe->memory.date_observer = &e; probe->file_controller_observer = &observer;
    run(*probe); probe->memory.date_observer = nullptr; probe->file_controller_observer = nullptr;
    auto result = std::make_unique<State>(); result->registers = initial.registers; result->unique = initial.unique;
    result->pc = initial.pc; result->budget = initial.budget; result->platform = initial.platform;
    result->allocated = initial.allocated; result->available = initial.available; result->heap_end = initial.heap_end; result->memory_error = initial.memory_error;
    for (const auto& [begin, length] : e.accesses) for (const auto& [address, size] : initial.memory.mapped_ranges()) {
        const auto low = std::max(std::uint64_t(begin), std::uint64_t(address)), high = std::min(std::uint64_t(begin) + length, std::uint64_t(address) + size);
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
        for (bool modern : {false, true}) for (unsigned i = 0; i < file_controller_entries.size(); ++i) {
            cm::file_controller_checks::FileModel model;
            check(full_setup(argv[1], i, modern), "Complete controller direct " + std::to_string(modern) + "/" + hex_address(file_controller_entries[i]), {}, false, {}, {}, &model);
            auto s = full_setup(argv[1], i, modern); s->memory.write<4>(fixture, file_controller_entries[i]); s->memory.write<4>(fixture + 4, toc);
            s->wr<4>(48, fixture); s->pc = 0x10781320;
            check(std::move(s), "Generated controller indirect " + std::to_string(modern) + "/" + std::to_string(i), {}, false, {}, {}, &model);
        }
        for (unsigned i = 0; i < state_entries.size(); ++i) check(setup(argv[1], i), "Direct file state " + hex_address(state_entries[i]));
        for (unsigned i = 0; i < file_controller_entries.size(); ++i)
            negative(full_setup(argv[1], i), file_controller_entries[i]);
        negative(full_setup(argv[1], 2), 0x10776a40); // mode child before fopen
        negative(full_setup(argv[1], 3), 0x10776da0); // open backend before reopen
        negative(full_setup(argv[1], 12), 0x10776bc0); // capability before read
        negative(full_setup(argv[1], 22), 0x10777c40); // tell before seek facade
        negative(full_setup(argv[1], 0), 0x10777990); // backend before remove facade
        for (const auto [parent, child] : {std::pair{2u, 4u}, {3u, 11u}, {12u, 5u}, {22u, 18u}, {0u, 16u}}) {
            cm::file_controller_checks::FileModel model;
            std::array<bool, 26> selection{}; selection[child] = true;
            check(full_setup(argv[1], parent), "Mixed original child " + std::to_string(parent) + "/" + std::to_string(child), {}, false, {}, {}, &model, {}, selection);
            selection.fill(false); selection[parent] = true;
            check(full_setup(argv[1], parent), "Mixed original parent " + std::to_string(parent) + "/" + std::to_string(child), {}, false, {}, {}, &model, {}, selection);
        }
        for (bool modern : {false, true}) for (bool existing : {false, true})
            for (const std::string mode : {"rb", "wb", "ab", "rb+", "wb+", "ab+"}) {
                auto s = full_setup(argv[1], 3, modern); cstring(*s, text + 256, mode);
                cm::file_controller_checks::FileModel model; model.files[7].exists = existing;
                check(std::move(s), "Reopen existing/mode " + std::to_string(modern) + "/" + std::to_string(existing) + "/" + mode,
                      [=](const State& s) {
                          require(reg(s, 3) == (existing || mode[0] != 'r' ? stream : 0), "Reopen missing/read policy");
                      }, false, {}, {}, &model, [=](const auto& history) {
                          const auto id = history.find("live.dat");
                          require(bool(id) == (existing || mode[0] != 'r'), "Reopen create policy");
                          if (id) require(history.files.at(id).size == (existing && mode[0] != 'w' ? 8 : 0), "Reopen truncate/retain size");
                          if (id && mode[0] == 'a') require(history.position == history.files.at(id).size, "Append starts at EOF");
                      });
            }
        for (bool modern : {false, true}) for (unsigned i : {12u, 13u, 14u}) {
            auto s = full_setup(argv[1], i, modern); s->wr<4>(24, vectors);
            s->memory.write<4>(vectors, 0x10776d10); // yield changes volatile r3
            cm::file_controller_checks::FileModel model; model.pending_async = true;
            check(std::move(s), "Pending completion " + std::to_string(modern) + "/" + std::to_string(i),
                  [](const State& s) { require(!reg(s, 3), "Pending I/O result"); }, false, {}, {}, &model,
                  [](const auto& history) { require(!history.pending_block, "Pending I/O was not completed by the yield callback"); });
        }
        for (bool modern : {false, true}) for (const auto& injected : {"FSOpenFork", "FSSetForkSize", "PBHOpenDFSync", "PBSetEOFSync"}) {
            if (modern != std::string(injected).starts_with("FS")) continue;
            auto s = full_setup(argv[1], 3, modern); cstring(*s, text + 256, "wb");
            cm::file_controller_checks::FileModel model; model.errors[injected] = -54;
            check(std::move(s), "Open/truncate failure " + std::string(injected),
                  [](const State& s) { require(!reg(s, 3) && !(s.memory.read<2>(stream + 4) & 0x1c0), "Failed open clears descriptor class"); }, false, {}, {}, &model);
        }
        for (bool modern : {false, true}) for (unsigned i : {0u, 1u, 16u, 17u}) {
            auto s = full_setup(argv[1], i, modern);
            cm::file_controller_checks::FileModel model; model.files[7].exists = false;
            check(std::move(s), "Missing remove/rename " + std::to_string(modern) + "/" + std::to_string(i),
                  [=](const State& s) { require(reg(s, 3) == (i < 2 ? UINT32_MAX : 1), "Missing file facade/backend error"); }, false, {}, {}, &model);
        }
        for (bool modern : {false, true}) for (unsigned location : {0u, 1u, 2u}) for (unsigned failure : {0u, 1u, 2u}) {
            auto s = full_setup(argv[1], 15, modern);
            args(*s, 0x10775130, {160}); run(*s); const auto record = reg(*s, 3);
            require(record != 0, "Temporary record guest allocation");
            s->memory.clear(record, 160); s->memory.write<2>(record, 121);
            cm::file_controller_checks::FileModel::spec(*s, record + 2, "live.dat");
            cm::file_controller_checks::FileModel::reference(*s, record + 72, 7);
            s->memory.clear(node, 160); s->memory.clear(second, 160);
            s->memory.write<2>(node, 12); s->memory.write<2>(second, 13);
            s->memory.write<4>(toc - 0x3508, location == 0 ? record : node);
            s->memory.write<4>(record + 156, location == 0 ? 0 : node);
            s->memory.write<4>(record + 152, location == 2 ? 0 : second);
            s->memory.write<4>(node + 152, record); s->memory.write<4>(second + 156, record);
            args(*s, file_controller_entries[15], {121});
            cm::file_controller_checks::FileModel model;
            if (failure) model.errors[modern ? failure == 1 ? "FSCloseFork" : "FSDeleteObject" : failure == 1 ? "PBCloseSync" : "PBHDeleteSync"] = -54;
            check(std::move(s), "Temporary unlink location/error " + std::to_string(modern) + "/" + std::to_string(location) + "/" + std::to_string(failure),
                  [=](const State& s) {
                      require(reg(s, 3) == unsigned(failure != 0), "Temporary close/delete result");
                      require(s.allocated.empty(), "Temporary record allocation remained owned");
                      require(s.memory.read<4>(toc - 0x3508) == (location == 0 ? second : node), "Temporary head unlink");
                      if (location != 0) require(s.memory.read<4>(node + 152) == (location == 2 ? 0 : second), "Temporary previous unlink");
                      if (location != 2) require(s.memory.read<4>(second + 156) == (location == 0 ? 0 : node), "Temporary next unlink");
                  }, false, {}, {}, &model, [=](const auto& history) {
                      require(history.files.at(7).exists == (failure != 0), "Temporary delete only after successful close");
                  });
        }
        {
            TemporaryRoots roots;
            auto s = setup(argv[1], 1);
            s->platform = std::make_shared<NativePlatform>(roots.game, roots.user, AudioMode::offline);
            s->memory.write<1>(toc - 0x3504, 0); s->memory.write<1>(toc - 0x3503, 0);
            check(std::move(s), "Native default fork capability discovery", [](const State& s) {
                require(reg(s, 3) == 1 && s.memory.read<1>(toc - 0x3504) == 1 && s.memory.read<1>(toc - 0x3503) == 1 && s.native_calls == 1, "Capability queried actual native service");
            });
        }
        for (unsigned cached : {0u, 1u, 0x80u, 0xffu}) {
            auto s = setup(argv[1], 1); s->memory.write<1>(toc - 0x3504, 0x80); s->memory.write<1>(toc - 0x3503, cached);
            check(std::move(s), "Cached fork capability " + std::to_string(cached), [cached](const State& s) {
                require(reg(s, 3) == cached && !s.native_calls, "Cached result retains byte value without another query");
            });
        }
        {
            auto s = base(argv[1]); descriptor(*s, 0, 0); args(*s, 0x10776940, {text, text + 8, 0});
            check(std::move(s), "Null reopen descriptor", [](const State& s) { require(!reg(s, 3), "Null reopen still initializes stdio"); });
        }
        {
            auto s = base(argv[1]); descriptor(*s, 0, 0); s->memory.write<1>(text + 8, 'x'); args(*s, 0x10776940, {text, text + 8, stream});
            check(std::move(s), "Reopen invalid mode after clearing status", [](const State& s) {
                require(!reg(s, 3) && !s.memory.read<1>(stream + 12) && !s.memory.read<1>(stream + 13), "Rejected mode preserves prior controller effects");
            });
        }
        for (const auto& mode : {"r", "w", "a", "rb", "r+", "rb+", "r+b", "wb+", "w+b", "ab+", "a+b", "rxyz", "w+xyz", "", "x", "R", "b", "\xff"}) {
            auto s = setup(argv[1], 0); const std::string value(mode);
            std::vector<std::uint8_t> bytes(value.begin(), value.end()); bytes.push_back(0); s->memory.copy_in(text, bytes);
            check(std::move(s), "Mode " + value, [value](const State& s) {
                const bool valid = !value.empty() && (value[0] == 'r' || value[0] == 'a' || value[0] == 'w');
                require(reg(s, 3) == unsigned(valid), "Mode validity");
                require(((s.memory.read<2>(output) >> 6) & 7) == 1, "Mode preserves class initialization even on rejection");
                require(!(s.memory.read<1>(output + 1) & 0x30), "Mode clears orientation");
            });
        }
        for (unsigned alias : {0u, 1u, 2u}) {
            auto s = setup(argv[1], 0); s->wr<4>(16, text + alias);
            check(std::move(s), "Mode output aliases source " + std::to_string(alias));
        }
        for (unsigned cls : {0u, 1u, 2u, 3u, 7u}) for (unsigned state : {0u, 1u, 2u, 3u, 4u, 7u}) {
            auto s = setup(argv[1], 6); descriptor(*s, cls, state); check(std::move(s), "Tell class/state " + std::to_string(cls) + "/" + std::to_string(state), [=](const State& s) {
                const auto position = !state ? 20u : 8u - (state >= 3 ? state - 2 : 0);
                require(reg(s, 3) == (cls == 1 || cls == 2 ? position : UINT32_MAX), "Tell position independent result");
            });
        }
        for (unsigned i : {6u, 7u, 8u, 9u, 10u, 11u}) {
            auto s = setup(argv[1], i); s->memory.write<1>(stream + 13, 1);
            check(std::move(s), "Sticky error entry " + std::to_string(i));
        }
        for (unsigned state : {0u, 1u, 2u, 3u, 4u}) for (unsigned origin : {0u, 1u, 2u, 3u}) for (unsigned offset : {3u, 4u, 19u, 20u, UINT32_MAX}) {
            auto s = setup(argv[1], 9); descriptor(*s, 1, state); s->wr<4>(16, offset); s->wr<4>(20, origin);
            check(std::move(s), "Seek state/origin/offset " + std::to_string(state) + "/" + std::to_string(origin) + "/" + std::to_string(offset), [=](const State& s) {
                require(!reg(s, 3), "Successful seek result");
                if (state == 2 && origin == 0 && offset >= 4 && offset < 20) {
                    require(s.memory.read<4>(stream + 40) == buffer + offset - 4 && s.memory.read<4>(stream + 44) == 20 - offset, "Buffered seek pointer/count");
                    require(s.memory.read<1>(stream + 12) == 1, "Buffered seek retains EOF");
                }
            });
        }
        for (bool drain : {false, true}) {
            auto s = setup(argv[1], 9); descriptor(*s, 1, drain ? 1 : 0);
            s->memory.write<4>(stream + (drain ? 68 : 60), vectors + 8);
            check(std::move(s), drain ? "Drain error before seek" : "Seek callback rejection", [](const State& s) {
                require(reg(s, 3) == UINT32_MAX && s.memory.read<1>(stream + 13) == 1 && !s.memory.read<4>(stream + 44) && s.memory.read<4>(output + 0x100) == 40, "Seek error contract");
            });
        }
        // The shared owned fork has exactly the same native reference in both
        // states. Restore its payload/size/position between executions instead
        // of normalizing different SDK reference numbers in the guest image.
        for (bool modern : {false, true}) for (bool writing : {false, true}) for (bool async : {false, true}) for (unsigned amount : {0u, 4u, 16u, 32u}) {
            if (writing && amount == 32) continue;
            TemporaryRoots roots; const auto path = roots.user / "data.dat"; std::ofstream(path, std::ios::binary) << "ABCDEFGH";
            auto s = base(argv[1]); s->platform = std::make_shared<NativePlatform>(roots.game, roots.user, AudioMode::offline);
            s->memory.write<1>(text, 8); s->memory.copy_in(text + 1, {'d','a','t','a','.','d','a','t'});
            args(*s, 0, {0xfffffffe, 3, text, node}); require(s->platform->dispatch(*s, "CarbonLib::FSMakeFSSpec") && !reg(*s, 3), "Native fork fixture FSSpec");
            args(*s, 0, {node, 3, output}); require(s->platform->dispatch(*s, "CarbonLib::FSpOpenDF") && !reg(*s, 3), "Native fork fixture open");
            const auto fork = std::int16_t(s->memory.read<2>(output));
            const bool empty_eof = amount == 32;
            if (empty_eof) require(!::FSSetForkPosition(fork, fsFromStart, 8), "Empty EOF fixture position");
            s->memory.write<1>(toc - 0x3503, modern); s->memory.write<4>(output, amount);
            s->memory.copy_in(buffer, std::vector<std::uint8_t>(32, 'z'));
            args(*s, writing ? 0x107772e0 : 0x10777140, {std::uint32_t(fork), buffer, output, async ? vectors : 0u});
            check(std::move(s), "Native transfer modern/write/async/amount " + std::to_string(modern) + "/" + std::to_string(writing) + "/" + std::to_string(async) + "/" + std::to_string(amount), [=](const State& s) {
                const auto actual = empty_eof ? 0 : writing ? amount : std::min(amount, 8u);
                require(reg(s, 3) == (empty_eof ? 2u : 0u) && s.memory.read<4>(output) == actual, "Actual native transfer count/EOF result");
                if (!writing && actual) {
                    std::vector<std::uint8_t> expected{'A','B','C','D','E','F','G','H'}; expected.resize(actual);
                    require(s.memory.copy_out(buffer, actual) == expected, "Native transfer payload");
                }
            }, false, {}, [&] {
                std::ofstream(path, std::ios::binary | std::ios::trunc) << "ABCDEFGH";
                require(!::FSSetForkPosition(fork, fsFromStart, empty_eof ? 8 : 0), "Restore native transfer position");
            });
        }
        for (bool modern : {false, true}) for (bool async : {false, true}) for (unsigned origin : {0u, 1u, 2u, 3u}) for (unsigned offset : {3u, 11u, UINT32_MAX}) {
            TemporaryRoots roots; const auto path = roots.user / "data.dat"; std::ofstream(path, std::ios::binary) << "ABCDEFGH";
            auto s = base(argv[1]); s->platform = std::make_shared<NativePlatform>(roots.game, roots.user, AudioMode::offline);
            s->memory.write<1>(text, 8); s->memory.copy_in(text + 1, {'d','a','t','a','.','d','a','t'});
            args(*s, 0, {0xfffffffe, 3, text, node}); require(s->platform->dispatch(*s, "CarbonLib::FSMakeFSSpec") && !reg(*s, 3), "Seek fixture FSSpec");
            args(*s, 0, {node, 3, output}); require(s->platform->dispatch(*s, "CarbonLib::FSpOpenDF") && !reg(*s, 3), "Seek fixture open");
            const auto fork = std::int16_t(s->memory.read<2>(output));
            s->memory.write<1>(toc - 0x3503, modern); s->memory.write<4>(output, offset);
            args(*s, 0x10777430, {std::uint32_t(fork), output, origin, async ? vectors : 0u});
            check(std::move(s), "Native seek modern/async/origin/offset " + std::to_string(modern) + "/" + std::to_string(async) + "/" + std::to_string(origin) + "/" + std::to_string(offset), [=](const State& s) {
                const bool valid = (origin == 0 && offset != UINT32_MAX) || origin == 2;
                require(reg(s, 3) == unsigned(!valid), "Native seek independent origin/sign result");
                if (valid) require(s.memory.read<4>(output) == (origin == 2 ? 8 + offset : offset), "Native seek low-word position");
            }, false, {}, [&] {
                require(!::FSSetForkSize(fork, fsFromStart, 8) && !::FSSetForkPosition(fork, fsFromStart, 0), "Restore native seek size/position through the same SDK");
            });
        }
        for (std::uint64_t size : {(UINT64_C(1) << 32) - 2, (UINT64_C(1) << 32) + 19})
            for (std::int32_t offset : {7, -1, -32}) {
                TemporaryRoots roots; const auto path = roots.user / "sparse.dat";
                std::ofstream(path, std::ios::binary) << "seed"; std::filesystem::resize_file(path, size);
                auto s = base(argv[1]); s->platform = std::make_shared<NativePlatform>(roots.game, roots.user, AudioMode::offline);
                s->memory.write<1>(text, 10); s->memory.copy_in(text + 1, {'s','p','a','r','s','e','.','d','a','t'});
                args(*s, 0, {0xfffffffe, 3, text, node}); require(s->platform->dispatch(*s, "CarbonLib::FSMakeFSSpec") && !reg(*s, 3), "Sparse seek FSSpec");
                args(*s, 0, {node, 3, output}); require(s->platform->dispatch(*s, "CarbonLib::FSpOpenDF") && !reg(*s, 3), "Sparse seek owned fork");
                const auto fork = std::int16_t(s->memory.read<2>(output));
                s->memory.write<1>(toc - 0x3503, 1); s->memory.write<4>(output, std::uint32_t(offset));
                args(*s, 0x10777430, {std::uint32_t(fork), output, 2, 0});
                const auto desired = std::uint64_t(std::int64_t(size) + offset);
                check(std::move(s), "Native sparse end seek " + std::to_string(size) + "/" + std::to_string(offset), [=](const State& s) {
                    SInt64 position = 0, length = 0;
                    require(!reg(s, 3) && s.memory.read<4>(output) == std::uint32_t(desired), "Sparse low-word position result");
                    require(!::FSGetForkPosition(fork, &position) && std::uint64_t(position) == desired &&
                            !::FSGetForkSize(fork, &length) && std::uint64_t(length) == std::max(size, desired), "Sparse real signed-64-bit position/size");
                }, false, {}, [&] {
                    require(!::FSSetForkSize(fork, fsFromStart, size) && !::FSSetForkPosition(fork, fsFromStart, 0), "Restore sparse SDK state");
                });
            }
        for (unsigned query : {0xffff0083u, 0x0000ff83u, 123u, 0u, 0x80000000u}) {
            auto s = setup(argv[1], 2); s->wr<4>(12, query);
            check(std::move(s), "Signed file reference lookup " + std::to_string(query), [query](const State& s) {
                require(reg(s, 3) == (std::int16_t(query) == -125 ? node : std::int16_t(query) == 123 ? second : 0), "Lookup signed word identity");
            });
        }
        for (unsigned i : {4u, 5u}) for (unsigned request : {0u, 1u, UINT32_MAX}) for (unsigned override : {0u, 0x41424344u, 0x80000000u}) {
            auto s = setup(argv[1], i); s->wr<4>(12, request); s->memory.write<4>(output + (i == 4 ? 0x110 : 0x114), override);
            check(std::move(s), "Metadata default/override " + std::to_string(i) + "/" + std::to_string(request) + "/" + std::to_string(override), [=](const State& s) {
                require(reg(s, 3) == (override ? override : i == 4 ? (request ? 0x3f3f3f3fu : 0x43574945u) : (request ? 0x42494e41u : 0x54455854u)), "Metadata code independent result");
            });
        }
        for (unsigned i = 0; i < state_entries.size(); ++i) {
            if (i == 3) continue; // The constant alias policy has no memory access.
            auto initial = setup(argv[1], i); check(sparse(*initial), "Minimal mapping " + std::to_string(i));
            auto probe = clone(*initial); probe->file_controller_original = true; Effects e; e.state = probe.get(); probe->memory.date_observer = &e; run(*probe); probe->memory.date_observer = nullptr;
            std::vector<std::uint32_t> bytes; for (const auto& access : e.log) if (access.kind == 2 || access.kind == 4) bytes.push_back(access.address + unsigned(access.size) - 1);
            require(!bytes.empty(), "Missing accessed bytes");
            for (unsigned index : {0u, unsigned(bytes.size() / 2), unsigned(bytes.size() - 1)}) check(sparse(*initial, bytes[index]), "Access prefix " + std::to_string(i) + "/" + std::to_string(index), {}, true);
        }
        for (bool modern : {false, true}) for (unsigned i : {0u, 1u, 2u, 3u, 6u, 11u, 12u, 13u, 14u, 15u, 16u, 17u}) {
            cm::file_controller_checks::FileModel model;
            auto initial = full_setup(argv[1], i, modern);
            check(sparse(*initial, UINT64_MAX, &model), "Controller minimal mapping " + std::to_string(modern) + "/" + std::to_string(i), {}, false, {}, {}, &model);
            auto probe = clone(*initial); probe->file_controller_original = true;
            Effects effects; effects.state = probe.get(); Imports imports; imports.fixture_imports = true;
            probe->memory.date_observer = &effects; probe->file_controller_observer = &imports; run(*probe);
            probe->memory.date_observer = nullptr; probe->file_controller_observer = nullptr;
            std::vector<std::uint32_t> bytes;
            for (const auto& access : effects.log) if (access.kind == 2 || access.kind == 4) {
                const auto byte = access.address + unsigned(access.size) - 1;
                for (const auto& [address, size] : initial->memory.mapped_ranges())
                    if (byte >= address && std::uint64_t(byte) < std::uint64_t(address) + size) { bytes.push_back(byte); break; }
            }
            require(!bytes.empty(), "Missing file-controller initial access prefix");
            for (unsigned index : {0u, unsigned(bytes.size() / 2), unsigned(bytes.size() - 1)})
                check(sparse(*initial, bytes[index], &model), "Controller fault prefix " + std::to_string(modern) + "/" + std::to_string(i) + "/" + std::to_string(index), {}, true, {}, {}, &model);
        }
        {
            auto s = base(argv[1]); Imports observer;
            s->file_controller_original = true; s->file_controller_entry_original.fill(true);
            s->file_controller_calls.fill(7); s->file_controller_original_calls.fill(9);
            s->file_controller_original_resume = file_controller_entries[0]; s->file_controller_dispatch_escapes = 3;
            s->file_controller_observer = &observer; s->stream_io_calls[0] = 42;
            reset_file_controller_diagnostics(*s);
            require(!s->file_controller_original && !s->file_controller_original_resume && !s->file_controller_dispatch_escapes &&
                    !s->file_controller_observer && s->file_controller_entry_original == std::array<bool, 26>{} &&
                    s->file_controller_calls == std::array<std::uint64_t, 26>{} &&
                    s->file_controller_original_calls == std::array<std::uint64_t, 26>{} && s->stream_io_calls[0] == 42,
                    "Independent file-controller diagnostic reset");
        }
        for (unsigned i = 0; i < witnesses.size(); ++i) {
            require(witnesses[i] > 0, "Missing normal file-controller witness " + hex_address(file_controller_entries[i]));
            std::cout << "file_controller.entry=" << hex_address(file_controller_entries[i]) << " normal=" << witnesses[i] << '\n';
        }
        std::cout << cases << " file-controller full-state/effect comparisons, " << faults << " expected mapping faults and " << negatives << " rejected boundary negatives\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "File controller checks failed: " << e.what() << '\n'; return 1; }
}
