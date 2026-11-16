// Call-trace argument findings accepted for this unit, each with its evidence. The argument masks
// (tools/call_arguments.py) take a varargs formatter or a routine that calls through a pointer to read all of
// r3-r10; a native caller leaves the registers past the ones it passes as its last callee left them. A line
// names the call (the address of its `bl`, as the checker prints it), the callee and the register, and says why
// the callee does not read it there. INJURY_UNIT_TRACE_ACCEPTED(bl, target, register, "reason")
INJURY_UNIT_TRACE_ACCEPTED(0x1018bdb8, 0x10768300, 10, "the sort takes base, count, row size and comparator in r3-r6 (docs/cpp-rewrites.md, the sort family); r10 is what the row copy before it left")
INJURY_UNIT_TRACE_ACCEPTED(0x1018c524, 0x10727890, 6, "text 'None<%s - COMMENT - no injury>': one %s, r5; r6 is what 0x10187a40 left and is only homed into this routine's own parameter area")
INJURY_UNIT_TRACE_ACCEPTED(0x101841b0, 0x10002390, 200, "checker cases only: FSMakeFSSpec's import cell is pointed at a game routine, which the glue enters by bctr; the call and its arguments match on both sides")
INJURY_UNIT_TRACE_ACCEPTED(0x1018c524, 0x10727890, 7, "same text: one %s, r5; a varargs home of what 0x10187a40 left")
INJURY_UNIT_TRACE_ACCEPTED(0x1018c524, 0x10727890, 8, "same text: one %s, r5; a varargs home of what 0x10187a40 left")
INJURY_UNIT_TRACE_ACCEPTED(0x1018c524, 0x10727890, 9, "same text: one %s, r5; a varargs home of what 0x10187a40 left")
INJURY_UNIT_TRACE_ACCEPTED(0x1018c524, 0x10727890, 10, "same text: one %s, r5; a varargs home of what 0x10187a40 left")
INJURY_UNIT_TRACE_ACCEPTED(0x10186480, 0x1018b2b0, 8, "the mask comes only from the two binary searches 0x10775600 in 0x1018b2b0: at 0x1018b47c (comparator TOC -0x5458 = 0x1018ffe0) and at 0x1018b554 (comparator TOC -0x5454 = 0x1018ff10, injury_unit_course.cpp), reached with r8/r10 unwritten when the first loop never calls 0x1018dcb0 and the first search returns 0; each calls its comparator through a pointer, which the mask counts as reading r3-r10, and both comparators read r3 and r4 only (mask 03)")
INJURY_UNIT_TRACE_ACCEPTED(0x10186480, 0x1018b2b0, 10, "same two searches: their comparators 0x1018ffe0 and 0x1018ff10 read r3 and r4 only, so r10 is never read")
INJURY_UNIT_TRACE_ACCEPTED(0x1018af18, 0x103fc710, 9, "same call into 0x103fc710 as DISCIPLINE's 0x100e0748: entry r9 reaches 0x10362ef0 (writes r9 at 0x10363064 before its one read; only indirect call is the comparator 0x10366410, mask 03) and the assertion path at 0x103fc8b0, whose sprintf format reads r6-r8 only")
INJURY_UNIT_TRACE_ACCEPTED(0x1018af18, 0x103fc710, 10, "same proof as r9: 0x10362ef0 never names r10; the assertion's sprintf format reads r6-r8 only")
