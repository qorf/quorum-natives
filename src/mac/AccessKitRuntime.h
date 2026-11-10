/*
    The run-time additions to AccessKit's macOS elements (AccessKitRuntime.m),
    as AccessKitJNI.m calls them. Main thread only.
*/
#ifndef QUORUM_ACCESSKIT_RUNTIME_H
#define QUORUM_ACCESSKIT_RUNTIME_H

#include <stdint.h>
#include "accesskit.h"

/* The content view of an NSWindow, as AccessKit's window adapter uses it. */
void* QuorumRuntimeContentView(void* window);

/*
    Watches the view AccessKit's adapter took over, so the additions go in
    the moment AccessKit first makes its elements. Call once the adapter
    exists.
*/
void QuorumRuntimeWatchView(void* view);

/* Adds the methods, once, if AccessKit has registered its element class. */
void QuorumRuntimeInstall(void);

/*
    Raises the events an update queued (events may be NULL: VoiceOver isn't
    active, and queued notices are dropped), with Quorum's own notices
    around them. view is the adapter's view.
*/
void QuorumRuntimeFinishUpdate(void* view, struct accesskit_macos_queued_events* events);

/* Forgets every node and notice (the adapter is gone). */
void QuorumRuntimeReset(void);

/*
    Where the requests the additions take from VoiceOver go: showing a
    node's context menu (ACCESSKIT_ACTION_SHOW_CONTEXT_MENU), and opening
    or closing a row (ACCESSKIT_ACTION_EXPAND, ACCESSKIT_ACTION_COLLAPSE),
    which AccessKit's adapter doesn't send. node is the Quorum node id.
    The sink is called on the main thread, from inside AppKit's call on an
    element, so it must only queue the request. NULL drops them.
*/
typedef void (*QuorumActionSink)(int action, uint64_t node, void* userdata);
void QuorumRuntimeSetActionSink(QuorumActionSink sink, void* userdata);

#endif
