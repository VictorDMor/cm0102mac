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
@property NSTextField* clock;
@property NSTextField* timeChoice;
@property NSButton* backButton;
@property NSButton* forwardTimeButton;
@property NSButton* backTimeButton;
@property NSButton* seekButton;
@property NSTextField* inspection;
@property NSButton* createButton;
@property NSButton* stepButton;
@property NSButton* runButton;
@property NSButton* pauseButton;
@property NSButton* resetButton;
@property NSButton* withdrawButton;
@property NSButton* goalButton;
@property NSPopUpButton* cardChoice;
@property NSButton* addCardButton;
@property NSButton* removeCardButton;
@property NSPopUpButton* rosterChoice;
@property NSPopUpButton* positionChoice;
@property NSButton* playerButton;
@property NSButton* positionButton;
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
        NSMutableDictionary* counts = [NSMutableDictionary new];
        for (const auto& club : catalog.clubs()) if (club.kind == cm::aot::match_debugger::ClubKind::club) {
            NSString* name = utf8(club.name); counts[name] = @([counts[name] unsignedIntegerValue] + 1);
        }
        for (const auto& club : catalog.clubs()) {
            if (club.kind != cm::aot::match_debugger::ClubKind::club) continue;
            NSString* name = utf8(club.name);
            NSString* item = [counts[name] unsignedIntegerValue] > 1 ? [NSString stringWithFormat:@"%@  [%u]", name, club.id] : name;
            IDs[item] = @(club.id);
            names[@(club.id)] = utf8(club.name);
        }
        self.clubIDs = IDs;
        self.clubNames = names;
        NSArray* ordered = [IDs.allKeys sortedArrayUsingSelector:@selector(localizedCaseInsensitiveCompare:)];
        [self.home addItemsWithObjectValues:ordered];
        [self.away addItemsWithObjectValues:ordered];
        self.home.stringValue = @"Arsenal";
        self.away.stringValue = @"Manchester United";
        self.status.stringValue = @"Choose two clubs and create an isolated match.";
        self.createButton.enabled = YES;
    } catch (const std::exception& error) { [self showError:utf8(error.what())]; }
    [self.window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}
- (void)buildWindow {
    self.window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 1040, 780)
        styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
        backing:NSBackingStoreBuffered defer:NO];
    self.window.title = @"CM Match Debugger";
    self.window.minSize = NSMakeSize(900, 740);
    [self.window center];
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
    self.clock = label(@"00:00", 28);
    self.clock.font = [NSFont monospacedDigitSystemFontOfSize:28 weight:NSFontWeightSemibold];
    self.clock.accessibilityLabel = @"Match time";
    self.stepButton = button(@"Next moment", self, @selector(stepMatch:));
    self.backButton = button(@"Back", self, @selector(backMatch:));
    self.backButton.toolTip = @"Return to the previous recorded moment, including its players, score and events.";
    self.runButton = button(@"Play", self, @selector(runMatch:));
    self.runButton.toolTip = @"Play the match. Pause to inject an event or make a substitution.";
    self.pauseButton = button(@"Pause", self, @selector(pauseMatch:));
    self.resetButton = button(@"Restart", self, @selector(resetMatch:));
    self.withdrawButton = button(@"Injury", self, @selector(withdrawPlayer:));
    self.withdrawButton.toolTip = @"Injure the selected player and withdraw them from the pitch.";
    self.goalButton = button(@"Add goal", self, @selector(addGoal:));
    self.goalButton.toolTip = @"Award a goal to the selected player and prepare the restart.";
    self.backTimeButton = button(@"−1 min", self, @selector(backMinute:));
    self.forwardTimeButton = button(@"+1 min", self, @selector(forwardMinute:));
    self.timeChoice = [NSTextField textFieldWithString:@"00:00"];
    self.timeChoice.accessibilityLabel = @"Jump to match time";
    [self.timeChoice.widthAnchor constraintEqualToConstant:72].active = YES;
    self.seekButton = button(@"Go", self, @selector(seekMatch:));
    NSStackView* controls = stack(@[self.backButton, self.stepButton, self.runButton, self.pauseButton,
        self.backTimeButton, self.forwardTimeButton, self.timeChoice, self.seekButton, self.resetButton], NSUserInterfaceLayoutOrientationHorizontal);
    self.rosterChoice = [[NSPopUpButton alloc] initWithFrame:NSZeroRect pullsDown:NO];
    [self.rosterChoice.widthAnchor constraintEqualToConstant:280].active = YES;
    self.rosterChoice.accessibilityLabel = @"Replacement player";
    self.playerButton = button(@"Choose player", self, @selector(setPlayer:));
    self.playerButton.toolTip = @"Replace the selected player. Substitutions are unrestricted in this experiment.";
    self.positionChoice = [[NSPopUpButton alloc] initWithFrame:NSZeroRect pullsDown:NO];
    [self.positionChoice addItemWithTitle:@"Bench"]; self.positionChoice.lastItem.tag = -1;
    for (NSInteger position = 0; position < 11; ++position) {
        [self.positionChoice addItemWithTitle:position ? [NSString stringWithFormat:@"Position %ld", position + 1] : @"Goalkeeper"];
        self.positionChoice.lastItem.tag = position;
    }
    self.positionChoice.accessibilityLabel = @"Starting position";
    self.positionButton = button(@"Set position", self, @selector(setPosition:));
    self.positionButton.toolTip = @"Set a starting position or move a player to the bench before kickoff.";
    NSStackView* lineup = stack(@[label(@"Replacement:"), self.rosterChoice, self.playerButton, self.positionChoice,
        self.positionButton, self.withdrawButton, self.goalButton], NSUserInterfaceLayoutOrientationHorizontal);
    self.cardChoice = [[NSPopUpButton alloc] initWithFrame:NSZeroRect pullsDown:NO];
    [self.cardChoice addItemsWithTitles:@[@"Yellow card", @"Red card"]];
    self.cardChoice.accessibilityLabel = @"Card color";
    self.cardChoice.target = self; self.cardChoice.action = @selector(cardChoiceChanged:);
    self.addCardButton = button(@"Add card", self, @selector(addCard:));
    self.removeCardButton = button(@"Remove card", self, @selector(removeCard:));
    self.addCardButton.toolTip = @"Give the selected player a card. A second yellow sends the player off.";
    self.removeCardButton.toolTip = @"Cancel a card. Canceling a dismissal returns the player to the pitch.";
    NSStackView* discipline = stack(@[label(@"Cards:"), self.cardChoice, self.addCardButton, self.removeCardButton], NSUserInterfaceLayoutOrientationHorizontal);
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
        @[@"team", @"Club", @200], @[@"name", @"Player", @310], @[@"active", @"Status", @120], @[@"yellowCards", @"Yellow", @65], @[@"redCards", @"Red", @55], @[@"goals", @"Goals", @55]], self);
    self.eventTable = [NSTableView new];
    self.eventTable.accessibilityLabel = @"Match events";
    NSScrollView* eventScroll = table_scroll(self.eventTable, @[@[@"clock", @"Time", @65], @[@"source", @"Source", @80], @[@"description", @"Match events", @730]], self);
    [playerScroll.heightAnchor constraintGreaterThanOrEqualToConstant:145].active = YES;
    [eventScroll.heightAnchor constraintGreaterThanOrEqualToConstant:145].active = YES;
    NSStackView* matchView = stack(@[label(@"Players and substitutes"), playerScroll, label(@"Match events"), eventScroll], NSUserInterfaceLayoutOrientationVertical);
    [playerScroll.widthAnchor constraintEqualToAnchor:matchView.widthAnchor].active = YES;
    [eventScroll.widthAnchor constraintEqualToAnchor:matchView.widthAnchor].active = YES;
    NSStackView* details = stack(@[label(@"Engine details"), self.inspection], NSUserInterfaceLayoutOrientationVertical);
    NSTabView* tabs = [NSTabView new];
    for (NSArray* description in @[@[@"match", @"Match", matchView], @[@"details", @"Details", details]]) {
        NSTabViewItem* item = [[NSTabViewItem alloc] initWithIdentifier:description[0]];
        item.label = description[1]; item.view = description[2]; [tabs addTabViewItem:item];
    }
    [tabs.heightAnchor constraintGreaterThanOrEqualToConstant:360].active = YES;
    [tabs setContentHuggingPriority:NSLayoutPriorityDefaultLow forOrientation:NSLayoutConstraintOrientationVertical];
    NSTextField* footer = label(@"Pause and select a player to change cards, add a goal, cause an injury or make a substitution. Playing or editing a past moment starts a new timeline.", 11);
    footer.textColor = NSColor.secondaryLabelColor;
    footer.maximumNumberOfLines = 2;
    NSStackView* scoreboard = stack(@[self.score, self.clock], NSUserInterfaceLayoutOrientationHorizontal);
    NSStackView* content = stack(@[selection, scoreboard, controls, lineup, discipline, status, tabs, footer], NSUserInterfaceLayoutOrientationVertical);
    content.translatesAutoresizingMaskIntoConstraints = NO;
    [self.window.contentView addSubview:content];
    NSView* view = self.window.contentView;
    [NSLayoutConstraint activateConstraints:@[
        [content.leadingAnchor constraintEqualToAnchor:view.leadingAnchor constant:24],
        [content.trailingAnchor constraintEqualToAnchor:view.trailingAnchor constant:-24],
        [content.topAnchor constraintEqualToAnchor:view.topAnchor constant:24],
        [content.bottomAnchor constraintEqualToAnchor:view.bottomAnchor constant:-20],
        [tabs.widthAnchor constraintEqualToAnchor:content.widthAnchor],
        [scoreboard.widthAnchor constraintEqualToAnchor:content.widthAnchor],
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
    self.backButton.enabled = !self.busy && !running && [self.latest[@"timelineCursor"] unsignedLongLongValue] > 0;
    self.backTimeButton.enabled = !self.busy && !running && [self.latest[@"matchSeconds"] unsignedIntValue] > 0;
    self.forwardTimeButton.enabled = self.seekButton.enabled = self.timeChoice.enabled = !self.busy && !running && state && ![state isEqual:@"empty"];
    self.runButton.enabled = !self.busy && paused;
    self.pauseButton.enabled = !self.busy && running;
    self.resetButton.enabled = !self.busy && state && ![state isEqual:@"empty"];
    const auto selected = self.playerTable.selectedRow;
    self.withdrawButton.enabled = !self.busy && paused && [self.latest[@"stepCount"] unsignedLongLongValue] > 0 &&
        selected >= 0 && NSUInteger(selected) < self.players.count && [self.players[selected][@"active"] boolValue];
    self.goalButton.enabled = self.withdrawButton.enabled;
    BOOL selectedPlayer = selected >= 0 && NSUInteger(selected) < self.players.count;
    const BOOL yellow = self.cardChoice.indexOfSelectedItem == 0;
    self.cardChoice.enabled = !self.busy && paused;
    self.addCardButton.enabled = self.withdrawButton.enabled && [self.latest[@"phase"] intValue] == 0 &&
        [self.players[selected][@"redCards"] unsignedIntValue] == 0;
    self.removeCardButton.enabled = !self.busy && paused && [self.latest[@"stepCount"] unsignedLongLongValue] > 0 && selectedPlayer &&
        [self.players[selected][yellow ? @"yellowCards" : @"redCards"] unsignedIntValue] > 0;
    BOOL beforeKickoff = !self.busy && paused && [self.latest[@"stepCount"] unsignedLongLongValue] == 0 &&
        [self.latest[@"engineUnits"] intValue] == 0;
    BOOL selection = selected >= 0 && NSUInteger(selected) < self.players.count;
    NSNumber* previousChoice = self.rosterChoice.selectedItem.representedObject;
    [self.rosterChoice removeAllItems];
    if (selection && [self.latest[@"rosters"] count] == 2) {
        NSMutableSet* assigned = [NSMutableSet new];
        for (NSDictionary* player in self.players) if ([player[@"active"] boolValue]) [assigned addObject:player[@"staffID"]];
        const auto team = [self.players[selected][@"team"] unsignedIntegerValue];
        for (NSDictionary* candidate in self.latest[@"rosters"][team]) {
            if ([assigned containsObject:candidate[@"staffID"]]) continue;
            [self.rosterChoice addItemWithTitle:candidate[@"name"]];
            self.rosterChoice.lastItem.representedObject = candidate[@"staffID"];
            if ([candidate[@"staffID"] isEqual:previousChoice]) [self.rosterChoice selectItem:self.rosterChoice.lastItem];
        }
    }
    BOOL canReplace = !self.busy && paused && selection && (beforeKickoff || [self.players[selected][@"active"] boolValue] ||
        (([self.players[selected][@"position"] intValue] == -2 || [self.players[selected][@"position"] intValue] == -1) &&
        [self.players[selected][@"injured"] boolValue]));
    self.rosterChoice.enabled = canReplace && self.rosterChoice.numberOfItems > 0;
    self.playerButton.enabled = self.rosterChoice.enabled;
    self.playerButton.title = beforeKickoff ? @"Choose player" : @"Substitute";
    self.positionChoice.hidden = self.positionButton.hidden = !beforeKickoff;
    self.positionChoice.enabled = beforeKickoff && selection && [self.players[selected][@"present"] boolValue] &&
        [self.players[selected][@"position"] intValue] >= -1;
    self.positionButton.enabled = self.positionChoice.enabled;
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
- (void)stepMatch:(id)sender { (void)sender; self.status.textColor = NSColor.labelColor; self.busy = YES; [self updateControls]; [self send:@{@"command": @"forward"}]; }
- (void)backMatch:(id)sender { (void)sender; self.status.textColor = NSColor.labelColor; self.busy = YES; [self updateControls]; [self send:@{@"command": @"back"}]; }
- (void)seekSeconds:(unsigned)seconds {
    self.status.textColor = NSColor.labelColor; self.busy = YES; [self updateControls];
    [self send:@{@"command": @"seek", @"seconds": @(seconds)}];
}
- (void)backMinute:(id)sender { (void)sender; const auto now = [self.latest[@"matchSeconds"] unsignedIntValue]; [self seekSeconds:now > 60 ? now - 60 : 0]; }
- (void)forwardMinute:(id)sender { (void)sender; [self seekSeconds:[self.latest[@"matchSeconds"] unsignedIntValue] + 60]; }
- (void)seekMatch:(id)sender {
    (void)sender;
    NSArray* parts = [self.timeChoice.stringValue componentsSeparatedByString:@":"];
    if (parts.count != 2) { [self showError:@"Enter a match time as minutes:seconds, for example 45:00."]; return; }
    unsigned long long minutes = 0, seconds = 0;
    NSScanner* first = [NSScanner scannerWithString:parts[0]], *second = [NSScanner scannerWithString:parts[1]];
    if (![first scanUnsignedLongLong:&minutes] || !first.isAtEnd || ![second scanUnsignedLongLong:&seconds] || !second.isAtEnd ||
        minutes > 180 || seconds >= 60 || minutes * 60 + seconds > 180 * 60) { [self showError:@"Choose a time between 00:00 and 180:00."]; return; }
    [self seekSeconds:unsigned(minutes * 60 + seconds)];
}
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
- (void)addGoal:(id)sender {
    (void)sender;
    const auto row = self.playerTable.selectedRow;
    if (row < 0 || NSUInteger(row) >= self.players.count) return;
    NSDictionary* player = self.players[row];
    self.status.textColor = NSColor.labelColor;
    self.busy = YES; [self updateControls];
    [self send:@{@"command": @"goal", @"team": player[@"team"], @"actor": player[@"actor"]}];
}
- (void)cardChoiceChanged:(id)sender { (void)sender; [self updateControls]; }
- (void)changeCard:(BOOL)add {
    const auto row = self.playerTable.selectedRow;
    if (row < 0 || NSUInteger(row) >= self.players.count) return;
    NSDictionary* player = self.players[row];
    self.busy = YES; self.status.textColor = NSColor.labelColor; [self updateControls];
    [self send:@{@"command": @"card", @"team": player[@"team"], @"actor": player[@"actor"],
        @"color": self.cardChoice.indexOfSelectedItem == 0 ? @"yellow" : @"red", @"action": add ? @"add" : @"remove"}];
}
- (void)addCard:(id)sender { (void)sender; [self changeCard:YES]; }
- (void)removeCard:(id)sender { (void)sender; [self changeCard:NO]; }
- (void)setPlayer:(id)sender {
    (void)sender;
    const auto row = self.playerTable.selectedRow;
    NSNumber* staff = self.rosterChoice.selectedItem.representedObject;
    if (row < 0 || NSUInteger(row) >= self.players.count || !staff) return;
    NSDictionary* player = self.players[row]; self.busy = YES; self.status.textColor = NSColor.labelColor;
    [self updateControls];
    if ([self.latest[@"stepCount"] unsignedLongLongValue] > 0)
        [self send:@{@"command": @"substitute", @"team": player[@"team"], @"actor": player[@"actor"], @"staffID": staff}];
    else {
        for (NSDictionary* candidate in self.players) if ([candidate[@"staffID"] isEqual:staff] && [candidate[@"team"] isEqual:player[@"team"]]) {
            [self send:@{@"command": @"position", @"team": player[@"team"], @"actor": candidate[@"actor"], @"position": player[@"position"]}]; return;
        }
        [self send:@{@"command": @"player", @"team": player[@"team"], @"actor": player[@"actor"], @"staffID": staff}];
    }
}
- (void)setPosition:(id)sender {
    (void)sender;
    const auto row = self.playerTable.selectedRow;
    if (row < 0 || NSUInteger(row) >= self.players.count) return;
    NSDictionary* player = self.players[row]; const auto position = self.positionChoice.selectedItem.tag;
    self.busy = YES; self.status.textColor = NSColor.labelColor; [self updateControls];
    [self send:@{@"command": @"position", @"team": player[@"team"], @"actor": player[@"actor"], @"position": @(position)}];
}
- (void)receive:(NSDictionary*)message {
    NSString* type = message[@"type"];
    if ([type isEqual:@"progress"]) { self.status.stringValue = @"Preparing your match…"; return; }
    const auto request = [message[@"requestID"] unsignedIntValue];
    if (self.pendingRequest && request != self.pendingRequest) {
        // A running update can already be in the pipe when Pause is sent.
        // The next accepted snapshot contains the complete current event
        // prefix; this update must not finish a newer control request.
        return;
    }
    if ([type isEqual:@"error"]) { [self showError:message[@"message"]]; return; }
    if (![type isEqual:@"state"]) return;
    self.pendingRequest = 0;
    const auto selectedRow = self.playerTable.selectedRow;
    NSDictionary* selectedPlayer = selectedRow >= 0 && NSUInteger(selectedRow) < self.players.count ? self.players[selectedRow] : nil;
    const BOOL newExperiment = ![self.latest[@"generation"] isEqual:message[@"generation"]];
    self.latest = message; self.busy = NO;
    if (newExperiment) { [self.events removeAllObjects]; [self.playerTable deselectAll:nil]; }
    [self.events setArray:message[@"matchEvents"] ? message[@"matchEvents"] : @[]];
    self.players = message[@"players"] ? [message[@"players"] filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"present == YES"]] : @[];
    NSString* state = message[@"status"];
    if (![state isEqual:@"empty"]) {
        self.score.stringValue = [NSString stringWithFormat:@"%@   %@ – %@   %@", self.clubNames[message[@"homeID"]], message[@"homeScore"], message[@"awayScore"], self.clubNames[message[@"awayID"]]];
        self.clock.stringValue = message[@"clock"] ? message[@"clock"] : @"00:00";
        self.inspection.stringValue = [NSString stringWithFormat:@"Engine units: %@   •   Steps: %@   •   Phase: %@   •   Interruption: %@   •   Transition: %@\nOn pitch: %@ / %@   •   Lineup edits: %@   •   Withdrawals: %@   •   Added goals: %@\nActor queue: %@   •   Last instructions: %@   •   Shared C++23 calls: %@",
            message[@"engineUnits"], message[@"stepCount"], message[@"phase"], message[@"interruption"], message[@"transition"],
            message[@"homeOnPitch"], message[@"awayOnPitch"], message[@"lineupEditCount"], message[@"withdrawalCount"], message[@"injectedGoalCount"], message[@"actorQueue"], message[@"lastInstructions"], message[@"sharedModernCalls"]];
        // Preserve a command validation error until the next accepted control;
        // a following snapshot is also how a failed creation exposes its old state.
        if (![self.status.textColor isEqual:NSColor.systemRedColor])
            self.status.stringValue = [state isEqual:@"paused"] ? @"Paused. Select a player to change the match, or move through time." :
                [state isEqual:@"running"] ? @"Playing…" : [state isEqual:@"completed"] ? @"Full time. Go back to explore another outcome." :
                [state isEqual:@"aborted"] ? @"The match stopped. Go back or restart." : @"Create a match to begin.";
        if ([state isEqual:@"faulted"]) [self showError:message[@"failure"]];
    }
    [self.playerTable reloadData]; [self.eventTable reloadData];
    if (!newExperiment && selectedPlayer) {
        const auto restoredRow = [self.players indexOfObjectPassingTest:^BOOL(NSDictionary* player, NSUInteger index, BOOL* stop) {
            (void)index; (void)stop;
            return [player[@"team"] isEqual:selectedPlayer[@"team"]] && [player[@"actor"] isEqual:selectedPlayer[@"actor"]];
        }];
        if (restoredRow != NSNotFound) [self.playerTable selectRowIndexes:[NSIndexSet indexSetWithIndex:restoredRow] byExtendingSelection:NO];
    }
    if (self.events.count) [self.eventTable scrollRowToVisible:self.events.count - 1];
    [self updateControls];
}
- (NSInteger)numberOfRowsInTableView:(NSTableView*)table { return table == self.playerTable ? self.players.count : self.events.count; }
- (void)tableViewSelectionDidChange:(NSNotification*)notification {
    if (notification.object == self.playerTable) {
        const auto row = self.playerTable.selectedRow;
        if (row >= 0 && NSUInteger(row) < self.players.count) {
            const auto position = [self.players[row][@"position"] intValue];
            [self.positionChoice selectItemWithTag:position >= -1 && position < 11 ? position : -1];
        }
        [self updateControls];
    }
}
- (NSView*)tableView:(NSTableView*)table viewForTableColumn:(NSTableColumn*)column row:(NSInteger)row {
    NSDictionary* value = table == self.playerTable ? self.players[row] : self.events[row];
    NSString* key = column.identifier;
    id cell = value[key];
    if (table == self.playerTable) {
        if (cell == NSNull.null) cell = @"";
        if ([key isEqual:@"team"]) cell = self.clubNames[self.latest[[cell boolValue] ? @"awayID" : @"homeID"]];
        if ([key isEqual:@"active"]) {
            const auto position = [value[@"position"] intValue];
            cell = [cell boolValue] ? (position == 0 ? @"Goalkeeper" : @"On pitch") :
                position == -3 ? @"Sent off" : position == -4 ? @"Substituted" : [value[@"injured"] boolValue] ? @"Injured" : position == -2 ? @"Unavailable" :
                [value[@"present"] boolValue] ? @"Bench" : @"Empty place";
        }
    }
    NSTextField* field = label([cell description] ? [cell description] : @"");
    field.lineBreakMode = NSLineBreakByTruncatingTail;
    if ([cell respondsToSelector:@selector(unsignedIntValue)] && [cell unsignedIntValue] > 0) {
        if ([key isEqual:@"yellowCards"]) field.textColor = NSColor.systemOrangeColor;
        if ([key isEqual:@"redCards"]) field.textColor = NSColor.systemRedColor;
    }
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
