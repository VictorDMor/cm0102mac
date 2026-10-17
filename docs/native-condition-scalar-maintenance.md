# Condition scalar maintenance

This phase reconstructs the complete `0x10187450..0x10187644` operation in
`native/actor_condition_lifecycle.cpp`. Its 125 indexed original instructions
occupy page `10187`; `0x1011c4b4` is the only indexed external caller, and no
external edge enters the interior. The original reseed routine `0x1077b770`
remains an explicit dependency.

The API accepts a condition controller and signed record index. The original
signed upper-bound guard follows the TOC loads and 96-byte frame setup. No lower
index check or null-controller check is added. An index at/above the limit returns
without dereferencing the controller. Eight integer saves, link state, flags,
source accounting, ordered accesses and dependency arguments are retained.

The live signed halfword scalar first decreases by one below -1000 through the
original wrapping halfword store. The staff trait comes from the low signed byte
of its halfword at `+0x5c`. A trait below 19 resets a still-negative scalar to
zero. Other paths either subtract 13 or subtract a signed remainder modulo 13,
with the original optional modulo-25 draw deciding between them. Each draw retains
its cursor reload, end comparison, cursor write, wrap/reseed and salt-read order.
The trait and controller table are reread at their original boundaries, including
after a callback. No unsigned normalization of negative salt/remainders is added.

The final profile-pointer read is live. A nonnull player profile whose raw byte
at `+0x0f` equals 20 limits the signed scalar to at most 1050. Other profile values
do not apply that cap. Earlier wrapping stores and their observable intermediate
values remain present.

## Accepted scope and evidence

This complete operation extends the accepted manifest from **700 to 701 entries**,
preserving the exact prior prefix. Nineteen attribute/condition entries now cover
**4,107 indexed original instructions**. Full generation retains 1,953,334
instructions across 1,922 pages, with no missing or pending targets. Only
`page_10187.cpp` changes among 1,923 generated C++ files. The entry hook precedes
accounting; the complete original body remains available for explicit controls.
Nine scope/source/reference/hook audits and 50 historical manifest-only methods
pass. Changes to 48 historical test files only update the current total.

Release and affected-unit UBSAN each pass **486,622 component comparisons**:
13,945 completions, 472,641 interruptions and 36 own/child negatives. Every one of
the 4,107 source sites is covered. This operation adds **24,654 comparisons**,
including all 125 sites, 3,966 completions and two discriminating negatives. Its
coverage requires no forced branch/register controls; the 36,400 structural cases
belong to preceding condition/sampler branches.

Independent complete original references agree on all registers, temporaries,
mapped memory, ordered reads/writes, dependency input/output ABI, source order and
accounting. Separate mathematical expectations cover signed narrowing, decrement,
reset, fixed/random selection, special cap and draw count. Fixtures include
halfword/trait/threshold boundaries, 32-bit salt overflow and negative remainders,
first/second draw wrap, signed invalid indices, null/aliased pointers, relocated
TOC and callback mutation of the live trait/controller table. Every budget cut,
each individually missing accessed byte and each dependency interruption is
compared. Own/reseed controls must actually discriminate changed behavior.

The shared-engine proof invokes the original BL at `0x1011c4b4` using an actual
captured controller, staff identifier and profile. It is a controlled callsite
invocation, not traversal of the complete daily/UI parent. Eight variants cover
the captured baseline, deep deficit, low-trait reset, fixed decrement, special
cap, both draw-wrap positions and an out-of-range index with null controller.
Both ordinary and guarded returns require dedicated C++ calls/returns. These add
**136 production comparisons** through direct/generated/CFM paths, global and
per-entry original controls, own negatives and budgets. Body cases use cuts at
20/30/40 instructions; the guarded short path uses earlier cuts.

Final aggregate Release/UBSAN each pass **742 production comparisons**: 440
returns, 302 budget stops and 44 negatives. Seventeen separate null-first-argument
fault comparisons also agree. **132 original-profile controls** pass in Release;
original/profile paths enter no modern root.

A paired **798-advance match** reaches full time with identical full state,
imports and per-tick accounting. Existing naturally used roots retain their
accepted counts, including 13,799 daily-refresh and nine queue-merge calls.
The new scalar-maintenance root has **0 natural calls** during that
simulation stepper. Its dedicated runtime coverage is the controlled actual-data
callsite and prepared variants described above. Earlier date, lifecycle,
post-match history-file and structural sampler-recursion scenarios also pass.

**742 nonlocal comparisons**, including **424 restore/handler faults**, pass
in Release and UBSAN. This root adds 42 comparisons across both memory channels,
its initial saves, first deficit write and final special-cap store. The prepared
negative-deficit input also completes its original reseed before the final
callback. Surviving handlers, full state, ordered effects/import ABI, accounting,
original-context and turbo controls agree. The interrupted C++ continuation is
abandoned and cannot falsely report a return. Preceding nested-parent/child
transfer cases remain included.

All four focused CTests pass. Fifteen affected units are instrumented with UBSAN;
all three complete runs have empty stderr, and every recorded source hash matches
the final reviewed source. Other archives/generated pages remain consistent
Release builds. This is not a whole-engine sanitizer or speed claim.

## Installed game and retention

The signed ARM64 game executable is installed with SHA-256
`e34a2933d4fcf5150c2331442d6c202ede170c251124fd6203ae17b60a88fc78`.
Candidate/installed identities and all four bundled original payloads agree.
Prepare Game rendering, original exit confirmation and normal exit pass. This
startup smoke is separate from the scalar-maintenance runtime proof.

All 845 protected original/save/handoff files, the previous installed-game
reference and installed Match Debugger remain unchanged. Cleanup removes only
the six reviewed linked Release/UBSAN checks (867,379,768
logical bytes), retaining apps, archives, objects, references, snapshots, recipes
and evidence. Transparent APFS compression additionally preserves all 165
selected snapshot files at the same paths, with identical ordinary-read SHA-256,
permissions and modification times. Their logical contents remain
20,949,696,570 bytes; allocated blocks decrease from 21,374,423,040 to
4,658,475,008 bytes, a reduction of 16,715,948,032 bytes. The disposable probe
copy is removed after its retained source is verified. Original/save/handoff
files are outside these compression targets. The gameplay checks precede this
storage operation; every snapshot's ordinary read bytes are independently
verified unchanged afterward.

Free space after cleanup is 81,231,818,752 bytes.

Save/Load: **not retested**. Performance: **not benchmarked**. This completes the
scalar-maintenance block; the full conversion goal remains active.
