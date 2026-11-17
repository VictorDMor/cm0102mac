// TRANSFER_MANAGER.CPP native-first block "terms" (see transfer_manager_native_routines.inc).
TRANSFER_MANAGER_NATIVE(0x103f89e0, 0x103f8cc0, R3, "45df6eb00f5a8f82", [](Cpu& c) {
    c.set(3, transfer_manager_native::term_date(c, c.r(3), c.r(4), c.r(5)));
})
TRANSFER_MANAGER_NATIVE(0x104058e0, 0x10405c98, NONE, "1178cc7560de6499", [](Cpu& c) {
    transfer_manager_native::post_term_news(c, c.r(3), c.r(4));
})
TRANSFER_MANAGER_NATIVE(0x103f9d60, 0x103fa284, R3, "170d14cca1a60aff", [](Cpu& c) {
    c.set(3, transfer_manager_native::check_offer_terms(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)));
})
TRANSFER_MANAGER_NATIVE(0x103f4fd0, 0x103f5a4c, R3, "6e928dbbfac390d7", [](Cpu& c) {
    c.set(3, transfer_manager_native::check_squad_room(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)));
})
TRANSFER_MANAGER_NATIVE(0x103f45a0, 0x103f4bf4, R3, "b271373fcd2fd549", [](Cpu& c) {
    c.set(3, transfer_manager_native::club_offer_rows(c, c.r(3), c.r(4), c.r(5)));
})
