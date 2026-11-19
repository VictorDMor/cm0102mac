// CUP.CPP, block `daily` (cup_base_daily*.cpp): the routines, all in production (CUP_BASE_ROUTINE). The comment after a virtual is its
// slot in the vtable 0x108d7724. 0x10482ca0 begins with the retirement test the generator's hook also makes.
CUP_BASE_ROUTINE(0x10482ca0, 0x10483194, R3, "9067961406f2d5e5", [](Cpu& c) { c.set(3, cup_base::daily::daily_update(c, c.r(3), c.r(4), c.r(5))); })  // +0x1c
CUP_BASE_ROUTINE(0x10485410, 0x104854e4, R3, "bc4386ea84044e6f", [](Cpu& c) { c.set(3, cup_base::daily::finished(c, c.r(3), c.r(4))); })  // +0x18
CUP_BASE_ROUTINE(0x10482640, 0x10482c98, R3, "9c585a2b903ab242", [](Cpu& c) { c.set(3, cup_base::daily::tie_outcome(c, c.r(3), c.r(4), c.r(5))); })  // +0xa4
CUP_BASE_ROUTINE(0x1047e540, 0x1047e978, R3, "ec2c1a3f4e54ee6a", [](Cpu& c) { c.set(3, cup_base::daily::fixture_winner(c, c.r(3), c.r(4))); })  // +0xa0
CUP_BASE_ROUTINE(0x1047bbb0, 0x1047bdd8, R3, "a2c3e8df7f1b74c3", [](Cpu& c) { c.set(3, cup_base::daily::screen_fixture(c, c.r(3), c.r(4), c.r(5))); })
CUP_BASE_ROUTINE(0x104864d0, 0x104866f0, NONE, "08c5a44d5c24c1b8", [](Cpu& c) { cup_base::daily::legs_into_fixtures(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
CUP_BASE_ROUTINE(0x104854f0, 0x10485570, NONE, "72485983f57e7423", [](Cpu& c) { cup_base::daily::set_draw_date(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9)); })
CUP_BASE_ROUTINE(0x10485570, 0x104855fc, NONE, "847d2317fe4323c3", [](Cpu& c) { cup_base::daily::set_fixture_date(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10), c.u32(c.sp() + 0x38)); })
CUP_BASE_ROUTINE(0x10485600, 0x10485830, NONE, "ccf3e76ae773ef60", [](Cpu& c) { cup_base::daily::set_round_slot(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9)); })
