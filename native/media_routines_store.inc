MEDIA_ROUTINE(0x10646d90, 0x10646dfc, R3, "3ab6bc87cd975181",
    []([[maybe_unused]] Cpu& c) { media::construct_press(c, c.r(3), c.r(4)); })
MEDIA_ROUTINE(0x10646e00, 0x10646e84, R3, "74a1ef524bf856ac",
    []([[maybe_unused]] Cpu& c) { media::destroy_press(c, c.r(3), c.r(4)); })
MEDIA_ROUTINE(0x10646e90, 0x10647418, R3, "bffd1e704c5167cc",
    []([[maybe_unused]] Cpu& c) { media::load_press(c, c.r(3), c.r(4)); })
MEDIA_ROUTINE(0x10647420, 0x10647730, R3, "b5adf370552833ff",
    []([[maybe_unused]] Cpu& c) { media::save_press(c, c.r(3), c.r(4)); })
MEDIA_ROUTINE(0x1069e190, 0x1069e3d0, R3, "77108222254259e3",
    []([[maybe_unused]] Cpu& c) { media::add_history_row(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8)); })
MEDIA_ROUTINE(0x1069e3d0, 0x1069e47c, R3, "a3b29f6700a8a477",
    []([[maybe_unused]] Cpu& c) { media::mark_history_row(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8)); })
MEDIA_ROUTINE(0x1069e480, 0x1069e6f4, R3, "6446ec50a0582731",
    []([[maybe_unused]] Cpu& c) { media::prune_history(c, c.r(3)); })
MEDIA_ROUTINE(0x1069e700, 0x1069e890, R3, "656989d2e70d1cfd",
    []([[maybe_unused]] Cpu& c) { media::has_history_row(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8)); })
MEDIA_ROUTINE(0x1069e990, 0x1069eb04, R3, "1ccdc04c6240e7b3",
    []([[maybe_unused]] Cpu& c) { media::story_roll_for_club(c, c.r(3)); })
MEDIA_ROUTINE(0x1069eb10, 0x1069ec88, R3, "dafb7dfe8b37827d",
    []([[maybe_unused]] Cpu& c) { media::story_roll_for_player(c, c.r(3)); })
MEDIA_ROUTINE(0x106a6e60, 0x106a7120, R3, "18cccbfa1b9bf80b",
    []([[maybe_unused]] Cpu& c) { media::log_kind_2e(c, c.r(4), c.r(5), c.r(6)); })
MEDIA_ROUTINE(0x10698bb0, 0x10698e80, R3, "87d52796838f9cf2",
    []([[maybe_unused]] Cpu& c) { media::add_press_event(c, c.r(3), c.r(4), c.r(5), std::int32_t(c.r(6)), c.f(1)); })
MEDIA_ROUTINE(0x10698e80, 0x10698f44, R3, "d56e12eb39d4e525",
    []([[maybe_unused]] Cpu& c) { media::remove_press_events(c, c.r(3), c.r(4)); })
MEDIA_ROUTINE(0x10688e80, 0x10688fc4, R3, "2dd653e8b0c6fb9e",
    []([[maybe_unused]] Cpu& c) { media::log_kind_1790(c, c.r(4)); })
MEDIA_ROUTINE(0x106a15f0, 0x106a1858, R3, "ccb07b6475effe10",
    []([[maybe_unused]] Cpu& c) { media::log_kind_178e(c, c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9)); })
MEDIA_ROUTINE(0x1069d1a0, 0x1069d460, R3, "d99aceb5e727dff7",
    []([[maybe_unused]] Cpu& c) { media::log_kind_bcb(c, c.r(4), c.r(5), c.r(6)); })
