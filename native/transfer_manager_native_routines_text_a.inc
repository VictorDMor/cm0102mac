// TRANSFER_MANAGER.CPP native-first block "text_a" (see transfer_manager_native_routines.inc).
// The four message texts: r4 the news item, r5 the long-text flag, r6 the message output (r3, the manager, is not read).
TRANSFER_MANAGER_NATIVE(0x103fe1a0, 0x103ffe74, NONE, "c0cd677800a4003c", [](Cpu& c) {
    transfer_manager_native::news_text_4000(c, c.r(4), c.r(5), c.r(6));
})
TRANSFER_MANAGER_NATIVE(0x104106d0, 0x104108bc, NONE, "693f1daf5a988e9b", [](Cpu& c) {
    transfer_manager_native::news_text_3020(c, c.r(4), c.r(5), c.r(6));
})
TRANSFER_MANAGER_NATIVE(0x104116e0, 0x104119d4, NONE, "5cb981bd6a1813df", [](Cpu& c) {
    transfer_manager_native::news_text_4035(c, c.r(4), c.r(5), c.r(6));
})
TRANSFER_MANAGER_NATIVE(0x10410a60, 0x10410e4c, NONE, "f13f7fa8c35e58f9", [](Cpu& c) {
    transfer_manager_native::news_text_4032(c, c.r(4), c.r(5), c.r(6));
})
