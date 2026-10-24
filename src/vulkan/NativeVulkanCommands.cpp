// Recording commands, with the values NativeVulkanCommands gathers from the
// Quorum commands, as DesktopVulkanCommands records them with LWJGL.

#include "NativeVulkan.h"

#include <cstring>

namespace
{
    VkCommandBuffer Buffer(jlong commandBuffer)
    {
        return FromJava<VkCommandBuffer>(commandBuffer);
    }
}

// clearValues holds 4 values of 32 bits for each VkClearValue, or is null.
extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanCommands, BeginRenderPass)(JNIEnv* env, jclass, jlong commandBuffer,
        jlong renderPass, jlong framebuffer, jint width, jint height, jintArray clearArray, jint contents)
{
    std::vector<jint> clearBits = NativeVulkanInts(env, clearArray);
    std::vector<VkClearValue> clearValues(clearBits.size() / 4);
    if (!clearValues.empty())
        memcpy(clearValues.data(), clearBits.data(), clearValues.size() * sizeof(VkClearValue));

    VkRenderPassBeginInfo beginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    beginInfo.renderPass = FromJava<VkRenderPass>(renderPass);
    beginInfo.framebuffer = FromJava<VkFramebuffer>(framebuffer);
    beginInfo.renderArea.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    beginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    beginInfo.pClearValues = clearValues.empty() ? nullptr : clearValues.data();
    vkCmdBeginRenderPass(Buffer(commandBuffer), &beginInfo, static_cast<VkSubpassContents>(contents));
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanCommands, EndRenderPass)(JNIEnv*, jclass, jlong commandBuffer)
{
    vkCmdEndRenderPass(Buffer(commandBuffer));
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanCommands, BindPipeline)(JNIEnv*, jclass, jlong commandBuffer, jint bindPoint,
        jlong pipeline)
{
    vkCmdBindPipeline(Buffer(commandBuffer), static_cast<VkPipelineBindPoint>(bindPoint), FromJava<VkPipeline>(pipeline));
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanCommands, BindDescriptorSet)(JNIEnv*, jclass, jlong commandBuffer, jint bindPoint,
        jlong layout, jint setIndex, jlong descriptorSet)
{
    VkDescriptorSet set = FromJava<VkDescriptorSet>(descriptorSet);
    vkCmdBindDescriptorSets(Buffer(commandBuffer), static_cast<VkPipelineBindPoint>(bindPoint), FromJava<VkPipelineLayout>(layout),
            static_cast<uint32_t>(setIndex), 1, &set, 0, nullptr);
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanCommands, BindIndexBuffer)(JNIEnv*, jclass, jlong commandBuffer, jlong buffer,
        jlong offset, jint indexType)
{
    vkCmdBindIndexBuffer(Buffer(commandBuffer), FromJava<VkBuffer>(buffer), static_cast<VkDeviceSize>(offset), static_cast<VkIndexType>(indexType));
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanCommands, BindVertexBuffer)(JNIEnv*, jclass, jlong commandBuffer, jint firstBinding,
        jlong buffer, jlong offset)
{
    VkBuffer handle = FromJava<VkBuffer>(buffer);
    VkDeviceSize bufferOffset = static_cast<VkDeviceSize>(offset);
    vkCmdBindVertexBuffers(Buffer(commandBuffer), static_cast<uint32_t>(firstBinding), 1, &handle, &bufferOffset);
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanCommands, Dispatch)(JNIEnv*, jclass, jlong commandBuffer, jint x, jint y, jint z)
{
    vkCmdDispatch(Buffer(commandBuffer), static_cast<uint32_t>(x), static_cast<uint32_t>(y), static_cast<uint32_t>(z));
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanCommands, CopyBuffer)(JNIEnv*, jclass, jlong commandBuffer, jlong source,
        jlong destination, jlong sourceOffset, jlong destinationOffset, jlong size)
{
    VkBufferCopy region = {};
    region.srcOffset = static_cast<VkDeviceSize>(sourceOffset);
    region.dstOffset = static_cast<VkDeviceSize>(destinationOffset);
    region.size = static_cast<VkDeviceSize>(size);
    vkCmdCopyBuffer(Buffer(commandBuffer), FromJava<VkBuffer>(source), FromJava<VkBuffer>(destination), 1, &region);
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanCommands, CopyBufferToImage)(JNIEnv* env, jclass, jlong commandBuffer,
        jlong buffer, jlong image, jint layout, jintArray valueArray)
{
    std::vector<jint> values = NativeVulkanInts(env, valueArray);
    if (values.size() < 13)
        return;

    VkBufferImageCopy region = {};
    region.bufferOffset = static_cast<VkDeviceSize>(values[0]);
    region.bufferRowLength = static_cast<uint32_t>(values[1]);
    region.bufferImageHeight = static_cast<uint32_t>(values[2]);
    region.imageSubresource.aspectMask = static_cast<VkImageAspectFlags>(values[3]);
    region.imageSubresource.mipLevel = static_cast<uint32_t>(values[4]);
    region.imageSubresource.baseArrayLayer = static_cast<uint32_t>(values[5]);
    region.imageSubresource.layerCount = static_cast<uint32_t>(values[6]);
    region.imageOffset = {values[7], values[8], values[9]};
    region.imageExtent = {static_cast<uint32_t>(values[10]), static_cast<uint32_t>(values[11]), static_cast<uint32_t>(values[12])};
    vkCmdCopyBufferToImage(Buffer(commandBuffer), FromJava<VkBuffer>(buffer), FromJava<VkImage>(image), static_cast<VkImageLayout>(layout), 1,
            &region);
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanCommands, Draw)(JNIEnv*, jclass, jlong commandBuffer, jint vertexCount,
        jint instanceCount, jint firstVertex, jint firstInstance)
{
    vkCmdDraw(Buffer(commandBuffer), static_cast<uint32_t>(vertexCount), static_cast<uint32_t>(instanceCount), static_cast<uint32_t>(firstVertex),
            static_cast<uint32_t>(firstInstance));
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanCommands, DrawIndexed)(JNIEnv*, jclass, jlong commandBuffer, jint indexCount,
        jint instanceCount, jint firstIndex, jint vertexOffset, jint firstInstance)
{
    vkCmdDrawIndexed(Buffer(commandBuffer), static_cast<uint32_t>(indexCount), static_cast<uint32_t>(instanceCount),
            static_cast<uint32_t>(firstIndex), vertexOffset, static_cast<uint32_t>(firstInstance));
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanCommands, DrawIndirect)(JNIEnv*, jclass, jlong commandBuffer, jlong buffer,
        jlong offset, jint drawCount, jint stride)
{
    vkCmdDrawIndirect(Buffer(commandBuffer), FromJava<VkBuffer>(buffer), static_cast<VkDeviceSize>(offset), static_cast<uint32_t>(drawCount),
            static_cast<uint32_t>(stride));
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanCommands, ImageBarrier)(JNIEnv* env, jclass, jlong commandBuffer, jlong image,
        jintArray valueArray)
{
    std::vector<jint> values = NativeVulkanInts(env, valueArray);
    if (values.size() < 13)
        return;

    VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.srcAccessMask = static_cast<VkAccessFlags>(values[2]);
    barrier.dstAccessMask = static_cast<VkAccessFlags>(values[3]);
    barrier.oldLayout = static_cast<VkImageLayout>(values[4]);
    barrier.newLayout = static_cast<VkImageLayout>(values[5]);
    barrier.srcQueueFamilyIndex = static_cast<uint32_t>(values[6]);
    barrier.dstQueueFamilyIndex = static_cast<uint32_t>(values[7]);
    barrier.subresourceRange.aspectMask = static_cast<VkImageAspectFlags>(values[8]);
    barrier.subresourceRange.baseMipLevel = static_cast<uint32_t>(values[9]);
    barrier.subresourceRange.levelCount = static_cast<uint32_t>(values[10]);
    barrier.subresourceRange.baseArrayLayer = static_cast<uint32_t>(values[11]);
    barrier.subresourceRange.layerCount = static_cast<uint32_t>(values[12]);
    barrier.image = FromJava<VkImage>(image);
    vkCmdPipelineBarrier(Buffer(commandBuffer), static_cast<VkPipelineStageFlags>(values[0]), static_cast<VkPipelineStageFlags>(values[1]), 0,
            0, nullptr, 0, nullptr, 1, &barrier);
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanCommands, MemoryBarrier)(JNIEnv*, jclass, jlong commandBuffer,
        jint sourceStageMask, jint destinationStageMask, jint sourceAccessMask, jint destinationAccessMask)
{
    VkMemoryBarrier barrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = static_cast<VkAccessFlags>(sourceAccessMask);
    barrier.dstAccessMask = static_cast<VkAccessFlags>(destinationAccessMask);
    vkCmdPipelineBarrier(Buffer(commandBuffer), static_cast<VkPipelineStageFlags>(sourceStageMask),
            static_cast<VkPipelineStageFlags>(destinationStageMask), 0, 1, &barrier, 0, nullptr, 0, nullptr);
}

// data is a direct buffer (NativeVulkanPushConstantBuffer's).
extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanCommands, PushConstants)(JNIEnv* env, jclass, jlong commandBuffer, jlong layout,
        jint stageFlags, jint offset, jobject data, jint size)
{
    void* bytes = env->GetDirectBufferAddress(data);
    if (bytes == nullptr)
        return;
    vkCmdPushConstants(Buffer(commandBuffer), FromJava<VkPipelineLayout>(layout), static_cast<VkShaderStageFlags>(stageFlags),
            static_cast<uint32_t>(offset), static_cast<uint32_t>(size), bytes);
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanCommands, SetScissor)(JNIEnv*, jclass, jlong commandBuffer, jint x, jint y,
        jint width, jint height)
{
    VkRect2D scissor = {};
    scissor.offset = {x, y};
    scissor.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    vkCmdSetScissor(Buffer(commandBuffer), 0, 1, &scissor);
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanCommands, SetViewport)(JNIEnv*, jclass, jlong commandBuffer, jfloat x, jfloat y,
        jfloat width, jfloat height, jfloat minimumDepth, jfloat maximumDepth)
{
    VkViewport viewport = {x, y, width, height, minimumDepth, maximumDepth};
    vkCmdSetViewport(Buffer(commandBuffer), 0, 1, &viewport);
}
