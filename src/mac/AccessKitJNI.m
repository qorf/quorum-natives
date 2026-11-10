/*
    VoiceOver on the Mac through AccessKit (accesskit-c 0.23.2, a static
    library linked into Mac.so). Called from Java through JNI by the classes
    in plugins.quorum.Libraries.Interface.Accessibility.AccessKitBinding,
    which replace the old dev.accesskit binding (accesskit.jar and
    libaccesskit_jni.dylib).

    Java builds Node and TreeUpdate objects, each holding a pointer to its
    AccessKit twin. A node's pointer belongs to the tree update once it is
    pushed in, and an update's pointer belongs to AccessKit once it is handed
    over, so Java forgets each pointer at that moment (see TreeUpdate.java).
    Strings and arrays are copied by AccessKit, so Java keeps its own.

    AccessKit calls back into Java in two ways, both through
    NativePointerSupplier.get, which hands over a TreeUpdate's pointer:
    the activation handler, the first time VoiceOver looks at the window,
    and the update factory, while updateIfActive runs. Both come on the
    AppKit main thread, which in a Quorum game is Java's main thread
    (-XstartOnFirstThread): VoiceOver's questions are answered inside
    glfwPollEvents, and updates inside the game's own call.

    AccessKit is built with panic = abort, so an invalid tree (a child id
    that isn't in the tree, a focus that isn't a node) ends the program.

    What AccessKit's adapter leaves out (menu notices, outline rows and so
    on) is added to its elements at run time by AccessKitRuntime.m, which
    the adapter natives here call (AccessKitRuntime.h).

    VoiceOver's action requests reach Java through NativeActionReceiver:
    AccessKit's (Act) and the ones AccessKitRuntime.m adds (RuntimeAct).
    Java only queues them; MacAccessibility carries them out in its next
    update, because AccessKit may have its tree borrowed while they come.

    Compiled with ARC (see CMakeLists.txt). No Objective-C objects are kept
    here; the autorelease pools catch what AccessKit's AppKit calls leave.
*/

#import <Foundation/Foundation.h>
#include <jni.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "accesskit.h"
#include "AccessKitRuntime.h"

// The JVM, kept so AccessKit's callbacks can find a JNIEnv. Set by the
// first adapter call; Mac.so has no JNI_OnLoad and doesn't need one.
static JavaVM* quorumVM = NULL;

// NativePointerSupplier.get()J, through which Java hands over a TreeUpdate.
static jmethodID supplierGet = NULL;

// NativeActionReceiver.receive, through which VoiceOver's requests reach Java.
static jmethodID receiverReceive = NULL;

/*
    One adapter as Java sees it: AccessKit's adapter, plus the global
    references AccessKit holds as userdata: the activation handler's
    supplier, and the action receiver (NULL if Java gave none). nativeDrop
    frees them all. view is the NSView the adapter took over, where
    AccessKitRuntime.m starts looking for elements; AccessKit keeps it
    alive while the adapter lives (subclass.rs, SubclassingAdapter).
*/
typedef struct QuorumAdapter
{
    struct accesskit_macos_subclassing_adapter* adapter;
    jobject activation;
    jobject actions;
    void* view;
} QuorumAdapter;

/*
    What the update factory needs while updateIfActive runs. It lives on the
    stack: AccessKit calls the factory at most once, and only before
    accesskit_macos_subclassing_adapter_update_if_active returns, on the
    same thread, so the JNIEnv and the local reference are still good.
*/
typedef struct UpdateRequest
{
    JNIEnv* env;
    jobject supplier;
    accesskit_node_id fallbackFocus;
} UpdateRequest;

static struct accesskit_node* NodeFrom(jlong pointer)
{
    return (struct accesskit_node*)(intptr_t)pointer;
}

static struct accesskit_tree_update* UpdateFrom(jlong pointer)
{
    return (struct accesskit_tree_update*)(intptr_t)pointer;
}

static QuorumAdapter* AdapterFrom(jlong pointer)
{
    return (QuorumAdapter*)(intptr_t)pointer;
}

static jlong ToJava(void* pointer)
{
    return (jlong)(intptr_t)pointer;
}

// AccessKit's counts and indexes are size_t; Java's are int. A negative one becomes 0.
static size_t ToSize(jint value)
{
    return value < 0 ? 0 : (size_t)value;
}

static void ThrowIllegalState(JNIEnv* env, const char* message)
{
    jclass exceptionClass = (*env)->FindClass(env, "java/lang/IllegalStateException");
    if (exceptionClass != NULL)
        (*env)->ThrowNew(env, exceptionClass, message);
}

/*
    Remembers the JVM and finds NativePointerSupplier.get. Returns false,
    with a Java exception pending, if either fails.
*/
static jboolean Prepare(JNIEnv* env)
{
    if (quorumVM == NULL && (*env)->GetJavaVM(env, &quorumVM) != JNI_OK)
    {
        quorumVM = NULL;
        ThrowIllegalState(env, "AccessKit: GetJavaVM failed");
        return JNI_FALSE;
    }

    if (supplierGet == NULL)
    {
        // The interface's method id works on any class that implements it,
        // including the lambdas TreeUpdate.makeNativeSupplier makes.
        jclass supplierClass = (*env)->FindClass(env,
            "plugins/quorum/Libraries/Interface/Accessibility/AccessKitBinding/NativePointerSupplier");
        if (supplierClass == NULL)
            return JNI_FALSE;
        supplierGet = (*env)->GetMethodID(env, supplierClass, "get", "()J");
        (*env)->DeleteLocalRef(env, supplierClass);
        if (supplierGet == NULL)
            return JNI_FALSE;
    }

    if (receiverReceive == NULL)
    {
        jclass receiverClass = (*env)->FindClass(env,
            "plugins/quorum/Libraries/Interface/Accessibility/AccessKitBinding/NativeActionReceiver");
        if (receiverClass == NULL)
            return JNI_FALSE;
        // receive(int action, long target, int dataKind, byte[] value, double number,
        //         long anchorNode, int anchorIndex, long focusNode, int focusIndex)
        receiverReceive = (*env)->GetMethodID(env, receiverClass, "receive", "(IJI[BDJIJI)V");
        (*env)->DeleteLocalRef(env, receiverClass);
        if (receiverReceive == NULL)
            return JNI_FALSE;
    }
    return JNI_TRUE;
}

/*
    The JNIEnv for the thread AccessKit called back on. That is Java's main
    thread in a Quorum game, which is already attached. Any other thread is
    attached as a daemon and left attached, so a later callback finds it.
*/
static JNIEnv* CurrentEnv(void)
{
    if (quorumVM == NULL)
        return NULL;

    JNIEnv* env = NULL;
    jint result = (*quorumVM)->GetEnv(quorumVM, (void**)&env, JNI_VERSION_1_6);
    if (result == JNI_EDETACHED)
    {
        if ((*quorumVM)->AttachCurrentThreadAsDaemon(quorumVM, (void**)&env, NULL) != JNI_OK)
            return NULL;
    }
    else if (result != JNI_OK)
        return NULL;
    return env;
}

/*
    Asks a NativePointerSupplier for a TreeUpdate pointer, which it gives up.
    Returns NULL if Java gave 0 or threw. TreeUpdate's suppliers catch and
    log everything, so an exception here is something like an
    OutOfMemoryError; AccessKit can't take a Java exception, so it is
    cleared here (Java has already logged what it could).
*/
static struct accesskit_tree_update* AskJava(JNIEnv* env, jobject supplier)
{
    jlong pointer = (*env)->CallLongMethod(env, supplier, supplierGet);
    if ((*env)->ExceptionCheck(env))
    {
        (*env)->ExceptionClear(env);
        return NULL;
    }
    return UpdateFrom(pointer);
}

/*
    AccessKit's activation handler: VoiceOver is looking at the window for
    the first time and AccessKit wants the whole tree. NULL means "not
    ready"; AccessKit then shows an empty placeholder and doesn't ask again,
    so the next update must carry the full tree (accesskit_macos 0.27.2,
    Adapter::update_if_active).
*/
static struct accesskit_tree_update* Activate(void* userdata)
{
    JNIEnv* env = CurrentEnv();
    if (env == NULL)
        return NULL;
    return AskJava(env, (jobject)userdata);
}

/*
    AccessKit's update factory, called inside updateIfActive. It must not
    return NULL (accesskit.h), so if Java fails, it sends an empty update
    whose focus is the last focus AccessKit accepted. That node is in the
    tree, so nothing changes and nothing panics.
*/
static struct accesskit_tree_update* Supply(void* userdata)
{
    UpdateRequest* request = (UpdateRequest*)userdata;
    struct accesskit_tree_update* update = AskJava(request->env, request->supplier);
    if (update == NULL)
        update = accesskit_tree_update_with_focus(request->fallbackFocus);
    return update;
}

// A character index for Java; AccessKit's are size_t.
static jint ToJavaIndex(size_t value)
{
    return value > INT32_MAX ? INT32_MAX : (jint)value;
}

/*
    Gives one request to Java's NativeActionReceiver as plain values, and
    clears any Java exception (receive catches everything; this is only for
    something like an OutOfMemoryError, which AppKit and AccessKit can't
    take). value may be NULL.
*/
static void Deliver(JNIEnv* env, jobject receiver, jint action, jlong target, jint dataKind,
    jbyteArray value, jdouble number, jlong anchorNode, jint anchorIndex, jlong focusNode, jint focusIndex)
{
    if (!(*env)->ExceptionCheck(env))
    {
        (*env)->CallVoidMethod(env, receiver, receiverReceive, action, target, dataKind, value, number,
            anchorNode, anchorIndex, focusNode, focusIndex);
    }
    if ((*env)->ExceptionCheck(env))
        (*env)->ExceptionClear(env);
}

/*
    AccessKit's action handler: VoiceOver pressing a button, focusing an
    item, setting a value, moving a caret. userdata is the global reference
    to Java's NativeActionReceiver, or NULL. The request is taken apart into
    plain values (the text as UTF-8 bytes) and given to Java, which only
    queues it: AccessKit has its tree borrowed while this runs, so updating
    it from here would panic. This callback owns the request and frees it.
*/
static void Act(struct accesskit_action_request* request, void* userdata)
{
    jobject receiver = (jobject)userdata;
    JNIEnv* env = receiver == NULL || receiverReceive == NULL ? NULL : CurrentEnv();
    if (env != NULL)
    {
        jint dataKind = -1;     // NativeActionReceiver.NO_DATA
        jbyteArray value = NULL;
        jdouble number = 0;
        jlong anchorNode = 0, focusNode = 0;
        jint anchorIndex = 0, focusIndex = 0;
        if (request->data.has_value)
        {
            const struct accesskit_action_data* data = &request->data.value;
            dataKind = (jint)data->tag;
            switch (data->tag)
            {
                case ACCESSKIT_ACTION_DATA_VALUE:
                    if (data->value != NULL)
                    {
                        jsize length = (jsize)strlen(data->value);
                        value = (*env)->NewByteArray(env, length);
                        if (value != NULL)
                            (*env)->SetByteArrayRegion(env, value, 0, length, (const jbyte*)data->value);
                    }
                    break;
                case ACCESSKIT_ACTION_DATA_NUMERIC_VALUE:
                    number = data->numeric_value;
                    break;
                case ACCESSKIT_ACTION_DATA_SET_TEXT_SELECTION:
                    anchorNode = (jlong)data->set_text_selection.anchor.node;
                    anchorIndex = ToJavaIndex(data->set_text_selection.anchor.character_index);
                    focusNode = (jlong)data->set_text_selection.focus.node;
                    focusIndex = ToJavaIndex(data->set_text_selection.focus.character_index);
                    break;
                default:
                    break;
            }
        }
        Deliver(env, receiver, (jint)request->action, (jlong)request->target_node, dataKind, value, number,
            anchorNode, anchorIndex, focusNode, focusIndex);
        if (value != NULL)
            (*env)->DeleteLocalRef(env, value);
    }
    accesskit_action_request_free(request);
}

/*
    The requests AccessKitRuntime.m adds (show a context menu, open or
    close a row), with no data. userdata is the same receiver as Act's.
    Called from inside AppKit's call on an element; Java only queues.
*/
static void RuntimeAct(int action, uint64_t node, void* userdata)
{
    jobject receiver = (jobject)userdata;
    JNIEnv* env = receiver == NULL || receiverReceive == NULL ? NULL : CurrentEnv();
    if (env == NULL)
        return;
    Deliver(env, receiver, (jint)action, (jlong)node, -1, NULL, 0, 0, 0, 0, 0);
}

/* Raises the events an update queued, if there are any. */
static void Raise(struct accesskit_macos_queued_events* events)
{
    if (events != NULL)
        accesskit_macos_queued_events_raise(events);
}

/*
    Makes the QuorumAdapter for a view or a window. The supplier is the
    activation handler's, wrapped by TreeUpdate.makeNativeSupplier, and the
    receiver wraps the ActionHandler (or is NULL); AccessKit keeps both as
    userdata, so they get global references.
*/
static QuorumAdapter* NewAdapter(JNIEnv* env, jobject supplier, jobject receiver)
{
    if (!Prepare(env))
        return NULL;

    QuorumAdapter* result = (QuorumAdapter*)calloc(1, sizeof(QuorumAdapter));
    if (result == NULL)
    {
        ThrowIllegalState(env, "AccessKit: out of memory");
        return NULL;
    }
    result->activation = (*env)->NewGlobalRef(env, supplier);
    if (result->activation == NULL)
    {
        free(result);
        ThrowIllegalState(env, "AccessKit: no global reference for the activation handler");
        return NULL;
    }
    if (receiver != NULL)
    {
        result->actions = (*env)->NewGlobalRef(env, receiver);
        if (result->actions == NULL)
        {
            (*env)->DeleteGlobalRef(env, result->activation);
            free(result);
            ThrowIllegalState(env, "AccessKit: no global reference for the action handler");
            return NULL;
        }
    }
    return result;
}

JNIEXPORT jlong JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_MacosSubclassingAdapter_nativeNew
    (JNIEnv* env, jclass type, jlong view, jobject supplier, jobject receiver)
{
    @autoreleasepool
    {
        QuorumAdapter* result = NewAdapter(env, supplier, receiver);
        if (result == NULL)
            return 0;
        // view is an NSView*.
        result->view = (void*)(intptr_t)view;
        result->adapter = accesskit_macos_subclassing_adapter_new(result->view,
            Activate, result->activation, Act, result->actions);
        QuorumRuntimeWatchView(result->view);
        QuorumRuntimeSetActionSink(result->actions == NULL ? NULL : RuntimeAct, result->actions);
        return ToJava(result);
    }
}

JNIEXPORT jlong JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_MacosSubclassingAdapter_nativeForWindow
    (JNIEnv* env, jclass type, jlong window, jobject supplier, jobject receiver)
{
    @autoreleasepool
    {
        QuorumAdapter* result = NewAdapter(env, supplier, receiver);
        if (result == NULL)
            return 0;
        // window is an NSWindow* (glfwGetCocoaWindow). AccessKit panics, and
        // so ends the program, if it has no content view; GLFW's always do.
        result->adapter = accesskit_macos_subclassing_adapter_for_window((void*)(intptr_t)window,
            Activate, result->activation, Act, result->actions);
        // The view AccessKit took over (subclass.rs, for_window).
        result->view = QuorumRuntimeContentView((void*)(intptr_t)window);
        QuorumRuntimeWatchView(result->view);
        QuorumRuntimeSetActionSink(result->actions == NULL ? NULL : RuntimeAct, result->actions);
        return ToJava(result);
    }
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_MacosSubclassingAdapter_nativeDrop
    (JNIEnv* env, jclass type, jlong pointer)
{
    @autoreleasepool
    {
        QuorumAdapter* adapter = AdapterFrom(pointer);
        if (adapter == NULL)
            return;
        // The adapter and the runtime's sink first, so neither AccessKit
        // nor the additions can use a reference that is already gone.
        QuorumRuntimeSetActionSink(NULL, NULL);
        if (adapter->adapter != NULL)
            accesskit_macos_subclassing_adapter_free(adapter->adapter);
        (*env)->DeleteGlobalRef(env, adapter->activation);
        if (adapter->actions != NULL)
            (*env)->DeleteGlobalRef(env, adapter->actions);
        free(adapter);
        QuorumRuntimeReset();
    }
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_MacosSubclassingAdapter_nativeUpdateIfActive
    (JNIEnv* env, jclass type, jlong pointer, jlong fallbackFocus, jobject supplier)
{
    @autoreleasepool
    {
        QuorumAdapter* adapter = AdapterFrom(pointer);
        UpdateRequest request = { env, supplier, (accesskit_node_id)fallbackFocus };
        // NULL when VoiceOver isn't active; the supplier isn't called then.
        // The events are raised with Quorum's own notices around them, once
        // AccessKit has let go of its tree.
        QuorumRuntimeFinishUpdate(adapter->view,
            accesskit_macos_subclassing_adapter_update_if_active(adapter->adapter, Supply, &request));
    }
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_MacosSubclassingAdapter_nativeUpdateViewFocusState
    (JNIEnv* env, jclass type, jlong pointer, jboolean focused)
{
    @autoreleasepool
    {
        QuorumAdapter* adapter = AdapterFrom(pointer);
        Raise(accesskit_macos_subclassing_adapter_update_view_focus_state(adapter->adapter, focused == JNI_TRUE));
    }
}

// ---------------------------------------------------------------- Node

typedef void (*StringSetter)(struct accesskit_node* node, const char* value, size_t length);
typedef void (*BytesSetter)(struct accesskit_node* node, size_t length, const uint8_t* values);
typedef void (*FloatsSetter)(struct accesskit_node* node, size_t length, const float* values);
typedef void (*Clearer)(struct accesskit_node* node);

/*
    Sets a string property from UTF-8 bytes (Util.bytesFromString), or
    clears it when Java passed null. AccessKit copies the bytes, so Java's
    array is released without copying anything back (JNI_ABORT).
*/
static void SetString(JNIEnv* env, jlong node, jbyteArray text, StringSetter setter, Clearer clearer)
{
    if (text == NULL)
    {
        clearer(NodeFrom(node));
        return;
    }
    jsize length = (*env)->GetArrayLength(env, text);
    jbyte* bytes = (*env)->GetByteArrayElements(env, text, NULL);
    if (bytes == NULL)
        return;     // Out of memory; Java gets the exception.
    setter(NodeFrom(node), (const char*)bytes, (size_t)length);
    (*env)->ReleaseByteArrayElements(env, text, bytes, JNI_ABORT);
}

/* The same for an array of unsigned bytes (Java's signed bytes, read as uint8_t). */
static void SetBytes(JNIEnv* env, jlong node, jbyteArray values, BytesSetter setter, Clearer clearer)
{
    if (values == NULL)
    {
        clearer(NodeFrom(node));
        return;
    }
    jsize length = (*env)->GetArrayLength(env, values);
    jbyte* elements = (*env)->GetByteArrayElements(env, values, NULL);
    if (elements == NULL)
        return;
    setter(NodeFrom(node), (size_t)length, (const uint8_t*)elements);
    (*env)->ReleaseByteArrayElements(env, values, elements, JNI_ABORT);
}

/* The same for an array of floats. */
static void SetFloats(JNIEnv* env, jlong node, jfloatArray values, FloatsSetter setter, Clearer clearer)
{
    if (values == NULL)
    {
        clearer(NodeFrom(node));
        return;
    }
    jsize length = (*env)->GetArrayLength(env, values);
    jfloat* elements = (*env)->GetFloatArrayElements(env, values, NULL);
    if (elements == NULL)
        return;
    setter(NodeFrom(node), (size_t)length, (const float*)elements);
    (*env)->ReleaseFloatArrayElements(env, values, elements, JNI_ABORT);
}

// role is a Role constant's value: its place in enum accesskit_role.
JNIEXPORT jlong JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeNew
    (JNIEnv* env, jclass type, jint role)
{
    return ToJava(accesskit_node_new((accesskit_role)role));
}

// Only for a node that was never pushed into an update (Node.drop).
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeDrop
    (JNIEnv* env, jclass type, jlong node)
{
    accesskit_node_free(NodeFrom(node));
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeAddAction
    (JNIEnv* env, jclass type, jlong node, jint action)
{
    accesskit_node_add_action(NodeFrom(node), (accesskit_action)action);
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeSetLabel
    (JNIEnv* env, jclass type, jlong node, jbyteArray text)
{
    SetString(env, node, text, accesskit_node_set_label_with_length, accesskit_node_clear_label);
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeSetDescription
    (JNIEnv* env, jclass type, jlong node, jbyteArray text)
{
    SetString(env, node, text, accesskit_node_set_description_with_length, accesskit_node_clear_description);
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeSetValue
    (JNIEnv* env, jclass type, jlong node, jbyteArray text)
{
    SetString(env, node, text, accesskit_node_set_value_with_length, accesskit_node_clear_value);
}

// What VoiceOver says instead of the role's name, such as "terrain".
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeSetRoleDescription
    (JNIEnv* env, jclass type, jlong node, jbyteArray text)
{
    SetString(env, node, text, accesskit_node_set_role_description_with_length, accesskit_node_clear_role_description);
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeSetAccessKey
    (JNIEnv* env, jclass type, jlong node, jbyteArray text)
{
    SetString(env, node, text, accesskit_node_set_access_key_with_length, accesskit_node_clear_access_key);
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeSetKeyboardShortcut
    (JNIEnv* env, jclass type, jlong node, jbyteArray text)
{
    SetString(env, node, text, accesskit_node_set_keyboard_shortcut_with_length, accesskit_node_clear_keyboard_shortcut);
}

// AccessKit gives it to VoiceOver as the element's identifier (AccessKitRuntime.m).
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeSetAuthorId
    (JNIEnv* env, jclass type, jlong node, jbyteArray text)
{
    SetString(env, node, text, accesskit_node_set_author_id_with_length, accesskit_node_clear_author_id);
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeSetBounds
    (JNIEnv* env, jclass type, jlong node, jdouble x0, jdouble y0, jdouble x1, jdouble y1)
{
    struct accesskit_rect bounds = { .x0 = x0, .y0 = y0, .x1 = x1, .y1 = y1 };
    accesskit_node_set_bounds(NodeFrom(node), bounds);
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeAddChild
    (JNIEnv* env, jclass type, jlong node, jlong child)
{
    accesskit_node_push_child(NodeFrom(node), (accesskit_node_id)child);
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeClearChildren
    (JNIEnv* env, jclass type, jlong node)
{
    accesskit_node_clear_children(NodeFrom(node));
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeSetToggled
    (JNIEnv* env, jclass type, jlong node, jint value)
{
    accesskit_node_set_toggled(NodeFrom(node), (accesskit_toggled)value);
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeSetLive
    (JNIEnv* env, jclass type, jlong node, jint value)
{
    accesskit_node_set_live(NodeFrom(node), (accesskit_live)value);
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeSetTextDirection
    (JNIEnv* env, jclass type, jlong node, jint value)
{
    accesskit_node_set_text_direction(NodeFrom(node), (accesskit_text_direction)value);
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeSetNumericValue
    (JNIEnv* env, jclass type, jlong node, jdouble value)
{
    accesskit_node_set_numeric_value(NodeFrom(node), value);
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeSetMinNumericValue
    (JNIEnv* env, jclass type, jlong node, jdouble value)
{
    accesskit_node_set_min_numeric_value(NodeFrom(node), value);
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeSetMaxNumericValue
    (JNIEnv* env, jclass type, jlong node, jdouble value)
{
    accesskit_node_set_max_numeric_value(NodeFrom(node), value);
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeSetNumericValueStep
    (JNIEnv* env, jclass type, jlong node, jdouble value)
{
    accesskit_node_set_numeric_value_step(NodeFrom(node), value);
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeSetNumericValueJump
    (JNIEnv* env, jclass type, jlong node, jdouble value)
{
    accesskit_node_set_numeric_value_jump(NodeFrom(node), value);
}

// Character indexes count entries of the node's character lengths.
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeSetTextSelection
    (JNIEnv* env, jclass type, jlong node, jlong anchorNode, jint anchorIndex, jlong focusNode, jint focusIndex)
{
    struct accesskit_text_selection selection;
    selection.anchor.node = (accesskit_node_id)anchorNode;
    selection.anchor.character_index = ToSize(anchorIndex);
    selection.focus.node = (accesskit_node_id)focusNode;
    selection.focus.character_index = ToSize(focusIndex);
    accesskit_node_set_text_selection(NodeFrom(node), selection);
}

// Each character's length in UTF-8 bytes.
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeSetCharacterLengths
    (JNIEnv* env, jclass type, jlong node, jbyteArray lengths)
{
    SetBytes(env, node, lengths, accesskit_node_set_character_lengths, accesskit_node_clear_character_lengths);
}

// The character index where each word starts, sorted.
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeSetWordStarts
    (JNIEnv* env, jclass type, jlong node, jbyteArray starts)
{
    SetBytes(env, node, starts, accesskit_node_set_word_starts, accesskit_node_clear_word_starts);
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeSetCharacterPositions
    (JNIEnv* env, jclass type, jlong node, jfloatArray positions)
{
    SetFloats(env, node, positions, accesskit_node_set_character_positions, accesskit_node_clear_character_positions);
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeSetCharacterWidths
    (JNIEnv* env, jclass type, jlong node, jfloatArray widths)
{
    SetFloats(env, node, widths, accesskit_node_set_character_widths, accesskit_node_clear_character_widths);
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeSetPositionInSet
    (JNIEnv* env, jclass type, jlong node, jint value)
{
    accesskit_node_set_position_in_set(NodeFrom(node), ToSize(value));
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeSetSizeOfSet
    (JNIEnv* env, jclass type, jlong node, jint value)
{
    accesskit_node_set_size_of_set(NodeFrom(node), ToSize(value));
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeSetLevel
    (JNIEnv* env, jclass type, jlong node, jint value)
{
    accesskit_node_set_level(NodeFrom(node), ToSize(value));
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeSetExpanded
    (JNIEnv* env, jclass type, jlong node, jboolean value)
{
    accesskit_node_set_expanded(NodeFrom(node), value == JNI_TRUE);
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_Node_nativeSetSelected
    (JNIEnv* env, jclass type, jlong node, jboolean value)
{
    accesskit_node_set_selected(NodeFrom(node), value == JNI_TRUE);
}

// ---------------------------------------------------------------- TreeUpdate

JNIEXPORT jlong JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_TreeUpdate_nativeWithFocus
    (JNIEnv* env, jclass type, jlong focus)
{
    return ToJava(accesskit_tree_update_with_focus((accesskit_node_id)focus));
}

// Only for an update that was never handed to AccessKit (TreeUpdate.drop).
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_TreeUpdate_nativeDrop
    (JNIEnv* env, jclass type, jlong update)
{
    accesskit_tree_update_free(UpdateFrom(update));
}

// The update takes the node; Java forgets it (TreeUpdate.add).
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_TreeUpdate_nativeAddNode
    (JNIEnv* env, jclass type, jlong update, jlong id, jlong node)
{
    accesskit_tree_update_push_node(UpdateFrom(update), (accesskit_node_id)id, NodeFrom(node));
}

// The update takes the new tree info, so nothing here frees it.
JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_TreeUpdate_nativeSetTree
    (JNIEnv* env, jclass type, jlong update, jlong root)
{
    accesskit_tree_update_set_tree_info(UpdateFrom(update), accesskit_tree_info_new((accesskit_node_id)root));
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_TreeUpdate_nativeClearTree
    (JNIEnv* env, jclass type, jlong update)
{
    accesskit_tree_update_clear_tree_info(UpdateFrom(update));
}

JNIEXPORT void JNICALL Java_plugins_quorum_Libraries_Interface_Accessibility_AccessKitBinding_TreeUpdate_nativeSetFocus
    (JNIEnv* env, jclass type, jlong update, jlong focus)
{
    accesskit_tree_update_set_focus(UpdateFrom(update), (accesskit_node_id)focus);
}
