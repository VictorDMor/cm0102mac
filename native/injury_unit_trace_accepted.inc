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
