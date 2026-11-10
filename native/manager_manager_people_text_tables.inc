// The decision data of 0x101980a0 (manager_manager_people_text.cpp), read from the listing by a script and
// checked against it by the checker's cases. A script's nodes compare sp+0x39 with k; `leaf(n)` is the cell's
// n-th leaf, `end` no text. A leaf is {format offset in the text pool, return address, extra sp+0x38 argument}.
// The cells are the 25 jump tables of 7 cases (the club's band, from 0x10197e00): five relation results
// (0, 1, 2, -1, -2: 0x100f55b0 on the club's record) for the middle days band, and for the long band the
// same five times four groups of the second band (0x10197e80: 0, 1, 2, 3..6).

constexpr Node pattern_0[] = {{3, leaf(0), node(1), node(1)}, {6, leaf(1), leaf(0), leaf(0)}};
constexpr Node pattern_1[] = {{1, leaf(0), leaf(1), leaf(0)}};
constexpr Node pattern_2[] = {{3, node(1), leaf(0), leaf(0)}, {1, leaf(0), leaf(1), leaf(1)}};
constexpr Node pattern_3[] = {{3, node(1), node(2), node(2)}, {1, end, leaf(0), leaf(0)}, {6, leaf(1), end, end}};
constexpr Node pattern_4[] = {{2, leaf(0), leaf(1), node(1)}, {5, leaf(2), leaf(1), leaf(0)}};
constexpr Node pattern_5[] = {{2, node(1), leaf(2), leaf(0)}, {1, leaf(0), leaf(1), leaf(1)}};
constexpr Node pattern_6[] = {{4, node(1), leaf(3), leaf(0)}, {3, node(2), leaf(2), leaf(2)}, {1, leaf(0), leaf(1), leaf(1)}};
constexpr Node pattern_7[] = {{3, node(1), leaf(2), leaf(0)}, {1, leaf(0), leaf(1), leaf(1)}};
constexpr Node pattern_8[] = {{3, node(1), leaf(2), leaf(0)}, {1, leaf(0), leaf(1), leaf(0)}};
constexpr Node pattern_9[] = {{3, node(1), leaf(3), leaf(0)}, {1, leaf(0), leaf(1), leaf(2)}};
constexpr Node pattern_10[] = {{3, leaf(0), leaf(1), leaf(0)}};
constexpr Node pattern_11[] = {{4, node(1), leaf(0), node(3)}, {3, node(2), leaf(2), leaf(2)}, {1, leaf(0), leaf(1), leaf(1)}, {6, leaf(1), leaf(0), leaf(0)}};
constexpr Node pattern_12[] = {{5, node(1), leaf(1), leaf(0)}, {1, leaf(0), leaf(1), leaf(0)}};

constexpr Leaf low_leaves[5] = {
    {0x23b, 0x101981b0},  // rel0
    {0x19a, 0x10198190},  // rel1
    {0xf6, 0x10198170},  // rel2
    {0x2b4, 0x101981d0},  // relm1
    {0x387, 0x101981f0},  // relm2
};

// Days 30..59: one table per relation result, indexed by the club band (case 0..6).
constexpr Cell band_tables[5][7] = {
    {  // rel0
        {0x101986a8, direct, {{0xd07, 0x101986c4}}},
        {0x101986c8, direct, {{0xd8f, 0x101986e4}}},
        {0x101986e8, direct, {{0xe26, 0x10198704}}},
        {0x10198708, direct, {{0xe96, 0x10198724}}},
        {0x10198728, pattern_0, {{0xf08, 0x10198780}, {0xe26, 0x10198760}}},
        {0x10198784, pattern_0, {{0xf6f, 0x101987dc}, {0xe26, 0x101987bc}}},
        {0x101987e0, pattern_0, {{0xfdb, 0x10198838}, {0xe26, 0x10198818}}},
    },
    {  // rel1
        {0x10198480, pattern_1, {{0x8af, 0x101984d4}, {0x442, 0x101984b4}}},
        {0x101984d8, pattern_1, {{0x95f, 0x1019852c}, {0x442, 0x1019850c}}},
        {0x10198530, direct, {{0xa1e, 0x1019854c}}},
        {0x10198550, direct, {{0xab6, 0x1019856c}}},
        {0x10198570, pattern_2, {{0xa1e, 0x101985cc}, {0xb50, 0x101985ac}}},
        {0x101985d0, pattern_0, {{0xbdf, 0x10198628}, {0xa1e, 0x10198608}}},
        {0x1019862c, pattern_0, {{0xc73, 0x10198684}, {0xa1e, 0x10198664}}},
    },
    {  // rel2
        {0x10198248, pattern_1, {{0x4dd, 0x1019829c}, {0x442, 0x1019827c}}},
        {0x101982a0, pattern_1, {{0x590, 0x101982f4}, {0x442, 0x101982d4}}},
        {0x101982f8, direct, {{0x442, 0x10198314}}},
        {0x10198318, direct, {{0x652, 0x10198334}}},
        {0x10198338, pattern_2, {{0x442, 0x10198394}, {0x6ef, 0x10198374}}},
        {0x10198398, pattern_3, {{0x781, 0x101983dc}, {0x442, 0x101983fc}}},
        {0x10198400, pattern_2, {{0x442, 0x1019845c}, {0x818, 0x1019843c}}},
    },
    {  // relm1
        {0x1019885c, direct, {{0x1047, 0x10198878}}},
        {0x1019887c, direct, {{0x1129, 0x10198898}}},
        {0x1019889c, direct, {{0x121a, 0x101988b8}}},
        {0x101988bc, direct, {{0x12e4, 0x101988d8}}},
        {0x101988dc, pattern_0, {{0x13b0, 0x10198934}, {0x121a, 0x10198914}}},
        {0x10198938, pattern_0, {{0x1471, 0x10198990}, {0x121a, 0x10198970}}},
        {0x10198994, pattern_0, {{0x1537, 0x101989ec}, {0x121a, 0x101989cc}}},
    },
    {  // relm2
        {0x10198a10, direct, {{0x15f4, 0x10198a2c}}},
        {0x10198a30, direct, {{0x16be, 0x10198a4c}}},
        {0x10198a50, direct, {{0x1797, 0x10198a6c}}},
        {0x10198a70, direct, {{0x1849, 0x10198a8c}}},
        {0x10198a90, pattern_4, {{0x19c1, 0x10198b10}, {0x18fd, 0x10198af0}, {0x1797, 0x10198ad0}}},
        {0x10198b14, pattern_4, {{0x1b33, 0x10198b94}, {0x1a6a, 0x10198b74}, {0x1797, 0x10198b54}}},
        {0x10198b98, pattern_4, {{0x1be1, 0x10198c18}, {0x18fd, 0x10198bf8}, {0x1797, 0x10198bd8}}},
    },
};

// Days 60 and more: per relation result and per group of the second band (0, 1, 2, 3..6), then the first band.
constexpr Cell pair_tables[5][4][7] = {
    {  // rel0
        {  // second band 0
            {0x1019a244, pattern_5, {{0x81a1, 0x1019a2e0}, {0x800d, 0x1019a290, true}, {0x80d4, 0x1019a2bc, true}}},
            {0x1019a2e4, pattern_5, {{0x83f2, 0x1019a380}, {0x820a, 0x1019a330, true}, {0x82fb, 0x1019a35c, true}}},
            {0x1019a384, pattern_5, {{0x86d5, 0x1019a420}, {0x8485, 0x1019a3d0, true}, {0x85ad, 0x1019a3fc, true}}},
            {0x1019a424, pattern_5, {{0x89f1, 0x1019a4c0}, {0x8779, 0x1019a470, true}, {0x88b5, 0x1019a49c, true}}},
            {0x1019a4c4, pattern_5, {{0x8d04, 0x1019a560}, {0x8aaa, 0x1019a510, true}, {0x8bd7, 0x1019a53c, true}}},
            {0x1019a564, pattern_5, {{0x9000, 0x1019a600}, {0x8dae, 0x1019a5b0, true}, {0x8ed7, 0x1019a5dc, true}}},
            {0x1019a604, pattern_5, {{0x92b1, 0x1019a6a0}, {0x90af, 0x1019a650, true}, {0x91ab, 0x1019a67c, true}}},
        },
        {  // second band 1
            {0x1019a6c4, direct, {{0x81a1, 0x1019a6e4}}},
            {0x1019a6e8, pattern_6, {{0x95c8, 0x1019a7b0}, {0x9357, 0x1019a734}, {0x940c, 0x1019a760, true}, {0x94e2, 0x1019a78c, true}}},
            {0x1019a7b4, pattern_6, {{0x98f2, 0x1019a87c}, {0x9657, 0x1019a800}, {0x9714, 0x1019a82c, true}, {0x97fb, 0x1019a858, true}}},
            {0x1019a880, pattern_7, {{0x9b55, 0x1019a914}, {0x9992, 0x1019a8c4}, {0x9a4c, 0x1019a8f0, true}}},
            {0x1019a918, pattern_8, {{0x9db4, 0x1019a9ac}, {0x9c0a, 0x1019a95c}, {0x9cba, 0x1019a988, true}}},
            {0x1019a9b0, pattern_9, {{0xa0bb, 0x1019aa6c}, {0x9e5a, 0x1019a9f8}, {0xa00a, 0x1019aa48}, {0x9f0b, 0x1019aa24, true}}},
            {0x1019aa70, pattern_8, {{0xa2eb, 0x1019ab04}, {0xa166, 0x1019aab4}, {0xa201, 0x1019aae0, true}}},
        },
        {  // second band 2
            {0x1019ab28, direct, {{0x81a1, 0x1019ab48}}},
            {0x1019ab4c, direct, {{0xa38c, 0x1019ab6c}}},
            {0x1019ab70, pattern_1, {{0xa4b0, 0x1019abcc}, {0xa3f0, 0x1019aba8}}},
            {0x1019abd0, pattern_1, {{0xa61d, 0x1019ac2c}, {0xa554, 0x1019ac08}}},
            {0x1019ac30, pattern_10, {{0xa77c, 0x1019ac90}, {0xa6d3, 0x1019ac6c}}},
            {0x1019ac94, pattern_10, {{0xa88c, 0x1019acf4}, {0xa7e9, 0x1019acd0}}},
            {0x1019acf8, pattern_10, {{0xa999, 0x1019ad58}, {0xa8f3, 0x1019ad34}}},
        },
        {  // second band 3..6
            {0x1019ad7c, pattern_1, {{0x81a1, 0x1019add8}, {0xaa03, 0x1019adb4}}},
            {0x1019addc, pattern_1, {{0xa38c, 0x1019ae38}, {0xaab3, 0x1019ae14}}},
            {0x1019ae3c, pattern_1, {{0xac12, 0x1019ae98}, {0xab5e, 0x1019ae74}}},
            {0x1019ae9c, direct, {{0xac7f, 0x1019aebc}}},
            {0x1019aec0, pattern_10, {{0xa88c, 0x1019af20}, {0xa7e9, 0x1019aefc}}},
            {0x1019af24, pattern_10, {{0xad90, 0x1019af84}, {0xace8, 0x1019af60}}},
            {0x1019af88, pattern_10, {{0xaea1, 0x1019afe8}, {0xadfc, 0x1019afc4}}},
        },
    },
    {  // rel1
        {  // second band 0
            {0x10199760, pattern_5, {{0x4f39, 0x101997f0}, {0x4d4f, 0x101997a8, true}, {0x4e41, 0x101997d0, true}}},
            {0x101997f4, pattern_5, {{0x520b, 0x10199884}, {0x4fcd, 0x1019983c, true}, {0x50e9, 0x10199864, true}}},
            {0x10199888, pattern_5, {{0x556f, 0x10199918}, {0x52c9, 0x101998d0, true}, {0x541c, 0x101998f8, true}}},
            {0x1019991c, pattern_5, {{0x591a, 0x101999ac}, {0x563e, 0x10199964, true}, {0x57ac, 0x1019998c, true}}},
            {0x101999b0, pattern_5, {{0x5cc3, 0x10199a40}, {0x5a05, 0x101999f8, true}, {0x5b64, 0x10199a20, true}}},
            {0x10199a44, pattern_5, {{0x6055, 0x10199ad4}, {0x5d9f, 0x10199a8c, true}, {0x5efa, 0x10199ab4, true}}},
            {0x10199ad8, pattern_5, {{0x639c, 0x10199b68}, {0x6136, 0x10199b20, true}, {0x6264, 0x10199b48, true}}},
        },
        {  // second band 1
            {0x10199b8c, direct, {{0x4f39, 0x10199ba8}}},
            {0x10199bac, pattern_6, {{0x6766, 0x10199c64}, {0x6474, 0x10199bf4}, {0x6554, 0x10199c1c, true}, {0x6655, 0x10199c44, true}}},
            {0x10199c68, pattern_6, {{0x6b3c, 0x10199d20}, {0x6820, 0x10199cb0}, {0x6908, 0x10199cd8, true}, {0x6a1a, 0x10199d00, true}}},
            {0x10199d24, pattern_7, {{0x6d42, 0x10199dac}, {0x6820, 0x10199d64}, {0x6c07, 0x10199d8c, true}}},
            {0x10199db0, pattern_8, {{0x7037, 0x10199e38}, {0x6e29, 0x10199df0}, {0x6f0b, 0x10199e18, true}}},
            {0x10199e3c, pattern_9, {{0x7406, 0x10199ee8}, {0x710f, 0x10199e80}, {0x7323, 0x10199ec8}, {0x71f2, 0x10199ea8, true}}},
            {0x10199eec, pattern_8, {{0x76cc, 0x10199f74}, {0x74e3, 0x10199f2c}, {0x75b0, 0x10199f54, true}}},
        },
        {  // second band 2
            {0x10199f98, direct, {{0x4f39, 0x10199fb4}}},
            {0x10199fb8, direct, {{0x779f, 0x10199fd4}}},
            {0x10199fd8, pattern_1, {{0x7919, 0x1019a02c}, {0x782e, 0x1019a00c}}},
            {0x1019a030, pattern_1, {{0x7ae3, 0x1019a084}, {0x79e8, 0x1019a064}}},
            {0x1019a088, direct, {{0x7bcb, 0x1019a0a4}}},
            {0x1019a0a8, direct, {{0x7c6a, 0x1019a0c4}}},
            {0x1019a0c8, direct, {{0x7d03, 0x1019a0e4}}},
        },
        {  // second band 3..6
            {0x1019a108, direct, {{0x4f39, 0x1019a124}}},
            {0x1019a128, direct, {{0x779f, 0x1019a144}}},
            {0x1019a148, direct, {{0x7d9f, 0x1019a164}}},
            {0x1019a168, direct, {{0x7e37, 0x1019a184}}},
            {0x1019a188, direct, {{0x7c6a, 0x1019a1a4}}},
            {0x1019a1a8, direct, {{0x7ed4, 0x1019a1c4}}},
            {0x1019a1c8, direct, {{0x7f72, 0x1019a1e4}}},
        },
    },
    {  // rel2
        {  // second band 0
            {0x10198c94, pattern_5, {{0x1e99, 0x10198d24}, {0x1c8f, 0x10198cdc, true}, {0x1d91, 0x10198d04, true}}},
            {0x10198d28, pattern_5, {{0x218c, 0x10198db8}, {0x1f2e, 0x10198d70, true}, {0x205a, 0x10198d98, true}}},
            {0x10198dbc, pattern_5, {{0x24f3, 0x10198e4c}, {0x224b, 0x10198e04, true}, {0x239f, 0x10198e2c, true}}},
            {0x10198e50, pattern_5, {{0x28a1, 0x10198ee0}, {0x25c3, 0x10198e98, true}, {0x2732, 0x10198ec0, true}}},
            {0x10198ee4, pattern_5, {{0x2c4d, 0x10198f74}, {0x298d, 0x10198f2c, true}, {0x2aed, 0x10198f54, true}}},
            {0x10198f78, pattern_5, {{0x2fe2, 0x10199008}, {0x2d2a, 0x10198fc0, true}, {0x2e86, 0x10198fe8, true}}},
            {0x1019900c, pattern_5, {{0x3334, 0x1019909c}, {0x30c4, 0x10199054, true}, {0x31f7, 0x1019907c, true}}},
        },
        {  // second band 1
            {0x101990c0, direct, {{0x1e99, 0x101990dc}}},
            {0x101990e0, pattern_6, {{0x3702, 0x10199198}, {0x340d, 0x10199128}, {0x34ee, 0x10199150, true}, {0x35f0, 0x10199178, true}}},
            {0x1019919c, pattern_6, {{0x3adc, 0x10199254}, {0x37bd, 0x101991e4}, {0x38a6, 0x1019920c, true}, {0x39b9, 0x10199234, true}}},
            {0x10199258, pattern_7, {{0x3ce4, 0x101992e0}, {0x37bd, 0x10199298}, {0x3ba8, 0x101992c0, true}}},
            {0x101992e4, pattern_8, {{0x3fdc, 0x1019936c}, {0x3dcc, 0x10199324}, {0x3eaf, 0x1019934c, true}}},
            {0x10199370, pattern_7, {{0x4286, 0x101993f8}, {0x41e7, 0x101993d8}, {0x40b5, 0x101993b8, true}}},
            {0x101993fc, pattern_11, {{0x451d, 0x10199490}, {0x4364, 0x10199448}, {0x4400, 0x10199470, true}}},
        },
        {  // second band 2
            {0x101994b4, direct, {{0x1e99, 0x101994d0}}},
            {0x101994d4, direct, {{0x45f1, 0x101994f0}}},
            {0x101994f4, pattern_1, {{0x476d, 0x10199548}, {0x4681, 0x10199528}}},
            {0x1019954c, pattern_1, {{0x4939, 0x101995a0}, {0x483d, 0x10199580}}},
            {0x101995a4, direct, {{0x4a22, 0x101995c0}}},
            {0x101995c4, direct, {{0x4ac2, 0x101995e0}}},
            {0x101995e4, direct, {{0x4b5c, 0x10199600}}},
        },
        {  // second band 3..6
            {0x10199624, direct, {{0x1e99, 0x10199640}}},
            {0x10199644, direct, {{0x45f1, 0x10199660}}},
            {0x10199664, direct, {{0x4bf9, 0x10199680}}},
            {0x10199684, direct, {{0x4c92, 0x101996a0}}},
            {0x101996a4, direct, {{0x4ac2, 0x101996c0}}},
            {0x101996c4, direct, {{0x41e7, 0x101996e0}}},
            {0x101996e4, direct, {{0x4364, 0x10199700}}},
        },
    },
    {  // relm1
        {  // second band 0
            {0x1019b048, direct, {{0xaf0a, 0x1019b068}}},
            {0x1019b06c, pattern_1, {{0xb097, 0x1019b0c8}, {0xafa9, 0x1019b0a4}}},
            {0x1019b0cc, pattern_1, {{0xb251, 0x1019b128}, {0xb160, 0x1019b104}}},
            {0x1019b12c, pattern_1, {{0xb438, 0x1019b188}, {0xb32b, 0x1019b164}}},
            {0x1019b18c, pattern_1, {{0xb621, 0x1019b1e8}, {0xb522, 0x1019b1c4}}},
            {0x1019b1ec, pattern_1, {{0xb7fc, 0x1019b248}, {0xb6fc, 0x1019b224}}},
            {0x1019b24c, pattern_1, {{0xb9bb, 0x1019b2a8}, {0xb8d8, 0x1019b284}}},
        },
        {  // second band 1
            {0x1019b2cc, direct, {{0xaf0a, 0x1019b2ec}}},
            {0x1019b2f0, pattern_1, {{0xbb6c, 0x1019b34c}, {0xba92, 0x1019b328}}},
            {0x1019b350, pattern_1, {{0xbd1e, 0x1019b3ac}, {0xbc31, 0x1019b388}}},
            {0x1019b3b0, pattern_12, {{0xbedb, 0x1019b414}, {0xbde3, 0x1019b3f0}}},
            {0x1019b418, pattern_12, {{0xc086, 0x1019b47c}, {0xbf98, 0x1019b458}}},
            {0x1019b480, pattern_1, {{0xc237, 0x1019b4dc}, {0xc141, 0x1019b4b8}}},
            {0x1019b4e0, pattern_1, {{0xc237, 0x1019b53c}, {0xc313, 0x1019b518}}},
        },
        {  // second band 2
            {0x1019b560, direct, {{0xaf0a, 0x1019b580}}},
            {0x1019b584, direct, {{0xc40e, 0x1019b5a4}}},
            {0x1019b5a8, pattern_1, {{0xc5a4, 0x1019b604}, {0xc4a8, 0x1019b5e0}}},
            {0x1019b608, pattern_1, {{0xc77b, 0x1019b664}, {0xc67e, 0x1019b640}}},
            {0x1019b668, direct, {{0xc867, 0x1019b688}}},
            {0x1019b68c, direct, {{0xc90e, 0x1019b6ac}}},
            {0x1019b6b0, direct, {{0xc9af, 0x1019b6d0}}},
        },
        {  // second band 3..6
            {0x1019b6f4, direct, {{0xaf0a, 0x1019b714}}},
            {0x1019b718, direct, {{0xc40e, 0x1019b738}}},
            {0x1019b73c, direct, {{0xca53, 0x1019b75c}}},
            {0x1019b760, pattern_10, {{0xcbd4, 0x1019b7c0}, {0xcaf6, 0x1019b79c}}},
            {0x1019b7c4, pattern_10, {{0xc90e, 0x1019b824}, {0xcc77, 0x1019b800}}},
            {0x1019b828, pattern_10, {{0xce34, 0x1019b888}, {0xcd53, 0x1019b864}}},
            {0x1019b88c, pattern_10, {{0xcfb8, 0x1019b8ec}, {0xceda, 0x1019b8c8}}},
        },
    },
    {  // relm2
        {  // second band 0
            {0x1019b94c, direct, {{0xd05b, 0x1019b96c}}},
            {0x1019b970, pattern_1, {{0xd22c, 0x1019b9cc}, {0xd10f, 0x1019b9a8}}},
            {0x1019b9d0, pattern_1, {{0xd424, 0x1019ba2c}, {0xd30a, 0x1019ba08}}},
            {0x1019ba30, pattern_1, {{0xd640, 0x1019ba8c}, {0xd513, 0x1019ba68}}},
            {0x1019ba90, pattern_1, {{0xd864, 0x1019baec}, {0xd756, 0x1019bac8}}},
            {0x1019baf0, pattern_1, {{0xda7c, 0x1019bb4c}, {0xd95b, 0x1019bb28}}},
            {0x1019bb50, pattern_1, {{0xdc92, 0x1019bbac}, {0xdb74, 0x1019bb88}}},
        },
        {  // second band 1
            {0x1019bbd0, direct, {{0xd05b, 0x1019bbf0}}},
            {0x1019bbf4, pattern_1, {{0xde98, 0x1019bc50}, {0xdd85, 0x1019bc2c}}},
            {0x1019bc54, pattern_1, {{0xe081, 0x1019bcb0}, {0xdf72, 0x1019bc8c}}},
            {0x1019bcb4, pattern_1, {{0xe28a, 0x1019bd10}, {0xe16c, 0x1019bcec}}},
            {0x1019bd14, pattern_1, {{0xe49b, 0x1019bd70}, {0xe38c, 0x1019bd4c}}},
            {0x1019bd74, pattern_1, {{0xda7c, 0x1019bdd0}, {0xe58e, 0x1019bdac}}},
            {0x1019bdd4, pattern_1, {{0xe79f, 0x1019be30}, {0xe6a2, 0x1019be0c}}},
        },
        {  // second band 2
            {0x1019be54, direct, {{0xd05b, 0x1019be74}}},
            {0x1019be78, direct, {{0xe887, 0x1019be98}}},
            {0x1019be9c, pattern_1, {{0xea41, 0x1019bef8}, {0xe936, 0x1019bed4}}},
            {0x1019befc, pattern_1, {{0xec49, 0x1019bf58}, {0xeb30, 0x1019bf34}}},
            {0x1019bf5c, pattern_10, {{0xee3b, 0x1019bfbc}, {0xed4a, 0x1019bf98}}},
            {0x1019bfc0, pattern_10, {{0xefda, 0x1019c020}, {0xeeef, 0x1019bffc}}},
            {0x1019c024, pattern_10, {{0xf176, 0x1019c084}, {0xf088, 0x1019c060}}},
        },
        {  // second band 3..6
            {0x1019c0a8, direct, {{0xd05b, 0x1019c0c8}}},
            {0x1019c0cc, direct, {{0xe887, 0x1019c0ec}}},
            {0x1019c0f0, direct, {{0xf227, 0x1019c110}}},
            {0x1019c114, direct, {{0xf2df, 0x1019c134}}},
            {0x1019c138, pattern_10, {{0xefda, 0x1019c198}, {0xf38f, 0x1019c174}}},
            {0x1019c19c, pattern_10, {{0xf560, 0x1019c1fc}, {0xf475, 0x1019c1d8}}},
            {0x1019c200, pattern_10, {{0xf6fb, 0x1019c260}, {0xf613, 0x1019c23c}}},
        },
    },
};
