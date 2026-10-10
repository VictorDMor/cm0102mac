# Semantic Mac stream storage, callbacks and lifecycle

## Scope and reference

Thirteen named C++23 operations reconstruct stream descriptor acquisition,
initialization, list disposal, flush selection, CR/LF exchange, aligned preparation,
callback refill/drain, buffer configuration, close and flush. Guest metadata and
callback descriptors remain big-endian word32. No host FILE or host stream/container
substitutes for the Mac descriptor or its buffering semantics. The productive
allocator and memory helpers remain semantic dependencies; item I/O, seek/tell,
mode/open parsing, lifecycle registration and platform file services outside this
block remain separate productive dependencies.

The starting checkout and freshly queried remote main are
`7826fad7667398eeaaa404ca68dc707672c75c77`. Append these entries after the exact
prior 127; the configured manifest has 140. The prior compact ordered JSON hash is
`03903b03d1f5f70885005976ccad6775dc338ce3983b6cf0ecc4b84e4b777164`.
The original Mac executable identity remains
`91692f95df17cf851e7c1d6dbdecd0bd0a5809ce1937133e209aee859a6b516b`.
The index is read only and private original listings/payloads are not published.

## Pinned contracts and production integration

Pins serialize address/assembly/ops/flows/fallthrough in ascending address order,
sorted compact UTF-8 JSON. Intervals are end-exclusive; the total is 541 indexed
instructions. None of these bodies crosses a page. All indexed flows/fallthroughs
were queried and there are zero external edges into their interiors.

| Entry | End exclusive | Instructions | Role | SHA-256 |
| --- | --- | ---: | --- | --- |
| `0x10775240` | `0x107752cc` | 35 | Reusable/dynamic stream acquisition | `727ebbeecba70fa1b66085ac8aebb7ce7b7af631e03733834d6d4b359bc00f53` |
| `0x107752d0` | `0x107753a8` | 54 | Stream descriptor initialization | `5ffe78935c6e6804c121f787bdebfeebd1662a8c9fa724772505015b96f344f3` |
| `0x107753b0` | `0x10775440` | 36 | Stream-list close/disposal | `0d7459eda69f6e5e9a518e6e1bed8df88074024ec8f699ab8baf4301113b4b6f` |
| `0x10775440` | `0x107754a8` | 26 | Flush all streams | `f3b43472f6083e1ee7598fe29a0d444df8b0552dc4adf39cf489874df8e8c910` |
| `0x107754b0` | `0x10775534` | 33 | Flush line-buffered output streams | `ab620579770f90bb04c88e5817d423266999458c37041763d2c00b765dda5a78` |
| `0x107756f0` | `0x10775734` | 17 | Output CR/LF exchange | `31f69e361d3765f759b2f0797bbd4d31b9a002ff48eb02e932f24332bfd9a971` |
| `0x10775740` | `0x10775784` | 17 | Input LF/CR exchange | `0a7302713b0dc2dd3bf4930504813bf463671aa12d0bdc9c3eddd4baab437208` |
| `0x10775790` | `0x107757c4` | 13 | Aligned buffer preparation | `a9920b6c21e2b26b0f2323439ea3ff5d22ce4504572cd284b470e710b0746f25` |
| `0x107757d0` | `0x10775898` | 50 | Callback buffer refill | `d6c366198ea2ac1d35fcf38b9693d36dcb3ac6f794d721a2957f3a1ab07eb218` |
| `0x107758a0` | `0x10775954` | 45 | Callback buffer drain | `3d109d0a7092f287dd63799d58f58d7c8f0f0323409bcba9922fa3ac554f4022` |
| `0x10775960` | `0x10775ac4` | 89 | Buffer mode/ownership configuration | `28c1c19daa15dccf33b92ca35486a9e39d6aa431b1d0550cf1b7abe0846353a8` |
| `0x107766f0` | `0x107767b0` | 48 | Stream close | `665d5317a3ff438254c1c8a3648492379ac07201bc2bef3a3135ceb8aeda026d` |
| `0x107767b0` | `0x107768e8` | 78 | Stream flush | `57b8f86a7a17877a7b5a7c166da8190517cf20a8898277c21643930366541648` |

Hooks precede the first original operation at generated destination labels on
pages 0x10775 and 0x10776. Local, cross-page and generated CFM dispatch share those
hooks. The default runs the new bodies; private global/per-entry original selection,
resume, counters, observers and reset retain the existing diagnostic pattern.
Bodies call named children or unchanged external dependencies, never their own
replaced bodies. Unknown transfers remain failures and have separate diagnostics.

The new shared MacAbi helper extracts the allocator's existing frame/register/
CR/access/call operations without changing their behavior. Both modules use it;
allocation policy and the next_pool_attempt correction remain untouched.

## Guest layout and contracts

Descriptors are 84 bytes. +0 is the callback handle; +4/+6 retain packed mode
halfwords; +8 contains operational state/ownership bits; +12/+13 are EOF/error
bytes; +16 marks dynamically allocated descriptors and +17 is the inline byte.
+28 is the observed position, +32 buffer base, +36 capacity, +40 current pointer,
+44 remaining/transferred count, +48 alignment mask, +56 saved position.
Callback transition vectors occupy +60/+64/+68/+72; +76 is refcon and +80 next.
The root is live through r2-0x5a68. Acquisition starts through root+248; when that
chain is null, the original uses the incoming r30 as the link predecessor. That
residual ABI dependence is retained rather than replaced by an inferred sentinel.

Class is `(mode_halfword >> 6) & 7`. Configuration modes 0/1/2 are accepted only
for an open descriptor in idle operational state; invalid modes retain failure.
Mode zero invokes flush first. A prior owned buffer is freed through the guest
allocator. Configuration establishes the inline one-byte fallback before optional
allocation, so failure can retain those already written descriptor effects.
Borrowed buffers remain borrowed; successful private allocation sets ownership.
Class 1 capacities divisible by 512 select alignment mask 511. Capacity and
position arithmetic wraps in word32. Other classes retain their original mask.
Initialization preserves untouched bytes and reads default callback vectors live
from four TOC slots only for class 1; it does not install host function pointers.

Preparation resets pointer/count from buffer/capacity, subtracts position & mask,
and saves the live position. Each reread/store retains order, including aliases.
CR/LF conversion exchanges both bytes, captures the count once into CTR before
stores, advances the pointer and preserves zero-length behavior. It does not
normalize CRLF pairs or introduce modern text/Unicode rules.

Refill prepares the buffer, optionally overrides the aligned capacity for force=1,
then calls the live read vector with handle/buffer/count pointer/refcon. Status 2
clears the count. The optional out count is written before success-only position
advance and text conversion. A count out-parameter may alias the position and
change the value that is subsequently reread.

Drain captures pointer-minus-base, converts text before the live write callback,
and retains that converted prefix even when the callback fails. Only success
advances position and reprepares the buffer. An empty drain skips the callback
and the out count. Read/write callbacks may alter count, payload, volatile ABI
state and their TOC; the caller restores TOC from the original linkage slot.

Flush(nullptr) selects the list-wide helper. Error/closed descriptors return -1;
input-only streams return success. Unget states normalize as on Mac; output drain
failure sets the sticky error byte and clears the count. Successful flush resets
operational state and position through the existing class-specific tell dependency.
Close(nullptr) returns -1 and closing an already closed descriptor returns zero.
An open descriptor is flushed, its close callback runs, mode/handle are cleared,
and owned buffer storage is released in original order. Flush or callback failure
returns -1 with the original carry effect. List disposal preserves static nodes,
marks them class 3, and detaches subsequent dynamic nodes before freeing them.
Line-buffer selection tests open mode, line-buffer bit and output state separately.

## Acceptance

The consistent Release ARM64 build and focused acceptance passed. The harness
completed **234 comparisons**, including **39 expected mapping faults**, and
rejected **17 discriminating negatives**: one at each entry's own return and four
at coupled child returns before a parent can hide the mutation. Every architectural
register/temporary byte, mapped guest byte, first-fault PC, allocator state,
ordered memory effect, CFM argument/link/refcon/TOC and imported native storage
history was compared. Instruction accounting and process-local diagnostics are
excluded. Budget exhaustion is not accepted as a mapping fault.

Coverage includes direct and generated CFM dispatch for every entry; ten mixed
original/modern parent-child configurations; minimally mapped success and three
removed-access-byte faults per entry; relocated frames and live TOCs; empty,
aliased and wrapping counts; all callback statuses; text/binary conversion with
partial effects retained on errors; callback count/payload and volatile ABI
mutation; reused/appended descriptors; heap failure; inline, borrowed and owned
buffers; owned-buffer release; static/dynamic list disposal; line-buffer selection;
flush-all error continuation; sticky/closed rejection and file-class tell before
drain. Reset preserves the independent allocator diagnostics.

The seven new pin/hook/index audits passed, as did all 53 tests in the ten affected
prior audit suites. The prior 127 manifest entries match their ordered hash.
Generation completed with 1,953,157 instructions, 1,922 pages, zero missing and
zero pending destinations. Six related CTests passed: allocator equivalence,
original allocator replenishment, memory primitives, formatted input, formatted
output and native bridges. Allocator equivalence retained 337 comparisons,
86 expected faults and 30 rejected negatives after shared ABI extraction.

Four focused UBSAN executables passed with empty diagnostic stderr: the stream
harness, allocator harness, replenishment harness and probe. All four new stream
units, four affected allocator units, the shared runtime and probe sources were
instrumented and linked against the consistent generated library. Unchanged
platform services and generated bodies are not claimed to be fully instrumented.

### Individual normal execution witnesses

All harness entries below have `direct=1`, `original=0` and `escapes=0`.
The installed menu smoke has all thirteen diagnostic modes `modern`, zero
original calls and zero dispatch escapes, but does not execute every branch.

| Entry | Focused normal semantic calls | Installed menu smoke calls |
| --- | ---: | ---: |
| `0x10775240` | 8 | 29 |
| `0x107752d0` | 7 | 29 |
| `0x107753b0` | 4 | 1 |
| `0x10775440` | 5 | 0 |
| `0x107754b0` | 15 | 0 |
| `0x107756f0` | 44 | 0 |
| `0x10775740` | 16 | 109 |
| `0x10775790` | 56 | 15865 |
| `0x107757d0` | 22 | 15863 |
| `0x107758a0` | 45 | 1 |
| `0x10775960` | 53 | 29 |
| `0x107766f0` | 24 | 61 |
| `0x107767b0` | 54 | 32 |

### Installed artifact and preservation

The linker snapshot, signed source, accepted candidate and pre-install reference
are retained privately. The internal executable is ARM64; source, candidate,
installed application and running smoke process all match SHA-256
`41326fc78142dcd8bd323c4139f6453842c0e1cf977be180378af718c7f9e97f`.
The preserved prior installed executable is
`14eb1c373854e95c6e642ebc1ddd99ae56f9c353ecb49de93e0822cc93e44ae8`.
Strict signatures and normal launch metadata passed; all four bundled original
runtime payloads match their source hashes.

The installed application rendered Prepare Game, opened the saved-game chooser,
cancelled without loading, displayed the exit confirmation and exited normally.
There were no fatal runtime diagnostics; the native process is closed. A native
accessibility observation after exit relaunched a further owned test session; that
session was closed normally and final process absence was checked without another
accessibility observation. The user's separate brief play/exit is not substituted
for scripted-flow or persistence evidence.

Before installation, the reference and full content/metadata inventories protected
809 Game entries, five Runtime entries, 57 Runs entries, the original payloads,
save snapshots, 23 private handoffs and 123 historical application bundles.
Immediately after installation these protected inventories matched. After UI
execution, Game, saves, payloads, handoffs, reference and every historical bundle
still matched their protected content and metadata, including forks, extended
attributes, Finder information, modes, timestamps and flags. Runtime, Runs and
presentation preferences changed legitimately during execution and are not claimed
to remain identical after UI use. Private listings, generated units, binaries,
payloads, logs, saves, references and untracked handoffs are excluded from Git.

**Save/Load: not retested.** This focused stream block does not establish complete
file-service, C-runtime or whole-game acceptance. The full modern-port objective
remains active, with buffered item I/O as the next characterized implementation.
