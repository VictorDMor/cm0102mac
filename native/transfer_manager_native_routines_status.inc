TRANSFER_MANAGER_NATIVE(0x103e8d30, 0x103e921c, NONE, "96d3c4fbaa5a9cd0", [](Cpu& c) {
    transfer_manager_native::match_transfer_statuses(c, c.r(3), c.r(4), c.r(5));
})
TRANSFER_MANAGER_NATIVE(0x103e9220, 0x103e92b0, R3, "bfbfd94cd48f6e43", [](Cpu& c) {
    c.set(3, std::uint32_t(transfer_manager_native::remaining_status_term(c, c.r(3), c.r(4), std::int8_t(c.r(5)))));
})
TRANSFER_MANAGER_NATIVE(0x103ed090, 0x103ed164, NONE, "0f47b2c82eb2b97e", [](Cpu& c) {
    transfer_manager_native::start_offer_with_weekday(c, c.r(3), c.r(4));
})
TRANSFER_MANAGER_NATIVE(0x103ed520, 0x103edef0, R3, "aa994cfe2fab3f98", [](Cpu& c) {
    c.set(3, transfer_manager_native::person_refused(c, c.r(3), c.r(4), c.r(6), c.r(7), c.r(8)));
})
TRANSFER_MANAGER_NATIVE(0x103ef8a0, 0x103efa98, NONE, "7e5b310085bae94f", [](Cpu& c) {
    transfer_manager_native::settle_offer_statuses(c, c.r(3), c.r(4));
})
TRANSFER_MANAGER_NATIVE(0x103f2560, 0x103f27fc, NONE, "91d39ff1cfcfe39b", [](Cpu& c) {
    transfer_manager_native::settle_status_fee(c, c.r(3), c.r(4), c.r(5));
})
TRANSFER_MANAGER_NATIVE(0x103f2800, 0x103f29f0, R3, "c253cc305359947f", [](Cpu& c) {
    c.set(3, transfer_manager_native::settle_status_row_fee(c, c.r(3), c.r(4), std::int32_t(c.r(5)), c.r(6), c.r(7)));
})
