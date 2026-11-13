#pragma once
// The original's TRANSFER_OFFER.CPP: the transfer offer record (who bids for whom, the fee and its
// instalments, extras and players in exchange), how an offer is built and valued, and its form in a
// saved game. 17 routines, 0x10411e60..0x10417590 (tools/transfer_offer_unit_audit.py). Contract and
// quirks: docs/cpp-rewrites.md. (native/transfer_offer_* without "unit" is an older owner of
// routines of TRANSFER_MANAGER.CPP.)
#include "transfer_offer_unit_record.hpp"
#include "transfer_offer_unit_value.hpp"
#include "transfer_offer_unit_build.hpp"
