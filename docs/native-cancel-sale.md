# Cancel a scheduled sale

With Debugging enabled, the player Action menu offers **Cancel Sale** for a
permanent transfer queued from the current manager's club. The action cancels
the entire transaction, including any exchanged players; it does not change
their current contracts or cancel unrelated loans and offers.

The original future-transfer queue and status 19 identify a scheduled sale.
Contract acceptance alone is not enough. Cancellation uses the original queue,
contract and transfer services, retains the offer in terminal status for old
references, and schedules its normal collection. Payment is made by the
original transfer completion path, after the stage targeted here. The player
page's cached offer indices are invalidated after a successful cancellation.

The same menu offers **Remove Release Clause** when a primary contract has a
minimum-fee release clause. The original clause-clear service removes only that
option; unrelated clauses, the stored amount and other terms are preserved.
