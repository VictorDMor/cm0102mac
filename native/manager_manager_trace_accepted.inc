// Call-trace argument findings accepted for native-first entries, each with its evidence. The argument masks
// (tools/call_arguments.py) take a varargs formatter or a routine that calls through a pointer to read all of
// r3-r10; a native caller leaves the registers past the ones it passes as its last callee left them. A line
// names the call (the address of its `bl`, as the checker prints it), the callee and the register, and says why the callee does
// not read it there. MANAGER_MANAGER_TRACE_ACCEPTED(bl, target, register, "reason")
MANAGER_MANAGER_TRACE_ACCEPTED(0x101ab08c, 0x1077b720, 6, "assertion text 'add_job_vacancy() - Can't find a manager for '%s U21'...': one %s, r5")
MANAGER_MANAGER_TRACE_ACCEPTED(0x101ab08c, 0x1077b720, 7, "same format: one %s")
MANAGER_MANAGER_TRACE_ACCEPTED(0x101ab0a8, 0x1077b720, 6, "assertion text 'add_job_vacancy() - Can't find a manager for '%s'...': one %s, r5")
MANAGER_MANAGER_TRACE_ACCEPTED(0x101ab0a8, 0x1077b720, 7, "same format: one %s")
MANAGER_MANAGER_TRACE_ACCEPTED(0x101c0108, 0x10773320, 8, "array constructor: saves and uses r3-r7 only (0x10773320..0x10773354), then calls the element constructor through the glue with registers it sets")
MANAGER_MANAGER_TRACE_ACCEPTED(0x101c0108, 0x10773320, 9, "same")
MANAGER_MANAGER_TRACE_ACCEPTED(0x101c0108, 0x10773320, 10, "same")
MANAGER_MANAGER_TRACE_ACCEPTED(0x101bf548, 0x10727890, 10, "the referee's dismissal text (pool + 0x2044b) has three <%s> fields, r7-r9 after the two sizes in r5/r6: r10 is not read")
