// TRANSFER_MANAGER.CPP native-first block "text_d" (see transfer_manager_native_routines.inc).
TRANSFER_MANAGER_NATIVE(0x10408f80, 0x104091f8, NONE, "af9f83ac116df184", [](Cpu& c) {
    transfer_manager_native::news_text_4009(c, c.r(4), c.r(5), c.r(6));
})
TRANSFER_MANAGER_NATIVE(0x104101f0, 0x104104fc, NONE, "28e71c574021b8b4", [](Cpu& c) {
    transfer_manager_native::news_text_4031(c, c.r(4), c.r(5), c.r(6));
})
TRANSFER_MANAGER_NATIVE(0x10405ca0, 0x10406910, NONE, "763f286665daba1b", [](Cpu& c) {
    transfer_manager_native::news_text_4005(c, c.r(4), c.r(5), c.r(6));
})
TRANSFER_MANAGER_NATIVE(0x10404530, 0x104058e0, NONE, "ae864959dc97e2a5", [](Cpu& c) {
    transfer_manager_native::news_text_4004(c, c.r(3), c.r(4), c.r(5), c.r(6));
})
