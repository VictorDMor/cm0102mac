// Audited INTER_AMER_CUP.CPP entry contracts; original listings stay private.
INTER_AMER_CUP_ROUTINE(0x106ebdd0, 0x106ec0b0, R3, "c45bf7d47b9b08b2", [](Cpu& c) { c.set(3, inter_amer_cup::construct(c, c.r(3), c.r(4), c.r(5))); })
INTER_AMER_CUP_ROUTINE(0x106ec0b0, 0x106ec1a0, R3, "d0d6d47a02cb5d63", [](Cpu& c) { c.set(3, inter_amer_cup::destroy(c, c.r(3), c.r(4))); })
INTER_AMER_CUP_ROUTINE(0x106ec1a0, 0x106ec330, R3, "d7ef4be6db643c46", [](Cpu& c) { c.set(3, inter_amer_cup::calendar(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7))); })
INTER_AMER_CUP_ROUTINE(0x106ec330, 0x106ec7b0, R3, "492b2c9938f30471", [](Cpu& c) { c.set(3, inter_amer_cup::select_clubs(c, c.r(3))); })
INTER_AMER_CUP_ROUTINE(0x106ec7b0, 0x106eca00, NONE, "112871c8479d5a88", [](Cpu& c) { inter_amer_cup::next_season(c, c.r(3)); })
INTER_AMER_CUP_ROUTINE(0x106eca00, 0x106eca50, R3, "7336c06ab425e814", [](Cpu& c) { c.set(3, inter_amer_cup::configure(c, c.r(3))); })
