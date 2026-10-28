# Debugging: extend a player contract

With Debugging enabled, a player with a primary contract has **Extend contract by
1 year** in the Action menu. It advances the existing expiration by one calendar
year, clamping February 29 to February 28, and keeps the remaining contract
terms. Both the primary contract and staff copy of the expiration are updated.

Direct offers from other clubs whose scheduled arrival follows the current
contract's expiration are withdrawn, including matching deferred negotiations
and accepted agreements already placed in the future-transfer queue. Ordinary
purchase offers, loans, immediate direct hires and the current club's renewal
are retained. The action can be repeated. A missing, invalid or inconsistent
current contract is left unchanged.

The original transfer representation uses `offer+0x37` for a direct approach;
the timing nibble at `+0x38` distinguishes dates based on the current
expiration (3 and 4) from immediate/window routes. The offer's price alone is
not a reliable discriminator. A pending/deferred contract is matched to its
original offer by person, destination, index and revision. The original list
unlink and transfer removal operations maintain the existing counters and
linked records. Scheduled agreements are also found directly in the original
future-transfer queue, even if the pending negotiation has already gone. The
original unschedule operation runs before deletion; person-wide transfer flags
are retained when another scheduled or selected agreement still requires them.
After a successful action, the original player-screen cache cleanup forces the
next redraw to rebuild its offer indices instead of reading deleted slots.
Before any removal, ambiguous pending rows or an exchange list on a selected
offer make the action decline rather than delete a different negotiation.
