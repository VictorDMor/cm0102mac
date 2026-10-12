#include "match_debugger_database.hpp"
#include "match_debugger_worker.hpp"
#import <AppKit/AppKit.h>
#include <filesystem>
#include <cstring>
#include <signal.h>
#include <unistd.h>

namespace {
NSString* utf8(std::string_view value) {
    return [[NSString alloc] initWithBytes:value.data() length:value.size() encoding:NSUTF8StringEncoding];
}
NSTextField* label(NSString* value, CGFloat size = 13) {
    NSTextField* result = [NSTextField labelWithString:value];
    result.font = [NSFont systemFontOfSize:size];
    return result;
}
NSStackView* stack(NSArray<NSView*>* views, NSUserInterfaceLayoutOrientation orientation) {
    NSStackView* result = [NSStackView stackViewWithViews:views];
    result.orientation = orientation;
    result.alignment = orientation == NSUserInterfaceLayoutOrientationVertical ? NSLayoutAttributeLeading : NSLayoutAttributeCenterY;
    result.spacing = 12;
    return result;
}
NSButton* button(NSString* title, id target, SEL action) {
    return [NSButton buttonWithTitle:title target:target action:action];
}
NSScrollView* table_scroll(NSTableView* table, NSArray<NSArray*>* columns, id delegate) {
    for (NSArray* description in columns) {
        NSTableColumn* column = [[NSTableColumn alloc] initWithIdentifier:description[0]];
        column.title = description[1];
        column.width = [description[2] doubleValue];
        column.minWidth = 45;
        [table addTableColumn:column];
    }
    table.delegate = delegate;
    table.dataSource = delegate;
    table.usesAlternatingRowBackgroundColors = YES;
    table.columnAutoresizingStyle = NSTableViewLastColumnOnlyAutoresizingStyle;
    table.rowHeight = 25;
    NSScrollView* scroll = [NSScrollView new];
    scroll.documentView = table;
    scroll.hasVerticalScroller = YES;
    scroll.autohidesScrollers = YES;
    scroll.borderType = NSBezelBorder;
    return scroll;
}
}

@interface MatchDebuggerApp : NSObject <NSApplicationDelegate, NSTableViewDataSource, NSTableViewDelegate>
@property NSWindow* window;
@property NSComboBox* home;
@property NSComboBox* away;
@property NSTextField* seed;
@property NSTextField* status;
@property NSTextField* score;
@property NSTextField* inspection;
@property NSButton* createButton;
@property NSButton* stepButton;
@property NSButton* runButton;
@property NSButton* pauseButton;
@property NSButton* resetButton;
@property NSButton* withdrawButton;
@property NSProgressIndicator* progress;
@property NSTableView* playerTable;
@property NSTableView* eventTable;
@property NSArray<NSDictionary*>* players;
@property NSMutableArray<NSDictionary*>* events;
@property NSDictionary<NSString*, NSNumber*>* clubIDs;
@property NSDictionary<NSNumber*, NSString*>* clubNames;
@property NSDictionary* latest;
@property NSTask* worker;
@property NSFileHandle* commands;
@property NSString* privateRoot;
@property NSString* program;
@property NSString* game;
@property BOOL busy;
@property std::uint32_t nextRequest;
@property std::uint32_t pendingRequest;
@end

@implementation MatchDebuggerApp
- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    (void)notification;
    self.players = @[];
    self.events = [NSMutableArray new];
    [self buildWindow];
    try {
        const std::filesystem::path bundle(NSBundle.mainBundle.bundlePath.fileSystemRepresentation);
        const auto program = bundle / "Contents/Resources/OriginalProgram";
        NSString* support = [NSFileManager.defaultManager URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask].firstObject.path;
        self.game = [support stringByAppendingPathComponent:@"CM 01-02 Native/Game"];
        self.program = utf8(program.string());
        if (!std::filesystem::is_regular_file(program / "program.json")) throw std::runtime_error("The debugger bundle is missing its original program data.");
        auto catalog = cm::aot::match_debugger::DatabaseCatalog::load(self.game.fileSystemRepresentation);
        NSMutableDictionary* IDs = [NSMutableDictionary new];
        NSMutableDictionary* names = [NSMutableDictionary new];
        for (const auto& club : catalog.clubs()) {
            if (club.kind != cm::aot::match_debugger::ClubKind::club) continue;
            NSString* item = [NSString stringWithFormat:@"%@  [%u]", utf8(club.name), club.id];
            IDs[item] = @(club.id);
            names[@(club.id)] = utf8(club.name);
        }
        self.clubIDs = IDs;
        self.clubNames = names;
        NSArray* ordered = [IDs.allKeys sortedArrayUsingSelector:@selector(localizedCaseInsensitiveCompare:)];
        [self.home addItemsWithObjectValues:ordered];
        [self.away addItemsWithObjectValues:ordered];
        self.home.stringValue = @"Arsenal  [677]";
        self.away.stringValue = @"Manchester United  [5677]";
        self.status.stringValue = @"Choose two clubs and create an isolated match.";
        self.createButton.enabled = YES;
    } catch (const std::exception& error) { [self showError:utf8(error.what())]; }
    [self.window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}
- (void)buildWindow {
    self.window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 1040, 740)
        styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
        backing:NSBackingStoreBuffered defer:NO];
    self.window.title = @"CM Match Debugger";
    self.window.minSize = NSMakeSize(840, 630);
    [self.window center];
    NSTextField* heading = label(@"Match experiment", 24);
    heading.font = [NSFont boldSystemFontOfSize:24];
    NSTextField* explanation = label(@"Select original clubs, execute the match engine and inspect its live state.");
    explanation.textColor = NSColor.secondaryLabelColor;
    self.home = [NSComboBox new]; self.away = [NSComboBox new];
    for (NSComboBox* combo in @[self.home, self.away]) {
        combo.completes = YES;
        combo.numberOfVisibleItems = 14;
        [combo.widthAnchor constraintGreaterThanOrEqualToConstant:250].active = YES;
    }
    self.home.accessibilityLabel = @"Home club";
    self.away.accessibilityLabel = @"Away club";
    self.seed = [NSTextField textFieldWithString:@"12345"];
    self.seed.accessibilityLabel = @"Random seed";
    [self.seed.widthAnchor constraintEqualToConstant:100].active = YES;
    self.createButton = button(@"Create match", self, @selector(createMatch:));
    self.createButton.enabled = NO;
    NSStackView* selection = stack(@[
        stack(@[label(@"Home club"), self.home], NSUserInterfaceLayoutOrientationVertical),
        stack(@[label(@"Away club"), self.away], NSUserInterfaceLayoutOrientationVertical),
        stack(@[label(@"Random seed"), self.seed], NSUserInterfaceLayoutOrientationVertical),
        self.createButton], NSUserInterfaceLayoutOrientationHorizontal);
    self.score = label(@"No match", 28);
    self.score.font = [NSFont monospacedDigitSystemFontOfSize:28 weight:NSFontWeightSemibold];
    self.stepButton = button(@"Step", self, @selector(stepMatch:));
    self.runButton = button(@"Run", self, @selector(runMatch:));
    self.runButton.toolTip = @"Execute up to 30 engine calls per second; stop at an engine pause or completion boundary.";
    self.pauseButton = button(@"Pause", self, @selector(pauseMatch:));
    self.resetButton = button(@"Reset", self, @selector(resetMatch:));
    self.withdrawButton = button(@"Withdraw (injury)", self, @selector(withdrawPlayer:));
    self.withdrawButton.toolTip = @"Select an on-pitch player while paused after kickoff. Apply the engine's injury withdrawal and tactical updates.";
    NSStackView* controls = stack(@[self.stepButton, self.runButton, self.pauseButton, self.resetButton, self.withdrawButton], NSUserInterfaceLayoutOrientationHorizontal);
    self.inspection = label(@"Engine units: —   •   Steps: —   •   Phase: —");
    self.inspection.font = [NSFont monospacedSystemFontOfSize:12 weight:NSFontWeightRegular];
    self.inspection.maximumNumberOfLines = 3;
    self.status = label(@"Loading original club catalog…");
    self.status.maximumNumberOfLines = 3;
    self.progress = [NSProgressIndicator new];
    self.progress.style = NSProgressIndicatorStyleSpinning;
    self.progress.displayedWhenStopped = NO;
    [self.progress.widthAnchor constraintEqualToConstant:18].active = YES;
    NSStackView* status = stack(@[self.progress, self.status], NSUserInterfaceLayoutOrientationHorizontal);

    self.playerTable = [NSTableView new];
    self.playerTable.accessibilityLabel = @"Actual match players";
    NSScrollView* playerScroll = table_scroll(self.playerTable, @[
        @[@"team", @"Side", @65], @[@"actor", @"Actor", @55], @[@"position", @"Slot", @55],
        @[@"active", @"State", @90], @[@"staffID", @"Staff ID", @75], @[@"name", @"Player", @450]], self);
    self.eventTable = [NSTableView new];
    self.eventTable.accessibilityLabel = @"Engine changes";
    NSScrollView* eventScroll = table_scroll(self.eventTable, @[@[@"step", @"Step", @65], @[@"description", @"Observed engine changes and controls", @780]], self);
    NSTabView* tabs = [NSTabView new];
    for (NSArray* description in @[@[@"players", @"Players", playerScroll], @[@"events", @"Engine changes", eventScroll]]) {
        NSTabViewItem* item = [[NSTabViewItem alloc] initWithIdentifier:description[0]];
        item.label = description[1]; item.view = description[2]; [tabs addTabViewItem:item];
    }
    [tabs.heightAnchor constraintGreaterThanOrEqualToConstant:240].active = YES;
    [tabs setContentHuggingPriority:NSLayoutPriorityDefaultLow forOrientation:NSLayoutConstraintOrientationVertical];
    NSTextField* footer = label(@"Shared advancement: C++23. Original match setup and football dependencies remain. Clock shows raw engine units.", 11);
    footer.textColor = NSColor.secondaryLabelColor;
    footer.maximumNumberOfLines = 2;
    NSStackView* content = stack(@[heading, explanation, selection, self.score, controls, self.inspection, status, tabs, footer], NSUserInterfaceLayoutOrientationVertical);
    content.translatesAutoresizingMaskIntoConstraints = NO;
    [self.window.contentView addSubview:content];
    NSView* view = self.window.contentView;
    [NSLayoutConstraint activateConstraints:@[
        [content.leadingAnchor constraintEqualToAnchor:view.leadingAnchor constant:24],
        [content.trailingAnchor constraintEqualToAnchor:view.trailingAnchor constant:-24],
        [content.topAnchor constraintEqualToAnchor:view.topAnchor constant:24],
        [content.bottomAnchor constraintEqualToAnchor:view.bottomAnchor constant:-20],
        [tabs.widthAnchor constraintEqualToAnchor:content.widthAnchor],
        [self.score.widthAnchor constraintEqualToAnchor:content.widthAnchor],
        [self.inspection.widthAnchor constraintEqualToAnchor:content.widthAnchor],
        [self.status.widthAnchor constraintLessThanOrEqualToAnchor:content.widthAnchor constant:-30],
        [footer.widthAnchor constraintEqualToAnchor:content.widthAnchor]]];
    NSMenu* menu = [NSMenu new];
    NSMenuItem* appMenu = [NSMenuItem new];
    appMenu.submenu = [NSMenu new];
    [appMenu.submenu addItemWithTitle:@"Quit CM Match Debugger" action:@selector(terminate:) keyEquivalent:@"q"];
    [menu addItem:appMenu];
    NSMenuItem* edit = [NSMenuItem new];
    edit.title = @"Edit"; edit.submenu = [[NSMenu alloc] initWithTitle:@"Edit"];
    [edit.submenu addItemWithTitle:@"Copy" action:@selector(copy:) keyEquivalent:@"c"];
    [edit.submenu addItemWithTitle:@"Paste" action:@selector(paste:) keyEquivalent:@"v"];
    [edit.submenu addItemWithTitle:@"Select All" action:@selector(selectAll:) keyEquivalent:@"a"];
    [menu addItem:edit]; NSApp.mainMenu = menu;
    [self updateControls];
}
- (void)updateControls {
    NSString* state = self.latest[@"status"];
    BOOL paused = [state isEqual:@"paused"], running = [state isEqual:@"running"];
    self.stepButton.enabled = !self.busy && paused;
    self.runButton.enabled = !self.busy && paused;
    self.pauseButton.enabled = !self.busy && running;
    self.resetButton.enabled = !self.busy && state && ![state isEqual:@"empty"];
    const auto selected = self.playerTable.selectedRow;
    self.withdrawButton.enabled = !self.busy && paused && [self.latest[@"stepCount"] unsignedLongLongValue] > 0 &&
        selected >= 0 && NSUInteger(selected) < self.players.count && [self.players[selected][@"active"] boolValue];
    self.createButton.enabled = !self.busy && self.clubIDs != nil;
    self.home.enabled = self.away.enabled = self.seed.enabled = !self.busy;
    if (self.busy) [self.progress startAnimation:nil]; else [self.progress stopAnimation:nil];
}
- (void)showError:(NSString*)message {
    self.busy = NO;
    self.status.stringValue = message;
    self.status.textColor = NSColor.systemRedColor;
    [self updateControls];
}
- (BOOL)startWorker {
    if (self.worker.running) return YES;
    [self cleanupPrivateRoot];
    self.latest = nil;
    NSString* temporary = [NSTemporaryDirectory() stringByAppendingPathComponent:@"CMMatchDebugger.XXXXXX"];
    std::string pattern(temporary.fileSystemRepresentation);
    if (!mkdtemp(pattern.data())) { [self showError:@"Cannot create the debugger working directory."]; return NO; }
    self.privateRoot = utf8(pattern);
    NSTask* task = [NSTask new];
    task.executableURL = [NSURL fileURLWithPath:NSBundle.mainBundle.executablePath];
    task.arguments = @[@"--engine-worker", self.program, self.game, self.privateRoot];
    NSPipe* input = [NSPipe pipe]; NSPipe* output = [NSPipe pipe];
    task.standardInput = input; task.standardOutput = output;
    // Keep diagnostics out of the JSON channel. This log is caller-owned and
    // deleted with the temporary root when this app exits.
    NSString* log = [self.privateRoot stringByAppendingPathComponent:@"worker.log"];
    // Worker requires the root to start empty; open/unlink the log so the file
    // handle owns it without interfering with experiment root validation.
    [NSFileManager.defaultManager createFileAtPath:log contents:nil attributes:nil];
    NSFileHandle* diagnostics = [NSFileHandle fileHandleForWritingAtPath:log];
    [NSFileManager.defaultManager removeItemAtPath:log error:nil];
    task.standardError = diagnostics;
    __weak MatchDebuggerApp* weak = self;
    task.terminationHandler = ^(NSTask* ended) {
        dispatch_async(dispatch_get_main_queue(), ^{
            MatchDebuggerApp* app = weak;
            if (!app || app.worker != ended) return;
            app.commands = nil;
            app.worker = nil;
            app.pendingRequest = 0;
            [app cleanupPrivateRoot];
            app.latest = nil;
            [app showError:ended.terminationStatus ? @"The engine worker stopped. Create a new match to retry." : @"Engine worker closed."];
        });
    };
    NSError* error = nil;
    if (![task launchAndReturnError:&error]) { [self showError:error.localizedDescription]; return NO; }
    self.worker = task;
    self.commands = input.fileHandleForWriting;
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        NSMutableData* pending = [NSMutableData new];
        NSFileHandle* responses = output.fileHandleForReading;
        while (true) {
            @autoreleasepool {
                NSData* bytes = [responses availableData];
                if (!bytes.length) break;
                [pending appendData:bytes];
                if (pending.length > 1024 * 1024) break;
                while (true) {
                    const char* start = static_cast<const char*>(pending.bytes);
                    const char* newline = static_cast<const char*>(memchr(start, '\n', pending.length));
                    if (!newline) break;
                    const auto length = NSUInteger(newline - start);
                    NSData* line = [pending subdataWithRange:NSMakeRange(0, length)];
                    [pending replaceBytesInRange:NSMakeRange(0, length + 1) withBytes:nullptr length:0];
                    id message = [NSJSONSerialization JSONObjectWithData:line options:0 error:nil];
                    dispatch_async(dispatch_get_main_queue(), ^{
                        MatchDebuggerApp* app = weak;
                        if (app.worker != task) return;
                        if ([message isKindOfClass:NSDictionary.class]) [app receive:message];
                        else [app showError:@"Invalid response from the engine worker."];
                    });
                }
            }
        }
    });
    return YES;
}
- (void)send:(NSDictionary*)command {
    self.pendingRequest = ++self.nextRequest;
    NSMutableDictionary* requested = [command mutableCopy];
    requested[@"requestID"] = @(self.pendingRequest);
    NSData* bytes = [NSJSONSerialization dataWithJSONObject:requested options:0 error:nil];
    NSMutableData* line = [bytes mutableCopy];
    [line appendBytes:"\n" length:1];
    NSError* error = nil;
    if (![self.commands writeData:line error:&error]) [self showError:@"Cannot send a command to the engine worker."];
}
- (void)createMatch:(id)sender {
    (void)sender;
    NSNumber* home = self.clubIDs[self.home.stringValue];
    NSNumber* away = self.clubIDs[self.away.stringValue];
    NSScanner* scanner = [NSScanner scannerWithString:self.seed.stringValue];
    unsigned long long seed = 0;
    if (!home || !away || [home isEqual:away] || ![scanner scanUnsignedLongLong:&seed] || !scanner.isAtEnd || !seed || seed > UINT32_MAX) {
        [self showError:@"Choose two different clubs from the list and a seed between 1 and 4294967295."]; return;
    }
    if (![self startWorker]) return;
    self.busy = YES; self.status.textColor = NSColor.labelColor;
    self.status.stringValue = @"Creating an independent match…";
    [self updateControls];
    [self send:@{@"command": @"create", @"homeID": home, @"awayID": away, @"seed": @(seed)}];
}
- (void)stepMatch:(id)sender { (void)sender; self.status.textColor = NSColor.labelColor; self.busy = YES; [self updateControls]; [self send:@{@"command": @"step"}]; }
- (void)runMatch:(id)sender { (void)sender; self.status.textColor = NSColor.labelColor; self.busy = YES; [self updateControls]; [self send:@{@"command": @"run"}]; }
- (void)pauseMatch:(id)sender { (void)sender; self.status.textColor = NSColor.labelColor; self.busy = YES; [self updateControls]; [self send:@{@"command": @"pause"}]; }
- (void)resetMatch:(id)sender {
    (void)sender; self.busy = YES; self.status.textColor = NSColor.labelColor;
    self.status.stringValue = @"Recreating the original selected setup…"; [self updateControls]; [self send:@{@"command": @"reset"}];
}
- (void)withdrawPlayer:(id)sender {
    (void)sender;
    const auto row = self.playerTable.selectedRow;
    if (row < 0 || NSUInteger(row) >= self.players.count) return;
    NSDictionary* player = self.players[row];
    self.status.textColor = NSColor.labelColor;
    self.busy = YES; [self updateControls];
    [self send:@{@"command": @"withdraw", @"team": player[@"team"], @"actor": player[@"actor"]}];
}
- (void)receive:(NSDictionary*)message {
    NSString* type = message[@"type"];
    if ([type isEqual:@"progress"]) { self.status.stringValue = message[@"message"]; return; }
    const auto request = [message[@"requestID"] unsignedIntValue];
    if (self.pendingRequest && request != self.pendingRequest) {
        // A running update can already be in the pipe when Pause is sent.
        // Retain its ordered observations while awaiting the control reply;
        // it must not re-enable controls or finish a newer creation request.
        if ([type isEqual:@"state"] && [self.latest[@"generation"] isEqual:message[@"generation"]])
            [self appendEvents:message[@"events"]];
        return;
    }
    if ([type isEqual:@"error"]) { [self showError:message[@"message"]]; return; }
    if (![type isEqual:@"state"]) return;
    self.pendingRequest = 0;
    const BOOL newExperiment = ![self.latest[@"generation"] isEqual:message[@"generation"]];
    self.latest = message; self.busy = NO;
    if (newExperiment) { [self.events removeAllObjects]; [self.playerTable deselectAll:nil]; }
    [self appendEvents:message[@"events"]];
    self.players = message[@"players"] ? message[@"players"] : @[];
    NSString* state = message[@"status"];
    if (![state isEqual:@"empty"]) {
        self.score.stringValue = [NSString stringWithFormat:@"%@   %@ – %@   %@", self.clubNames[message[@"homeID"]], message[@"homeScore"], message[@"awayScore"], self.clubNames[message[@"awayID"]]];
        self.inspection.stringValue = [NSString stringWithFormat:@"Engine units: %@   •   Steps: %@   •   Phase: %@   •   Interruption: %@   •   Transition: %@\nOn pitch: %@ / %@   •   Injury withdrawals: %@   •   Actor work queue: %@\nLast step instructions: %@   •   Shared C++23 calls: %@",
            message[@"engineUnits"], message[@"stepCount"], message[@"phase"], message[@"interruption"], message[@"transition"],
            message[@"homeOnPitch"], message[@"awayOnPitch"], message[@"withdrawalCount"], message[@"actorQueue"], message[@"lastInstructions"], message[@"sharedModernCalls"]];
        // Preserve a command validation error until the next accepted control;
        // a following snapshot is also how a failed creation exposes its old state.
        if (![self.status.textColor isEqual:NSColor.systemRedColor])
            self.status.stringValue = [NSString stringWithFormat:@"Session %@. Seed %@. %@", state, message[@"seed"], [state isEqual:@"paused"] ? @"Step advances one real engine call." : @""];
        if ([state isEqual:@"faulted"]) [self showError:message[@"failure"]];
    }
    [self.playerTable reloadData]; [self.eventTable reloadData];
    if (self.events.count) [self.eventTable scrollRowToVisible:self.events.count - 1];
    [self updateControls];
}
- (void)appendEvents:(NSArray*)events {
    [self.events addObjectsFromArray:events ? events : @[]];
    if (self.events.count > 2000) [self.events removeObjectsInRange:NSMakeRange(0, self.events.count - 2000)];
    [self.eventTable reloadData];
    if (self.events.count) [self.eventTable scrollRowToVisible:self.events.count - 1];
}
- (NSInteger)numberOfRowsInTableView:(NSTableView*)table { return table == self.playerTable ? self.players.count : self.events.count; }
- (void)tableViewSelectionDidChange:(NSNotification*)notification {
    if (notification.object == self.playerTable) [self updateControls];
}
- (NSView*)tableView:(NSTableView*)table viewForTableColumn:(NSTableColumn*)column row:(NSInteger)row {
    NSDictionary* value = table == self.playerTable ? self.players[row] : self.events[row];
    NSString* key = column.identifier;
    id cell = value[key];
    if (table == self.playerTable) {
        if ([key isEqual:@"team"]) cell = [cell boolValue] ? @"Away" : @"Home";
        if ([key isEqual:@"active"]) cell = [cell boolValue] ? @"On pitch" : @"Off pitch";
    }
    NSTextField* field = label([cell description] ? [cell description] : @"");
    field.lineBreakMode = NSLineBreakByTruncatingTail;
    return field;
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)app { (void)app; return YES; }
- (void)applicationWillTerminate:(NSNotification*)notification {
    (void)notification;
    if (self.worker.running) { [self.worker terminate]; [self.worker waitUntilExit]; }
    [self.commands closeFile];
    [self cleanupPrivateRoot];
}
- (void)cleanupPrivateRoot {
    if (self.privateRoot && [self.privateRoot.lastPathComponent hasPrefix:@"CMMatchDebugger."] &&
        [self.privateRoot.stringByDeletingLastPathComponent isEqual:[NSTemporaryDirectory() stringByStandardizingPath]])
        [NSFileManager.defaultManager removeItemAtPath:self.privateRoot error:nil];
    self.privateRoot = nil;
}
@end

int main(int argc, char** argv) {
    signal(SIGPIPE, SIG_IGN);
    if (argc > 1 && std::string_view(argv[1]) == "--engine-worker") return run_match_debugger_worker(argc, argv);
    @autoreleasepool {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        MatchDebuggerApp* app = [MatchDebuggerApp new]; NSApp.delegate = app;
        [NSApp run];
    }
    return 0;
}
