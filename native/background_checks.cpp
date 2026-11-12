#include "aot_runtime.hpp"
#include <iostream>
#include <vector>

using namespace cm::aot;
namespace {
constexpr std::uint32_t fixture = 0x30000000, category = fixture + 0x100;
constexpr std::uint32_t tree = fixture + 0x200, lists = fixture + 0x300;
constexpr std::uint32_t node = fixture + 0x400, alt_node = fixture + 0x420;
constexpr std::uint32_t club = fixture + 0x500, descriptor = fixture + 0x800;
constexpr std::uint32_t primary_image = fixture + 0x900, alternate_image = fixture + 0x940;
constexpr std::uint32_t random_cursor = fixture + 0xa00, random_end = fixture + 0xa04;
constexpr std::uint32_t random_seed = fixture + 0xa08, random_bytes = fixture + 0xb00;
constexpr std::uint32_t data_mode = fixture + 0xc00, mapping = fixture + 0xc10;
constexpr std::uint32_t mapping_table = fixture + 0xc20, cache = fixture + 0xd00;
struct BackgroundCase {
    std::uint32_t category_id, wrapper, getter_return, getter, null_address;
};
// Nation, club, competition, award-screen (null from 10713514) and match/tactics-screen (null from 1071367c)
// wrappers. The getters are DATABASE.CPP's C++ owners, which report a fault at their entry.
constexpr BackgroundCase cases[] = {
    {0, 0x10713a80, 0x10713aa0, 0x100d4df0, 0x68},
    {1, 0x10713ad0, 0x10713af0, 0x100d5310, 0},
    {3, 0x10713b20, 0x10713b40, 0x100d4fd0, 0},
    {4, 0x10713b70, 0x10713b90, 0x100d50e0, 0},
    {2, 0x10713bc0, 0x10713be0, 0x100d5450, 0},
};
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
std::unique_ptr<State> setup(const char* program, const BackgroundCase& type) {
    auto s = std::make_unique<State>();
    load_program(*s, program);
    s->background_original = true; // Keep this original-body regression independent of semantic hooks.
    s->memory.map(fixture, 0x10000);
    s->memory.write<1>(0x108b84bc, 1); // Original background-picture enable flag.
    s->memory.write<4>(0x108b5e54, fixture);
    s->memory.write<4>(fixture + type.category_id * 4, category); // Original picture category.
    s->memory.write<4>(category, tree);
    s->memory.write<4>(category + 12, 1);
    s->memory.write<4>(tree, descriptor);
    s->memory.write<4>(tree + 4, lists);
    s->memory.write<4>(lists, node);
    s->memory.write<4>(lists + 4, node);
    s->memory.write<4>(lists + 8, 1);
    s->memory.write<4>(lists + 12, alt_node);
    s->memory.write<4>(lists + 16, alt_node);
    s->memory.write<4>(lists + 20, 1);
    s->memory.write<4>(node, primary_image);
    s->memory.write<4>(alt_node, alternate_image);
    s->memory.copy_in(primary_image, {'c','l','u','b','.','r','g','m',0});
    s->memory.copy_in(alternate_image, {'a','l','t','.','r','g','m',0});
    s->memory.write<4>(0x108b3f0c, random_cursor);
    s->memory.write<4>(0x108b3f08, random_end);
    s->memory.write<4>(0x108b3f00, random_seed);
    s->memory.write<4>(0x108b3f04, random_bytes);
    s->memory.write<4>(random_cursor, random_bytes);
    s->memory.write<4>(random_end, random_bytes + 128);
    s->memory.write<4>(random_seed, 7);
    s->memory.write<4>(0x108b3fe0, data_mode);
    s->memory.write<1>(data_mode, 2); // The loaded-database path from the failure.
    s->memory.write<4>(0x108b4598, mapping);
    s->memory.write<4>(mapping, mapping_table);
    s->memory.write<4>(mapping_table, 0);
    s->memory.write<4>(club, 0);
    s->memory.write<4>(0x108b80ac, cache);
    s->memory.write<4>(cache, 1);
    s->memory.write<4>(cache + 32, descriptor); // Club name cache.
    s->memory.write<4>(cache + 44, descriptor); // Nation name cache (nation index at +0x68 is zero).
    s->memory.write<4>(cache + 20, 1); // One regular competition.
    s->memory.write<4>(cache + 56, descriptor); // Competition name cache.
    s->memory.write<4>(cache + 52, descriptor); // Category 2 rows (0x34 bytes each).
    s->memory.write<4>(cache + 60, descriptor); // Category 4 rows (0x4f bytes each).
    for (unsigned r = 14; r < 32; ++r) s->wr<4>(r * 4, 0xcafe0000u + r);
    s->budget = 10000;
    return s;
}
void preserved(const State& s) {
    require(s.r<4>(4) == 0x700fff00 && s.r<4>(8) == 0x108bba90, "Background lookup changed stack or TOC");
    for (unsigned r = 14; r < 32; ++r)
        require(s.r<4>(r * 4) == 0xcafe0000u + r, "Background lookup changed a nonvolatile register");
}
void checked_run(State& s, const char* label) {
    try { run(s); }
    catch (const std::runtime_error& error) {
        throw std::runtime_error(std::string(label) + " at " + hex_address(s.pc) + ": " + error.what() +
            "; LR=" + hex_address(s.r<4>(4128)) + "; r3=" + hex_address(s.r<4>(12)) +
            "; r4=" + hex_address(s.r<4>(16)));
    }
}
void run_wrapper(State& s, const BackgroundCase& type, std::uint32_t object, bool alternate) {
    s.pc = type.wrapper;
    s.wr<4>(12, object);
    s.wr<4>(16, alternate);
    checked_run(s, object ? "Object-specific wrapper" : "Default wrapper");
    preserved(s);
}
}
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: cm-background-checks <original-segments>\n"; return 2; }
    try {
        for (const auto& type : cases) {
            for (bool alternate : {false, true}) {
                auto reference = setup(argv[1], type);
                reference->pc = 0x107139c0;
                reference->wr<4>(12, type.category_id);
                reference->wr<4>(16, 0);
                reference->wr<4>(20, 0xffff0000u); // Unmapped; null-object path must ignore this descriptor.
                reference->wr<4>(24, alternate);
                checked_run(*reference, "Original default selector reference");
                const auto expected = alternate ? alternate_image : primary_image;
                require(reference->r<4>(12) == expected, "Original default selector did not choose the fixture picture");
                preserved(*reference);
                auto s = setup(argv[1], type);
                run_wrapper(*s, type, 0, alternate);
                require(s->r<4>(12) == expected, "Null-object wrapper did not delegate to original default picture selection");
                require(s->memory.read<4>(random_cursor) == reference->memory.read<4>(random_cursor) &&
                        s->memory.read<4>(random_seed) == reference->memory.read<4>(random_seed),
                        "Null-object fix changed original random-number consumption");
                s = setup(argv[1], type);
                s->pc = type.wrapper;
                s->wr<4>(12, club);
                s->wr<4>(16, alternate);
                // Stop at the getter's return boundary to check its actual result.
                // No comparison callback is needed for the separate empty-tree
                // case exercised after that boundary.
                for (unsigned steps = 0; s->pc != type.getter_return && steps < 1000; ++steps) {
                    s->budget = s->executed + 1;
                    try { run(*s); }
                    catch (const std::runtime_error& error) {
                        require(std::string(error.what()) == "Native instruction budget exhausted",
                                "Nonnull descriptor path failed before its return boundary");
                    }
                }
                require(s->pc == type.getter_return && s->r<4>(12) == descriptor,
                        "Nonnull object descriptor lookup was skipped or changed");
                s->memory.write<4>(category, 0);
                s->budget = 10000;
                checked_run(*s, "Empty object-specific tree");
                preserved(*s);
                require(!s->r<4>(12), "Empty object-specific tree returned a picture");
            }
            {
                // The guarded null path (loaded language object, mode 2) must leave the stack, registers and
                // picture of the original getter actually running: mode 0 with no language object returns
                // object+offset at once, without reading through the null object or creating anything.
                constexpr std::uint32_t window = 0x700ffe00, window_size = 0x120;
                std::vector<std::uint8_t> fill(window_size);
                for (std::uint32_t i = 0; i < window_size; ++i) fill[i] = std::uint8_t(0xa5 ^ i);
                auto guarded = setup(argv[1], type);
                auto original = setup(argv[1], type);
                original->memory.write<1>(data_mode, 0);
                original->memory.write<4>(0x108b80ac, 0);
                for (auto* state : {guarded.get(), original.get()}) {
                    state->memory.copy_in(window, fill);
                    run_wrapper(*state, type, 0, false);
                }
                require(guarded->r<4>(12) == original->r<4>(12) && guarded->r<4>(12) == primary_image,
                        "Guarded null wrapper chose a different picture from the original getter's path");
                if (guarded->memory.copy_out(window, window_size) != original->memory.copy_out(window, window_size)) {
                    const auto a = guarded->memory.copy_out(window, window_size), b = original->memory.copy_out(window, window_size);
                    std::uint32_t at = 0;
                    while (a[at] == b[at]) ++at;
                    throw std::runtime_error("Guarded null wrapper " + hex_address(type.wrapper) +
                        " left different stack bytes from the original getter, first at " + hex_address(window + at));
                }
            }
            auto s = setup(argv[1], type);
            s->memory.write<1>(0x108b84bc, 0);
            run_wrapper(*s, type, 0, false);
            require(!s->r<4>(12), "Disabled backgrounds returned a picture");
            s = setup(argv[1], type);
            s->memory.write<4>(category + 12, 0);
            run_wrapper(*s, type, 0, false);
            require(!s->r<4>(12), "Empty default list returned a picture");
            s = setup(argv[1], type);
            s->pc = type.getter;
            s->wr<4>(12, 0);
            bool stopped = false;
            std::string fault;
            try { run(*s); }
            catch (const std::runtime_error& error) {
                fault = error.what();
                stopped = std::string(error.what()) == "Unmapped data access at " + hex_address(type.null_address);
            }
            if (!stopped || s->pc != type.getter)
                throw std::runtime_error("Direct null getter " + hex_address(type.getter) + " stopped at " +
                    hex_address(s->pc) + ": " + fault);
        }
        std::cout << "Original nation/club/competition/award/screen default background selection, RNG state, and null-access checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
