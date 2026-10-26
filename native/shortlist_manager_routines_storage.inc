// SHORTLIST_MANAGER.CPP: the shortlist manager object: construction, destruction, the category lists' reset, the random range draw, the category sort, "shortlist manager" load and save (shortlist_manager_storage.hpp).
SHORTLIST_MANAGER_ROUTINE(0x10356410, 0x103567b0, R3, "be5b5ea10663ae6d", [](Cpu& c) { shortlist_manager::construct_storage(c, c.r(3), c.r(4)); })
SHORTLIST_MANAGER_ROUTINE(0x103567b0, 0x103568e0, R3, "bda7019a5f42b7c4", [](Cpu& c) { shortlist_manager::destroy_storage(c, c.r(3), c.r(4)); })
SHORTLIST_MANAGER_ROUTINE(0x103568e0, 0x10356e40, NONE, "aba44c2892272a3e", [](Cpu& c) { shortlist_manager::reset_storage(c, c.r(3)); })
SHORTLIST_MANAGER_ROUTINE(0x10356e40, 0x103573c0, R3, "86e14839a27810a7", [](Cpu& c) { shortlist_manager::sample_range(c, std::int32_t(c.r(3))); })
SHORTLIST_MANAGER_ROUTINE(0x10357d30, 0x10358020, NONE, "e0bf2d9ac23ccec2", [](Cpu& c) { shortlist_manager::rate_categories(c, c.r(3)); })
SHORTLIST_MANAGER_ROUTINE(0x10358020, 0x10358af0, NONE, "5ff2ffa8e57e4fe0", [](Cpu& c) { shortlist_manager::load_storage(c, c.r(3), c.r(4)); })
SHORTLIST_MANAGER_ROUTINE(0x10358af0, 0x10358b30, R3, "7964a2aa20b82f82", [](Cpu& c) { shortlist_manager::initialize_record(c, c.r(3)); })
SHORTLIST_MANAGER_ROUTINE(0x10358b30, 0x10358f20, NONE, "47a6957d13cafcf2", [](Cpu& c) { shortlist_manager::save_storage(c, c.r(3), c.r(4)); })
