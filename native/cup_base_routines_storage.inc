// CUP.CPP, block `storage` (cup_base_storage*.cpp): the cup's save reader and writer. The comment after a virtual is
// its slot in the vtable 0x108d7724.
CUP_BASE_ROUTINE(0x104831a0, 0x10484418, R3, "693fe4c6cbf69830", [](Cpu& c) { c.set(3, cup_base::load_from_disk(c, c.r(3), c.r(4))); })  // +0x20
CUP_BASE_ROUTINE(0x10484420, 0x10484b8c, NONE, "538ac11c688d7adc", [](Cpu& c) { cup_base::save_to_disk(c, c.r(3), c.r(4)); })  // +0x24
