// TRANSFER_MANAGER.CPP native-first block "daily" (see transfer_manager_native_routines.inc).
TRANSFER_MANAGER_NATIVE(0x103ea840, 0x103eae90, NONE, "50f3a983d4757f06", [](Cpu& c) {
    transfer_manager_native::run_daily_pass(c, c.r(3), c.r(4), c.r(5));
})
TRANSFER_MANAGER_NATIVE(0x103eb650, 0x103ebf48, NONE, "6e3916fdd127ea76", [](Cpu& c) {
    transfer_manager_native::process_offer(c, c.r(3), c.r(4));
})
TRANSFER_MANAGER_NATIVE(0x103ec0f0, 0x103ec440, NONE, "f01829fbb5b1ebdc", [](Cpu& c) {
    transfer_manager_native::dispatch_rule_dates(c, c.r(3), c.r(4));
})
TRANSFER_MANAGER_NATIVE(0x103e9ce0, 0x103ea244, NONE, "c29f1178d43640c2", [](Cpu& c) {
    transfer_manager_native::process_nation_offers(c, c.r(3), c.r(4));
})
TRANSFER_MANAGER_NATIVE(0x103e8280, 0x103e84fc, NONE, "656748d7f63485e2", [](Cpu& c) {
    transfer_manager_native::decide_answered_offer(c, c.r(3), c.r(4), c.r(5));
})
TRANSFER_MANAGER_NATIVE(0x103f3100, 0x103f353c, NONE, "142556e1e610ef4a", [](Cpu& c) {
    transfer_manager_native::run_due_offers(c, c.r(3));
})
