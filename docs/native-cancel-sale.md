# Cancel a scheduled sale

The player Action menu offers **Cancel sale** for a permanent transfer queued
from the current manager's club. It does not require Debugging. The action
cancels the entire transaction, including any exchanged players; it does not
change their current contracts or cancel unrelated loans and offers.

The original future-transfer queue and status 19 identify a scheduled sale.
Contract acceptance alone is not enough. Cancellation uses the original queue,
contract and transfer services, retains the offer in terminal status for old
references, and schedules its normal collection. Payment is made by the
original transfer completion path, after the stage targeted here. The player
page's cached offer indices are invalidated after a successful cancellation.
