MEDIA_ROUTINE(0x1069f270, 0x106a15f0, R3, "1736eee7a5ea14d8",
    []([[maybe_unused]] Cpu& c) { media::pick_reactions(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
MEDIA_ROUTINE(0x106a1f00, 0x106a2330, R3, "0e7f7d3a412a48ce",
    []([[maybe_unused]] Cpu& c) { media::post_item_be6(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
MEDIA_ROUTINE(0x106a2e10, 0x106a4098, R3, "d35144e582263923",
    []([[maybe_unused]] Cpu& c) { media::settle_event(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
MEDIA_ROUTINE(0x106a40a0, 0x106a44c8, R3, "78939fa2224519ca",
    []([[maybe_unused]] Cpu& c) { media::post_item_be8(c, c.r(3), c.r(4), c.r(5)); })
MEDIA_ROUTINE(0x106a52d0, 0x106a5e88, R3, "ea279f992c1ce46a",
    []([[maybe_unused]] Cpu& c) { media::react_to_event(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
MEDIA_ROUTINE(0x106a5e90, 0x106a6228, R3, "bdd95398beb3b4df",
    []([[maybe_unused]] Cpu& c) { media::post_item_be5(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8)); })
MEDIA_ROUTINE(0x106a7d70, 0x106a810c, R3, "c9db3b9df6d6bbfe",
    []([[maybe_unused]] Cpu& c) { media::post_item_be4(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
