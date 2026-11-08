MANAGER_MANAGER_ROUTINE(0x10195e10, 0x10196388, R3, "7419c39d48831a5f", [](Cpu& c) { c.set(3, std::uint32_t(manager_manager::draw(c, std::int32_t(c.r(3))))); })
MANAGER_MANAGER_ROUTINE(0x101a8ad0, 0x101a9df4, NONE, "5d365332df70de2e", [](Cpu& c) { manager_manager::review(c, c.r(3), c.r(4), c.r(5)); })
MANAGER_MANAGER_ROUTINE(0x101c1690, 0x101c1824, R3, "fc73fab54ec0d9b4", [](Cpu& c) { manager_manager::departure(c, c.r(3), c.r(4)); })
