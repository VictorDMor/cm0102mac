FINANCE_ROUTINE(0x100fefa0, 0x100ffeb4, R3, "6ac2379d82d953d5", [](Cpu& c) { c.set(3, finance::benefactor_gift(c, c.r(3), c.r(4), c.r(5))); })
FINANCE_ROUTINE(0x100ffec0, 0x10100694, R3, "684e5e4f8c114426", [](Cpu& c) { c.set(3, finance::board_cover(c, c.r(3), c.r(4), c.r(5))); })
FINANCE_ROUTINE(0x101006a0, 0x101013f8, R3, "8de4771a6e45c3e0", [](Cpu& c) { c.set(3, finance::owner_bailout(c, c.r(3), c.r(4), c.r(5))); })
FINANCE_ROUTINE(0x10101400, 0x10101fe8, NONE, "d32091a86107e89c", [](Cpu& c) { finance::board_review(c, c.r(3), c.r(4), c.r(5)); })
