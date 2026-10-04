# Semantic Mac Date subsystem reconstruction

Status: **installed on October 4, 2026, with explicitly accepted weekly test
limits**. Production uses all 34 semantic Date entries: 26 newly reconstructed
entries and eight preserved entries. The user requested proceeding with delivery
and finding any remaining bugs during use. Earlier reports retain their own
acceptance evidence; their passing results are not new subsystem checks.

## Verified starting point

On October 3, 2026, the checkout and a fresh `origin/main` query both identified
`b6ce6ceb7c961a1f0d4d41f032b21117a4ae5cde`. Seven untracked Date handoff prompts
were present and are excluded from product delivery. No applicable `AGENTS.md`
was found. The immutable LFS save and private original inputs remain protected.

The read-only instruction audit reconfirmed all 26 fingerprints, 2,898 indexed
instructions, direct caller counts, all indexed exits, and no external indexed
flow into a pending body's interior. No external direct branch/fallthrough leaves
these bodies except their dependency calls. This does not exhaust indirect flow
or establish runtime coverage. Private complete records and caller lists are in
`.local/date-subsystem-reconstruction/contract-inventory.json`.

A new executable installed reference was copied with `ditto --rsrc --extattr` to
`.local/date-subsystem-reconstruction/reference/installed.app`. Its complete file
and metadata inventory matches the installed bundle. Source, installed and new
reference executables are ARM64 and hash to
`7284838b3e6d74785a6969d64bf2d19619ad2cdc3e5e7c0601e6e551a97db063`.
The initial personal inventory observes 809 Game / 5 Runtime / 43 Runs files,
with content, resource forks, FinderInfo, all xattrs, modes, mtime and flags.
858 protected file identities were recorded. Counts are observations, not fixed
requirements. Existing references and checkpoints remain immutable.

## Contracts and composition

All dates retain the five-byte Mac layout: big-endian signed16 ordinal day,
unsigned16 year and byte leap flag. Difference's deferred asymmetric ascending
year behavior, pre-epoch negative weekday remainders, invalid values, mutable
tables and ordered alias effects remain part of the legacy contract.

| New entry | Verified contract | Date dependencies |
| --- | --- | --- |
| 0x107156b0 | Ordinal construction; raw day/year stores; destination return | leap |
| 0x107156f0 | Civil A; signed day/month/request validation; destination return | leap, adjustment |
| 0x10715880 | Ordered day=40, year=1972, flag=1 initialization | none |
| 0x107158a0 | Civil B; distinct diagnostics and residual return | leap, adjustment |
| 0x10715a30 | Nearest weekday; one correction by seven; ordered copies | weekday, subtract |
| 0x10715bc0 | Numeric date and optional phase; signed16 capacity >=9 | phase A |
| 0x10715e00 | Short numeric date; language-dependent argument order; capacity >=9 | phase A |
| 0x10716130 | Weekday and short numeric date; capacity >=18 | phase A, weekday B |
| 0x107164f0 | Abbreviated names, ordinal and year; capacity >=18 | phase A, month A, weekday A |
| 0x107168a0 | Abbreviated month, ordinal and year; capacity >=18 | phase A, month A, day-of-month |
| 0x10716b00 | Abbreviated names and ordinal without year; capacity >=18 | phase A, month A, weekday A |
| 0x10716eb0 | Full names, ordinal and year; capacity >=50 | phase B, month B, weekday B |
| 0x10717270 | Full names and ordinal without year; capacity >=50 | phase B, month B, weekday B |
| 0x10717620 | Full month, ordinal and year; capacity >=50 | phase B, month B |
| 0x10717a10 | Unsigned8 weeks times seven; own scratch and guard | leap |
| 0x10717cb0 | Unsigned8 weeks times seven backward; own scratch and guard | leap |
| 0x10717dd0 | Date-record interval; by-value second date, borrow and orientation | initialization, later, recursion, leap |
| 0x10718440 | Independent unsigned16 leap classification; four exits | none |
| 0x107184d0 | Phase byte 0/1/2; own text scratch | none |
| 0x10718570 | Phase byte and signed byte mode; own text scratch | none |
| 0x10718650 | Repeated language queries; language-specific phase classification | none |
| 0x10718700 | Signed16 weekday; mutable dispatch table; abbreviated text | none |
| 0x107187d0 | Signed16 weekday; mutable dispatch table; full text | none |
| 0x107188a0 | Signed8 month; mutable dispatch table; abbreviated text | none |
| 0x107189d0 | Signed8 month; mutable dispatch table; full text | none |
| 0x10718af0 | Signed8 first parameter / signed16 second parameter; presentation grammar predicate and carry effects | none |

The nine formatters perform ascending table scans with late day reloads. Their
invalid-day behavior differs from the independent reverse month conversion:
an unrecognized month can retain an incoming nonvolatile register. The semantic
implementation preserves this behavior. Several compute weekday inline;
that arithmetic is not an independent weekday entry call.

Language/template/string/ordinal services `0x10727880`, `0x10727890`,
`0x1077dbf0`, `0x107467a0`, diagnostic services `0x10739890`, `0x10768810`,
`0x1077b720`, `0x10769b60`, and record helper `0x101fe9f0` remain original
dependencies. Their call boundaries and effects are preserved. No Langlib,
scheduler, competition or player subsystem reconstruction is claimed.

| Generated dependency | Preserved Date-side calling contract |
| --- | --- |
| 0x10727880 | Current language query; its signed8 result is tested at each original query boundary, including repeated queries |
| 0x10727890 | r3 destination, r4 original template, original r5..r10 and stack variadic arguments; returned residue and mutable scratches remain visible |
| 0x1077dbf0 | r3 destination/r4 source for original fallback text copying; no new Unicode conversion or replacement text |
| 0x107467a0 | Original ordinal value in r3 and mode=1 in r4; returned text pointer feeds the original template call |
| 0x10739890 / 0x10768810 | Original diagnostic preparation with r3 template base, r4/r5 zero and frame scratch pointers in r6/r7; next service consumes its returned state |
| 0x1077b720 / 0x10769b60 | Original diagnostic or special-language formatting arguments, frame scratch, source-line parameter and original reporting mode; stores and callback/import effects are compared |
| 0x101fe9f0 | Original final interval copy/helper branch with r3 destination/r4 mutable Date scratch; full return/ABI/memory effects remain part of the caller comparison |

Calls use direct translated ABI execution through the generated dispatcher,
with the original LR and context TOC. They do not use callback marshalling that
would restore away effects. The Date implementation owns parameter selection,
validation, table traversal, arithmetic, branches, spills and output ordering.
The original dependencies own their text assets, imports, callbacks and allocation.

## Direct implementation and final checks

All 26 new semantic bodies are implemented and hooked at their production
destinations. The final 2,494 core original/semantic cases pass full registers,
temporaries, PC,
all mapped memory, allocator/import state and ordered reads/writes/map/clear.
This includes 1,080 record interval cases, high bits, lazy guards, partial and
frame aliases, adjusted weekday extremes, modified civil thresholds, February 31,
unsigned8 weeks and pre-epoch years. The same 2,494 cases also passed with
semantic construction/adjustment/weeks/interval dependencies composed.

The final 4,377 presentation original/semantic cases pass the same complete
ABI/memory and ordered access comparisons. Cases include nine formatter contracts, signed
capacity thresholds and null/overlapping buffers, years 0/1599/1600/1601 and
centuries/extremes, invalid days and flags, phases, modified and relocated
threshold/name dispatch tables, fallback targets, and isolated language queries.
These are direct contract checks, including semantic composition. Real UI and
persistence evidence is recorded separately below. Modal diagnostics are not
claimed from these cases: fixtures select the original nonmodal diagnostic branch. No absent
locale's asset translation was fabricated or claimed.

The composed check exposed extra/reordered reads in the previously delivered
add/subtract adapter. The three arithmetic bodies now share the precise Date
call boundary implementation: real leap subcalls enter their independent
semantic hook, with original link/context/register residue. Scratch initialization
in add/subtract/difference is inline in the originals and remains inline; it
creates no invented initialization calls. The asymmetric difference year loop
continues to classify current-1 in both orientations. The final acceptance suite
reverified all eight delivered entries after this composition change.

The final diagnostic State layout now includes separate original/modern counts
and selective original flags for the 26 entries. A separate optional memory
observer preserves Date observation across native import transcript observer
switches. Production read observation remains disabled. New CMDAT001 begin/end
chunks contain nesting identity, full registers/temporaries, result bytes,
allocator/mapping/import state and ordered read/write/map/clear effects. The
four historical streams retain their formats and suppress historical nested
edges while still allowing modern children of an original parent to execute.

The final State layout was rebuilt across all production objects. Its header
SHA-256 is `cb1d78314f205defefe42fd2274f8f0b9a461c377a51c7cbd547250949534e8f`.
The final link audit finds 1,974 State-dependent objects and 22 other objects,
with no missing or stale objects. Production destination checks pass 2,494
construction cases and 4,377 presentation cases; 104 composed CMDAT001 agreements,
52 own-result rejections and original/semantic callbacks for all 26 entries pass.
The complete suite passes 39/39 CTest. Final Python discovery has 39 tests:
35 pass and four optional groups are skipped; those four groups pass their
explicit audit runs. The final Python rerun includes the lossless storage tests.
Generated-temporary UBSAN passes 199,742 differential executions from 2,050
actual patterns and 46 P-code operation kinds. Focused UBSAN passes all ten
instrumented harnesses with empty sanitizer diagnostics. The first report write
failed after disk exhaustion; the ten
checks were rerun successfully before the final report was accepted. Their
binaries and raw private oracles were then archived losslessly, with decompressed
SHA-256 verification, to release space.

## Accepted validation decision and limits

The user explicitly requested stopping the prolonged tests and proceeding with
delivery. Implementation scope remains all 34 entries. Day and ordering pass
their complete campaigns. The weekly original capture and both fully semantic
replays completed, including their per-run identity checks, before the checker
entered its first historical calendar baseline. This is observed sequential
checker phase evidence; no final aggregate weekly report was produced.

The owned baseline was intentionally stopped with SIGTERM (native exit -15;
checker exit 1). This termination is neither a native regression nor a passing
campaign. The seven complete weekly baselines, remaining negative diagnostic
acceptance and final weekly mode/count matrix remain unverified and were waived
by the user. Their table cells say **not recorded**. No weekly own-call counts,
negative acceptance or full-campaign PASS are inferred from the two replays.

The final 34-entry production manifest, final State rebuild, direct ABI/effect checks,
composition, all 26 own negatives and real caller coverage, eight delivered-entry
regressions, complete suites and focused/generated UBSAN are verified below.
The two real Save/normal exit/process absence/exact reopen/Load cycles also pass.

The accepted evidence is reconciled in the private
`acceptance-accepted-limited-01.json`, with the exact user decision and explicit
weekly limits. Personal/historical inventories passed immediately before and
after the single installation; separate bundle identities are recorded below.
The product delivery excludes the seven handoff prompts and private artifacts.
The private delivery receipt records the reviewed diff and freshly queried remote
revision. Generated temporary payloads are cleaned at the user's request, as
described below; retained receipts distinguish completed checks from waived ones.

## Per-entry ABI and direct-case closure

The following counts are the final production-destination comparisons, not
synthetic gameplay counts. Together they comprise 6,871 complete original versus
semantic cases. Each of the 26 entries also passes two isolated own-result
negative checks (52 total), indirect calls and callbacks. The comparator checks
all register and temporary bytes, CR/XER, LR/CTR, context TOC, PC, stack, all mapped
bytes, allocation/import state, and ordered read/write/map/clear operations.
The frame column names the bytes allocated and the nonvolatile registers spilled
and restored; spill order and the stmw/lmw cursor remain observable.

| Entry | Cases | Return / own effects | Frame / preserved registers |
| --- | ---: | --- | --- |
| 0x107156b0 | 224 | r3=destination; day then year then independent leap flag | 0x40 / r31 |
| 0x107156f0 | 301 | r3=destination; diagnostics, year/flag/day stores, optional in-place adjustment | 0xb0 / r24..r31 |
| 0x10715880 | 9 | r3 unchanged; day=40, year=1972, flag=1, in order; r0=1/r4=1972 | leaf |
| 0x107158a0 | 301 | Residual r3 from leap/table or adjustment; variant-specific diagnostics; no destination-return rewrite | 0xb0 / r24..r31 |
| 0x10715a30 | 237 | Equal branch retains weekday result in r3; other branches retain subtraction residue; year/day/flag ordered copy | 0x60 / r29..r31 |
| 0x10715bc0 | 461 | r3=1 on success, 0 after diagnostics; numeric full-year output, optional phase | 0xa0 / r26..r31 |
| 0x10715e00 | 461 | r3=1/0; short-year output; repeated language service and original argument ordering | 0xb0 / r25..r31 |
| 0x10716130 | 461 | r3=1/0; weekday plus short numeric date; inline weekday arithmetic and name scratch | 0xb0 / r25..r31 |
| 0x107164f0 | 461 | r3=1/0; abbreviated weekday/month, ordinal and year; helper scratches | 0xb0 / r25..r31 |
| 0x107168a0 | 461 | r3=1/0; abbreviated month, ordinal and year; independent day extraction | 0xa0 / r26..r31 |
| 0x10716b00 | 461 | r3=1/0; abbreviated weekday/month and ordinal without year | 0xa0 / r26..r31 |
| 0x10716eb0 | 461 | r3=1/0; full weekday/month, ordinal and year; original variadic stack arguments | 0xb0 / r25..r31 |
| 0x10717270 | 461 | r3=1/0; full weekday/month and ordinal without year | 0xa0 / r26..r31 |
| 0x10717620 | 461 | r3=1/0; full month, ordinal and year; optional full phase | 0xa0 / r26..r31 |
| 0x10717a10 | 175 | Residual r3; unsigned8 weeks times seven; mutable scratch/guard and ordered output | 0x50 / r29..r31 |
| 0x10717cb0 | 175 | Residual r3; backward weeks; distinct scratch/guard and previous-year leap subcalls | 0x50 / r29..r31 |
| 0x10717dd0 | 1080 | Residual branch-dependent r3; day/year interval record; copied legacy flag, by-value argument padding and recursion | 0x70 / r28..r31 |
| 0x10718440 | 42 | r3=unsigned16 leap classification; four exits preserve distinct r0/r4/r5, CR0 and XER.CA effects | leaf |
| 0x107184d0 | 15 | r3=mutable phase scratch pointer; byte phase, fallback text and original template residue | 0x40 / r31 |
| 0x10718570 | 15 | r3=distinct phase scratch pointer; byte phase/signed-byte mode selects original full phase template | 0x40 / r31 |
| 0x10718650 | 30 | r3=language-specific phase classification; repeated language calls remain ordered | 0x40 / r31 |
| 0x10718700 | 15 | r3=abbreviated weekday scratch; signed16 index, mutable dispatch/CTR and fallback | 0x40 / r31 |
| 0x107187d0 | 15 | r3=full weekday scratch; distinct mutable dispatch and templates | 0x40 / r31 |
| 0x107188a0 | 20 | r3=abbreviated month scratch; signed8 index, mutable dispatch and fallback | 0x40 / r31 |
| 0x107189d0 | 20 | r3=full month scratch; distinct mutable dispatch and templates | 0x40 / r31 |
| 0x10718af0 | 48 | r3=0/1; signed8 language/signed16 weekday grammar rule; distinct CR0/carry paths | leaf |

Formatter counts each include 324 year/day/flag combinations, five phases, 60
capacity/buffer cases, 12 mutable-threshold cases and 60 scratch/output overlap
cases. The two civil constructors each combine 252 core cases with 49 original
nonmodal diagnostic cases. Arbitrary mutated name dispatch targets outside the
audited case/epilogue/return domain fail explicitly; they never enter an original
Date body as a semantic fallback. Concurrent mutation, unmapped/wrapping memory
and absent locales are outside the demonstrated domain.

The closed production manifest has 34 entries: the 26 above plus add
0x10717890, subtract 0x10717b20, difference 0x10718130, later 0x10331860,
before 0x103317f0, day-of-month 0x107182f0, month 0x10718360 and weekday
0x10715b20. Destination hooks cover same-page gotos, cross-page dispatch,
indirect calls and callbacks. Normal production selects the semantic bodies;
private original selection is retained for comparisons and capture.

## Rare real caller checkpoint

After the second verified Load, normal UI navigation in `app.T76m9A` exercised
five entries missing from the ordering fixture. No career date was edited.

| Entry | Actual flow / caller | Observed output | Own semantic calls in this run |
| --- | --- | --- | ---: |
| 0x10718af0 | Existing August 31 Nenem injury news / 0x102c4c18 | language=9, weekday=5, return=0 | 1 |
| 0x107168a0 | Marcelo Cruz (Coritiba) loan draft / 0x1042d520 | default end date `4 Dez 2002` | 1 |
| 0x10715bc0 | Coach note, normal One Week reminder, Contract panel / 0x10700a84 | `11.09.2002` | 2 |
| 0x107164f0 | Existing Hall of Fame manager record / 0x10340768 | `Sex 2 Out 2026` | 1 |
| 0x10717270 | Brazilian First Division Fixtures via club calendar / 0x1006a8c8 | `Sexta-Feira 6 Setembro` | 1 |

The loan draft was cancelled without sending an offer. The note used the normal
One Week choice, without editing its date, and exists only in the private runtime.
The career remained Wednesday September 4 night. Language classification
0x10718650 also has two own calls in this run; its observed actual caller is
0x1028b0b0/0x1028b0c0. These samples and call summaries are private evidence,
separate from complete direct ABI comparisons and integrated fixture streams.
Full-month/year formatter 0x10717620 is now exercised in the additional isolated
England career described below. All 26 pending entries have their own real calls.

The new private `date-subsystem-rare-01` career was created through New Game,
England only, with Date Validation managing Arsenal. Its default start was
Saturday July 14, 2001 morning. The normal Calendar Arrange Tournament flow
created the default Taça CM as a cup; the first available date selected July 21
and its paired July 22. Ajax, Anderlecht and Alavés were invited through the UI.
One normal Continue stopped at July 14 night with accepted-invitation news.
Reading Ajax's acceptance calls 0x10717620 at caller 0x102c0934, returns 1 and
outputs `21 Julho 2001`. Run `app.l0f0wh` has one own semantic call. No career
date, snapshot, RNG or save bytes were edited. Original Save Game wrote
`DateModule-Tournament-14Jul200.sav`; the original filename input truncated the
requested 2001 suffix to 200. Its SHA-256 is
`806fe1c2e4caa8559be06a7e47d685d43eb683a78e68b17c6aac1acfd049da77`.
Normal exit completed and process absence was verified. This additional flow
resolves a specific coverage gap; it is not a new season acceptance claim.

A separate fresh signed ARM64 candidate, `date-subsystem-original-ui-01`,
selects all 34 original Date entries via the existing season-reference mode.
It loaded exactly the new tournament save without Continue or a whole-app Date
capture. The header date/weekday/phase and complete Ajax acceptance text match
the semantic candidate. Its own original sample at the same caller has identical
return and output bytes. The exit summary verifies zero semantic Date calls.
The same original-only candidate then loaded the exact September 4 checkpoint.
Its Wednesday/night header, Cruzeiro position and fixtures/results, Luisao's
profile and contract, and the saved U21 news match the semantic candidate.
Normal navigation also reproduces the loan draft's `4 Dez 2002`, the coach
note's `11.09.2002`, Hall of Fame's `Sex 2 Out 2026`, competition Fixtures'
`Sexta-Feira 6 Setembro`, and the August 31 injury news's weekday grammar.
For each of the six rare entries, an own original sample at the same actual
caller agrees in return and complete output bytes with its semantic sample.
Both original-only runs exited normally; process absence was verified before
reopening. No Continue or oracle recording occurred in this comparison app.
Private closure is `ui-original-comparisons-01.json`.

Independent Gregorian calculations approve 394 valid live samples across
`app.pg7G2e`, `app.mL5Otx`, `app.T76m9A`, `app.Lh9kqF` and `app.QKZpyo`:
80 leap classifications, 80 civil A, 80 civil B, 74 ordinal constructions and
80 adjusted output weekdays/flags. The first-16 per-entry sample cap does not
retain matching civil parent inputs for every early adjustment; that limitation
is not treated as proof of its nearest-date choice.

Instead, 32 actual adjustment inputs were reconstructed from their first ordered
reads in the original ordering fixture's CMDAT001 stream. Independent arithmetic
verifies their output dates as the nearest requested weekday in +/-3 days.
Observed deltas are -2 (20 cases), 0 (three), +1 (one), +2 (two), +3 (six).
The complete candidate campaign agrees with these original bytes and effects.
This prefix extraction is sample analysis; full EOF and CRC acceptance comes
from the completed integrated campaign. Other deltas and invalid inputs remain
delimited by the direct contract checks. Private records are
`live-independent-sample-checks-01.json` and
`real-adjustment-independent-checks-01.json`.

## Integrated campaign checkpoint (October 3, 2026)

The fixed campaign executable is private at
`.local/date-subsystem-reconstruction/campaign-bin-01/cm-season-reference`,
SHA-256 `0b2d3bc4d300b4055e57a7b34630900b7ec8fdba012794123b2afc81af7f3fcc`.
The ordering campaign passes completely, including two strict original runs,
all-original capture, seven historical/subsystem baselines, two fully semantic
candidates, every covered own-entry negative, and strict EOF/checksum checks.
Its original CMDAT001 stream has 19 exercised pending entries and seven zeros;
those zeros are resolved by the separate real UI flows above, not accepted as
fixture coverage. Full ABI snapshots make these streams large. Parallel day/week
captures failed when the filesystem filled; these are not acceptance results.
Their failed Date files were removed only after recording identities in
`aborted-captures-02.json`. The replacement day campaign passes all 35 runs,
including 19 own-entry negatives, two strict originals, seven baselines and
two semantic candidates. Its 1,583,937 own pending-entry calls are recorded
separately from other evidence. Historical streams, fixtures and protection
inventories have not been removed or changed.
Earlier raw characterization files were archived losslessly with gzip, verified
by their decompressed SHA-256, to release space without reducing test contents.

The weekly regular-gzip capture exceeded local storage. Its 7,200-second limit
expired while the writer was paused; this incomplete attempt is not acceptance
evidence. All its available gzip bytes were retained losslessly in verified xz
chunks, with original size, SHA-256, mode and mtime. Reassembling every chunk
reproduces the complete retained gzip SHA-256; the working partial file was
released only after that verification.
The replacement weekly campaign uses `--archive-date-oracle`: the unchanged native executable
writes the identical raw gzip bytes to a fresh FIFO, and the host stores those
bytes losslessly in xz. Every native comparison receives the original gzip bytes
through a fresh FIFO; its full input/result/ABI/effect, nesting, CRC and EOF checks
remain unchanged. The checker still recomputes the complete raw gzip SHA-256
after every run, and additionally pins the physical xz archive's SHA-256.
The first archived weekly attempt also expired at the default 7,200-second
host limit while its capture was still progressing. Its available bytes were
privately retained in the closed xz container; it is not acceptance evidence.
The replacement uses `--date-oracle-timeout 21600`, a finite six-hour host
budget for each complete Date capture/replay. The executable, recorded contents,
comparison fields, instruction/transcript bounds, checksums, EOF and all
controls remain unchanged. The default host limit remains 7,200 seconds.
The revised Python discovery passes 35 tests with the same four optional skips.
The original regular-file versus FIFO attempt agrees for every retained
22,726,131,712 gzip byte, with the same prefix SHA-256. This is a separate
storage-path identity check, not complete Date framing/CRC/EOF acceptance.
The final weekly archive independently passes the same complete retained-prefix
comparison in `week-archive-prefix-agreement-02.json`: 22,726,131,712 bytes,
SHA-256 `b366161821120faf514a4a83034730879ebfdcd9ca942d2a1e4f7f762a024d1f`.
Its original capture completed CMDAT001 framing/nesting and compressed-stream
EOF/checksum validation before the first semantic replay started. All four weekly
historical streams independently match their prior byte identities in
`weekly-original-capture-checkpoint-05.json`. This capture checkpoint alone does
not establish native replay comparisons or covered negative acceptance.
The checker completed its first semantic replay and post-run identity checks
before launching the second semantic replay.
`weekly-first-semantic-replay-checkpoint-05.json` records this intermediate phase;
the complete mode/count matrix and remaining controls were still pending.
The second native semantic execution ended while the checker was still active,
recorded separately in `weekly-second-semantic-replay-checkpoint-05.json`.
This phase evidence does not replace the final aggregate campaign report.
The checker subsequently reached the first historical calendar baseline after
both semantic replays and their complete per-run identity checks completed,
recorded in `weekly-semantic-pair-checkpoint-05.json`. The first baseline was then
intentionally stopped at the user's request. Negative diagnostic acceptance and
the complete baseline/count matrix are unverified and explicitly waived.
`weekly-user-stop-completed-05.json` records the signal, exits, owned process
absence and FIFO cleanup. The error file describes that intentional termination;
the empty aggregate report is not presented as a PASS.

The four historical streams keep their original regular-gzip paths and encodings.
No native source, State layout, snapshot, RNG or original Date body was changed
for this storage path. Storage tests cover exact bytes, full gzip EOF/checksum,
outer xz corruption/truncation, early negative closure, fresh subsequent replay
and a consumer failure before opening the FIFO. Default regular storage remains
available. The revised Python discovery passes as described above.

Caller evidence corrects one candidate role: every one of the nine indexed
callers of `0x10718af0` supplies the current language service result and weekday,
then passes its result to a language template. Thus the observed production
role is a presentation grammar predicate, not a calendar month rule. The raw
signed8/signed16 rule remains identical: first=9 returns second<5 or second>=7;
otherwise first=3 returns true. No scheduler or competition behavior is added.
Private caller contexts are retained in `rare-callers-01`.

The fresh signed ARM64 candidate is
`.local/performance/date-subsystem-live-01/CM Validation date-subsystem-live-01.app`,
source executable SHA-256
`a35a303f441d352d61e0ab0a8e154e06f14f747461310c9e6ac28ce36abaf1e5`,
signed executable SHA-256
`6bfc62d0f5df558571d84ff8eaa79941102ba3d35e4b1613cd1b31e91f6646d8`.
Its first Load reconfirms Tuesday September 3, 2002 morning (`3.9.02 MNH`),
Victor Moraes/Cruzeiro, Grêmio's BRL 5,000,000 offer for Abedi pending, 16th
national position, Goiás August 31 0:3, Coritiba/Santos/Colo Colo pending
on September 6/8/11. Luisao reconfirms birth February 13, 1981, age 21,
one cap, condition 89%, 26 (4) official appearances, one goal, two assists,
7.20 average, 8-7-9-8-6 form and Good morale. His full-time contract remains
BRL 8,750/week, N/D start, linked, December 24, 2004 expiry, rotation,
no bonuses or clauses. Original Save Game completes under the new name
`DateModule-03Sep2002`; size 34,464,680, SHA-256
`ce1b006a19508ff43c2507e7f7855a0dac9e9b8f1887c53ea8931a9a3138c446`.
Normal exit and process absence were verified before controlled reopen.
The exact controlled reopen and Original Load preserve date/phase, manager,
club, fixtures/results, and all listed player/contract values.

The pending Grêmio offer was rejected through the normal interface, with no
additional reason. One normal Continue Game processed Uruguay U21 1:7 Brazil U21
and stopped naturally at Wednesday September 4, 2002 night (`4.9.02 NTE`).
Cruzeiro fixtures/results and 16th national position remain unchanged. Luisao
now has 85% condition, 7-9-8-6-8 form, Very Good morale and two noncompetitive
appearances with 7.50 average; official statistics and contract remain as above.
Original Save Game completes as `DateModule-04Sep2002`, size 34,557,282,
SHA-256 `0d0ca840ef270751a2c8537191aacff301c52fb653f34c9bb5171d172fdcf86a`.
Normal Original exit and process absence were verified before exact reopen.
Original Load then reconfirms the full observed state, including the natural
player changes and unchanged contract. Both persistence cycles pass. Private
structured observations are retained in `live-cycle-{01,02}-before.json`.

## Build identities and preservation checkpoint

The six preinstall bundle inventories were rechecked after the real UI
validation; every file's content and metadata agrees with
`app-final-identities-before-install-01.json`. The independent current receipt
is `app-identities-current-02.json`; `installation-final-01.json` adds the final
installed inventory. All listed executables are ARM64. Signing changes executable identity,
so the signed candidates and the unsigned production source are listed separately.

| Bundle role | Complete file inventory | Executable SHA-256 |
| --- | ---: | --- |
| Production source | 2 | `a35a303f441d352d61e0ab0a8e154e06f14f747461310c9e6ac28ce36abaf1e5` |
| Installed before this delivery | 6 | `7284838b3e6d74785a6969d64bf2d19619ad2cdc3e5e7c0601e6e551a97db063` |
| Installed after this delivery | 6 | `a35a303f441d352d61e0ab0a8e154e06f14f747461310c9e6ac28ce36abaf1e5` |
| New private installed reference | 6 | `7284838b3e6d74785a6969d64bf2d19619ad2cdc3e5e7c0601e6e551a97db063` |
| Signed persistence candidate | 7 | `6bfc62d0f5df558571d84ff8eaa79941102ba3d35e4b1613cd1b31e91f6646d8` |
| Signed rare-caller candidate | 7 | `8291dd073b73c8480a8986b00abdf6d2b01a390242465baae37b1771858bc372` |
| Signed original-only UI candidate | 7 | `c92215ae9fe46d17933acb677c29374d8562a8b0019536762ca90ba382b577a2` |

The source bundle contains the executable and plist. The installer adds the
four private original-program files; signed validation bundles additionally
contain their signature resource. File counts describe these observed inventories.
They do not replace complete content, resource-fork, FinderInfo, xattr, mode,
mtime and flag checks.

The post-UI preservation receipt verifies all 809 Game / 5 Runtime / 43 Runs
personal files against the initial complete inventory, all 858 protected
identities and the new reference's full inventory. The original LFS save and
seven handoff prompts remain unchanged. Fresh preinstall and post-install
inventories also pass for all personal contents and metadata, all 858 protected
identities and the full new reference inventory. `tools/install_native.py` ran
once. `installation-final-01.json` verifies ARM64, source/installed executable
identity, common bundle content and metadata, identical production plist without
diagnostic properties, and all four bundled original-program inputs. No installed
personal career was launched during installation verification.

## Reproduction commands and sanitizer boundary

These commands require the user's private Mac inputs and immutable fixtures.
Use fresh output directories; do not overwrite earlier captures. Public clones
do not contain the original exports, snapshots, text assets or private saves.
The candidate executable was frozen before the integrated campaigns and UI work.
The commands below describe reproduction, not additional runs after acceptance.

```sh
python3 tools/compile_recomp.py .local/recomp-input .local/recompiled --root 0x10773710 --all-analyzed
cmake -S native -B .local/aot-build -DCMAKE_BUILD_TYPE=Release \
  -DCM_GENERATED_DIR="$PWD/.local/recompiled" \
  -DCM_ORACLE_DIR="$PWD/.local/recomp-input" \
  -DCM_DATE_SNAPSHOT="$PWD/.local/performance/season-day-01/SeasonFixture/initial.snapshot"
cmake --build .local/aot-build --clean-first -j 4
ctest --test-dir .local/aot-build --output-on-failure
python3 -m unittest discover -s tools -p 'test_*.py'
python3 tools/test_control_cursor_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_match_presentation_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_unsigned_division_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_local_temporaries.py .local/recomp-input/instructions.sqlite
python3 tools/test_generated_temporaries.py .local/recomp-input/instructions.sqlite --sanitize
```

The three new CTest groups invoke these production-destination harnesses:

```sh
.local/aot-build/cm-date-construction-checks .local/recomp-input .local/aot-build/date-construction-private.bin --composed
.local/aot-build/cm-date-presentation-checks .local/performance/season-day-01/SeasonFixture/initial.snapshot .local/aot-build/date-presentation-private.bin --composed
.local/aot-build/cm-date-subsystem_audit-checks .local/performance/season-day-01/SeasonFixture/initial.snapshot .local/aot-build/date-audit-private
```

Integrated commands use one frozen CLI executable and produce all five streams.
Use new output destinations to reproduce the cleaned captures and run them
sequentially when storage is limited. The weekly command below describes the
complete campaign; this delivery stopped it after the two semantic replays:

```sh
python3 tools/check_calendar_season.py .local/performance/season-day-01/SeasonFixture --date-subsystem \
  --executable .local/date-subsystem-reconstruction/campaign-bin-01/cm-season-reference \
  --edges-directory .local/date-subsystem-reconstruction/day-edges-reproduce-01 \
  --historical-edges-directory .local/date-weekday-reconstruction/day-edges-01
python3 tools/check_calendar_season.py .local/performance/season-week-01/SeasonFixture --date-subsystem --archive-date-oracle --date-oracle-timeout 21600 \
  --executable .local/date-subsystem-reconstruction/campaign-bin-01/cm-season-reference \
  --edges-directory .local/date-subsystem-reconstruction/week-edges-reproduce-01 \
  --historical-edges-directory .local/date-weekday-reconstruction/week-edges-01
python3 tools/check_calendar_season.py .local/performance/date-ordering-capture-02/SeasonFixture --date-subsystem \
  --executable .local/date-subsystem-reconstruction/campaign-bin-01/cm-season-reference \
  --edges-directory .local/date-subsystem-reconstruction/ordering-edges-reproduce-01 \
  --historical-edges-directory .local/date-weekday-reconstruction/ordering-edges-01
```

The focused UBSAN recipe compiles the changed runtime, calendar adapter, both
Date units, callbacks, all ten harnesses and six pertinent generated pages with
`-std=c++23 -arch arm64 -O1 -g -ffp-contract=off -fsanitize=undefined
-fno-sanitize-recover=all -Inative`. The generated pages are `page_10331`,
`page_10332`, `page_10715`, `page_10716`, `page_10717` and `page_10718`.
The harness sources are `date_construction_checks.cpp`,
`date_presentation_checks.cpp`, `date_subsystem_audit_checks.cpp`,
`date_weekday_checks.cpp`, `date_day_of_month_checks.cpp`,
`date_month_checks.cpp`, `date_ordering_checks.cpp` (twice, the before build
with `-DCM_CHECK_BEFORE=1`), `date_difference_checks.cpp` and
`calendar_checks.cpp`. Compile each unit to a fresh private object, then link
each harness with those common instrumented objects before `libcm_aot.a`,
`libcm_ui.a`, `libcm_audio.a`, `-lz` and the AppKit, Carbon, AVFAudio,
Foundation, CoreFoundation, CoreServices and ApplicationServices frameworks.
Use the three new harness arguments above; the seven delivered-entry harnesses
each receive `.local/recomp-input`. The retained exact driver is private at
`.local/date-subsystem-reconstruction/ubsan/run.py`, with the successful
ten-run receipt at `ubsan/report.json`.

The remaining archive members and native OS bridges are not instrumented by
this focused recipe. It is not a whole-program UBSAN or ASan claim. Generated
temporary checks independently instrument and compare the generated operation
patterns. No sanitizer result replaces real application/persistence evidence.

The final suite rechecks the eight delivered contracts after composition:

| Delivered contract | New final regression evidence |
| --- | --- |
| add/subtract | 16,380 original/semantic shift cases, including guards, aliases and leap subcall effects |
| difference | 6,892 ABI/effect cases and 494 independent oracle cases; the deferred -365/+366 asymmetry remains |
| later / before | 5,646 direct cases per entry, 156 independent oracle cases and 196,608 broad cases per predicate |
| month | 1,684 direct cases, 239 independent oracle cases and 131,072 pure signed-day checks |
| day-of-month | 3,173 direct cases, 1,700 independent Gregorian cases and 131,072 pure signed-day checks |
| weekday | 4,240 direct cases, 559 chrono cases, 319,675 wide Gregorian cases and 786,432 pure signed-day checks |

Python discovery's four skipped optional groups were executed explicitly:
five control-cursor, six match-presentation, seven unsigned-division and five
local-temporary audit tests pass. This is separate from discovery's 39 tests
and the generated-temporary differential UBSAN executions.

<!-- BEGIN ACCEPTANCE TABLES -->
## Own-call coverage matrix

Day and ordering counts are per complete semantic candidate, reconciled against
all-original CMDAT001 boundaries. Both candidates agree in those completed
campaigns. Every positive original/baseline report in those two campaigns has
also been checked for its exact mode selection: zero semantic new
entries in all seven baselines, with the delivered eight added in their
historical order. The subsystem baseline has eight semantic and 26 original
entries. Normal candidates have zero original calls for all 26 new entries.

The two live columns are complete run exit counters, not fixture/negative
proofs. T76m9A is the September 4 persistence reload with normal rare panels;
l0f0wh is the isolated Arsenal tournament career. Zero explicitly means no
coverage in that column. Together the real fixture and UI flows exercise all
26 own entries. Direct ABI cases and two own negatives per entry remain
separate evidence.

| Entry | Day | Week | Ordering | UI T76m9A | UI l0f0wh |
| --- | ---: | ---: | ---: | ---: | ---: |
| 0x107156b0 | 26589 | not recorded | 12936 | 2664 | 435688 |
| 0x107156f0 | 8413 | not recorded | 5700 | 427 | 43464 |
| 0x10715880 | 322248 | not recorded | 533705 | 48734264 | 47905925 |
| 0x107158a0 | 4426 | not recorded | 4007 | 114 | 3086 |
| 0x10715a30 | 1212 | not recorded | 943 | 156 | 1514 |
| 0x10715bc0 | 0 | not recorded | 0 | 2 | 0 |
| 0x10715e00 | 62 | not recorded | 11 | 61 | 2 |
| 0x10716130 | 231 | not recorded | 106 | 1455 | 280 |
| 0x107164f0 | 0 | not recorded | 0 | 1 | 0 |
| 0x107168a0 | 0 | not recorded | 0 | 1 | 0 |
| 0x10716b00 | 138 | not recorded | 300 | 558 | 149 |
| 0x10716eb0 | 9 | not recorded | 15 | 2 | 6 |
| 0x10717270 | 2 | not recorded | 0 | 1 | 0 |
| 0x10717620 | 0 | not recorded | 0 | 0 | 1 |
| 0x10717a10 | 55 | not recorded | 57 | 16 | 14076 |
| 0x10717cb0 | 0 | not recorded | 7 | 14 | 11316 |
| 0x10717dd0 | 10301 | not recorded | 14996 | 110 | 32947 |
| 0x10718440 | 1209350 | not recorded | 225802 | 13762 | 9505792 |
| 0x107184d0 | 369 | not recorded | 406 | 1797 | 297 |
| 0x10718570 | 3 | not recorded | 3 | 1 | 3 |
| 0x10718650 | 0 | not recorded | 0 | 2 | 0 |
| 0x10718700 | 138 | not recorded | 300 | 559 | 149 |
| 0x107187d0 | 242 | not recorded | 121 | 1460 | 286 |
| 0x107188a0 | 138 | not recorded | 300 | 561 | 149 |
| 0x107189d0 | 11 | not recorded | 15 | 4 | 7 |
| 0x10718af0 | 0 | not recorded | 0 | 1 | 0 |

Each exercised entry in the completed day/ordering campaigns has an isolated first-own-result rejection.
Reports retain its original first boundary index and diagnostic field/offset.
Date records reject at result offset 8; scalar/formatter r3 controls reject
at offset 7. Pointer-return helpers reject at the same own result boundary
as `result/ABI/effects header/size offset=0`: toggling the returned pointer
also changes the diagnostic output extent. Child results remain untouched.
This shape diagnostic is distinguished from a byte-value diagnostic.

| Campaign | Complete checker runs | New own calls | Exercised new entries / own negatives |
| --- | ---: | ---: | ---: |
| day | 35 | 1583937 | 19 |
| week | not recorded | not recorded | not recorded |
| ordering | 37 | 799730 | 19 |

The delivered entries are also counted separately in both candidates:

| Delivered entry | Day | Week | Ordering |
| --- | ---: | ---: | ---: |
| add | 25475 | not recorded | 13640 |
| subtract | 322828 | not recorded | 51573 |
| difference | 62897 | not recorded | 25546 |
| ordering | 0 | not recorded | 38051 |
| before | 0 | not recorded | 12911 |
| month | 8807 | not recorded | 8433 |
| day-of-month | 8639 | not recorded | 8733 |
| weekday | 1690 | not recorded | 1419 |

## Retained stream identities

All three fixtures' four recaptured historical gzip streams match their protected weekday
campaign predecessors byte for byte. Hashes below identify complete files,
not a prefix. CMDAT001 is a separate stream; it never replaces the older
formats. Private originals, exports and stream payloads are excluded from
the repository.

| Fixture | Stream | SHA-256 |
| --- | --- | --- |
| day | calendar historical gzip | `bdce201d4fc83a5c7e8eb9144421726f8bbbe76a1e12f1beaf424f756f5fd083` |
| day | month historical gzip | `0161ab1c87aa34562f70e54d3049736b27aab35e188303623e5ae6efad463608` |
| day | day historical gzip | `09ac0ffd9445ddcfa2793766c6d7902decac235ac0180a02219a47817a44cf63` |
| day | weekday historical gzip | `1d183246c61844490835a76b6c6b35dcd11da933cefd7be7e33ad31f9b4239aa` |
| day | Date original gzip | `cc0fea23ff09659e686df844a24027f1ba0f177a802e5f1ae436011a42fc0ab3` |
| week | calendar historical gzip | `8ff9f77b64c342d5074136d350850cdf53b10eaf8b1987d9632a226e46974b12` |
| week | month historical gzip | `1cef11c1352e2f3a67186a6761212c03c08fda6182502dc6c2def6e0cd6f2e66` |
| week | day historical gzip | `e106535c0c739ead6bb714b468a1dc6d4b3f60358e1a0358fbbcf807556713ba` |
| week | weekday historical gzip | `97f7d2761f770f1db7ded0235d92199bdf70f432a4794e2884d18a12c8f2f6b8` |
| week | Date original gzip | not recorded outside unfinished checker; no final aggregate receipt |
| week | Date physical xz archive, recorded before cleanup | `c3b4a392544a5a258b7d91741fc2cce831fbc19467051f9415790765a5d4dfa1` |
| ordering | calendar historical gzip | `a7849b48626ea92da01ed062417522323477196cd3a3a78c857357076f7b8954` |
| ordering | month historical gzip | `adfd71c6417a9a69d00bc5a250aab1d45b33c70e23a37f7c8688d15cf930c6bf` |
| ordering | day historical gzip | `9157b1525021b92dc0e81b92cd297a9d2b47228e4ae505dc63fc4cffc89f5220` |
| ordering | weekday historical gzip | `2a43559651d29c84f1c32ac468310c45da6d166672b271adcddeedeece440a61` |
| ordering | Date original gzip | `d99e47fa752f1f10de16a95d656706ec06ce3501183625fabbaae4be484caaf8` |

Day and ordering raw Date gzip files were archived losslessly with decompressed
identity checks before their working gzip copies were released. Their hashes
above remain recorded in the complete campaign and archive receipts. The user
subsequently requested storage cleanup. Current-goal large Date captures,
incomplete archive attempts, compressed test binaries/raw oracles and temporary
sanitizer objects were removed after file identities were recorded. They must
be rebuilt or recaptured from the retained source and immutable fixtures for
another replay; the removed payloads are no longer claimed to be retained.
The older protected historical streams, original inputs, all saves, fixtures,
final source/installed app, signed candidates, new reference, logs and evidence
receipts remain intact. `cleanup-final-01.json` records removal of 368 generated
files totaling 18,887,556,557 bytes (about 18.9 GB), after recording each physical
file's SHA-256. Available storage rose from about 10.2 GB to 29.1 GB at that
checkpoint. Post-cleanup full personal metadata, 858 protected identities and
new-reference inventory checks also pass. No ignore or Git-LFS policy was changed.

<!-- END ACCEPTANCE TABLES -->
