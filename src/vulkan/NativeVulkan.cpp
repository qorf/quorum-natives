// Helpers shared by the rest of src/vulkan, and NativeVulkan's own native
// method (wrapping memory C owns in a Java buffer).

#include "NativeVulkan.h"

#include <cstdarg>
#include <cstdio>

#ifdef __ANDROID__
#include <android/log.h>
#endif

void NativeVulkanLog(const char* format, ...)
{
    va_list arguments;
    va_start(arguments, format);
#ifdef __ANDROID__
    __android_log_vprint(ANDROID_LOG_INFO, "Quorum Vulkan", format, arguments);
#else
    fprintf(stderr, "Quorum Vulkan: ");
    vfprintf(stderr, format, arguments);
    fprintf(stderr, "\n");
#endif
    va_end(arguments);
}

void NativeVulkanThrow(JNIEnv* env, const std::string& message)
{
    jclass exception = env->FindClass("java/lang/RuntimeException");
    if (exception != nullptr)
        env->ThrowNew(exception, message.c_str());
}

const char* NativeVulkanResultName(VkResult result)
{
    switch (result)
    {
        case VK_SUCCESS: return "VK_SUCCESS";
        case VK_NOT_READY: return "VK_NOT_READY";
        case VK_TIMEOUT: return "VK_TIMEOUT";
        case VK_INCOMPLETE: return "VK_INCOMPLETE";
        case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
        default: return "an unnamed result";
    }
}

std::vector<std::string> NativeVulkanStrings(JNIEnv* env, jobjectArray array)
{
    std::vector<std::string> result;
    if (array == nullptr)
        return result;

    jsize size = env->GetArrayLength(array);
    for (jsize i = 0; i < size; i++)
    {
        jstring value = static_cast<jstring>(env->GetObjectArrayElement(array, i));
        if (value == nullptr)
            continue;
        const char* characters = env->GetStringUTFChars(value, nullptr);
        result.push_back(characters);
        env->ReleaseStringUTFChars(value, characters);
        env->DeleteLocalRef(value);
    }
    return result;
}

std::vector<const char*> NativeVulkanPointers(const std::vector<std::string>& strings)
{
    std::vector<const char*> result;
    for (const std::string& value : strings)
        result.push_back(value.c_str());
    return result;
}

bool NativeVulkanContains(const std::vector<std::string>& strings, const char* value)
{
    for (const std::string& candidate : strings)
    {
        if (candidate == value)
            return true;
    }
    return false;
}

std::vector<jint> NativeVulkanInts(JNIEnv* env, jintArray array)
{
    std::vector<jint> result;
    if (array == nullptr)
        return result;
    result.resize(env->GetArrayLength(array));
    if (!result.empty())
        env->GetIntArrayRegion(array, 0, static_cast<jsize>(result.size()), result.data());
    return result;
}

std::vector<jlong> NativeVulkanLongs(JNIEnv* env, jlongArray array)
{
    std::vector<jlong> result;
    if (array == nullptr)
        return result;
    result.resize(env->GetArrayLength(array));
    if (!result.empty())
        env->GetLongArrayRegion(array, 0, static_cast<jsize>(result.size()), result.data());
    return result;
}

std::vector<jfloat> NativeVulkanFloats(JNIEnv* env, jfloatArray array)
{
    std::vector<jfloat> result;
    if (array == nullptr)
        return result;
    result.resize(env->GetArrayLength(array));
    if (!result.empty())
        env->GetFloatArrayRegion(array, 0, static_cast<jsize>(result.size()), result.data());
    return result;
}

std::vector<jboolean> NativeVulkanBooleans(JNIEnv* env, jbooleanArray array)
{
    std::vector<jboolean> result;
    if (array == nullptr)
        return result;
    result.resize(env->GetArrayLength(array));
    if (!result.empty())
        env->GetBooleanArrayRegion(array, 0, static_cast<jsize>(result.size()), result.data());
    return result;
}

jintArray NativeVulkanIntArray(JNIEnv* env, const std::vector<jint>& values)
{
    jintArray array = env->NewIntArray(static_cast<jsize>(values.size()));
    if (array != nullptr && !values.empty())
        env->SetIntArrayRegion(array, 0, static_cast<jsize>(values.size()), values.data());
    return array;
}

jlongArray NativeVulkanLongArray(JNIEnv* env, const std::vector<jlong>& values)
{
    jlongArray array = env->NewLongArray(static_cast<jsize>(values.size()));
    if (array != nullptr && !values.empty())
        env->SetLongArrayRegion(array, 0, static_cast<jsize>(values.size()), values.data());
    return array;
}

jobjectArray NativeVulkanStringArray(JNIEnv* env, const std::vector<std::string>& values)
{
    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray array = env->NewObjectArray(static_cast<jsize>(values.size()), stringClass, nullptr);
    for (size_t i = 0; i < values.size(); i++)
    {
        jstring value = env->NewStringUTF(values[i].c_str());
        env->SetObjectArrayElement(array, static_cast<jsize>(i), value);
        env->DeleteLocalRef(value);
    }
    return array;
}

// NativeVulkan.WrapNative: a direct buffer over memory C owns, such as mapped
// GPU memory. Java sets its byte order.
extern "C" JNIEXPORT jobject JNICALL NATIVE_VULKAN(NativeVulkan, WrapNative)(JNIEnv* env, jclass, jlong address, jlong size)
{
    return env->NewDirectByteBuffer(FromJava<void*>(address), size);
}
