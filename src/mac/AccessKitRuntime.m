/*
    What VoiceOver needs from Quorum's controls but AccessKit's macOS adapter
    (accesskit_macos 0.27.2, inside accesskit-c 0.23.2) leaves out, added to
    AccessKit's own elements at run time:
    - menus: AXMenuBarItem for a menu bar's own items, AXMenuOpened and
      AXMenuClosed, and shortcuts the native way (AXMenuItemCmdChar,
      AXMenuItemCmdModifiers, AXMenuItemCmdVirtualKey, AXMenuItemCmdGlyph,
      AXMenuItemMarkChar);
    - outlines (trees): every shown item is a row, with its index, level
      and expanded state, the outline's row count, and AXRowExpanded,
      AXRowCollapsed and AXRowCountChanged;
    - lists: each row's index, and the list's row count (all its rows,
      shown or not);
    - a row's place among the rows at its depth (AXARIAPosInSet,
      AXARIASetSize), and a level for list rows drawn as a tree;
    - headings (AXHeading) and a Headings rotor on every element and the view;
    - expanded on any node that opens and closes;
    - two requests AccessKit's adapter has no way to send: showing a
      node's context menu (AXShowMenu, VO-Shift-M) and opening or closing a
      row (AXDisclosing set, VO-\), passed to Java's action queue through
      the sink AccessKitJNI.m sets (QuorumRuntimeSetActionSink).
    The plan is plans/Natives/AccessKitMacSteps/10-Runtime-Patch.md, with
    the requests and the extra install points in 14-Actions-Focus-Terrain.md.

    How it works:
    - Every element AccessKit gives macOS is an instance of a class it
      registers at run time, AccessKitNode (adapters/macos/src/node.rs,
      declare_class!). Once that class exists, this file adds the methods
      it lacks (class_addMethod) and wraps five it has
      (method_setImplementation), keeping AccessKit's originals.
    - Each node's author id is its Quorum node id in decimal
      (MacAccessibility.Identify), and AccessKit gives that out as the
      element's accessibilityIdentifier (node.rs, identifier). That matches
      an element to the facts the Java kits send through
      MacAccessibilityExtras, kept here in a table by node id.
    - Notices Java asks for are posted around AccessKit's own events in
      QuorumRuntimeFinishUpdate.

    This leans on AccessKit's internals (the class name, the identifier,
    which selectors it allows), so it checks what it can and turns a
    feature off, saying so once in the log, rather than guess. It is
    switched off entirely for any accesskit-c other than the one it was
    tested with (QUORUM_ACCESSKIT_VERSION, from CMakeLists.txt).

    Reentrancy: AccessKit's element methods borrow its tree (a Rust RefCell)
    for reading while they run (node.rs, resolve_with_context: tree.borrow()),
    and only an update borrows it for writing (adapter.rs,
    update_if_active: tree.borrow_mut()). Reading borrows can nest, so the
    methods here may call AccessKit's methods, originals included. Nothing
    here runs during an update: the update factory only calls Java, which
    only fills the table, and notices are posted after
    accesskit_macos_subclassing_adapter_update_if_active has returned, as
    AccessKit itself raises its events (event.rs, QueuedEvents::raise).
    Nothing here calls Java, except the action sink, which only queues the
    request (MacAccessibility carries it out in its next update, outside
    AccessKit). The view's own methods (accessibilityChildren,
    accessibilityFocusedUIElement, accessibilityHitTest:) hold a write
    borrow of the view's AccessKit state while AccessKit's original runs,
    so the wrappers below install only before and after the original,
    never inside it.

    Everything runs on the main thread: Java's updates, and VoiceOver's
    questions, which AppKit answers inside the run loop. ARC (-fobjc-arc).
*/

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#import <Carbon/Carbon.h>
#include <jni.h>
#include <objc/runtime.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "accesskit.h"
#include "AccessKitRuntime.h"

// The accesskit-c release in download/accesskit, from CMakeLists.txt.
#ifndef QUORUM_ACCESSKIT_VERSION
#define QUORUM_ACCESSKIT_VERSION "unknown"
#endif

// The accesskit-c release this file was written and tested against. After
// a version bump, retest (the plan's list) before changing it.
#define QUORUM_RUNTIME_TESTED_ACCESSKIT "0.23.2"

// Facts about a node; the same numbers as MacAccessibilityExtras.java.
enum
{
    QKMenuBarItem = 1,
    QKOutline = 2,
    QKOutlineRow = 4,
    QKListRow = 8,
    QKExpandable = 16,
    QKExpanded = 32,
    QKMarked = 64,
    QKList = 128,
    QKHeading = 256,
    QKHasMenu = 512,
    QKCanDisclose = 1024,
    // An outline whose rows VoiceOver must not count ("1 of 2"): a 3D
    // item's cursor (Item3DKit). Its AXRows is empty.
    QKUncounted = 2048
};

// Notices; the same numbers as MacAccessibilityExtras.java.
enum
{
    QKMenuOpened = 1,
    QKMenuClosed = 2,
    QKRowExpanded = 3,
    QKRowCollapsed = 4,
    QKRowCountChanged = 5,
    QKMoved = 6
};

// How far QKFind looks from the view before giving up, and how deep the
// walks up and down a tree go.
#define QK_SEARCH_LIMIT 5000
#define QK_DEPTH_LIMIT 64

// Notices waiting for the next update. More than this in one frame are dropped.
#define QK_MAX_NOTICES 64

/* What the kits sent for one node (MacAccessibilityExtras.setInfo). */
@interface QuorumAccessKitExtra : NSObject
@property (nonatomic) int flags;
@property (nonatomic) NSInteger level;
@property (nonatomic) NSInteger index;
@property (nonatomic) NSInteger count;
@property (nonatomic) NSInteger position;
@property (nonatomic) BOOL hasShortcut;
@property (nonatomic, copy) NSString* commandCharacter;
@property (nonatomic) int commandModifiers;
@property (nonatomic) int commandVirtualKey;
@property (nonatomic) int commandGlyph;
@end

@implementation QuorumAccessKitExtra
@end

typedef struct QKNotice
{
    uint64_t node;
    uint64_t near;
    int notice;
} QKNotice;

typedef id (*QKObjectGetter)(id self, SEL _cmd);
typedef BOOL (*QKSelectorTest)(id self, SEL _cmd, SEL selector);
typedef id (*QKAttributeGetter)(id self, SEL _cmd, NSString* attribute);
typedef id (*QKHitTest)(id self, SEL _cmd, NSPoint point);

// The facts, by node id.
static NSMutableDictionary<NSNumber*, QuorumAccessKitExtra*>* extras = nil;

// Elements seen so far, by node id. Weak: AccessKit owns them.
static NSMapTable<NSNumber*, id>* elements = nil;

// Each open menu's element, kept until AXMenuClosed is posted on it.
static NSMutableDictionary<NSNumber*, id>* openMenus = nil;

// Each outline's rows, worked out once between updates.
static NSMutableDictionary<NSNumber*, NSArray*>* outlineRows = nil;

static QKNotice notices[QK_MAX_NOTICES];
static int noticeCount = 0;

// The key of the node id kept on each element (objc_setAssociatedObject).
static char nodeIdKey;

// AccessKitNode, once patched.
static Class nodeClass = Nil;

// AccessKit's own methods, for the wrappers to call.
static IMP originalAllowed = NULL;
static IMP originalRole = NULL;
static IMP originalRows = NULL;
static IMP originalSelectedRows = NULL;
static IMP originalAttributeValue = NULL;

// Which added methods went in (class_addMethod fails if AccessKit has its own).
static BOOL hasIndex = NO;
static BOOL hasDisclosureLevel = NO;
static BOOL hasDisclosed = NO;
static BOOL hasDisclosedByRow = NO;
static BOOL hasDisclosedRows = NO;
static BOOL hasExpanded = NO;
static BOOL hasRowCount = NO;
static BOOL hasCustomRotors = NO;
static BOOL hasShowMenu = NO;
static BOOL hasSetDisclosed = NO;

// Where VoiceOver's show-menu and open/close requests go (QuorumRuntimeSetActionSink).
static QuorumActionSink actionSink = NULL;
static void* actionSinkData = NULL;

// The window's headings, in order, worked out once between updates, and
// the rotor that lists them (VoiceOver's rotor, "Headings").
static NSArray* headings = nil;
static NSAccessibilityCustomRotor* headingRotor = nil;

/*
    A test switch, read once from the environment, as comma-separated flags
    (QUORUM_VOICEOVER_EXPERIMENT=aria): aria adds 100 to AXARIAPosInSet and
    AXARIASetSize, and disclosed reverses AXDisclosedRows (plan 12). Unset:
    no change.
*/
static int experiment = -1;
enum { QKExperimentNone = 0, QKExperimentAria = 1, QKExperimentDisclosed = 2 };

// True if the flag's name is one of the switch's comma-separated words.
static BOOL HasWord(const char* value, const char* word)
{
    size_t length = strlen(word);
    const char* found = value;
    while ((found = strstr(found, word)) != NULL)
    {
        BOOL starts = found == value || found[-1] == ',';
        BOOL ends = found[length] == '\0' || found[length] == ',';
        if (starts && ends)
            return YES;
        found += length;
    }
    return NO;
}

static int Experiment(void)
{
    if (experiment < 0)
    {
        const char* value = getenv("QUORUM_VOICEOVER_EXPERIMENT");
        experiment = QKExperimentNone;
        if (value != NULL)
        {
            if (HasWord(value, "aria"))
                experiment |= QKExperimentAria;
            if (HasWord(value, "disclosed"))
                experiment |= QKExperimentDisclosed;
            NSLog(@"Quorum VoiceOver additions: test switch %s is on.", value);
        }
    }
    return experiment;
}

// True if the test switch has this flag.
static BOOL HasExperiment(int flag)
{
    return (Experiment() & flag) != 0;
}

// The view's class and its three methods that make AccessKit's elements
// (QuorumRuntimeWatchView; subclass.rs, subclass_of).
static Class watchedViewClass = Nil;
static IMP originalViewChildren = NULL;
static IMP originalViewFocus = NULL;
static IMP originalViewHitTest = NULL;

static void PrepareTables(void)
{
    if (extras != nil)
        return;
    extras = [NSMutableDictionary dictionary];
    elements = [NSMapTable strongToWeakObjectsMapTable];
    openMenus = [NSMutableDictionary dictionary];
    outlineRows = [NSMutableDictionary dictionary];
}

// ---------------------------------------------------------------- Finding nodes

/* A node id from an identifier: decimal digits only, or 0 (no Quorum node uses 0). */
static uint64_t ParseNodeId(NSString* text)
{
    if (text == nil || text.length == 0 || text.length > 20)
        return 0;
    const char* characters = text.UTF8String;
    if (characters == NULL)
        return 0;
    for (const char* c = characters; *c != '\0'; c++)
    {
        if (*c < '0' || *c > '9')
            return 0;
    }
    return (uint64_t)strtoull(characters, NULL, 10);
}

/*
    The Quorum node id of one of AccessKit's elements, or 0. An element
    stands for one node for its whole life (node.rs, PlatformNodeIvars), so
    the id is kept on it after the first time, and the element is
    remembered for QKFind.
*/
static uint64_t NodeIdOf(id element)
{
    if (element == nil || nodeClass == Nil || ![element isKindOfClass:nodeClass])
        return 0;
    NSNumber* known = objc_getAssociatedObject(element, &nodeIdKey);
    if (known != nil)
        return known.unsignedLongLongValue;

    uint64_t node = ParseNodeId([element accessibilityIdentifier]);
    if (node == 0)
        return 0;
    NSNumber* key = @(node);
    objc_setAssociatedObject(element, &nodeIdKey, key, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [elements setObject:element forKey:key];
    return node;
}

static QuorumAccessKitExtra* ExtraFor(uint64_t node)
{
    if (node == 0)
        return nil;
    return extras[@(node)];
}

static QuorumAccessKitExtra* ExtraOf(id element)
{
    return ExtraFor(NodeIdOf(element));
}

/* The element still stands for this node in AccessKit's tree. */
static BOOL StillShows(id element, uint64_t node)
{
    NSString* identifier = [element accessibilityIdentifier];
    return ParseNodeId(identifier) == node;
}

/*
    The element for a node: one seen before, else among the children of
    near (a node whose children include it), else the first one found
    looking breadth first from the view, up to QK_SEARCH_LIMIT elements.
    Asking AccessKit for children makes elements for them, so the search
    is the last resort.
*/
static id Find(NSView* view, uint64_t node, uint64_t near)
{
    NSNumber* key = @(node);
    id known = [elements objectForKey:key];
    if (known != nil && StillShows(known, node))
        return known;

    if (near != 0)
    {
        id parent = [elements objectForKey:@(near)];
        if (parent != nil)
        {
            for (id child in [parent accessibilityChildren])
            {
                if (NodeIdOf(child) == node)
                    return child;
            }
        }
    }

    NSArray* top = [view accessibilityChildren];
    if (top == nil)
        return nil;
    NSMutableArray* queue = [NSMutableArray arrayWithArray:top];
    NSUInteger next = 0;
    while (next < queue.count && next < QK_SEARCH_LIMIT)
    {
        id element = queue[next];
        next = next + 1;
        if (NodeIdOf(element) == node)
            return element;
        NSArray* children = [element accessibilityChildren];
        if (children != nil)
            [queue addObjectsFromArray:children];
    }
    return nil;
}

// ---------------------------------------------------------------- Outlines

/*
    Adds a container's tree items to rows in display order, going into an
    item only when it is expanded (as NSOutlineView lists its rows). The
    children come from AccessKit's own accessibilityChildren, so the rows
    are AccessKit's own elements.
*/
static void AddRows(id container, NSMutableArray* rows, int depth)
{
    if (depth > QK_DEPTH_LIMIT)
        return;
    for (id child in [container accessibilityChildren])
    {
        QuorumAccessKitExtra* extra = ExtraOf(child);
        if (extra == nil || (extra.flags & QKOutlineRow) == 0)
            continue;
        [rows addObject:child];
        if ((extra.flags & QKExpanded) != 0)
            AddRows(child, rows, depth + 1);
    }
}

static NSArray* RowsOf(id outline, uint64_t node)
{
    NSNumber* key = @(node);
    NSArray* rows = outlineRows[key];
    if (rows == nil)
    {
        NSMutableArray* found = [NSMutableArray array];
        AddRows(outline, found, 0);
        rows = [found copy];
        outlineRows[key] = rows;
    }
    return rows;
}

/* The outline a row is in, walking up AccessKit's parents, or nil. */
static id OutlineOf(id row, uint64_t* outlineNode)
{
    id current = row;
    for (int depth = 0; depth <= QK_DEPTH_LIMIT; depth++)
    {
        current = [current accessibilityParent];
        uint64_t node = NodeIdOf(current);
        if (node == 0)
            return nil;
        QuorumAccessKitExtra* extra = ExtraFor(node);
        if (extra != nil && (extra.flags & QKOutline) != 0)
        {
            *outlineNode = node;
            return current;
        }
    }
    return nil;
}

// ---------------------------------------------------------------- Added methods

static NSInteger Index(id self, SEL _cmd)
{
    QuorumAccessKitExtra* extra = ExtraOf(self);
    if (extra == nil)
        return 0;
    if ((extra.flags & QKListRow) != 0)
        return extra.index < 0 ? 0 : extra.index;
    if ((extra.flags & QKOutlineRow) != 0)
    {
        uint64_t outlineNode = 0;
        id outline = OutlineOf(self, &outlineNode);
        if (outline != nil)
        {
            NSUInteger place = [RowsOf(outline, outlineNode) indexOfObjectIdenticalTo:self];
            if (place != NSNotFound)
                return (NSInteger)place;
        }
    }
    return 0;
}

static NSInteger DisclosureLevel(id self, SEL _cmd)
{
    QuorumAccessKitExtra* extra = ExtraOf(self);
    return extra == nil ? 0 : extra.level;
}

static BOOL IsDisclosed(id self, SEL _cmd)
{
    QuorumAccessKitExtra* extra = ExtraOf(self);
    return extra != nil && (extra.flags & QKExpanded) != 0;
}

static id DisclosedByRow(id self, SEL _cmd)
{
    id parent = [self accessibilityParent];
    QuorumAccessKitExtra* extra = ExtraOf(parent);
    if (extra != nil && (extra.flags & QKOutlineRow) != 0)
        return parent;
    return nil;
}

static id DisclosedRows(id self, SEL _cmd)
{
    NSMutableArray* rows = [NSMutableArray array];
    for (id child in [self accessibilityChildren])
    {
        QuorumAccessKitExtra* extra = ExtraOf(child);
        if (extra != nil && (extra.flags & QKOutlineRow) != 0)
            [rows addObject:child];
    }
    if (HasExperiment(QKExperimentDisclosed))
        return [[rows reverseObjectEnumerator] allObjects];
    return rows;
}

static BOOL IsExpanded(id self, SEL _cmd)
{
    QuorumAccessKitExtra* extra = ExtraOf(self);
    return extra != nil && (extra.flags & QKExpandable) != 0 && (extra.flags & QKExpanded) != 0;
}

/*
    An outline's row count is its shown rows. A list's is the count Java
    sent (List:GetSize), which includes rows that have no node because the
    list only makes nodes for rows it shows.
*/
static NSInteger RowCount(id self, SEL _cmd)
{
    uint64_t node = NodeIdOf(self);
    QuorumAccessKitExtra* extra = ExtraFor(node);
    if (extra == nil)
        return 0;
    if ((extra.flags & QKOutline) != 0)
        return (NSInteger)RowsOf(self, node).count;
    if ((extra.flags & QKList) != 0)
        return extra.count < 0 ? 0 : extra.count;
    return 0;
}

/*
    VoiceOver's show menu (VO-Shift-M, AXShowMenu). Allowed only on a node
    with a context menu (QKHasMenu). The request goes to Java's queue; the
    menu opens in Quorum's next update, as Shift+F10 opens it.
*/
static BOOL PerformShowMenu(id self, SEL _cmd)
{
    uint64_t node = NodeIdOf(self);
    QuorumAccessKitExtra* extra = ExtraFor(node);
    if (extra == nil || (extra.flags & QKHasMenu) == 0 || actionSink == NULL)
        return NO;
    actionSink((int)ACCESSKIT_ACTION_SHOW_CONTEXT_MENU, node, actionSinkData);
    return YES;
}

/*
    Opening or closing a row (VO-\ sets AXDisclosing). Allowed only on a
    row Java says can do it (QKCanDisclose). Only a change is sent; the row
    reports its new state (AXRowExpanded or AXRowCollapsed, and its
    expanded state) after Quorum's next update.
*/
static void SetDisclosed(id self, SEL _cmd, BOOL disclosed)
{
    uint64_t node = NodeIdOf(self);
    QuorumAccessKitExtra* extra = ExtraFor(node);
    if (extra == nil || (extra.flags & QKCanDisclose) == 0 || actionSink == NULL)
        return;
    BOOL open = (extra.flags & QKExpanded) != 0;
    if (disclosed == open)
        return;
    actionSink((int)(disclosed ? ACCESSKIT_ACTION_EXPAND : ACCESSKIT_ACTION_COLLAPSE), node, actionSinkData);
}

void QuorumRuntimeSetActionSink(QuorumActionSink sink, void* userdata)
{
    actionSink = sink;
    actionSinkData = userdata;
}

// ---------------------------------------------------------------- Wrapped methods

static id Role(id self, SEL _cmd)
{
    QuorumAccessKitExtra* extra = ExtraOf(self);
    if (extra != nil && (extra.flags & QKMenuBarItem) != 0)
        return NSAccessibilityMenuBarItemRole;
    // AccessKit's role for a heading is "Heading" (node.rs:125), which
    // isn't one of macOS's role names. WebKit's headings are "AXHeading".
    if (extra != nil && (extra.flags & QKHeading) != 0)
        return @"AXHeading";
    return ((QKObjectGetter)originalRole)(self, _cmd);
}

static id Rows(id self, SEL _cmd)
{
    uint64_t node = NodeIdOf(self);
    QuorumAccessKitExtra* extra = ExtraFor(node);
    if (extra != nil && (extra.flags & QKOutline) != 0)
        return RowsOf(self, node);
    // VoiceOver follows a selection only to a row the outline lists, so an
    // "uncounted" outline (the terrain's cursor) lists just its selected
    // row: VoiceOver reads each new one, with nothing else to count.
    if (extra != nil && (extra.flags & QKUncounted) != 0)
    {
        id selected = originalSelectedRows != NULL
            ? ((QKObjectGetter)originalSelectedRows)(self, @selector(accessibilitySelectedRows))
            : nil;
        return selected != nil ? selected : @[];
    }
    return ((QKObjectGetter)originalRows)(self, _cmd);
}

/* An outline's selected rows, nested ones included (AccessKit gives only top-level ones). */
static id SelectedRows(id self, SEL _cmd)
{
    uint64_t node = NodeIdOf(self);
    QuorumAccessKitExtra* extra = ExtraFor(node);
    if (extra == nil || (extra.flags & QKOutline) == 0)
        return ((QKObjectGetter)originalSelectedRows)(self, _cmd);
    NSMutableArray* selected = [NSMutableArray array];
    for (id row in RowsOf(self, node))
    {
        if ([row isAccessibilitySelected])
            [selected addObject:row];
    }
    return selected;
}

static BOOL HasMenuAttributes(QuorumAccessKitExtra* extra)
{
    return extra != nil && (extra.hasShortcut || (extra.flags & QKMarked) != 0);
}

/* A row's place among the rows at its depth, which WebKit gives as AXARIAPosInSet and AXARIASetSize. */
static BOOL HasSetAttributes(QuorumAccessKitExtra* extra)
{
    return extra != nil && extra.position > 0 && (extra.flags & (QKOutlineRow | QKListRow)) != 0;
}

/*
    The menu item attributes, which have no NSAccessibility method, through
    the older informal protocol. Untested: whether AppKit passes these to an
    NSAccessibilityElement subclass at all. It is the first thing to check
    (plan, "Test first").
*/
static id AttributeValue(id self, SEL _cmd, NSString* attribute)
{
    QuorumAccessKitExtra* extra = ExtraOf(self);
    if (HasMenuAttributes(extra) && attribute != nil)
    {
        if ([attribute isEqualToString:(__bridge NSString*)kAXMenuItemCmdCharAttribute])
            return extra.hasShortcut ? extra.commandCharacter : nil;
        if ([attribute isEqualToString:(__bridge NSString*)kAXMenuItemCmdModifiersAttribute])
            return extra.hasShortcut ? @(extra.commandModifiers) : nil;
        if ([attribute isEqualToString:(__bridge NSString*)kAXMenuItemCmdVirtualKeyAttribute])
            return extra.hasShortcut && extra.commandVirtualKey >= 0 ? @(extra.commandVirtualKey) : nil;
        if ([attribute isEqualToString:(__bridge NSString*)kAXMenuItemCmdGlyphAttribute])
            return extra.hasShortcut && extra.commandGlyph >= 0 ? @(extra.commandGlyph) : nil;
        if ([attribute isEqualToString:(__bridge NSString*)kAXMenuItemMarkCharAttribute])
            return (extra.flags & QKMarked) != 0 ? @"✓" : nil;
    }
    if (HasSetAttributes(extra) && attribute != nil)
    {
        NSInteger offset = HasExperiment(QKExperimentAria) ? 100 : 0;
        if ([attribute isEqualToString:@"AXARIAPosInSet"])
            return @(extra.position + offset);
        if ([attribute isEqualToString:@"AXARIASetSize"])
            return @((extra.count < extra.position ? extra.position : extra.count) + offset);
    }
    if (originalAttributeValue == NULL)
        return nil;
    return ((QKAttributeGetter)originalAttributeValue)(self, _cmd, attribute);
}

/*
    AccessKit answers NO for any selector it doesn't list (node.rs,
    is_selector_allowed), and AppKit then ignores the method, so each added
    method is allowed here for the nodes it is for. Anything else goes to
    AccessKit.
*/
static BOOL SelectorAllowed(id self, SEL _cmd, SEL selector)
{
    // Every element offers the Headings rotor (CustomRotors).
    if (selector == @selector(accessibilityCustomRotors) && hasCustomRotors)
        return YES;

    BOOL ours = (selector == @selector(accessibilityIndex) && hasIndex)
        || (selector == @selector(accessibilityDisclosureLevel) && hasDisclosureLevel)
        || (selector == @selector(isAccessibilityDisclosed) && hasDisclosed)
        || (selector == @selector(accessibilityDisclosedByRow) && hasDisclosedByRow)
        || (selector == @selector(accessibilityDisclosedRows) && hasDisclosedRows)
        || (selector == @selector(isAccessibilityExpanded) && hasExpanded)
        || (selector == @selector(accessibilityRowCount) && hasRowCount)
        || (selector == @selector(accessibilityPerformShowMenu) && hasShowMenu)
        || (selector == @selector(setAccessibilityDisclosed:) && hasSetDisclosed);
    if (ours)
    {
        QuorumAccessKitExtra* extra = ExtraOf(self);
        if (extra == nil)
            return NO;
        if (selector == @selector(accessibilityPerformShowMenu))
            return (extra.flags & QKHasMenu) != 0 && actionSink != NULL;
        if (selector == @selector(setAccessibilityDisclosed:))
            return (extra.flags & QKCanDisclose) != 0 && (extra.flags & QKOutlineRow) != 0 && actionSink != NULL;
        if (selector == @selector(accessibilityIndex))
            return (extra.flags & (QKOutlineRow | QKListRow)) != 0;
        if (selector == @selector(isAccessibilityExpanded))
            return (extra.flags & QKExpandable) != 0;
        if (selector == @selector(accessibilityRowCount))
            return (extra.flags & (QKOutline | QKList)) != 0;
        // A list row drawn as a tree has a level too (ListItem:GetAccessibilityLevel).
        if (selector == @selector(accessibilityDisclosureLevel))
            return (extra.flags & (QKOutlineRow | QKListRow)) != 0;
        return (extra.flags & QKOutlineRow) != 0;
    }

    if ((selector == @selector(accessibilityRows) && originalRows != NULL)
        || (selector == @selector(accessibilitySelectedRows) && originalSelectedRows != NULL))
    {
        QuorumAccessKitExtra* extra = ExtraOf(self);
        if (extra != nil && (extra.flags & QKOutline) != 0)
            return YES;
    }
    else if (selector == @selector(accessibilityAttributeValue:) && originalAttributeValue != NULL)
    {
        QuorumAccessKitExtra* extra = ExtraOf(self);
        if (HasMenuAttributes(extra) || HasSetAttributes(extra))
            return YES;
    }
    return ((QKSelectorTest)originalAllowed)(self, _cmd, selector);
}

// ---------------------------------------------------------------- Headings rotor

/*
    Adds the headings under an element, depth first (display order). Text
    areas are not entered: their children are text runs, never headings,
    and there can be thousands.
*/
static void AddHeadings(id element, NSMutableArray* found, int depth)
{
    if (depth > QK_DEPTH_LIMIT)
        return;
    for (id child in [element accessibilityChildren])
    {
        QuorumAccessKitExtra* extra = ExtraOf(child);
        if (extra != nil && (extra.flags & QKHeading) != 0)
            [found addObject:child];
        if ([[child accessibilityRole] isEqualToString:NSAccessibilityTextAreaRole])
            continue;
        AddHeadings(child, found, depth + 1);
    }
}

/*
    VoiceOver's rotor asks this for the next or previous heading from the
    one it is on (NSAccessibilityCustomRotor, macOS 10.13).
*/
@interface QuorumHeadingSearch : NSObject <NSAccessibilityCustomRotorItemSearchDelegate>
@property (nonatomic, weak) id root;
@end

@implementation QuorumHeadingSearch

- (NSAccessibilityCustomRotorItemResult*)rotor:(NSAccessibilityCustomRotor*)rotor
    resultForSearchParameters:(NSAccessibilityCustomRotorSearchParameters*)parameters
{
    id root = self.root;
    if (root == nil)
        return nil;
    if (headings == nil)
    {
        NSMutableArray* found = [NSMutableArray array];
        AddHeadings(root, found, 0);
        headings = [found copy];
    }

    id current = parameters.currentItem.targetElement;
    NSUInteger place = current == nil ? NSNotFound : [headings indexOfObjectIdenticalTo:current];
    BOOL forward = parameters.searchDirection == NSAccessibilityCustomRotorSearchDirectionNext;
    NSInteger step = forward ? 1 : -1;
    NSInteger next;
    if (place == NSNotFound)
        next = forward ? 0 : (NSInteger)headings.count - 1;
    else
        next = (NSInteger)place + step;

    NSString* filter = parameters.filterString;
    for (; next >= 0 && next < (NSInteger)headings.count; next += step)
    {
        id heading = headings[(NSUInteger)next];
        if (filter.length > 0)
        {
            NSString* title = [heading accessibilityTitle];
            if (title == nil || [title rangeOfString:filter options:NSCaseInsensitiveSearch].location == NSNotFound)
                continue;
        }
        return [[NSAccessibilityCustomRotorItemResult alloc] initWithTargetElement:heading];
    }
    return nil;
}

@end

// The rotor's delegate; the rotor holds it weakly.
static QuorumHeadingSearch* headingSearch = nil;

/* The top of AccessKit's elements above this one: the window's root node. */
static id RootOf(id element)
{
    id current = element;
    for (int depth = 0; depth <= QK_DEPTH_LIMIT; depth++)
    {
        id parent = [current accessibilityParent];
        if (parent == nil || nodeClass == Nil || ![parent isKindOfClass:nodeClass])
            return current;
        current = parent;
    }
    return current;
}

/* The one Headings rotor, searching from root. */
static NSArray* HeadingRotors(id root)
{
    if (root == nil)
        return @[];
    if (headingRotor == nil)
    {
        headingSearch = [[QuorumHeadingSearch alloc] init];
        headingRotor = [[NSAccessibilityCustomRotor alloc]
            initWithRotorType:NSAccessibilityCustomRotorTypeHeading itemSearchDelegate:headingSearch];
    }
    headingSearch.root = root;
    return @[headingRotor];
}

/*
    Every element offers the Headings rotor, searching the whole window
    from its root node. Apple doesn't document where VoiceOver collects
    custom rotors; offering it only on the root node (step 11) gave an
    empty rotor, so it is offered wherever VoiceOver may ask: on each
    element, and on the view (ViewRotors).
*/
static id CustomRotors(id self, SEL _cmd)
{
    return HeadingRotors(RootOf(self));
}

/* The view's custom rotors: the Headings rotor, from its first AccessKit element. */
static id ViewRotors(id self, SEL _cmd)
{
    if (nodeClass == Nil)
        return @[];
    for (id child in [self accessibilityChildren])
    {
        if ([child isKindOfClass:nodeClass])
            return HeadingRotors(child);
    }
    return @[];
}

// ---------------------------------------------------------------- Installing

/* The class's own method for a selector (not an inherited one), or NULL. */
static Method OwnMethod(Class owner, SEL selector)
{
    unsigned int count = 0;
    Method* methods = class_copyMethodList(owner, &count);
    Method found = NULL;
    for (unsigned int i = 0; i < count; i++)
    {
        if (method_getName(methods[i]) == selector)
        {
            found = methods[i];
            break;
        }
    }
    free(methods);
    return found;
}

/* The method returns an object (@) or a BOOL (B on arm64, c on x86_64), and takes this many arguments, self and _cmd included. */
static BOOL HasShape(Method method, BOOL returnsBool, unsigned int arguments)
{
    if (method_getNumberOfArguments(method) != arguments)
        return NO;
    char returnType[8];
    method_getReturnType(method, returnType, sizeof(returnType));
    if (returnsBool)
        return returnType[0] == 'B' || returnType[0] == 'c';
    return returnType[0] == '@';
}

/* Puts replacement in place of AccessKit's method; returns AccessKit's, or NULL (the feature is then off). */
static IMP Wrap(Class owner, SEL selector, IMP replacement, BOOL returnsBool, unsigned int arguments)
{
    Method method = OwnMethod(owner, selector);
    if (method == NULL || !HasShape(method, returnsBool, arguments))
    {
        NSLog(@"Quorum VoiceOver additions: AccessKitNode has no %@ as expected; what depends on it is off.",
            NSStringFromSelector(selector));
        return NULL;
    }
    return method_setImplementation(method, replacement);
}

static BOOL Add(Class owner, SEL selector, IMP implementation, const char* types)
{
    if (class_addMethod(owner, selector, implementation, types))
        return YES;
    NSLog(@"Quorum VoiceOver additions: AccessKitNode has its own %@ now; AccessKit's is used.",
        NSStringFromSelector(selector));
    return NO;
}

void QuorumRuntimeInstall(void)
{
    static BOOL done = NO;
    if (done || ![NSThread isMainThread])
        return;

    // AccessKit registers the class the first time it makes an element,
    // when VoiceOver first looks at the window.
    Class found = objc_getClass("AccessKitNode");
    if (found == Nil)
        return;
    done = YES;

    if (strcmp(QUORUM_ACCESSKIT_VERSION, QUORUM_RUNTIME_TESTED_ACCESSKIT) != 0)
    {
        NSLog(@"Quorum VoiceOver additions are off: they were tested with accesskit-c %s, and this is %s.",
            QUORUM_RUNTIME_TESTED_ACCESSKIT, QUORUM_ACCESSKIT_VERSION);
        return;
    }
    if (![found isSubclassOfClass:[NSAccessibilityElement class]]
        || OwnMethod(found, @selector(accessibilityIdentifier)) == NULL)
    {
        NSLog(@"Quorum VoiceOver additions are off: AccessKitNode isn't what they expect.");
        return;
    }

    PrepareTables();
    nodeClass = found;

    // Without this, AppKit would never use the added methods.
    originalAllowed = Wrap(found, @selector(isAccessibilitySelectorAllowed:), (IMP)SelectorAllowed, YES, 3);
    if (originalAllowed == NULL)
    {
        nodeClass = Nil;
        return;
    }
    originalRole = Wrap(found, @selector(accessibilityRole), (IMP)Role, NO, 2);
    originalRows = Wrap(found, @selector(accessibilityRows), (IMP)Rows, NO, 2);
    originalSelectedRows = Wrap(found, @selector(accessibilitySelectedRows), (IMP)SelectedRows, NO, 2);
    originalAttributeValue = Wrap(found, @selector(accessibilityAttributeValue:), (IMP)AttributeValue, NO, 3);

    char integerGetter[16];
    char boolGetter[16];
    char objectGetter[16];
    snprintf(integerGetter, sizeof(integerGetter), "%s@:", @encode(NSInteger));
    snprintf(boolGetter, sizeof(boolGetter), "%s@:", @encode(BOOL));
    snprintf(objectGetter, sizeof(objectGetter), "%s@:", @encode(id));
    hasIndex = Add(found, @selector(accessibilityIndex), (IMP)Index, integerGetter);
    hasDisclosureLevel = Add(found, @selector(accessibilityDisclosureLevel), (IMP)DisclosureLevel, integerGetter);
    hasDisclosed = Add(found, @selector(isAccessibilityDisclosed), (IMP)IsDisclosed, boolGetter);
    hasDisclosedByRow = Add(found, @selector(accessibilityDisclosedByRow), (IMP)DisclosedByRow, objectGetter);
    hasDisclosedRows = Add(found, @selector(accessibilityDisclosedRows), (IMP)DisclosedRows, objectGetter);
    hasExpanded = Add(found, @selector(isAccessibilityExpanded), (IMP)IsExpanded, boolGetter);
    hasRowCount = Add(found, @selector(accessibilityRowCount), (IMP)RowCount, integerGetter);
    hasCustomRotors = Add(found, @selector(accessibilityCustomRotors), (IMP)CustomRotors, objectGetter);

    char boolAction[16];
    char boolSetter[16];
    snprintf(boolAction, sizeof(boolAction), "%s@:", @encode(BOOL));
    snprintf(boolSetter, sizeof(boolSetter), "%s@:%s", @encode(void), @encode(BOOL));
    hasShowMenu = Add(found, @selector(accessibilityPerformShowMenu), (IMP)PerformShowMenu, boolAction);
    hasSetDisclosed = Add(found, @selector(setAccessibilityDisclosed:), (IMP)SetDisclosed, boolSetter);
}

/*
    The view's three methods that hand AppKit AccessKit's elements:
    accessibilityChildren, accessibilityFocusedUIElement (VoiceOver and the
    system ask it often) and accessibilityHitTest: (Accessibility
    Inspector's pointer, VoiceOver's mouse tracking). Whichever AppKit asks
    first, the additions go in before AppKit gets an element: AccessKit
    registers its element class while the original makes the first
    element, so it is installed after the original returns and before its
    element is handed on. The call before the original covers a class that
    already exists. Neither runs inside AccessKit's borrow.
*/
static id ViewChildren(id self, SEL _cmd)
{
    QuorumRuntimeInstall();
    id children = ((QKObjectGetter)originalViewChildren)(self, _cmd);
    QuorumRuntimeInstall();
    return children;
}

static id ViewFocus(id self, SEL _cmd)
{
    QuorumRuntimeInstall();
    id focused = ((QKObjectGetter)originalViewFocus)(self, _cmd);
    QuorumRuntimeInstall();
    return focused;
}

static id ViewHitTest(id self, SEL _cmd, NSPoint point)
{
    QuorumRuntimeInstall();
    id hit = ((QKHitTest)originalViewHitTest)(self, _cmd, point);
    QuorumRuntimeInstall();
    return hit;
}

void QuorumRuntimeWatchView(void* view)
{
    if (view == NULL || watchedViewClass != Nil)
        return;
    // AccessKit's subclassing adapter gives the view a class of its own
    // (subclass.rs, subclass_of), whose accessibilityChildren is AccessKit's.
    Class viewClass = object_getClass((__bridge id)view);
    const char* name = class_getName(viewClass);
    Method method = OwnMethod(viewClass, @selector(accessibilityChildren));
    if (strncmp(name, "AccessKitSubclassOf", strlen("AccessKitSubclassOf")) != 0
        || method == NULL || !HasShape(method, NO, 2))
    {
        NSLog(@"Quorum VoiceOver additions: the view's class is %s, not AccessKit's; they go in at the first update instead.", name);
        return;
    }
    watchedViewClass = viewClass;
    originalViewChildren = method_setImplementation(method, (IMP)ViewChildren);

    // The other two ways AccessKit makes elements. If one is missing, the
    // additions still go in from accessibilityChildren or the first update.
    Method focusMethod = OwnMethod(viewClass, @selector(accessibilityFocusedUIElement));
    if (focusMethod != NULL && HasShape(focusMethod, NO, 2))
        originalViewFocus = method_setImplementation(focusMethod, (IMP)ViewFocus);
    else
        NSLog(@"Quorum VoiceOver additions: the view has no accessibilityFocusedUIElement of AccessKit's.");
    Method hitTestMethod = OwnMethod(viewClass, @selector(accessibilityHitTest:));
    if (hitTestMethod != NULL && HasShape(hitTestMethod, NO, 3))
        originalViewHitTest = method_setImplementation(hitTestMethod, (IMP)ViewHitTest);
    else
        NSLog(@"Quorum VoiceOver additions: the view has no accessibilityHitTest: of AccessKit's.");

    // The Headings rotor on the view too (CustomRotors). NSView answers
    // accessibilityCustomRotors itself; AccessKit's subclass doesn't.
    char objectGetter[16];
    snprintf(objectGetter, sizeof(objectGetter), "%s@:", @encode(id));
    if (!class_addMethod(viewClass, @selector(accessibilityCustomRotors), (IMP)ViewRotors, objectGetter))
        NSLog(@"Quorum VoiceOver additions: the view's class has its own accessibilityCustomRotors; the view offers no Headings rotor.");
}

void* QuorumRuntimeContentView(void* window)
{
    if (window == NULL)
        return NULL;
    NSWindow* nsWindow = (__bridge NSWindow*)window;
    return (__bridge void*)[nsWindow contentView];
}

// ---------------------------------------------------------------- Notices

static void Post(NSView* view, QKNotice notice)
{
    NSNumber* key = @(notice.node);
    if (notice.notice == QKMenuClosed)
    {
        // The menu's node is already gone from AccessKit's tree, so the
        // element kept when it opened is used, before AccessKit reports it
        // destroyed.
        id menu = openMenus[key];
        [openMenus removeObjectForKey:key];
        [elements removeObjectForKey:key];
        if (menu != nil)
            NSAccessibilityPostNotification(menu, (__bridge NSString*)kAXMenuClosedNotification);
        return;
    }

    id element = Find(view, notice.node, notice.near);
    if (element == nil)
        return;
    switch (notice.notice)
    {
        case QKMenuOpened:
            openMenus[key] = element;
            NSAccessibilityPostNotification(element, (__bridge NSString*)kAXMenuOpenedNotification);
            break;
        case QKRowExpanded:
            NSAccessibilityPostNotification(element, NSAccessibilityRowExpandedNotification);
            break;
        case QKRowCollapsed:
            NSAccessibilityPostNotification(element, NSAccessibilityRowCollapsedNotification);
            break;
        case QKRowCountChanged:
            NSAccessibilityPostNotification(element, NSAccessibilityRowCountChangedNotification);
            break;
        case QKMoved:
            // AccessKit posts nothing when only bounds change, so VoiceOver
            // would keep drawing its box where the element used to be.
            NSAccessibilityPostNotification(element, NSAccessibilityMovedNotification);
            // VoiceOver needs the layout change too, both to redraw its box
            // and to read a terrain cursor's move (plan 20's tests).
            NSAccessibilityPostNotificationWithUserInfo(element, NSAccessibilityLayoutChangedNotification,
                @{ NSAccessibilityUIElementsKey: @[element] });
            break;
        default:
            break;
    }
}

void QuorumRuntimeFinishUpdate(void* view, struct accesskit_macos_queued_events* events)
{
    // The tree may have changed, so outline rows and headings are worked out again.
    [outlineRows removeAllObjects];
    headings = nil;
    if (events == NULL)
    {
        // VoiceOver isn't active: nothing to tell it.
        noticeCount = 0;
        return;
    }

    NSView* nsView = (__bridge NSView*)view;
    // A closed menu first, while its element is alive and not yet reported destroyed.
    for (int i = 0; i < noticeCount; i++)
    {
        if (notices[i].notice == QKMenuClosed)
            Post(nsView, notices[i]);
    }
    accesskit_macos_queued_events_raise(events);

    QuorumRuntimeInstall();
    // The rest after AccessKit's own events, as a native control would.
    for (int i = 0; i < noticeCount && nsView != nil; i++)
    {
        if (notices[i].notice != QKMenuClosed)
            Post(nsView, notices[i]);
    }
    noticeCount = 0;
}

void QuorumRuntimeReset(void)
{
    [extras removeAllObjects];
    [elements removeAllObjects];
    [openMenus removeAllObjects];
    [outlineRows removeAllObjects];
    headings = nil;
    noticeCount = 0;
}

// ---------------------------------------------------------------- Shortcuts

/*
    For the names KeyboardEvent:ToText gives keys that aren't a single
    character: the virtual key code (kVK_..., HIToolbox Events.h) and the
    menu glyph a native menu shows for it (kMenu...Glyph, HIToolbox
    Menus.h). Returns NO for a name it doesn't know.
*/
static BOOL NamedKey(NSString* name, int* virtualKey, int* glyph)
{
    static NSDictionary<NSString*, NSArray<NSNumber*>*>* keys = nil;
    if (keys == nil)
    {
        keys = @{
            @"Up": @[@(kVK_UpArrow), @(kMenuUpArrowGlyph)],
            @"Down": @[@(kVK_DownArrow), @(kMenuDownArrowGlyph)],
            @"Left": @[@(kVK_LeftArrow), @(kMenuLeftArrowGlyph)],
            @"Right": @[@(kVK_RightArrow), @(kMenuRightArrowGlyph)],
            @"Enter": @[@(kVK_Return), @(kMenuReturnGlyph)],
            @"Escape": @[@(kVK_Escape), @(kMenuEscapeGlyph)],
            @"Tab": @[@(kVK_Tab), @(kMenuTabRightGlyph)],
            @"Space": @[@(kVK_Space), @(kMenuSpaceGlyph)],
            @"Backspace": @[@(kVK_Delete), @(kMenuDeleteLeftGlyph)],
            @"Forward Delete": @[@(kVK_ForwardDelete), @(kMenuDeleteRightGlyph)],
            @"Home": @[@(kVK_Home), @(kMenuNorthwestArrowGlyph)],
            @"End": @[@(kVK_End), @(kMenuSoutheastArrowGlyph)],
            @"Page Up": @[@(kVK_PageUp), @(kMenuPageUpGlyph)],
            @"Page Down": @[@(kVK_PageDown), @(kMenuPageDownGlyph)],
            @"F1": @[@(kVK_F1), @(kMenuF1Glyph)], @"F2": @[@(kVK_F2), @(kMenuF2Glyph)],
            @"F3": @[@(kVK_F3), @(kMenuF3Glyph)], @"F4": @[@(kVK_F4), @(kMenuF4Glyph)],
            @"F5": @[@(kVK_F5), @(kMenuF5Glyph)], @"F6": @[@(kVK_F6), @(kMenuF6Glyph)],
            @"F7": @[@(kVK_F7), @(kMenuF7Glyph)], @"F8": @[@(kVK_F8), @(kMenuF8Glyph)],
            @"F9": @[@(kVK_F9), @(kMenuF9Glyph)], @"F10": @[@(kVK_F10), @(kMenuF10Glyph)],
            @"F11": @[@(kVK_F11), @(kMenuF11Glyph)], @"F12": @[@(kVK_F12), @(kMenuF12Glyph)]
        };
    }
    NSArray<NSNumber*>* key = keys[name];
    if (key == nil)
        return NO;
    *virtualKey = key[0].intValue;
    *glyph = key[1].intValue;
    return YES;
}

/*
    Reads a shortcut as MenuItem:GetShortcut gives it (InputSet:GetDisplayName:
    "Command + Shift + S", "Control + F5"). Command is assumed unless
    kAXMenuItemModifierNoCommand is set (AXAttributeConstants.h). Anything
    it can't read leaves the item with no shortcut.
*/
static void ReadShortcut(NSString* text, QuorumAccessKitExtra* extra)
{
    extra.hasShortcut = NO;
    extra.commandCharacter = nil;
    extra.commandModifiers = 0;
    extra.commandVirtualKey = -1;
    extra.commandGlyph = -1;
    if (text == nil || text.length == 0)
        return;

    NSArray<NSString*>* parts = [text componentsSeparatedByString:@" + "];
    BOOL command = NO;
    int modifiers = 0;
    for (NSUInteger i = 0; i + 1 < parts.count; i++)
    {
        NSString* part = parts[i];
        if ([part isEqualToString:@"Command"] || [part isEqualToString:@"Meta"])
            command = YES;
        else if ([part isEqualToString:@"Shift"])
            modifiers |= kAXMenuItemModifierShift;
        else if ([part isEqualToString:@"Alt"] || [part isEqualToString:@"Option"])
            modifiers |= kAXMenuItemModifierOption;
        else if ([part isEqualToString:@"Control"])
            modifiers |= kAXMenuItemModifierControl;
        else
            return;
    }
    if (!command)
        modifiers |= kAXMenuItemModifierNoCommand;

    NSString* key = parts.lastObject;
    if (key.length == 1)
        extra.commandCharacter = key.uppercaseString;
    else
    {
        // A key with no character, as a native menu item gives it: its
        // virtual key and glyph, and no AXMenuItemCmdChar.
        int virtualKey = -1;
        int glyph = -1;
        if (!NamedKey(key, &virtualKey, &glyph))
            return;
        extra.commandVirtualKey = virtualKey;
        extra.commandGlyph = glyph;
    }
    extra.commandModifiers = modifiers;
    extra.hasShortcut = YES;
}

// ---------------------------------------------------------------- MacAccessibilityExtras

static NSString* StringFrom(JNIEnv* env, jbyteArray text)
{
    if (text == NULL)
        return nil;
    jsize length = (*env)->GetArrayLength(env, text);
    jbyte* bytes = (*env)->GetByteArrayElements(env, text, NULL);
    if (bytes == NULL)
        return nil;     // Out of memory; Java gets the exception.
    NSString* result = [[NSString alloc] initWithBytes:bytes length:(NSUInteger)length encoding:NSUTF8StringEncoding];
    (*env)->ReleaseByteArrayElements(env, text, bytes, JNI_ABORT);
    return result;
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_MacAccessibilityExtras_nativeSetInfo
    (JNIEnv* env, jclass type, jlong node, jint flags, jint level, jint index, jint count, jint position, jbyteArray shortcut)
{
    @autoreleasepool
    {
        PrepareTables();
        QuorumAccessKitExtra* extra = [[QuorumAccessKitExtra alloc] init];
        extra.flags = (int)flags;
        extra.level = level < 0 ? 0 : (NSInteger)level;
        extra.index = (NSInteger)index;
        extra.count = (NSInteger)count;
        extra.position = (NSInteger)position;
        ReadShortcut(StringFrom(env, shortcut), extra);
        extras[@((uint64_t)node)] = extra;
        [outlineRows removeAllObjects];
        headings = nil;
    }
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_MacAccessibilityExtras_nativeRemove
    (JNIEnv* env, jclass type, jlong node)
{
    @autoreleasepool
    {
        PrepareTables();
        NSNumber* key = @((uint64_t)node);
        [extras removeObjectForKey:key];
        headings = nil;
        [elements removeObjectForKey:key];
        [openMenus removeObjectForKey:key];
        [outlineRows removeAllObjects];
    }
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_MacAccessibilityExtras_nativeQueueNotice
    (JNIEnv* env, jclass type, jlong node, jlong near, jint notice)
{
    if (noticeCount >= QK_MAX_NOTICES)
        return;
    notices[noticeCount].node = (uint64_t)node;
    notices[noticeCount].near = (uint64_t)near;
    notices[noticeCount].notice = (int)notice;
    noticeCount = noticeCount + 1;
}
