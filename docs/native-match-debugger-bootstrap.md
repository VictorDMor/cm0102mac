# Independent match debugger database bootstrap

## Result and boundary

The debugger can now create a fresh live world from the original game database without opening a career, loading a save or restoring a captured match. `DatabaseCatalog` supplies real club IDs and names; `DatabaseWorld` owns an independent guest state and executes the same production dispatcher and remaining original loading dependencies as the game.

This block delivers a database prerequisite. The standalone `.app`, arbitrary-club fixture and lineup factory, match-service initialization and player/goal interventions remain pending. The semantic manifest remains at 601 entries; host orchestration does not count as reconstruction of the original database worker. The installed game executable is unchanged.

## Original data contract

The original Mac index has an eight-byte header and 68-byte descriptors. Domestic and national club records use a 584-byte stride and IDs 0 through 11,018. All present original database tables have records beginning at byte zero and a final `0xf1fffff1` word. Earlier asset documentation incorrectly called this a prefix. The original reader at `0x107464d0` saves the stream position, seeks four bytes before EOF, reads the marker and restores the position. The club reader at `0x105d8f60` uses stride `0x248` for the Mac representation.

The selection catalog loads 10,593 clubs and 426 national teams. It preserves original record bytes, decodes English names to UTF-8 and applies the existing audited English club-name alignment in memory. Original files are never rewritten. Malformed descriptors, duplicate/out-of-order IDs, invalid or misplaced markers, truncated records and mismatched language counts are rejected.

## Fresh live world

The bootstrap starts at `0x10773710` and stops at `0x10773754`, after the actual twenty global initializers and before the application's event loop. It retains the reserved startup frame and calls `0x10118ef0` to initialize the original settings and application objects.

It then calls `0x1009ef90` with the original `CM 01-02:Data` path and second argument 1. That wrapper selects ordinary file input; `0x1009ef30` selects compressed input. Both are read paths. Argument 1 selects fresh table allocation and the original `0x100af8a0` ID-to-pointer resolver. Argument 0 is not a substitute: raw IDs remain unresolved at the later live-world checks.

The live club array at TOC `-0x7be8` contains the combined domestic/national records; `-0x7b50` supplies its count. Staff base/count come from `-0x7be0`/`-0x7be4`. Club `+0x98` contains fifty resolved staff references. Staff `+0x14` points to the actual player record, using the player base/count at `-0x7b38`/`-0x74d4` and a `0x48` stride. Staff `+0x0c` is a name reference and must not be treated as a player.

Each world requires a new private directory, independent of both original input roots. Existing debugger configuration cannot silently seed a new experiment. The platform writes to that world's private overlay. Invalid selection data is rejected before creating writable backing. Failed bootstrap state is not returned as a usable world. Ownership can be transferred once to the future match factory; subsequent state access is rejected.

## Focused validation

- Four Swift asset validation groups pass, including trailer validation and misplaced-marker rejection.
- Catalog checks pass against the actual original database: ten assertions and fifteen malformed/unknown input rejections.
- Three focused CTests pass: catalog structure, actual database and fresh live world.
- Two newly created worlds independently resolve all 11,019 live club identities, 5,320 populated rosters and 92,199 player references. These are roster references, not a count of unique players.
- Arsenal and Manchester United have complete real rosters in both worlds. Corresponding staff IDs and all 72 bytes of their original player records agree.
- A guest write remains confined to its owning world. Ten invalid operations are rejected, covering input overlap, zero budget, scratch reuse, unknown/mutated identities, unresolved staff/player references and repeated ownership transfer.
- UBSAN passes for the affected catalog, bootstrap and check units; output agrees with the ordinary check and stderr is empty. Remaining generated dependencies were not newly sanitized.
- The existing production archive was reused. This block did not generate or compile a duplicate set of 1,922 pages.

Private evidence is retained under `.local/match-debugger-bootstrap`; original program/game files, installed career files and the installed app are compared before and after. Career persistence is **not retested**. Performance is **not benchmarked**.

## Next dependency

Initialize the required original match services in their characterized order, build an isolated fixture and consistent selected lineups, and invoke the original match constructor (`0x101d16a0`) and initializer (`0x101d1760`). Acceptance requires productive advancement through the already shared C++23 engine for more than one real club pairing. A successful database load alone does not meet that requirement.
