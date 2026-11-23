// Entries compared on the dead stack too (assertion_checks.cpp): both. 0x10739890 builds the original's whole 0x460-byte frame (the saved
// registers, the link register, the back chain, the four 256-byte buffers) and its string calls run at its own stack pointer: MAIN's
// printf 0x10778db0 (zero tests at 0x10779594..0x107795e8), the stream routines 0x10776940 and 0x10776a40 and the import area's 0x10bf7bbc
// read those bytes as leftovers (tools/stack_contract.py module MAIN.CPP; the zero tests of 0x10776a40 and 0x10bf7bbc "differ").
// 0x107755c0 builds its 0x40-byte frame (the link register and the back chain) and calls fprintf and abort at its own stack pointer.
0x10739890, 0x107755c0,
