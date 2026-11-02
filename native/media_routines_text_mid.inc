MEDIA_ROUTINE(0x106a1860, 0x106a1ef8, R3, "7d1af8dd67a21e46",
    [](Cpu& c) { media::manager_outburst_text(c, c.r(4), c.r(5), c.r(6)); })
MEDIA_ROUTINE(0x1069d460, 0x1069e18c, R3, "6e99a08d75c4430e",
    [](Cpu& c) { media::staff_match_text(c, c.r(4), c.r(5), c.r(6)); })
MEDIA_ROUTINE(0x106a2330, 0x106a2e0c, R3, "ac2b823539cfe110",
    [](Cpu& c) { media::sent_off_text(c, c.r(4), c.r(5), c.r(6)); })
MEDIA_ROUTINE(0x106a44d0, 0x106a52c8, R3, "b05d4aa4f7f21588",
    [](Cpu& c) { media::outlet_report_text(c, c.r(4), c.r(5), c.r(6)); })
MEDIA_ROUTINE(0x106a6230, 0x106a6e5c, R3, "74b94d580a5cfaf4",
    [](Cpu& c) { media::signing_reaction_text(c, c.r(4), c.r(5), c.r(6)); })
MEDIA_ROUTINE(0x106a7120, 0x106a7d68, R3, "e4ab1285089412bc",
    [](Cpu& c) { media::competition_win_text(c, c.r(4), c.r(5), c.r(6)); })
MEDIA_ROUTINE(0x106a8110, 0x106a8a50, R3, "125934eb84886f20",
    [](Cpu& c) { media::player_sale_text(c, c.r(4), c.r(5), c.r(6)); })
MEDIA_ROUTINE(0x1069ace0, 0x1069d194, R3, "4706bafddb58e37a",
    [](Cpu& c) { media::transfer_rumour_text(c, c.r(4), c.r(5), c.r(6)); })
