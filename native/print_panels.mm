#include "print_panels.hpp"
#import <AppKit/AppKit.h>

namespace cm::aot {
namespace {
NSPrintInfo* print_info(PMPrintSettings settings, PMPageFormat format) {
    NSPrintInfo* info = [[NSPrintInfo sharedPrintInfo] copy];
    if (::PMCopyPageFormat(format, static_cast<PMPageFormat>(info.PMPageFormat)) != noErr) return nil;
    [info updateFromPMPageFormat];
    if (settings) {
        if (::PMCopyPrintSettings(settings, static_cast<PMPrintSettings>(info.PMPrintSettings)) != noErr) return nil;
        [info updateFromPMPrintSettings];
    }
    return info;
}
PrintPanelResult retained(NSPrintInfo* info) {
    PrintPanelResult result{static_cast<PMPrintSession>(info.PMPrintSession),
                            static_cast<PMPrintSettings>(info.PMPrintSettings),
                            static_cast<PMPageFormat>(info.PMPageFormat)};
    ::PMRetain(result.session); ::PMRetain(result.settings); ::PMRetain(result.format);
    return result;
}
}
std::optional<PrintPanelResult> run_print_panel(PMPrintSettings settings, PMPageFormat format) {
    @autoreleasepool {
        NSPrintInfo* info = print_info(settings, format);
        if (!info) return std::nullopt;
        NSPrintPanel* panel = [NSPrintPanel printPanel];
        // No page range: SIPRINT draws every page and these jobs do not filter them.
        panel.options = NSPrintPanelShowsCopies | NSPrintPanelShowsPaperSize | NSPrintPanelShowsOrientation;
        if ([panel runModalWithPrintInfo:info] != NSModalResponseOK) return std::nullopt;
        return retained(info);
    }
}
std::optional<PrintPanelResult> run_page_layout(PMPageFormat format) {
    @autoreleasepool {
        NSPrintInfo* info = print_info(nullptr, format);
        if (!info) return std::nullopt;
        if ([[NSPageLayout pageLayout] runModalWithPrintInfo:info] != NSModalResponseOK) return std::nullopt;
        return retained(info);
    }
}
}
