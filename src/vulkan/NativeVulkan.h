// Quorum's own Vulkan layer for iOS and Android: the C behind the Java classes
// in plugins.quorum.Libraries.Game.Graphics.Vulkan.Native (quorum-language's
// QuorumPlugins). The desktop uses LWJGL instead.
//
// It calls Vulkan directly: MoltenVK, merged into the iOS archive, or the
// phone's own libvulkan.so on Android. Handles go to Java as longs.

#pragma once

#include <jni.h>
#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <vector>

// The JNI name of a native method of a class in the Native package.
#define NATIVE_VULKAN(Class, Method) Java_plugins_quorum_Libraries_Game_Graphics_Vulkan_Native_##Class##_##Method

// A Vulkan handle (or pointer) as a Java long, and back. Quorum's iOS and
// Android builds are 64-bit, where every Vulkan handle is a pointer.
template <typename T> inline T FromJava(jlong value)
{
    return reinterpret_cast<T>(static_cast<uintptr_t>(value));
}

template <typename T> inline jlong ToJava(T value)
{
    return static_cast<jlong>(reinterpret_cast<uintptr_t>(value));
}

// Writes a line to the system log: logcat ("Quorum Vulkan") on Android, the
// console on iOS.
void NativeVulkanLog(const char* format, ...);

// Throws a java.lang.RuntimeException with the message.
void NativeVulkanThrow(JNIEnv* env, const std::string& message);

// Vulkan's name for a result, for messages.
const char* NativeVulkanResultName(VkResult result);

// The API version Quorum asked for when it made the instance (VMA uses it).
uint32_t NativeVulkanInstanceApiVersion();

// Java's strings, as C++ strings, and as the pointers Vulkan's structures take.
std::vector<std::string> NativeVulkanStrings(JNIEnv* env, jobjectArray array);
std::vector<const char*> NativeVulkanPointers(const std::vector<std::string>& strings);
bool NativeVulkanContains(const std::vector<std::string>& strings, const char* value);

// A Java array's values, copied into C++ for the length of a call.
std::vector<jint> NativeVulkanInts(JNIEnv* env, jintArray array);
std::vector<jlong> NativeVulkanLongs(JNIEnv* env, jlongArray array);
std::vector<jfloat> NativeVulkanFloats(JNIEnv* env, jfloatArray array);
std::vector<jboolean> NativeVulkanBooleans(JNIEnv* env, jbooleanArray array);

// C++ values as new Java arrays.
jintArray NativeVulkanIntArray(JNIEnv* env, const std::vector<jint>& values);
jlongArray NativeVulkanLongArray(JNIEnv* env, const std::vector<jlong>& values);
jobjectArray NativeVulkanStringArray(JNIEnv* env, const std::vector<std::string>& values);
