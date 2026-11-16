// TRANSFER_MANAGER.CPP native-first block "news_outcomes" (see transfer_manager_native_routines.inc).
TRANSFER_MANAGER_NATIVE(0x10409200, 0x1040940c, NONE, "513ff76bf43fa233", [](Cpu& c) {
    transfer_manager_native::post_offer_news_fab(c, c.r(3), c.r(4), c.r(5));
})
TRANSFER_MANAGER_NATIVE(0x104099f0, 0x10409bd0, NONE, "0969758d77d04695", [](Cpu& c) {
    transfer_manager_native::post_offer_news_fad(c, c.r(4), c.r(5), c.r(6), c.r(7));
})
TRANSFER_MANAGER_NATIVE(0x1040a050, 0x1040a280, NONE, "ae43ae5400ba7376", [](Cpu& c) {
    transfer_manager_native::post_offer_news_fb0(c, c.r(3), c.r(4), c.r(5), c.r(6));
})
TRANSFER_MANAGER_NATIVE(0x1040c220, 0x1040c578, NONE, "8d103cf442cc26ff", [](Cpu& c) {
    transfer_manager_native::post_offer_news_fb3(c, c.r(3), c.r(4), c.r(5));
})
TRANSFER_MANAGER_NATIVE(0x1040ce40, 0x1040d0c8, NONE, "d4c65fb3adb4e1dc", [](Cpu& c) {
    transfer_manager_native::post_offer_news_fb4(c, c.r(3), c.r(4), c.r(5));
})
TRANSFER_MANAGER_NATIVE(0x1040ddc0, 0x1040df6c, NONE, "c82598203c006d5f", [](Cpu& c) {
    transfer_manager_native::post_offer_news_fb6(c, c.r(3), c.r(4), c.r(5));
})
TRANSFER_MANAGER_NATIVE(0x1040e2d0, 0x1040e490, NONE, "fff3b539798f0eb7", [](Cpu& c) {
    transfer_manager_native::post_offer_news_fb7(c, c.r(3), c.r(4), c.r(5));
})
TRANSFER_MANAGER_NATIVE(0x1040ea40, 0x1040ed98, NONE, "44e9628399a0863a", [](Cpu& c) {
    transfer_manager_native::post_offer_news_fb9(c, c.r(3), c.r(4));
})
TRANSFER_MANAGER_NATIVE(0x1040f270, 0x1040f444, NONE, "0cf57761c668180a", [](Cpu& c) {
    transfer_manager_native::post_person_news_fba(c, c.r(3), c.r(4), c.r(5), c.r(6));
})
TRANSFER_MANAGER_NATIVE(0x1040f780, 0x1040f984, NONE, "8087e5c75dd38e9a", [](Cpu& c) {
    transfer_manager_native::post_offer_news_fbb(c, c.r(4), c.r(5), c.r(6));
})
TRANSFER_MANAGER_NATIVE(0x10410500, 0x104106d0, NONE, "fabe603e780e7605", [](Cpu& c) {
    transfer_manager_native::post_person_news_bcc(c, c.r(4), c.r(5));
})
TRANSFER_MANAGER_NATIVE(0x104108c0, 0x10410a58, NONE, "9616e5c1d0260763", [](Cpu& c) {
    transfer_manager_native::post_offer_news_fc0(c, c.r(4), c.r(5));
})
TRANSFER_MANAGER_NATIVE(0x104113e0, 0x104116d8, NONE, "0b219819bc90df7a", [](Cpu& c) {
    transfer_manager_native::post_offer_news_fc3(c, c.r(4));
})
