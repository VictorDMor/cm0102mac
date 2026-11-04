// Only verified handwritten routines are registered for production.
TRANSFER_MANAGER_ROUTINE(0x103ea250, [](Cpu& c) { transfer_manager::mark_pending_contract(c, c.r(3), c.r(4)); })
TRANSFER_MANAGER_ROUTINE(0x103ea270, [](Cpu& c) { transfer_manager::clear_pending_contract(c, c.r(3), c.r(4)); })
TRANSFER_MANAGER_ROUTINE(0x103ea290, [](Cpu& c) { transfer_manager::find_pending_contract(c, c.r(3), c.r(4), c.r(5)); })
TRANSFER_MANAGER_ROUTINE(0x103fac70, [](Cpu& c) { transfer_manager::transfer_price(c, c.r(4), c.r(5), c.r(6)); })
TRANSFER_MANAGER_ROUTINE(0x103faff0, [](Cpu& c) { transfer_manager::transfer_eligibility(c, c.r(4)); })
