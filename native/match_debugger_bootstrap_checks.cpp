#include "match_debugger_bootstrap.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
template<class Operation> void rejects(Operation&& operation) {
    try { operation(); } catch (const std::exception&) { return; }
    throw std::runtime_error("Invalid debugger bootstrap operation was accepted");
}
}

int main(int argc, char** argv) {
    try {
        if (argc != 4) throw std::runtime_error("Expected original program, read-only game and private check root");
        const auto root = std::filesystem::path(argv[3]) /
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        using cm::aot::match_debugger::DatabaseWorld;
        rejects([&] { (void)DatabaseWorld::load(argv[1], argv[2], root / "zero", 0); });
        rejects([&] { (void)DatabaseWorld::load(argv[1], argv[2], std::filesystem::path(argv[2]) / "Debugger"); });
        rejects([&] { (void)DatabaseWorld::load(argv[1], argv[2], std::filesystem::path(argv[1]) / "Debugger"); });
        require(!std::filesystem::exists(root), "Rejected setup created private files");

        auto first = DatabaseWorld::load(argv[1], argv[2], root / "first");
        auto second = DatabaseWorld::load(argv[1], argv[2], root / "second");
        require(first.catalog().clubs().size() == 11019, "Original club world count");
        require(first.staff_count() == second.staff_count(), "Independent staff counts differ");
        require(&first.state() != &second.state(), "Independent worlds share guest state");
        std::size_t members = 0, populated_clubs = 0;
        for (const auto& club : first.catalog().clubs()) {
            require(first.state().memory.read<4>(first.club_pointer(club.id)) == club.id, "Live club ID");
            const auto roster = first.roster(club.id);
            populated_clubs += !roster.empty();
            members += roster.size();
        }
        for (auto club_id : {677u, 5677u}) {
            const auto a = first.roster(club_id), b = second.roster(club_id);
            require(a.size() >= 11 && a.size() == b.size(), "Selected real club has no complete roster");
            for (std::size_t n = 0; n < a.size(); ++n) {
                require(a[n].staff_id == b[n].staff_id, "Independent roster identities differ");
                require(a[n].player != a[n].staff, "Staff reference presented as player record");
                require(first.state().memory.copy_out(a[n].player, 0x48) ==
                    second.state().memory.copy_out(b[n].player, 0x48), "Original player attributes differ between fresh worlds");
            }
        }
        rejects([&] { (void)first.club_pointer(std::numeric_limits<std::uint32_t>::max()); });
        rejects([&] { (void)DatabaseWorld::load(argv[1], argv[2], root / "first"); });

        auto& memory = first.state().memory;
        const auto club = first.club_pointer(677), other = second.club_pointer(677);
        const auto finances = memory.read<4>(club + 4);
        memory.write<4>(club + 4, finances ^ 0xffffffffu);
        require(second.state().memory.read<4>(other + 4) == finances, "A write escaped its owned guest world");
        memory.write<4>(club + 4, finances);
        memory.write<4>(club, 5677);
        rejects([&] { (void)first.club_pointer(677); });
        memory.write<4>(club, 677);
        const auto staff = memory.read<4>(club + 0x98);
        memory.write<4>(club + 0x98, staff + 1);
        rejects([&] { (void)first.roster(677); });
        memory.write<4>(club + 0x98, staff);
        const auto player = memory.read<4>(staff + 0x14);
        memory.write<4>(staff + 0x14, memory.read<4>(staff + 0x0c));
        rejects([&] { (void)first.roster(677); });
        memory.write<4>(staff + 0x14, player);
        require(first.roster(677).size() >= 11, "Restored original roster is incomplete");

        auto transferred = std::move(first).release_state();
        require(bool(transferred), "Owned database state transfer failed");
        rejects([&] { (void)first.club_pointer(677); });
        rejects([&] { (void)std::move(first).release_state(); });
        std::cout << "PASS: 2 fresh independent worlds, 11019 live club identities, " << populated_clubs
                  << " populated rosters, " << members << " resolved player references, 10 invalid operations rejected\n";
        std::cout << "No career or captured guest state loaded; original dependencies execute through production dispatch\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
