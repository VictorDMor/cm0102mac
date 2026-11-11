// MANAGER_MANAGER.CPP standing block (manager_manager_routines.inc).
MANAGER_MANAGER_ROUTINE(0x101bd320, 0x101bd45c, NONE, "ca883829d269a8f8", [](Cpu& c) { manager_manager::post_club_message(c, c.r(3), c.r(4), std::int8_t(c.r(5)), std::int8_t(c.r(6))); c.finish(); })
MANAGER_MANAGER_ROUTINE(0x101bb9f0, 0x101bd318, NONE, "232078f2f2b123d0", [](Cpu& c) { manager_manager::club_request(c, c.r(3), c.r(4), c.r(5)); c.finish(); })
MANAGER_MANAGER_ROUTINE(0x101bd460, 0x101bde64, NONE, "8f3e314fbd73402b", [](Cpu& c) { manager_manager::board_reply_text(c, c.r(3), c.r(4), std::int8_t(c.r(5)), c.r(6)); c.finish(); })
