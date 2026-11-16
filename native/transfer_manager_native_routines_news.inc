// TRANSFER_MANAGER.CPP native-first block "news" (see transfer_manager_native_routines.inc).
TRANSFER_MANAGER_NATIVE(0x103ffe80, 0x104001e0, NONE, "620ec257c3c41d1f", [](Cpu& c) {
    transfer_manager_native::post_signing_news(c, c.r(3), c.r(4), c.r(5));
})
TRANSFER_MANAGER_NATIVE(0x104019d0, 0x10401c90, NONE, "0aa8c03bcd5a865a", [](Cpu& c) {
    transfer_manager_native::post_news_fa2(c, c.r(3), c.r(4), c.r(5));
})
TRANSFER_MANAGER_NATIVE(0x10402b90, 0x10402e68, NONE, "b71a0528d3ec2d2f", [](Cpu& c) {
    transfer_manager_native::post_news_fac(c, c.r(4));
})
TRANSFER_MANAGER_NATIVE(0x10403890, 0x10403a90, NONE, "7ba48cf8ec960cff", [](Cpu& c) {
    transfer_manager_native::post_news_faf(c, c.r(3), c.r(4), c.r(5), c.r(6));
})
TRANSFER_MANAGER_NATIVE(0x10403e20, 0x1040452c, NONE, "1ba0ef0684317782", [](Cpu& c) {
    transfer_manager_native::post_news_fa4(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10));
})
TRANSFER_MANAGER_NATIVE(0x10406910, 0x10406aec, NONE, "d5f98b74a1b195b3", [](Cpu& c) {
    transfer_manager_native::post_news_fa7(c, c.r(4), c.r(5), c.r(6), c.r(7));
})
TRANSFER_MANAGER_NATIVE(0x10406e00, 0x10406fc8, NONE, "ef0c9cf4ac2f61f3", [](Cpu& c) {
    transfer_manager_native::post_news_fa6(c, c.r(4), c.r(5), c.r(6), c.r(7));
})
TRANSFER_MANAGER_NATIVE(0x10407460, 0x10407a50, NONE, "5f3c9d58feaa136a", [](Cpu& c) {
    transfer_manager_native::post_news_fa8(c, c.r(3), c.r(4));
})
