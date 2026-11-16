// TRANSFER_MANAGER.CPP native-first block "decisions" (see transfer_manager_native_routines.inc).
TRANSFER_MANAGER_NATIVE(0x103eed00, 0x103ef580, R3, "33cc592b499f27a8", [](Cpu& c) {
    c.set(3, transfer_manager_native::plan_offer(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9)));
})
TRANSFER_MANAGER_NATIVE(0x103ef580, 0x103ef898, R3, "e029f46aecb3a979", [](Cpu& c) {
    c.set(3, transfer_manager_native::start_reaction_offer(c, c.r(4), c.r(5), c.r(6), c.r(7), c.r(8)));
})
TRANSFER_MANAGER_NATIVE(0x103fb630, 0x103fb838, NONE, "296242e01e216f0d", [](Cpu& c) {
    transfer_manager_native::remove_offer(c, c.r(3), c.r(4));
})
TRANSFER_MANAGER_NATIVE(0x103fdc20, 0x103fe194, NONE, "43d2ac95a880a44d", [](Cpu& c) {
    transfer_manager_native::post_offer_news(c, c.r(3), c.r(4));
})
TRANSFER_MANAGER_NATIVE(0x10410e50, 0x104113dc, R3, "d205217583e7e474", [](Cpu& c) {
    c.set(3, transfer_manager_native::describe_refusal(c, c.f(1), std::int32_t(c.r(4)), std::int32_t(c.r(5)), c.r(8)));
})
