// PLAYER_REGEN.CPP entries exempt from the r4-r10 comparison at return (player_regen_checks.cpp compares them for every
// other entry). An entry goes here only with the listing's proof that no caller can read them: no direct
// caller reads them (tools/register_contract.py --after ENTRY), and no virtual, comparator or qsort path can
// hand them on. One line each: 0xENTRY,  // the proof
0x102dc410,  // its two callers are the direct `bl`s of 0x102dbf00 (no descriptor, no virtual slot) and read r3 only: tools/register_contract.py --after 0x102dc410
0x102dbf00,  // its one caller is the direct `bl` of 0x102dabb0 (no descriptor, no virtual slot) and reads r3 only: tools/register_contract.py --after 0x102dbf00
