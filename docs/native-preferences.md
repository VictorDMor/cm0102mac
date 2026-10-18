# Complete preference lifecycle

Status: **integrated and installed**. Twelve complete operations reconstruct
preference initialization, copying, destruction, defaults, option selection,
language, currency, interval and video setters, and application with persistence.
They span **1,444 original instructions**, with individual complete-return
coverage and two discriminating negatives per root.

The [combined integration](native-preferences-integration.md) adds these roots
and the complete currency configuration after the exact 884-owner prefix,
reaching **897 installed owners**. It records actual platform/file services,
whole load/apply callers, independent 303-byte preference-file layout, original
controls, focused UBSAN and protected installation. Prepare Game and normal quit
are verified. The installed ARM64 executable has SHA-256
`7af41b4b2882a5dbc16d679884cb60caa2175cc7242cf3074733c8076bb9e00d`.
The whole-game goal remains active; isolated modeled proof below is separate
from the integration's actual-service coverage.

## Full operations and source boundaries

| Root | Exclusive end | Instructions | Operation |
| --- | --- | ---: | --- |
| `0x1013f030` | `0x1013f404` | 245 | Initialize preferences |
| `0x1013f410` | `0x1013f4c4` | 45 | Copy into a linked edit record |
| `0x1013f4d0` | `0x1013f564` | 37 | Destroy preferences |
| `0x1013f570` | `0x1013f67c` | 67 | Select defaults |
| `0x1013f680` | `0x1013fb58` | 310 | Select a mutually exclusive option |
| `0x1013fb60` | `0x1013fc44` | 57 | Set language |
| `0x1013fc50` | `0x1013fcfc` | 43 | Store currency choice |
| `0x1013fd00` | `0x1013fdf4` | 61 | Select currency for language |
| `0x1013fe00` | `0x1013feac` | 43 | Set interval |
| `0x1013feb0` | `0x101406d4` | 521 | Apply and persist preferences |
| `0x101406e0` | `0x101406e8` | 2 | Read video mode |
| `0x101406f0` | `0x10140724` | 13 | Set video mode and description |

The source contract pins all thirteen page fragments and 28 alignment words.
The index has 60 incoming root edges and zero external interior entries; guards
reject new external interior entries. The contract pins nine TOC pointers,
the 262-byte original string bank and the initial bytes of the audio cell,
command line, capability/UI/context cells, video mode, diagnostic flag and
scratch. These are initial-image contracts, not runtime immutability rules.
Native bodies contain named stages and structured branches, with no PC/opcode
interpreter, generated-body fallback or embedded original string bank.

The two currency calls are retained inside their **whole** callers: preference
initialization calls configuration at `0x1013f200`; application calls it at
`0x10140004`. They are not separately owned interior slices.

## Record and lifecycle

The root record starts with a null link. Copying records the source pointer at
offset zero, copies flags/language/currency/interval, performs three bounded
string copies and explicit final NUL stores, and copies the load marker and video
mode. The modeled overlap/self-copy cases preserve the original access order.

Applying a linked edit record copies its fields into the linked primary and
recursively applies that primary. It re-reads the link between field writes and
string services. This is not an atomic transaction: copying and live service
changes precede the file result, and failure does not roll them back.

Destroying a primary owns audio/display/language cleanup; destroying a linked
record skips those services. A positive signed low halfword of the delete flag
then invokes deallocation. Null destruction returns null. Other required object
and data accesses retain their faults.

Defaults start with flags `0x212552a9`. Platform/capability queries select the
lowest three-way, platform and capability groups. Each valid option clears the other bits
in its group, stores that intermediate result, re-reads it, then ORs the choice.
The original signed comparison tree rejects unknown values and combinations.
Audio selection also manages allocation, construction, readiness, resource
loading and destruction. Failed initialization recursively selects option 256.

## Language, currency and interval

Language uses the external validator, selects a default currency, applies the
language and refreshes presentation. The original language-byte mapping for
0..10 is `0,24,9,10,14,20,6,11,16,18,21`; other signed byte values select zero.

The currency setter validates the signed low byte against 0..24 and stores only
that byte. It does **not** configure monetary factors. Applying preferences
compares the stored choice with the current display setting, then calls the
[complete configuration](native-money-configuration.md) when needed.

The interval setter accepts the signed low halfword from 7 through 365 inclusive.
Initialization defaults it to 30. Its user-facing meaning is not yet established;
it is not described as game speed. Loading reads its halfword directly without
calling this setter, so out-of-range saved values are preserved by the original
loader. This distinction is covered explicitly rather than silently hardened.

## Preference file, command line and devices

Initialization starts defaults and resource/language setup, then opens the
preference resource in `rb` mode. The first word must be `0x1553117c`. Eight
single-element reads cover magic (4), flags (4), language (1), currency (1),
interval (2), folder payload (256), video mode (4) and description (31).
Language and currency have their original validation branches. The 256-byte
folder payload is read into a temporary area; the initialized games folder stays
in the record. A mismatching device description resets video mode to -2.
Incomplete/invalid input closes and removes the preference resource. Complete
accepted input sets the load marker. These are modeled file-service effects.

Command-line substring checks recognize `-windowed`, `-nosound` and
`-defaultfont`. Display initialization uses 800 by 600 and the selected video
mode. Applying changed display settings can flip the window option and retry.
Language and audio changes run before opening the preference resource in `wb`
mode. The same eight fields are then written in order. An open failure or any
short write reports its original diagnostic, closes an opened handle and returns
zero. Full success closes, sets the marker and returns one.

Setters load the diagnostic flag pointer after their diagnostic services;
application caches it in its prologue. Audio ownership cells are cached by an
outer operation while a recursive option selection reloads its TOC slot. Explicit
dependency mutations exercise these differences, including the otherwise cold
outer cleanup after that slot is redirected.

## Evidence and limits

The focused CTest passes **2,339 comparisons**: 2,185 complete returns under the
declared models, 130 matching stops and 24 rejected deliberate corruptions.
Every root reaches all of its own original sites in complete starting
comparisons; all 1,444 sites are covered, with no structural exclusions. Eight
source/data guards pass. Two negative controls per root change the own result or
add and restore a memory effect, detecting differences even with equal final
memory images.

Comparisons include architecture and generated temporary residues, all mapped
bytes, ordered memory effects with instruction PCs, source order, dependency ABI
and selected-body instruction counts. Recursive original calls are recorded at
the same boundary as native children. Completed child counts include 131 default
selections, 1,045 option selections, 192 currency mappings, two apply calls and
157 video getters. **152 currency configurations** complete against their own
independent 834-instruction original reference; their sites are excluded from
the 1,444-site preference count. The already accepted money-text setting getter
is an explicit shared dependency on both sides.

Strings, diagnostics, platform/resource/language services, audio, video, allocation
and preference-file I/O are **explicit models**, with volatile register clobbers.
They do not prove actual platform behavior. Thirteen complete cases permit the
modeled process-exit service to return in order to compare the remaining body;
five stops use a terminating model. Neither count claims a natural return from
the production exit service. Reentrant pointer changes are deliberate modeled
effects, not observations of normal startup.

The corpus covers all language/currency bytes with upper-bit patterns, signed
interval boundaries and direct file loads, valid groups and neighboring invalid
options, platform thresholds, all eight short-read/write positions, bad magic,
device mismatches, command-line overrides, allocation/readiness failures, display
retry/exit paths, root/copy application, linked-pointer mutations, overlap,
required null faults, measured budget cuts, removed accessed bytes and dependency
interruptions. Shared call/runtime helpers and prior semantic bodies are unchanged.

Save/Load is **not retested**; performance is **not benchmarked**. The cumulative
Portuguese PDF explains the lifecycle, examples and the same acceptance limits.
