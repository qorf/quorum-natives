/*
    The run-time additions to AccessKit's macOS elements (AccessKitRuntime.m),
    as AccessKitJNI.m calls them. Main thread only.
*/
#ifndef QUORUM_ACCESSKIT_RUNTIME_H
#define QUORUM_ACCESSKIT_RUNTIME_H

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

#endif
