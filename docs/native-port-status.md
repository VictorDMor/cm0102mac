# Native port investigation — updated 2026-10-15

## Required outcome

Run the actual Championship Manager 01/02 simulation as an ARM64 macOS application,
without an emulator. Asset viewers, recompiled test programs, decompiler output,
and a native launcher around a legacy runtime do not satisfy this outcome.

A previously accepted native build completed the core single-player flow in live use:
managed game creation/loading, player and competition screens, date advancement,
a full match, return to management, saving, normal exit, and reloading the
post-match save with the Anapolina result preserved (user confirmed). Multiplayer and some other services remain
unfinished; this is a working development build, not a fully validated release.

## Latest C++23 coordinator delivery

The complete [ball-action coordinator](native-ball-action-reconstruction.md),
`0x1023cd20..0x10244c58`, is now installed in both signed ARM64 apps. Its 8,142
original instructions have 518,549 Release/UBSAN comparisons and the accepted
manifest has 678 entries, retaining every prior entry. Productive calls,
original/profile controls, nonlocal transfers and a complete paired match pass:
789 advances and 444 C++23 actions match 444 original controls with equal final
state and per-tick instruction accounting. The installed Match Debugger reaches
full time and retains undo/redo. The six contact dependencies now have complete
C++23 candidate bodies with 198,811 Release/UBSAN component comparisons covering
all 2,043 original instructions. They remain generated in the installed game
until production, real-match and performance acceptance is complete. Save/Load
was not retested for these deliveries.

## Original Mac media: observed facts

- Image: `CM-01-02.cdr`, 628,631,552 bytes, HFS+.
- Mounted disc inventory: 831 files; no C, C++, Objective-C, Swift, or assembly
  source files were found.
- Game data fork: 9,394,972 bytes; PowerPC 32-bit PEF, format version 1.
- Data-fork SHA-256: `91692f95df17cf851e7c1d6dbdecd0bd0a5809ce1937133e209aee859a6b516b`.
- Resource fork: 37,297 bytes; SHA-256
  `1f4ae4f499d43f163b9153026939b19a2ad022a8b4181690ba7b1b49cf5e551c`.
- Code/constant section: 9,124,484 bytes. This is not a pure instruction count.
- Three PEF sections; 259 imports in four library records: two CarbonLib records,
  Apple Carbon Multimedia, and Apple Carbon Networking.
- Zero exported symbols. This does not mean there are no internal functions.
- Ghidra 12.0, explicitly configured for `PowerPC:BE:32:default`, detected 5,833
  functions. Analysis completed without its time limit firing. This count is an
  analyzer result, not proof of complete function recovery.
- The original readme identifies Elegant Chaos and Sports Interactive as the Mac
  port authors and Feral Interactive as publisher. It documents manual folder-copy
  installation, but the copied PowerPC executable remains incompatible with ARM64.

The input image was already mounted read/write when work began. The tools created
here only read the supplied media; do not infer an original-distribution image
checksum from the current mounted copy.

## Match foul and referee outcomes reconstructed

The [match-discipline block](native-match-discipline-reconstruction.md) adds six
complete C++23 entries / 1,078 indexed instructions: contact and single-actor foul
outcomes, referee incident response and three match facades. The current
manifest is **655**, with the full 649-entry prefix preserved. Original card,
foul, rating, referee, clock, random-stream and reason effects retain their ABI,
ordered access and fault behavior. Both applications share these bodies.
Six focused CTests, seven source audits, 39 changed manifest checks, affected-unit
UBSAN and installed worker/UI acceptance pass. Signed ARM64 apps are installed,
with current saves and original files preserved. The report records per-entry
production and nonlocal evidence and the explicit diagnostic limits. This completes
this bounded block; the full match engine and whole game port remain unfinished.
Save/Load is **not retested**; performance is **not benchmarked**.

## Complete player movement and debugger cards delivered

The [complete movement/interaction root](native-player-movement-reconstruction.md)
adds one C++23 entry / 7,537 indexed instructions across eight original pages,
bringing that phase manifest to **649** with its 648-entry prefix preserved.
All source sites are covered in component comparisons; actual productive,
original/profile, negative and nonlocal checks pass. A real match observes
597 dedicated C++23 movement calls and returns without original fallback,
fault or dispatcher escape. Both ARM64 production apps contain the new root.
This completes that routine, not the whole match engine or complete game port.

The [debugger card controls](native-match-debugger-cards.md) add/remove yellow
and red cards, second-yellow dismissal, original pitch re-entry, visible counts,
football events and complete history controls. Original injury/dismissal
classification is corrected: the Injury control now invokes actual injury
processing without adding a red card. Focused engine, protocol, UBSAN and
installed acceptance are recorded in the reports. The user's current Reading
save and all original inputs are preserved. Save/Load is **not retested**;
performance is **not benchmarked**.

## Actor movement primitives reconstructed

The [actor-motion block](native-actor-motion-reconstruction.md) adds five complete C++23 entries / 462 indexed instructions: live coordinate/cell migration, heading selection/readback, turning eligibility and possession. That phase manifest is **648**, with its full 643-entry prefix preserved. All original body sites are covered in 427 component comparisons; individual actual direct/generated/CFM, original/profile and nonlocal checks pass, including leaf scopes. The real debugger lab and worker regressions pass. Only three generated pages change, retaining the reusable build tree. At that phase the larger coordinator remained original; its 25 whole-body and 129 computed-prefix characterization cases pass. This is progress toward the full engine, not completion. Save/Load is **not retested**; performance is **not benchmarked**.

## Complete player decision reconstructed

The [complete player decision body](native-player-decision-reconstruction.md)
adds one 2,090-instruction C++23 entry across three pages, bringing that phase
manifest to **643** with the full 642-entry prefix preserved. Named tactical,
pressure, marking, destination, keeper and outfield phases retain original guest
ABI, effects and dependency behavior. Ten focused CTests, source audits and
changed manifest checks pass; individual component/negative, actual
direct/generated/CFM, original/profile and nonlocal evidence are recorded in the
report. Both signed ARM64 apps are installed and their packaged-program worker
sequence is verified; disposable cleanup recovers 2.80 GB. The full movement/action engine and final visual acceptance remain
unfinished. Save/Load is **not retested**; performance is **not benchmarked**.

## Marking relations and local pressure reconstructed

The [marking/pressure block](native-match-marking-pressure-reconstruction.md)
adds 13 complete C++23 entries / 396 instructions, bringing that phase's manifest
to **642** with the full 629-entry prefix preserved. Marking-list maintenance,
tactics-selected traits, marker eligibility and weighted opposing-player
assessment now run in both production apps. Seven focused CTests, source audits,
34 changed manifest checks and affected-unit UBSAN runs pass. Every entry has
individual component/negative, actual direct/generated/CFM, original/profile and
nonlocal proof; actual guest candidate collection allocation/copy is compared.
Installed signed ARM64 identities and a real goal/withdrawal/following-step worker
sequence are verified. Full movement/action reconstruction and final visual
acceptance remain unfinished. Save/Load is **not retested**; performance is
**not benchmarked**.

## Prior spatial membership and tactical role reconstruction

The [spatial/role block](native-match-spatial-role-reconstruction.md) adds 23
complete C++23 entries / 754 instructions, bringing the current manifest to
that phase's **629** with the complete 606-entry prefix preserved. Role lookup, coordinate
and marking access, cell insertion/removal and both-side goalkeeper resolution
now execute the same semantic code in the game and debugger. Eight focused
CTests, original/source audits, 33 changed manifest assertions and affected-unit
UBSAN checks pass. All 23 entries have individual component/negative and actual
direct/generated/CFM, original/profile and nonlocal runtime evidence. Installed
signed ARM64 bundles and a real worker reserve/goal/keeper-withdrawal/replacement
sequence are verified. Remaining movement/football dependencies and the full
debugger product contract remain unfinished; final visual acceptance is pending.
Save/Load is **not retested** and performance is **not benchmarked**.

## Prior match actor construction reconstruction

The [complete actor-construction block](native-match-actor-construction-reconstruction.md)
adds two C++23 entries / 2,778 indexed instructions, bringing the current manifest
to that phase's **606** with the complete 604-entry prefix preserved. Preparation and player
initialization now run in both production apps, including the debugger roster
editor. Six focused CTests, pinned original/source audits, 32 changed manifest
assertions and affected-unit UBSAN checks pass. Both entries have dedicated
direct/generated/CFM runtime coverage, original/profile controls, own/child
negatives and nonlocal transfer proof. Installed signed ARM64 identities and an
actual packaged-program worker sequence are verified. Final visual control
verification awaits Mac unlock; Save/Load is **not retested** and performance is
**not benchmarked**. Remaining football dependencies, red-card dismissal and a
complete decoded commentary inspector remain unfinished.

## Prior match advancement reconstruction

The subsequent [match-actor block](native-match-actor-reconstruction.md) adds three
complete C++23 entries / 207 instructions for injury withdrawal and position
access, bringing that phase's manifest to **604** with the full 601-entry prefix
preserved. It passes 562 component comparisons, actual direct/generated/CFM and
original/profile controls, 21 nonlocal comparisons and real withdrawal/worker
checks. Both signed ARM64 apps are updated; final GUI verification awaits an
unlocked Mac. Goal injection and pre-kickoff lineup editing were subsequently
delivered. The advancement acceptance below records its prior phase.

Three complete entries / 1,940 indexed instructions now execute C++23 watched
orchestration, background advancement and the shared match advancement state
machine. The production manifest has **601** replacements with the accepted
598-entry prefix preserved. Existing football/RNG dependencies, original match
rules, random draw order, event/clock handling, floating rounding, completion
effects and guest ABI remain intact.

Seven focused CTests, fourteen boundary/reference audits, 31 updated manifest
assertions and four affected-unit UBSAN runs pass. Evidence includes 1,189
component comparisons, 967 fault/interrupted prefixes and 670 budget stops;
53 actual direct/generated/CFM comparisons, nine original-profile comparisons,
21 nonlocal comparisons including twelve restore/handler faults, and 24 actual
shared-engine steps agreeing with the captured original in complete guest state,
ordered effects and accounting. The fresh build has 2,057 objects and retains
the unchanged State header/layout.

The installed signed ARM64 executable SHA-256 is
`084136e6a0dd6835430ec520b24928149a5abb9db537665fe7aa24766874857c`.
Candidate/installed/running identity, original menu, quit confirmation and normal
exit pass in the 800×600 content window. Menu match counters are zero; individual
entry coverage comes from actual production checks. Original game files, current
runtime files, saves, references and private handoffs are preserved. Save/Load
is **not retested** and performance is **not benchmarked**. See
[the advancement report](native-match-advancement-reconstruction.md).

The separate [match debugger](native-match-debugger.md) has an ARM64 application/session core using the same production engine. Its [match lab](native-match-debugger-lab.md) shows elapsed time, score, actual reserves and committed football events together. Complete guest-state history supports rewind/redo, time jumps and branch edits; goals, injury withdrawal and unrestricted substitutions change actual engine state. Full-roster replacements, re-entry, more than three substitutions and injured-goalkeeper replacement are covered by focused checks. History shares unchanged pages in RAM and creates no per-step SSD captures. The existing Liverpool/Chelsea injury/history bootstrap regression remains covered through 100 actual calls. The manifest remains **643** entries; this lab delivery adds no semantic replacement. Red-card intervention, broader engine reconstruction and the complete port remain unfinished. See [the application report](native-match-debugger-application.md).

## Prior record-history insertion reconstruction

Two complete entries / 393 indexed instructions now execute named C++23
current-row history insertion and matching cached-record traversal. Generation
has **598** replacements with the accepted 596-entry prefix, roots, indexed
instructions, generated inventory and five fixups preserved. Actual byte exchange,
signed history bounds, marked-node selection, owned-field disposal, live row
reloads, callbacks, fallback arguments and guest ABI retain original behavior.

Nine focused CTests, 250 executed generator audits and thirteen storage tests
pass. Evidence includes 2,359 component comparisons, 2,307 interrupted/faulting
outcomes including 484 budget stops, two own negatives and a semantic-child
negative, 25 actual production comparisons, two root profile comparisons, an
application-exit prefix and 25 nonlocal comparisons including twelve
restore/handler faults. Seven affected UBSAN units pass four modes with exact
Release output. The complete fresh build contains 2,055 objects and retains the
unchanged State header/layout.

At that delivery, the installed signed ARM64 executable SHA-256 was
`96d8399080b94158afe410834f39b5b7d19e48d815c61e91b9ba2d2a97682723`.
Candidate/installed/running identity and menu/cancel/original-confirmation/normal-
exit smoke pass in the 800×600 content-sized window. Both entries execute with
three semantic calls/returns and zero own failures, original/profile selection
or dispatch escapes. Dedicated actual production checks cover their additional
branches. The newer external save, original files, inputs, references, 23 handoffs
and 216 prior bundles are preserved. Cleanup removes 4,128 prior reproducible
outputs and recovers approximately 1.36 GB, with approximately 49.39 GB available
after preservation checks. Save/Load is **not retested**, performance is **not
benchmarked**, and the full port remains unfinished. See
[the record-history report](native-record-history-reconstruction.md).

## Current-record queries reconstructed

Six complete entries / 253 indexed instructions now execute semantic C++23
argument publication, cache counting, first/last-node queries, unsigned node-mark
retrieval and payload presence. Generation has **596** replacements with the
accepted 590 records, roots, instruction/file inventory and five fixups preserved.
Live row/count/pointer reloads, original reporting dependencies, guest ABI,
signed/narrow fields and known nonlocal ownership retain original behavior.

Seven focused CTests, 243 executed generator audits and thirteen storage tests
pass. Evidence includes 496 component comparisons, 416 interrupted/faulting
outcomes including 196 budget stops, six own negatives, 54 actual production
comparisons, six profile comparisons, an application-exit prefix and 46 nonlocal
comparisons including 24 restore/handler faults. Seven affected UBSAN units pass
four modes with exact Release output; all 2,036 State-dependent objects are fresh
and the State header/layout is unchanged.

At the query delivery, the installed signed ARM64 executable SHA-256 was
`456b0195b0ebd66dbd23a4aaee28f4982db2b5492a0304b68aa55d6f49b4d559`.
Candidate/installed/running identity and menu/cancel/original-confirmation/normal-
exit smoke pass in the 800×600 content-sized window. One query executes once;
all six have dedicated actual production coverage and zero own smoke failures,
original/profile selection or dispatch escapes. Current saves, original files,
inputs, references, 23 handoffs and 212 prior bundles are protected. Cleanup
removes 4,126 old reproducible outputs and recovers approximately 1.42 GB; the
current consistent build remains, with approximately 50.64 GB available after
preservation checks. Save/Load is **not retested**, performance is **not
benchmarked**, and the full port remains unfinished. See
[the current-record query report](native-record-query-reconstruction.md).

## Record-group operations reconstructed

Eleven complete entries / 853 indexed instructions now execute semantic C++23
labels, owned/borrowed fields, bulk field updates, group insertion/removal, callback
publication and identifier swapping. Generation has **590** replacements with the
accepted 579 records, roots, instruction/file inventory and five fixups preserved.
Signed fields/types/counts, live owner/row reloads, guest ABI, actual dependencies
and known nonlocal ownership retain original behavior. The cross-page getter has
separately pinned 36/39-instruction fragments and a complete private reference.

Seven focused CTests, 236 executed generator audits and thirteen storage tests
pass. Evidence includes 6,002 component comparisons, 5,856 interrupted/faulting
outcomes including 3,664 budget stops, eleven own negatives, 99 actual production
comparisons, eleven profile comparisons, an application-exit prefix and 81 nonlocal
comparisons including 44 restore/handler faults. Seven affected UBSAN units pass
four modes with exact Release output; the State header/layout is unchanged.

At the record-group delivery, the installed signed ARM64 executable SHA-256 was
`be589e7f51dda32ccdf2746d69a3ba9d2d50181486a16715845730d35ac514c5`.
Candidate/installed/running identity and menu/cancel/original-confirmation/normal-
exit smoke pass: five entries execute with 271,437 semantic calls/returns and zero
own failures, original/profile selection or dispatch escapes. The other six have
dedicated actual production coverage. Current saves, original files, inputs,
references, 23 handoffs and 208 prior bundles are protected. Cleanup removes 4,120
old reproducible outputs and recovers approximately 1.41 GB; the current consistent
build remains. Save/Load is **not retested**, performance is **not benchmarked**,
and the full port remains unfinished. See [the record-group report](native-record-group-reconstruction.md).

## Aggregate-object lifetimes reconstructed

Six complete entries / 411 indexed instructions now execute semantic C++23
embedded-table and aggregate construction/destruction, record-group cleanup and
stream detachment. Generation has **579** replacements with the prior 573 records,
roots, instruction inventory and five source fixups unchanged. Guest field
layouts, signed counts/deleting flags, live callbacks and owner reloads, actual
allocator/array/CFM dependencies and nonlocal ownership retain original behavior.

Seven focused CTests, 229 executed generator audits and thirteen storage
regression tests pass. Evidence includes 4,006 component comparisons, 3,929
interrupted/faulting outcomes including 3,021 budget stops, six own negatives,
54 actual production comparisons, six profile comparisons, an application-exit
prefix and 44 nonlocal comparisons including 24 restore/handler faults. Seven
affected units pass UBSAN in four modes with exact Release output.

At the aggregate-object delivery, the installed signed ARM64 executable SHA-256 was
`86f34c78266f270d83b7f477322c635b38b417138ff68bf2f181c9a561c0d8df`.
Candidate/installed/running identity and the menu/cancel/original-confirmation/
normal-exit smoke pass. All six entries execute with ten semantic calls/returns
and zero original/profile selection, own failures or dispatch escapes. Current
saves, original files, inputs, references, 23 handoffs and 204 prior bundles are
protected. The prior build's 4,114 reproducible outputs are cleaned, recovering
approximately 1.47 GB; the current consistent build remains. Save/Load is **not
retested**, performance is **not benchmarked**, and the complete port remains
unfinished. See [the aggregate-object report](native-aggregate-object-reconstruction.md).

## Build storage and cycle cleanup

Generated C++ can now be seeded from a previous completed generation through
independent APFS copy-on-write files. All instructions/pages are still rendered
and audited before completion. Thirteen focused tests and a full private
regeneration pass: 1,923 cloned C++ files / 752,660,018 logical bytes produce
identical source and manifest hashes, while every original file retains its
identity and metadata. At that storage-only delivery, the accepted 573 semantic
entries and installed game remained unchanged. Earlier reproducible objects/test
outputs are cleaned after each accepted cycle; the latest consistent build and
active phase are retained.
See [build storage and retention](native-build-storage.md).

## Descriptor-tree mutation reconstructed

Three complete entries / 285 indexed instructions now execute semantic C++23
root insertion, recursive insertion/equal-key replacement and postorder cleanup.
Generation has **573** replacements with the prior 570 records, roots, instruction
inventory and all five source fixups unchanged. Guest links, live CFM callbacks,
signed comparison, original failure/count quirks, four expanded traversal levels,
deep recursion, caller ABI and nonlocal ownership are preserved.

Eight focused CTests, 222 generator audit tests without final skips/failures and
seven affected UBSAN units across four exact Release-equivalent runs pass.
Evidence includes 2,196 component comparisons, 2,101 faults, 1,073 exact budget
stops, three own negatives, 45 actual production comparisons, three profile
comparisons, an application-exit prefix and 23 nonlocal comparisons including
twelve restore/handler faults.

At this tree delivery, the installed signed ARM64 executable SHA-256 was
`7a4ed5b5b47583e4a44fc1d9188c751d52f0ac7230b8b7e44090dabb3505b675`.
Candidate/installed/running identity and the menu/cancel/original-confirmation/
normal-exit smoke pass: all three entries execute with 53 semantic calls/returns
and zero original/profile selection, own failures or dispatch escapes. Original
files, saves, inputs, references, 23 handoffs and 199 prior bundles are protected.
The previous cycle's reproducible build/test outputs have been cleaned; the
current consistent build is retained. Save/Load is **not retested**, performance
is **not benchmarked**, and the complete port remains unfinished. See
[the tree-mutation report](native-tree-mutation-reconstruction.md).

## Basic utility-object lifetimes reconstructed

Eleven complete entries / 239 indexed instructions now execute semantic C++23
node, list, buffer and descriptor lifetime operations in production. Generation
has **570** replacements with all 559 prior records and five source fixups
unchanged. Guest layouts, signed deleting flags, padding, aliasing, allocator
policy, CFM cleanup, the retained descriptor-root quirk and nonlocal ownership
are preserved. The tree algorithm and aggregate constructor remained generated at this delivery;
tree mutation is subsequently reconstructed in the report above.

Nine focused CTests, 26 affected/new Python suites / 174 tests without final skips
and seven-unit UBSAN across four runs pass. Evidence includes 991 component
comparisons, 843 faults, 399 exact budget stops, eleven own negatives, 66 real
production comparisons, eleven profile comparisons, an application-exit prefix
and 79 nonlocal comparisons including 44 restore/handler faults and both leaves.

At this utility delivery, the installed signed ARM64 executable SHA-256 was
`4b2fd9c202ef2293475e4f37a45b81ec0887e01787f5e4165b4490b22b0bc058`. Candidate/installed/running identity and the protected
menu/chooser/cancel/normal-exit smoke pass: 388 semantic calls and returns,
zero original/profile selection or own failures. Ten entries occur there;
list clear is covered by dedicated actual production checks. Original game/save/
payload/reference/handoff metadata and 194 older bundles are protected. Old
reproducible build/test outputs are cleaned after each accepted cycle while the
latest validated build is retained. Save/Load is **not retested** and performance
is **not benchmarked**. The full port remains incomplete. See
[the utility-object report](native-utility-object-reconstruction.md).

## Engine global initialization reconstructed

The startup root and its nineteen global initializers now execute named C++23
operations in production: twenty complete entries / 327 indexed instructions.
The root preserves original call order and actual constructor/destructor
dependencies. Complete generation has **559** semantic replacements, with the
preceding 539 records and all five source fixups unchanged. Owned opaque runtime
diagnostics keep future metadata additions independent of generated State layout;
this delivery rebuilt all 2,025 State-dependent objects consistently.

Ten focused CTests and 25 affected/new Python audit suites / 167 unittest tests
pass without final skips. The component matrix records 2,628 comparisons, 2,448
faults, 820 exact budget stops and twenty own negatives. Production checks record
124 direct/generated/CFM/global/entry/mixed comparisons, twenty profile checks
and 142 nonlocal comparisons including eighty restore/handler faults. Twenty-nine
UBSAN units pass four runs with empty sanitizer stderr and exact Release output.

Protected installed menu/chooser/cancel/normal-exit smoke executes all twenty
initializers once, with twenty semantic calls and returns and zero original,
profile, own fault, budget stop, transfer or escape. The signed ARM64 SHA-256 is
`0af6b5bf6bea154f6b25e00f49593fc0c7fa2948c810b1daa70f27e4f5e5809c`.
Game/save/payload/handoff/reference metadata and 190 older bundles are preserved.
The same build hides the original desktop backdrop and sizes the visible game
window to its scaled content; 800×600, 1000×750 and 1200×900 were verified at
100%, 125% and 150%. The prior 100% preference was restored before exit.
Save/Load is **not retested**; the full engine/runtime port remains incomplete.
See [the initializer report](native-engine-initialization-reconstruction.md).

## Framed native-service facades reconstructed

The complete AppleEvent, NewPtr and DisposePtr facades now execute semantic
C++23 service calls in production: three entries / 27 indexed instructions.
Complete generation has **539** semantic replacements with all accepted 536
records unchanged. Caller frames, live TOC/link restoration, word32 arithmetic,
existing native allocation policy and callback/nonlocal ownership are preserved.

Twelve focused CTests, 24 affected/new Python suites / 160 tests with zero final
skips, and ten-unit UBSAN across six runs pass. The facade harness records 198
component and 213 production comparisons, 57 exact budget stops per mode,
individual negatives and direct/generated/CFM evidence for every entry. Another
96 mixed nonlocal comparisons include 48 restore/handler faults; actual AppKit
launch dispatch executes the original callback once in both selection modes.

Protected installed menu/chooser/cancel/normal-exit smoke passes. All three new
entries execute there with 185 semantic calls/returns and zero original/profile
selection, own fault, budget stop, transfer or escape. The installed signed ARM64
SHA-256 is `dd0d8a6e67eba0165719e2acd19ba554bc6412735e16157f1ba096335e8b90bd`.
Game/save/payload/handoff/reference protection and 185 older bundles pass.
Save/Load remains **not retested**; full engine/runtime reconstruction is still
incomplete. See [the service-facade report](native-service-facades-reconstruction.md).

## Import descriptors and generic CFM tails reconstructed

All 259 Mac import glues and the generic CFM descriptor tail now execute named
semantic C++23 operations in production: 260 complete entries / 1,559 indexed
instructions. Live guest descriptors, word32 pointers, caller/callee TOCs,
function publication order and frame/dispatcher ownership retain the original
contracts. Complete generation has **536** semantic replacements with the
accepted first 276 unchanged. Nine focused CTests, 23 affected Python suites /
153 tests with zero skips and nine-unit UBSAN pass. The new harness records
11,434 component comparisons, 8,580 production comparisons, 260 own negatives
per mode and positive direct/generated/native/guest evidence for every entry;
all 259 imports also execute through the generic CFM tail in composition.

Protected installation and installed menu/chooser/named-cancel/normal-exit smoke
pass. The signed ARM64 executable SHA-256 is
`7d78391d80e68f4de4e911cf6c66c17dfa3dcf7309c6b8a29e8062cbd4ccc452`.
Installed diagnostics record 37,360,301 semantic calls/tails across 115 entries,
including 5,442,294 generic tails, with zero original/profile/budget selection,
own fault, budget stop, context transfer or escape in this block. The other 145
entries are covered by the harness. Game/save/payload/handoff/reference content
and 180 older bundles are preserved. Save/Load is **not retested**. Remaining
native services and engine/runtime reconstruction keep the complete goal active.
See [the descriptor-tail report](native-import-descriptor-reconstruction.md).

## Word utilities and comparator search reconstructed

Four complete routines / 90 indexed instructions now have semantic C++23
implementations in default production: the two absolute-word facades,
floor-style signed remainder and comparator binary search. Complete generation
has 268 semantic replacements with the accepted first 264 unchanged. Component
acceptance has 4,095 comparisons, 569 exact budget stops and four individual
negatives. Production has 742 comparisons, 585 exact budget stops, two mapping
faults and four individual negatives, including real guest string-comparator
composition. Thirty-two nonlocal search cases / sixteen restore-handler faults
preserve abandoned callbacks and nested search frames. Six focused CTests,
21 affected Python suites / 136 tests with zero skips, and twelve-unit UBSAN
with output identical to Release pass.

Protected installation and installed menu/chooser/named-cancel/normal-exit smoke
pass. The ARM64 executable SHA-256 is
`1f08d21e93d6c1be300a244a3a081db4e91eed7d32cf794425a0fdcfcd484bde`.
Installed counters record seven absolute-long-word and 63 binary-search calls/
returns with zero original selection, continuation, fault or escape. The other
two UI roots have zero calls; their coverage is the harness. Game/save/payload/
handoff/reference content is preserved. Save/Load is **not retested** and the
complete port objective remains active. See
[the word-utility/search report](native-stdlib-core-reconstruction.md).

## Register context reconstructed

All thirty complete entries for f18..f31 suffix save/restore and scalar nonlocal
capture/restore run in semantic C++23 production. Four shared bodies contain 91
unique indexed instructions. At that delivery the manifest was **264**, preserving its first 234
exactly. Component acceptance has 2,883 comparisons / 756 mapping-fault prefixes /
30 individual negatives. Production adds 1,169 comparisons, 598 exact budget
stops, sixty mapping faults, thirty negatives and direct/generated/CFM evidence
for every entry. Eighty nested original/semantic/mixed cases prove surviving and
abandoned host continuations, including shared frames, recursion, copied contexts
and peer-State isolation. Ten focused CTests, fifteen affected Python suites /
101 tests with zero skips and eleven-unit UBSAN pass. See
[the register-context report](native-register-context-reconstruction.md).

The protected installed ARM64 executable SHA-256 is
`05f50e3350ac9244a0796093ddf186e7171f3c5a7e34c954e4d7741e162a42f1`.
The installed menu/chooser/cancel/normal-exit smoke records 337 captures and one
restoration, with zero original selection, fault, escape or unknown transfer.
The other 28 UI entry counts are zero; their individual coverage is from the
harness. Protected game/save/payload/handoff/reference content is unchanged.
Save/Load is **not retested**; the complete port objective remains active.

## Compiler arithmetic reconstructed

All eight complete compiler-arithmetic routines / 412 indexed instructions run
named C++23 operations in default production: signed/unsigned 64-bit quotient
and remainder, plus integer/floating conversions. At this earlier delivery the manifest expanded to 234
entries with the accepted first 226 unchanged. Complete generation retains
1,953,334 instructions, 1,922 pages and zero missing/pending targets. All units
use the expanded State layout in a fresh complete Release ARM64 build.

13,199 component comparisons include 77 mapping-fault prefixes, two enabled
conversion traps and eight rejected negatives. Production adds 7,422 comparisons,
4,895 exact budget stops, ten mapping faults and eight individual negatives;
all eight have normal native/generated/CFM calls with zero original selections
and escapes. Sixteen separate profile comparisons retain actual original-PC
samples. Unsigned quotient keeps its exact full-call charge; the other seven
use semantic entry charges and original execution for tight budgets/profiling.

Ten focused CTests, 20 audit suites / 127 tests with zero skips, and fresh ten-unit
UBSAN pass. One protected installation preserves saves/payloads/handoffs and
159 earlier private bundles. Installed menu/chooser/cancel/normal-exit smoke
passes; unsigned quotient has 12,125,476 modern calls/returns with zero faults,
original selections or escapes, while the other seven UI counters are zero.
Internal ARM64 executable SHA-256:
`6814c65faad2615a547a5d3ce97a3b6f840ec20c58bd09c7cac8b7a76ae3beb3`.
Save/Load is **not retested**; no benchmark or whole-port completion is claimed.
See [the arithmetic report](native-compiler-arithmetic-reconstruction.md).

## Exception runtime reconstructed

All 21 routines / 1,657 indexed instructions now run named C++23 operations in
default production: action/frame traversal, saved registers, cleanup, encoded
type/specification matching, search/catch installation, scalar/vector capture,
context transfer, exception objects and termination/unexpected callbacks. The
interior unexpected landing continuation belongs to its original root. The
manifest has 226 entries, preserving the accepted first 205 exactly. Read-only
recovery added 177 previously unindexed instructions while preserving every
prior record and initialized payload; complete generation has 1,953,334
instructions, 1,922 pages and zero missing/pending targets.

Component comparisons preserve full architecture, guest bytes, ordered fault
prefixes and callback/import effects. Production acceptance adds 79 comparisons
and 23 root/landing/child negatives: all 21 normal direct, generated CFM-indirect
and diagnostic-original roots; independent child selection/reset; separately
counted unfamiliar transfers; six actual throws into the unexpected landing;
and native termination. Guest transfers abandon interrupted host continuations
and retain state/function/frame ownership, including recursion and array cleanup.

Twelve focused CTests pass, with related object-storage, fragment-metadata and
lifecycle checks rebuilt against the consistent expanded State layout. Twenty
Python suites pass 125 unique test functions, including the recovered full-export
local-temporary audit. Fresh UBSAN covers 29 units and nine executables, all with
empty stderr and output identical to Release. See
`native-exception-runtime-reconstruction.md` for per-component counts, pins,
undefined vector-lane limits and the exact acceptance scope.

Protected installation verified source/candidate/installed/running ARM64
executable SHA-256
`8454b2568e865c76675c4fa73ca5601bdd4cb910366bc947ae0fdc18e4d592a5`.
Prepare Game, named Load Game chooser/cancel and original Quit/confirmation
completed normally; owned PID 52542 exited and logged 21,593,344,557 recompiled
instructions. The new exception counters stayed zero in the installed UI; their
per-entry runtime evidence is the production harness. Game, existing saves,
four payloads, 23 handoffs, previous references and 154 historical bundles are
preserved. Logs/preferences may change. Save/Load is **not retested**; performance
and gameplay campaigns were not run for this block. The full port remains active.

## C++ object-array lifetime and storage reconstructed

Twelve production routines now use named C++23 bodies: cookie/raw array
construction and destruction, partial-construction cleanup, object/array
allocation and release, enabled allocation failure, failure-object destruction
and its type metadata. The complete bodies contain 335 original instructions.
The manifest contains 205 entries with the accepted first 193 unchanged;
generation retains 1,953,157 instructions, 1,922 pages and zero missing/pending
targets. Full bodies, interior-flow constraints and destination hooks are pinned;
the full Release rebuild uses a consistent State layout for all generated units.

Guest cookies/guards, ascending constructors, reverse destructor order, live
counter/stride/callback rereads, signed deleting flags, word32 wrap and ordered
vtable publication are preserved. Allocation delegates to the existing semantic
allocator and native heap services. The new-handler owner is captured once and
its callback reloaded after failure. ApplicationExit is preserved as a successful
nonreturning boundary. Selection, observers and counters remain private.

171 production comparisons passed, including 36 expected mapping faults, two
real native terminations and 22 own/child negatives. All twelve have normal
direct/CFM evidence and independent reset; parent/child mixed selection is tested
in allocation/release chains. Default cases have zero original calls and escapes.
Native allocation/construction/destruction/
reuse/release composition preserves payload, padding and guest address reuse.
Four directly related CTests, seventeen audit suites (102 tests), and UBSAN of
modified implementation/dispatch/harness code passed.

One protected installation delivered source/candidate/installed/running ARM64
executable SHA-256
`6d9ed067922846327fac9fc294bac76feb15acb932cd8a403c4a0322a1913647`.
Prepare Game and original named Quit/confirmation completed normally; owned
process 63146 closed and logged 4,977,661,890 recompiled instructions. Eight
entries executed in the installed UI, including tens of thousands of object/
array allocation/release calls; the harness covers all twelve. Game, existing
saves, four payloads, 23 handoffs, the previous reference and 151 older bundles
are preserved. Logs/preferences may change.

The exception matcher, throw/unwind machinery, nonlocal contexts, startup, heap
glues and native heap bridges remain outside this increment. Enabled failure
with no matching fragment follows the original termination path; successful
catch/unwind is not claimed. The full modernization goal remains active.
Save/Load: **not retested**; performance: **not benchmarked**. Contracts, pins and
evidence are in [the object-storage report](native-cpp-object-storage-reconstruction.md).

## C++ code-fragment registration and exception metadata reconstructed

Five production helpers now use named C++23 bodies: code-fragment registration,
cached-region lookup, exception metadata resolution, saved-frame-word reading
and action-kind reading. The bodies contain 301 original instructions. The
manifest contains 193 entries with the accepted first 188 unchanged; generation
retains 1,953,157 instructions, 1,922 pages and zero missing/pending targets.
The full Release rebuild uses a consistent State layout across every generated
unit. Body/index/interior-flow contracts and destination hooks are pinned.

The guest 32-slot linked blocks, once-only native capability probe, allocation,
cache asymmetry, signed binary search, inclusive function/action ends, inline
descriptors and three action formats retain original ABI, scratch and live alias
effects. Native Gestalt/NewPtrClear and guest heap ownership remain the existing
dependencies. Selection, observers and counters are private and unpersisted.

165 production comparisons passed, including 23 expected mapping faults and
six own/child negatives. Every entry has normal direct/CFM evidence; default
cases have zero original calls and escapes. A five-stage composition registers
through real native services and uses the resulting metadata in all four query
helpers. Three directly related CTests, sixteen audit suites (95 tests), and
UBSAN of modified implementation/dispatch/harness code passed.

One protected installation delivered source/candidate/installed/running ARM64
executable SHA-256
`cf03e10179523d539f03ff222af074560c82eee58f1cfd84cd18b14acdb832b3`.
Prepare Game and original named Quit/confirmation completed normally; owned
process 29445 closed and logged 4,532,133,418 recompiled instructions. The UI
called registration once; the harness covers all five. Game, existing saves,
four payloads, 23 handoffs, the previous reference and 148 older bundles are
preserved. Logs/preferences may change.

This increment does not reconstruct startup, exception execution/unwinding,
nonlocal contexts or heap glues. The full modernization goal remains active.
Save/Load: **not retested**; performance: **not benchmarked**. Contracts, pins and
evidence are recorded in [the metadata report](native-code-fragment-reconstruction.md).

## Runtime shutdown, global destructors and signals reconstructed in C++23

Six production entries now use named semantic C++23 bodies: global-destructor
registration/traversal, abort, exit, reverse exit callbacks/native termination
and raise. They contain 145 original instructions. The manifest contains 188
entries with its accepted first 182 unchanged; generation retains 1,953,157
instructions, 1,922 pages and zero missing/pending targets. Complete bodies,
exit page fragments, interior-flow restrictions and destination hooks are pinned.

The live guest lists/table, unlink/reset-before-callback order, signed guards,
low-word arithmetic, Mac ABI/fault prefixes and callback-driven mutation are
preserved. The adapter distinguishes a successful return from the existing
nonreturning ApplicationExit unwind. Production uses the native ExitToShell
bridge; attached dependency-return fixtures are private. Original mode,
selection/reset/counters/observers remain process-local and unpersisted.

128 complete-state/effect comparisons passed, including 28 expected mapping
faults, 45 native terminations and 19 own/child return/termination negatives.
All six have normal direct/CFM witnesses, with zero original selection/escapes
in default cases. Seven focused CTests, fifteen audit suites (88 tests), and
UBSAN of modified implementation/dispatch/harness code passed. The full Release
rebuild compiled all 1,922 generated units consistently after State changed.

One protected installation delivered the signed source/candidate/installed/
running ARM64 executable with SHA-256
`fdcd54f2fdb4386a8d9dd2350491c0e15cd071c9964cbae031060e70533be34e`.
Installed Prepare Game and original named Quit/confirmation completed normally;
owned process 90304 closed and logged 4,280,759,117 recompiled instructions.
This UI covers four entries; the harness establishes all six. Game, existing
saves, original payloads, 23 handoffs, the previous installed reference and
145 older bundles are preserved. Logs/preferences may legitimately change.

Startup/root initialization, nonlocal contexts, heap glues, signal installation
APIs and the broader C/C++ runtime remain outside this bounded increment. The
complete modernization goal remains active. Save/Load: **not retested**;
performance: **not benchmarked**. Contracts/pins and evidence are recorded in
[the lifecycle report](native-runtime-lifecycle-reconstruction.md).

## Working native code

`CMAssets` and `cm-assets` compile into a Mach-O ARM64 executable. Four deterministic
test groups cover byte order, index boundaries, path rejection, stale image masks, RGB555
channels, truncated images, and excessive dimensions.

The real disc pass reads 22 index entries and validates the fixed-record boundaries
of the 21 present tables. `staff.dat` has zero records and is absent on this disc.
The inspector decodes all nine root `.rgm` images. The descriptor bytes at 12...47
are obsolete: eight files retain Windows-style mask words and `feral.rgm` has
other values. The actual Mac payload uses big-endian QuickDraw RGB555 pixels.
All pixels across the nine files have the high bit clear; white is stored as
`0x7fff`. Visual checking caught and corrected an initial stale-mask interpretation
that tinted whites pink. The output also tracks unsupported images explicitly.

The `.dam` index uses an eight-byte prefix and 68-byte entries with big-endian
numeric fields. Present table records start at byte zero and end with the four-
byte `0xf1fffff1` marker. Earlier inspection inferred a prefix from file sizes;
club IDs and the original reader's seek/read confirm that the extra word is a
trailer. Relevant observed
record sizes differ from the existing Windows definitions:

| Record | Mac bytes | Existing Windows bytes |
| --- | ---: | ---: |
| Index entry | 68 | 67 |
| Player | 72 | 70 |
| Club | 584 | 581 |
| Staff history | 20 | 17 |

Sizes establish structure only; individual Mac fields have not yet been mapped.
RGM descriptors retain little-endian Windows mask words despite their big-endian
Mac dimensions and pixel payload. Do not use those stale masks to interpret pixels.

## Existing Windows reconstruction

The user reports having Ghidra-decompiled Windows code. A local candidate was found
in a Google Drive-synced `recm-0102` folder.
The exact preferred source location is still awaiting the user's reply.

The candidate includes original decompilation snapshots, Ghidra exports, and a
substantial C++23 reimplementation. Its README describes 9,243 decompiled functions;
that historical metric has not been re-counted in this investigation. Its roadmap
explicitly describes the main day-advance loop as orchestration with unfinished
subsystem functions, not an end-to-end runnable game.

Current verification against those files, without editing the source project:

- CMake configuration targeting ARM64 succeeds.
- Full `engine` compilation fails real 32-bit pointer-layout assertions. Example:
  `si::ListNode` expects 14 bytes and measures 26 bytes with 64-bit pointers.
- The standalone database-structure test compiles as ARM64 and passes 58/58 checks.
- A date-subsystem build probe reaches an unused-function warning in `langlib.cpp`
  under `-Werror`; omitting the language module instead leaves a real unresolved
  symbol. Neither probe is recorded as a passing date-engine port.
- `rng.cpp` still includes Windows time APIs. `MappedFile` uses Windows handles and
  mapping APIs. These need actual platform implementations.

## Next implementation boundary

An additional route now has working code: [static recompilation of the original
Mac executable](aot-port.md). Five original routines pass 297 ARM64 cases against
a build-time PowerPC oracle and independent arithmetic expectations. The compiler
has generated C++ for all 1,953,048 analyzed instructions; this still does not prove
a complete game. Native OS services and indirect control-flow coverage must be
validated through real startup and gameplay. The Windows route below remains
available as a source of semantic references.

The entire analyzed listing builds as ARM64. Native startup reads the original
language database and fonts, updates menus, restores private printing preferences,
delivers the original launch callback, and creates two native windows. The original
background paint callback runs; original-code rendering now shows opening logos
and the Game Settings interface.
RGB555 pixel buffers, color conversion, overlapping copies, native cursor resources,
and private file writes now have checks. The supplied executable and resource fork
still match their audited SHA-256 values.

The installer now copies 809 original game/Extras files into user Application
Support and installs an ARM64 app in `~/Applications`. Resource forks, Finder
metadata, and file checksums are verified. Updates preserve private preferences
and runtime files. The original opening and main menu were physically observed
with the image unmounted; the user navigated the new-game league selection.
Installing the original `Extras/qpr for the cup` also satisfies the original
CD check through the local logical volume, without replacing the check. After
the user dismissed the prompt, database loading reached "Iniciando Ligas".

That run stopped at the original `fctiwz f2,f6` instruction at `0x1022cd24`, an
exceptional floating-to-integer conversion missing from the translation. The
log records 109,166,643,823 instructions and 18,338,941 native calls. A separate
quit attempt reached the missing `DisposeCCursor` bridge. Private file deletion
now uses the native File Manager, including resource forks and busy-file errors;
deleting original media entries still requires unimplemented tombstones.
Both original opening sounds have native rendering/completion evidence, but
audibility remains unverified. At that stage, day advancement, matches,
and save/reload were still unverified. The conversion now follows documented PowerPC
rules; 96 edge/mode cases execute the original failing instruction. All eleven
CTest groups pass, including the 297 original-routine comparisons. The updated
installed app reopened at the main menu without mounted media. The user's next
new-game attempt passed the conversion boundary, completed league initialization,
and reached the original "Escolha Nome" trainer form, observed on the physical
display. The user then advanced to the Brazilian team list, where background-image fallback caused
a null access at `0x100d53d8`. The original caller deliberately supplies a null
club for its default picture but eagerly fetches an unused club descriptor first.
A guarded source correction at `0x10713aec` skips only that null lookup and keeps
the original picture selection. All twelve CTest groups now pass, including
nonempty default picture lists, preserved RNG consumption, nonnull descriptor
lookup, and checks that unrelated null accesses still fail. The updated ARM64
app passed team selection: the user assumed control of Cruzeiro, and the retained
frame from `Runs/app.rCcF8C` shows the original squad screen. Opening a player
then stopped at the missing `CarbonLib::num2dec` service (`0x10bf7b10`, caller
`0x1077a070`); a second attempt, `app.tCEQVP`, reproduced that same boundary.
The native decimal bridge now marshals the original floating argument, format,
and 42-byte output to Apple's ARM64 function. It preserves rounding settings,
unused record bytes, and the original documented exception behavior. Tests cover
1,064 ABI cases and 14 executions of the original formatter. The user has now
opened a player in the recovered game. The first save attempt
revealed a canonical filename mismatch in private league temporaries, followed
by a missing final rename operation. Both filesystem boundaries are implemented
and all 13 test groups pass. The original rename wrapper completed the recovered
`Mac-Teste.sav` with unchanged bytes/forks/metadata. Its first load failed with
`INDEX.CPP 5773`: the original English language table misorders one club, shifting
the names used by saved database IDs. A separate copy with only that language
block aligned, `Mac-Teste-Recuperado.sav`, loaded successfully in `app.BAEOk6`;
the Cruzeiro squad was observed and the user confirmed that player pages open.
Opening the competition then failed at `0x100d5084`, another eagerly evaluated
null background descriptor. The exact call at `0x10713b3c` is now guarded;
club and competition default selection, RNG state, nonnull getters, and unrelated
null faults pass the original-routine tests. All 13 CTest groups pass after this
change. The user confirmed competition navigation in `app.esyws0`. A normal save
as `Mac-Teste-Validado` in `app.VDk5TM` and its successful load in `app.pigLIp`
are also user-confirmed; the new file needed no repair and all 118 compressed
blocks pass boundary/length validation. `Continue Game` initially stopped at
`0x101355d4`, where the original code truncates a fractional divisor to zero.
The new focused test reproduces that stop; a local lowering now preserves the
nontrapping PowerPC continuation where the undefined quotient is provably unused.
Its 96 arithmetic/RNG cases pass. In `app.cwxsEo`, the updated game advanced
from 21 December 2001 to the interactive news screen on 3 January 2002, then
entered a friendly against Anapolina on 4 January. The user confirmed normal
advancement and reported a stop during the match. The retained match frame shows
minute 4, score 0–0; the log identifies another zero divisor at `0x1024773c`.
The correction subsequently passed live in `app.yPRT55`: the user confirmed
completion, and the retained frame shows Cruzeiro 1–0 Anapolina at minute 93,
with "Final Jogo" displayed. Continuing from full time then triggered an audio
callback during the original sound teardown, after its PCM/header had been freed.
The new match correction passes 540 original arithmetic/quotient cases and seven
compiler-proof test groups. All 17 CTest groups pass in 38.52 seconds; the updated
match arithmetic and the sound transition are now live verified.
The audio scheduling correction reproduces and fixes the teardown failure using
the actual original cleanup routine in four variants. All 18 CTest groups pass
in 38.82 seconds, and the matching ARM64 build is installed. The added memory
bounds test checks 13,823 mapped-byte cases; no memory optimization is applied
yet. The user confirmed return from full time in `app.pis6iu`, saved as
`Mac-Teste-Validado-Pos-Jogo`, and exited normally. The save's 120 compressed blocks
validate and a private backup is retained. Reloading that latest save is pending.
The latest installed build also reports an explicit unsupported-network error
instead of throwing at initialization. Multiplayer is not implemented.
The day-advance arithmetic correction and normal-exit bridges are now installed.
All 16 CTest groups pass in 26.94 seconds. Shutdown tests execute the original
cursor initialization/cleanup routines three times using the supplied resources,
verify Resource Manager handle reuse and released allocations, and check that
`ExitToShell` unwinds nested callbacks to a successful host exit. Normal UI quit
is now live verified in `app.pis6iu`: its log records successful exit after
47,838,604,786 original instructions, without the diagnostic stop alert.
The optional AddressSanitizer build succeeded, but its runtime
deadlocked during initialization before entering the test; that check is
inconclusive and is not counted among passing tests.
Current evidence is in `reports/generated/aot-status.json` and the detailed
[port notes](aot-port.md). The Windows-specific possibilities below remain reference
options; current implementation proceeds through the original Mac program.

1. Establish the preferred ReCM source snapshot and its version; preserve the
   original source and decompilation artifacts.
2. Separate the Windows injection ABI from native runtime types. Preserve explicit
   32-bit serialized IDs/offsets and use native pointers only for runtime objects.
   Do not simply disable layout assertions or truncate ARM64 pointers.
3. Port time, filesystem, graphics, input, audio, and networking boundaries while
   keeping the already-recovered engine semantics and regression tests.
4. Map the Mac `.dam` fields or deliberately adopt a verified Windows data source.
   Mac records must not be reinterpreted as packed Windows structs.
5. Recover and implement the missing startup, simulation, match, UI, and save paths.
6. Validate new-game creation, a playable match, day advancement, and save/reload
   on ARM64. Compare recovered logic against trusted original-game outputs.

A native managed game, player/competition screens, normal save/reload, day
advancement, a full match simulation, return from full time, a post-match save,
and normal exit have run. The user also confirmed reloading the post-match save
and finding the completed Anapolina result in Last Game in run `app.oXBAh4`.

## Completed milestone: season reference and regression foundation

As of 2026-10-01, the user has replaced performance optimization with the
[season reference milestone](native-season-reference.md). Complete a real native
season, verify persistence on both sides of its annual transition, and establish
a bounded repeatable simulation comparison before semantic modernization.
Existing experiments and private data are preserved. New optimizations,
features, rules, and graphics changes are suspended for this increment.
The real application has completed 2002 and reached the ongoing 2003 edition.
Normal Save/exit/reopen passed on both sides, and exact bounded comparisons of
one and seven full days pass equivalent executions and reject a known RNG
divergence. Inspected titles, division changes, player history and contract/
transfer fields are documented separately from unexercised paths. The validated
ARM64 application is installed, with every installed Game/Runtime file and its
Mac metadata preserved. Calendar/date arithmetic is the first proposed semantic
replacement for the next increment, backed by 1,232 original-routine
checks; no semantic replacement is included in this milestone.

## Accepted implementation sequence

As of 2026-10-02, the user has selected calendar/date arithmetic as the first
semantic reconstruction increment. Its executable handoff is the
[calendar reconstruction goal prompt](features/native-calendar-reconstruction-goal-prompt.md).
The season reference milestone above is complete. The modern calendar module and
production entry hooks are implemented. Direct equivalence, original/modern
one- and seven-day simulation comparisons with negative controls, and the full
30-group suite have passed. Month advancement and preannual Save/Load were
observed in the app. Normal advancement also crossed into 2003; postannual
Save/exit/reopen/Load preserved the checked dates, club, player contract/history,
fixtures and Mineirao state. The validated ARM64 app is installed, with all
809 Game / 9 Runtime / 41 Runs files and their captured Mac metadata unchanged.
The [calendar reconstruction report](native-calendar-reconstruction.md) records
production call counts, delivery identities, commands and the bounded scope.
Later subsystem ordering remains open and should follow
the contracts and behavior comparisons established by each increment.

First produce and verify the playable native ARM64 game through static recompilation
and native OS bridges. Then recover clear names, types, and responsibilities and
replace generated low-level C++ incrementally with maintainable C++ modules, using
behavior comparisons before and after each replacement. C++23 compilation already
works; semantic modernization is a separate stage. The ReCM reconstruction remains
a reference, with its Windows-specific layouts and incomplete subsystems accounted
for. Its engine is not linked into the current Mac prototype.

Historical performance work ran alongside functional porting. Measure real startup and
league initialization before selecting changes; prioritize redundant work,
filesystem overhead, and diagnostic logging without changing game semantics.
Larger simulation optimizations need a working new-game/day/save baseline and
behavior comparisons. Correctness and performance evidence remain separate.
The first measured change replaces quadratic filename merging with an ordered
index using the same native comparison rules. Median isolated volume-query time
fell from 36.0 ms to 7.7 ms against the installed data (nine warm samples per
build). Eleven CTest groups pass; complete game-loading speedup is not yet measured.

## References

- [Apple: PEF structure and PowerPC runtime](https://developer.apple.com/library/archive/documentation/mac/pdf/MacOS_RT_Architectures.pdf), chapter 8.
- [Apple: porting an application to Apple silicon](https://developer.apple.com/documentation/apple-silicon/porting-your-macos-apps-to-apple-silicon).
- [Apple: video pixel formats](https://developer.apple.com/documentation/coremedia/video-pixel-formats), including 16BE555.
- Original disc readme, preserved locally by `tools/audit_media.py`.
- Existing ReCM `AGENTS.md`, `CLAUDE.md`, `src/cpp/CMakeLists.txt`, and current source files.

## Semantic date difference delivery — 2026-10-02

Future work, explicitly deferred by the user: investigate correcting the
ascending-year leap quirk after assessing caller dependencies and gameplay
effects. The [deferred improvement](native-date-difference-reconstruction.md#deferred-improvement--2026-10-02)
requires a separate decision and validation; the delivered behavior remains
compatible with the original.

Mac date difference `0x10718130` now uses the C++23 calendar module and legacy
adapter in production. Its second operand is an eight-byte by-value record,
including three padding bytes. The ascending-year leap quirk, initialization,
ordered scratch writes, ABI and stack residue are preserved. Independent date
comparison/conversion and other engine subsystems are unchanged.

The [date difference reconstruction report](native-date-difference-reconstruction.md)
records 6,892 direct equivalence cases, 494 independent-domain oracle checks,
31/31 CTest groups, Python/export audits and UBSAN. One- and seven-day immutable
fixtures passed strict original runs, the delivered addition/subtraction baseline,
two modern runs each and all RNG/shift/difference negative controls. Only the
existing instruction-accounting difference is permitted; no state masks were added.

A single isolated ARM64 app advanced normally from 19 July 2002 night to
3 August afternoon. The advancing run logged 500,953 semantic difference calls,
with 32 auditable argument/result samples and no original fallback. Calendar,
player/contract information and Save/exit/reopen/Load were checked before and
after the month boundary. The validated app is installed with its prior
executable backed up privately. Current before/after inventories agree for all
809 Game / 5 Runtime / 42 Runs files, including captured Mac metadata.

This establishes a bounded semantic increment, not full-career or historical
hardware equivalence. The previous calendar/annual report remains separate
historical evidence. No matched speed benchmark or new optimization is claimed.

## Semantic independent date ordering delivery — 2026-10-02

Mac date-ordering predicate `0x10331860` now executes the C++23 calendar module
through the legacy adapter. It returns exactly whether the second by-value date
is strictly later, preserving unsigned years, signed days, complete padding
spills, aliasing and architectural clobbers. Production dispatch covers the
actual same-page caller, other pages, indirect calls and callbacks. Addition,
subtraction and difference remain delivered; the deferred leap quirk is unchanged.
Other independent/inlined comparisons and larger date-record routines remain
outside this increment.

The [independent ordering report](native-date-ordering-reconstruction.md) records
5,646 direct original/modern cases, 156 Gregorian oracle cases, 196,608 pure
signed-day checks, 32/32 CTest groups, 26 Python tests with four optional groups
also executed explicitly, temporary auditing and focused/generated UBSAN.
The old day/week fixtures passed strict original and modern/baseline comparisons,
with zero ordering calls reported honestly. A new entirely original 20–21 July
morning fixture exercises 38,051 modern ordering calls in each of two equivalent
candidate runs; an isolated flipped return fails at its first ordering boundary.
Existing RNG/shift/difference controls and original fixture/stream hashes remain
preserved. Only translated instruction accounting may differ.

The isolated ARM64 candidate advanced normally from 3 to 10 August afternoon.
Contract-expiry sorting logged 38,422 modern orderings; 32 live samples agreed
with independent date ordering. Calendar, player/contract panels and normal
Save/exit/reopen/Load passed before and after advancement. The report separates
these UI observations from automatic equivalence and records excluded capture
attempts. No annual UI crossing or matched speed benchmark is claimed.

The candidate is installed. Source/installed executable SHA-256 is
`b61765e722c6e5f7624550ac15524fcdb623f933e9306c3329644310a887f545`;
the signed isolated candidate is
`01395e5f2e345635f414390dc89d8816103ef68dc5af9490a4ee538fd46a227c`.
The prior installed ARM64 app remains backed up privately. All 809 Game / 5
Runtime / 42 Runs files and inventoried metadata match before/after installation;
personal saves, original data and the immutable LFS reference remain unchanged.
This is a bounded semantic increment, not reconstruction of the remaining engine.

## Semantic independent date-before delivery — 2026-10-02

Independent entry `0x103317f0` now executes the C++23 predicate for second date
strictly earlier than first, preserving unsigned years, signed days, complete
by-value spills, aliasing and its distinct first-day r5 clobber. The actual
same/inter-page callers, indirect dispatch and callbacks reach the destination
hook. The four previous contracts and deferred ascending-year leap quirk remain
unchanged; other comparisons and date/engine subsystems remain outside scope.

The [date-before report](native-date-before-reconstruction.md) records 5,646
direct comparisons, 156 Gregorian oracle cases and 196,608 signed-day checks
for each predicate; 33/33 CTest groups, 27 Python tests with all four optional
groups also run explicitly, temporary auditing and generated/focused UBSAN passed.
All linked State-dependent objects use the final layout. Immutable day/week
fixtures passed with zero before calls. The original ordering fixture passed
two candidates and all three baselines, with 12,911 modern before calls per
candidate; its inverted return failed at the first before edge, 26. Existing
negative controls and strict state/transcript/RNG comparisons remain preserved.
New versioned original streams remain separate from historical streams.

Real isolated ARM64 contract-expiry sorting exercised 13,153 modern before calls,
with 32 independently checked samples. Calendar, player/contract information and
normal Save/exit/reopen/Load passed on 10 August and after normal advancement to
27 August 2002 night. These observations are separate from fixture equivalence.
The report records an excluded native File-menu save attempt and the accepted
original in-game save route. No annual crossing, whole-career equivalence or
speed improvement is claimed.

The production app is installed. Source/installed executable SHA-256 is
`d2784cfde149a28d8e8c1d067e26ac90cfff482581a36858a0d215a7790194a1`;
the signed private candidate is
`c20788488bb734e5a97590ca75c315a6d5d4f39e75f93fbc65613892ec606511`.
The current reference remains backed up privately as executable ARM64 with hash
`b61765e722c6e5f7624550ac15524fcdb623f933e9306c3329644310a887f545`.
Before/preinstall/after inventories agree for all 809 Game / 5 Runtime / 42 Runs
files and captured Mac metadata. Personal saves, prior checkpoints, immutable
LFS reference, private inputs and historical fixture/stream hashes are preserved.


## Semantic independent date-to-month delivery — 2026-10-02

Independent entry `0x10718360` now executes semantic C++23 month conversion.
The stored byte flag selects a mutable common/leap table through the current
TOC; the year is not read. A reverse signed16 strict-threshold search returns
zero-based month or -1, preserving invalid-day behavior, exact ordered reads,
CTR groups, complete ABI/temporaries/PC and absence of writes/imports/RNG.
The destination hook covers generated same/inter-page dispatch, indirect calls
and callbacks. The five previous semantic contracts and the explicitly deferred
ascending-year leap quirk remain unchanged. Adjacent day conversion, weekday,
formatting and the remaining engine are outside this increment.

The [date-to-month report](native-date-month-reconstruction.md) records 1,684
direct original/modern cases, 239 Gregorian oracle cases, 131,072 pure signed-day
checks, mutable tables/TOC, synthetic caller checks and independent ordered-read
comparison. The final suite passed 34/34 CTest groups and 28 Python tests, with
all four optional export groups separately executed, full temporary auditing
and generated/focused UBSAN. All 1,967 linked State-dependent objects use the
final layout. Immutable day/week/ordering fixtures each passed two strict originals,
original stream capture, two candidates and all four historical/delivered
baselines. Own month counts were 8,807 / 38,256 / 8,433 per candidate.
The ordering fixture also exercised 38,051 later and 12,911 before
calls. All six exercised negatives passed, including month rejected at its first
return boundary. Legacy CMCAL002 hashes, original fixture files, strict state /
transcript / RNG comparisons and permitted instruction-accounting difference
remain preserved; month uses a separate CMMON001 stream.

A single private ARM64 candidate instance advanced normally from 27 to 28 August
2002 night, with original Game Options Save/normal exit/process absence/exact
reopen/Load checked on both dates. Calendar results and Luisao's profile/contract
survived; final condition 63% also survived reload. Three app runs counted
774, 870 and 85 modern month calls, with 96 independently checked valid samples.
These UI observations are separate from automatic original/modern equivalence.
No full-engine reconstruction or matched speed improvement is claimed.

The final production ARM64 application is installed; source/installed executable
SHA-256 is `caf91df0b7d4313937711725022c38a59949bebe66a5d2c07e934efae1058756`.
The signed private candidate is
`f3e17857f83cf0922028847df8c2718b5f6fb9df83ea4d82f0f5b86e5a23de5d`.
A new executable reference backup retains
`d2784cfde149a28d8e8c1d067e26ac90cfff482581a36858a0d215a7790194a1`.
Before/preinstall/after inventories agree for the currently observed 809 Game /
5 Runtime / 43 Runs files and all inventoried metadata. Personal saves, previous
checkpoints, immutable LFS save, original inputs and historical streams remain
unchanged. Private diagnostic settings are absent from the installed plist.

## Semantic independent day-of-month delivery — 2026-10-03

The independent `0x107182f0` entry is implemented in semantic C++23 with its
own twelve-probe loop, ordered day reloads, selected-threshold reload and
signed16 narrowing. The six previous operations and deferred leap behavior
remain intact. The [day-of-month report](native-date-day-of-month-reconstruction.md)
records 3,173 direct comparisons, 1,700 Gregorian oracle cases, 131,072 pure
signed-day checks, sparse mapped-field evidence, own CMDOM001 stream controls,
35/35 CTest groups and generated/focused UBSAN passing. The clean rebuild uses
1,968 State-dependent linked objects with the stabilized layout.

All three immutable fixtures passed two strict originals, two candidates,
all five baselines and exercised negatives, with own counts 8,639 / 14,209 /
8,733 per candidate. Historical streams retain byte-identity; ordering also
exercised later/before and all seven negatives. The new private ARM64
candidate passed original Save/normal exit/process absence/exact reopen/Load at
28 August 2002 night and 1 September 2002 afternoon. Ordinary holidays crossed
the month boundary and were stopped at the first observed September interval;
Goias on August 31 completed 0:3. Calendar results and pending fixtures,
Luisao's profile/contract and final 80% condition survived reload. Three runs
recorded 5 / 10,735 / 3 own modern day-of-month calls; 40 available valid samples
passed independent checks. This is separate from automatic equivalence evidence.

The validated production ARM64 build is installed. Source/installed executable
SHA-256 is `1fa57f3f11d3c911f4d452ed5abf58a55112ed6b71defafb6439428f0fd234dc`;
the signed candidate retains
`6e5ccab6215b16680874424bd8ad51cca2c4642456047ab1f0f01ce3b3deb964`.
The new retained reference backup is
`caf91df0b7d4313937711725022c38a59949bebe66a5d2c07e934efae1058756`.
Before/preinstall/after personal inventories agree for 809 Game / 5 Runtime /
43 Runs files and all inventoried metadata. All 645 protected file identities
remain intact; the new saves/logs/builds are private and ignored. Installed
program data matches the private inputs, and private diagnostic settings are
absent from the production plist. No speed or full-engine completion is claimed.

## Semantic weekday conversion — October 3, 2026

Independent `0x10715b20` now uses semantic C++23 in normal ARM64 production,
completing this bounded eight-entry calendar increment. The destination hook
covers same-page `0x10715a54`, cross-page and indirect dispatch and callbacks.
Formatting/localized names, date adjustment `0x10715a30`, date construction,
player/contract and other neighboring routines remain generated. The deferred
ascending-year leap difference remains unchanged; no speed gain is claimed.

The exact year/flag/signed-day three-read contract, Monday=0..Sunday=6 normal
domain, negative legacy remainders, all exits, CR/XER/carry and complete ABI are
documented in [the weekday report](native-date-weekday-reconstruction.md).
The final direct harness passed 4,240 original/modern cases, 559 chrono and
319,675 wide Gregorian oracle cases and 786,432 pure signed-day checks. Full
CTest passed 36/36; Python discovery passed 26 with four optional groups skipped,
then all four groups were explicitly passed. Generated/focused UBSAN passed.
All 1,969 linked State-dependent objects and 22 others use the final layout.

The day/week/ordering fixtures each passed two strict originals, entirely original
capture, six baselines and two candidates, with 1,690 / 5,831 / 1,419 own weekday
calls. Their first-return negatives fail at edge 0, entry `0x10715b20`, return
byte 20. CMWKD001 is independent; all three historical streams, including CMDOM001,
remain byte-identical. Existing RNG/calendar/difference/later/before/month/
day-of-month controls remain exercised where covered.

A new private ARM64 candidate independently loaded Sunday September 1, 2002
afternoon. Original Save/normal exit/process absence/exact reopen/Load preserved
manager/club, fixtures/results and Luisao's profile/contract. After reading the
checkpoint's pending old news, one normal Continue Game simulation step stopped
on Tuesday September 3 morning. Monday was skipped naturally; no date or RNG
was forced. A second Save/exit/reopen/Load preserved this actual weekday change,
all observations and Luisao's normally recovered 89% condition. Three normal
runs recorded 160 / 1,794 / 152 own semantic weekday calls; all 96 available
samples passed independent checks, including real same-page caller evidence.

The approved production build is installed and verified ARM64. Source/installed
executable SHA-256 is
`7284838b3e6d74785a6969d64bf2d19619ad2cdc3e5e7c0601e6e551a97db063`;
the signed candidate is
`ace1c411a6e863fcec0125105f763df7e81543df53ae440c151b82580060db14`.
The new reference backup preserves the previously installed executable
`1fa57f3f11d3c911f4d452ed5abf58a55112ed6b71defafb6439428f0fd234dc`,
including all inventoried Mac metadata. Before/preinstall/after personal inventories
agree for 809 Game / 5 Runtime / 43 Runs files, contents and metadata. All 745
protected identities and the four pending prompts remain unchanged. Installed
program data matches private inputs; production diagnostic plist properties are
absent. Saves, logs, original payloads and builds remain private and ignored.

## Semantic Date subsystem delivery — October 4, 2026

The [Date subsystem report](native-date-subsystem-reconstruction.md) records
all 26 newly implemented entries and the closed 34-entry production manifest.
Construction, adjustment, leap, week/record arithmetic, nine formatters and
presentation helpers are semantic C++23; their original language/template,
diagnostic, ordinal-string and native-service dependencies remain outside Date.
The original five-byte layout, mutable tables and invalid-input behavior remain,
including the separately deferred ascending-year difference leap quirk.

The final build passes 6,871 direct original/semantic ABI/effect comparisons,
104 composed stream agreements, 52 own-result negatives and 26 callback pairs.
All eight earlier contracts pass new regressions; CTest passes 39/39, explicit
Python audits and generated/focused UBSAN pass. Day and ordering integrated
campaigns pass with byte-identical recapture of all four historical streams.
The weekly original capture and both semantic replays completed, with historical
identities and per-run identity checks. At the user's explicit request, the
remaining extended weekly baselines/negatives and final count matrix were stopped
and waived. This delivery does not claim a full weekly campaign PASS.

Two Save/normal exit/process absence/exact reopen/Original Load cycles pass
around natural progression from September 3 morning to September 4 night.
Dates/phases, fixtures/results and Luisao's observed profile/contract persist.
Additional normal UI flows resolve rare formatter and language coverage; all
26 new entries have their own real calls. Original-only UI samples at the six
rare presentation callers match the semantic return and output bytes. Personal
content and metadata, 858 protected identities and the new installed reference
remain unchanged. That delivery installed the complete 34-entry ARM64 build;
its source and installed executable SHA-256 was
`a35a303f441d352d61e0ab0a8e154e06f14f747461310c9e6ac28ce36abaf1e5`.
The new executable reference preserves the preceding installed
`7284838b3e6d74785a6969d64bf2d19619ad2cdc3e5e7c0601e6e551a97db063`.
Fresh preinstall/post-install personal content and all metadata checks pass.
At the user's request, large captures and temporary test artifacts from this
execution were cleaned after identities were recorded; saves, immutable fixtures,
protected history, original inputs, candidates/reference and evidence receipts
remain private and preserved. Cleanup removed about 18.9 GB; post-cleanup complete
personal/historical content and metadata checks pass.

## Semantic Mac RNG core delivery — October 4, 2026

The [RNG core report](native-rng-core-reconstruction.md) records three production
C++23 entries: CRT draw `0x1077b770`, seed `0x1077b790`, and pool initialization
`0x107474a0`. The manifest now contains 37 semantic replacements, preserving all
34 Date entries and the deferred ascending-year leap behavior. The original
unsigned16 big-endian pool consumers, sampler, scheduler and match engine remain
generated. The Windows ReCM generator and its services are not transplanted.

Direct validation passes 2,022 complete ABI/memory/ordered-effect comparisons,
three own-entry negatives, seven controlled clocks, generated consumer wrap,
callbacks and indirect transitions. CTest passes 40/40; Python discovery passes
37 with four optional groups skipped, and all four pass explicitly. Generated
and focused UBSAN pass within the report's stated instrumentation boundaries.
Two immutable day replays per mode agree with 209,662 draw and 4,201 seed calls
each, exact state and EOF; the initial RNG negative rejects. Initialization has
zero day-fixture coverage and two actual calls in each final app process.
A final app-only oracle guard rejects captures unless both Date and RNG are
explicitly original; the rebuilt app passes its targeted selection controls.
The replay executable and core/UBSAN units retain their verified identities.

The unchanged final candidate completes two Original Save/normal exit/process
absence/exact reopen/Original Load cycles before and after normal night-game
processing at September 4, 2002 NTE. Club fixtures, Luisao's profile/contract and
the new Brazil 5:1 Uruguay result persist. All three RNG entries have separate
real semantic counts, zero original-mode calls and 54 checked samples. Actual
Load preserves career/world state without restoring the prior startup CRT
trajectory; the report separates this behavior from deterministic snapshot
replay. Preliminary automation/relaunch failures are documented and excluded.
The explicitly waived remaining Date weekly campaign was not reopened.

One protected installation completed at 21:35:45 -03:00 on October 4, 2026.
Source and installed ARM64 executable SHA-256:
`1b84152cc5ddad11d43fa2b3fc7c0293b20d2dcf0a8008ee0750843867a0d9b6`.
The signed validation executable is distinct, and the new installed reference
preserves the preceding Date build `a35a303f...`. Production has no diagnostic
flags; four bundled originals match their input identities. Fresh personal
inventories retain 809 Game / 5 Runtime / 44 Runs files with all content, forks,
xattrs, modes, mtime and flags; all 1,491 historical identities remain preserved.
Cleanup and reproducible-payload details are recorded in the RNG report.
This remains a working development build, not an entirely reconstructed engine,
a full-career guarantee or a claimed speed improvement.


## Native presentation and direct commands — October 5, 2026

The installed app includes interface scale controls, native text/control drawing and
named Continue/Save/Load/Options/Back commands. The user expanded scope to native
text after rejecting bitmap-only enlargement, and confirmed physical clicks.
Mouse menu events now follow the same original command path as shortcuts without
querying a keyboard-only AppKit property. The 37 semantic replacements are unchanged.

The user accepted the native appearance. Live checks cover 200% on the external
1x display, 100%/125%/Fit on Retina, and recovery after monitor removal. The final
candidate completed a keyboard Save/normal exit/reopen/Load cycle at 125%, with
career fields compared afterward. Menu Continue produced one original response;
Options and cancellation worked. Reused-image drawing and stale Return input
were corrected and rechecked. The final suite passed 44/44 CTest.

The protected installer ran once at 15:12 UTC on October 5. The installed ARM64
executable matches the tested source; personal Game, Runtime and Runs content and
metadata, protected references and fixtures remain unchanged. Bitmap artwork
retains its original resolution; Load is supported from the start screen.
See the [detailed report](native-presentation-and-direct-commands.md).


## Semantic background selection core — October 5, 2026

Four related Mac background selectors are now semantic C++23: catalog selection
`0x107122e0`, image-list selection `0x10712a60`, default category `0x10713960` and
object/descriptor category `0x107139c0`. Original payload choices, fallbacks,
signed16/byte reductions, ordered ABI/memory effects and Mac pool/RNG consumption
are retained. Destination hooks on pages `0x10712`/`0x10713` cover direct/local
calls, generated indirect transitions and callbacks. The manifest contains 41
entries, preserving the 34 Date and three RNG entries.

The consistent ARM64 build passes. Focused acceptance passes 163 original-versus-
semantic background comparisons, 18 faults, four own negatives, nonempty normal
production dispatch of all four entries, four callbacks and four indirect
transitions. The existing background regression and 2,022-case RNG check pass;
four new Python audit/hook tests, two existing RNG audit tests and UBSAN of the
new adapter/harness pass. Only failed fixture checks were repeated after correcting
original alias/null-traversal expectations; no whole suite or Date/RNG campaign
was run. Sampler, accessors, wrappers, null getter guards and drawing remain
generated or retain their existing native integration. The original Date leap
asymmetry, native text/controls, scale and commands remain unchanged.

The protected installer ran once at 18:13:23 UTC (15:13 local time). Source and
installed ARM64 executable hashes match; four original payloads and the
production plist are verified. Personal content/metadata, the presentation
preference and prior installed reference are preserved. No live app session was
opened, and harness integration is not represented as fresh UI coverage.
Save/Load was not retested at the user's direction; it is neither approved by
this delivery nor a required pending gate. This reconstructs the four-selector
core, not the whole engine, and makes no visual or performance claim.
See the [detailed report](native-background-selection-reconstruction.md).

## Semantic collection lookup core — October 5, 2026

Five Mac collection queries now run semantic C++23: list indexing `0x1074dd40`,
positional wrapper `0x1074df60`, descriptor wrapper `0x1074df90`, preorder helper
`0x1074e150` and comparator-search helper `0x1074e5b0`. Destination hooks on
pages `0x1074d`/`0x1074e` preserve their original callers and recursive/callback
dispatch. The manifest contains 46 entries, retaining the prior 41 in order.
Guest position aliases, wrapped decrement, four expanded levels, 80-byte helper
frames, mutable comparator vectors/children, TOC/clobbers and ordered fault
prefixes remain compatible with the original Mac export.

The consistent Release ARM64 build passes. Focused acceptance passes 263
original-versus-semantic comparisons, 27 faults, five own negatives, nonempty
normal dispatch of all five entries, recursion of both helpers, five callbacks,
five generated indirect transitions and eight background/RNG compositions.
Both existing background regression/equivalence checks pass. Five new Python
audit tests and four updated background audit tests pass without skips; UBSAN
of only the new adapter/harness passes. Only the new harness was repeated after
correcting an original-behavior expectation in a child-fault fixture.

The protected installer ran once at 18:55:18 UTC (15:55 local time). Source,
candidate and installed ARM64 hashes match, bundled originals and production
plist are verified, and ad-hoc bundle signatures pass strict verification.
Personal content/metadata, presentation preference state, retained handoffs and
both installed references are preserved. No live app session was opened;
Save/Load was not retested at the user's direction and is not a required gate.
Date, RNG, background selectors, appearance and commands retain their delivered
behavior. Collection lifecycle operations, other accessors and descriptor
comparison remain generated. This completes the five-query block, not the whole
engine or collection lifecycle, with no visual or performance claim.
See the [detailed report](native-collection-lookup-reconstruction.md).


## Semantic Mac basic strings and descriptors — October 5, 2026

Fourteen basic Mac string/descriptor entries now run readable semantic C++23:
Mac table comparison (complete and signed16 bounded), descriptor comparison
wrapper, allocated duplication, in-place table mapping, length, copy, word32
bounded copy/padding, concatenation, unsigned-byte comparison (complete and
word32 bounded), first/last byte search and substring search. The selection →
collection query → descriptor comparison dependency chain is now semantic.
Destination hooks on `0x10739`/`0x1074d`/`0x1077d` cover direct/local and indirect
calls. The manifest contains 60 entries, preserving the previous 46 in order.
Mutable/relocatable tables, raw NUL/0xff rules, signed mapping, lookahead reads,
progressive overlaps, exact byte differences, guest frames/clobbers and partial
fault effects remain original-compatible. Tables and allocator remain generated.

The consistent Release ARM64 build passes. Focused acceptance passes 314
original-versus-semantic comparisons, 62 faults, fourteen own return-boundary
negatives, nonempty normal dispatch of all fourteen entries, four mixed modes,
one generated descriptor-vector transition, two background/query/string/RNG
compositions, six Date fallback copies and two normal formatter compositions.
The four directly related collection/background/decimal CTests pass. The three
affected Python audits pass 5/5/4 tests without private-index skips. UBSAN of
only the new adapter/harness passes with empty stderr. Two formatter-fixture
issues were corrected; only the new failed harness was rerun. No production
adapter correction, whole-suite run or Date/RNG/allocator campaign was needed.

The protected installer ran once at 21:34:57 UTC (18:34 local time). Signed
source, candidate and installed ARM64 hashes match; bundled originals,
production plist and strict ad-hoc signatures are verified. Personal content
and metadata, absent presentation preference, all handoffs and the three
prior-installed references remain unchanged. No live UI session was opened.
Persistence was not retested at the user's direction and is not a pending gate.
The original Date leap asymmetry, prior semantic modules, appearance and
commands retain their delivered behavior. General text/language/templates/
formatting, character classification, parsing, memory services, table
initialization and allocation remain outside this basic string/descriptor
subsystem; the large service at `0x1077de10` stays generated.
See the [detailed report](native-string-core-reconstruction.md).


## Semantic Mac block-memory primitives — October 5, 2026

Six bounded Mac memory entries now run readable semantic C++23: Carbon-backed
block copy `0x10739ef0`, destination-returning fill wrapper `0x10778300`, first/
last byte search `0x10778330`/`0x10778360`, normalized block comparison
`0x10778390` and aligned fill leaf `0x107783e0`. Internal NULs remain block data,
comparison preserves divergence rereads and exact -1/0/+1 results, and fill
preserves alignment, separate four-byte stores, counters/flags and fault prefixes.
The leaf's large-path r3 count return differs from the wrapper's destination.
Copy retains the generated mutable-vector stub and existing native BlockMoveData
source snapshot, bulk validation, overlap and zero-count import. Destination
hooks on pages `0x10739`/`0x10778` reach local/page/indirect dispatch. The manifest
contains 66 entries, preserving the previous ordered 60 and Date leap behavior.

One regeneration and a consistent Release ARM64 build pass without warnings or
errors. Focused acceptance passes 270 original-versus-semantic ABI/memory/ordered-
effect comparisons, 96 unmapped-access faults, six own negatives plus a nested
child negative, six nonempty normal entries, two mixed fill modes, six generated
indirect transitions, two Pascal conversions and two real precision-string
formatter/search compositions. The required string, decimal and native bridge
regressions pass: four focused CTests total. New/affected Python audits pass
5/5/5/4 tests without skips. UBSAN of only the new adapter/harness passes with
empty stderr. No failed runtime check, repeat of green groups, whole-suite,
Date/RNG/allocator, I/O, season or UI campaign was needed.

The protected installer ran once at 23:58:15 UTC (20:58:15 in São Paulo).
Signed source, candidate and installed ARM64 hashes match; bundled originals,
production plists and strict signatures are verified. Personal Game/Runtime/Runs
content and metadata, absent presentation preference, LFS snapshot, nineteen
retained prompts, new installed reference and prior references/candidates are
preserved. No live UI or personal career was opened. Persistence was not retested
at the user's direction and is not a pending gate. Stub/import, allocator/pools,
generated consumers and broader memory/text/runtime services retain their
existing paths. This closes the six-entry block with no visual or performance
claim. See the [detailed report](native-memory-primitives-reconstruction.md).


## Semantic Mac integer parsing — October 6, 2026

Five Mac entries now execute readable C++23 in production ARM64: callback-based
word32/word64 magnitude parsing `0x1077ee20` / `0x1077f180`, signed32 conversion
`0x1077f570` and decimal ABI wrappers `0x1077f670` / `0x1077f6a0`. Named phases
preserve base/prefix/field/EOF/lookahead behavior, separate sign/overflow,
modular accumulation, guest classification/mapping tables, mutable descriptor
callbacks, phase-table reads/CTR and the existing unsigned-division path.
The wrapper preserves end-pointer/flag/error ordering and signed saturation.
Destination hooks include the parser32 page crossing. Whole-body and fragment
pins plus edge/hook audits protect the boundaries. The manifest has 71 entries,
with the prior 66 preserved in order; Date leap behavior and existing guards
remain intact. Unknown phase destinations retain the generated dynamic-transfer
boundary and are separately counted, outside normal semantic coverage.

Focused acceptance passes 435 complete architecture/memory/ordered-effect
comparisons, including 222 faults, five own negatives and a child negative,
five nonempty semantic entries, two mixed modes, five generated indirect
transitions and two scanner compositions (`%d%n` / `%llu%n`). The string,
memory and unsigned-division regressions pass. Five new audit tests and the
5/5/5/4 affected prior Python tests pass without skips; UBSAN of only the new
adapter/harness passes with empty stderr. Two new fixture expectations were
corrected; only the failed new harness was rebuilt/rerun. One regeneration and
a consistent Release ARM64 build completed without warnings/errors. Final
review corrected the five-entry harness witness loop; the new CTest/UBSAN were
rerun without changing the productive adapter or installed executable. No full
suite, Date/RNG/allocator, replay, season, benchmark or career campaign ran.

The protected installer ran once at 18:20:36 São Paulo time. Signed source,
candidate and installed ARM64 executable SHA-256 is
`191e7c599671efb7f0f1bed0273020693c86eddc59a5d40349f0a79e28d845b4`;
original payloads, strict signatures and production plists are verified. Personal
contents/metadata, the LFS snapshot, twenty retained prompts, the new installed
reference and twenty-four historical bundles were unchanged at installation.
A subsequent short smoke test observed the installed Prepare Game menu,
Command-O's Load chooser, Command-['s cancellation and normal quit confirmed
by Return, with process termination and zero fatal diagnostics. The smoke
created logs/preferences; Game/snapshot/prompts/references remain unchanged.
It did not call the integer entries; their coverage is in the harness.
Persistence is explicitly not retested. General scanner/formatter/floating/
locale/stream/runtime services remain outside this integer block, with no visual
or performance claim. See the [detailed report](native-integer-parsing-reconstruction.md).


## Semantic Mac formatted input — October 6, 2026

The complete seven-entry formatted-input block now runs semantic C++23: format
analysis `0x1077b7a0`, scanner `0x1077bf20`, string callback `0x1077d960`, string
vector/variadic facades `0x1077da70`/`0x1077dad0`, stream callback `0x1077d8d0`
and stream variadic facade `0x1077d9f0`. Destination hooks and whole-body/three-page
pins protect all 2,248 indexed instructions. The manifest has 78 replacements,
retaining the previous 71 in order, including independent integer-parser selection.
Live guest format templates, classification/destination tables, descriptors,
argument cursors, ABI effects, packed extensions and string/stream lookahead are
preserved. Output formatting, floating parsing, character conversion and stream
services remain dependencies.

Focused acceptance passes 931 full-state/ordered-effect comparisons, including
645 expected faults, seven own negatives, one format-child negative, seven indirect
transitions and 12 mixed modes. All seven entries have successful nonempty semantic
witnesses with zero original calls/normal escapes. Integer and string CTests pass;
30 Python audit tests and focused new-adapter/harness UBSAN pass. Missing decimal
`dec2num` and NaN services remain matching dependency failures; a bounded original
hexadecimal-float parser attempt did not terminate. Successful infinity paths cover
float/double and packed stores. Those retained services are not reconstructed.

One protected installation completed at 20:10:53 São Paulo time. The signed source,
candidate and installed ARM64 executable match SHA-256
`2e88cca135176aed98fe9771c06a269463e7fd16ebc14161fd66bd9c4492437e`.
Original payloads, signatures and production plist are verified; personal content,
metadata, saves, prompts and historical bundles were preserved before smoke.
The installed app rendered Prepare Game, opened/cancelled its chooser by named
shortcuts without loading, and exited normally without fatal diagnostics. The smoke
selected modern entries but did not call this block; individual coverage is offline.
Runtime/Runs may change from the intentional smoke. Save/Load is **not retested**,
neither passed nor a required future gate. No full suite, days, match or career ran.
See the [formatted-input report](native-formatted-input-reconstruction.md).


## Semantic Mac formatted output, characters and floating input — October 9, 2026

Twenty-two new entries now run semantic C++23 in production ARM64: fifteen
output format/parser/numeric/descriptor/callback/facade entries, six multibyte/
wide16 character entries, and the descriptor-based floating input parser. The
missing CarbonLib dec2num/nan imports now have explicit PPC record/payload ABI
bridges. Decimal and NaN scanner fixtures that previously failed at absent
services now have independently expected successful results. The manifest has
100 ordered replacements and retains the previous 78 unchanged. Destination
hooks cover local, cross-page and indirect dispatch, including the one shared
reset edge `0x10779ff4 -> 0x10779ed8`. Whole-body and eight fragment pins protect
all 3,902 indexed instructions. Live guest tables/descriptors, Mac argument
layouts, callback clobbers, faults and architectural effects are retained.

Focused acceptance passed 1,395 output comparisons (1,028 expected faults),
335 character comparisons (49 faults), and 156 floating input comparisons
(89 faults). Every new entry has nonempty normal semantic calls, zero original
selection/normal escapes, an own negative and a generated indirect transition.
Each chain has a child negative. Output also has two shared-reset transitions,
16 mixed modes and three unknown-destination diagnostics excluded from normal
witnesses; characters have eight mixed modes and floating input has two. The
scanner regression passes 940 comparisons (641 faults), seven own negatives,
a child negative, seven indirect transitions and 12 mixed modes. Decimal checks
retain 1,064 num2dec ABI cases and 14 original formatting examples and add 101
independent dec2num/nan input/CFM checks plus two independent negative controls.

Final regeneration and a consistent Release ARM64 build passed. Seven focused
CTests passed in 26.44 seconds; nine Python audit scripts passed 46 tests.
Focused UBSAN of new/modified adapters, bridges and harnesses passed all five
binaries with empty stderr against the final consistent libraries. The bounded
legacy hexadecimal exponent attempt exhausts 1,500 instructions in both
original and semantic modes and is excluded from passing conversion counts;
executable hexadecimal paths are checked separately. No correction was made
to that legacy behavior or Date's ascending-year leap behavior.

The protected installer ran once at 21:12:21 São Paulo time. Signed source,
candidate and installed ARM64 executable SHA-256 is
`0725840a0907bc6bd866d47632fbe75606defaa98d320ecb53b50c7348fed749`.
Original payloads, strict signatures and production plists passed. Personal
content/metadata, the LFS snapshot, all 22 handoffs (17 private), the new baseline
reference and 117 historical bundles were preserved immediately after install.
The installed app rendered Prepare Game, opened/cancelled its chooser by named
commands without loading, and exited normally with no fatal diagnostics.
Seven output entries had semantic calls in the smoke; complete individual
coverage is in the harness. Final verification found no app process and
Game/snapshot/handoffs/references/historical bundles remained unchanged.
Runtime/Runs and preferences may change from the intentional smoke.

Save/Load is **not retested**. No career, day, match, serialization campaign,
benchmark or full suite ran. Locale, num2dec, fwrite/flush/refill and remaining
file/stream services retain their existing ownership and implementation.
No visual/performance improvement is claimed. See the
[detailed report](native-formatted-output-reconstruction.md).


## Semantic Mac arenas, small pools and allocation facades — October 9, 2026

Twenty-seven new allocator entries now run named semantic C++23 in production
ARM64: arenas, boundary tags and free lists, small-object pools, context dispatch,
reallocation/clearing and four global facades. Guest big-endian word32 metadata,
addresses, live class tables and access/ABI effects are retained. The manifest
has 127 ordered replacements and preserves the prior 100 exactly. The full
1,057-instruction body pins, two reallocation page fragments and destination hooks
are audited; no external interior edge is present in the indexed flow graph.
Lifecycle, acquisition/disposal glue, streams and Carbon allocation policy retain
their existing ownership.

The productive pool progress correction is preserved at the same failed-allocation
branch, using the existing next_pool_attempt helper. The oracle is the guarded
translation. Independent checks establish decreasing retry limits, progress,
payload preservation and nonoverlapping slots across the 16 synthetic hint/size
combinations. No raw-loop retry or captured historical heap campaign was run.

Focused acceptance passes 337 full-state/ordered-effect comparisons, including
86 expected mapping faults, 27 own negatives and three child-before-parent
negatives. All 27 entries have nonempty normal semantic witnesses, zero original
selection/escapes and generated indirect dispatch coverage. Fourteen mixed modes,
all six pool classes, boundary tags/coalescing, refill/disposal, reallocation,
word32 counted-clear wrap, relocated TOC, live table mutation, aliases and access
prefixes are covered. Five focused CTests pass; the new audit and nine affected
prior audits pass 53 tests. Focused UBSAN passes three binaries with empty stderr
against the final consistent Release ARM64 libraries.

The protected installer ran once at 22:18:59 São Paulo time. The signed source,
candidate and installed ARM64 executable match SHA-256
`14eb1c373854e95c6e642ebc1ddd99ae56f9c353ecb49de93e0822cc93e44ae8`.
Strict deep signatures, original payloads and production plists pass. Personal
content and metadata, saves, all 23 handoffs (18 private), the new prior-app
reference and 120 historical bundles were preserved before smoke.
The installed app rendered Prepare Game, opened/cancelled its chooser by named
commands without loading, and exited normally without fatal diagnostics. The
exit summary records semantic calls to all 27 entries, zero original selection
and zero allocator escapes. This observes execution in that UI sequence; branch
and equivalence evidence remains in the harness. Final protection again passed
for Game/snapshot/handoffs/payloads/references/historical bundles and no app process
remained. Runtime/Runs and preferences may change from the intentional smoke.

Save/Load is **not retested**. No career, day, match, serialization campaign,
benchmark, whole suite or historical heap replay ran. Date's ascending-year leap
behavior and adjacent subsystem behavior remain unchanged. No visual or
performance improvement is claimed. See the
[detailed allocator report](native-allocator-pools-reconstruction.md).


## Semantic Mac stream storage and byte/item I/O — October 10, 2026

The thirteen stream-storage/lifecycle replacements delivered in
`f71ae1b782c0cc8afaed337c90442a5478acd876` brought the manifest from 127 to 140.
Their [report](native-stream-buffer-reconstruction.md) preserves the focused
234-comparison, callback/fault and installed-smoke acceptance, with Save/Load
not retested for that bounded delivery.

Nine further byte/item-I/O replacements now bring the ordered manifest to 149,
preserving the exact prior 140. Slow/fast byte reads, line reads, byte pushback,
item read/write bodies/facades and orientation execute named C++23 by default.
Guest metadata, callback ABI, bulk overlap behavior, word32 wrap and the original
read-floor/write-partial-item credit remain intact. The original pinned bodies
cover 682 instructions with no indexed external interior edges.

Acceptance passed 226 comparisons, 27 expected faults and 13 own/child negatives,
six focused CTests, seven new audit tests plus 60 tests in affected prior audits,
and three focused UBSAN binaries. All nine entries have direct/indirect normal
semantic witnesses, zero own original calls and zero dispatch escapes. Full
Release ARM64 generation retains 1,953,157 indexed instructions across 1,922 pages,
with zero missing/pending destinations.

An isolated signed app using identical compiled code loaded Reading, saved through
the original in-game menu to a new private streamio.sav, exited, reopened and loaded
that file. Club, manager, observed date and visible squad persisted. This witness
used the existing diagnostic event-coordinate/unmodified-click mode to address
CUA background input; installed-production Save/Load and complete serializer
coverage are not claimed. User Reading and original/private artifacts remained
protected. No day or match was advanced.

The protected installation ran once. Source, candidate, installed and running
smoke ARM64 executable SHA-256 is
`5e219ce7b0c4e35a0761f0ba4ff52a326873bfe1faf9f7e99dd60172ec6c373a`.
Strict signatures, normal production launch metadata and four original payload
hashes passed. The installed app rendered Prepare Game, opened/cancelled the
chooser and exited normally with no fatal diagnostics. Final preservation passed
for Game, user saves, payloads, handoffs, references and 126 historical bundles;
Runtime/Runs/presentation preferences may change legitimately after UI.

File seek/tell, open/mode parsing and remaining runtime/engine services remain
in progress. The [detailed I/O report](native-stream-io-reconstruction.md) and
[complete-port ledger](native-cpp23-port-progress.md) distinguish this accepted
subsystem from the still-active whole-port objective.

## Native 64-bit and async file transport — October 10, 2026

Seventeen missing File Manager imports now have real native fork behavior,
including 64-bit positions/sizes and Classic/fork asynchronous completion.
The shared bounded transfer retains original-media overlays and guest access
boundaries. The [transport report](native-fork-io-bridge.md) records 377 assertions,
three real generated CFM completion executions, seven generated stream callback
executions, five focused CTest passes and three UBSAN binaries with empty stderr.
Native errors, sparse files above 4 GiB, EOF, large counts, close ownership and
read-only rejection are exercised against actual private files.

The signed source/candidate/installed/running ARM64 executable SHA-256 is
`b773d4ab9281a23b673243b384e935b0eec215728bf01e772d32e7a6531113c5`.
A preliminary installation preceded the final SDK review of the Classic async
queue-result ABI. The corrected artifact was rebuilt/retested and installed in
a second protected update, preserving the prior app,
personal content/metadata, saves, payloads, 23 handoffs and 131 earlier bundles.
The installed smoke reached Prepare Game, opened/cancelled the original chooser
and exited normally with no fatal diagnostics. Final protection passed; no
game process remained. Save/Load is **not retested** for this increment.

At this transport delivery the semantic manifest remained exactly 149, and
production capability selection stayed Classic pending FSRef/Unicode acceptance.
These transport checks do not claim new imports executed in ordinary menu smoke,
semantic file-controller replacement or whole-port completion. The complete
file-controller and remaining engine/runtime work continues under the active goal.

## Native FSRef and Unicode file services — October 10, 2026

Thirteen further native services now implement opaque file references, Unicode
lookup/create/rename, selected catalog metadata, named fork opening, comparison,
deletion and owned text-conversion contexts, together with the Classic catalog
setter. Production `Gestalt('fs  ')` returns `0x1302`, so the existing generated
controllers discover and use the FSRef/64-bit path normally. Guest catalog
fields, UTC dates and Finder records are explicitly marshalled; host pointers and
volume IDs do not enter the guest ABI. Original-media writes retain private-copy
preservation. The [detailed report](native-file-references.md) records the contracts
and remaining original-media tombstone limitation.

Focused acceptance passed 401 assertions, 17 generated routine calls and 28
distinct import services. Every new service has positive and rejection evidence
and is exercised through the original chain, including a mixed Classic fixture.
Six related CTests passed in 17.94 seconds, and three UBSAN binaries passed with
empty stderr against the final consistent Release ARM64 build. The semantic
manifest still contains the exact prior 149 entries; native import implementation
does not reconstruct the original file-controller bodies.

One protected installation delivered the signed source/candidate/installed/running
ARM64 executable with SHA-256
`b3c376d2cb730bef9173890da3f5dc5d73d996107539269fc379611984103c88`.
Strict signatures, normal production plist and four original payload hashes passed.
The previous installed executable is preserved at SHA-256
`b773d4ab9281a23b673243b384e935b0eec215728bf01e772d32e7a6531113c5`.
The installed app rendered Prepare Game, opened/cancelled the original chooser
without loading a career and exited normally with no fatal diagnostics. The
process was confirmed closed. Final protection passed for Game content/metadata,
the workspace save snapshot, 23 handoffs, payloads, reference and 136 earlier
private bundles. Runtime/Runs and presentation preferences may change after UI.

Save/Load is **not retested**. No career, day, match or performance benchmark ran.
Menu smoke does not establish per-entry coverage; the native-file fixtures supply
that evidence. Semantic file controllers and the remaining engine/runtime work
continue under the complete-port goal.


## Native Mac file controllers — October 10, 2026

Twenty-six file-controller entries now execute named C++23 bodies in production:
filename facades, fopen/reopen/mode parsing, capability and metadata selection,
Classic/FSRef open/read/write/seek/close/remove/rename, temporary-record lifetime,
tell/get-position/seek/set-position and status/stdio initialization. Their 1,421
indexed instructions, the two open-backend fragments and destination hooks are
pinned. The manifest now has 175 entries, preserving all prior 149 in order and
content. The [detailed report](native-file-controller-reconstruction.md) records
contracts, complete ABI/effect comparisons and per-entry counters.

Focused equivalence passed 607 comparisons, 111 expected mapping faults and 31
own/child boundary negatives. All 26 entries have normal semantic and generated
indirect witnesses. The fixture matrix includes pending callbacks, 82 real owned
fork comparisons, empty EOF and sparse signed-64-bit carry/borrow. A separate
real-OS composition passed 420 assertions, 17 production dispatches and 28 import
services, with normal semantic counts for 18 controllers and zero original
selection/escapes. Seven related CTests passed in 14.79 seconds; the final added
EOF cases passed the affected CTest again in 4.64 seconds. Thirteen audit suites
and both UBSAN harnesses passed against a consistent Release ARM64 build.

One protected final installation delivered source/candidate/installed/running
executable SHA-256
`6ca38d4eb7c44ca73f098b758e9a65b6ebd0274b529052321d42b75783943515`.
The previous installed FSRef/Unicode artifact is preserved. Strict signatures,
normal plist and four payload hashes passed. Prepare Game rendered; original
Load Game opened the chooser, cancellation returned to the menu, and original
Quit/confirmation exited normally. The tested process was confirmed closed.
Game content/metadata, user/workspace saves, 23 handoffs, original payloads,
reference and 139 older private bundles were preserved. Runtime/Runs may change
through UI. Save/Load is **not retested**; no career, day, match or performance
benchmark ran. The complete C++23 engine/runtime goal remains active.


## Native Mac file paths — October 10, 2026

Seven path/name helpers now execute named C++23 bodies in production: CString to
Pascal, Classic/capability-aware FSSpec construction, script encoding, path to
FSRef, Unicode-name conversion and parent/leaf resolution. Their 537 indexed
instructions and destination hooks are pinned, with zero external interior
edges. The manifest now contains 182 entries and retains the exact prior 175.
The [detailed report](native-file-path-reconstruction.md) records contracts,
complete ABI/effect comparisons and per-entry native counters.

Focused equivalence passed 277 comparisons, 21 expected mapping faults and 12
own/child negatives. All seven have direct/generated-indirect normal witnesses
and independent original selection/reset. The matrix includes live aliases,
relocated TOCs, length wrap/sign behavior, full-word script errors, partial
conversion and the legacy sign-extended-byte fallback. Fifty-four comparisons use
real native services. A separate real-OS composition passed 428 assertions, 17
production dispatches and 28 import services, with normal semantic calls for all
seven path entries and zero original selection/escapes.

One consistent full Release ARM64 build completed after the State change. Seven
related CTests passed in 21.85 seconds, fourteen audit suites passed (81 tests),
and both instrumented UBSAN harnesses passed with empty stderr. The two final
native-directory cases passed the affected path CTest again in 2.16 seconds. One protected
installation delivered source/candidate/installed/running executable SHA-256
`ecb56a3ecf166793c88189d5c1ff10af049686c049e21182739220327b28d6da`.
Strict signatures, normal plist and four payload hashes passed; the prior
controller app is preserved. The app rendered Prepare Game, opened/cancelled the
original chooser and exited normally. A UI-query-triggered reopening was also
closed normally; both owned processes were confirmed closed.

Game content/metadata, user/workspace saves, 23 handoffs, payloads, reference and
142 older private bundles were preserved. Runtime/Runs may change through UI.
Save/Load is **not retested**; no career, day, match or performance benchmark ran.
The complete C++23 engine/runtime goal remains active.


## Semantic Mac clock, calendar and daylight runtime — October 11, 2026

Eight complete time-runtime entries now run named semantic C++23 in production:
zero-status leaf, year-offset leap predicate, calendar conversion, field
initialization, optional-output time facade, shared local fields, Mac clock
epoch conversion, and machine-location daylight status. Word32 epoch behavior,
live signed month rows, guest frames/registers, ordered effects, nullable guards
and nonlocal transfers retain their original contracts. Existing host clock and
location services, Date's ascending-year leap quirk and allocator policy remain
unchanged.

The complete generation has 1,953,334 instructions, 1,922 pages, zero missing/
pending targets and **276** semantic replacements, preserving the first 268
exactly. Acceptance passes 3,025 component comparisons, 2,036 production
comparisons, individual direct/generated/CFM coverage, eight separate profile
selections, 128 nonlocal comparisons, six focused CTests, 22 Python suites /
144 tests with zero skips, and thirteen-unit UBSAN with identical Release output.
Expected faults and budget stops are separately classified and are not normal
coverage. See [native-time-runtime-reconstruction.md](native-time-runtime-reconstruction.md)
for contracts, per-entry evidence and the corrected fixture/packaging attempts.

The protected final installed ARM64 executable is
`c21cad895a67e5ff3142cbd532bcfad1b9d5716b2d68553358bb2f759b333405`.
The owned smoke rendered the menu, opened/cancelled the original chooser, and
exited normally. Three time entries had normal semantic calls and zero original,
profile, budget continuation, transfer, fault or escape diagnostics. Individual
coverage of all eight belongs to the harnesses. Game/saves/payloads, 23 handoffs,
references and 175 older private bundles were preserved. Runtime/Runs may change
through the smoke. Save/Load is **not retested**; no career/day/match/season or
performance benchmark ran. The complete C++23 port goal remains active.
