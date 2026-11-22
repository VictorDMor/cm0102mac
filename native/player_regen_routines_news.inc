// PLAYER_REGEN.CPP blocks `news_posting` (player_regen_news.cpp) and `news_texts` (player_regen_news_text.cpp): the three messages the module posts
// (0xbce youth promotion, 0xbba and 0xbbb retirements) and the callbacks that build their texts. None returns a result; the posting
// routines' arguments are the person (r4), the reason (r5) and a flag (r6) or the club (r5); the texts' are the message item (r4), the headline
// or body kind (r5) and the output item (r6).
PLAYER_REGEN_ROUTINE(0x102df780, 0x102dfbb4, NONE, "358fc24a0e51ffd5", [](Cpu& c) { player_regen::announce_youth_promotion(c, c.r(4), c.r(5)); })
PLAYER_REGEN_ROUTINE(0x102e0090, 0x102e0508, NONE, "7cc1544c2c27d0c0", [](Cpu& c) { player_regen::announce_retirement(c, c.r(4), c.r(5), c.r(6)); })
PLAYER_REGEN_ROUTINE(0x102e13c0, 0x102e15a8, NONE, "e6e6492fa64eddf0", [](Cpu& c) { player_regen::announce_staff_retirement(c, c.r(4), c.r(5)); })
PLAYER_REGEN_ROUTINE(0x102dfbc0, 0x102e0084, NONE, "dbb33758b52e7ce9", [](Cpu& c) { player_regen::youth_promotion_text(c, c.r(4), c.r(5), c.r(6)); })
PLAYER_REGEN_ROUTINE(0x102e0510, 0x102e13c0, NONE, "09a622103999baa2", [](Cpu& c) { player_regen::retirement_text(c, c.r(4), c.r(5), c.r(6)); })
PLAYER_REGEN_ROUTINE(0x102e15b0, 0x102e1f18, NONE, "0783374977edfab3", [](Cpu& c) { player_regen::staff_retirement_text(c, c.r(4), c.r(5), c.r(6)); })
