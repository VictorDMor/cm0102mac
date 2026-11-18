// COMP.CPP block `news` (comp_news.cpp, comp_texts*.cpp): the news items a competition files and the texts that
// render them. Items write news records; texts format into the shared text buffer and the news text object.
COMP_ROUTINE(0x10452010, 0x10452284, NONE, "06de1f9511b62974", [](Cpu& c) {
    comp::news_prize_money(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10));
})
COMP_ROUTINE(0x104527c0, 0x10452880, NONE, "c598e620a5de57b8", [](Cpu& c) { comp::news_competition_year(c, c.r(3), c.r(4), c.r(5)); })
COMP_ROUTINE(0x10452a60, 0x10452bac, NONE, "18b4f0e4e9540af5", [](Cpu& c) { comp::news_movement(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
// The fate text's last three arguments are the caller's: bytes at entry sp+0x3b and +0x3f, the output at +0x40.
COMP_ROUTINE(0x10450030, 0x10450578, NONE, "acff56618904b41b", [](Cpu& c) {
    comp::league_fate_text(c, c.r(3), c.r(4), c.r(5), c.r(8), c.r(9), c.u8(c.sp() + 0x3b), c.u8(c.sp() + 0x3f), c.u32(c.sp() + 0x40));
})
COMP_ROUTINE(0x10450580, 0x104506fc, NONE, "5193e361dafdb36e", [](Cpu& c) { comp::position_text(c, c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9)); })
COMP_ROUTINE(0x10451ac0, 0x10451c90, R3, "8091e43a03d7a015", [](Cpu& c) { c.set(3, comp::place_text(c, c.r(3), c.r(4), c.r(5), c.r(6))); })
COMP_ROUTINE(0x10452290, 0x104527c0, NONE, "872ee469e8dca215", [](Cpu& c) { comp::prize_news_text(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
COMP_ROUTINE(0x10452880, 0x10452a58, NONE, "917e94e4648ba0dc", [](Cpu& c) { comp::money_news_text(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
COMP_ROUTINE(0x10452bb0, 0x10452ec4, NONE, "3ecb513472a5ce90", [](Cpu& c) { comp::movement_news_text(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
