#import <AppKit/AppKit.h>
#include "competition_editor.hpp"
#include "career_rules.hpp"
#include "competition_defaults.hpp"
#include "competition_editor_document.hpp"
#include "competition_editor_text.hpp"
#include <algorithm>
#include <charconv>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using cm::aot::CompetitionEdition;
using cm::aot::CompetitionEditorDocument;
using T = cm::aot::EditorText;
using cm::aot::competition_values::Kind;
using cm::aot::competition_values::Parameter;

namespace {
// Game records hold Windows-1252 text; the database's own name is UTF-8.
NSString* game_text(const std::string& bytes) {
    NSString* text = [[NSString alloc] initWithBytes:bytes.data() length:bytes.size() encoding:NSWindowsCP1252StringEncoding];
    return text ? text : @"?";
}
NSString* utf8(const std::string& text) {
    NSString* string = [NSString stringWithUTF8String:text.c_str()];
    return string ? string : @"?";
}
std::string plain(NSString* text) {
    const char* bytes = text.UTF8String;
    return bytes ? bytes : "";
}
fs::path path_of(NSURL* url) { return fs::path(url.fileSystemRepresentation); }

// The language the window was opened in (the game's language byte).
unsigned language = 0;
NSString* tr(T text) { return utf8(std::string(cm::aot::editor_text(language, text))); }
NSString* tr(T text, std::initializer_list<std::string> values) { return utf8(cm::aot::filled(cm::aot::editor_text(language, text), values)); }
NSString* label_of(const Parameter& p) { return utf8(cm::aot::editor_label(language, p)); }

NSString* edition_title(CompetitionEdition edition) {
    switch (edition) {
    case CompetitionEdition::cm0001_389: return tr(T::edition_database, {"2000/01"});
    case CompetitionEdition::cm0102: return tr(T::edition_database, {"2001/02"});
    case CompetitionEdition::si_3968: return tr(T::edition_database, {"2002/03"});
    }
    return @"?";
}
NSString* effect_note(const Parameter& p) {
    using E = cm::aot::competition_values::Effect;
    switch (p.effect) {
    case E::results_after_edit: return tr(T::effect_results);
    case E::fixtures_after_edit: return tr(T::effect_fixtures);
    case E::season_end: return tr(T::effect_season_end);
    case E::at_once:
    case E::rounds_not_drawn: break;
    }
    return @"";
}
// The tie-break codes of a league table's ordering (league_base_table_order.cpp).
NSArray<NSString*>* tie_break_titles() {
    return @[tr(T::tie_break_end), tr(T::tie_break_goal_difference), tr(T::tie_break_goals_scored), tr(T::tie_break_wins),
             tr(T::tie_break_head_to_head), tr(T::tie_break_away_goals), tr(T::tie_break_keep_order)];
}
bool is_tie_break(const Parameter& p) { return p.id.starts_with("league.tiebreak."); }

struct Shown {
    const Parameter* parameter;
    std::int32_t round;
    NSControl* control;
    NSTextField* label;
};
}

@interface CMFlippedView : NSView
@end
@implementation CMFlippedView
- (BOOL)isFlipped { return YES; }
@end

@interface CMCompetitionEditor : NSObject <NSTableViewDataSource, NSTableViewDelegate, NSSearchFieldDelegate, NSWindowDelegate> {
@public
    cm::aot::CompetitionEditorSetup setup;
    std::unique_ptr<CompetitionEditorDocument> document;
    std::optional<fs::path> file;        // where the database being edited is saved
    std::vector<std::size_t> rows;       // the competitions the list shows (the filter's result)
    std::vector<Shown> shown;            // the controls of the selected competition
    std::vector<CompetitionEdition> editions;  // those with recorded defaults
    std::int32_t round;                  // the cup round the form shows
}
@property(strong) NSWindow* window;
@property(strong) NSTableView* table;
@property(strong) NSSearchField* search;
@property(strong) NSTextField* name;
@property(strong) NSPopUpButton* edition;
@property(strong) NSButton* chosen;
@property(strong) NSScrollView* form;
@property(strong) NSTextField* status;
@property(strong) NSButton* revert;
@end

@implementation CMCompetitionEditor

// MARK: window

- (NSButton*)button:(NSString*)title action:(SEL)action x:(CGFloat*)x y:(CGFloat)y in:(NSView*)parent {
    NSButton* button = [NSButton buttonWithTitle:title target:self action:action];
    [button sizeToFit];
    button.frame = NSMakeRect(*x, y, button.frame.size.width + 8, 28);
    button.autoresizingMask = NSViewMinYMargin;
    [parent addSubview:button];
    *x += button.frame.size.width + 4;
    return button;
}

- (void)build {
    const CGFloat width = 1120, height = 660, list = 300, top = 44, bottom = 76;
    self.window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, width, height)
                                              styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable |
                                                        NSWindowStyleMaskMiniaturizable
                                                backing:NSBackingStoreBuffered defer:NO];
    self.window.title = tr(T::window_title);
    self.window.releasedWhenClosed = NO;
    self.window.delegate = self;
    self.window.minSize = NSMakeSize(1000, 480);
    NSView* content = self.window.contentView;

    // Top row: the database, its base, and what to do with it.
    CGFloat x = 12;
    const CGFloat row = height - top + 8;
    NSTextField* caption = [NSTextField labelWithString:tr(T::database)];
    [caption sizeToFit];
    caption.frame = NSMakeRect(x, row + 4, caption.frame.size.width, 20);
    caption.autoresizingMask = NSViewMinYMargin;
    [content addSubview:caption];
    x += caption.frame.size.width + 6;
    self.name = [NSTextField textFieldWithString:@""];
    self.name.placeholderString = tr(T::name_placeholder);
    self.name.frame = NSMakeRect(x, row + 2, 190, 24);
    self.name.autoresizingMask = NSViewMinYMargin;
    self.name.target = self;
    self.name.action = @selector(nameChanged:);
    [content addSubview:self.name];
    x += 196;
    self.edition = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(x, row, 190, 28) pullsDown:NO];
    self.edition.autoresizingMask = NSViewMinYMargin;
    self.edition.target = self;
    self.edition.action = @selector(editionChanged:);
    [content addSubview:self.edition];
    x += 196;
    [self button:tr(T::new_database) action:@selector(newDatabase:) x:&x y:row in:content];
    [self button:tr(T::open) action:@selector(openDatabase:) x:&x y:row in:content];
    [self button:tr(T::save) action:@selector(saveDatabase:) x:&x y:row in:content];
    [self button:tr(T::save_as) action:@selector(saveDatabaseAs:) x:&x y:row in:content];
    self.chosen = [NSButton checkboxWithTitle:tr(T::use_for_new) target:self action:@selector(chosenChanged:)];
    [self.chosen sizeToFit];
    self.chosen.frame = NSMakeRect(x + 8, row + 4, self.chosen.frame.size.width + 8, 20);
    self.chosen.autoresizingMask = NSViewMinYMargin;
    [content addSubview:self.chosen];

    // Left: the competitions.
    self.search = [[NSSearchField alloc] initWithFrame:NSMakeRect(12, height - top - 30, list, 24)];
    self.search.placeholderString = tr(T::search);
    self.search.delegate = self;
    self.search.autoresizingMask = NSViewMinYMargin;
    [content addSubview:self.search];
    NSScrollView* scroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(12, bottom, list, height - top - 38 - bottom)];
    scroll.hasVerticalScroller = YES;
    scroll.borderType = NSBezelBorder;
    scroll.autoresizingMask = NSViewHeightSizable;
    self.table = [[NSTableView alloc] initWithFrame:scroll.bounds];
    NSTableColumn* column = [[NSTableColumn alloc] initWithIdentifier:@"competition"];
    column.width = list - 24;
    [self.table addTableColumn:column];
    self.table.headerView = nil;
    self.table.dataSource = self;
    self.table.delegate = self;
    self.table.rowHeight = 20;
    scroll.documentView = self.table;
    [content addSubview:scroll];

    // Right: the selected competition's values.
    self.form = [[NSScrollView alloc] initWithFrame:NSMakeRect(list + 24, bottom, width - list - 36, height - top - bottom - 8)];
    self.form.hasVerticalScroller = YES;
    self.form.borderType = NSBezelBorder;
    self.form.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    self.form.drawsBackground = NO;
    [content addSubview:self.form];

    // Bottom: what the user should know.
    self.status = [NSTextField wrappingLabelWithString:@""];
    self.status.frame = NSMakeRect(12, 8, width - 24, bottom - 16);
    self.status.autoresizingMask = NSViewWidthSizable;
    self.status.textColor = NSColor.secondaryLabelColor;
    self.status.font = [NSFont systemFontOfSize:NSFont.smallSystemFontSize];
    [content addSubview:self.status];
    [self.window center];
}

// MARK: documents

- (void)findEditions {
    editions.clear();
    [self.edition removeAllItems];
    for (std::size_t e = 0; e < cm::aot::competition_edition_count; ++e) {
        const auto edition = CompetitionEdition(e);
        std::error_code error;
        if (!fs::exists(cm::aot::CompetitionDefaults::file(setup.defaults_directory, edition), error)) continue;
        editions.push_back(edition);
        [self.edition addItemWithTitle:edition_title(edition)];
    }
    self.edition.enabled = editions.size() > 1;
}

// A new, empty database on an edition's defaults. NO when the defaults cannot be read.
- (BOOL)start:(CompetitionEdition)edition {
    auto defaults = cm::aot::read_competition_defaults(cm::aot::CompetitionDefaults::file(setup.defaults_directory, edition));
    document = std::make_unique<CompetitionEditorDocument>(edition, std::move(defaults));
    file.reset();
    for (std::size_t i = 0; i < editions.size(); ++i)
        if (editions[i] == edition) [self.edition selectItemAtIndex:NSInteger(i)];
    [self refreshAll];
    return YES;
}

- (void)alert:(NSString*)title text:(NSString*)text {
    NSAlert* alert = [NSAlert new];
    alert.messageText = title;
    alert.informativeText = text;
    [alert beginSheetModalForWindow:self.window completionHandler:nil];
}

- (BOOL)read:(const fs::path&)path {
    std::ifstream input(path, std::ios::binary);
    const std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    if (!input && text.empty()) {
        [self alert:tr(T::cannot_read) text:utf8(path.string())];
        return NO;
    }
    auto database = cm::aot::competition_overrides::read_custom_database(text);
    if (!database) {
        // The parser's own message stays in English: it quotes the file.
        [self alert:tr(T::not_custom_database) text:tr(T::line, {std::to_string(database.error().line), database.error().message})];
        return NO;
    }
    if (std::ranges::find(editions, database->base) == editions.end()) {
        [self alert:tr(T::no_base_title) text:tr(T::no_base_text, {plain(edition_title(database->base))})];
        return NO;
    }
    if (!document || document->base() != database->base) {
        if (![self start:database->base]) return NO;
    }
    if (const auto why = document->load(*database); !why.empty()) {
        [self alert:tr(T::not_opened) text:utf8(why)];
        return NO;
    }
    file = path;
    [self refreshAll];
    return YES;
}

- (BOOL)writeTo:(const fs::path&)path {
    document->name = plain(self.name.stringValue);
    if (document->name.empty()) document->name = path.stem().string();
    const auto text = cm::aot::competition_overrides::write_custom_database(document->database());
    std::error_code error;
    fs::create_directories(path.parent_path(), error);
    auto temporary = path;
    temporary += ".writing";
    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output.write(text.data(), std::streamsize(text.size())).flush()) {
            output.close();
            fs::remove(temporary, error);
            [self alert:tr(T::not_saved) text:utf8(path.string())];
            return NO;
        }
    }
    fs::rename(temporary, path, error);
    if (error) {
        fs::remove(temporary, error);
        [self alert:tr(T::not_saved) text:utf8(error.message())];
        return NO;
    }
    // A chosen database saved under another name stays the chosen one, under its new name.
    const auto chosen = cm::aot::selected_custom_database(setup.selection_file);
    const bool follows = chosen && file && *chosen == *file && path != *file;
    file = path;
    document->saved();
    if (follows) [self choose:file];
    // The game holds what it read when the file was chosen: hand the chosen file over again.
    else if (chosen && *chosen == path && setup.selection_changed) setup.selection_changed(file);
    [self refreshAll];
    return YES;
}

// Writes the choice for new careers (none: the original rules) and tells the game. NO when it cannot be kept.
- (BOOL)choose:(const std::optional<fs::path>&)path {
    std::error_code error;
    if (path) {
        fs::create_directories(setup.selection_file.parent_path(), error);
        std::ofstream output(setup.selection_file, std::ios::binary | std::ios::trunc);
        if (!(output << path->string() << '\n').flush()) {
            [self alert:tr(T::choice_not_saved) text:utf8(setup.selection_file.string())];
            return NO;
        }
    } else fs::remove(setup.selection_file, error);
    if (setup.selection_changed) setup.selection_changed(path);
    return YES;
}

- (BOOL)save {
    if (!document) return NO;
    if (file) return [self writeTo:*file];
    return [self saveAs];
}

- (BOOL)saveAs {
    if (!document) return NO;
    std::error_code error;
    fs::create_directories(setup.databases_directory, error);
    NSSavePanel* panel = [NSSavePanel savePanel];
    panel.directoryURL = [NSURL fileURLWithPath:utf8(setup.databases_directory.string())];
    NSString* suggestion = self.name.stringValue.length ? self.name.stringValue : tr(T::default_name);
    panel.nameFieldStringValue = [suggestion stringByAppendingString:@".toml"];
    if ([panel runModal] != NSModalResponseOK) return NO;
    auto path = path_of(panel.URL);
    if (path.extension() != ".toml") path += ".toml";
    return [self writeTo:path];
}

// YES when the edits are saved or the user gives them up.
- (BOOL)settle {
    if (!document || !document->dirty()) return YES;
    NSAlert* alert = [NSAlert new];
    alert.messageText = tr(T::save_changes);
    [alert addButtonWithTitle:tr(T::save)];
    [alert addButtonWithTitle:tr(T::cancel)];
    [alert addButtonWithTitle:tr(T::dont_save)];
    const auto answer = [alert runModal];
    if (answer == NSAlertFirstButtonReturn) return [self save];
    return answer == NSAlertThirdButtonReturn;
}

- (BOOL)windowShouldClose:(NSWindow*)sender {
    (void)sender;
    [self.window makeFirstResponder:nil];
    return [self settle];
}

// MARK: actions

- (void)newDatabase:(id)sender {
    (void)sender;
    [self.window makeFirstResponder:nil];
    if (![self settle] || editions.empty()) return;
    [self start:document ? document->base() : editions.front()];
}
- (void)editionChanged:(id)sender {
    (void)sender;
    [self.window makeFirstResponder:nil];
    const auto index = std::size_t(self.edition.indexOfSelectedItem);
    if (index >= editions.size() || (document && document->base() == editions[index])) return;
    // A database belongs to one base: another base starts another database.
    if (![self settle] || ![self start:editions[index]]) [self refreshAll];
}
- (void)openDatabase:(id)sender {
    (void)sender;
    [self.window makeFirstResponder:nil];
    if (![self settle]) return;
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    std::error_code error;
    if (fs::exists(setup.databases_directory, error)) panel.directoryURL = [NSURL fileURLWithPath:utf8(setup.databases_directory.string())];
    panel.allowsMultipleSelection = NO;
    if ([panel runModal] != NSModalResponseOK) return;
    [self read:path_of(panel.URL)];
}
- (void)saveDatabase:(id)sender { (void)sender; [self.window makeFirstResponder:nil]; [self save]; }
- (void)saveDatabaseAs:(id)sender { (void)sender; [self.window makeFirstResponder:nil]; [self saveAs]; }
- (void)nameChanged:(id)sender {
    (void)sender;
    if (!document) return;
    document->rename(plain(self.name.stringValue));
    [self refreshStatus];
}

- (void)chosenChanged:(id)sender {
    (void)sender;
    const bool on = self.chosen.state == NSControlStateValueOn;
    [self.window makeFirstResponder:nil];
    // Only a saved file can be chosen: the game reads the file. The box then follows the selection file.
    if (on && document && (!(document->dirty() || !file) || [self save])) [self choose:file];
    else if (!on) [self choose:std::nullopt];
    [self refreshAll];
}

- (void)revertCompetition:(id)sender {
    (void)sender;
    [self.window makeFirstResponder:nil];
    const auto selected = [self selected];
    if (!selected) return;
    document->revert(*selected);
    [self refreshForm];
    [self.table reloadData];
    [self reselect:*selected];
}

- (void)roundChanged:(NSPopUpButton*)sender {
    [self.window makeFirstResponder:nil];
    round = std::int32_t(sender.indexOfSelectedItem);
    [self refreshForm];
}

- (void)valueChanged:(NSControl*)sender {
    const auto selected = [self selected];
    const auto index = std::size_t(sender.tag);
    // A field of a form already replaced (its edit ended as it was removed) is not this form's.
    if (!selected || index >= shown.size() || shown[index].control != sender) return;
    const auto& item = shown[index];
    std::int32_t value = 0;
    bool number = true;
    if ([sender isKindOfClass:NSPopUpButton.class]) value = std::int32_t(((NSPopUpButton*)sender).indexOfSelectedItem);
    else {
        const auto text = plain([sender.stringValue stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceCharacterSet]);
        const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
        number = !text.empty() && error == std::errc{} && end == text.data() + text.size();
    }
    NSString* why = nil;
    if (!number) why = tr(T::need_number);
    else if (const auto refusal = document->set(*selected, *item.parameter, item.round, value); !refusal.empty()) {
        using Kind = CompetitionEditorDocument::Refusal::Kind;
        const auto a = std::to_string(refusal.a), b = std::to_string(refusal.b);
        switch (refusal.kind) {
        case Kind::range: why = tr(T::need_range, {a, b}); break;
        case Kind::places: why = tr(T::places_exceed, {a, b}); break;
        case Kind::head_to_head: why = tr(T::head_to_head_refused); break;
        case Kind::no_round: why = tr(T::no_such_round); break;
        case Kind::none:
        case Kind::other: why = utf8(refusal.detail); break;
        }
    }
    if (why) {
        NSBeep();
        [self show:item];
        self.status.stringValue = [NSString stringWithFormat:@"%@: %@", label_of(*item.parameter), why];
        self.status.textColor = NSColor.systemRedColor;
        return;
    }
    [self show:item];
    [self.table reloadDataForRowIndexes:[NSIndexSet indexSetWithIndex:NSUInteger(self.table.selectedRow)]
                          columnIndexes:[NSIndexSet indexSetWithIndex:0]];
    [self refreshStatus];
}

// MARK: showing

- (std::optional<std::size_t>)selected {
    const auto row = self.table.selectedRow;
    if (!document || row < 0 || std::size_t(row) >= rows.size()) return std::nullopt;
    return rows[std::size_t(row)];
}
- (void)reselect:(std::size_t)competition {
    for (std::size_t i = 0; i < rows.size(); ++i)
        if (rows[i] == competition) [self.table selectRowIndexes:[NSIndexSet indexSetWithIndex:i] byExtendingSelection:NO];
}

- (NSString*)title:(std::size_t)competition {
    const auto& e = document->competitions()[competition];
    NSString* name = game_text(e.identity.name);
    if (e.identity.stage >= 0) name = [name stringByAppendingFormat:@" (%@)", tr(T::stage, {std::to_string(e.identity.stage + 1)})];
    NSString* nation = e.identity.nation.empty() ? game_text(e.identity.continent) : game_text(e.identity.nation);
    return [NSString stringWithFormat:@"%@%@  %@%@", document->modified(competition) ? @"● " : @"", nation, name,
                                      e.kind == Kind::cup ? [@"  · " stringByAppendingString:tr(T::cup_tag)] : @""];
}

- (void)filter {
    rows.clear();
    if (!document) return;
    NSString* wanted = self.search.stringValue;
    for (std::size_t i = 0; i < document->competitions().size(); ++i)
        if (!wanted.length || [[self title:i] rangeOfString:wanted options:NSCaseInsensitiveSearch | NSDiacriticInsensitiveSearch].location != NSNotFound)
            rows.push_back(i);
}

- (NSInteger)numberOfRowsInTableView:(NSTableView*)tableView { (void)tableView; return NSInteger(rows.size()); }
- (NSView*)tableView:(NSTableView*)tableView viewForTableColumn:(NSTableColumn*)column row:(NSInteger)row {
    (void)column;
    NSTextField* cell = [tableView makeViewWithIdentifier:@"cell" owner:self];
    if (!cell) {
        cell = [NSTextField labelWithString:@""];
        cell.identifier = @"cell";
        cell.lineBreakMode = NSLineBreakByTruncatingTail;
    }
    if (document && std::size_t(row) < rows.size()) cell.stringValue = [self title:rows[std::size_t(row)]];
    return cell;
}
- (void)tableViewSelectionDidChange:(NSNotification*)notification {
    (void)notification;
    round = 0;
    [self refreshForm];
}
- (void)controlTextDidChange:(NSNotification*)notification {
    if (notification.object != self.search) return;
    const auto selected = [self selected];
    [self filter];
    [self.table reloadData];
    if (selected) [self reselect:*selected];
    [self refreshForm];
}

// A control's value and look: an edited value is bold, and its label says what the original is.
- (void)show:(const Shown&)item {
    const auto selected = [self selected];
    if (!selected) return;
    const auto value = document->value(*selected, *item.parameter, item.round);
    const bool edited = document->edited(*selected, *item.parameter, item.round);
    if ([item.control isKindOfClass:NSPopUpButton.class]) [(NSPopUpButton*)item.control selectItemAtIndex:value];
    else item.control.stringValue = [NSString stringWithFormat:@"%d", value];
    item.control.font = edited ? [NSFont boldSystemFontOfSize:NSFont.systemFontSize] : [NSFont systemFontOfSize:NSFont.systemFontSize];
    NSString* note = effect_note(*item.parameter);
    if (edited) {
        const auto original = document->original(*selected, *item.parameter, item.round);
        NSString* was = is_tie_break(*item.parameter) && original >= 0 && original < 7
                            ? tie_break_titles()[NSUInteger(original)] : [NSString stringWithFormat:@"%d", original];
        note = tr(T::original, {plain(was)});
    }
    item.label.stringValue = note;
    item.label.textColor = edited ? NSColor.systemOrangeColor : NSColor.tertiaryLabelColor;
}

- (CGFloat)heading:(NSString*)text at:(CGFloat)y in:(NSView*)view {
    NSTextField* heading = [NSTextField labelWithString:text];
    heading.font = [NSFont boldSystemFontOfSize:NSFont.systemFontSize];
    heading.frame = NSMakeRect(16, y, 560, 20);
    [view addSubview:heading];
    return y + 28;
}

- (CGFloat)rows:(const std::vector<const Parameter*>&)parameters round:(std::int32_t)r at:(CGFloat)y in:(NSView*)view {
    for (const auto* p : parameters) {
        NSTextField* label = [NSTextField labelWithString:label_of(*p)];
        label.frame = NSMakeRect(16, y + 3, 280, 20);
        label.lineBreakMode = NSLineBreakByTruncatingTail;
        [view addSubview:label];
        if (p->read_only) {
            NSTextField* fixed = [NSTextField labelWithString:[NSString stringWithFormat:@"%d", document->original(*[self selected], *p, r)]];
            fixed.frame = NSMakeRect(304, y + 3, 110, 20);
            fixed.textColor = NSColor.secondaryLabelColor;
            [view addSubview:fixed];
            y += 26;
            continue;
        }
        NSControl* control;
        if (is_tie_break(*p)) {
            NSPopUpButton* choice = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(302, y, 230, 26) pullsDown:NO];
            [choice addItemsWithTitles:tie_break_titles()];
            choice.autoenablesItems = NO;
            [choice itemAtIndex:4].enabled = NO;
            control = choice;
        } else {
            NSTextField* field = [NSTextField textFieldWithString:@""];
            field.frame = NSMakeRect(304, y, 110, 24);
            field.alignment = NSTextAlignmentRight;
            control = field;
        }
        control.tag = NSInteger(shown.size());
        control.target = self;
        control.action = @selector(valueChanged:);
        [view addSubview:control];
        NSTextField* note = [NSTextField labelWithString:@""];
        note.font = [NSFont systemFontOfSize:NSFont.smallSystemFontSize];
        note.frame = NSMakeRect(is_tie_break(*p) ? 540 : 424, y + 4, 300, 18);
        [view addSubview:note];
        shown.push_back({p, r, control, note});
        [self show:shown.back()];
        y += 30;
    }
    return y;
}

- (void)refreshForm {
    shown.clear();
    CMFlippedView* view = [[CMFlippedView alloc] initWithFrame:NSMakeRect(0, 0, 860, 100)];
    const auto selected = [self selected];
    CGFloat y = 14;
    if (!document || document->competitions().empty()) {
        NSTextField* empty = [NSTextField wrappingLabelWithString:tr(T::empty_state)];
        empty.frame = NSMakeRect(16, y, 560, 120);
        [view addSubview:empty];
        y += 130;
    } else if (!selected) {
        y = [self heading:tr(T::choose) at:y in:view];
    } else {
        const auto& e = document->competitions()[*selected];
        NSString* title = game_text(e.identity.name);
        if (e.identity.stage >= 0) title = [title stringByAppendingFormat:@" (%@)", tr(T::stage, {std::to_string(e.identity.stage + 1)})];
        y = [self heading:title at:y in:view];
        y = [self rows:CompetitionEditorDocument::parameters(e.kind, false) round:-1 at:y in:view];
        if (e.kind == Kind::cup && !e.rounds.empty()) {
            y += 10;
            y = [self heading:tr(T::rounds) at:y in:view];
            if (round >= std::int32_t(e.rounds.size())) round = 0;
            NSPopUpButton* choice = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(16, y, 200, 26) pullsDown:NO];
            for (std::size_t i = 0; i < e.rounds.size(); ++i) [choice addItemWithTitle:tr(T::round_of, {std::to_string(i + 1), std::to_string(e.rounds.size())})];
            [choice selectItemAtIndex:round];
            choice.target = self;
            choice.action = @selector(roundChanged:);
            [view addSubview:choice];
            y += 34;
            y = [self rows:CompetitionEditorDocument::parameters(e.kind, true) round:round at:y in:view];
        }
        y += 10;
        NSButton* revert = [NSButton buttonWithTitle:tr(T::revert) target:self action:@selector(revertCompetition:)];
        [revert sizeToFit];
        revert.frame = NSMakeRect(16, y, revert.frame.size.width + 12, 28);
        [view addSubview:revert];
        self.revert = revert;
        y += 40;
    }
    view.frame = NSMakeRect(0, 0, 860, y);
    self.form.documentView = view;
    [self refreshStatus];
}

- (void)refreshStatus {
    self.status.textColor = NSColor.secondaryLabelColor;
    if (const auto selected = [self selected]) self.revert.enabled = document->modified(*selected);
    self.window.documentEdited = document && document->dirty();
    NSMutableArray<NSString*>* lines = [NSMutableArray array];
    if (const auto selected = [self selected])
        for (const auto& warning : document->warnings(*selected)) {
            using Kind = CompetitionEditorDocument::Warning::Kind;
            switch (warning.kind) {
            case Kind::places_unchecked: [lines addObject:tr(T::warn_places)]; break;
            case Kind::more_substitutions_than_bench: [lines addObject:tr(T::warn_substitutions)]; break;
            case Kind::scheduled: [lines addObject:tr(T::warn_scheduled, {std::to_string(warning.n)})]; break;
            }
        }
    if (document) {
        const auto count = document->database().overrides.size();
        NSString* where = file ? utf8(file->filename().string()) : tr(T::not_saved_yet);
        // What the game will use is what the selection file names, whichever database is open here.
        const auto chosen = cm::aot::selected_custom_database(setup.selection_file);
        NSString* use = !chosen ? tr(T::uses_original)
                        : file && *chosen == *file ? tr(T::uses_this) : tr(T::uses_other, {chosen->filename().string()});
        [lines addObject:[NSString stringWithFormat:@"%@ · %@ · %@", tr(T::changed_values, {std::to_string(count)}), where, use]];
    }
    self.status.stringValue = [lines componentsJoinedByString:@"\n"];
}

- (void)refreshAll {
    self.name.stringValue = document ? utf8(document->name) : @"";
    const auto chosen = cm::aot::selected_custom_database(setup.selection_file);
    self.chosen.state = file && chosen && *chosen == *file ? NSControlStateValueOn : NSControlStateValueOff;
    self.chosen.enabled = document != nullptr;
    for (std::size_t i = 0; document && i < editions.size(); ++i)
        if (editions[i] == document->base()) [self.edition selectItemAtIndex:NSInteger(i)];
    [self filter];
    [self.table reloadData];
    round = 0;
    [self refreshForm];
}

- (void)open:(const cm::aot::CompetitionEditorSetup&)given {
    setup = given;
    const unsigned now = setup.language ? setup.language() : 0;
    if (self.window && !self.window.visible && now != language) self.window = nil;
    if (!self.window) {
        language = now;
        [self build];
    }
    if (!self.window.visible) {
        [self findEditions];
        document.reset();
        file.reset();
        const auto chosen = cm::aot::selected_custom_database(setup.selection_file);
        std::error_code error;
        // The chosen database if it can be opened, else a new one on the first recorded edition (2001/02 if there).
        if (!(chosen && fs::exists(*chosen, error) && [self read:*chosen]) && !editions.empty()) {
            const auto preferred = std::ranges::find(editions, CompetitionEdition::cm0102);
            [self start:preferred != editions.end() ? *preferred : editions.front()];
        }
        if (!document) [self refreshAll];
    }
    [self.window makeKeyAndOrderFront:nil];
}
@end

namespace cm::aot {
namespace {
CMCompetitionEditor* editor = nil;
}
void show_competition_editor(const CompetitionEditorSetup& setup) {
    if (!editor) editor = [CMCompetitionEditor new];
    [editor open:setup];
}
bool competition_editor_may_quit() {
    if (!editor || !editor.window.visible) return true;
    [editor.window makeFirstResponder:nil];
    return [editor settle];
}
}
