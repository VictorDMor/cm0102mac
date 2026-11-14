// The rb_*.cpp rulebooks (tools/ruling_bodies_audit.py): entry, end, results, digest of the original body, hook.
// rb_ireland.cpp (its constructor 0x106ab8f0 and report 0x106ab970 are rb_greece.cpp's in this tree)
RULING_BODIES_ROUTINE(0x106abe20, 0x106abed4, R3, "a90f4757ce5ea339", [](Cpu& c) { c.set(3, ruling_bodies::nation_text(c, ruling_bodies::ireland::text, c.r(4), c.r(5), c.r(6))); })
RULING_BODIES_ROUTINE(0x106abee0, 0x106abf58, R3, "83aee9d44c3f7916", [](Cpu& c) { c.set(3, ruling_bodies::ireland::next_card_bans(c, c.r(4), c.r(5), c.r(6))); })
// rb_northern_ireland.cpp
RULING_BODIES_ROUTINE(0x106abf60, 0x106abfe0, R3, "f0c7619aed365abc", [](Cpu& c) { c.set(3, ruling_bodies::construct(c, ruling_bodies::northern_ireland::rulebook, c.r(3), c.r(4))); })
RULING_BODIES_ROUTINE(0x106abfe0, 0x106ac21c, NONE, "c948794206216ea9", [](Cpu& c) { ruling_bodies::northern_ireland::report_cards(c, c.r(3), c.r(4), c.r(5)); })
RULING_BODIES_ROUTINE(0x106ac220, 0x106ac2d4, R3, "3e5c06f62c735101", [](Cpu& c) { c.set(3, ruling_bodies::nation_text(c, ruling_bodies::northern_ireland::text, c.r(4), c.r(5), c.r(6))); })
RULING_BODIES_ROUTINE(0x106ac2e0, 0x106ac39c, R3, "70d1c73e4a2c01bd", [](Cpu& c) { c.set(3, ruling_bodies::northern_ireland::next_card_bans(c, c.r(4), c.r(5), c.r(6))); })
RULING_BODIES_ROUTINE(0x106ac3a0, 0x106ac3cc, NONE, "ebc95b45bd8814b0", [](Cpu& c) { ruling_bodies::northern_ireland::clear_points(c, c.r(3)); })
