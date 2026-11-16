// TRANSFER_MANAGER.CPP native-first block "offers" (see transfer_manager_native_routines.inc).
TRANSFER_MANAGER_NATIVE(0x103e3860, 0x103e3dd8, R3, "92bd59c4fccb2004", [](Cpu& c) {
    c.set(3, std::uint32_t(transfer_manager_native::random_below(c, std::int32_t(c.r(3)))));
})
TRANSFER_MANAGER_NATIVE(0x103e4130, 0x103e438c, NONE, "c3c20c83e2750faa", [](Cpu& c) {
    transfer_manager_native::close_offer_stage_2_with_bidders(c, c.r(3), c.r(4), c.r(5), c.r(6));
})
TRANSFER_MANAGER_NATIVE(0x103e4390, 0x103e44ac, NONE, "61f960b3840ae038", [](Cpu& c) {
    transfer_manager_native::close_offer_stage_2(c, c.r(3), c.r(4), c.r(5), c.r(6));
})
TRANSFER_MANAGER_NATIVE(0x103e44b0, 0x103e4740, NONE, "7a47de4a4fbd5cbc", [](Cpu& c) {
    transfer_manager_native::close_offer_stage_3(c, c.r(3), c.r(4), c.r(5), c.r(6));
})
TRANSFER_MANAGER_NATIVE(0x103e5b70, 0x103e628c, NONE, "0ad8eeea9dc4edac", [](Cpu& c) {
    transfer_manager_native::assess_player_after_offer(c, c.r(3), c.r(4));
})
TRANSFER_MANAGER_NATIVE(0x103e8500, 0x103e8bac, NONE, "89a5ea40bb5d8c1c", [](Cpu& c) {
    transfer_manager_native::answer_offer(c, c.r(3), c.r(4), c.r(5), c.r(6));
})
TRANSFER_MANAGER_NATIVE(0x103e8bb0, 0x103e8d24, R3, "77582f23a177d002", [](Cpu& c) {
    c.set(3, transfer_manager_native::review_pending_offer(c, c.r(3), c.r(4)) ? 1u : 0u);
})
TRANSFER_MANAGER_NATIVE(0x103e9910, 0x103e9ac0, NONE, "ebb076a6bcd96864", [](Cpu& c) {
    transfer_manager_native::withdraw_offer(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7));
})
TRANSFER_MANAGER_NATIVE(0x103e9ac0, 0x103e9cd4, NONE, "1a3be6c7cd079193", [](Cpu& c) {
    transfer_manager_native::withdraw_offers_of_club(c, c.r(3), c.r(4));
})
