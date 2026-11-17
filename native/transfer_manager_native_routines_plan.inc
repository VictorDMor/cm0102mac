// TRANSFER_MANAGER.CPP native-first block "plan" (see transfer_manager_native_routines.inc).
TRANSFER_MANAGER_NATIVE(0x103efaa0, 0x103efec0, NONE, "607ae630d504a784", [](Cpu& c) {
    transfer_manager_native::conclude_agreed_offer(c, c.r(3), c.r(4));
})
TRANSFER_MANAGER_NATIVE(0x103efec0, 0x103f1b30, NONE, "64951ae75b3c1566", [](Cpu& c) {
    transfer_manager_native::move_player_to_buyer(c, c.r(3), c.r(4), c.r(5), c.r(6));
})
TRANSFER_MANAGER_NATIVE(0x103edef0, 0x103ee194, NONE, "fa98ca504360f0f1", [](Cpu& c) {
    transfer_manager_native::update_person_row(c, c.r(3), c.r(4), c.r(5));
})
TRANSFER_MANAGER_NATIVE(0x103f2c00, 0x103f2d4c, NONE, "65e5611cfc4a4c73", [](Cpu& c) {
    transfer_manager_native::pass_on_person_of_kind(c, c.r(3), c.r(4), c.r(5));
})
