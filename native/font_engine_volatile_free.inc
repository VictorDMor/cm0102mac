// The entries whose callers read none of r4-r10 (one line each with the proof): none yet.
0x1075c0f0,  // glyph: its one caller is the direct `bl` of 0x1075fa10 (0x1075fecc, no descriptor) and reads nothing after the call (r3 and r4 are rewritten before the `bl 0x1075c8c0` / `bl 0x10767bb0` that follow): tools/register_contract.py --after 0x1075c0f0
0x1075e0c0,  // style: its one caller is the direct `bl` of 0x1075e320 (0x1075e678, no descriptor) and reads nothing after the call (the registers it needs are reloaded from memory): tools/register_contract.py --after 0x1075e0c0
0x10766dc0,  // truetype_glyph: its one caller is the direct `bl` of 0x1075eef0 (0x1075f328, no descriptor) and reads nothing after the call: tools/register_contract.py --after 0x10766dc0
0x107670e0,  // truetype_glyph: its one caller is the direct `bl` of 0x1075af60 (0x1075b958, no descriptor) and reads nothing after the call: tools/register_contract.py --after 0x107670e0
