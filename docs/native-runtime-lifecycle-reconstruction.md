# Native Mac runtime lifecycle reconstruction

This increment reconstructs six shutdown/global-destructor/signal bodies as
named C++23 routines, reached by default at the ARM64 production destination
hooks. The complete engine/runtime modernization goal continues beyond this
bounded block.

## Source and production integration

The six complete bodies contain 145 original indexed instructions. Full-index
flow/fallthrough inspection finds zero external interior edges. The audit pins
all original fields, complete bodies, the two fragments of `exit` across pages
`0x10773`/`0x10774`, and one destination hook before the first original operation.
The shutdown page uses the existing hot-page counter; its hook is also required
to precede that counter's first original step. Both generated temporary-storage
modes and missing/duplicate/changed/misplaced hooks/bodies have negative controls.

| Entry | End exclusive | Instructions | Role | SHA-256 |
|---|---|---:|---|---|
| `0x10772df0` | `0x10772e0c` | 7 | Register global destructor record | `50559af8d1871af006b2f1e420ba496a86227d93bdd330e483b80666b9782640` |
| `0x10772e10` | `0x10772e64` | 21 | Drain global destructor records | `08660daaa3fd1e587b56f63106c1ddc669e6c93b2198ab7a22ae8fffd4d43e5f` |
| `0x10773fb0` | `0x10773fe8` | 14 | Abort signal and termination | `5deebb97b7f7025b52b860a02a56f2e85d408af0b833d0892527f9be8e316407` |
| `0x10773ff0` | `0x1077405c` | 27 | Exit cleanup and termination | `14653993f6f58af5c8ae50a89465fb70279041c7294830b418386339d2f6e601` |
| `0x10774060` | `0x107740e4` | 33 | Runtime exit callbacks and ExitToShell | `b60742fc3b5da3458566cf3a895753e70d09fd6bc39167a7db72f7b9409aa094` |
| `0x1077db20` | `0x1077dbcc` | 43 | Raise signal through live handler table | `8112cd00fdc13e0f85574d819149801b91b7a1ac245826a1f51480adae9595c5` |

The manifest appends six entries to the accepted 182-entry prefix, whose digest
is `21eeea757467d5a9e871fc5ac9b6f482f328e7132cb4dd91a4c8cdf0778abd2c`.
The 188-entry digest is
`71685ebd95d1c0fc78ebeaa5ec1d9da103cc49027e1b75cca68860c93cc976af`.
Generation retains 1,953,157 instructions, 1,922 pages and zero missing/pending
targets. One consistent Release ARM64 rebuild compiled all 1,922 generated pages
after State gained lifecycle diagnostics (652.16 seconds).

`runtime_lifecycle_adapter` owns independent global/per-entry original selection,
modern/original counters, successful modern return/termination counters,
dispatch escapes, reset and observers. Diagnostics and attached fixtures remain
private and process-local; none is serialized into guest data. Original mode with
an observer runs through a one-use destination resume, retaining child selection.
The adapter catches only the existing `ApplicationExit` success signal to report
a nonreturning boundary, then rethrows it. A failed child-boundary comparison is
rejected before its parent can report successful termination.

## Preserved contracts

`runtime_lifecycle_objects.cpp` owns registration and destruction. The leaf
registration reads the live head, stores the old head/vector/object into the
three-word guest record in that order, then publishes the record. It keeps the
original alias behavior. Destruction has a 64-byte frame, saves `r31`, unlinks each
record before loading its object and vector, invokes the original CFM thunk with
`r4=-1`, restores caller TOC and rereads the live head. A callback can add another
object during traversal; no host list snapshot replaces these accesses.

`runtime_lifecycle_exit.cpp` owns abort, exit and terminal cleanup. All have
64-byte frames. Abort raises signal 1 first, then writes the skip-cleanup flag
and exits with argument 1. This original policy leaves global destructors and
stream cleanup pending while still executing tail callbacks. Exit retains its
status in `r30`; only a zero skip flag drains global records and invokes the
optional stream-cleanup vector. Each optional cleanup cell is cleared only after
its callback returns. Exit then invokes terminal cleanup with the retained
status. No POSIX abort/status policy is substituted.

Terminal cleanup uses the live guest callback array and signed count. A strictly
positive count is loaded again, decremented and stored before selecting the
reverse-order CFM vector. The count is reread after every callback, including
callback repopulation. The optional terminal vector is invoked and then cleared.
Finally, the existing native `ExitToShell` bridge throws `ApplicationExit` and
unwinds the application successfully. Production never executes the following
return epilogue. An explicitly attached dependency-return fixture characterizes
that otherwise dormant original epilogue without changing the production bridge.

`runtime_lifecycle_signal.cpp` owns raising a signed signal in the range 1..6.
Invalid values return the original low word `-1` without accessing the table.
The live six-word table contains either default 0, ignore 1 or a CFM vector.
Every value except 1 is reset to zero before the policy test/invocation. Default
signal 1 is ignored; default signals 2..6 invoke exit with argument zero. Custom
handlers receive the signal and may reinstall a handler. Ignore/custom successful
returns produce zero. If a characterization fixture makes the default exit
return, the original falls into a null CFM-vector mapping fault; this edge is
preserved rather than repaired. Load-with-update publishes the base only after a
successful handler read, retaining its fault prefix.

All bodies use the established Mac ABI: low-word arithmetic, big-endian ordered
memory, scratch/nonvolatile registers, CR/CA/CTR, original stack saves, LR/TOC and
PC at returns, mapping faults and the native termination boundary. Generated
CFM thunks, the existing semantic stream cleanup and the SDK bridge remain
explicit dependencies. There is no replay of any reconstructed body.

Startup/root initialization `0x10773710`, nonlocal contexts
`0x10773eb0/0x10773f30`, heap glues `0x10778d50/0x10778d80`, signal installation
APIs and host OS/resource services remain outside this increment. No unobserved
`atexit` registration API is invented, and this block is not described as the
whole C runtime lifecycle service.

## Focused validation

Before production integration, an unchanged-State component build compared all
six root bodies against the accepted 182-entry library: 90 complete-state/effect
comparisons, 11 expected mapping faults and 27 actual native terminations passed.
Those preliminary calls use generated children and are not production-hook
coverage. The production harness additionally exercises direct/CFM entry,
independent child/parent original selection, own/child return and terminal
negatives, sparse mapping prefixes, callback list/count/handler mutation,
relocated TOC and actual semantic stream-cleanup composition.

The final production harness passed **128 full-state/effect comparisons**,
including **28 expected mapping faults**, **45 actual native terminations** and
**19 rejected own/child return or termination mutations**. Every architectural
register/temporary byte, PC, mapped guest byte, allocator map/error and native-call
count is compared, together with ordered accesses, CFM ABI and import prefixes.
Only instruction accounting and private diagnostics are excluded. Default cases
have zero original selection and zero dispatch escapes; mixed cases explicitly
select only their intended original entry.

| Entry | Nonfault modern witnesses |
|---|---:|
| `0x10772df0` | 8 |
| `0x10772e10` | 32 |
| `0x10773fb0` | 8 |
| `0x10773ff0` | 29 |
| `0x10774060` | 46 |
| `0x1077db20` | 47 |

All six have direct and generated-CFM entry coverage, independent original
selection and own boundary negatives. Child checks cover abort/raise/exit,
exit/destructors/termination and default raise/exit/termination, rejecting a
changed child before its parent completes. Real native termination remains a
successful nonreturning boundary, including nested unwind. A real semantic
`dispose_all` stream-cleanup call has its own nonzero dependency counter and zero
original/escape diagnostics. The existing stream, controller and allocator
harnesses retain their broader cleanup/storage evidence.

Seven focused CTests passed in 20.63 seconds: lifecycle, stream-buffer,
allocator, guarded allocator replenishment, file-controller equivalence, native
bridges and original application shutdown. All fifteen affected Python audit
suites passed (88 tests). UBSAN instrumented all four lifecycle implementation
units, modified dispatch and the final harness; it linked against the consistent
Release libraries and passed with the same coverage counts and empty stderr.

One protected installation delivered signed source/candidate/installed ARM64
executable SHA-256
`fdcd54f2fdb4386a8d9dd2350491c0e15cd071c9964cbae031060e70533be34e`.
The unsigned linker executable was
`991e9f5109240fced31bbebe93da88d9216cd9e1a20d44b980c8fa8869f2b8ae`.
Strict signatures, normal production plist and four original payloads passed.
The previous installed path artifact is preserved with forks/attributes/metadata
at `ecb56a3ecf166793c88189d5c1ff10af049686c049e21182739220327b28d6da`.

The installed app rendered Prepare Game. The original named Quit command opened
its in-game confirmation, and Return confirmed normal exit. The exclusively
owned process 90304 was confirmed closed without another post-exit UI query.
The normal-exit log records 4,280,759,117 recompiled instructions and zero original
lifecycle selection/dispatch escapes, with no fatal AOT diagnostics. UI smoke
covers four entries: registration 12 returns, destruction 1 return, exit 1
termination and terminal cleanup 1 termination. Abort and raise remain zero in
this UI flow; all-six coverage is established by the production harness.

Before installation, Game (809), Runtime (5) and Runs (68) files were protected
with full content/fork/attribute/metadata inventories. Immediate post-install
verification passed. Final post-smoke preservation covers Game, existing
user/workspace saves, all 23 handoffs, original payloads, the installed reference
and 145 older private bundles. Runtime/Runs and presentation preferences may
change during UI; their post-smoke byte identity is not claimed. Original
material, generated sources, receipts/logs and bundles remain private and
unversioned.

Save/Load: **not retested**. Performance: **not benchmarked**. No day/week/season
or match campaign was added for this bounded runtime increment.
