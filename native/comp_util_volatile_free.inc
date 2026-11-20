// COMP_UTIL.CPP entries exempt from the r4-r10 comparison at return (comp_util_checks.cpp compares them for every
// other entry). An entry goes here only with the listing's proof that no caller can read them: no direct
// caller reads them (tools/register_contract.py --after ENTRY), and no virtual, comparator or qsort path can
// hand them on. One line each: 0xENTRY,  // the proof
0x10464160,  // its 12 callers (all direct `bl`s, no descriptor, no virtual slot) read r3 only after the call: tools/register_contract.py --after 0x10464160
