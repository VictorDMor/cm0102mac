// COMP_UTIL.CPP entries exempt from the r4-r10 comparison at return (comp_util_checks.cpp compares them for every
// other entry). An entry goes here only with the listing's proof that no caller can read them: no direct
// caller reads them (tools/register_contract.py --after ENTRY), and no virtual, comparator or qsort path can
// hand them on. One line each: 0xENTRY,  // the proof
0x10464160,  // its 12 callers (all direct `bl`s, no descriptor, no virtual slot) read r3 only after the call: tools/register_contract.py --after 0x10464160
0x10463be0,  // its one caller (a direct `bl`, cup_base_queries.cpp) reads nothing after the call: tools/register_contract.py --after 0x10463be0
0x10463dc0,  // its one caller (a direct `bl`, news_message_local.cpp) reads nothing after the call: tools/register_contract.py --after 0x10463dc0
0x104770c0,  // its one caller (CLUB_SCREENS.CPP, a direct `bl`) reads r3 only after the call: tools/register_contract.py --after 0x104770c0
0x10463000,  // only 0x10462cd0 and 0x104635b0 call it (a direct `bl`, and never reached: their bound fits a signed halfword), and they read r3 only after the call: tools/register_contract.py --after 0x10463000
