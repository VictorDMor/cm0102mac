MEDIA_ROUTINE(0x1069ed10, 0x1069f268, R3, "f561663d7f4a8b6b",
    []([[maybe_unused]] Cpu& c) { media::post_squad_story(c, c.r(4), c.r(5)); })
MEDIA_ROUTINE(0x106891b0, 0x1068a468, R3, "768efcb1811bad6e",
    []([[maybe_unused]] Cpu& c) { media::post_item_bdf(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9)); })
MEDIA_ROUTINE(0x1067e210, 0x1067fed0, R3, "3e879c498d6afc26",
    []([[maybe_unused]] Cpu& c) { media::post_item_bdd(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
MEDIA_ROUTINE(0x10698f50, 0x10699d10, R3, "17e770f405624e64",
    []([[maybe_unused]] Cpu& c) { media::process_press_events(c, c.r(3)); })
MEDIA_ROUTINE(0x10699d10, 0x1069acdc, R3, "cb180f2f6b670f96",
    []([[maybe_unused]] Cpu& c) { media::weigh_story_targets(c, c.r(4), c.r(5), c.r(6), c.r(7), c.r(8)); })
