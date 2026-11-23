// The font engine, block `hstrat97` (font_engine_hstrat97.cpp): the hint strategy and its two leaves. All three leave r3-r10 as the original does.
FONT_ENGINE_ROUTINE(0x1075c980, 0x1075ca50, NONE, "1f244de39eb4785c", [](Cpu& c) { font_engine::sift_heap(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
FONT_ENGINE_ROUTINE(0x1075ca60, 0x1075cbbc, R3, "1b3d759a557c7aa5", [](Cpu& c) { c.set(3, font_engine::strategy_score(c, c.r(3), c.r(4), c.r(5))); })
FONT_ENGINE_ROUTINE(0x1075cbc0, 0x1075e0bc, R3, "fe0b9a9bc59d0759", [](Cpu& c) { font_engine::hint_strategy(c, c.r(3), c.r(4)); })
