// CUP.CPP entries exempt from the r4-r10 comparison at return (cup_base_checks.cpp compares them for every
// other entry). An entry goes here only with the listing's proof that no caller can read them: no direct
// caller reads them (.local/rewrites/cup/registers_after.txt), and no virtual, comparator or qsort path can
// hand them on. One line each: 0xENTRY,  // the proof
