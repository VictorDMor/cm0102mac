// TRANSFER_MANAGER.CPP native-first block "approaches" (see transfer_manager_native_routines.inc).
TRANSFER_MANAGER_NATIVE(0x103e25d0, 0x103e2da8, NONE, "b6714d4dc62f0140", [](Cpu& c) {
    transfer_manager_native::add_offer(c, c.r(3), c.r(4), c.r(5));
})
TRANSFER_MANAGER_NATIVE(0x103e3170, 0x103e323c, R3, "f418802e8256667f", [](Cpu& c) {
    c.set(3, transfer_manager_native::build_and_add_offer(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9)));
})
TRANSFER_MANAGER_NATIVE(0x103e3410, 0x103e3860, NONE, "54a361240de173be", [](Cpu& c) {
    transfer_manager_native::add_regen_offer(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), std::int32_t(c.r(8)), c.r(9));
})
TRANSFER_MANAGER_NATIVE(0x103f3bf0, 0x103f4238, NONE, "e03afb5d4a4d25f8", [](Cpu& c) {
    transfer_manager_native::choose_first_status(c, c.r(3), c.r(4), c.r(5), c.r(6));
})
TRANSFER_MANAGER_NATIVE(0x1040d5c0, 0x1040d820, NONE, "782246b60203356c", [](Cpu& c) {
    transfer_manager_native::post_offer_news(c, c.r(3), c.r(4), c.r(5));
})
TRANSFER_MANAGER_NATIVE(0x103e2db0, 0x103e316c, NONE, "e5c466d0f44c1042", [](Cpu& c) {
    transfer_manager_native::add_approach(c, c.r(3), c.r(4), c.r(5), std::int8_t(c.r(6)));
})
TRANSFER_MANAGER_NATIVE(0x103e3240, 0x103e340c, R3, "d113a6956c17e198", [](Cpu& c) {
    c.set(3, transfer_manager_native::add_approach_from_terms(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8)));
})
TRANSFER_MANAGER_NATIVE(0x103e3de0, 0x103e4130, NONE, "e64e684b7fb09320", [](Cpu& c) {
    transfer_manager_native::sign_person(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7));
})
