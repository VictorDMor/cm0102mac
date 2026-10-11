#include "aot_runtime.hpp"
#include "data_forks.hpp"
#include "platform.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <sys/stat.h>
#include <unistd.h>

using namespace cm::aot;
namespace fs = std::filesystem;
namespace {
constexpr std::uint32_t fixture = 0x30000000, pb = fixture + 0x200,
    text = fixture + 0x400, spec = fixture + 0x600, vector = fixture + 0x800,
    count_pointer = fixture + 0x900, buffer = fixture + 0x1000, toc = 0x108bba90;
constexpr std::uint64_t large_position = (UINT64_C(1) << 32) + 19;
std::map<std::string, unsigned> calls;
std::string last_call;
unsigned checks = 0, callbacks = 0;
void require(bool ok, const std::string& message) {
    ++checks;
    if (!ok) throw std::runtime_error(message);
}
struct Temporary {
    fs::path path;
    Temporary() {
        auto pattern = (fs::temp_directory_path() / "cm-fork-io-XXXXXX").string();
        if (!mkdtemp(pattern.data())) throw std::runtime_error("Cannot create private fork fixture");
        path = pattern;
    }
    ~Temporary() { std::error_code error; fs::remove_all(path, error); }
};
void pascal(State& s, const std::string& value) {
    s.memory.write<1>(text, value.size());
    s.memory.copy_in(text + 1, {value.begin(), value.end()});
}
std::int32_t call(State& s, const char* name, std::initializer_list<std::uint32_t> arguments) {
    last_call = name;
    unsigned n = 3;
    for (auto argument : arguments) s.wr<4>(n++ * 4, argument);
    s.wr<4>(4128, return_sentinel); s.pc = 0x1234;
    s.budget = s.executed + 100000;
    require(s.platform->dispatch(s, name), std::string(name) + " unhandled");
    require(s.pc == return_sentinel && s.r<4>(4128) == return_sentinel, "Native fork return boundary");
    ++calls[name];
    return std::bit_cast<std::int32_t>(std::uint32_t(s.r<4>(12)));
}
std::int16_t open(State& s, const char* name, unsigned permission) {
    pascal(s, name);
    require(!call(s, "CarbonLib::FSMakeFSSpec", {0xfffffffe, 3, text, spec}), "Private file lookup");
    require(!call(s, "CarbonLib::FSpOpenDF", {spec, permission, count_pointer}), "Private native fork open");
    return std::int16_t(s.memory.read<2>(count_pointer));
}
void parameter(State& s, std::int16_t ref, std::uint64_t offset = 0, unsigned mode = 1,
               unsigned count = 4, bool fork = true) {
    s.memory.copy_in(pb - 4, std::vector<std::uint8_t>(88, 0xa5));
    s.memory.write<4>(pb + 12, 0); s.memory.write<2>(pb + 16, 0x7777);
    s.memory.write<2>(pb + 24, std::uint16_t(ref));
    s.memory.write<4>(pb + 32, buffer); s.memory.write<4>(pb + 36, count);
    s.memory.write<4>(pb + 40, 0xeeeeeeee); s.memory.write<2>(pb + 44, mode);
    if (fork) s.memory.write<8>(pb + 46, offset);
    else s.memory.write<4>(pb + 46, std::uint32_t(offset));
}
void result(State& s, std::int16_t error) {
    const auto observed = std::int16_t(s.memory.read<2>(pb + 16));
    require(observed == error, last_call + " ioResult=" + std::to_string(observed) + " expected=" + std::to_string(error));
    require(s.memory.read<4>(pb - 4) == 0xa5a5a5a5 && s.memory.read<4>(pb + 80) == 0xa5a5a5a5,
            "Fork parameter-block guards");
}
std::vector<std::uint8_t> host_bytes(const fs::path& file) {
    std::ifstream stream(file, std::ios::binary);
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
struct Completion : MemoryObserver {
    State& state;
    unsigned published = 0, pending = 0, callback_writes = 0;
    explicit Completion(State& s) : state(s) {}
    void mapped(std::uint32_t, std::uint64_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t at, const std::uint8_t* bytes, std::size_t size) override {
        if (at == pb + 16 && size == 2) {
            if (read_be<2>(bytes) == 1) ++pending;
            else ++published;
        }
        // This is the real translated completion body, not a test callback.
        // 0x107784a0 takes one pointer and clears bytes +12/+13.
        if (state.pc == 0x107784a4 || state.pc == 0x107784a8) {
            require(published == 1 && pending == 1, "Callback preceded completed I/O");
            require(at == pb + 12 || at == pb + 13, "Completion argument ABI");
            ++callback_writes;
        }
    }
};
void generated(State& s, std::uint32_t entry, std::initializer_list<std::uint32_t> arguments) {
    unsigned n = 3;
    for (auto argument : arguments) s.wr<4>(n++ * 4, argument);
    s.wr<4>(4, 0x700fff00); s.wr<4>(8, toc); s.wr<4>(4128, return_sentinel);
    s.pc = entry; s.budget = s.executed + 100000; run(s);
    require(s.pc == return_sentinel, "Generated stream callback return");
}
}
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    try {
        Temporary temporary;
        const auto game = temporary.path / "Game", user = temporary.path / "User";
        fs::create_directories(game); fs::create_directories(user);
        std::ofstream(game / "media.dat", std::ios::binary) << "original";
        std::ofstream(user / "small.dat", std::ios::binary) << "ABCDEFGH";
        std::ofstream(user / "sparse.dat", std::ios::binary) << "seed";
        fs::resize_file(user / "sparse.dat", large_position + 16);
        auto state = std::make_unique<State>(); auto& s = *state;
        load_program(s, argv[1]); s.memory.map(fixture, 0x220000);
        s.wr<4>(4, 0x700fff00); s.wr<4>(8, toc);
        s.platform = std::make_shared<NativePlatform>(game, user, AudioMode::offline);
        const auto small = open(s, "small.dat", 3), sparse = open(s, "sparse.dat", 3);
        const std::vector<std::uint8_t> payload{'p', 'o', 'r', 't'};

        for (const char* name : {"CarbonLib::PBWriteForkSync", "CarbonLib::PBWriteForkAsync"}) {
            parameter(s, sparse, large_position); s.memory.copy_in(buffer, payload);
            call(s, name, {pb}); result(s, 0);
            require(s.memory.read<4>(pb + 40) == 4 && s.memory.read<8>(pb + 46) == large_position,
                    "64-bit fork write amount and input offset");
            std::ifstream stream(user / "sparse.dat", std::ios::binary);
            stream.seekg(large_position); std::array<char, 4> bytes{}; stream.read(bytes.data(), bytes.size());
            require(std::string(bytes.data(), bytes.size()) == "port", "Write lost the offset high word");
            require(host_bytes(user / "small.dat") == std::vector<std::uint8_t>({'A','B','C','D','E','F','G','H'}),
                    "64-bit write damaged another open fork");
        }
        for (const char* name : {"CarbonLib::PBReadForkSync", "CarbonLib::PBReadForkAsync"}) {
            parameter(s, sparse, large_position); s.memory.clear(buffer, 8);
            call(s, name, {pb}); result(s, 0);
            require(s.memory.read<4>(pb + 40) == 4 && s.memory.copy_out(buffer, 4) == payload,
                    "64-bit fork read payload");
            require(s.memory.read<8>(pb + 46) == large_position, "Fork read overwrote its input offset");
            parameter(s, small, 6, 1, 4); s.memory.copy_in(buffer, std::vector<std::uint8_t>(8, 0xa5));
            call(s, name, {pb}); result(s, -39);
            require(s.memory.read<4>(pb + 40) == 2 && s.memory.copy_out(buffer, 4) ==
                    std::vector<std::uint8_t>({'G','H',0xa5,0xa5}), "Partial EOF count/prefix/guards");
            parameter(s, small, 8); call(s, name, {pb}); result(s, -39);
            require(!s.memory.read<4>(pb + 40), "Empty EOF count");
        }
        for (const char* name : {"CarbonLib::PBGetForkSizeSync", "CarbonLib::PBGetForkSizeAsync"}) {
            parameter(s, sparse); call(s, name, {pb}); result(s, 0);
            require(s.memory.read<8>(pb + 46) == large_position + 16, "64-bit fork EOF high/low order");
        }
        for (const char* name : {"CarbonLib::PBSetForkSizeSync", "CarbonLib::PBSetForkSizeAsync"}) {
            parameter(s, sparse, large_position + 20); call(s, name, {pb}); result(s, 0);
            require(fs::file_size(user / "sparse.dat") == large_position + 20, "64-bit absolute resize");
            parameter(s, sparse, std::uint64_t(-4LL), 2); call(s, name, {pb}); result(s, 0);
            require(fs::file_size(user / "sparse.dat") == large_position + 16, "64-bit EOF-relative shrink");
            parameter(s, sparse, std::uint64_t(-1LL));
            call(s, name, {pb}); require(std::int16_t(s.memory.read<2>(pb + 16)) < 0, "Negative size accepted");
            require(fs::file_size(user / "sparse.dat") == large_position + 16, "Failed resize mutated EOF");
        }
        for (const char* name : {"CarbonLib::PBSetForkPositionSync", "CarbonLib::PBSetForkPositionAsync"}) {
            parameter(s, sparse, large_position); call(s, name, {pb}); result(s, 0);
            parameter(s, sparse, 0, 0); call(s, "CarbonLib::PBReadForkSync", {pb}); result(s, 0);
            require(s.memory.copy_out(buffer, 4) == payload, "64-bit seek/current-mark continuation");
            parameter(s, sparse, std::uint64_t(-4LL), 3); call(s, name, {pb}); result(s, 0);
            parameter(s, sparse, 0, 0); call(s, "CarbonLib::PBReadForkSync", {pb}); result(s, 0);
            require(s.memory.copy_out(buffer, 4) == payload, "Signed mark-relative seek");
        }
        require(!call(s, "CarbonLib::FSSetForkSize", {std::uint32_t(sparse), 1, 1, 39}), "PPC SInt64 register-pair resize");
        require(fs::file_size(user / "sparse.dat") == (UINT64_C(1) << 32) + 39, "Direct resize lost a word");

        for (const char* name : {"CarbonLib::PBReadAsync", "CarbonLib::PBWriteAsync", "CarbonLib::PBGetEOFAsync",
                                 "CarbonLib::PBSetEOFAsync", "CarbonLib::PBSetFPosAsync"}) {
            parameter(s, small, 0, 1, 4, false); s.memory.write<4>(pb + 28, 8);
            if (std::string_view(name) == "CarbonLib::PBWriteAsync") s.memory.copy_in(buffer, {'A','B','C','D'});
            require(call(s, name, {pb}) == 0, "Classic async queue acceptance ABI"); result(s, 0);
            if (std::string_view(name) == "CarbonLib::PBGetEOFAsync") require(s.memory.read<4>(pb + 28) == 8, "Classic async EOF");
            if (std::string_view(name) == "CarbonLib::PBReadAsync" || std::string_view(name) == "CarbonLib::PBWriteAsync")
                require(s.memory.read<4>(pb + 40) == 4 && s.memory.read<4>(pb + 46) == 4, "Classic actualCount/mark output");
        }
        // Each new parameter service must reject a closed/unknown reference,
        // publish the error, and leave independent files intact.
        for (const char* name : {"CarbonLib::PBReadForkSync", "CarbonLib::PBReadForkAsync", "CarbonLib::PBWriteForkSync",
                "CarbonLib::PBWriteForkAsync", "CarbonLib::PBGetForkSizeSync", "CarbonLib::PBGetForkSizeAsync",
                "CarbonLib::PBSetForkSizeSync", "CarbonLib::PBSetForkSizeAsync", "CarbonLib::PBSetForkPositionSync",
                "CarbonLib::PBSetForkPositionAsync", "CarbonLib::PBReadAsync", "CarbonLib::PBWriteAsync",
                "CarbonLib::PBGetEOFAsync", "CarbonLib::PBSetEOFAsync", "CarbonLib::PBSetFPosAsync"}) {
            const bool fork = std::string_view(name).find("Fork") != std::string_view::npos;
            parameter(s, -32768, 0, 1, 4, fork);
            const auto returned = call(s, name, {pb}); result(s, -51);
            if (!fork) require(!returned, "Classic queue result was confused with completed operation error");
        }
        // Completion is a genuine CFM transition to translated code. It sees
        // completed data, executes once, and preserves the interrupted ABI.
        s.memory.write<4>(vector, 0x107784a0); s.memory.write<4>(vector + 4, toc);
        for (const char* name : {"CarbonLib::PBReadForkAsync", "CarbonLib::PBReadAsync"}) {
            parameter(s, small, 0, 1, 4, std::string_view(name).find("Fork") != std::string_view::npos);
            s.memory.write<4>(pb + 12, vector); s.memory.clear(buffer, 4);
            auto registers = s.registers;
            const auto unique = s.unique;
            Completion observer(s); s.memory.date_observer = &observer;
            call(s, name, {pb}); s.memory.date_observer = nullptr;
            require(observer.pending == 1 && observer.published == 1 && observer.callback_writes == 2,
                    "Missing or repeated completion callback");
            require(s.memory.copy_out(buffer, 4) == std::vector<std::uint8_t>({'A','B','C','D'}), "Completion lost read data");
            const auto returned = std::string_view(name).find("Fork") != std::string_view::npos ? pb : 0;
            for (unsigned n = 0; n < 4; ++n) {
                registers[12 + n] = std::uint8_t(returned >> (8 * (3 - n)));
                registers[4128 + n] = std::uint8_t(return_sentinel >> (8 * (3 - n)));
            }
            require(registers == s.registers, "Completion leaked register clobbers");
            require(unique == s.unique, "Completion leaked generated temporaries"); ++callbacks;
        }
        parameter(s, small); s.memory.write<4>(pb + 12, vector);
        call(s, "CarbonLib::PBReadForkSync", {pb}); result(s, 0);
        require(!s.memory.read<4>(pb + 12), "Sync retained a stale completion routine");
        parameter(s, small, 0, 1, 0x80000000);
        call(s, "CarbonLib::PBReadForkSync", {pb}); result(s, -39);
        require(s.memory.read<4>(pb + 40) == 8, "Fork requestCount was incorrectly sign-extended");
        parameter(s, -32768); s.memory.write<4>(pb + 12, vector);
        Completion failed_completion(s); s.memory.date_observer = &failed_completion;
        call(s, "CarbonLib::PBReadForkAsync", {pb}); s.memory.date_observer = nullptr;
        result(s, -51);
        require(failed_completion.pending == 1 && failed_completion.published == 1 &&
                failed_completion.callback_writes == 2, "Failed I/O did not complete exactly once"); ++callbacks;

        for (const char* name : {"CarbonLib::PBReadForkSync", "CarbonLib::PBReadForkAsync",
                                 "CarbonLib::PBWriteForkSync", "CarbonLib::PBWriteForkAsync"}) {
            parameter(s, small, 0, 1, 0); s.memory.write<4>(pb + 32, UINT32_MAX);
            call(s, name, {pb}); result(s, 0);
            require(!s.memory.read<4>(pb + 40), "Zero-count I/O accessed the unmapped buffer");
            const auto unchanged = host_bytes(user / "small.dat");
            parameter(s, small, 0, 1, UINT32_MAX);
            bool rejected = false;
            try { call(s, name, {pb}); }
            catch (const std::runtime_error& error) {
                rejected = std::string_view(error.what()).find("crosses the 32-bit data address boundary") != std::string_view::npos;
            }
            require(rejected && s.memory.read<4>(pb + 40) == 0xeeeeeeee, "Oversized request missed the guest extent guard");
            require(host_bytes(user / "small.dat") == unchanged, "Rejected write changed a private file");
        }
        const auto read_only = open(s, "small.dat", 1);
        parameter(s, read_only); s.memory.copy_in(buffer, payload);
        call(s, "CarbonLib::PBWriteForkSync", {pb}); result(s, -61); // wrPermErr: fork opened read-only.
        require(!s.memory.read<4>(pb + 40), "Read-only write reported bytes");
        require(!call(s, "CarbonLib::FSCloseFork", {std::uint32_t(read_only)}), "Read-only fork close");

        // Exercise the actual generated Mac callbacks over the new imports.
        // A flag in a fixture selects the fork path; production Gestalt stays
        // Classic until the entire FSRef/open/Unicode family is accepted.
        s.memory.write<1>(toc - 0x3504, 1); s.memory.write<1>(toc - 0x3503, 1);
        s.memory.write<4>(count_pointer, 4);
        generated(s, 0x10777140, {std::uint32_t(sparse), buffer, count_pointer, 0});
        require(s.r<4>(12) == 0 && s.memory.read<4>(count_pointer) == 4, "Generated fork read callback");
        s.memory.write<4>(count_pointer, 4); s.memory.copy_in(buffer, payload);
        generated(s, 0x107772e0, {std::uint32_t(sparse), buffer, count_pointer, vector});
        require(s.r<4>(12) == 0 && s.memory.read<4>(count_pointer) == 4, "Generated async fork write callback");
        s.memory.write<4>(count_pointer, 4);
        generated(s, 0x10777430, {std::uint32_t(sparse), count_pointer, 0, vector});
        require(s.r<4>(12) == 0 && s.memory.read<4>(count_pointer) == 4, "Generated async fork seek callback");
        parameter(s, sparse, (UINT64_C(1) << 32) + 35); s.memory.copy_in(buffer, {'t','a','i','l'});
        call(s, "CarbonLib::PBWriteForkSync", {pb}); result(s, 0);
        s.memory.write<4>(count_pointer, std::uint32_t(-4));
        generated(s, 0x10777430, {std::uint32_t(sparse), count_pointer, 2, 0});
        require(s.r<4>(12) == 0 && s.memory.read<4>(count_pointer) == 35,
                "Generated EOF-relative seek lost 64-bit carry or original low-word output");
        parameter(s, sparse, 0, 0); call(s, "CarbonLib::PBReadForkSync", {pb}); result(s, 0);
        require(s.memory.copy_out(buffer, 4) == std::vector<std::uint8_t>({'t','a','i','l'}),
                "Generated EOF-relative seek truncated the native mark");
        s.memory.write<4>(count_pointer, 7);
        generated(s, 0x10777430, {std::uint32_t(sparse), count_pointer, 1, 0});
        require(s.r<4>(12) == 1 && s.memory.read<4>(count_pointer) == 7,
                "Generated backend accepted its originally unsupported origin");
        parameter(s, small, 6); call(s, "CarbonLib::PBSetForkPositionSync", {pb}); result(s, 0);
        s.memory.write<4>(count_pointer, 4);
        generated(s, 0x10777140, {std::uint32_t(small), buffer, count_pointer, 0});
        require(s.r<4>(12) == 0 && s.memory.read<4>(count_pointer) == 2, "Generated partial EOF translation");
        s.memory.write<4>(count_pointer, 4);
        generated(s, 0x10777140, {std::uint32_t(small), buffer, count_pointer, 0});
        require(s.r<4>(12) == 2 && !s.memory.read<4>(count_pointer), "Generated empty EOF translation");

        // A multi-chunk transfer checks that later chunks continue at the native
        // mark rather than reusing the initial absolute offset.
        std::vector<std::uint8_t> bulk(1024 * 1024 + 37);
        for (std::size_t n = 0; n < bulk.size(); ++n) bulk[n] = std::uint8_t(n * 17 + n / 251);
        s.memory.copy_in(buffer, bulk); parameter(s, small, 0, 1, bulk.size());
        call(s, "CarbonLib::PBWriteForkSync", {pb}); result(s, 0);
        require(host_bytes(user / "small.dat") == bulk, "Multi-chunk write extent/continuation");
        s.memory.clear(buffer, bulk.size()); parameter(s, small, 0, 1, bulk.size());
        call(s, "CarbonLib::PBReadForkSync", {pb}); result(s, 0);
        require(s.memory.copy_out(buffer, bulk.size()) == bulk, "Multi-chunk read continuation");

        // Write access to original media uses the existing private overlay.
        pascal(s, "media.dat");
        require(!call(s, "CarbonLib::FSMakeFSSpec", {0xffffffff, 2, text, spec}), "Media fixture lookup");
        require(!call(s, "CarbonLib::FSpOpenDF", {spec, 3, count_pointer}), "Private media write fork");
        const auto media = std::int16_t(s.memory.read<2>(count_pointer));
        parameter(s, media); s.memory.copy_in(buffer, payload);
        call(s, "CarbonLib::PBWriteForkSync", {pb}); result(s, 0);
        require(host_bytes(game / "media.dat") == std::vector<std::uint8_t>({'o','r','i','g','i','n','a','l'}),
                "Fork bridge changed original media");
        require(host_bytes(user / "Files/media.dat") == std::vector<std::uint8_t>({'p','o','r','t','i','n','a','l'}),
                "Fork bridge lost private overlay writes");
        require(!call(s, "CarbonLib::FSCloseFork", {std::uint32_t(media)}), "Media fork close");
        require(!call(s, "CarbonLib::FSCloseFork", {std::uint32_t(sparse)}), "Sparse fork close");
        parameter(s, sparse); call(s, "CarbonLib::PBReadForkSync", {pb}); result(s, -51);
        require(call(s, "CarbonLib::FSCloseFork", {std::uint32_t(sparse)}) == -51, "Double close accepted");
        require(!call(s, "CarbonLib::FSCloseFork", {std::uint32_t(small)}), "Small fork close");
        struct stat metadata{}; require(!stat((user / "sparse.dat").c_str(), &metadata), "Sparse file metadata");
        require(std::uint64_t(metadata.st_blocks) * 512 < (UINT64_C(16) << 20), "64-bit fixture allocated its logical hole");
        for (const auto& [name, count] : calls) std::cout << name << " calls=" << count << '\n';
        std::cout << "Native fork I/O checks: " << checks << ", CFM completions: " << callbacks << '\n';
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
