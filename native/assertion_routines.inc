// The assertion module (tools/assertion_audit.py): entry, end, results, digest of the original body, hook.
// ASSERTION_ROUTINE runs in production; ASSERTION_DRAFT only in the check tools. The hooks read the guest ABI's arguments and leave the
// registers the original leaves (r3-r10 are compared in full by the checker).
ASSERTION_ROUTINE(0x10739890, 0x10739b3c, R3, "6fb8f344147b63fc", [](Cpu& c) { assertion::split_file_name(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7)); })
ASSERTION_ROUTINE(0x107755c0, 0x107755fc, R3, "9e32e6cda6f2ff85", [](Cpu& c) { assertion::report_failed_assertion(c, c.r(3), c.r(4), c.r(5)); })
