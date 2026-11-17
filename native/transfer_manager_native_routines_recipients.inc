// TRANSFER_MANAGER.CPP native-first block "recipients" (see transfer_manager_native_routines.inc).
TRANSFER_MANAGER_NATIVE(0x103fc710, 0x103fd414, R3, "be443cb8c8458095", [](Cpu& c) {
    c.set(3, transfer_manager_native::news_recipients(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)));
})
TRANSFER_MANAGER_NATIVE(0x10410000, 0x104101e8, NONE, "d54f2564763003b3", [](Cpu& c) {
    transfer_manager_native::announce_signing(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7));
})
TRANSFER_MANAGER_NATIVE(0x103e68c0, 0x103e6ca4, NONE, "df2ae28badd471e3", [](Cpu& c) {
    transfer_manager_native::record_completed_transfer(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8));
})
TRANSFER_MANAGER_NATIVE(0x1040b470, 0x1040bc04, NONE, "3ecfd979d57a604f", [](Cpu& c) {
    transfer_manager_native::close_offers_for_person(c, c.r(3), c.r(4));
})
TRANSFER_MANAGER_NATIVE(0x103e64e0, 0x103e68b8, NONE, "73e6f204f259bb09", [](Cpu& c) {
    transfer_manager_native::complete_due_moves(c, c.r(3));
})
