// AMD's Vulkan Memory Allocator (VMA), compiled here, and the buffers and
// images made with it, as the desktop makes them with LWJGL's VMA binding.
// VMA calls Vulkan's functions directly (VMA_STATIC_VULKAN_FUNCTIONS, set in
// CMakeLists.txt).

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "NativeVulkan.h"

// The allocator, with buffer device addresses, as on the desktop. It uses the
// Vulkan version Quorum asked for, up to 1.3.
extern "C" JNIEXPORT jlong JNICALL NATIVE_VULKAN(NativeVulkanMemoryAllocator, CreateAllocator)(JNIEnv*, jclass, jlong instance,
        jlong physicalDevice, jlong device)
{
    uint32_t apiVersion = NativeVulkanInstanceApiVersion();
    if (apiVersion > VK_API_VERSION_1_3)
        apiVersion = VK_API_VERSION_1_3;

    VmaAllocatorCreateInfo createInfo = {};
    createInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    createInfo.instance = FromJava<VkInstance>(instance);
    createInfo.physicalDevice = FromJava<VkPhysicalDevice>(physicalDevice);
    createInfo.device = FromJava<VkDevice>(device);
    createInfo.vulkanApiVersion = apiVersion;

    VmaAllocator allocator = VK_NULL_HANDLE;
    VkResult result = vmaCreateAllocator(&createInfo, &allocator);
    if (result != VK_SUCCESS)
    {
        NativeVulkanLog("vmaCreateAllocator failed: %s", NativeVulkanResultName(result));
        return 0;
    }
    return ToJava(allocator);
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanMemoryAllocator, DestroyAllocator)(JNIEnv*, jclass, jlong allocator)
{
    vmaDestroyAllocator(FromJava<VmaAllocator>(allocator));
}

extern "C" JNIEXPORT jdouble JNICALL NATIVE_VULKAN(NativeVulkanMemoryAllocator, CalculateAllocatedBytes)(JNIEnv*, jclass, jlong allocator)
{
    VmaTotalStatistics statistics;
    vmaCalculateStatistics(FromJava<VmaAllocator>(allocator), &statistics);
    return static_cast<jdouble>(statistics.total.statistics.allocationBytes);
}

extern "C" JNIEXPORT jint JNICALL NATIVE_VULKAN(NativeVulkanMemoryAllocator, CalculateAllocationCount)(JNIEnv*, jclass, jlong allocator)
{
    VmaTotalStatistics statistics;
    vmaCalculateStatistics(FromJava<VmaAllocator>(allocator), &statistics);
    return static_cast<jint>(statistics.total.statistics.allocationCount);
}

// A buffer, its allocation, and its device address when its usage asks for one.
extern "C" JNIEXPORT jboolean JNICALL NATIVE_VULKAN(NativeVulkanBuffer, CreateBuffer)(JNIEnv* env, jclass, jlong device, jlong allocator,
        jint size, jint bufferUsage, jint memoryUsage, jint allocationFlags, jint requiredMemoryFlags, jint preferredMemoryFlags,
        jint sharingMode, jlongArray resultArray)
{
    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = static_cast<VkDeviceSize>(size);
    bufferInfo.usage = static_cast<VkBufferUsageFlags>(bufferUsage);
    bufferInfo.sharingMode = static_cast<VkSharingMode>(sharingMode);

    VmaAllocationCreateInfo allocationInfo = {};
    allocationInfo.usage = memoryUsage == 0 ? VMA_MEMORY_USAGE_AUTO : static_cast<VmaMemoryUsage>(memoryUsage);
    allocationInfo.flags = static_cast<VmaAllocationCreateFlags>(allocationFlags);
    allocationInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(requiredMemoryFlags);
    allocationInfo.preferredFlags = static_cast<VkMemoryPropertyFlags>(preferredMemoryFlags);

    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkResult result = vmaCreateBuffer(FromJava<VmaAllocator>(allocator), &bufferInfo, &allocationInfo, &buffer, &allocation, nullptr);
    if (result != VK_SUCCESS)
    {
        NativeVulkanLog("vmaCreateBuffer failed: %s", NativeVulkanResultName(result));
        return JNI_FALSE;
    }

    jlong address = 0;
    if ((bufferUsage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) != 0)
    {
        VkBufferDeviceAddressInfo addressInfo = {VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
        addressInfo.buffer = buffer;
        address = static_cast<jlong>(vkGetBufferDeviceAddress(FromJava<VkDevice>(device), &addressInfo));
    }

    jlong values[3] = {ToJava(buffer), ToJava(allocation), address};
    env->SetLongArrayRegion(resultArray, 0, 3, values);
    return JNI_TRUE;
}

extern "C" JNIEXPORT jlong JNICALL NATIVE_VULKAN(NativeVulkanBuffer, MapBuffer)(JNIEnv*, jclass, jlong allocator, jlong allocation)
{
    void* data = nullptr;
    if (vmaMapMemory(FromJava<VmaAllocator>(allocator), FromJava<VmaAllocation>(allocation), &data) != VK_SUCCESS)
        return 0;
    return ToJava(data);
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanBuffer, UnmapBuffer)(JNIEnv*, jclass, jlong allocator, jlong allocation)
{
    vmaUnmapMemory(FromJava<VmaAllocator>(allocator), FromJava<VmaAllocation>(allocation));
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanBuffer, DestroyBuffer)(JNIEnv*, jclass, jlong allocator, jlong buffer,
        jlong allocation)
{
    vmaDestroyBuffer(FromJava<VmaAllocator>(allocator), FromJava<VkBuffer>(buffer), FromJava<VmaAllocation>(allocation));
}

/*
An image and its allocation, from NativeVulkanImage's values. As on the
desktop, the memory usage is passed to VMA as it is (no "auto" for 0).
*/
extern "C" JNIEXPORT jboolean JNICALL NATIVE_VULKAN(NativeVulkanImage, CreateImage)(JNIEnv* env, jclass, jlong allocator, jintArray valueArray,
        jlongArray resultArray)
{
    std::vector<jint> values = NativeVulkanInts(env, valueArray);
    if (values.size() < 16)
        return JNI_FALSE;

    VkImageCreateInfo imageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.extent = {static_cast<uint32_t>(values[0]), static_cast<uint32_t>(values[1]), static_cast<uint32_t>(values[2])};
    imageInfo.imageType = static_cast<VkImageType>(values[3]);
    imageInfo.format = static_cast<VkFormat>(values[4]);
    imageInfo.mipLevels = static_cast<uint32_t>(values[5]);
    imageInfo.arrayLayers = static_cast<uint32_t>(values[6]);
    imageInfo.samples = static_cast<VkSampleCountFlagBits>(values[7]);
    imageInfo.tiling = static_cast<VkImageTiling>(values[8]);
    imageInfo.usage = static_cast<VkImageUsageFlags>(values[9]);
    imageInfo.sharingMode = static_cast<VkSharingMode>(values[10]);
    imageInfo.flags = static_cast<VkImageCreateFlags>(values[11]);

    VmaAllocationCreateInfo allocationInfo = {};
    allocationInfo.usage = static_cast<VmaMemoryUsage>(values[12]);
    allocationInfo.flags = static_cast<VmaAllocationCreateFlags>(values[13]);
    allocationInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(values[14]);
    allocationInfo.preferredFlags = static_cast<VkMemoryPropertyFlags>(values[15]);

    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkResult result = vmaCreateImage(FromJava<VmaAllocator>(allocator), &imageInfo, &allocationInfo, &image, &allocation, nullptr);
    if (result != VK_SUCCESS)
    {
        NativeVulkanLog("vmaCreateImage failed: %s", NativeVulkanResultName(result));
        return JNI_FALSE;
    }

    jlong handles[2] = {ToJava(image), ToJava(allocation)};
    env->SetLongArrayRegion(resultArray, 0, 2, handles);
    return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanImage, DestroyImage)(JNIEnv*, jclass, jlong allocator, jlong image, jlong allocation)
{
    vmaDestroyImage(FromJava<VmaAllocator>(allocator), FromJava<VkImage>(image), FromJava<VmaAllocation>(allocation));
}
