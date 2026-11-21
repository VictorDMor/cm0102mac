// PLAYER_REGEN.CPP entries whose callers read only some of r4-r10 after the call: the checker compares those registers (bit n of
// the mask is r(4+n): 0x60 is r9 and r10) and no others. One line each: {0xENTRY, 0xMASK},  // the proof
{0x102e99f0, 0x60},  // its two callers are the direct `bl`s of 0x102e4530 (no descriptor, no virtual slot) and read r9 and r10 (and the result r3) after the call: tools/register_contract.py --after 0x102e99f0; no routine of the block writes r9 or r10, so they are what the last callee left, as in the original
{0x102ea3c0, 0x60},  // its one caller is the direct `bl` of 0x102e4530 and reads r9, r10 and the result: tools/register_contract.py --after 0x102ea3c0; r9 and r10 are never written by the block
{0x102dc000, 0x60},  // its two callers are the direct `bl`s of 0x102e4530 and read r9 and r10 (the second ignores the result): tools/register_contract.py --after 0x102dc000; r9 and r10 are never written by the block
