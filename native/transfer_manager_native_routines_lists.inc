// TRANSFER_MANAGER.CPP native-first block "lists" (see transfer_manager_native_routines.inc).
TRANSFER_MANAGER_NATIVE(0x103f9200, 0x103f93dc, NONE, "9c42c27fed7f34d9", [](Cpu& c) {
    transfer_manager_native::first_accepted_day(c, c.r(3), c.r(4), c.r(5));
})
TRANSFER_MANAGER_NATIVE(0x103f3670, 0x103f3bf0, NONE, "a46dadd020a83596", [](Cpu& c) {
    transfer_manager_native::list_dated_entries(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9));
})
TRANSFER_MANAGER_NATIVE(0x103f6240, 0x103f80e4, NONE, "73c6bc68eab14b1d", [](Cpu& c) {
    transfer_manager_native::advance_contract_stage(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10));
})
