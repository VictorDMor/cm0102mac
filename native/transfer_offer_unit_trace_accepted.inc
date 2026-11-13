// Call-trace argument findings accepted for this unit, each with its evidence. The argument masks
// (tools/call_arguments.py) take a varargs formatter or a routine that calls through a pointer to read all of
// r3-r10; a native caller leaves the registers past the ones it passes as its last callee left them. A line
// names the call (the address of its `bl`, as the checker prints it), the callee and the register, and says why
// the callee does not read it there. TRANSFER_OFFER_UNIT_TRACE_ACCEPTED(bl, target, register, "reason")
