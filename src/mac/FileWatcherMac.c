// FileWatcherMac.c: Quorum's FileWatcher on the Mac, through FSEvents
// (Studio plan FileWatching.md, 2026-11-06).
// Java's plugins.quorum.Libraries.System.MacFileEvents calls these. Events
// are kept in a queue that Java's watcher thread empties with Take; C never
// calls into Java. Plain C, so it is compiled without -fobjc-arc and uses
// dispatch_release.

#include <jni.h>
#include <CoreServices/CoreServices.h>
#include <dispatch/dispatch.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define MOST_WAITING 65536

typedef struct QueuedEvent {
    char* path;
    uint32_t flags;
    int32_t root;
} QueuedEvent;

typedef struct EventQueue {
    pthread_mutex_t mutex;
    pthread_cond_t ready;
    QueuedEvent* events;
    size_t count;
    size_t capacity;
    int overflowed;
} EventQueue;

typedef struct Stream {
    EventQueue* queue;
    int32_t root;
    FSEventStreamRef stream;
    dispatch_queue_t dispatch;
} Stream;

static void NothingToDo(void* context) {
}

static void Callback(ConstFSEventStreamRef ref, void* info, size_t count, void* eventPaths,
        const FSEventStreamEventFlags flags[], const FSEventStreamEventId ids[]) {
    Stream* stream = (Stream*) info;
    EventQueue* queue = stream->queue;
    char** paths = (char**) eventPaths;
    pthread_mutex_lock(&queue->mutex);
    size_t i;
    for (i = 0; i < count && !queue->overflowed; i++) {
        if (queue->count >= MOST_WAITING) {
            queue->overflowed = 1;
            break;
        }
        if (queue->count == queue->capacity) {
            size_t larger = queue->capacity == 0 ? 256 : queue->capacity * 2;
            QueuedEvent* grown = (QueuedEvent*) realloc(queue->events, larger * sizeof(QueuedEvent));
            if (grown == NULL) {
                queue->overflowed = 1;
                break;
            }
            queue->events = grown;
            queue->capacity = larger;
        }
        char* copy = strdup(paths[i]);
        if (copy == NULL) {
            queue->overflowed = 1;
            break;
        }
        queue->events[queue->count].path = copy;
        queue->events[queue->count].flags = (uint32_t) flags[i];
        queue->events[queue->count].root = stream->root;
        queue->count++;
    }
    pthread_cond_signal(&queue->ready);
    pthread_mutex_unlock(&queue->mutex);
}

static void FreeEvents(EventQueue* queue) {
    size_t i;
    for (i = 0; i < queue->count; i++) {
        free(queue->events[i].path);
    }
    queue->count = 0;
}

// The version of this API, so Java can check Mac.so has it.
JNIEXPORT jint JNICALL Java_plugins_quorum_Libraries_System_MacFileEvents_Version(JNIEnv* env, jclass type) {
    return 1;
}

JNIEXPORT jlong JNICALL Java_plugins_quorum_Libraries_System_MacFileEvents_CreateQueue(JNIEnv* env, jclass type) {
    EventQueue* queue = (EventQueue*) calloc(1, sizeof(EventQueue));
    if (queue == NULL) {
        return 0;
    }
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->ready, NULL);
    return (jlong) (intptr_t) queue;
}

// Starts watching a folder and everything in it. Returns 0 if it couldn't.
JNIEXPORT jlong JNICALL Java_plugins_quorum_Libraries_System_MacFileEvents_AddStream(JNIEnv* env, jclass type,
        jlong queueHandle, jstring folder, jdouble latency, jint root) {
    EventQueue* queue = (EventQueue*) (intptr_t) queueHandle;
    if (queue == NULL || folder == NULL) {
        return 0;
    }
    const char* utf = (*env)->GetStringUTFChars(env, folder, NULL);
    if (utf == NULL) {
        return 0;
    }
    CFStringRef path = CFStringCreateWithCString(kCFAllocatorDefault, utf, kCFStringEncodingUTF8);
    (*env)->ReleaseStringUTFChars(env, folder, utf);
    if (path == NULL) {
        return 0;
    }
    const void* values[1] = { path };
    CFArrayRef paths = CFArrayCreate(kCFAllocatorDefault, values, 1, &kCFTypeArrayCallBacks);
    CFRelease(path);
    if (paths == NULL) {
        return 0;
    }

    Stream* stream = (Stream*) calloc(1, sizeof(Stream));
    if (stream == NULL) {
        CFRelease(paths);
        return 0;
    }
    stream->queue = queue;
    stream->root = root;

    FSEventStreamContext context = { 0, stream, NULL, NULL, NULL };
    stream->stream = FSEventStreamCreate(kCFAllocatorDefault, Callback, &context, paths,
            kFSEventStreamEventIdSinceNow, (CFTimeInterval) latency,
            kFSEventStreamCreateFlagFileEvents | kFSEventStreamCreateFlagNoDefer | kFSEventStreamCreateFlagWatchRoot);
    CFRelease(paths);
    if (stream->stream == NULL) {
        free(stream);
        return 0;
    }
    stream->dispatch = dispatch_queue_create("Quorum File Watcher", DISPATCH_QUEUE_SERIAL);
    FSEventStreamSetDispatchQueue(stream->stream, stream->dispatch);
    if (!FSEventStreamStart(stream->stream)) {
        FSEventStreamInvalidate(stream->stream);
        FSEventStreamRelease(stream->stream);
        dispatch_release(stream->dispatch);
        free(stream);
        return 0;
    }
    return (jlong) (intptr_t) stream;
}

// Stops one stream. Its events already queued stay; Java drops them by root.
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_System_MacFileEvents_RemoveStream(JNIEnv* env, jclass type,
        jlong streamHandle) {
    Stream* stream = (Stream*) (intptr_t) streamHandle;
    if (stream == NULL) {
        return;
    }
    FSEventStreamStop(stream->stream);
    FSEventStreamInvalidate(stream->stream);
    // Waits for a callback that may still be running, then releases.
    dispatch_sync_f(stream->dispatch, NULL, NothingToDo);
    FSEventStreamRelease(stream->stream);
    dispatch_release(stream->dispatch);
    free(stream);
}

// Waits up to milliseconds for events, then copies up to paths' length of
// them. Returns how many, or -1 if events were lost (the queue overflowed;
// everything waiting is dropped, and Java rescans).
JNIEXPORT jint JNICALL Java_plugins_quorum_Libraries_System_MacFileEvents_Take(JNIEnv* env, jclass type,
        jlong queueHandle, jint milliseconds, jobjectArray paths, jintArray flags, jintArray roots) {
    EventQueue* queue = (EventQueue*) (intptr_t) queueHandle;
    if (queue == NULL) {
        return 0;
    }
    pthread_mutex_lock(&queue->mutex);
    if (queue->count == 0 && !queue->overflowed && milliseconds > 0) {
        struct timeval now;
        gettimeofday(&now, NULL);
        struct timespec until;
        long long nanoseconds = (long long) now.tv_usec * 1000LL + (long long) (milliseconds % 1000) * 1000000LL;
        until.tv_sec = now.tv_sec + milliseconds / 1000 + (time_t) (nanoseconds / 1000000000LL);
        until.tv_nsec = (long) (nanoseconds % 1000000000LL);
        pthread_cond_timedwait(&queue->ready, &queue->mutex, &until);
    }
    if (queue->overflowed) {
        FreeEvents(queue);
        queue->overflowed = 0;
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }
    jsize room = (*env)->GetArrayLength(env, paths);
    size_t taken = queue->count < (size_t) room ? queue->count : (size_t) room;
    size_t i;
    for (i = 0; i < taken; i++) {
        QueuedEvent* event = &queue->events[i];
        jstring text = (*env)->NewStringUTF(env, event->path);
        (*env)->SetObjectArrayElement(env, paths, (jsize) i, text);
        (*env)->DeleteLocalRef(env, text);
        jint flag = (jint) event->flags;
        jint root = (jint) event->root;
        (*env)->SetIntArrayRegion(env, flags, (jsize) i, 1, &flag);
        (*env)->SetIntArrayRegion(env, roots, (jsize) i, 1, &root);
        free(event->path);
    }
    if (taken > 0) {
        memmove(queue->events, queue->events + taken, (queue->count - taken) * sizeof(QueuedEvent));
        queue->count -= taken;
    }
    pthread_mutex_unlock(&queue->mutex);
    return (jint) taken;
}

// Frees a queue. Every stream feeding it must be removed first, and no Take
// may be running.
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_System_MacFileEvents_DestroyQueue(JNIEnv* env, jclass type,
        jlong queueHandle) {
    EventQueue* queue = (EventQueue*) (intptr_t) queueHandle;
    if (queue == NULL) {
        return;
    }
    pthread_mutex_lock(&queue->mutex);
    FreeEvents(queue);
    pthread_mutex_unlock(&queue->mutex);
    free(queue->events);
    pthread_cond_destroy(&queue->ready);
    pthread_mutex_destroy(&queue->mutex);
    free(queue);
}
