// PLAYER_REGEN.CPP entries exempt from the r4-r10 comparison at return (player_regen_checks.cpp compares them for every
// other entry). An entry goes here only with the listing's proof that no caller can read them: no direct
// caller reads them (tools/register_contract.py --after ENTRY), and no virtual, comparator or qsort path can
// hand them on. One line each: 0xENTRY,  // the proof
0x102dc410,  // its two callers are the direct `bl`s of 0x102dbf00 (no descriptor, no virtual slot) and read r3 only: tools/register_contract.py --after 0x102dc410
0x102dbf00,  // its one caller is the direct `bl` of 0x102dabb0 (no descriptor, no virtual slot) and reads r3 only: tools/register_contract.py --after 0x102dbf00
0x102deb70,  // its three callers are direct `bl`s (0x102d8920, 0x102dabb0, 0x102dbeb0; no descriptor, no virtual slot) and read nothing after the call: tools/register_contract.py --after 0x102deb70
0x102ded90,  // its one caller is the direct `bl` of 0x102e4530 (no descriptor, no virtual slot) and reads nothing after the call: tools/register_contract.py --after 0x102ded90
0x102dbeb0,  // its two callers are direct `bl`s of CONTRACT_MANAGER's C++ (no descriptor, no virtual slot) and read nothing after the call: tools/register_contract.py --after 0x102dbeb0
0x102eb670,  // its one caller is the direct `bl` of 0x102de470 and ignores the result and the other registers: tools/register_contract.py --after 0x102eb670
0x102ea670,  // its one caller is the direct `bl` of 0x102ea3c0 and reads r3 only: tools/register_contract.py --after 0x102ea670 (its other registers flow on through 0x102ea3c0, whose callers read r9 and r10 only, which it never writes)
0x102e9b90,  // its one caller is the direct `bl` of 0x102e99f0 and reads r3 only: tools/register_contract.py --after 0x102e9b90 (same flow through 0x102e99f0)
0x102e98a0,  // its one caller is the direct `bl` of the constructor 0x102d8230 and reads nothing after the call: tools/register_contract.py --after 0x102e98a0
0x102e1f20,  // its one caller is the direct `bl` of 0x102d8920 and reads nothing after the call: tools/register_contract.py --after 0x102e1f20
0x102e21c0,  // its one caller is the direct `bl` of 0x102d8920 and reads nothing after the call: tools/register_contract.py --after 0x102e21c0
