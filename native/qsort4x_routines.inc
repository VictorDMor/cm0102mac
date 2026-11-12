QSORT4X_ROUTINE(0x10768300, 0x10768340, R3, "f9ff2e514c82235a", [](Cpu& c) { qsort4x::sort(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
QSORT4X_ROUTINE(0x10768340, 0x107684b8, R3, "7d0b9c27d4bf7f20", [](Cpu& c) { qsort4x::sort_by_bytes(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
QSORT4X_ROUTINE(0x107684c0, 0x10768658, R3, "03b276fd57bf4a61", [](Cpu& c) { qsort4x::sort_by_words(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
QSORT4X_ROUTINE(0x10768660, 0x10768808, R3, "3e9365c6efabaf31", [](Cpu& c) { qsort4x::sort_by_pairs(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
