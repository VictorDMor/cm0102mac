#pragma once
// DISPLAY.CPP 0x1071de80: interprets one screen command read from a byte stream into a display table.
// The original is one routine (5,618 instructions) with a 0x18b0-byte frame: its prologue loads
// r13-r31 once, reads the command byte, and a jump table (TOC -0x3f90, 27 entries) sends each command
// to its code, which ends at the common exit 0x10723638 (r3 = -1) or, for two commands, at 0x1072363c
// with its own r3. Command 8 dispatches again through a second table (TOC -0x3f94, 11 entries).
//
// Here the dispatcher (display_interpreter.cpp) owns the frame, the prologue and the command read; each
// command is one C++ function taking the shared state below and returning r3. The files are split by
// command range only to keep each one readable:
//   display_interpreter.cpp           the dispatcher and commands 0-5
//   display_interpreter_build.cpp     commands 6-8 (and command 8's eleven sub-commands)
//   display_interpreter_controls.cpp  commands 9-26
#include "display_detail.hpp"

namespace cm::aot::display {
// The state the original keeps in registers and frame across a command. `regs` holds what r13-r31
// hold in the original at each point (CallRegisters): the dispatcher holds the prologue's values,
// and a command `hold`s every register it changes before its next dispatched call, then `apply`s.
struct Interpreter {
    Cpu& c;
    const Frame& frame;       // 0x18b0 bytes; the caller's r4 is stored at frame +0x18cc, r5's halfword at +0x18d2
    CallRegisters& regs;
    std::uint32_t table;      // r15 (entry r3): the display table
    std::uint32_t stream;     // r16 (entry r6): {bytes, position, length} words
    std::int8_t command;      // r21: the command byte (sign-extended); the caller's r21 when the read failed
    std::uint32_t r4_at_case; // r4 as the dispatcher leaves it at the bctr (the stream's bytes pointer, or
                              // the message buffer after the read's failed assertion); for a command that
                              // reads r4 before writing it
    CheckedStream source;     // the stream the commands read, with the register image applied before a report
};
using ReadSite = Site;
// The prologue's registers (held in `regs`): r14 game object (TOC -0x7bb0), r15 table, r16 stream,
// r18 TOC -0x5c24, r19 TOC -0x7468, r25 TOC -0x7408, r26 TOC -0x79e0, r28 success flag, r29 message
// buffer, r30 the pool; r21 the command.
inline constexpr std::uint32_t common_exit_result = 0xffffffffu;

// Commands by jump-table index (the address of the original's code for each).
std::uint32_t command_0(Interpreter&);   // 0x1071df5c
std::uint32_t command_1(Interpreter&);   // 0x1071ed10
std::uint32_t command_2(Interpreter&);   // 0x1071e430
std::uint32_t command_3(Interpreter&);   // 0x10720080
std::uint32_t command_4(Interpreter&);   // 0x107200b4 (exits at 0x1072363c with its own r3)
std::uint32_t command_5(Interpreter&);   // 0x107200bc
std::uint32_t command_6(Interpreter&);   // 0x10720148 (exits at 0x1072363c with its own r3 from 0x10721b14)
std::uint32_t command_7(Interpreter&);   // 0x10721b18
std::uint32_t command_8(Interpreter&);   // 0x10721d00, with the second table (0x10721e44 .. 0x107226d0)
std::uint32_t command_9(Interpreter&);   // 0x10722774
std::uint32_t command_10(Interpreter&);  // 0x107227dc
std::uint32_t command_11(Interpreter&);  // 0x10722c68
std::uint32_t command_12(Interpreter&);  // 0x10722d08
std::uint32_t command_13(Interpreter&);  // 0x10722e1c
std::uint32_t command_14(Interpreter&);  // 0x10722f30
std::uint32_t command_15(Interpreter&);  // 0x10722fb4
std::uint32_t command_16(Interpreter&);  // 0x107230f4
std::uint32_t command_17(Interpreter&);  // 0x10723228
std::uint32_t command_18(Interpreter&);  // 0x10723238
std::uint32_t command_19(Interpreter&);  // 0x10723248
std::uint32_t command_20(Interpreter&);  // 0x107232e0
std::uint32_t command_21(Interpreter&);  // 0x10723378
std::uint32_t command_22(Interpreter&);  // 0x10723390
std::uint32_t command_23(Interpreter&);  // 0x107233b8
std::uint32_t command_24(Interpreter&);  // 0x1072346c
std::uint32_t command_25(Interpreter&);  // 0x107234c8 (also index 26)

// 0x1071de80 itself: (table, r4, r5 halfword, stream) -> r3.
std::uint32_t interpret(Cpu&, std::uint32_t table, std::uint32_t r4, std::uint32_t r5, std::uint32_t stream);
}
