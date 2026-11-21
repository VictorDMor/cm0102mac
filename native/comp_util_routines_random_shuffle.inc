// COMP_UTIL.CPP block `random_shuffle` (comp_util_shuffle.cpp): the unit's pooled Random(n) and the shuffles
// built on it. The shuffles' callers read r7..r10 (results R3-free: they return nothing).
COMP_UTIL_ROUTINE(0x10462cd0, 0x10462ffc, NONE, "e8c94ec7bb82f811", [](Cpu& c) { comp_util::shuffle(c, c.r(3), c.r(4)); })
COMP_UTIL_ROUTINE(0x10463000, 0x10463578, R3, "5b936d325b255323", [](Cpu& c) { c.set(3, std::uint32_t(comp_util::pooled_draw(c, std::int32_t(c.r(3))))); })
COMP_UTIL_ROUTINE(0x104635b0, 0x10463988, NONE, "25e0c09b62a258e4", [](Cpu& c) { comp_util::shuffle_front(c, c.r(3), c.r(4), c.r(5)); })
