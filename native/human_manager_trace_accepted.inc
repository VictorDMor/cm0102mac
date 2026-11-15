// Call-trace argument findings accepted for this unit, each with its evidence. The argument masks
// (tools/call_arguments.py) take a varargs formatter or a routine that calls through a pointer to read all of
// r3-r10; a native caller leaves the registers past the ones it passes as its last callee left them. A line
// names the call (the address of its `bl`, as the checker prints it), the callee and the register, and says why
// the callee does not read it there. HUMAN_MANAGER_TRACE_ACCEPTED(bl, target, register, "reason")
HUMAN_MANAGER_TRACE_ACCEPTED(0x10145bc8, 0x10002390, 200, "checker cases only: FSMakeFSSpec's import cell is pointed at a game routine, which the glue enters by bctr (as FINANCE.CPP's 0x100f85b0); the call and its arguments match on both sides")
HUMAN_MANAGER_TRACE_ACCEPTED(0x10145f8c, 0x107734a0, 5, "destroy_array (cpp_object_storage_arrays.cpp) reads r3 and r4 only, and passes r3/r4 to the element destructor 0x10145d70, which reads r3/r4 only; the original's r5 is a leftover")
