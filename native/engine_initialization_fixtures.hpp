#pragma once
#include "platform.hpp"
#include "semantic_io_checks.hpp"
#include "engine_initialization_adapter.hpp"
#include <unistd.h>
#include <copyfile.h>
#include <cstdlib>

namespace cm::engine_initialization_checks {
// Both independent platforms see the same private resource-file copy. User
// state stays separate; no installed game, save or preference is written.
struct NativeEnvironment {
    std::filesystem::path path;
    NativeEnvironment(State& original, State& modern) {
        auto pattern = (std::filesystem::temp_directory_path() / "cm-initialization-XXXXXX").string();
        cm::semantic_io_checks::require(mkdtemp(pattern.data()), "Private native initialization environment");
        path = pattern; std::filesystem::create_directory(path / "Game");
        const char* source = std::getenv("CM_AOT_ORIGINAL_APPLICATION");
        cm::semantic_io_checks::require(source && *source, "Explicit original resource file for production initialization checks");
        const auto resource_file = path / "Game/CM 01-02";
        cm::semantic_io_checks::require(copyfile(source, resource_file.c_str(), nullptr, COPYFILE_ALL) == 0,
            "Private original resource file copy with resource fork and metadata");
        original.platform = std::make_shared<NativePlatform>(path / "Game", path / "OriginalUser", AudioMode::offline);
        modern.platform = std::make_shared<NativePlatform>(path / "Game", path / "ModernUser", AudioMode::offline);
    }
    ~NativeEnvironment() { std::error_code ignored; std::filesystem::remove_all(path, ignored); }
};
inline void prepare_initializer(State& original, State& modern, unsigned index) {
    if (index != 19) return;
    // The final array constructor queries the file controller through its CFM
    // vectors. The original startup root creates that controller at entry 16
    // before entry 19. Preserve that real prerequisite in isolated entry checks.
    for (State* state : {&original, &modern}) {
        auto& diagnostics = engine_initialization_diagnostics(*state);
        diagnostics.original = true;
        state->wr<4>(4128, return_sentinel);
        state->pc = engine_initialization::entries[16];
        run_until(*state, return_sentinel);
        cm::semantic_io_checks::require(state->pc == return_sentinel &&
            state->memory.read<4>(state->memory.read<4>(state->r<4>(8) - 0x3e70) + 4) != 0,
            "Original file-controller prerequisite published its CFM vector");
        diagnostics = EngineInitializationDiagnostics{};
    }
    cm::semantic_io_checks::Image(original).compare(modern, "Identical original initializer prerequisite");
    cm::semantic_io_checks::require(original.executed == modern.executed,
        "Identical original prerequisite accounting");
}
}
