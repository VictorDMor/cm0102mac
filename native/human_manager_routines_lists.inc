// HUMAN_MANAGER.CPP lists: each human's sorted list of 0x20-byte entries (human_manager_lists.hpp).
HUMAN_MANAGER_ROUTINE(0x1014b690, 0x1014b808, NONE, "349478d7a1dafa0d", [](Cpu& c) { human_manager::remove_key_everywhere(c, c.r(3), c.r(4)); })
HUMAN_MANAGER_ROUTINE(0x1014b810, 0x1014bb4c, NONE, "a2cb1a05b83a8d74", [](Cpu& c) { human_manager::add_entry(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
HUMAN_MANAGER_ROUTINE(0x1014bb50, 0x1014bdbc, NONE, "ebf3cba12cbf3ee1", [](Cpu& c) { human_manager::remove_entry(c, c.r(3), c.r(4), c.r(5)); })
HUMAN_MANAGER_ROUTINE(0x1014bdc0, 0x1014bf74, R3, "78b4699a66cb451d", [](Cpu& c) { c.set(3, human_manager::entry_text(c, c.r(3), c.r(4), c.r(5))); })
HUMAN_MANAGER_ROUTINE(0x1014bf80, 0x1014c0d8, R3, "59590d75c5a4a52a", [](Cpu& c) { c.set(3, std::uint32_t(std::int32_t(human_manager::has_entry(c, c.r(3), c.r(4), c.r(5))))); })
HUMAN_MANAGER_ROUTINE(0x1014c0e0, 0x1014c2dc, NONE, "9169708bf07a6bbd", [](Cpu& c) { human_manager::set_entry_text(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
HUMAN_MANAGER_ROUTINE(0x1014d520, 0x1014d560, R3, "ecaa227f571ed0c4", [](Cpu& c) { c.set(3, human_manager::compare_entries(c, c.r(3), c.r(4))); })
