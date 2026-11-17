// TRANSFER_MANAGER.CPP native-first block "flow" (see transfer_manager_native_routines.inc).
TRANSFER_MANAGER_NATIVE(0x103e4740, 0x103e4c90, NONE, "16c6a1cbc54dcedf", [](Cpu& c) {
    transfer_manager_native::conclude_offer_round(c, c.r(3), c.r(4), c.r(5));
})
TRANSFER_MANAGER_NATIVE(0x103ee1b0, 0x103eebf8, NONE, "d4af110143cda14e", [](Cpu& c) {
    transfer_manager_native::review_offer_slot(c, c.r(3), c.r(4), c.r(5));
})
TRANSFER_MANAGER_NATIVE(0x103eec00, 0x103eecf4, R3, "7c06c30ac790473d", [](Cpu& c) {
    c.set(3, transfer_manager_native::make_candidate_offer(c, c.r(4), c.r(5), c.r(6), c.r(7), c.r(8)));
})
TRANSFER_MANAGER_NATIVE(0x103e4d70, 0x103e580c, NONE, "86c25834705df715", [](Cpu& c) {
    transfer_manager_native::evaluate_listed_offer(c, c.r(3), c.r(4), c.r(5));
})
TRANSFER_MANAGER_NATIVE(0x103e5810, 0x103e5b6c, NONE, "aa550ca3ab1648ce", [](Cpu& c) {
    transfer_manager_native::settle_contract_fee_offer(c, c.r(3), c.r(4));
})
