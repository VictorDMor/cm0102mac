// TRANSFER_MANAGER.CPP native-first block "text_b" (see transfer_manager_native_routines.inc).
TRANSFER_MANAGER_NATIVE(0x104026b0, 0x10402b88, NONE, "c8a00d2d6c1cce4f", [](Cpu& c) {
    transfer_manager_native::news_text_4003(c, c.r(4), std::int8_t(c.r(5)), c.r(6));
})
TRANSFER_MANAGER_NATIVE(0x10401c90, 0x104024d8, NONE, "e033201a48fb7055", [](Cpu& c) {
    transfer_manager_native::news_text_4002(c, c.r(4), c.r(5), c.r(6));
})
TRANSFER_MANAGER_NATIVE(0x104001e0, 0x104019cc, NONE, "0468182b5b1ed8a4", [](Cpu& c) {
    transfer_manager_native::news_text_4001(c, c.r(4), std::int8_t(c.r(5)), c.r(6));
})
