MANAGER_MANAGER_ROUTINE(0x1019e8f0, 0x1019e98c, NONE, "ccf0343d988d9554", [](Cpu& c) { manager_manager::after_match(c, c.r(3), c.r(4), c.r(5)); })
MANAGER_MANAGER_ROUTINE(0x1019e990, 0x101a05c0, NONE, "98f124d3054d3e02", [](Cpu& c) { manager_manager::update(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
MANAGER_MANAGER_ROUTINE(0x101c15f0, 0x101c1688, NONE, "60bc70bc2f4bc618", [](Cpu& c) { manager_manager::drop_pending(c, c.r(3), c.r(4)); })
