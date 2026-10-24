// The instance, the GPU (physical device) with its features and limits, the
// logical device, queues, the surface, the swapchain, and taking and showing
// its images. Each part does what the matching desktop strategy does with
// LWJGL (DesktopVulkanInstance and the rest), unless its comment says otherwise.

#include "NativeVulkan.h"
#include "NativeVulkanFeatureTable.h"
#include "NativeVulkanLimitTable.h"

#include <cstring>

#ifdef __ANDROID__
#include <android/native_window_jni.h>
#endif

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

namespace
{
    // MoltenVK's extension, which Vulkan's headers only name with the beta
    // extensions turned on.
    const char* const PORTABILITY_SUBSET = "VK_KHR_portability_subset";

    // The debug messenger, if the instance was made with the debug
    // utilities extension (validation layers are rarely on phones).
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    uint32_t instanceApiVersion = VK_API_VERSION_1_0;

    VKAPI_ATTR VkBool32 VKAPI_CALL LogDebugMessage(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT,
            const VkDebugUtilsMessengerCallbackDataEXT* data, void*)
    {
        const char* level = "DEBUG";
        if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            level = "ERROR";
        else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            level = "WARNING";
        else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
            level = "INFO";
        NativeVulkanLog("Vulkan Debug Message, %s: %s", level, data->pMessage);
        return VK_FALSE;
    }

    std::vector<VkExtensionProperties> InstanceExtensions()
    {
        uint32_t count = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
        std::vector<VkExtensionProperties> extensions(count);
        if (count > 0)
            vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data());
        return extensions;
    }

    std::vector<VkExtensionProperties> DeviceExtensions(VkPhysicalDevice device)
    {
        uint32_t count = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
        std::vector<VkExtensionProperties> extensions(count);
        if (count > 0)
            vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data());
        return extensions;
    }

    bool HasExtension(const std::vector<VkExtensionProperties>& extensions, const char* name)
    {
        for (const VkExtensionProperties& extension : extensions)
        {
            if (strcmp(extension.extensionName, name) == 0)
                return true;
        }
        return false;
    }

    // The desktop's order of device types: higher is better.
    int DeviceTypePriority(VkPhysicalDeviceType type)
    {
        switch (type)
        {
            case VK_PHYSICAL_DEVICE_TYPE_OTHER: return 0;
            case VK_PHYSICAL_DEVICE_TYPE_CPU: return 1;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return 2;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return 3;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return 4;
            default: return -1;
        }
    }

    // The device's features, from Vulkan 1.0 to 1.4 (1.4's only when the
    // device has 1.4).
    struct DeviceFeatures
    {
        VkPhysicalDeviceFeatures2 features2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        VkPhysicalDeviceVulkan11Features features11 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
        VkPhysicalDeviceVulkan12Features features12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
        VkPhysicalDeviceVulkan13Features features13 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
        VkPhysicalDeviceVulkan14Features features14 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES};

        explicit DeviceFeatures(VkPhysicalDevice device)
        {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(device, &properties);

            features2.pNext = &features11;
            features11.pNext = &features12;
            features12.pNext = &features13;
            if (properties.apiVersion >= VK_API_VERSION_1_4)
                features13.pNext = &features14;
            vkGetPhysicalDeviceFeatures2(device, &features2);
        }

        // Where a feature of NativeVulkanFeatureTable.h is.
        VkBool32* Feature(const NativeVulkanFeature& feature)
        {
            unsigned char* structure = nullptr;
            switch (feature.structure)
            {
                case 0: structure = reinterpret_cast<unsigned char*>(&features2.features); break;
                case 1: structure = reinterpret_cast<unsigned char*>(&features11); break;
                case 2: structure = reinterpret_cast<unsigned char*>(&features12); break;
                case 3: structure = reinterpret_cast<unsigned char*>(&features13); break;
                default: structure = reinterpret_cast<unsigned char*>(&features14); break;
            }
            return reinterpret_cast<VkBool32*>(structure + feature.offset);
        }
    };
}

uint32_t NativeVulkanInstanceApiVersion()
{
    return instanceApiVersion;
}

/*
The instance. Portability enumeration is turned on when Vulkan offers it
(MoltenVK does), as Quorum asks for it on the Mac.
*/
extern "C" JNIEXPORT jlong JNICALL NATIVE_VULKAN(NativeVulkanInstance, CreateInstance)(JNIEnv* env, jclass,
        jstring applicationName, jint applicationVersion, jstring engineName, jint engineVersion, jint apiVersion,
        jobjectArray layerArray, jobjectArray extensionArray)
{
    const char* application = env->GetStringUTFChars(applicationName, nullptr);
    const char* engine = env->GetStringUTFChars(engineName, nullptr);

    std::vector<std::string> layers = NativeVulkanStrings(env, layerArray);
    std::vector<std::string> extensions = NativeVulkanStrings(env, extensionArray);

    bool portability = NativeVulkanContains(extensions, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    if (!portability && HasExtension(InstanceExtensions(), VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
    {
        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        portability = true;
    }
    bool debug = NativeVulkanContains(extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

#if defined(__APPLE__) && TARGET_OS_SIMULATOR
    // The iOS simulator can't nest Metal argument buffers, which MoltenVK uses
    // for descriptor sets, so pipelines fail to compile there. MoltenVK's
    // MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS turns them off (through
    // VK_EXT_layer_settings, layer "MoltenVK"); MoltenVK then reports the
    // smaller binding limits, which Quorum sizes its descriptor sets from.
    // Real iPhones keep argument buffers.
    if (!NativeVulkanContains(extensions, VK_EXT_LAYER_SETTINGS_EXTENSION_NAME)
            && HasExtension(InstanceExtensions(), VK_EXT_LAYER_SETTINGS_EXTENSION_NAME))
        extensions.push_back(VK_EXT_LAYER_SETTINGS_EXTENSION_NAME);
    VkBool32 useArgumentBuffers = VK_FALSE;
    VkLayerSettingEXT argumentBufferSetting = {"MoltenVK", "MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS",
            VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &useArgumentBuffers};
    VkLayerSettingsCreateInfoEXT layerSettings = {VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT};
    layerSettings.settingCount = 1;
    layerSettings.pSettings = &argumentBufferSetting;
#endif

    std::vector<const char*> layerPointers = NativeVulkanPointers(layers);
    std::vector<const char*> extensionPointers = NativeVulkanPointers(extensions);

    VkApplicationInfo applicationInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    applicationInfo.pApplicationName = application;
    applicationInfo.applicationVersion = static_cast<uint32_t>(applicationVersion);
    applicationInfo.pEngineName = engine;
    applicationInfo.engineVersion = static_cast<uint32_t>(engineVersion);
    applicationInfo.apiVersion = static_cast<uint32_t>(apiVersion);

    VkDebugUtilsMessengerCreateInfoEXT debugInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugInfo.pfnUserCallback = LogDebugMessage;

    VkInstanceCreateInfo createInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    createInfo.pNext = debug ? &debugInfo : nullptr;
    createInfo.pApplicationInfo = &applicationInfo;
    createInfo.enabledLayerCount = static_cast<uint32_t>(layerPointers.size());
    createInfo.ppEnabledLayerNames = layerPointers.empty() ? nullptr : layerPointers.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensionPointers.size());
    createInfo.ppEnabledExtensionNames = extensionPointers.empty() ? nullptr : extensionPointers.data();
    if (portability)
        createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#if defined(__APPLE__) && TARGET_OS_SIMULATOR
    layerSettings.pNext = createInfo.pNext;
    createInfo.pNext = &layerSettings;
#endif

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

    env->ReleaseStringUTFChars(applicationName, application);
    env->ReleaseStringUTFChars(engineName, engine);

    if (result != VK_SUCCESS)
    {
        NativeVulkanLog("vkCreateInstance failed: %s", NativeVulkanResultName(result));
        return 0;
    }
    instanceApiVersion = static_cast<uint32_t>(apiVersion);

    if (debug)
    {
        PFN_vkCreateDebugUtilsMessengerEXT create = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
        if (create == nullptr || create(instance, &debugInfo, nullptr, &debugMessenger) != VK_SUCCESS)
            debugMessenger = VK_NULL_HANDLE;
    }
    return ToJava(instance);
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanInstance, DestroyInstance)(JNIEnv*, jclass, jlong instanceHandle)
{
    VkInstance instance = FromJava<VkInstance>(instanceHandle);
    if (debugMessenger != VK_NULL_HANDLE)
    {
        PFN_vkDestroyDebugUtilsMessengerEXT destroy = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (destroy != nullptr)
            destroy(instance, debugMessenger, nullptr);
        debugMessenger = VK_NULL_HANDLE;
    }
    vkDestroyInstance(instance, nullptr);
}

extern "C" JNIEXPORT jobjectArray JNICALL NATIVE_VULKAN(NativeVulkanUtilities, GetInstanceLayers)(JNIEnv* env, jclass)
{
    uint32_t count = 0;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> layers(count);
    if (count > 0)
        vkEnumerateInstanceLayerProperties(&count, layers.data());

    std::vector<std::string> names;
    for (const VkLayerProperties& layer : layers)
        names.push_back(layer.layerName);
    return NativeVulkanStringArray(env, names);
}

extern "C" JNIEXPORT jobjectArray JNICALL NATIVE_VULKAN(NativeVulkanUtilities, GetInstanceExtensions)(JNIEnv* env, jclass)
{
    std::vector<std::string> names;
    for (const VkExtensionProperties& extension : InstanceExtensions())
        names.push_back(extension.extensionName);
    return NativeVulkanStringArray(env, names);
}

/*
Chooses the GPU as DesktopVulkanPhysicalDevice does: one with queue families
for every requested queue feature and with every requested extension, the
best type first, then the most device memory. It also writes the chosen GPU's
name, Vulkan version and the features Quorum's renderer uses to the log, to
help when a device can't run Quorum.
*/
extern "C" JNIEXPORT jlong JNICALL NATIVE_VULKAN(NativeVulkanPhysicalDevice, SelectPhysicalDevice)(JNIEnv* env, jclass,
        jlong instanceHandle, jint featuresBitmask, jobjectArray extensionArray)
{
    VkInstance instance = FromJava<VkInstance>(instanceHandle);
    std::vector<std::string> requested = NativeVulkanStrings(env, extensionArray);

    uint32_t count = 0;
    if (vkEnumeratePhysicalDevices(instance, &count, nullptr) != VK_SUCCESS || count == 0)
    {
        NativeVulkanLog("No Vulkan devices were found.");
        return 0;
    }
    std::vector<VkPhysicalDevice> devices(count);
    if (vkEnumeratePhysicalDevices(instance, &count, devices.data()) != VK_SUCCESS)
        return 0;

    VkPhysicalDevice chosen = VK_NULL_HANDLE;
    int chosenPriority = -1;
    VkDeviceSize chosenMemory = 0;
    for (VkPhysicalDevice device : devices)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);

        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, families.data());

        bool supportsFeatures = featuresBitmask == 0;
        uint32_t unsatisfied = static_cast<uint32_t>(featuresBitmask);
        for (const VkQueueFamilyProperties& family : families)
        {
            unsatisfied &= ~(family.queueFlags & unsatisfied);
            if (unsatisfied == 0)
                supportsFeatures = true;
        }
        if (!supportsFeatures)
        {
            NativeVulkanLog("%s doesn't have the queues Quorum needs.", properties.deviceName);
            continue;
        }

        std::vector<VkExtensionProperties> extensions = DeviceExtensions(device);
        bool hasExtensions = true;
        for (const std::string& name : requested)
        {
            if (!HasExtension(extensions, name.c_str()))
            {
                NativeVulkanLog("%s doesn't have the extension %s.", properties.deviceName, name.c_str());
                hasExtensions = false;
            }
        }
        if (!hasExtensions)
            continue;

        VkPhysicalDeviceMemoryProperties memory;
        vkGetPhysicalDeviceMemoryProperties(device, &memory);
        VkDeviceSize memorySize = 0;
        for (uint32_t i = 0; i < memory.memoryHeapCount; i++)
        {
            if ((memory.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0 && memory.memoryHeaps[i].size > memorySize)
                memorySize = memory.memoryHeaps[i].size;
        }

        int priority = DeviceTypePriority(properties.deviceType);
        if (chosen == VK_NULL_HANDLE || priority > chosenPriority || (priority == chosenPriority && memorySize > chosenMemory))
        {
            chosen = device;
            chosenPriority = priority;
            chosenMemory = memorySize;
        }
    }

    if (chosen == VK_NULL_HANDLE)
        return 0;

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(chosen, &properties);
    DeviceFeatures features(chosen);
    NativeVulkanLog("Using %s, Vulkan %u.%u.%u.", properties.deviceName, VK_API_VERSION_MAJOR(properties.apiVersion),
            VK_API_VERSION_MINOR(properties.apiVersion), VK_API_VERSION_PATCH(properties.apiVersion));
    NativeVulkanLog("Features Quorum uses: buffer device address %d, scalar block layout %d, shader output layer %d, "
            "64-bit integers %d, multi-draw indirect %d, first instance %d, robust buffer access %d, descriptor indexing %d, "
            "geometry shaders %d; push constants %u bytes, %u bound descriptor sets.",
            features.features12.bufferDeviceAddress, features.features12.scalarBlockLayout, features.features12.shaderOutputLayer,
            features.features2.features.shaderInt64, features.features2.features.multiDrawIndirect,
            features.features2.features.drawIndirectFirstInstance, features.features2.features.robustBufferAccess,
            features.features12.descriptorIndexing, features.features2.features.geometryShader,
            properties.limits.maxPushConstantsSize, properties.limits.maxBoundDescriptorSets);
    return ToJava(chosen);
}

// Pairs of queue count and queue flags, one pair per family.
extern "C" JNIEXPORT jintArray JNICALL NATIVE_VULKAN(NativeVulkanPhysicalDevice, GetQueueFamilies)(JNIEnv* env, jclass, jlong deviceHandle)
{
    VkPhysicalDevice device = FromJava<VkPhysicalDevice>(deviceHandle);
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

    std::vector<jint> values;
    for (const VkQueueFamilyProperties& family : families)
    {
        values.push_back(static_cast<jint>(family.queueCount));
        values.push_back(static_cast<jint>(family.queueFlags));
    }
    return NativeVulkanIntArray(env, values);
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanPhysicalDevice, FillLimits)(JNIEnv* env, jclass, jlong deviceHandle,
        jlongArray integerArray, jdoubleArray numberArray)
{
    if (env->GetArrayLength(integerArray) != NATIVE_VULKAN_LIMIT_INTEGERS || env->GetArrayLength(numberArray) != NATIVE_VULKAN_LIMIT_NUMBERS)
    {
        NativeVulkanLog("The limit arrays don't match NativeVulkanLimitTable.h; regenerate it with NativeVulkanPhysicalDeviceLimits.");
        return;
    }

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(FromJava<VkPhysicalDevice>(deviceHandle), &properties);
    jlong integers[NATIVE_VULKAN_LIMIT_INTEGERS];
    jdouble numbers[NATIVE_VULKAN_LIMIT_NUMBERS];
    NativeVulkanFillLimits(properties.limits, integers, numbers);
    env->SetLongArrayRegion(integerArray, 0, NATIVE_VULKAN_LIMIT_INTEGERS, integers);
    env->SetDoubleArrayRegion(numberArray, 0, NATIVE_VULKAN_LIMIT_NUMBERS, numbers);
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanPhysicalDeviceFeatures, GetFeatures)(JNIEnv* env, jclass, jlong deviceHandle,
        jbooleanArray featureArray)
{
    if (env->GetArrayLength(featureArray) != NATIVE_VULKAN_FEATURE_COUNT)
    {
        NativeVulkanLog("The feature array doesn't match NativeVulkanFeatureTable.h; regenerate it with NativeVulkanPhysicalDeviceFeatures.");
        return;
    }

    DeviceFeatures features(FromJava<VkPhysicalDevice>(deviceHandle));
    jboolean values[NATIVE_VULKAN_FEATURE_COUNT];
    for (int i = 0; i < NATIVE_VULKAN_FEATURE_COUNT; i++)
        values[i] = *features.Feature(NATIVE_VULKAN_FEATURES[i]) ? JNI_TRUE : JNI_FALSE;
    env->SetBooleanArrayRegion(featureArray, 0, NATIVE_VULKAN_FEATURE_COUNT, values);
}

/*
The logical device, as DesktopVulkanDevice makes it: every queue of every
family (priority 0), the Vulkan 1.0 features Quorum's features object holds,
plus the ones the renderer needs. One difference: the desktop turns those on
whether or not the GPU has them, and here only the ones the GPU has are
turned on (the missing ones are logged), so a phone without, say, output
layers still gets a device, and Quorum's renderer, which checks the features,
uses its other paths. VK_KHR_portability_subset is turned on when the GPU has
it (MoltenVK), as Vulkan requires.
*/
extern "C" JNIEXPORT jlong JNICALL NATIVE_VULKAN(NativeVulkanDevice, CreateDevice)(JNIEnv* env, jclass, jlong physicalHandle,
        jobjectArray extensionArray, jintArray queueCountArray, jbooleanArray baseFeatureArray, jboolean descriptorIndexing)
{
    VkPhysicalDevice physicalDevice = FromJava<VkPhysicalDevice>(physicalHandle);
    std::vector<std::string> extensions = NativeVulkanStrings(env, extensionArray);
    std::vector<jint> queueCounts = NativeVulkanInts(env, queueCountArray);
    std::vector<jboolean> baseFeatures = NativeVulkanBooleans(env, baseFeatureArray);

    // Requested extensions the GPU doesn't have are left out (and logged), as
    // missing features are. VulkanDevice asks for VK_EXT_descriptor_indexing,
    // which the iOS simulator's GPU doesn't list; it's core since Vulkan 1.2,
    // and turned on below through the 1.2 features when the GPU has it.
    std::vector<VkExtensionProperties> available = DeviceExtensions(physicalDevice);
    std::vector<std::string> kept;
    for (const std::string& name : extensions)
    {
        if (HasExtension(available, name.c_str()))
            kept.push_back(name);
        else
            NativeVulkanLog("This GPU doesn't have the extension %s, so it is left out.", name.c_str());
    }
    extensions = kept;

    if (!NativeVulkanContains(extensions, PORTABILITY_SUBSET) && HasExtension(available, PORTABILITY_SUBSET))
        extensions.push_back(PORTABILITY_SUBSET);
    std::vector<const char*> extensionPointers = NativeVulkanPointers(extensions);

    std::vector<std::vector<float>> priorities(queueCounts.size());
    std::vector<VkDeviceQueueCreateInfo> queueInfos(queueCounts.size());
    for (size_t i = 0; i < queueCounts.size(); i++)
    {
        priorities[i].assign(static_cast<size_t>(queueCounts[i]), 0.0f);
        queueInfos[i] = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queueInfos[i].queueFamilyIndex = static_cast<uint32_t>(i);
        queueInfos[i].queueCount = static_cast<uint32_t>(queueCounts[i]);
        queueInfos[i].pQueuePriorities = priorities[i].data();
    }

    DeviceFeatures supported(physicalDevice);

    VkPhysicalDeviceFeatures2 features2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    VkPhysicalDeviceVulkan12Features features12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features2.pNext = &features12;

    // The features object's Vulkan 1.0 features, in NativeVulkanFeatureTable.h's order.
    for (int i = 0; i < NATIVE_VULKAN_BASE_FEATURE_COUNT && i < static_cast<int>(baseFeatures.size()); i++)
    {
        VkBool32* feature = reinterpret_cast<VkBool32*>(reinterpret_cast<unsigned char*>(&features2.features) + NATIVE_VULKAN_FEATURES[i].offset);
        *feature = baseFeatures[i] ? VK_TRUE : VK_FALSE;
    }

    std::string missing;
    auto require = [&missing](VkBool32& enabled, VkBool32 available, const char* name)
    {
        enabled = available;
        if (!available)
            missing += std::string(" ") + name;
    };
    VkPhysicalDeviceFeatures& features = features2.features;
    const VkPhysicalDeviceFeatures& has = supported.features2.features;
    require(features.shaderInt64, has.shaderInt64, "shaderInt64");
    require(features.multiDrawIndirect, has.multiDrawIndirect, "multiDrawIndirect");
    require(features.drawIndirectFirstInstance, has.drawIndirectFirstInstance, "drawIndirectFirstInstance");
    require(features.robustBufferAccess, has.robustBufferAccess, "robustBufferAccess");
    if (has.samplerAnisotropy)
        features.samplerAnisotropy = VK_TRUE;

    const VkPhysicalDeviceVulkan12Features& has12 = supported.features12;
    require(features12.bufferDeviceAddress, has12.bufferDeviceAddress, "bufferDeviceAddress");
    require(features12.scalarBlockLayout, has12.scalarBlockLayout, "scalarBlockLayout");
    require(features12.shaderOutputLayer, has12.shaderOutputLayer, "shaderOutputLayer");
    if (descriptorIndexing)
    {
        require(features12.descriptorBindingPartiallyBound, has12.descriptorBindingPartiallyBound, "descriptorBindingPartiallyBound");
        require(features12.descriptorBindingSampledImageUpdateAfterBind, has12.descriptorBindingSampledImageUpdateAfterBind,
                "descriptorBindingSampledImageUpdateAfterBind");
        require(features12.descriptorBindingUpdateUnusedWhilePending, has12.descriptorBindingUpdateUnusedWhilePending,
                "descriptorBindingUpdateUnusedWhilePending");
        require(features12.descriptorBindingVariableDescriptorCount, has12.descriptorBindingVariableDescriptorCount,
                "descriptorBindingVariableDescriptorCount");
        require(features12.descriptorIndexing, has12.descriptorIndexing, "descriptorIndexing");
        require(features12.runtimeDescriptorArray, has12.runtimeDescriptorArray, "runtimeDescriptorArray");
    }
    if (!missing.empty())
        NativeVulkanLog("This GPU doesn't have these features Quorum's renderer uses:%s", missing.c_str());

    VkDeviceCreateInfo createInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    createInfo.pNext = &features2;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
    createInfo.pQueueCreateInfos = queueInfos.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensionPointers.size());
    createInfo.ppEnabledExtensionNames = extensionPointers.empty() ? nullptr : extensionPointers.data();

    VkDevice device = VK_NULL_HANDLE;
    VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
    if (result != VK_SUCCESS)
    {
        NativeVulkanLog("vkCreateDevice failed: %s", NativeVulkanResultName(result));
        return 0;
    }
    return ToJava(device);
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanDevice, WaitForDevice)(JNIEnv*, jclass, jlong device)
{
    vkDeviceWaitIdle(FromJava<VkDevice>(device));
}

extern "C" JNIEXPORT jboolean JNICALL NATIVE_VULKAN(NativeVulkanQueueFamily, CanPresent)(JNIEnv*, jclass, jlong physicalDevice,
        jint familyIndex, jlong surface)
{
    VkBool32 supported = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(FromJava<VkPhysicalDevice>(physicalDevice), static_cast<uint32_t>(familyIndex),
            FromJava<VkSurfaceKHR>(surface), &supported);
    return supported ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jlong JNICALL NATIVE_VULKAN(NativeVulkanQueue, GetQueue)(JNIEnv*, jclass, jlong device, jint familyIndex, jint queueIndex)
{
    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(FromJava<VkDevice>(device), static_cast<uint32_t>(familyIndex), static_cast<uint32_t>(queueIndex), &queue);
    return ToJava(queue);
}

extern "C" JNIEXPORT jboolean JNICALL NATIVE_VULKAN(NativeVulkanQueue, Submit)(JNIEnv* env, jclass, jlong queue, jlongArray commandBufferArray,
        jlongArray waitArray, jintArray maskArray, jlongArray signalArray, jlong fence)
{
    std::vector<jlong> commandBufferHandles = NativeVulkanLongs(env, commandBufferArray);
    std::vector<jlong> waitHandles = NativeVulkanLongs(env, waitArray);
    std::vector<jint> masks = NativeVulkanInts(env, maskArray);
    std::vector<jlong> signalHandles = NativeVulkanLongs(env, signalArray);

    std::vector<VkCommandBuffer> commandBuffers;
    for (jlong handle : commandBufferHandles)
        commandBuffers.push_back(FromJava<VkCommandBuffer>(handle));
    std::vector<VkSemaphore> waits;
    std::vector<VkPipelineStageFlags> waitStages;
    for (size_t i = 0; i < waitHandles.size(); i++)
    {
        waits.push_back(FromJava<VkSemaphore>(waitHandles[i]));
        waitStages.push_back(i < masks.size() ? static_cast<VkPipelineStageFlags>(masks[i]) : VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    }
    std::vector<VkSemaphore> signals;
    for (jlong handle : signalHandles)
        signals.push_back(FromJava<VkSemaphore>(handle));

    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
    submitInfo.pCommandBuffers = commandBuffers.data();
    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waits.size());
    submitInfo.pWaitSemaphores = waits.empty() ? nullptr : waits.data();
    submitInfo.pWaitDstStageMask = waitStages.empty() ? nullptr : waitStages.data();
    submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signals.size());
    submitInfo.pSignalSemaphores = signals.empty() ? nullptr : signals.data();

    VkResult result = vkQueueSubmit(FromJava<VkQueue>(queue), 1, &submitInfo, FromJava<VkFence>(fence));
    if (result != VK_SUCCESS)
        NativeVulkanLog("vkQueueSubmit failed: %s", NativeVulkanResultName(result));
    return result == VK_SUCCESS ? JNI_TRUE : JNI_FALSE;
}

// iOS: a surface on the view's CAMetalLayer.
extern "C" JNIEXPORT jlong JNICALL NATIVE_VULKAN(NativeVulkanSurface, CreateMetalSurface)(JNIEnv*, jclass, jlong instance, jlong layer)
{
#ifdef VK_USE_PLATFORM_METAL_EXT
    VkMetalSurfaceCreateInfoEXT createInfo = {VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT};
    createInfo.pLayer = FromJava<const CAMetalLayer*>(layer);
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult result = vkCreateMetalSurfaceEXT(FromJava<VkInstance>(instance), &createInfo, nullptr, &surface);
    if (result != VK_SUCCESS)
    {
        NativeVulkanLog("vkCreateMetalSurfaceEXT failed: %s", NativeVulkanResultName(result));
        return 0;
    }
    return ToJava(surface);
#else
    NativeVulkanLog("Metal surfaces are only on iOS.");
    return 0;
#endif
}

// Android: a surface on the SurfaceView's window. Vulkan keeps its own
// reference to the window, so this one is released.
extern "C" JNIEXPORT jlong JNICALL NATIVE_VULKAN(NativeVulkanSurface, CreateAndroidSurface)(JNIEnv* env, jclass, jlong instance, jobject javaSurface)
{
#ifdef __ANDROID__
    if (javaSurface == nullptr)
    {
        NativeVulkanLog("There's no Android surface to draw on.");
        return 0;
    }
    ANativeWindow* window = ANativeWindow_fromSurface(env, javaSurface);
    if (window == nullptr)
        return 0;

    VkAndroidSurfaceCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR};
    createInfo.window = window;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult result = vkCreateAndroidSurfaceKHR(FromJava<VkInstance>(instance), &createInfo, nullptr, &surface);
    ANativeWindow_release(window);
    if (result != VK_SUCCESS)
    {
        NativeVulkanLog("vkCreateAndroidSurfaceKHR failed: %s", NativeVulkanResultName(result));
        return 0;
    }
    return ToJava(surface);
#else
    (void)env;
    (void)instance;
    (void)javaSurface;
    NativeVulkanLog("Android surfaces are only on Android.");
    return 0;
#endif
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanSurface, DestroySurface)(JNIEnv*, jclass, jlong instance, jlong surface)
{
    vkDestroySurfaceKHR(FromJava<VkInstance>(instance), FromJava<VkSurfaceKHR>(surface), nullptr);
}

// Minimum and maximum image counts, current width, height and transform, and
// the composite alphas offered; null if Vulkan failed.
extern "C" JNIEXPORT jintArray JNICALL NATIVE_VULKAN(NativeVulkanSwapchain, GetSurfaceCapabilities)(JNIEnv* env, jclass, jlong physicalDevice,
        jlong surface)
{
    VkSurfaceCapabilitiesKHR capabilities;
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(FromJava<VkPhysicalDevice>(physicalDevice), FromJava<VkSurfaceKHR>(surface),
            &capabilities);
    if (result != VK_SUCCESS)
    {
        NativeVulkanLog("vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed: %s", NativeVulkanResultName(result));
        return nullptr;
    }

    std::vector<jint> values = {
        static_cast<jint>(capabilities.minImageCount), static_cast<jint>(capabilities.maxImageCount),
        static_cast<jint>(capabilities.currentExtent.width), static_cast<jint>(capabilities.currentExtent.height),
        static_cast<jint>(capabilities.currentTransform), static_cast<jint>(capabilities.supportedCompositeAlpha)
    };
    return NativeVulkanIntArray(env, values);
}

// Pairs of format and color space.
extern "C" JNIEXPORT jintArray JNICALL NATIVE_VULKAN(NativeVulkanSwapchain, GetSurfaceFormats)(JNIEnv* env, jclass, jlong physicalDevice,
        jlong surface)
{
    uint32_t count = 0;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(FromJava<VkPhysicalDevice>(physicalDevice), FromJava<VkSurfaceKHR>(surface), &count, nullptr) != VK_SUCCESS)
        return nullptr;
    std::vector<VkSurfaceFormatKHR> formats(count);
    if (count > 0 && vkGetPhysicalDeviceSurfaceFormatsKHR(FromJava<VkPhysicalDevice>(physicalDevice), FromJava<VkSurfaceKHR>(surface), &count,
            formats.data()) != VK_SUCCESS)
        return nullptr;

    std::vector<jint> values;
    for (const VkSurfaceFormatKHR& format : formats)
    {
        values.push_back(static_cast<jint>(format.format));
        values.push_back(static_cast<jint>(format.colorSpace));
    }
    return NativeVulkanIntArray(env, values);
}

extern "C" JNIEXPORT jintArray JNICALL NATIVE_VULKAN(NativeVulkanSwapchain, GetPresentModes)(JNIEnv* env, jclass, jlong physicalDevice,
        jlong surface)
{
    uint32_t count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(FromJava<VkPhysicalDevice>(physicalDevice), FromJava<VkSurfaceKHR>(surface), &count, nullptr);
    std::vector<VkPresentModeKHR> modes(count);
    if (count > 0)
        vkGetPhysicalDeviceSurfacePresentModesKHR(FromJava<VkPhysicalDevice>(physicalDevice), FromJava<VkSurfaceKHR>(surface), &count, modes.data());

    std::vector<jint> values;
    for (VkPresentModeKHR mode : modes)
        values.push_back(static_cast<jint>(mode));
    return NativeVulkanIntArray(env, values);
}

extern "C" JNIEXPORT jlong JNICALL NATIVE_VULKAN(NativeVulkanSwapchain, CreateSwapchain)(JNIEnv* env, jclass, jlong device, jlong surface,
        jint imageCount, jint imageFormat, jint colorSpace, jint width, jint height, jint arrayLayers, jint usage, jint sharingMode,
        jint compositeAlpha, jboolean clipped, jint preTransform, jint presentMode, jintArray queueFamilyArray)
{
    std::vector<jint> queueFamilies = NativeVulkanInts(env, queueFamilyArray);
    std::vector<uint32_t> indices(queueFamilies.begin(), queueFamilies.end());

    VkSwapchainCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    createInfo.surface = FromJava<VkSurfaceKHR>(surface);
    createInfo.minImageCount = static_cast<uint32_t>(imageCount);
    createInfo.imageFormat = static_cast<VkFormat>(imageFormat);
    createInfo.imageColorSpace = static_cast<VkColorSpaceKHR>(colorSpace);
    createInfo.imageExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    createInfo.imageArrayLayers = static_cast<uint32_t>(arrayLayers);
    createInfo.imageUsage = static_cast<VkImageUsageFlags>(usage);
    createInfo.imageSharingMode = static_cast<VkSharingMode>(sharingMode);
    createInfo.compositeAlpha = static_cast<VkCompositeAlphaFlagBitsKHR>(compositeAlpha);
    createInfo.clipped = clipped ? VK_TRUE : VK_FALSE;
    createInfo.preTransform = static_cast<VkSurfaceTransformFlagBitsKHR>(preTransform);
    createInfo.presentMode = static_cast<VkPresentModeKHR>(presentMode);
    if (!indices.empty())
    {
        createInfo.queueFamilyIndexCount = static_cast<uint32_t>(indices.size());
        createInfo.pQueueFamilyIndices = indices.data();
    }

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkResult result = vkCreateSwapchainKHR(FromJava<VkDevice>(device), &createInfo, nullptr, &swapchain);
    if (result != VK_SUCCESS)
    {
        NativeVulkanLog("vkCreateSwapchainKHR failed: %s", NativeVulkanResultName(result));
        return 0;
    }
    return ToJava(swapchain);
}

extern "C" JNIEXPORT jlongArray JNICALL NATIVE_VULKAN(NativeVulkanSwapchain, GetImages)(JNIEnv* env, jclass, jlong device, jlong swapchain)
{
    uint32_t count = 0;
    if (vkGetSwapchainImagesKHR(FromJava<VkDevice>(device), FromJava<VkSwapchainKHR>(swapchain), &count, nullptr) != VK_SUCCESS)
        return nullptr;
    std::vector<VkImage> images(count);
    if (count > 0 && vkGetSwapchainImagesKHR(FromJava<VkDevice>(device), FromJava<VkSwapchainKHR>(swapchain), &count, images.data()) != VK_SUCCESS)
        return nullptr;

    std::vector<jlong> handles;
    for (VkImage image : images)
        handles.push_back(ToJava(image));
    return NativeVulkanLongArray(env, handles);
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanSwapchain, DestroySwapchain)(JNIEnv*, jclass, jlong device, jlong swapchain)
{
    vkDestroySwapchainKHR(FromJava<VkDevice>(device), FromJava<VkSwapchainKHR>(swapchain), nullptr);
}

extern "C" JNIEXPORT jint JNICALL NATIVE_VULKAN(NativeVulkanGraphics, AcquireImage)(JNIEnv* env, jclass, jlong device, jlong swapchain,
        jlong semaphore, jintArray indexArray)
{
    uint32_t index = 0;
    VkResult result = vkAcquireNextImageKHR(FromJava<VkDevice>(device), FromJava<VkSwapchainKHR>(swapchain), UINT64_MAX,
            FromJava<VkSemaphore>(semaphore), VK_NULL_HANDLE, &index);
    jint value = static_cast<jint>(index);
    env->SetIntArrayRegion(indexArray, 0, 1, &value);
    return static_cast<jint>(result);
}

extern "C" JNIEXPORT jint JNICALL NATIVE_VULKAN(NativeVulkanGraphics, Present)(JNIEnv*, jclass, jlong queue, jlong swapchain,
        jlong waitSemaphore, jint imageIndex)
{
    VkSemaphore semaphore = FromJava<VkSemaphore>(waitSemaphore);
    VkSwapchainKHR swapchainHandle = FromJava<VkSwapchainKHR>(swapchain);
    uint32_t index = static_cast<uint32_t>(imageIndex);

    VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &semaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchainHandle;
    presentInfo.pImageIndices = &index;
    return static_cast<jint>(vkQueuePresentKHR(FromJava<VkQueue>(queue), &presentInfo));
}
