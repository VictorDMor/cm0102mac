// TRANSFER_MANAGER.CPP native-first block "queries" (see transfer_manager_native_routines.inc).
TRANSFER_MANAGER_NATIVE(0x103f4c00, 0x103f4d10, R3, "c587ac652045ad41", [](Cpu& c) {
    c.set(3, transfer_manager_native::person_has_open_offer(c, c.r(3), c.r(4)) ? 1 : 0);
})
TRANSFER_MANAGER_NATIVE(0x103f4d10, 0x103f4fc8, R3, "f411b0c88fd06c33", [](Cpu& c) {
    c.set(3, transfer_manager_native::offer_fee_refused(c, c.r(4), c.r(5), c.r(6)) ? 1 : 0);
})
TRANSFER_MANAGER_NATIVE(0x103f5a50, 0x103f5bf8, R3, "8b684a8b6274bb3f", [](Cpu& c) {
    c.set(3, transfer_manager_native::staff_offer_refused(c, c.r(4), c.r(5), c.r(7), c.r(8), c.r(9)) ? 1 : 0);
})
TRANSFER_MANAGER_NATIVE(0x103f5c00, 0x103f5d1c, R3, "4b32db80ac543985", [](Cpu& c) {
    c.set(3, transfer_manager_native::person_has_offer_from_club(c, c.r(3), c.r(4), c.r(5)) ? 1 : 0);
})
TRANSFER_MANAGER_NATIVE(0x103f5d20, 0x103f5e50, R3, "8ff96068c9bea848", [](Cpu& c) {
    c.set(3, transfer_manager_native::person_offer_from_club(c, c.r(3), c.r(4), c.r(5), c.r(6)));
})
TRANSFER_MANAGER_NATIVE(0x103f80f0, 0x103f8378, NONE, "557d73846a59ad21", [](Cpu& c) {
    transfer_manager_native::touch_status_row(c, c.r(3), c.r(4), c.r(5));
})
TRANSFER_MANAGER_NATIVE(0x103f83c0, 0x103f8810, R3, "1658885b1b0d42c4", [](Cpu& c) {
    c.set(3, transfer_manager_native::offer_allowed(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)) ? 1 : 0);
})
TRANSFER_MANAGER_NATIVE(0x103f8cc0, 0x103f9200, R3, "35f1873ddb6987bb", [](Cpu& c) {
    c.set(3, std::uint32_t(transfer_manager_native::earliest_offer_date(c, c.r(3), c.r(4), c.r(5))));
})
TRANSFER_MANAGER_NATIVE(0x103fa290, 0x103fa840, R3, "eea73a903414afaf", [](Cpu& c) {
    c.set(3, transfer_manager_native::transfer_possible(c, c.r(3), c.r(4), c.r(5), c.r(6)) ? 1 : 0);
})
TRANSFER_MANAGER_NATIVE(0x103fa8e0, 0x103faba0, NONE, "fb58156edf74cf63", [](Cpu& c) {
    transfer_manager_native::narrow_window_to_contract(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10));
})
TRANSFER_MANAGER_NATIVE(0x103fb130, 0x103fb384, NONE, "1a192dbed5fc2e3b", [](Cpu& c) {
    transfer_manager_native::season_window(c, c.r(4), c.r(5));
})
