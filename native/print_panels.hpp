#pragma once
#include <ApplicationServices/ApplicationServices.h>
#include <optional>

namespace cm::aot {
// The Carbon dialog calls do not exist for 64-bit processes; AppKit's panels
// show the same choices. An accepted panel owns new objects carrying them (the
// session holds the chosen printer), each retained for the caller.
struct PrintPanelResult {
    PMPrintSession session;
    PMPrintSettings settings;
    PMPageFormat format;
};
std::optional<PrintPanelResult> run_print_panel(PMPrintSettings settings, PMPageFormat format);
std::optional<PrintPanelResult> run_page_layout(PMPageFormat format);
}
