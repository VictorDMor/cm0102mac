# Native presentation and direct commands

## Delivery status

The candidate now includes native text and control drawing, explicitly added to
scope by the user on October 5 after rejecting bitmap-only enlargement. The user
confirmed physical clicks and legibility, and reported that mouse selections in
Game failed. The menu exception is fixed and covered by a mouse-event regression.

**Validated and installed.** Native 200% drawing was observed on
an external 1x display; 100%, 125% and Fit were subsequently observed on the
built-in Retina display. The user accepted the result. A stale Return key from
Load was found to close the next Save filename editor; accepted host commands
now clear both previous original keyboard fields. Live Load-to-Command-S editing
passed in the corrected candidate. The same final candidate completed normal
Save/exit/reopen/Load at 125%. The protected installer ran once after acceptance;
the installed executable and personal-file preservation were verified.

## Controls

`View > Interface Scale` offers `100% (Original)`, `125%`, `150%`, `200%` and
`Fit to Screen`. The checkmark represents the effective scale. A fixed choice
which exceeds the current usable screen is disabled. A previously saved choice
which stops fitting falls back to Fit; 100% is the default when it fits.
Changing scale preserves the current original screen, career and source bitmap.

The visible macOS game window matches the scaled 800×600 content: 800×600 at
100%, 1000×750 at 125%, 1200×900 at 150% and 1600×1200 at 200%, when that scale
fits the display. The original nonactivating desktop backdrop remains a logical
engine window, but its host window is hidden; it no longer covers the desktop
around the game. Repeated original ShowWindow requests retain that policy.
Overlays keep their relative position and scale. Fit uses the largest content
rectangle that fits the usable screen, retaining the original aspect ratio.
The October 11 installed build verified 100%, 125% and 150% window extents,
chooser/cancel and normal exit; the existing 100% preference was restored. Its
AppKit regression also checks repeated desktop ShowWindow suppression and exact
game frame/content dimensions. Save/Load was not retested for this change.

| Stable identifier | Native control / shortcut | Supported context |
| --- | --- | --- |
| `continue_game` | Game > Continue Game; Command-Return | Enabled original Continue control on a career screen |
| `save_game` | File > Save; Command-S; Game > Save Game… | Enabled original Save action on a career screen |
| `load_game` | Game > Load Game…; Command-O | Start screen only |
| `open_game_options` | Game > Game Options…; Command-comma | Start screen and supported career screens |
| `back_or_cancel` | Game > Back or Cancel; Command-[ | Original Back control, file/options dialog Cancel, or confirmation No |
| `set_interface_scale` | View > Interface Scale | Native presentation; child IDs `.100`, `.125`, `.150`, `.200`, `.fit` |

Load deliberately remains unavailable inside a career. The Mac original Restart
control (action 1071) opens a confirmation but its confirmed restart reaches
`0x1076f880`, an empty original routine. This was reproduced with keyboard and
original button input. The native menu explains: save and quit first, reopen,
then Load. It neither silently discards a career nor pretends Restart succeeded.
Rebuilding the original restart lifecycle is outside this presentation change.

The File Save shortcut takes precedence over the original Command-S sound toggle.
Ordinary letters, Return, Escape and filename editing retain original input.
The original file dialog accepts a typed name, Return to finish editing and
Return to confirm. Existing names can be edited with End and Backspace. The
original overwrite confirmation is retained. Command-[ cancels without requiring
pointer coordinates. Command-Q retains the original quit confirmation.

Automation targets the selected application's accessible menu title/identifier
or shortcut. It checks enabled state before one invocation and observes the
original dialog/result afterward. It must not retry uncertain Continue or Save
requests. There is no socket, server, arbitrary memory/function interface or
hidden fixed-coordinate click. The bitmap's other buttons remain ordinary game
controls, not a semantic automation API.

The disabled status item in Game distinguishes queued input, dispatch, original
file/options/confirmation interaction, return to an interactive screen, processing
and refusal. Dispatch is not proof of a completed save: confirm the original UI,
closed output file and subsequent Load. Commands are unavailable during processing
and incompatible modals. Enqueue immediately disables duplicates; autorepeat,
stale context and command key events older than one second are rejected. This
bounded interface supports the original single active UI frame; additional active
frames fail closed.

## Original dispatch contract

At the original interactive WaitNextEvent boundary (`LR 0x1076ff7c`, mask `ffff`),
`direct_commands.cpp` reads the active frame descriptor and its live control
count/table. It uses action identities and enabled flags, never text labels or
screen positions. Processing's `0400` poll does not accept commands. The context
identity covers handler, screen, action identities and flags; delivery rechecks
that context and refuses an occupied original input slot.

Continue, startup Load, Options and Back/Cancel select the original control index
at screen `+0x12a904` and its matching pressed-input flag. These are the same
input slots populated by the original control registration/input routines
`0x107232bc` and `0x107705cc`. Original picker `0x107193cc` consumes and clears the
selection exactly once, returning it to the existing screen handler. The native
callback only queues an identity; the engine is never reentered from AppKit.

Save uses the original File menu identity 2/item 1. The native menu bridge latches
that selection and returns a reserved EventRecord Point token through the normal
FindWindow/menu preparation/MenuSelect path. It is a dispatch token, not a physical
position. The original `0x107708c0` menu handler and original save serialization
execute normally. MenuSelect verifies installed/enabled identities after original
menu preparation and consumes the latch once. Enable/DisableMenuItem and HiliteMenu
support that path. GetMenu resolves the already installed menu for this narrow
preparation path; this does not implement general menu-resource loading.

The original Save/Quit global action is withdrawn at the next event boundary
only if it still equals the value owned by that native event. This prevents a
held global action from masking keyboard input in its nested dialog. Other UI
commands use the consumed control slot, avoiding action-number collisions between
parent and nested screens. Deleted original menu items tolerate later text updates.
Repeated SetMenuBar detaches reused AppKit submenus before rebuilding the bar and
restores the native scale/command menus.

## Coordinate and sampling model

The engine retains its original 800×600 bitmap and logical global rectangles.
The first activating 800×600 panel anchors the scene. Related windows share one
uniform affine transform; the larger nonactivating desktop background is excluded
from scene sizing. Native z-order is retained. Drawing ports, regions and
window_bounds remain in original logical coordinates.

The usable `NSScreen.visibleFrame` excludes the menu bar and Dock. Fit uses the
smaller width/height ratio and centers the scene; fixed choices retain the original
center where possible, constrained to that usable area. Screen/backing notifications
recompute the layout. Negative screen origins are valid. Alignment snaps inward
to backing pixels, while the user scale remains in AppKit points.

The original raster buffer remains byte-identical. Backgrounds, pictures, tiny
bitmap masks and deliberately blurred shadows use direct nearest-neighbor Quartz
sampling, without an intermediate enlarged image. Text and geometric controls
are drawn separately into the actual backing context, at the selected scale.
There is no second multiplication by backingScaleFactor. Fractional and Retina
scales resolve new vector edge coverage instead of enlarging the old text pixels.

Window mouse down/up/move, right/other buttons, drag and wheel positions pass
through the same inverse as continuous pointer queries. FindWindow tests logical
half-open rectangles in native front-to-back order. The native menu bar has
explicit host-coordinate precedence. Margins map outside the scene and are never
clamped to an edge button. Carbon event parameters retain their original button,
modifier and signed wheel contracts. The original cursor art/mask/hotspot remain
unchanged in host points; its location follows the inverse transform.

`CM_AOT_VALIDATION_EVENT_POINTER=1` remains an explicit test device. Its synthetic
pointer and the physical pointer use the same logical space, but their observations
are separate evidence. An isolated bundle can set `CMValidationEventPointer=false`
to exercise the production continuous pointer. Production does not need Validation.

## Native text and control drawing

`native_drawing.cpp` observes audited generated destination labels before the
original instructions execute. It never replaces the original routine or writes
game memory/registers. All original raster instructions and CopyBits conversions
still run. An optional memory observer tracks surface writes, including writes
which store the same value, so repainting correctly removes old vector ink.
Existing fixture observers retain exclusive ownership and disable this optional
presentation layer. The host display list and clean background are never saved
in the original career format.

The original outline-font engine decodes its own T2K fonts. At `0x107620d0`, the
observer retains the decoded quadratic contours, then associates them with the
original glyph cache and blit metrics. Quartz renders those original curves,
including their original advances, baselines, clipping and symbols. No original
font file, decoded contour payload or replacement font package enters Git.

The separate bitmap FNT branch has no native outline. It uses Core Text Arial /
Arial Narrow outlines fitted to the original glyph ink boxes and spacing. This
is an intentional presentation-only font substitution for that branch; it is
not a claim that a vector version of the original bitmap artwork was recovered.
The underlying original text, hit targets and line layout remain unchanged.
The original-font branch retains its own curves rather than using this fallback.

Lines, filled rectangles and circular controls use Quartz paths. CopyBits carries
the host display list alongside the unchanged original pixels, respecting clips,
nonzero port origins and overlapping copies. Original saved-under dialogs,
startup fades, grayscale and modal dimming retain the vector layer. Saved image
objects are checked against their original dimensions, pixel pointer and bytes
before reuse: the game can overwrite an object without freeing it. A stale host
copy must be discarded, or text from a prior screen would reappear in Save.

The generator pins each observation instruction to a digest of the private
original export. Tests verify hooks are at destination labels (including same-page
calls), preserve original instructions and reject altered observation points.
These are presentation observations, not additions to the semantic manifest.

Native menu callbacks query `NSEvent.isARepeat` only for keyboard events. AppKit
raises an exception for this property on mouse events, which explained why the
user's Game-menu clicks failed while keyboard shortcuts worked. Both menu paths
are covered by a real `NSEventTypeLeftMouseUp` regression. Host shortcuts consume
both key-down and matching key-up, even if Command is released first.
Accepted commands also clear the preceding original character and hardware-key
fields after validation. The original file handler can consume Return while
leaving hardware code 36, which would immediately end the next filename editor
at `0x1071b70c`. Rejected commands retain both fields, and subsequent queued
keyboard events are preserved. A fresh Load followed immediately by Command-S
and the first typed letter reproduces the regression and verifies the correction.

## Preference and isolation

Production stores only Version 1 and InterfaceScale in
`~/Library/Application Support/CM 01-02 Native/Presentation.plist`, next to Runtime.
Candidates use `<private runtime parent>/Presentation.plist`; standalone component
tests use temporary directories or no persistent path. Values are validated before
use, with a 4 KiB file-size bound. Missing/corrupt/unsupported values use 100% or Fit.
Atomic writes cannot alter a save; a failed write leaves the current scale usable
and reports failure in the application log.

No calendar, RNG, match, scheduler, serialization, background selector or semantic
replacement was added or changed. The manifest remains 37 entries: 34 Date and
three RNG. The original asymmetric leap-year difference contract is retained.
The previously waived long Date campaign is not reopened or represented as passed.

## Live presentation and input evidence

Private evidence is under `.local/presentation-commands`; screenshots, run logs,
saves and proprietary inputs are excluded from Git. The pre-change installed app
was copied to a fresh private reference before any replacement. Its source bitmap
captures are not treated as physical presentation evidence.

The external MSI display was observed at 2560×1440 AppKit points, backing 1×,
usable AppKit frame `(0,67,2560,1343)`. At 200%, the game window was 1600×1200
pixels. Native startup, career, Options, modal dimming, Save and cancellation were
observed. The reused-image defect was reproduced in candidate 04 and absent in 05.
The user accepted the native text/control result on October 5.

After the external monitor was disconnected, candidate 05 recovered its stored
200% choice to Fit on the built-in Retina display without rewriting the preference.
The built-in display is 1800×1169 AppKit points, usable frame
`(0,67,1800,1063)` in AppKit coordinates, backing 2×. Actual window captures of
the native drawing layer show:

| Effective choice | Window size in AppKit points | Screenshot pixels |
| --- | --- | --- |
| 100% | 800×600 | 1600×1200 |
| 125% | 1000×750 | 2000×1500 |
| Fit | approximately 1418×1063 | 2836×2126 |

The 200% option is disabled when it cannot fit. The scene stays proportional and
its current date/phase survives scale changes. The original Futurista preference
is retained. Live evidence covers these two displays and recovery after removal;
other screen arrangements remain covered by geometry tests rather than live proof.

The user directly confirmed physical clicks and legibility. CUA's production
pointer check remains distinct: its synthetic click can disagree with the actual
desktop pointer position. Bitmap-navigation evidence therefore uses an explicitly
signed event-pointer variant of the same source, sharing one canonical private
Runtime with only one game process active. This exercised profile and contract
tabs, manager dropdowns, squad and club calendar. Central commands use native
menus/shortcuts without the Validation coordinate helper. Neither synthetic
navigation nor a queued command is presented as physical-click proof.

## Final keyboard and persistence acceptance

Candidate `presentation-native-text-10` uses the production pointer path and the
final command-key correction. A fresh Command-O Load of the preferred reference,
Return confirmations, immediate Command-S and a typed `X` verified that the new
Save editor keeps focus. This regression was cancelled without overwriting the
reference. Menu Save had separately completed original serialization in candidate
06, creating `Native-UI-Verified.sav`.

In candidate 10, one Game > Continue Game selection moved from the Brazil U21
news to Iranildo's news without changing September 4 NTE. Command-S opened the
original filename editor; End/Backspace and typing selected the new private name
`Native-Final`. Two Return confirmations completed original serialization and
the successful temporary-file rename. `Native-Final.sav` is 34,877,925 bytes,
SHA-256 `4d269a00a1ff45825819e93b9aa54d9878eef9b069a363e3e0998898d9b090e8`.
Command-Q and the original confirmation exited normally; process absence was
verified before reopening the same signed candidate. Its 125% preference returned.
Command-O and the typed filename loaded the new save, restoring September 4 NTE,
Victor Moraes and the next unread news. Game > Game Options opened the original
dialog and Command-[ cancelled. The process then exited normally again.

Before the final Save, `presentation-native-fields-11` reloaded the unchanged
reference using the same source binary and recorded these fields:

- September 4, 2002 NTE; Victor Moraes / Cruzeiro.
- Goias 0:3 on August 31; pending Coritiba, Santos and Colo Colo on September 6, 8
  and 11; Brazil 5:1 Uruguay in Alex's news.
- Luisao: born February 13, 1981, age 21, one cap, condition 85%, form 7-9-8-6-8,
  morale Bom, official 26(4), one goal, two assists, average 7.20; noncompetitive
  two appearances, average 7.50.
- Full-time contract, BRL 8,750/week, start N/D, linked, expires December 24, 2004,
  squad rotation, no bonuses or clauses.

After the final lifecycle, the same fields variant loaded `Native-Final` and
verified date/phase, manager/club, results/fixtures, birth/age/caps, condition/form,
statistics and contract. Morale was Muito Bom rather than the initial Bom. A
directed control loaded the unchanged reference again, invoked one Continue and
observed Muito Bom before any Save, matching the post-load value. The initial
pre-Continue observation is therefore not reported as an unchanged morale value.
Both comparison runs exited without saving. Screenshots and compact field records
are retained separately from the production-pointer lifecycle evidence.

The earlier scale-only build had already completed two keyboard persistence
cycles, but those are historical evidence, not a substitute for native-drawing
acceptance. The user later played its private career to September 6; that changed
save has been preserved. The current candidate starts from a separate copy of the
unchanged September 4 reference, SHA-256
`44dd1f372cf9f568f63b1f2e5942c5a68f357cb816b1b2c9b58941c32580e4a3`.
The immutable LFS reference remains SHA-256
`82c504f46ce54a723137dbf42c005d458a4d7b18a9abab4c9f92627aa94f08fb`.

## Automated checks and preservation

The final isolated run passed **44/44 CTest in 65.84 seconds**. A preceding run
made while driving the live game failed the UI fixture's empty-queue assertion;
that fixture passed alone, then the full suite passed with the game closed.
The final command-key tests verify that accepted input clears the previous Return,
rejected input preserves it, and the original picker consumes one selected control.

`native_vector_drawing` compares actual original raster execution with and without
observation: original pixels, registers and instruction counts match. It covers
same-value repaint occlusion, CopyBits clips/nonzero origins, saved-under restore,
reused-image invalidation, fades/dimming, original quadratic paths and newly
resolved antialiasing at 1×/2×/4×. Focused UBSAN passed for the observer, vector
rasterizer and checks; linked existing engine libraries were not instrumented.

Python discovery ran 42 tests with four optional skips. All four groups were then
run explicitly and passed: 5 control-cursor, 6 match presentation, 7 unsigned-division and 5 local-temporary tests, including all 1,953,157 original instructions
and 6,437,244 operations. The generator retained 37 semantic replacements, 1,922
pages and zero missing instructions. No new oracle or decompiler export was made.

Presentation/input checks cover negative origins, overlays, half-open bounds,
margins, Retina independence, constrained displays, checked menu state, repeated
menus, preference reopening/corruption/isolation, immutable original bitmap
capture, stale/modal/duplicate refusal, keyboard/menu parity and actual mouse
menu events. `cm-menu-checks` separately verified five original menus and 30 item
texts. The previously waived long Date campaign remains waived, not passed.

The final preinstall and postinstall preservation receipts passed: 809 personal Game files,
five Runtime files and 44 Runs files retain their content, resource forks,
extended attributes, modes, mtime and flags. All 664 protected private identities
and the previous installed reference's full inventory match. All 648 protected
private metadata records also match. Ten pending handoff
prompts remain excluded. Historical references, unique saves and fixtures are
retained; candidates share a canonical Runtime to avoid multiplying saves.

## Installation and publication

`tools/install_native.py` ran once on October 5, 2026, from
15:12:03.710 to 15:12:04.460 UTC (12:12 local time). It verified the existing game
data and replaced the app. The installed executable is ARM64 and its plist has
no validation keys or diagnostic environment. All four bundled original payloads
match `.local/recomp-input`. Personal Game/Runtime/Runs were not launched or edited;
no production presentation preference was created by validation.

| Artifact | Executable SHA-256 |
| --- | --- |
| Final source and installed app | `e6907400e9aa29896c061af4144efa99d5141fe39f119a025830f5b72bb24c41` |
| Signed final candidate 10 | `645402b3ced753b25bbc45202dcddb580a7b81da81949d14a40ca06e39628247` |
| Signed event-pointer field variant 11 | `055f35451f00d0549fbcce3f04cb2e45df7d0041d5bfc822cd703c4652672e01` |
| Previous installed app, retained private reference | `1b84152cc5ddad11d43fa2b3fc7c0293b20d2dcf0a8008ee0750843867a0d9b6` |

A short postinstall check used a signed copy of the installed bundle with only
private runtime/bundle isolation and the production pointer path. It exposed no
Validation menu. Game command availability, menu Options/keyboard Cancel,
125% to 100% scale change and keyboard Load/Cancel all worked. It exited normally;
the installed executable remained unchanged. This private clone's signed hash is
`9f015269dda15c2456c66c7512f8c8198477d14f7cbe4925233e0b2728ec7078`.

Signing private bundle identities changes executable hashes; those are not
confused with the unchanged source/installed identity. Repository delivery excludes
private evidence, original payloads, saves and the ten pending handoff prompts.
