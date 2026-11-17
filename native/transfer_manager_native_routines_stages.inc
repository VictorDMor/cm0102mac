// TRANSFER_MANAGER.CPP native-first block "stages" (see transfer_manager_native_routines.inc).
TRANSFER_MANAGER_NATIVE(0x103e7cf0, 0x103e80d8, NONE, "c572667af4c44fac", [](Cpu& c) {
    transfer_manager_native::advance_offer(c, c.r(3), c.r(4));
})
TRANSFER_MANAGER_NATIVE(0x103e80e0, 0x103e8280, NONE, "941ff405b13f45da", [](Cpu& c) {
    transfer_manager_native::route_answered_offer(c, c.r(3), c.r(4), c.r(5));
})
TRANSFER_MANAGER_NATIVE(0x103ebf50, 0x103ec0e8, NONE, "72e40146e2d86320", [](Cpu& c) {
    transfer_manager_native::settle_agreed_offer(c, c.r(3), c.r(4));
})
TRANSFER_MANAGER_NATIVE(0x103e95a0, 0x103e9910, NONE, "159fd71fd623f8a3", [](Cpu& c) {
    transfer_manager_native::remove_listed_person(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7));
})
TRANSFER_MANAGER_NATIVE(0x103e6cb0, 0x103e6e24, NONE, "3f875b2fe0288f9d", [](Cpu& c) {
    transfer_manager_native::answer_person_offer(c, c.r(3), c.r(4), c.r(5), c.r(6));
})
TRANSFER_MANAGER_NATIVE(0x103e70f0, 0x103e76a4, NONE, "ef7193191cabb873", [](Cpu& c) {
    transfer_manager_native::accept_offer(c, c.r(3), c.r(4), std::int32_t(c.r(5)), c.r(6), c.r(7), c.r(8));
})
TRANSFER_MANAGER_NATIVE(0x103e76c0, 0x103e7cec, NONE, "a19eefe84ed78392", [](Cpu& c) {
    transfer_manager_native::reject_offer(c, c.r(3), c.r(4), std::int32_t(c.r(5)), c.r(6));
})
TRANSFER_MANAGER_NATIVE(0x103ed170, 0x103ed520, R3, "5f1b753da05a5218", [](Cpu& c) {
    c.set(3, transfer_manager_native::review_offer_people(c, c.r(3), c.r(4), c.r(5)));
})
