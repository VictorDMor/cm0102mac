#include "match_debugger_bootstrap.hpp"
#include "engine_initialization_adapter.hpp"
#include "platform.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace cm::aot::match_debugger {
namespace {
constexpr std::uint32_t club_stride = 0x248, staff_stride = 0x70;

bool contains(const std::filesystem::path& parent, const std::filesystem::path& child) {
    const auto mismatch = std::mismatch(parent.begin(), parent.end(), child.begin(), child.end());
    return mismatch.first == parent.end();
}

std::uint32_t global_value(const State& state, std::int32_t offset) {
    const auto slot = std::uint32_t(state.memory.read<4>(std::uint32_t(state.r<4>(8) + offset)));
    if (!slot) throw std::runtime_error("Original database global slot is absent");
    return std::uint32_t(state.memory.read<4>(slot));
}

std::uint32_t call(State& state, std::uint32_t entry,
    std::initializer_list<std::uint32_t> arguments) {
    unsigned reg = 3;
    for (const auto argument : arguments) state.wr<4>(reg++ * 4, argument);
    state.wr<4>(4128, return_sentinel);
    state.pc = entry;
    try {
        run_until(state, return_sentinel);
    } catch (const std::exception& error) {
        throw std::runtime_error("Independent database bootstrap failed at " + hex_address(state.pc) + ": " + error.what());
    }
    return std::uint32_t(state.r<4>(12));
}
}

DatabaseWorld DatabaseWorld::load(const std::filesystem::path& original_program,
    const std::filesystem::path& original_game,
    const std::filesystem::path& new_private_directory,
    std::uint64_t instruction_limit, std::optional<std::uint32_t> random_seed) {
    if (!instruction_limit) throw std::invalid_argument("Database bootstrap requires a positive instruction limit");
    if (random_seed == 0) throw std::invalid_argument("Debugger random seed must be explicit and nonzero");
    const auto program = std::filesystem::canonical(original_program);
    const auto game = std::filesystem::canonical(original_game);
    const auto scratch = std::filesystem::weakly_canonical(new_private_directory);
    if (contains(game, scratch) || contains(scratch, game) ||
        contains(program, scratch) || contains(scratch, program))
        throw std::invalid_argument("Debugger scratch must be independent of original program and game inputs");
    if (std::filesystem::exists(std::filesystem::symlink_status(scratch)))
        throw std::invalid_argument("Debugger bootstrap requires a new private directory");

    DatabaseWorld world;
    // Reject malformed selection data before creating any writable backing.
    world.catalog_ = DatabaseCatalog::load(game);
    if (!std::filesystem::create_directories(scratch))
        throw std::runtime_error("Could not create independent debugger backing");
    world.state_ = std::make_unique<State>();
    auto& state = *world.state_;
    load_program(state, program);
    state.platform = std::make_shared<NativePlatform>(game, scratch, AudioMode::offline, EventMode::isolated);
    state.budget = instruction_limit;
    state.pc = 0x10773710;
    // Stop after actual global initialization, before the application event
    // loop. Keep its reserved startup frame while calling original helpers.
    run_until(state, 0x10773754);
    const auto& initialization = engine_initialization_diagnostics(state);
    for (unsigned entry = 0; entry < 20; ++entry)
        if (initialization.calls[entry] != 1 || initialization.returns[entry] != 1)
            throw std::runtime_error("Independent bootstrap did not compose all original global initializers");
    if (call(state, 0x10118ef0, {0}) != 1)
        throw std::runtime_error("Original application objects did not initialize");
    if (random_seed) (void)call(state, 0x107474a0, {*random_seed});

    constexpr char data_path[] = "CM 01-02:Data";
    const auto path = allocate(state, sizeof(data_path), false);
    if (!path) throw std::runtime_error("Could not allocate original database path");
    for (std::size_t offset = 0; offset < sizeof(data_path); ++offset)
        state.memory.write<1>(path + offset, std::uint8_t(data_path[offset]));
    // 0x1009ef90 selects ordinary file input. Argument 1 selects fresh table
    // allocation and the original ID-to-pointer resolver (0x100af8a0).
    // 0x1009ef30 instead selects compressed input; it is not a write wrapper.
    if (call(state, 0x1009ef90, {path, 1}) != 1)
        throw std::runtime_error("Original database loader did not complete");
    deallocate(state, path);
    world.clubs_ = global_value(state, -0x7be8);
    world.staff_ = global_value(state, -0x7be0);
    world.staff_count_ = global_value(state, -0x7be4);
    world.players_ = global_value(state, -0x7b38);
    world.player_count_ = global_value(state, -0x74d4);
    if (!world.clubs_ || !world.staff_ || !world.staff_count_ || !world.players_ || !world.player_count_ ||
        global_value(state, -0x7b50) != world.catalog_.clubs().size())
        throw std::runtime_error("Original live database and club catalog do not agree");
    for (const auto& club : world.catalog_.clubs()) (void)world.club_pointer(club.id);
    return world;
}

std::uint32_t DatabaseWorld::club_pointer(std::uint32_t id) const {
    (void)catalog_.club(id);
    if (!state_) throw std::logic_error("Debugger database state has been transferred");
    const auto pointer = std::uint64_t(clubs_) + std::uint64_t(id) * club_stride;
    if (pointer > std::numeric_limits<std::uint32_t>::max() || state_->memory.read<4>(pointer) != id)
        throw std::runtime_error("Original live club record identity is invalid");
    return std::uint32_t(pointer);
}

std::vector<RosterMember> DatabaseWorld::roster(std::uint32_t club_id) const {
    const auto club = club_pointer(club_id);
    std::vector<RosterMember> result;
    // The original resolver processes 50 staff references at club + 0x98.
    // Staff + 0x14 is the resolved player record; non-playing staff are excluded.
    for (unsigned slot = 0; slot < 50; ++slot) {
        const auto staff = std::uint32_t(state_->memory.read<4>(club + 0x98 + slot * 4));
        if (!staff) continue;
        if (staff < staff_ || (staff - staff_) % staff_stride ||
            (staff - staff_) / staff_stride >= staff_count_)
            throw std::runtime_error("Original club roster contains an unresolved staff reference");
        const auto id = std::uint32_t(state_->memory.read<4>(staff));
        if (id != (staff - staff_) / staff_stride)
            throw std::runtime_error("Original roster staff identity is inconsistent");
        const auto player = std::uint32_t(state_->memory.read<4>(staff + 0x14));
        if (player) {
            if (player < players_ || (player - players_) % 0x48 || (player - players_) / 0x48 >= player_count_)
                throw std::runtime_error("Original roster contains an unresolved player record");
            result.push_back({id, staff, player});
        }
    }
    return result;
}

std::unique_ptr<State> DatabaseWorld::release_state() && {
    if (!state_) throw std::logic_error("Debugger database state has already been transferred");
    return std::move(state_);
}
}
