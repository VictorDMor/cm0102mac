# Tasks

The live work list. Claude keeps it current during a run; read this instead of
the scrollback. Remove ticked items once their commit is pushed.

## Blocked on Victor

- [x] **Uninitialised stack decides results** (found 2026-10-23). Decided by Victor 2026-10-23: option A, keep exact results; every rewrite reproduces the stack image the original leaves, checked byte for byte. Architectural changes that give up stack fidelity wait until the recompilation is complete.

- [x] In-game check of the file fixes: new save, overwrite and reload work (confirmed by Victor, 2026-10-21).

## Now

- [x] **CASH.CPP complete C++23 reconstruction (2026-10-29):** all four completion dimensions are closed: 12/12 audited entries, complete typed C++23, normal production and verified installation/delivery. Private per-entry evidence: `.local/rewrites/cash/ledger.json`.
  - [x] Close 12 entries /2,847 indexed instructions, source/TOC/neighbors, 868 incoming calls, six owned calls, four switch banks, no callbacks/interior entries and all 21 skipped words. Nine structural tests and 33 retained legacy source/data audits pass.
  - [x] Reconstruct all 12 entries as typed C++23: conversions/editing/presets, 25 currency profiles, labels and full/compact formatting. All 12 exclusive `cash_adapter` owners replace the three legacy builder families, including direct callers; 89 module/caller pages regenerated and audited.
  - [x] Complete 9,354 returned comparisons (4,677 controlled +4,677 observed production), zero differences/partials/internal faults. Three result/amount/text corruptions are detected; all 12 no-owned-instruction-budget production controls pass while the originals fail their negative controls. Actual arithmetic/string/printf dependencies run; alert and transitive BlockMoveData boundaries are explicitly modeled.
  - [x] Close behavioral coverage: 197/208 body outcomes plus2/2 shared helper outcomes exercised; all 11 remaining outcomes have independently reviewed immutable-bank, decimal-loop, format-layout or enum reachability proofs. LLVM fork coverage uses a locked merge pool with checked explicit child flushes. Independent source/oracle/coverage and scoped diff reviews pass; code-review skill unavailable.
  - [x] Final normal ARM64 app/checker/replay build (CASH coverage OFF), strict syntax and all 4 selected CTests pass. Original/default-C++ game-save2 replay pair reaches the recorded state; combined checked replay compares three actual calls plus12 completed real-state synthetic calls, all without differences. Old game-load fails HGetVol r27 on both routes and is excluded. Save/reload not retested; CASH owns no serialization. Performance not measured.
  - [x] Existing tracker row 13 certified Concluído and read back with 12/12 C++23 and 100% normal production. Formulas/neighbors are preserved; summary verifies 22 completed modules /895 routines. Native Google Sheets visual inspection confirms the final note and summary fit.
  - [x] After Victor closed the game, ordinary installation and strict deep signature verification pass. Installed ARM64 UUID 878C5AF0-5031-33AF-9E15-1437E4E21A64 matches the validated build, contains all 12 CASH symbols and no LLVM coverage runtime. Existing game data were verified/preserved; the earlier running-game guard was respected. Unrelated quick-save/MATCH_PL/bridge work is preserved and excluded from this scoped delivery.

- [x] **FIFA_RANKINGS.CPP complete C++23 reconstruction (2026-10-29):** all 15 audited entries implemented, active in normal production and installed; all four completion dimensions are closed. Private per-entry evidence: `.local/rewrites/fifa_rankings/ledger.json`.
  - [x] Close inventory: 15 entries / 1,575 indexed instructions, 0x104866f0–0x10487ff0. Neighbor/source and numeric pools, all incoming/cross-entry flows, both comparator transition vectors, 77 instruction-byte pointer coincidences and 25 skipped padding words audited. Eight structural tests pass, including stale-digest/duplicate/disabled-owner controls.
  - [x] Reconstruct lifecycle/storage, match points, signed-period queries, expiry/annual shifting, six-year weighting, sorting and positions as typed handwritten C++23. All 15 default entry guards and direct owned calls are active, without owned builder/generated fallback or observer gate. Exactly seven module/caller pages regenerated.
  - [x] Complete 1,688 returned comparisons (844 controlled +844 observed normal production), zero differences/partials/internal faults. Output, returned-count and serialized-byte corruptions are detected. All 15 zero-instruction-budget production controls pass while original bodies fail their corresponding negative controls. External calls compare ordered arguments, CR and preserved FPRs; partial-item reads/writes preserve byte prefixes.
  - [x] Close behavioral coverage: 249/250 domain outcomes and all 501 source lines exercised; the remaining repeated world-cup equality is dominated by an earlier inequality across read-only queries. The compiler-only constant-loop 0/0 record is excluded explicitly. Independent source/oracle/final-diff review and strict syntax pass; code-review skill unavailable, explicit reviews performed.
  - [x] Original/C++ native-save replay pair and exhaustive combined module replay reach the recorded state; the real capture reaches one owned save call, compared without differences. Final coverage-OFF ARM64 build and 4/4 focused CTests pass (module audit/equivalence, generated direct-owner calls and block accounting); strengthened bytewise-pointer audit also passes after its final refinement.
  - [x] Save an isolated all-leagues career through the original File menu, quit normally, restart and load the new file through document-open. All 213 rows (8,520 nonpointer bytes), 12 header bytes, 107 populated histories /2,964 bytes and the date are preserved; Continue remains available. Original files and the user's running session were preserved.
  - [x] Guarded installation and strict deep signature verification pass; installed UUID DBE26C2B-42B0-3E60-B1E5-518C1E71883D matches the normal build, with no LLVM coverage runtime. Existing progress spreadsheet updated/read back and visually checked: 15/15, 100% production, Concluído; formulas/neighbor rows preserved, summary 21 completed modules /883 routines.
  - Limits: external fixture models establish caller boundaries, not dependency internals. Append retains the original unchecked allocation-null access; no returned recovery comparison is claimed for that fault. Performance not measured; Victor owns gameplay QA. Unrelated quick-save/MATCH_PL changes are preserved and excluded from the scoped delivery.


- [x] **TRANSFER_SCREENS.CPP complete C++23 reconstruction (2026-10-29):** all four completion dimensions closed: 75/75 audited entries, 25,003 indexed instructions, complete handwritten C++23, normal production routing and verified delivery. Private per-entry evidence: `.local/rewrites/transfer_screens/ledger.json`.
  - [x] Close boundaries with source/TOC/neighbors, direct calls, 51 callback vectors, alignment and reference digests. Ten structural audits and the full production-owner gate pass; zero DRAFT entries or owned generated fallbacks remain.
  - [x] Reconstruct offers, rejection/completion, status/listing, comparison/shortlist, transfer history, release/trial and loan screens/commands in reviewed functional blocks. The final six entries add 5,861 instructions; browser helper names now reflect the actual Transfers screen. Guest data, ABI, arithmetic, RNG order and external call ordering are preserved.
  - [x] All 33,406 final returned comparisons agree: 16,703 controlled and 16,703 observed production, across all 75 entries, with zero partials/internal faults. All 75 ownership controls and deliberate corruption controls pass. Valid final controlled coverage closes every reachable outcome; the final six bodies exercise 543/544 outcomes and all 1,183 lines, with one independently proved dominated outcome. Earlier block exclusions retain their specific nonreturning/unreachable proofs.
  - [x] Independent source/fixture/scoped-diff review, strict syntax and 4/4 normal focused CTests pass. Original/C++ swap-offer replays reach recorded states; combined checking compares 9,465 calls across four captured entries without differences. Other entries are fixture-verified; external models and noncanonical mutations are explicitly classified. The code-review skill was unavailable; explicit independent reviews were performed.
  - [x] Guarded normal ARM64 installation passes strict deep signature verification, matches build UUID 74337415-C43D-30BE-A120-7D62C1A14B4B, contains all 75 symbols and no LLVM coverage runtime. Existing progress spreadsheet updated and summary totals verified. Save/reload not retested; no serialization implementation changed. Performance not measured. Unrelated quicksave/MATCH_PL changes preserved and excluded from commits.

- [x] **DISPUTE.CPP complete C++23 reconstruction (2026-10-29):** all 13 audited entries implemented, active in normal production and installed; all four completion dimensions are closed. Private per-entry ledger `.local/rewrites/dispute/ledger.json`.
  - [x] Close inventory: 13 entries / 2,063 indexed instructions, 0x1070bd00–0x1070dd80; source/numeric pools and neighbors, all incoming/cross-entry calls, traceback false pointer, five unindexed words and original digests audited. Ten structural tests pass, including negative controls.
  - [x] Reconstruct storage/lifecycle, dispute queries/daily expiration, RNG/attribute/peer updates and news construction/rendering as handwritten C++23; 13/13 normal entry guards and direct owned calls, no owned builder/generated fallback. Initial generation changes 19 module/caller pages; the final watched-boundary refresh changes only page_10738.
  - [x] Complete 1,132 returned comparisons (566 controlled +566 observed normal-production) across all 13 entries, zero differences/partials/internal faults. Explicit result, flag, actual-sample cursor and reseeded-bias corruptions are detected. Two expected fatal dependency-boundary prefixes match separately; two matching null-list faults are diagnostic only, never counted as equivalence. Partial IO mutates only its transferred prefix; list models preserve actual linked-node layout and live count/cursor effects.
  - [x] Close behavioral coverage: 305/310 module-body/shared-helper outcomes exercised; five have specific dominated-read, null-address or nonzero-divisor proofs. Wide sampling unroll/recursion is excluded by exact original arithmetic. Independent source/oracle/coverage review and explicit owned-diff review pass; code-review skill unavailable.
  - [x] Final coverage-OFF ARM64 app/checker build, strict syntax and 4/4 selected CTests pass (module audit/equivalence, generated direct-owner calls and block accounting). Original/C++ native-save replay pair and combined checked replay reach recorded states; combined replay checks one reached save entry without differences.
  - [x] Native original save, normal quit/restart and default C++ reload of an isolated all-leagues career preserve all 101,016 flag bytes, empty record list and date exactly. Populated streams, legacy versions, allocation/IO errors and rare branches have controlled original/C++ evidence.
  - [x] Existing progress spreadsheet updated/read back and visually verified: 13/13 C++ routines, 100% production, status Concluído. Formulas and neighboring rows preserved; completed-module summary is 17 modules / 767 routines. Only the affected note row was enlarged to prevent clipping.
  - [x] Guarded installation completed after Victor closed the game. Installed ARM64 app passes strict deep signature verification; UUID 5B42C78D-C833-310A-B2F9-794C591544C5 matches the validated build, DISPUTE production adapter is present and LLVM coverage runtime is absent. Game data preserved. The earlier running-app guard was respected.
  - Limits: external boundary fixtures prove caller contracts, not service internals. Wide-bias and mapped malformed-index/payload cases are explicitly noncanonical characterization. Performance not measured. Gameplay QA belongs to Victor, outside this delivery's gates.

- [ ] **MATCH_PL.CPP complete C++23 reconstruction (2026-10-28):** private per-entry evidence and pending contracts in `.local/rewrites/match_pl/`; overall module remains incomplete.
  - [x] Close inventory: 95 entries / 105,650 indexed instructions, 0x102238a0–0x1028ad60. Source/TOC neighbors, incoming/cross-entry flows, two TVs, 30 switch tables / 1,166 words, false pointers, padding and every original digest audited. All 14 structural tests pass; the older inferred inventory omitted the default constructor.
  - [x] Reconstruct and promote 81 entries (40,584 original indexed instructions): player state, spatial/role/strategy queries, geometry, RNG, transient attributes, condition/recovery, coordinates, heading, facing, marking, readiness, incidents, action bias, morale, contact failure, response acceptance, decision sampling, peer contacts, incident responses, event filtering, action dispatch, all three action selectors, pass completion/destination selection, direct contests and the common ball-action core. Calls between promoted entries use typed C++ directly; pending owned dependencies remain inventoried. Forty-nine old builder hooks across 16 families are superseded; direct legacy original/profile modes retain their original execution scope.
  - [x] All 81 bodies return 68,264/68,264 controlled and 68,264/68,264 normal-production comparisons, with zero differences/partials/internal faults. Wrong-result, wrong-memory, wrong-RNG and matching-nonreturn controls are detected. All 4,831 reachable instrumented body branch outcomes are exercised; 35 additional outcomes and diagnostic tails have independently reviewed reachability proofs. Fixtures include signed/FP extremes, exact shifts, RNG wraps, callback-mutated live state and coefficients, duplicate contacts, roster aliases, movement and recursion. Sixteen cases execute the actual candidate-population query and verify complete list cleanup. Eight expected fatal-boundary prefixes also match ABI/CR/FPSCR/memory at exact PC/SP/LR; these are separate from normal returns.
  - [x] Positive observer/budget controls prove 81 normal C++ routes; 49 direct legacy-original controls prove generated execution and global decline-scope restoration. Five additional original-budget exhaustion controls distinguish the action/pass/core bodies and their primary caller from generated execution. Independent source/oracle and scoped diff review, strict syntax, 14 structural tests and all 12 selected CTests pass. The pass-selection allowance includes its actual external abs/column queries and still rejects its original body. The normal ARM64 app is built with coverage OFF. The code-review skill is unavailable; explicit review was used.
  - [x] Close the coherent action/pass block: three complete bodies at 0x10245220, 0x10245870 and 0x10274170 (5,268 original instructions), sharing frame-free pass sampling. All 2,917 dedicated block comparisons return identically; 705 branch outcomes are exercised and five are proved unreachable. Parent comparisons execute the actual pass/core and receiver bodies; external services remain modeled at their real boundaries.
  - [x] The 81-entry build passes 600 actual-match advances on both original/rewrite routes, preserving memory, ABI, RNG and clock consumption. The earlier 76-entry checkpoint also passed five alternating timing pairs; no new timing or whole-game performance claim. Gameplay and full-season QA belong to Victor.
  - [x] Installed the validated 81-entry ARM64 build, UUID B6B00B55-9723-33AA-B999-F1E6C84B257A, with strict deep signature verification and preserved game data. Per Victor's 2026-10-29 guidance, develop larger coherent blocks and batch integrated verification and installation at block closure.
  - [ ] Reconstruct/integrate the remaining 14 entries: four have builder owners and ten use generated bodies. Next is the keeper-response helper at 0x10251270. Preserve the host-created interior phases of the large shot/event routine at 0x102576e0 through typed resumable phases, retaining the full live activation until liveness is proven. The progress spreadsheet is not marked complete.
  - [x] Characterize and integrate the full shared action core (0x10247030, 10,370 instructions), including four jump tables, actual disciplinary-list allocation/payloads, signed RNG paths, trajectory publication and terminal callbacks. All 13,778 dedicated core comparisons return identically; 1,174 body branch outcomes are exercised and eight have independently reviewed reachability proofs. Six expected fatal-boundary prefixes match separately; typed callers, RNG/result/memory corruption controls and normal production routing are verified.

- [x] **Retire the Inter-American Cup (2026-10-28, user-authorized gameplay change):** disable post-1998 editions in new and existing careers while retaining historical data and save compatibility; leave other competitions unchanged.
  - [x] Audit creation/load, daily/annual updates and FIX_MAN ownership. Astra resolved the persistence ambiguity: cup+8 owns statistics, while pending fixtures live in the separate year-based FIX_MAN; preserve the former and filter only validated pending rows in the latter.
  - [x] Implement the new-career scope and daily/annual guards separately from all six faithful C++23 entries. Native tests pass through generated new/load factories, preserve definition/registration/history and daily RNG, and compare retained FIX_MAN rows with the original reader. Real writer/reader checks preserve played results, another competition and 1998, filter pending 2001/2002 rows, and retain byte-identical repeated saves; lazy loading passes.
  - [x] Suppress independent future host announcements without dropping other competitions from mixed lists. The original real-news publication is a positive control; retired-only publication is absent, and the mixed list still publishes the other competition. Schedule history and RNG seed/pool/cursor remain intact.
  - [x] Independent source/diff review passes; code-review skill unavailable, explicit review used. Strict syntax, 12 original module audits, 5 retirement audits and all 6 focused CTests pass, including actual generated new-career/annual gates and native constructor services. Seven deliberate source fixups leave the six faithful C++23 entries intact. Normal ARM64 app built, installed and strictly signature-verified. Tracker M192 updated/read back/visually checked; formulas, adjacent rows and totals 15 modules/723 routines preserved. Scoped commit excludes concurrent EURO_CHAMP_QUAL, world-replay and pre-existing task changes.
  - Limits: calendar file persistence and native news publication were tested directly; full-career UI save/reload not retested, performance not measured. New careers intentionally change the RNG trajectory by omitting the retired competition. Gameplay QA belongs to Victor.

- [x] **EURO_CHAMP_QUAL.CPP complete C++23 reconstruction (2026-10-28):** all 31 audited entries implemented and active in normal production; private per-entry evidence in `.local/rewrites/euro_champ_qual/`.
  - [x] Close inventory: 31 entries / 8,590 instructions, 0x104f9fa0–0x105026c0; source/TOC neighbors, incoming/direct flows, 20 TVs, false data pointers, alignment and volatile dependencies audited. Per-entry contracts/path maps remain private.
  - [x] Implement and promote all 31 handwritten C++23 entries through normal production guards/direct calls; no owned builder/generated fallback. Named domain state separates ABI preservation from control flow. Eleven module-related generated pages changed.
  - [x] Complete 798 returned comparisons across all 31 entries with strict F1, zero differences/partials/internal faults; four wrong-result/RNG-cursor/RNG-seed/serialized-byte controls detected. Coverage closes 582/600 exercised domain outcomes plus 18 individually proved unreachable outcomes and 2/2 shared call boundaries.
  - [x] Original-native save capture accepts both original and C++ end states; combined all 31 configuration checks the reached save routine (one complete comparison, zero differences). Native original save, quit/restart and default C++ reload preserve module scalars/date and empty history. Populated/legacy streams and IO errors are covered by returned controlled cases.
  - [x] Independent source/oracle reviews and explicit owned-diff review pass; code-review skill unavailable. Final strict syntax passes. No bounded ambiguity required Astra. The old season-day capture fails even with this module forced original at PBWriteForkSync r29; it is retained privately and not counted as equivalence. The fresh original-native save capture supplies representative integration evidence.
  - [x] Final normal coverage-OFF build and 4/4 selected CTests pass (module equivalence/audit, generated direct-owner calls and block accounting). ARM64 app installed; strict deep signature passes and LLVM coverage runtime is absent.
  - [x] Existing spreadsheet updated to 31/31 complete and 100% production; formulas and adjacent rows preserved. Summary recalculates to 16 complete modules / 754 routines; exact readback and native Google Sheets visual checks pass. Scoped delivery is recorded in Git history.
  - Limits: boundary fixtures establish caller contracts, not external service internals. Native persistence checks an empty custom history; populated/legacy and short/error IO paths have controlled original/C++ evidence. Performance not measured. Gameplay QA belongs to Victor, outside delivery gates. Concurrent retirement/debugging changes and pre-existing TASKS content are preserved.

- [x] **INTER_AMER_CUP.CPP complete C++23 reconstruction (2026-10-28):** all six audited entries implemented and active in normal production; private per-entry ledger `.local/rewrites/inter_amer_cup/ledger.json`.
  - [x] Close inventory: six entries / 794 instructions, 0x106ebdd0–0x106eca50; source pools/neighbors, full incoming and cross-entry flows, four TVs/vtable, one skipped NOP and alignment audited. Twelve structural tests pass, including stale/duplicate/disabled/missing-owner controls.
  - [x] Implement lifecycle, one-round calendar and continental entrant selection as handwritten C++23; all 6/6 default entry guards and three direct-call hooks verified, no owned generated/builder fallback or observer gate. Exactly three generated pages changed.
  - [x] Complete 417 returned controlled comparisons across all six entries and five returned real-career-state comparisons (constructor/destructor/calendar/selection/configuration), zero differences or internal faults in the final cases. Wrong-result, RNG cursor/seed and entrant-pointer controls detected. All 98/98 instrumented domain branch outcomes and 2/2 shared call-boundary outcomes exercised.
  - [x] Independent source/oracle review and explicit final diff review pass; code-review skill unavailable. Preserve the original first-continent fallback writing slot two and the callers continuing on selector result zero. No correctness ambiguity or repeated divergence required Astra escalation.
  - [x] Original/C++ day replay pair and combined four-call real-state replay accept their recorded end states. The ordinary day reaches zero owned calls; per-entry coverage comes from the complete focused cases. The interrupted constructor experiment is superseded by a complete comparison using real NativePlatform services without a day transcript.
  - [x] Native constructor closure: original and default C++ hook return successfully with the captured 2001 definition, two entrants, one round and initialized fixture/calendar storage. All 22 actual imports across nine services match, including two HGetVol calls; observable memory/ABI, allocator, RNG and file effects match. The shared assertion flag starts with a fresh canary, and FSRefs are unified only after real native identity validation. Exact corrupted-r3 control is detected after complete returns; empty disk-baseline guard prevents cross-run contamination. Three focused CTests pass, including the registered native-constructor regression. Independent review passes. Tracker now records 422 complete comparisons (417 controlled + five real-state), with formulas, adjacent rows and totals 15/723 preserved and native visual readback checked.
  - [x] Final normal coverage-OFF build and 4/4 selected tests pass (module equivalence/audit and generated direct-call/block-accounting checks); strict syntax passes. ARM64 app installed and strict deep signature verified, without LLVM coverage runtime symbols. Existing spreadsheet reads 6/6 complete and 100% production, summary 15 modules/723 routines; formulas and neighboring rows preserved, native visual check passed.
  - Limits: boundary fixtures establish external call contracts, not service internals. No owned serialization changed; native save/reload not retested, performance not measured. Gameplay QA belongs to Victor and is outside delivery gates. Concurrent Best XI changes and pre-existing TASKS content are preserved and excluded from this module's scoped commit.

- [x] **Best XI v2 (2026-10-28):** compare available players' role-specific attributes, positional familiarity and condition against current tactical instructions; optimize the entire starting XI without duplicate players, then choose the fixture's bench from unused players.
  - [x] Original Mac profile getter decodes technical attributes and goalkeeper exceptions. Explicit role/instruction weights and deterministic global assignment replace the original positional selector; availability and squad publication still use original services.
  - [x] Focused `debugging_best_xi` passes: 960 enumerated alternatives, attribute/tactic-driven fullback changes in an original database world, versatile-player allocation, identity ties, condition, depleted/empty squads, bench limits, caller/formation/player preservation and unchanged RNG pool/seed/cursor; existing recovery and localization checks also pass. Scoped diff reviewed; code-review skill unavailable, explicit review used. ARM64 app built and installed with signature verification. Save/reload and performance not retested; weights are a selection heuristic, not measured match-outcome superiority.

- [x] **Debugging contract extension:** add the localized Action-menu option for an existing player contract. Extend the current expiry by one calendar year and cancel linked pending/deferred direct offers due after expiry; preserve purchases, loans, immediate hires, internal renewals and chosen agreements.
  - Original-service fixture passes with actual offer constructors and registration, concurrent direct/purchase/loan/immediate/chosen offers, selective deferred cleanup, internal renewal, retained contract bytes, repeated use and Debugging gating. Calendar anniversaries match `std::chrono` for 1601–2399; generated block accounting and existing Best XI/localization checks pass.
  - A copied career's André contract advanced from 28 December 2004 to 28 December 2005 with the same displayed terms; ordinary save, quit and reload retained the change. Debugging off removes the option. Pending-offer cancellation was tested with original services, not through save/reload of a career containing actual pending pre-contracts.
  - The signed installed candidate isolates concurrent unfinished CLUB_RECORDS work by declining that family's hooks and using its original routines. The original fallback and the contract fixture passed on this candidate; its player menu separately advanced André's copied contract from 2 December 2007 to 2 December 2008.

- [x] **CLUB_RECORDS.CPP complete C++23 reconstruction (2026-10-28):** all 92 audited entries implemented and active in normal production; private per-entry ledger `.local/rewrites/club_records/ledger.json`. Unrelated debugging changes preserved.
  - [x] Close inventory: 15,473 indexed instructions, 0x1000e6b0–0x1001dae0; source pools/neighbors, direct/indirect entries, CFG closure, two transition vectors, six switch tables and digests audited. Ten structural tests pass, including stale/duplicate/disabled/missing-owner controls. The apparent extra code pointer is RNG sample data.
  - [x] Implement record layouts/date initialization, storage/lifecycle, transfers/totals, league/archive/player selection, notices/presentation and post-match records as handwritten C++23; 92/92 generated default hooks and direct-call ownership, no owned generated/builder fallback.
  - [x] Complete 6,388 returned original/C++ comparisons over all 92 entries, zero partial/import-stopped/fault/different comparisons; wrong-result/RNG-cursor/RNG-seed/serialized-byte controls detected. Malformed index -2 is an explicit returned error case, distinct from -1 creation and ordinary career data.
  - [x] Close behavioral coverage: 1,644/1,668 instrumented domain branch outcomes exercised; 24 have specific gate, immutable-table, dominated-test or host-selector proofs. Source/oracle reviews fixed observable ABI/FP leftovers, including the reputation arguments seen by RNG reseeding. Code-review skill unavailable; explicit diff and independent source/coverage reviews used.
  - [x] Final original/C++ day pair reaches recorded end state; all-module combined replay checks all 20,072 reached calls over eight entries, including 12 match updates, without differences. Original/C++ save pair and combined serialization comparison also pass.
  - [x] Private career save, normal quit/restart and reload preserve 947,580 modern-record bytes, 338,640 season-record bytes, 282,744 index bytes, scalar headers and date exactly. Pending queue was empty; populated serialization and IO failures are covered by focused fixtures.
  - [x] Normal coverage-OFF ARM64 app build, installation and strict signature verification pass; installed executable has no coverage runtime dependency. Four selected tests pass (module equivalence/audit, generated direct calls and block accounting), plus all 6,388 comparisons in the normal build; strict syntax passes. Existing progress spreadsheet reads 92/92 complete and 100% production, summary 14 modules/717 routines; formulas and neighboring rows are unchanged. Scoped delivery is recorded in Git history. Gameplay QA belongs to Victor.
  - Limits: external service fixtures establish caller boundaries and modeled effects, not those services' internals. Invalid/malformed-memory characterization is distinguished from ordinary career states; matching faults are not equivalence. Performance not measured. Victor will verify the spreadsheet visually; agent readback verifies exact cells, preserved formulas and summary totals.

- [x] **OCEANIA_NATIONS.CPP complete C++23 reconstruction (2026-10-28):** 15/15 handwritten entries active in production; private per-entry ledger `.local/rewrites/oceania_nations/ledger.json`.
  - [x] Close inventory: 15 entries / 2,315 instructions, 0x106e9940–0x106ebdd0; source/TOC neighbors, flows, 11 TVs and two unreachable NOPs audited. Eleven structural tests pass; four generated pages changed and all 15 production guards verified.
  - [x] Implement lifecycle/calendar, national selection/RNG, stages, events and record updates; no owned generated/builder fallback. Independent source/oracle reviews found two ABI defects, fixed before final comparisons. Code-review skill unavailable; explicit final diff review used.
  - [x] Complete 1,038 returned comparisons across all 15 entries, zero partial/fault/different comparisons; four wrong-result/RNG-cursor/RNG-seed/group-payload controls detected. All 304/304 instrumented domain branch outcomes exercised. Original duplicate formatter branch is proved unreachable; coverage evidence matches final sources/configuration.
  - [x] Original/C++ normal day replay pair reaches the recorded state; separate combined checked day returns four real update comparisons, zero differences. Final selected CTest run passes 4/4 (module equivalence/audit and generator direct-call/block-accounting checks). Normal ARM64 app build, installer and strict signature verification pass.
  - [x] Existing progress spreadsheet reads 15/15 complete, 100% production; formulas preserved, neighboring rows unchanged, summary 13 modules/625 routines verified by readback and native visual inspection. Scoped delivery is recorded in Git history.
  - Limits: scripted external services prove caller boundaries, not those services' internals; injected allocation/virtual failures are distinguished from ordinary play. Underfilled national pools characterize the original's untouched spare stack slots, not ordinary-career reachability. No owned serialization changed; live career persistence not retested. Performance not measured. Victor owns gameplay QA, outside delivery gates.

- [x] **NEWS.CPP complete C++23 reconstruction (2026-10-28):** all 33 inventoried entries implemented and active in production; private per-entry ledger `.local/rewrites/news/ledger.json`. Verification follows Victor's reduced scope of 2026-10-28; gameplay QA belongs to him.
  - [x] Close original inventory: 33 entries, 16,819 indexed instructions; source pools, neighbors, direct/indirect targets and alignment audited. Ten structural tests pass, including exact production closure.
  - [x] Implement and route all 33/33 entries as handwritten C++23; 144 external message delegates and 27 local type heads, direct owned calls, no owned builder/generated fallback. All 33 entry guards and 5,273 direct-call hooks verified.
  - [x] Final focused checks: 42,903 returned comparisons, zero partial/fault/different comparisons; 6,387 item/priority, 68 storage, 787 sampling, 475 queue/action integration, 9,488 actions and 25,698 messages. Wrong result/RNG/cursor controls are detected. Storage includes 45 explicitly injected fatal-return boundaries; mapped/noncanonical memory cases are distinguished from ordinary career states.
  - [x] Close behavioral coverage: 1,833/1,860 domain branch outcomes exercised; remaining 27 have inspected gate, immutable-table or stable-read proofs. INJURY has 82 valid table records (0..81); code82 is separately documented adjacent-memory characterization. Independent reviews found and fixed three observable call/return-state defects; explicit final diff review passed (code-review skill unavailable).
  - [x] Original/C++ day pair reaches the recorded end state. Combined sampled replay: 12 reached entries, 131,413 calls / 1,264 checked, zero differences. Real text capture: 646,924 calls / 1,513 checked over seven entries; native save capture: 2,512 calls / 224 checked over seven entries, including save_queue; both reach their recorded states.
  - [x] Normal ARM64 build and signed installation pass. Isolated all-leagues career save, quit/restart and reload preserve all 22,800 news-record bytes, 192 inbox bytes, scalar headers and date exactly; original Continue returns. Localized type13 calendar text and managed links were visually checked.
  - [x] Required focused verification passes, including NEWS audit and equivalence in the final suite run. Before Victor reduced the scope, 219/221 available tests passed with no failures; the two unrelated `shortlist_manager_contract_offer` / `shortlist_manager_swap_offer` replays were stopped at his request and are not claimed as passes. No full-suite completion is claimed or required. Existing progress spreadsheet reads 33/33 and 100% production, with preserved formulas and unchanged neighboring rows; summary 12 completed modules / 610 routines, verified by readback and native visual inspection. Scoped delivery is recorded in Git history.
  - Limits: external service internals are outside NEWS ownership; boundary fixtures do not prove those services. The separately invoked 4011 missing-offer diagnostic faults at0x34 in both implementations and is never counted as equivalence. Astra did not prove its external lifetime invariant; the original unconditional access remains unchanged. Performance not measured. Victor owns full-season gameplay QA, outside this delivery's gates.

- [x] **POL_CUP.CPP complete C++23 reconstruction (2026-10-27):** private per-entry evidence in `.local/rewrites/pol_cup/ledger.json`; pre-existing code changes preserved.
  - [x] Close original inventory: six entries, 0x106cd810–0x106ce820, 1,018 indexed instructions; neighboring pools, every direct/TV target and absence of interior resumes audited. Eight structural tests pass, including negative controls.
  - [x] Implement calendar, entrant selection and lifecycle as ordinary C++23; all 6/6 entries hook production, no owned generated/builder fallback. Generation changes exactly three pages.
  - [x] Complete 263 returned controlled fixtures plus six returned calls over registered real-career competition state, zero partial/fault/different comparisons; corrupted output detected. Native imports and private files match, including 708 history bytes written by reset.
  - [x] Close behavioral coverage: 125/126 instrumented branch outcomes exercised; the one missing constructor selection==0 outcome is impossible because the owned selector returns only 1/-1. Preserve that original branch. Injected virtual failures remain distinguished from ordinary concrete-vtable behavior.
  - [x] Review source and host-input oracle. Bounded Astra diagnosis proves native FSRef bytes contain the issuer PID; a preparatory child supplies common natively-equivalent references after per-run validation. No comparison bytes ignored and no production/runtime behavior changed. The unavailable code-review skill was replaced by explicit diff reviews.
  - [x] Original/C++ day replay pair and combined checked replay reach the recorded final state. The day reaches zero owned entries; it is integration evidence, not per-entry coverage or a performance claim.
  - [x] Normal ARM64 build and strict signed installation; 218/218 available full-suite tests (2352.03s), then 3/3 focused tests after final identifier/coverage-output refinements. Progress spreadsheet reads 6/6 complete and summary 11 modules/577 routines; native visual verification passed. Scoped commit/push evidence is repository history.
  - [x] Save, quit/restart and reload an isolated all-leagues career through original menus/document-open. Date, cup year, all 52 entrant IDs/classes, seven round headers and budget retained exactly; normal career commands return.
  - Verification scope: all six owned entries and reachable behaviors are exercised or specifically proved unreachable. Reset returns with matching native IO over real career state. Full-season gameplay QA belongs to Victor and is outside module-delivery acceptance; no annual campaign is required. Performance not measured.
  - Baseline artifact incident: the pre-existing root `default.profraw` was overwritten by this run's implicit LLVM profile output. Exact previous bytes were not recovered (no local snapshots); other `.local/` shortlist profiles remain intact. Do not treat the root profile as prior-run evidence. Coverage now instruments the checker only, with a private default output path; the no-environment-variable regression preserves the root profile.

- [x] **CONTRACT_MANAGER.CPP complete C++23 rewrite (goal set 2026-10-26):** preserve guest memory/stack/register effects, simulation, RNG ordering and file formats. Private evidence and per-entry limits: `.local/contract-manager/progress.json`.
  - [x] Audit exact module bounds and census: 95 entries, 0x10077ee0–0x10093a50, 28,209 indexed instructions; comparators, initializer and unindexed interior branch accounted for.
  - [x] Implement and enable all 95 complete semantic C++23 bodies; direct internal calls, zero drafts and no original-body fallback. Source census/digest/ownership audit passes.
  - [x] Generate production95 (exactly two final pages changed), build the ARM64 app and all 79 existing test executables.
  - [x] Complete focused equivalence/edge checks and whole-goal source/proof review. Final 20-news fixture: 38,718 comparisons, 28,954 returns (54 explicitly injected-bound cases), 9,734 dependency interruptions, nine separate FP guards, 21 negative controls and 767 continuations (three injected-only). Last renderer: 4,563 normal returns, 64 continuations, 128 returned real-callee cases, zero partial/differences; ordinary capture reaches zero calls, not coverage. Other per-entry evidence retains its own limits.
  - [x] Prove pending news cleanup effects: eight completed original/rewrite pairs with final queue count/payload postconditions; wrong-count and unmapped-memory negative controls rejected with exact diagnostics. Discarded stale-binary runs are excluded.
  - [x] Final full available suite: 212/212 tests passed (676.86s). Ownership-aware bootstrap and 135 legacy nonlocal initializer comparisons pass; instruction accounting remains diagnostic.
  - [x] Save and reload a copy of a career through the real app: original File menu save, process restart and document-open reload; 11 January 2002 retained, normal career commands available. Protected reference and installed runtime preserved.
  - [x] Five alternating timing pairs, 30 runs accepting the recorded end state: reference median 21.4348s, C++23 median 21.2528s, median paired wall reduction 0.80%, faster in 5/5 pairs. Scope: one captured day on this machine.
  - [x] Final source/proof reviews and scoped delivery; ARM64 app installed and strict signature verification passed. Commit/push evidence is the repository history. Unrelated dirty changes and private game-derived evidence preserved.

- [x] **Pending prior-work spreadsheet audit (2026-10-26):** checked all 21 flagged modules and reproduced all 84 historical/production percentage cells without differences. CASH's candidate count tightens from 11–12 to 12/12 with audited neighbours. Strict new-style review confirms three DATE entries; five more DATE candidates and one possible LANGLIB boundary entry remain unresolved. CONTRACT_MANAGER's two full direct fast paths still use register-array algorithm state and do not qualify as modern. Private evidence: `.local/module-audit-20261026/audit-summary.json`. Spreadsheet unchanged; module denominator envelopes remain heuristic, no exact modern percentage established, runtime equivalence/save-load/performance not retested.

- [x] **CLUB_HISTORY.CPP plain C++ rewrite (2026-10-26):** all 11 routines (1,702 indexed instructions, 0x1000cbc0-0x1000e6b0) implemented and hooked in production; audit checks pass. Existing game-load/game-save2/ps-newgame captures compare without differences; a deliberately corrupted save result is detected. 62 basic synthetic cases pass (56 returned, six stopped at an uncaptured import); populated competition archival cases are compared separately. The exploratory archival set included nine absent competition pointers: invalid setup writes at 0x44, excluded from the valid set, not counted as passes. Live career save/reload and performance are not retested; Victor will play-test.
- [x] CLUB_HISTORY delivery: the 25 valid archival/destruction cases pass (four returned, 21 matched at uncaptured imports); both synthetic parent replays reach their recorded end states. Final ARM64 app built and installed, signature verified; both CLUB_HISTORY and SHORTLIST_MANAGER hooks are present. Five focused CTest entries registered; no general suite, live career or performance run.

- [x] Debugging Best XI: select available starters for the current formation and reserves within the competition's bench limit. Localize Best XI, Add Goal, repeat-goal and Debugging On/Off controls in the 11 original languages. Actual-database checks cover gating, availability, formation changes, bench limits and depleted/empty squads; isolated Reading UI verified with Debugging off and in Portuguese/English. Career save/reload and performance not retested.

- [x] **Fused multiply-add (decided 2026-10-20).** One rounding at all 1,049 sites, in generated code and in the 41+ builder call sites of rewritten routines. Two 600-advance matches: state differs transiently after 4 and 1 advances, final state identical.
- [x] Track 1, first measurements (2026-10-20, 600-advance pairs): big-endian accessors as one load/store + byte swap: **5.3% / 8.3% faster** (Arsenal / Cruzeiro), identical final state; kept. Partial-page bounds entries for globals: 1.6% / 0.3% *slower* in match play; reverted.
- [x] Plumbing (2026-10-21): scope chains per State (3.6%), inline PC-profile flag (1.5%). With byte-swap: **13.9% / 13.5% faster** match intervals than before today's speed work.
- [ ] Next plumbing: per-call diagnostics accessors (`spatial_role_diagnostics` ~2%), what dominates `spatial_role::invoke` (~12%), owner→generated calls re-entering `run_until` (track 2 removes these).
- [x] World replays work again (2026-10-21): `cm-world-replay` runs pure imports (pow, BlockMoveData, NewPtr…) live and replays OS imports from the transcript; the import check compares only registers the import reads or its caller keeps. Interest pass ×4, daily transition, attribute world, database and daily sort reach their recorded end states and are ctest checks (`world_replay_*`). `background-fixture` is invalid since single rounding: its match now calls an OS import the capture never recorded (match play is covered by the 677/2262 intervals).
- [x] Profiled world processing (guest profiler, 2026-10-21); see Profile notes. The time is spread thin and dominated by mechanism: per-instruction accounting, dispatcher round trips, builder methods, slow global bounds checks.
- [x] Add-link keeps its link indexes sorted by insertion instead of append + full heapsort, when that provably equals the sort (see commit).
- [x] **Block accounting** in generated pages, fused sort and lowered leaves: one step per straight-line run. Measured: attribute world 28.9% faster, interest passes 7.6–9.5%, daily transition 8.4%, match intervals 4.5%/2.9%.
- [ ] Builder-based owners still step every instruction (`SemanticGuestCall::step`); they dominate the interest pass, where removing all accounting was worth 16–27%. Cheaper builder steps, or real C++ rewrites of the hot owners.
- [x] ~~Bridge the loader padding in the fast bounds table~~ — rejected 2026-10-21. It does remove the slow path for the whole TOC page (interest pass 1: 3.65M slow checks → 5.5k), but paired timing showed no gain (world −2.2…+2.9%, match +1.2%/+0.3%, mixed pairs): a slow check costs about a nanosecond, and the host profile overstated it.
- [x] ~~Force-inline `Memory::write`/`check_scalar` in generated pages~~ — rejected 2026-10-21: binary +16%, world ±1% (attribute +3.4%), match 1.7%/2.4% slower.
- [x] Inline leaf routines at their call sites in generated code (197 leaves): attribute world 12.1% faster, match 2.2%/3.9%, recruitment flat.
- [x] Owner→leaf calls run directly in `run_until` (`run_leaf`): match 5.4%/5.1% faster, world neutral.
- [ ] Production/transfer checks of other owner families with leaf entries (marking_pressure, actor_order, contract_terms/review, stdlib_core, utility_object, actor_effects, …; not built here) expect their hook to run when `run_until` enters a leaf, like the spatial-role checks did; use `is_leaf_entry` as those were updated.
- [x] Production pages call C++ owners in place (693 owners, ~37,000 call sites): `bl X` runs the owner's hook and continues at the return address unless the innermost dispatcher stops there. Day replay 23.43 -> 22.81 s (-2.6%, 5/5 pairs). Owners whose hook counts before declining (string_core, file_path, ...) keep the dispatcher.
- [x] Builder routines count instructions in batches when nothing watches them (f493033): day replay -2.1%. Routines with a host-watched PC keep stepping.
- [x] Player contract lookups in plain C++ (438e283): 17.0M first-contract, 4.6M club and 1.3M second-contract lookups per game day; -0.3..-1.2%. The club lookup declines to the builder when it needs the club resolver (1.0M/day).
- [x] ~~Force-inline the builder's instruction methods~~ — rejected 2026-10-22: day replay 1.6% slower (5/5 pairs), binary +8 MB.
- [ ] FP status bookkeeping: 210M updates per game day in generated pages (~25 byte operations each). Dropping it is not an option: the flags reach setjmp's jmp_buf (the only `mffs`, 0x10773ecc) and from there game memory and FP registers, and the replay diverged. An exact cheaper helper might save ~2-3%.
- [x] Registration shortcut: inactive recruitment entries return at once, writing the original's stack image (2.8%); rejected shortlist rows skipped natively (0.6%). Club control `0x1014c2e0` in C++ (`native/club_control.cpp`), checked against the original for every club.
- [x] Globals and TOC on the fast bounds path: a block where a mapping starts part-way has a second entry (the data segment starts in the block where the code ends). Day replay -1.4% (6/6). The 2026-10-21 attempts (bridging the gap, a begin offset) had measured nothing.
- [x] Builder transcriptions are slower than the generated code they replace: with production hooks off for actor_attributes, contract_terms, shortlist_assessment, shortlist_policy and transfer_offer (`GENERATED_FASTER_FAMILIES`), day replay 21.56 -> 20.19 s (-6.4%, 6/6). Owners stay, with their checks, as specifications. Screening the other families in original mode on the day was confounded by machine drift; on the week, original mode took the actor families 255 -> 247 s and 15 other match families 255 -> 238 s; all 23 now run generated code too (day 20.34 -> 19.16 s). A word-level FPSCR summary helper measured 2% slower than the generated byte chain and was dropped. `player_decision` in original mode diverges at an import (r19), so its owner and original differ somewhere: investigate.
- [ ] **The original reads uninitialised stack** (136 sites/day, `cm-world-replay --stack-audit`; see docs/aot-port.md). Rewrites that skip calls must reproduce the stack image those calls leave, or prove nothing reads it. This bounds every deeper rewrite; a definedness tracker (which bytes of game memory come from leftovers) would let checks ignore pure padding instead.
- [ ] Recruitment is ~53% of the 26-country day (guest profile): daily interest registration `0x10079050` -> `0x10363ca0` -> `0x10363f60` 24%, candidate scans `0x1035e9f0` from shortlist links 17% and `0x10359020` 9%, plus 4% from `0x1011c330`. Registration runs per (distributed player x club in order): per day 261 players x ~6,700 clubs = 1.76M `register_club_interest` calls, 76% stop at the club's inactive recruitment entry, 16.9M shortlist rows scanned, 477K rows available, 326K evaluated (two assessments each), 255 registrations.
- [ ] Recruitment speed (after SHORTLIST_MANAGER.CPP in C++): the module's own time is small; its share of a day is in the routines it calls (contract and club lookups, generated callees).
- [ ] Direct cross-page calls between generated routines (skip two dispatcher round trips per call); page frames are 48–96 bytes, so nesting is cheap.
- [ ] FPSCR: the only reader is `setjmp` (0x10773ecc), which stores it into the jmp_buf; status bits are never used for a decision. Find a way to stop computing them that keeps jmp_buf memory identical.
- [x] **All-leagues benchmarks (2026-10-22).** Captures of the 26-country career in format 2: the 21->28 Dec week in `.local/performance/week-capture/SeasonFixture` (937 MB, ~300 s per replay) and 21->22 Dec in `.local/performance/day-capture/SeasonFixture` (91 MB, ~23 s). `cm-world-replay <capture> 1 --timing` replays to the recorded end state; use the day for quick pairs and the week for headline numbers. Real-game Continue times vary run to run (the game's results differ), so time with replays and find costs with `profile start/stop` through the driver.
- [x] Host overhead while the game works (732b5c0): cached snapshot env check (was 29% of samples), event polls (~20% -> 0.2%), import names as views.
- [x] List search `0x101b1620` in C++ (bc4abba): week replay -9.9% / -9.4%.
- [x] Fast paths ran only in benchmarks: the app's drawing observer made them step aside (b6c121e). Real-game week now ~290 s (two runs), against 350-372 s for the previous build's recent runs; not a paired measurement.
- [ ] Real-game profile now (current build, 2026-10-22): match simulation 32% inclusive; candidate scan `0x1035e9f0` 30% inclusive / 6.3% self; system calls 11%, of which file-system path work ~7.4% (`FSMakeFSSpec` 2.2%, `FSGetCatalogInfo` 1.4%, `FSOpenFork` 1.2%, `ResolveAliasFile` 1.1%); actor role wrapper `0x10289560` 4.5% self; list search 3.8% self (linear scans).
- [ ] File opens during processing: `0x10308350` -> `0x10748ce0` -> `0x10739de0` -> stdio open (`0x107768f0`/`0x10776940`/`0x10776da0`) resolves the same host path again on each open. Cache path resolution in the bridge (`allowed()` canonicalizes per component; `directory_id()` stats every known directory), invalidated on create/delete/rename/copy-on-write. Needs a save and reload on a career copy.
- [x] The dispatcher asks import_name only for PCs inside the import stubs (generated bounds): day replay -2.4% (5/5).
- [ ] Import dispatch: resolve each import stub to a handler once and switch on it (names are views now).
- [x] Capture format 2 (6458137): week 34 GB -> 937 MB, replay 563 s -> ~300 s.
- [x] Idle screens no longer spin a core (d4b29a1): start screen 99% -> ~5% CPU.
- [ ] Real-game runs differ (the game seeds randomness from the clock at startup). A fixed-clock validation mode would make real-game timing and save comparisons deterministic.

## Done: SHORTLIST_MANAGER.CPP in C++23 (goal set 2026-10-25)

Finish line: every routine of the original's `SHORTLIST_MANAGER.CPP`
(0x10356410-0x1036ff60, 57 routines, 26,197 instructions;
`tools/shortlist_manager_audit.py`) runs as hand-written C++ in production,
checked against the generated reference (synthetic calls where no capture
reaches), replays at their end states, suite passing, reviewed commit, one
save/reload on a career copy, paired timing on the 26-country day.

- [x] Bounds (pool slot -0x4dd8; SETUP.CPP before, SIMULATED_STATS.CPP from
      0x1036ff60), audit tool, adapter, generator hook on every entry, the
      shortlist builder families out of production.
- [x] 11 groups (compare, storage, links, policy, recruitment, assessment,
      interest, context, candidates, squads, daily), all 57 ROUTINE; calls
      inside the module run the callee's C++ in place.
- [x] Final checks, all families together: season-day, launch-restart,
      game-load, game-save, game-save2, launch-sav, daily-transition,
      tactics-continue, ps-newgame: 0 differ, end states reached; also with
      --compare-f1 (every routine leaves f1 as the original). Synthetic sets
      per group pass.
- [x] Review (high): legacy-save copy fixed (+0, not +2); divisions follow the
      reference (divide/modulo with the divw site); one random draw, shared
      record views and sign helpers.
- [x] A shortlist_links builder owner differed from the reference (heap byte);
      tactics-continue and ps-newgame expected states rewritten from the
      generated reference (old ones kept as expected.snapshot.builder-owners).
- [x] Real game: All-Leagues copy, Continue 21->24 Dec, saved over it, quit,
      relaunched, loaded (24 Dec). Save size matches the generated code's.
- [x] Timing, season day, 6 alternating pairs on the final build: generated
      19.52 s vs C++ 18.95 s mean (-2.9%, 6/6).
- [x] Captured calls for the screen paths (2026-10-26): ui/contract-offer (a
      new contract and transfer status for a Cruzeiro player:
      0x1036f670/0x1036f7e0/0x1036f920) and ui/swap-offer (a bid with a part
      exchange: 0x10364d00, 34 calls), every call checked, f1 too. A week of
      the 26-country career (its expected state rewritten from the reference
      as above): 645M calls, 69,939 checked, 0 differ.
- [x] 0x1036ec30 (a club's shortlist rebuilt for a human manager: the router
      0x102a54b0 when he accepts a job, 0x1019dd00 when a board appoints a
      human, i.e. one of the last 16 persons): ui/take-control (Options >
      add a manager mid-game, choose KAA Gent, take control), its one call
      checked, 0 differ; plus 639 synthetic calls.
- [x] Error and legacy paths: the eight builder families' fixture programs
      now also run the SHORTLIST_MANAGER C++ on every case (~32,000: legacy
      save versions 0x3e015/0x3e016, short and partial IO, allocation
      failures, fatal exits, assertion reports), 0 differ; they found 11
      bugs (nine wrong return addresses, the rand order at a table wrap in
      mark_primary_contract, f1 across a call in registration), fixed.
- [x] Builder families: kept as specifications (as the GENERATED_FASTER
      families); out of production, their fixtures now check the C++.
- [ ] Branch coverage (2026-10-26, in progress): clang coverage of the C++
      (CMake CM_SHORTLIST_COVERAGE) over every capture, fixture case and
      synthetic call; each region no check reaches gets a checked synthetic
      call (memory writes for the state it needs) or a proof from the listing
      that the original cannot reach it.
- [x] Resume after Claude's limit (2026-10-26): all 57 production routines
      were already converted at cc91da2. Recovered the unfinished groups'
      individual successful comparisons into `.local/shortlist/recovery/`
      with log provenance; these are historical results, not new test runs.
      There are 4,110 unique returned calls across seven capture-specific
      sets. Stopped calls and failures remain separate in `recovery.json`;
      the context probes deliberately stopped their parent replays early.
      No combined coverage percentage is claimed from these mixed profiles.
- [x] Review of the pending checker change: matching internal faults must
      fail the equivalence check instead of returning success without
      comparing memory or imports. Matching import stops still compare
      state. The interest probe's 0x4 vs 0x50 fault was caused by its own
      diagnostic dereference, absent from production; no gameplay change
      was justified by that probe. Corrected assessment proof addresses.
- [x] Lightweight verification of this resumption: four
      `test_shortlist_manager_audit.py` checks passed, changed C++ translation
      units passed Clang's syntax-only check, and all recovered call lines
      matched the copied source logs. The changed fault-handling behavior
      has not been executed in a rebuilt checker; no new runtime-equivalence
      claim is made.
- [ ] Remaining verification: complete branch coverage for links, daily,
      squads, candidates, interest and context is not established. Review
      the recovered cases before any future coverage merge; internal faults
      previously reported as `ok (both stopped)` are not equivalence passes.
      Per Victor's current scope, stay in SHORTLIST_MANAGER and leave game
      testing to him; no app build, installation, career replay, save/reload
      or performance run in the initial recovery (not retested).
- [x] Authorized build/install follow-up (2026-10-26): built the Release
      app with CM_SHORTLIST_COVERAGE=OFF and installed it through
      `tools/dev.sh install`. The first link exposed the pending weak LLVM
      profile symbol in normal builds; compile the child profile flush only
      when coverage is enabled. Normal build/link passed; coverage-enabled
      checker syntax passed. Installed ARM64 architecture, strict code
      signature and the modern shortlist adapter symbol verified. All 57
      C++ routines remain production hooks. Career save/reload, gameplay
      and performance not retested; Victor will test in the game.

## Done: PLAYER_STATS.CPP in C++23 (goal set 2026-10-25)

Finish line: every routine of the original's `PLAYER_STATS.CPP` (0x10301540-
0x10309ee0, 36 routines, 8,759 instructions; `tools/player_stats_audit.py`)
runs as hand-written C++ in production, each checked against the generated
reference on captured calls (synthetic calls where no capture reaches),
replays reaching their end states, suite passing, reviewed commit, and one
save/reload on a career copy.

- [x] All 36 hooked (PLAYER_STATS_ROUTINE), six groups (relations, queries,
      text, records, daily, files); cross-group calls are direct C++.
- [x] Checks, all 36 together on 12 captures (season-day, daily-transition,
      ps-profile, ps-profile2, ps-newgame, drag-pitch, tactics-views,
      tactics-continue, game-load, game-save2, launch-sav, launch-restart):
      0 differ; corrupted runs caught. ps-newgame: the constructor's whole
      new-game import matches.
- [x] Captures ps-profile, ps-profile2, ps-newgame (.local/benchmarks/ui);
      ctests player_stats_*, ui_replay_ps_*.
- [ ] Only synthetic calls reach 0x103058f0 and 0x10305ae0 (match-actor
      construction's "ordinary eligibility" branch: a real Cruzeiro match
      start ran 0x10223ac0 1,440 times, never that branch) and 0x10309cc0
      (transfer-rule predicates 0x106ff490/0x106ff4f0; neither a new game nor
      a week reaches them). Error paths (assertion alerts, allocation
      failures, old-save conversion in load_stats) are compared only up to
      the alert, or not at all.
- [x] Volatile audit (ctest player_stats_volatile_audit): no reads of r0,
      r11, r12, CTR, the carry, f0, f2-f13, or r3 after the NONE/F1
      routines, around the module or after the comparators' callbacks.
- [x] CR at calls: only the exception throw's context save reads it.
- [x] Timing, neutral (the module is <1% of a day): season day 5 pairs,
      original 18.998 s mean vs C++ 19.140 s (2/5 faster); new game 3 pairs,
      54.4 vs 53.5 s (mixed).
- [x] Save/reload: Reading career copy, Continue 3 days, save over it,
      quit, relaunch, load: 25 Aug 2001 intact.
- [ ] Speed lead: the comparator runs 26M times a day (906M in a new game)
      through bsearch/qsort's pointer glue and the dispatcher; a C++ bsearch
      calling it directly is the next step if the module ever shows in a
      profile. The adapter's per-call table lookup was not measured apart.
- [ ] Unconfirmed names (marked in the code): Person::joined, adaptability,
      CareerRow status/other_club, Match::rated, the history import flags.

## Done: MAIN.CPP in C++23 (goal set 2026-10-24, accepted by Victor 2026-10-25)

Finish line: every routine of the original's `MAIN.CPP` (0x1076f2b0-0x10771110,
44 routines, 1,869 instructions; `tools/main_program_audit.py`) runs as
hand-written C++ in production, each checked against the generated reference
on captured calls (fixtures where no capture can reach), replays reaching their
end states, suite passing, reviewed commit.

- [x] Module bounds: main() (called by the runtime start) to the runtime's byte
      copy; MAIN's TOC pool and globals; the MoreFiles helpers before main()
      are a separate unit.
- [x] Launch captures (`CM_AOT_LAUNCH_CAPTURE`, `game_driver.py launch
      --capture-launch`): from the first instruction to ExitToShell, so main()
      and run() are checked whole.
- [x] C++ for all 44 (`native/main_program_*.cpp`), hooked; native_service's
      0x10770a80 taken over. The quit runs fatal_exit -> longjmp into run(),
      which the C++ catches (traced once).
- [x] Checker: a call the session capture ends inside compares at the end of
      the transcript (both runs stop at the same import).
- [x] Per-routine checks (--only each, every call up to 2000 then 1 in 100)
      on 2 launch captures and the 13 UI sessions: 0 differ. Launch captures
      in .local/benchmarks/ui: launch-quit (start screen quit) and
      launch-menus (keys, command-S, wheel, Apple menu, File > Save dialog,
      AppKit quit).
- [x] Automation verbs `menu M I`, `wheel N`, `quit`.
- [x] Fixture checks (`cm-main-program-checks`, 608 cases + a spoiled one):
      every routine, the paths the port never reaches included (open
      documents, its callbacks, the script, speech, the Preferences dialog,
      the exit path); each Toolbox call's arguments and r13-r31 compared.
- [x] Static audit (`main_program_audit.py --volatiles`, ctest
      main_program_volatile_audit): no code around MAIN.CPP reads r0, r11,
      r12, CTR, the carry, or r3 after a routine that returns nothing,
      before writing it.
- [x] Plain C++ (2026-10-25): typed views (Field, EventRecord, FSSpec,
      Globals, Shared) and one Toolbox call per Carbon routine; jump tables as
      switches; registers mirrored only where the game or the replay contract
      observes them (r13-r31 at calls, results, r4-r10 at returns, cr0).
      Import contract: the arity of 26 more Toolbox calls. Fixtures: 608
      cases, digest of each Toolbox call's arguments and r13-r31; 23 planted
      faults all caught. Captures: 675 runs (each routine alone and all
      together on 15 captures), 0 differ.
- [x] DLOG 128 is the Preferences dialog (sound, speech), not an About box:
      routine renamed preferences_dialog; the original reads none of its
      controls.
- [x] Host Dialog Manager (2026-10-25): GetNewDialog/ModalDialog/DisposeDialog
      over DLOG/DITL as an AppKit panel; automation `dialog N`. Capture
      launch-dialog (Edit > Preferences twice, OK and Cancel).
- [x] Finder document opens (2026-10-25): one 'odoc' event with a real AE list
      of FSSpecs (AEGetParamDesc, AECountItems, AEGetNthDesc, AECoerceDesc,
      AEGetDescData), no 'oapp' after a launch with documents; EqualString
      bridged. `game_driver.py launch --open DOC`. Captures launch-sav (loads
      the save at start), launch-cfg (quick start, then exits), launch-rgn
      (setup script, then exits). Host checks in cm-platform-checks.
- [x] Calls into the rest of the program typed too (`Program`: the game, its
      settings, the C library); no raw guest calls left in the routines.
- [x] Automation drags arrive as clicks do (the window's Carbon click handler,
      then the event queue); capture launch-click reaches click_event.
- [x] Checks on the final code: 900 capture runs (each routine alone and all
      together on 20 captures), 0 differ, 40 of 44 routines reached;
      fixtures 609/609; suite: known match_debugger_actual_lineup only (114; fixed since).
- [x] Live tactics drag: skipped (Victor, 2026-10-25); drags take the real
      click path, and the start-screen drag is verified.
- [x] Readable register state (2026-10-25): `Kept<R>` locals, `keep()`,
      `enter()`, `leave()` name what the original keeps in r13-r31 and leaves
      in r3-r10; the raw writes left are the original's own loads and stores.
- [x] Automation `activate` and `print PATH` (as AppKit queues them); 'pdoc'
      delivered with its document list. Capture launch-events reaches
      activate_event and print_documents.
- [x] Speech, opt-in (CM_AOT_SPEECH=on|silent; off by default, so play is
      unchanged): the Speech Manager bridged; capture launch-speech speaks
      news headlines and disposes the channel at quit.
- [x] Speech stays off for players (Victor, 2026-10-25); CM_AOT_SPEECH=on
      or silent for those who want it.
- [x] Stack rule kept for MAIN.CPP (Victor left it to whichever is better,
      faster and more robust, 2026-10-25): its own code is ~0.3% of a
      season day (pump_events 0.2%, track_mouse 0.1% self), so giving up
      exact stack images would buy nothing measurable.
- [ ] Speed lead: 10.9% of a season day is under pump_events, almost all of
      it in the host's WaitNextEvent/GetMouse bridges during Continue
      (track_mouse alone 3.0% inclusive). Measure the host side first.
- [x] nothing() is the Restart hook (not error returns, as noted before):
      capture launch-restart (Game Options > Restart, Yes; the game goes on)
      checks it. Automation `popup N` chooses a closed pop-up's item.
- [x] debug_menu unreachable, now a ctest (main_program_volatile_audit): one
      GetNewMBar(1) and one SetMenuBar in the program, no menu added, MBAR 1
      = menus 1-3 in the installed original.
- [x] Fixture-only, accepted: fatal_exit's exit() for a fatal error before
      the game started (an empty eng.lng in a copy's overlay did not reach it;
      the language file is read after start).
- [x] Automation `control N` refuses an item of a closed pop-up (kind 0x10,
      no rectangle): activating one directly crashed the game once (null
      read at PC 0x100d4ea0); a user reaches those only through the open
      pop-up. Live: two such items refused; a tab, a list row and Continue
      still work.
- [x] Real game: load a copy of the reference career, Continue (a week),
      save over it, quit through the game, relaunch, load: day 17 of 2002.
- [x] Suite: known match_debugger_actual_lineup only (106 tests).
      main_program_detect_difference needed a checker fix (the spoil wrote a
      partly mapped page).
- [x] Review: native_service keeps 0x10770a80 in SEMANTIC_REPLACEMENTS (the
      audits pin that list) and MAIN.CPP takes it over in OWNER_REPLACEMENTS,
      as tactics did; native_service's runtime tests skip it on the paths that
      now reach MAIN.CPP's hook; its event checks were removed (that facade is
      checked by main_program_*).
- [ ] native_service_component_equivalence and modern_native_service_production
      fail before any MAIN.CPP change (the NewPtr entry, which nothing here
      touched, fails the same "ordered effects/PC/register prefixes" check):
      the old instruction-exact rule. Binary removed again; rework with the
      reference test set below.

## Now: optimal tactics (goal set 2026-10-24)

Finish line: two .tct tactics, (1) an exploit that uses what only a
hand-edited file or an engine flaw allows, (2) an honest one (every value
settable in the tactics screens), each the best found by measured search,
validated on held-out matchups against the best community tactics.

- [x] `cm-tactic-lab`: full detailed matches between two original clubs with a
      .tct injected into one side's AI team setup (after the formation choice
      and the manager's adjustments), in-match AI tactic changes undone; ~0.45 s
      per match. Factory split into prepare_match/start_match for it.
- [x] Engine reading of tactics (static, two agents): spots -> 9x12 zone grid ->
      targets; positional penalty from position codes; which option/instruction
      bits the engine reads (see docs once written).
- [x] Baselines: AI choice 1.09 ppg on the panel; top community tactics
      1.6-2.0 (community benchmark: 3,526 tactics, .local/community-tactics-full).
      Hand-placed spots are worth ~0.4 ppg (respot drops 1.80 -> 1.36).
- [x] Measured exploit probes (all <= base): code 0 (no positional penalty;
      ~500 assertion dialogs per match in the real game), Att+Def mentality,
      multi-bit fields, free-role bit, roles; spots y >= 36 crash the engine.
      Promising: decoupled central codes for wide midfielders (+0.16 +- 0.12).
- [x] Race of the 150 best community tactics plus the best probe on 16
      matchups (successive halving, 2,016 matches each at the end): the top
      five are within 0.06 ppg (4132 Marc 2.085, scemoka 2.089, ...).
- [x] Honest search: coordinate rounds from 4132 Marc over every team option
      (11), instruction (230) and spot shift (240), each screened on 128
      matches and the best six confirmed on 1,024 paired matches; no single
      change beat 2 SE (local-optimality certificate). The three best
      near-misses combined (+0.04 +- 0.04 on train and on holdout) give the
      honest candidate.
- [x] Exploit search: coordinate rounds over codes (251), instructions and
      team options from the honest candidate with central codes for the wide
      midfielders; accepted both strikers coded F-RC (+0.114 +- 0.039), then
      a full cycle with nothing better.
- [x] Validation on 8 unseen matchups, 2,048 matches each, paired with Marc:
      exploit +0.167 +- 0.035 ppg, honest +0.077 +- 0.035. Files in
      .local/tactics-optimal (Lab Exploit.tct, Lab Honest.tct).
- [x] Certificates on fresh seeds for the final files: the honest file over
      team, instructions and spots (nothing passed); the exploit's spots
      accepted the left back's column 1 three rows up (+0.112 +- 0.039), then
      spots, codes, instructions and team passed nothing. Validation again:
      exploit +0.219 +- 0.035, honest +0.052 +- 0.035 vs Marc.
- [x] The game's own loader (`load:FILE`): the exploit raises 10 assertion
      reports and keeps its codes; same matches as the lab build.
- [x] In the app (isolated validation copy `tactics-exploit`, driven through
      its automation channel): Tácticas -> Carregar -> Lab Exploit -> Ok shows
      "Error v3.9.62 FORMATION..CPP 712" ten times; after the tenth the
      screen's record and, after Ok, the club's record hold the file's codes
      (M-C, DM-C, M-C, F-RC, F-RC, M-C), instructions, team options and all
      528 spots (read from guest memory). Saving the career and loading it
      again in a fresh launch: no dialogs, the club's record still identical.
- [x] Multi-start: the honest search from 4-1-2-1-2 scemoka and 262 MMJ
      accepted nothing in a full cycle (both are single-change optima of their
      own). Validation, third seed set, paired with Lab Honest: scemoka
      -0.046 +- 0.035, Marc -0.057, MMJ -0.112, exploit +0.152. Both final
      files stand. How to run another loop: docs/tactic-lab.md, "Running
      another search".
- [x] Commit the lab (review pass done earlier), tools/tactic_lab.py, a short doc.

## Done: the tactics system in C++23 (goal set 2026-10-23, all 234 hooked)

Finish line: every routine of the original's `FORMATION.CPP`, `TACTICS.CPP` and
`TACTICS_SCREENS.CPP` (234 routines, 43,152 instructions; `tools/tactics_audit.py`)
runs as hand-written C++ in production, each checked against the generated
reference on captured calls and by end-to-end replays, suite passing, one
reviewed commit per block. Method: docs/cpp-rewrites.md.

- `FORMATION.CPP` 0x10110c80-0x10118ec0 (47): formation record (0x18e0 bytes),
  positions, zones, shape code, staff slots, instruction bits (+0x1888), `.tct`
  files. Called by the match engine, recruitment (0x10117490, 29 callers), screens.
- `TACTICS.CPP` 0x103acf80-0x103b3830 (30): CTACTICS, per-team records (0x94
  bytes at +0x5c4), set pieces, captain, presets, `tactics.dat`, AI choices.
- `TACTICS_SCREENS.CPP` 0x103b3830-0x103cf5f0 (157): the tactics, team
  instructions and penalty-taker screens.
- The 26-country day reaches 33 routines (`.local/tactics/census-day.txt`); the
  rest need session captures of the screens and file dialogs.

- [x] Inventory and pins: `tools/tactics_audit.py`, `native/tactics_routines*.inc`
      (TACTICS_ROUTINE hooked, TACTICS_DRAFT check tools only).
- [x] Guest ABI layer (`native/guest_abi.hpp`), `tools/call_registers.py`.
- [x] Fork-based routine checker (`cm-world-replay --check-routines`, `--only`,
      `--census`, `--strict-volatile`, `--synthetic`), with a negative control test.
      Exit rule: r4-r10 and every CR field as the original (they leak through
      later varargs spills and `mfcr`).
- [x] Session captures (automation `capture start/stop`, `drag FROM TO`): 13 UI
      sessions in `.local/benchmarks/ui` (game-save2 reaches the career save's
      tactics write; game-save stops before it), all replaying to their end state.
- [x] All 234 routines hooked (TACTICS_ROUTINE), every captured call equal to
      the original on 14 captures plus 845 synthetic calls; reviewed per group.
      The tactics hook replaces the builder hooks at 0x10113e10, 0x10117360,
      0x10117490. Save and reload on a career copy with the C++ save: OK.
- [ ] Not replayed (reviewed against the listings only): assertion paths (the
      error dialog's imports aren't in transcripts), national-team paths, the
      handler branches listed in 2a9e3a7, construct's AI-club path, old-save
      conversion, the preset dialog callback 0x10118210 (only with the presets
      flag at TOC -0x6330 set; same code as 0x10118180 apart from r6).
- [ ] Cleanups: one shared tactics helper header (fail/in_range/compare and the
      FPSCR model repeated in 4-5 files); direct C++ calls between the rewritten
      routines instead of `c.call` through the dispatcher (the performance step).

## Next: native lowering (decided 2026-10-22)

Victor's target: identical in-game results (same RNG seeds and draws) and the highest speed possible, well beyond 10x; the code may differ completely. The generated pages emulate the PowerPC: registers live in `State::registers`, a byte array the compiler must reload after every guest-memory store (both are `uint8_t`, so they may alias); every access is bounds-checked; every run is counted; every call goes through `run_until`/`dispatch_page`. Lowering each routine to a C++ function removes that:

- [x] ~~Registers as C++ locals inside a routine~~ — tried 2026-10-22 and not kept. 51 of the 60 hottest generated routines (34% of the week's self time) lowered to functions with registers in locals, rewritten from their page code: identical end states and instruction counts on every capture, but only ~1% faster (day capture, 5/5 pairs: 23.33 -> 23.08 s). Generated code already runs ~750M guest instructions/s (~1.3 ns each); register traffic is not the cost. Review also found that `run_until(s, return)` stops on a PC match alone, so a recursive re-entry that falls back to page mode can return to the outer lowered frame early; a lowering needs a frame check and should render from the IR, not reparse page C++. Sources kept outside the repo.
- [ ] **Where the time is (day capture, native sample):** the game's own work (billions of guest instructions per game day), builder-based owners stepping per instruction (`SemanticGuestCall::load/store/multiple`, ~10%), dispatcher round trips (`run_until`/`dispatch_page`/`run_leaf`, ~8%). The 10x path is algorithmic: rewrite hot subsystems so they execute fewer instructions for the same reads, writes and RNG draws (the list search did this: -10% of the week), starting with the recruitment candidate scan (30% of the week: own loop 6.3%, list search 3.8%, assessment 3.5%, `0x101ad080` 2.8%, contract terms 2.2%, transfer offer 2.2%).
- [ ] Condition and XER/FPSCR bits computed only where something reads them (liveness in the generator).
- [ ] Direct C++ calls between lowered routines (no dispatcher); indirect calls through one table.
- [ ] No instruction accounting in production builds (counts are diagnostics); keep it for checks.
- [ ] Guest memory: hardware guard pages instead of software bounds checks where a fault is a bug anyway.
- [ ] Then the hand-written C++ subsystems call each other directly (named types), replacing lowered routines by measured cost.

## Next: rewrite track

The end goal is every routine in C++ (see CLAUDE.md). Start where speed and rewriting overlap.

- [ ] Recruitment distribution and staff-link callees behind the 8-minute Continue: rewrite the hot generated callees (`0x1035e9f0` candidate scan, `0x101b1620` list search, `0x1014c2e0`) as real C++ called directly from their rewritten parents.
- [ ] Sorted insertion in add-link instead of append + full heapsort (`native/shortlist_links_storage.cpp`), now allowed: ~17–22 s per weekly Continue.
- [ ] Rebuild a runnable reference test set: only 63 of 298 tests run here. The old component tests (e.g. `shortlist_links_component_equivalence`, not built here) require identical dependency calls, instruction counts and budget cuts — the rule replaced on 2026-10-20 — so they fail on add-link's skipped sort and will fail on block accounting. Rework them to compare what the game observes (as `cm-world-replay` and `cm-shortlist-links-insert-checks` do).

## Next: speed

From the 2026-10-20 review. Estimates are unmeasured.

- [ ] Global-variable (TOC) reads miss the inline bounds check (`native/aot_runtime.cpp:54-63`). A begin-offset entry was slower in match play (extra compare on every access); re-test on world processing, where generated code dominates.
- [ ] Native sort: accept comparator `0x10309a00` (`tools/native_sort8.py`), ~6 s per weekly Continue.
- [ ] FPSCR bookkeeping is about half the statements on FP-heavy pages; emit one helper, or compute it lazily for its three readers.
- [ ] Fuse the candidate-scan helpers (`0x101b1620` + iterator `0x10738e00`, `0x1036f590` at `0x1035f734`, `0x10362d70`), est. 8–15 s.
- [ ] Generator: per-block instruction accounting; switch cases only at entry points; direct calls to known cross-page targets.
- [ ] Re-profile the weekly Continue with a cheaper counter: the current probe inflates per-entry seconds and ~160 s is unattributed.

## Later: fixes from the review

- [ ] `divw` INT_MIN / -1 guard never fires (returns `0x80000000`). Divide by zero now continues at the 1,385 quotient-free sites; the other 446 still stop (what the hardware returns there is undecided).
- [ ] `lfs` quietens signalling NaNs.
- [ ] Unknown intrinsics compile and throw at runtime; fail generation instead.
- [ ] `native_record_fields` promotion accepts calls that read `State` (latent).
- [ ] Turbo stores speed 0 in a manager record; check whether it reaches saves.
- [ ] An audio output device change stalls sound channels.
- [ ] Copy-on-write of an original data file isn't atomic (`native/platform.cpp`, `writable_file`).
- [ ] The installer writes the Game manifest after moving Game into place.
- [ ] Names over 31 characters can overflow the game's 64-byte name buffer (`0x1076ec60`).
- [ ] No App Nap / idle-sleep assertion during long processing.
- [ ] Run logs accumulate in `Runtime/Runs`; input tracing is on by default.
- [ ] Mouse-moved events pile up while the game is busy; every small `CopyBits` redraws a full frame.
- [ ] Idle at the start menu the app uses 100% of a core: the original main loop calls `WaitNextEvent` with sleep 0 (~10,000 calls/s; confirmed 2026-10-21 with an import trace). Blocking a millisecond when such a poll finds no event would idle the core; it changes only wall-clock readings.
- [ ] Three jump tables don't match `check_jump_tables.py`'s pattern; inspect them by hand.
- [ ] Single-precision multiply-add with non-single operands rounds to double, then single (`native/fused_multiply_add.hpp`); exact would need round-to-odd. Rare.
- [ ] Builder multiply-adds look up their kind per call (binary search over 1,049 sites); resolve per call site if it ever shows in a profile.
- [ ] 250 of 288 tests can't run here: their binaries aren't built or their inputs were deleted by earlier cleanups. Restore the ones that matter.

## Profile notes

- Match play (600 advances, Arsenal, 2026-10-20): ~12% generated pages; ~28% routine-to-routine plumbing (`run_until`, `dispatch_page`, dispatch/native scopes, thread-local lookups, hook entry, diagnostics checks); ~15% `SemanticGuestCall` instruction methods; `spatial_role::invoke` 12.6%; rewritten bodies ~11%; guest memory ~8%.
- World processing, interest pass (`cm-world-replay --profile`, 9,208 guest samples over the four captures, 2026-10-21): register club interest `0x10363f60` (builder-based owner) 91.5% inclusive / 39.1% self; generated callees `0x1014c2e0` 10.8%, `0x100f5770` 10.7%, `0x100c7d00` 8.4%, `0x100d9580` 4.4%; contract query `0x10082f90` 6.4%; assessment `0x10361490` 21.3% inclusive.

## Known test state

- `tools/dev.sh test`: 94/95 runnable tests pass (2026-10-23, Best XI/localization build); `debugging_best_xi`, actual goals and injury shortcuts pass. `match_debugger_actual_lineup` failed from ed7749e to 2026-10-25: it required the `match_actor` owner's call count, and production pages run that family's generated code. It now requires each lineup edit's effects to be non-empty and equal to the original setter's on the control, whichever implementation runs. `match_engine_live_equivalence` and most others lack a built binary or inputs.
- Python audits: every `tools/test_*.py` passes against `.local/current/export`; `test_generated_temporaries.py` (needs a build dir) not run.

## Euro symbol (2026-10-23)

- [x] Locate the original E currency prefix and both native font paths.
- [x] Render the euro prefix as € with unchanged guest text and layout; both font paths, other currencies and 1x/2x/4x raster checks pass. Built and installed.
