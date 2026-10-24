// Command pools and buffers, fences and semaphores, image views,
// framebuffers, samplers, render passes, descriptor pools, layouts and sets,
// pipeline layouts and caches, and pipelines. Each does what the matching
// desktop strategy does with LWJGL; the Java side describes the layout of the
// arrays it passes.

#include "NativeVulkan.h"

extern "C" JNIEXPORT jlong JNICALL NATIVE_VULKAN(NativeVulkanCommandPool, CreateCommandPool)(JNIEnv*, jclass, jlong device, jint flags,
        jint queueFamilyIndex)
{
    VkCommandPoolCreateInfo createInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    createInfo.flags = static_cast<VkCommandPoolCreateFlags>(flags);
    createInfo.queueFamilyIndex = static_cast<uint32_t>(queueFamilyIndex);

    VkCommandPool pool = VK_NULL_HANDLE;
    if (vkCreateCommandPool(FromJava<VkDevice>(device), &createInfo, nullptr, &pool) != VK_SUCCESS)
        return 0;
    return ToJava(pool);
}

extern "C" JNIEXPORT jlong JNICALL NATIVE_VULKAN(NativeVulkanCommandBuffer, AllocateCommandBuffer)(JNIEnv*, jclass, jlong device, jlong pool,
        jint level)
{
    VkCommandBufferAllocateInfo allocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocateInfo.commandPool = FromJava<VkCommandPool>(pool);
    allocateInfo.level = static_cast<VkCommandBufferLevel>(level);
    allocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(FromJava<VkDevice>(device), &allocateInfo, &commandBuffer) != VK_SUCCESS)
        return 0;
    return ToJava(commandBuffer);
}

extern "C" JNIEXPORT jboolean JNICALL NATIVE_VULKAN(NativeVulkanCommandBuffer, BeginCommandBuffer)(JNIEnv*, jclass, jlong commandBuffer)
{
    VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    return vkBeginCommandBuffer(FromJava<VkCommandBuffer>(commandBuffer), &beginInfo) == VK_SUCCESS ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL NATIVE_VULKAN(NativeVulkanCommandBuffer, EndCommandBuffer)(JNIEnv*, jclass, jlong commandBuffer)
{
    return vkEndCommandBuffer(FromJava<VkCommandBuffer>(commandBuffer)) == VK_SUCCESS ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanCommandBuffer, ResetCommandBuffer)(JNIEnv*, jclass, jlong commandBuffer,
        jboolean releaseResources)
{
    vkResetCommandBuffer(FromJava<VkCommandBuffer>(commandBuffer), releaseResources ? VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT : 0);
}

extern "C" JNIEXPORT jlong JNICALL NATIVE_VULKAN(NativeVulkanFence, CreateFence)(JNIEnv*, jclass, jlong device, jboolean signaled)
{
    VkFenceCreateInfo createInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    createInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

    VkFence fence = VK_NULL_HANDLE;
    if (vkCreateFence(FromJava<VkDevice>(device), &createInfo, nullptr, &fence) != VK_SUCCESS)
        return 0;
    return ToJava(fence);
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanFence, DestroyFence)(JNIEnv*, jclass, jlong device, jlong fence)
{
    vkDestroyFence(FromJava<VkDevice>(device), FromJava<VkFence>(fence), nullptr);
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanFence, WaitForFence)(JNIEnv*, jclass, jlong device, jlong fence)
{
    VkFence handle = FromJava<VkFence>(fence);
    vkWaitForFences(FromJava<VkDevice>(device), 1, &handle, VK_TRUE, UINT64_MAX);
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanFence, ResetFence)(JNIEnv*, jclass, jlong device, jlong fence)
{
    VkFence handle = FromJava<VkFence>(fence);
    vkResetFences(FromJava<VkDevice>(device), 1, &handle);
}

extern "C" JNIEXPORT jlong JNICALL NATIVE_VULKAN(NativeVulkanSemaphore, CreateSemaphore)(JNIEnv*, jclass, jlong device)
{
    VkSemaphoreCreateInfo createInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkSemaphore semaphore = VK_NULL_HANDLE;
    if (vkCreateSemaphore(FromJava<VkDevice>(device), &createInfo, nullptr, &semaphore) != VK_SUCCESS)
        return 0;
    return ToJava(semaphore);
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanSemaphore, DestroySemaphore)(JNIEnv*, jclass, jlong device, jlong semaphore)
{
    vkDestroySemaphore(FromJava<VkDevice>(device), FromJava<VkSemaphore>(semaphore), nullptr);
}

extern "C" JNIEXPORT jlong JNICALL NATIVE_VULKAN(NativeVulkanImageView, CreateImageView)(JNIEnv*, jclass, jlong device, jlong image,
        jint viewType, jint format, jint aspectMask, jint baseMipLevel, jint levelCount, jint baseArrayLayer, jint layerCount)
{
    VkImageViewCreateInfo createInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    createInfo.image = FromJava<VkImage>(image);
    createInfo.viewType = static_cast<VkImageViewType>(viewType);
    createInfo.format = static_cast<VkFormat>(format);
    createInfo.subresourceRange.aspectMask = static_cast<VkImageAspectFlags>(aspectMask);
    createInfo.subresourceRange.baseMipLevel = static_cast<uint32_t>(baseMipLevel);
    createInfo.subresourceRange.levelCount = static_cast<uint32_t>(levelCount);
    createInfo.subresourceRange.baseArrayLayer = static_cast<uint32_t>(baseArrayLayer);
    createInfo.subresourceRange.layerCount = static_cast<uint32_t>(layerCount);

    VkImageView view = VK_NULL_HANDLE;
    if (vkCreateImageView(FromJava<VkDevice>(device), &createInfo, nullptr, &view) != VK_SUCCESS)
        return 0;
    return ToJava(view);
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanImageView, DestroyImageView)(JNIEnv*, jclass, jlong device, jlong view)
{
    vkDestroyImageView(FromJava<VkDevice>(device), FromJava<VkImageView>(view), nullptr);
}

extern "C" JNIEXPORT jlong JNICALL NATIVE_VULKAN(NativeVulkanFramebuffer, CreateFramebuffer)(JNIEnv* env, jclass, jlong device,
        jlong renderPass, jlongArray viewArray, jint width, jint height, jint layers)
{
    std::vector<jlong> viewHandles = NativeVulkanLongs(env, viewArray);
    std::vector<VkImageView> views;
    for (jlong handle : viewHandles)
        views.push_back(FromJava<VkImageView>(handle));

    VkFramebufferCreateInfo createInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    createInfo.renderPass = FromJava<VkRenderPass>(renderPass);
    createInfo.attachmentCount = static_cast<uint32_t>(views.size());
    createInfo.pAttachments = views.data();
    createInfo.width = static_cast<uint32_t>(width);
    createInfo.height = static_cast<uint32_t>(height);
    createInfo.layers = static_cast<uint32_t>(layers);

    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    if (vkCreateFramebuffer(FromJava<VkDevice>(device), &createInfo, nullptr, &framebuffer) != VK_SUCCESS)
        return 0;
    return ToJava(framebuffer);
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanFramebuffer, DestroyFramebuffer)(JNIEnv*, jclass, jlong device, jlong framebuffer)
{
    vkDestroyFramebuffer(FromJava<VkDevice>(device), FromJava<VkFramebuffer>(framebuffer), nullptr);
}

extern "C" JNIEXPORT jlong JNICALL NATIVE_VULKAN(NativeVulkanSampler, CreateSampler)(JNIEnv* env, jclass, jlong device, jintArray valueArray,
        jfloatArray numberArray)
{
    std::vector<jint> values = NativeVulkanInts(env, valueArray);
    std::vector<jfloat> numbers = NativeVulkanFloats(env, numberArray);
    if (values.size() < 12 || numbers.size() < 4)
        return 0;

    VkSamplerCreateInfo createInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    createInfo.magFilter = static_cast<VkFilter>(values[0]);
    createInfo.minFilter = static_cast<VkFilter>(values[1]);
    createInfo.mipmapMode = static_cast<VkSamplerMipmapMode>(values[2]);
    createInfo.addressModeU = static_cast<VkSamplerAddressMode>(values[3]);
    createInfo.addressModeV = static_cast<VkSamplerAddressMode>(values[4]);
    createInfo.addressModeW = static_cast<VkSamplerAddressMode>(values[5]);
    createInfo.anisotropyEnable = values[6] ? VK_TRUE : VK_FALSE;
    createInfo.compareEnable = values[7] ? VK_TRUE : VK_FALSE;
    createInfo.compareOp = static_cast<VkCompareOp>(values[8]);
    createInfo.borderColor = static_cast<VkBorderColor>(values[9]);
    createInfo.unnormalizedCoordinates = values[10] ? VK_TRUE : VK_FALSE;
    createInfo.flags = static_cast<VkSamplerCreateFlags>(values[11]);
    createInfo.mipLodBias = numbers[0];
    createInfo.maxAnisotropy = numbers[1];
    createInfo.minLod = numbers[2];
    createInfo.maxLod = numbers[3];

    VkSampler sampler = VK_NULL_HANDLE;
    if (vkCreateSampler(FromJava<VkDevice>(device), &createInfo, nullptr, &sampler) != VK_SUCCESS)
        return 0;
    return ToJava(sampler);
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanSampler, DestroySampler)(JNIEnv*, jclass, jlong device, jlong sampler)
{
    vkDestroySampler(FromJava<VkDevice>(device), FromJava<VkSampler>(sampler), nullptr);
}

/*
A render pass, from NativeVulkanRenderPass' arrays. As LWJGL does on the
desktop, the input attachment and preserved attachment counts come from their
arrays, and the resolve attachments use the color attachments' count.
*/
extern "C" JNIEXPORT jlong JNICALL NATIVE_VULKAN(NativeVulkanRenderPass, CreateRenderPass)(JNIEnv* env, jclass, jlong device,
        jintArray attachmentArray, jint subpassCount, jintArray subpassArray, jintArray dependencyArray)
{
    std::vector<jint> attachmentValues = NativeVulkanInts(env, attachmentArray);
    std::vector<jint> subpassValues = NativeVulkanInts(env, subpassArray);
    std::vector<jint> dependencyValues = NativeVulkanInts(env, dependencyArray);

    std::vector<VkAttachmentDescription> attachments(attachmentValues.size() / 9);
    for (size_t i = 0; i < attachments.size(); i++)
    {
        const jint* value = &attachmentValues[i * 9];
        VkAttachmentDescription& description = attachments[i];
        description.flags = static_cast<VkAttachmentDescriptionFlags>(value[0]);
        description.format = static_cast<VkFormat>(value[1]);
        description.samples = static_cast<VkSampleCountFlagBits>(value[2]);
        description.loadOp = static_cast<VkAttachmentLoadOp>(value[3]);
        description.storeOp = static_cast<VkAttachmentStoreOp>(value[4]);
        description.initialLayout = static_cast<VkImageLayout>(value[5]);
        description.finalLayout = static_cast<VkImageLayout>(value[6]);
        description.stencilLoadOp = static_cast<VkAttachmentLoadOp>(value[7]);
        description.stencilStoreOp = static_cast<VkAttachmentStoreOp>(value[8]);
    }

    // Each subpass' references are kept here until the render pass is made.
    std::vector<std::vector<VkAttachmentReference>> colors(subpassCount), inputs(subpassCount), resolves(subpassCount);
    std::vector<std::vector<uint32_t>> preserves(subpassCount);
    std::vector<VkAttachmentReference> depths(subpassCount);
    std::vector<VkSubpassDescription> subpasses(subpassCount);

    size_t at = 0;
    auto next = [&subpassValues, &at]() -> jint
    {
        return at < subpassValues.size() ? subpassValues[at++] : 0;
    };
    auto readReferences = [&next](std::vector<VkAttachmentReference>& references, jint count)
    {
        for (jint i = 0; i < count; i++)
        {
            VkAttachmentReference reference;
            reference.attachment = static_cast<uint32_t>(next());
            reference.layout = static_cast<VkImageLayout>(next());
            references.push_back(reference);
        }
    };

    for (jint i = 0; i < subpassCount; i++)
    {
        VkSubpassDescription& subpass = subpasses[i];
        subpass = {};
        subpass.pipelineBindPoint = static_cast<VkPipelineBindPoint>(next());
        subpass.flags = static_cast<VkSubpassDescriptionFlags>(next());
        jint colorCount = next();
        jint inputCount = next();
        jint resolveCount = next();
        jint preserveCount = next();
        jint hasDepth = next();
        depths[i].attachment = static_cast<uint32_t>(next());
        depths[i].layout = static_cast<VkImageLayout>(next());
        readReferences(colors[i], colorCount);
        readReferences(inputs[i], inputCount);
        readReferences(resolves[i], resolveCount);
        for (jint j = 0; j < preserveCount; j++)
            preserves[i].push_back(static_cast<uint32_t>(next()));

        if (!colors[i].empty())
        {
            subpass.colorAttachmentCount = static_cast<uint32_t>(colors[i].size());
            subpass.pColorAttachments = colors[i].data();
        }
        if (!inputs[i].empty())
        {
            subpass.inputAttachmentCount = static_cast<uint32_t>(inputs[i].size());
            subpass.pInputAttachments = inputs[i].data();
        }
        if (!resolves[i].empty())
            subpass.pResolveAttachments = resolves[i].data();
        if (!preserves[i].empty())
        {
            subpass.preserveAttachmentCount = static_cast<uint32_t>(preserves[i].size());
            subpass.pPreserveAttachments = preserves[i].data();
        }
        if (hasDepth)
            subpass.pDepthStencilAttachment = &depths[i];
    }

    std::vector<VkSubpassDependency> dependencies(dependencyValues.size() / 7);
    for (size_t i = 0; i < dependencies.size(); i++)
    {
        const jint* value = &dependencyValues[i * 7];
        VkSubpassDependency& dependency = dependencies[i];
        dependency.srcSubpass = static_cast<uint32_t>(value[0]);
        dependency.dstSubpass = static_cast<uint32_t>(value[1]);
        dependency.srcStageMask = static_cast<VkPipelineStageFlags>(value[2]);
        dependency.dstStageMask = static_cast<VkPipelineStageFlags>(value[3]);
        dependency.srcAccessMask = static_cast<VkAccessFlags>(value[4]);
        dependency.dstAccessMask = static_cast<VkAccessFlags>(value[5]);
        dependency.dependencyFlags = static_cast<VkDependencyFlags>(value[6]);
    }

    VkRenderPassCreateInfo createInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    createInfo.pAttachments = attachments.data();
    createInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
    createInfo.pSubpasses = subpasses.data();
    createInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    createInfo.pDependencies = dependencies.empty() ? nullptr : dependencies.data();

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkResult result = vkCreateRenderPass(FromJava<VkDevice>(device), &createInfo, nullptr, &renderPass);
    if (result != VK_SUCCESS)
    {
        NativeVulkanLog("vkCreateRenderPass failed: %s", NativeVulkanResultName(result));
        return 0;
    }
    return ToJava(renderPass);
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanRenderPass, DestroyRenderPass)(JNIEnv*, jclass, jlong device, jlong renderPass)
{
    vkDestroyRenderPass(FromJava<VkDevice>(device), FromJava<VkRenderPass>(renderPass), nullptr);
}

extern "C" JNIEXPORT jlong JNICALL NATIVE_VULKAN(NativeVulkanDescriptorPool, CreateDescriptorPool)(JNIEnv* env, jclass, jlong device,
        jintArray typeArray, jintArray countArray, jint flags, jint maxSets)
{
    std::vector<jint> types = NativeVulkanInts(env, typeArray);
    std::vector<jint> counts = NativeVulkanInts(env, countArray);
    std::vector<VkDescriptorPoolSize> sizes(types.size());
    for (size_t i = 0; i < types.size() && i < counts.size(); i++)
    {
        sizes[i].type = static_cast<VkDescriptorType>(types[i]);
        sizes[i].descriptorCount = static_cast<uint32_t>(counts[i]);
    }

    VkDescriptorPoolCreateInfo createInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    createInfo.flags = static_cast<VkDescriptorPoolCreateFlags>(flags);
    createInfo.maxSets = static_cast<uint32_t>(maxSets);
    createInfo.poolSizeCount = static_cast<uint32_t>(sizes.size());
    createInfo.pPoolSizes = sizes.data();

    VkDescriptorPool pool = VK_NULL_HANDLE;
    VkResult result = vkCreateDescriptorPool(FromJava<VkDevice>(device), &createInfo, nullptr, &pool);
    if (result != VK_SUCCESS)
    {
        NativeVulkanLog("vkCreateDescriptorPool failed: %s", NativeVulkanResultName(result));
        return 0;
    }
    return ToJava(pool);
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanDescriptorPool, DestroyDescriptorPool)(JNIEnv*, jclass, jlong device, jlong pool)
{
    vkDestroyDescriptorPool(FromJava<VkDevice>(device), FromJava<VkDescriptorPool>(pool), nullptr);
}

/*
A descriptor set layout. Binding flags are only given if a binding has some,
and a binding with immutable samplers takes its descriptor count from them,
as on the desktop.
*/
extern "C" JNIEXPORT jlong JNICALL NATIVE_VULKAN(NativeVulkanDescriptorSetLayout, CreateDescriptorSetLayout)(JNIEnv* env, jclass,
        jlong device, jintArray bindingArray, jlongArray samplerArray, jint flags)
{
    std::vector<jint> values = NativeVulkanInts(env, bindingArray);
    std::vector<jlong> samplerHandles = NativeVulkanLongs(env, samplerArray);
    std::vector<VkSampler> samplers;
    for (jlong handle : samplerHandles)
        samplers.push_back(FromJava<VkSampler>(handle));

    size_t count = values.size() / 6;
    std::vector<VkDescriptorSetLayoutBinding> bindings(count);
    std::vector<VkDescriptorBindingFlags> bindingFlags(count);
    bool hasBindingFlags = false;
    size_t samplerAt = 0;
    for (size_t i = 0; i < count; i++)
    {
        const jint* value = &values[i * 6];
        VkDescriptorSetLayoutBinding& binding = bindings[i];
        binding.binding = static_cast<uint32_t>(value[0]);
        binding.descriptorCount = static_cast<uint32_t>(value[1]);
        binding.descriptorType = static_cast<VkDescriptorType>(value[2]);
        binding.stageFlags = static_cast<VkShaderStageFlags>(value[3]);
        bindingFlags[i] = static_cast<VkDescriptorBindingFlags>(value[4]);
        if (value[4] != 0)
            hasBindingFlags = true;

        jint samplerCount = value[5];
        if (samplerCount > 0 && samplerAt + samplerCount <= samplers.size())
        {
            binding.pImmutableSamplers = &samplers[samplerAt];
            binding.descriptorCount = static_cast<uint32_t>(samplerCount);
            samplerAt += samplerCount;
        }
    }

    VkDescriptorSetLayoutBindingFlagsCreateInfo flagInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO};
    flagInfo.bindingCount = static_cast<uint32_t>(count);
    flagInfo.pBindingFlags = bindingFlags.data();

    VkDescriptorSetLayoutCreateInfo createInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    createInfo.pNext = hasBindingFlags ? &flagInfo : nullptr;
    createInfo.flags = static_cast<VkDescriptorSetLayoutCreateFlags>(flags);
    createInfo.bindingCount = static_cast<uint32_t>(count);
    createInfo.pBindings = bindings.data();

    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    VkResult result = vkCreateDescriptorSetLayout(FromJava<VkDevice>(device), &createInfo, nullptr, &layout);
    if (result != VK_SUCCESS)
    {
        NativeVulkanLog("vkCreateDescriptorSetLayout failed: %s", NativeVulkanResultName(result));
        return 0;
    }
    return ToJava(layout);
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanDescriptorSetLayout, DestroyDescriptorSetLayout)(JNIEnv*, jclass, jlong device,
        jlong layout)
{
    vkDestroyDescriptorSetLayout(FromJava<VkDevice>(device), FromJava<VkDescriptorSetLayout>(layout), nullptr);
}

// A descriptor set, always with the variable descriptor count, as on the desktop.
extern "C" JNIEXPORT jint JNICALL NATIVE_VULKAN(NativeVulkanDescriptorSet, AllocateDescriptorSet)(JNIEnv* env, jclass, jlong device,
        jlong pool, jlong layout, jint variableDescriptorCount, jlongArray resultArray)
{
    VkDescriptorSetLayout layoutHandle = FromJava<VkDescriptorSetLayout>(layout);
    uint32_t variableCount = static_cast<uint32_t>(variableDescriptorCount);

    VkDescriptorSetVariableDescriptorCountAllocateInfo variableInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO};
    variableInfo.descriptorSetCount = 1;
    variableInfo.pDescriptorCounts = &variableCount;

    VkDescriptorSetAllocateInfo allocateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocateInfo.pNext = &variableInfo;
    allocateInfo.descriptorPool = FromJava<VkDescriptorPool>(pool);
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &layoutHandle;

    VkDescriptorSet set = VK_NULL_HANDLE;
    VkResult result = vkAllocateDescriptorSets(FromJava<VkDevice>(device), &allocateInfo, &set);
    if (result == VK_SUCCESS)
    {
        jlong handle = ToJava(set);
        env->SetLongArrayRegion(resultArray, 0, 1, &handle);
    }
    return static_cast<jint>(result);
}

// Descriptor writes, from NativeVulkanDescriptorSet's arrays.
extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanDescriptorSet, UpdateDescriptorSet)(JNIEnv* env, jclass, jlong device,
        jlong descriptorSet, jint writeCount, jintArray writeArray, jlongArray bufferArray, jlongArray imageArray)
{
    std::vector<jint> writeValues = NativeVulkanInts(env, writeArray);
    std::vector<jlong> bufferValues = NativeVulkanLongs(env, bufferArray);
    std::vector<jlong> imageValues = NativeVulkanLongs(env, imageArray);

    std::vector<VkDescriptorBufferInfo> buffers(bufferValues.size() / 3);
    for (size_t i = 0; i < buffers.size(); i++)
    {
        buffers[i].buffer = FromJava<VkBuffer>(bufferValues[i * 3]);
        buffers[i].offset = static_cast<VkDeviceSize>(bufferValues[i * 3 + 1]);
        buffers[i].range = static_cast<VkDeviceSize>(bufferValues[i * 3 + 2]);
    }
    std::vector<VkDescriptorImageInfo> images(imageValues.size() / 3);
    for (size_t i = 0; i < images.size(); i++)
    {
        images[i].sampler = FromJava<VkSampler>(imageValues[i * 3]);
        images[i].imageView = FromJava<VkImageView>(imageValues[i * 3 + 1]);
        images[i].imageLayout = static_cast<VkImageLayout>(imageValues[i * 3 + 2]);
    }

    std::vector<VkWriteDescriptorSet> writes(writeCount);
    size_t bufferAt = 0;
    size_t imageAt = 0;
    for (jint i = 0; i < writeCount && static_cast<size_t>(i * 7 + 6) < writeValues.size(); i++)
    {
        const jint* value = &writeValues[i * 7];
        VkWriteDescriptorSet& write = writes[i];
        write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = FromJava<VkDescriptorSet>(descriptorSet);
        write.dstBinding = static_cast<uint32_t>(value[0]);
        write.descriptorType = static_cast<VkDescriptorType>(value[1]);
        write.descriptorCount = static_cast<uint32_t>(value[2]);
        write.dstArrayElement = static_cast<uint32_t>(value[3]);
        jint bufferCount = value[4];
        jint imageCount = value[5];
        if (bufferCount > 0)
        {
            write.pBufferInfo = &buffers[bufferAt];
            bufferAt += bufferCount;
        }
        if (imageCount > 0)
        {
            write.pImageInfo = &images[imageAt];
            imageAt += imageCount;
        }
    }

    vkUpdateDescriptorSets(FromJava<VkDevice>(device), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

extern "C" JNIEXPORT jlong JNICALL NATIVE_VULKAN(NativeVulkanPipelineLayout, CreatePipelineLayout)(JNIEnv* env, jclass, jlong device,
        jlongArray setLayoutArray, jintArray rangeArray)
{
    std::vector<jlong> layoutHandles = NativeVulkanLongs(env, setLayoutArray);
    std::vector<jint> range = NativeVulkanInts(env, rangeArray);
    std::vector<VkDescriptorSetLayout> setLayouts;
    for (jlong handle : layoutHandles)
        setLayouts.push_back(FromJava<VkDescriptorSetLayout>(handle));

    VkPushConstantRange pushConstantRange = {};
    VkPipelineLayoutCreateInfo createInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    createInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    createInfo.pSetLayouts = setLayouts.empty() ? nullptr : setLayouts.data();
    if (range.size() >= 3)
    {
        pushConstantRange.stageFlags = static_cast<VkShaderStageFlags>(range[0]);
        pushConstantRange.offset = static_cast<uint32_t>(range[1]);
        pushConstantRange.size = static_cast<uint32_t>(range[2]);
        createInfo.pushConstantRangeCount = 1;
        createInfo.pPushConstantRanges = &pushConstantRange;
    }

    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkResult result = vkCreatePipelineLayout(FromJava<VkDevice>(device), &createInfo, nullptr, &layout);
    if (result != VK_SUCCESS)
    {
        NativeVulkanLog("vkCreatePipelineLayout failed: %s", NativeVulkanResultName(result));
        return 0;
    }
    return ToJava(layout);
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanPipelineLayout, DestroyPipelineLayout)(JNIEnv*, jclass, jlong device, jlong layout)
{
    vkDestroyPipelineLayout(FromJava<VkDevice>(device), FromJava<VkPipelineLayout>(layout), nullptr);
}

extern "C" JNIEXPORT jlong JNICALL NATIVE_VULKAN(NativeVulkanPipelineCache, CreatePipelineCache)(JNIEnv*, jclass, jlong device)
{
    VkPipelineCacheCreateInfo createInfo = {VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
    VkPipelineCache cache = VK_NULL_HANDLE;
    if (vkCreatePipelineCache(FromJava<VkDevice>(device), &createInfo, nullptr, &cache) != VK_SUCCESS)
        return 0;
    return ToJava(cache);
}

/*
A graphics pipeline from NativeVulkanPipeline's arrays, as
DesktopVulkanPipeline makes it: the viewport and scissor are dynamic, there's
no depth clamp or bias, and depth and stencil state is only given when the
depth test is on.
*/
extern "C" JNIEXPORT jlong JNICALL NATIVE_VULKAN(NativeVulkanPipeline, CreateGraphicsPipeline)(JNIEnv* env, jclass, jlong device, jlong cache,
        jlong renderPass, jlong layout, jintArray attributeArray, jintArray bindingArray, jintArray stageArray, jlongArray moduleArray,
        jintArray stateArray, jfloat lineWidth, jintArray blendArray, jintArray dynamicStateArray)
{
    std::vector<jint> attributeValues = NativeVulkanInts(env, attributeArray);
    std::vector<jint> bindingValues = NativeVulkanInts(env, bindingArray);
    std::vector<jint> stageValues = NativeVulkanInts(env, stageArray);
    std::vector<jlong> moduleValues = NativeVulkanLongs(env, moduleArray);
    std::vector<jint> state = NativeVulkanInts(env, stateArray);
    std::vector<jint> blendValues = NativeVulkanInts(env, blendArray);
    std::vector<jint> dynamicValues = NativeVulkanInts(env, dynamicStateArray);
    if (state.size() < 11)
        return 0;

    std::vector<VkVertexInputAttributeDescription> attributes(attributeValues.size() / 4);
    for (size_t i = 0; i < attributes.size(); i++)
    {
        attributes[i].binding = static_cast<uint32_t>(attributeValues[i * 4]);
        attributes[i].location = static_cast<uint32_t>(attributeValues[i * 4 + 1]);
        attributes[i].format = static_cast<VkFormat>(attributeValues[i * 4 + 2]);
        attributes[i].offset = static_cast<uint32_t>(attributeValues[i * 4 + 3]);
    }
    std::vector<VkVertexInputBindingDescription> bindings(bindingValues.size() / 3);
    for (size_t i = 0; i < bindings.size(); i++)
    {
        bindings[i].binding = static_cast<uint32_t>(bindingValues[i * 3]);
        bindings[i].stride = static_cast<uint32_t>(bindingValues[i * 3 + 1]);
        bindings[i].inputRate = static_cast<VkVertexInputRate>(bindingValues[i * 3 + 2]);
    }

    VkPipelineVertexInputStateCreateInfo vertexInput = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
    vertexInput.pVertexAttributeDescriptions = attributes.empty() ? nullptr : attributes.data();
    vertexInput.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
    vertexInput.pVertexBindingDescriptions = bindings.empty() ? nullptr : bindings.data();

    std::vector<VkPipelineShaderStageCreateInfo> stages(stageValues.size());
    for (size_t i = 0; i < stages.size(); i++)
    {
        stages[i] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        stages[i].stage = static_cast<VkShaderStageFlagBits>(stageValues[i]);
        stages[i].module = i < moduleValues.size() ? FromJava<VkShaderModule>(moduleValues[i]) : VK_NULL_HANDLE;
        stages[i].pName = "main";
    }

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = static_cast<VkPrimitiveTopology>(state[0]);

    VkPipelineViewportStateCreateInfo viewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = static_cast<uint32_t>(state[1]);
    viewportState.scissorCount = static_cast<uint32_t>(state[2]);

    VkPipelineRasterizationStateCreateInfo rasterization = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterization.polygonMode = static_cast<VkPolygonMode>(state[3]);
    rasterization.cullMode = static_cast<VkCullModeFlags>(state[4]);
    rasterization.frontFace = static_cast<VkFrontFace>(state[5]);
    rasterization.lineWidth = lineWidth;
    rasterization.depthClampEnable = VK_FALSE;
    rasterization.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisample = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisample.rasterizationSamples = static_cast<VkSampleCountFlagBits>(state[6]);

    VkPipelineDepthStencilStateCreateInfo depthStencil = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    bool depthTest = state[7] != 0;
    depthStencil.depthTestEnable = depthTest ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = state[8] ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = static_cast<VkCompareOp>(state[9]);
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = state[10] ? VK_TRUE : VK_FALSE;

    std::vector<VkPipelineColorBlendAttachmentState> blends(blendValues.size() / 8);
    for (size_t i = 0; i < blends.size(); i++)
    {
        const jint* value = &blendValues[i * 8];
        blends[i].colorWriteMask = static_cast<VkColorComponentFlags>(value[0]);
        blends[i].blendEnable = value[1] ? VK_TRUE : VK_FALSE;
        blends[i].srcColorBlendFactor = static_cast<VkBlendFactor>(value[2]);
        blends[i].dstColorBlendFactor = static_cast<VkBlendFactor>(value[3]);
        blends[i].colorBlendOp = static_cast<VkBlendOp>(value[4]);
        blends[i].srcAlphaBlendFactor = static_cast<VkBlendFactor>(value[5]);
        blends[i].dstAlphaBlendFactor = static_cast<VkBlendFactor>(value[6]);
        blends[i].alphaBlendOp = static_cast<VkBlendOp>(value[7]);
    }
    VkPipelineColorBlendStateCreateInfo colorBlend = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlend.attachmentCount = static_cast<uint32_t>(blends.size());
    colorBlend.pAttachments = blends.empty() ? nullptr : blends.data();

    std::vector<VkDynamicState> dynamicStates;
    for (jint value : dynamicValues)
        dynamicStates.push_back(static_cast<VkDynamicState>(value));
    VkPipelineDynamicStateCreateInfo dynamicState = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.empty() ? nullptr : dynamicStates.data();

    VkGraphicsPipelineCreateInfo createInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    createInfo.stageCount = static_cast<uint32_t>(stages.size());
    createInfo.pStages = stages.data();
    createInfo.pVertexInputState = &vertexInput;
    createInfo.pInputAssemblyState = &inputAssembly;
    createInfo.pViewportState = &viewportState;
    createInfo.pRasterizationState = &rasterization;
    createInfo.pMultisampleState = &multisample;
    createInfo.pDepthStencilState = depthTest ? &depthStencil : nullptr;
    createInfo.pColorBlendState = &colorBlend;
    createInfo.pDynamicState = &dynamicState;
    createInfo.layout = FromJava<VkPipelineLayout>(layout);
    createInfo.renderPass = FromJava<VkRenderPass>(renderPass);

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkResult result = vkCreateGraphicsPipelines(FromJava<VkDevice>(device), FromJava<VkPipelineCache>(cache), 1, &createInfo, nullptr, &pipeline);
    if (result != VK_SUCCESS)
    {
        NativeVulkanLog("vkCreateGraphicsPipelines failed: %s", NativeVulkanResultName(result));
        return 0;
    }
    return ToJava(pipeline);
}

extern "C" JNIEXPORT jlong JNICALL NATIVE_VULKAN(NativeVulkanComputePipeline, CreateComputePipeline)(JNIEnv*, jclass, jlong device, jlong cache,
        jlong layout, jint stage, jlong module)
{
    VkComputePipelineCreateInfo createInfo = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    createInfo.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    createInfo.stage.stage = static_cast<VkShaderStageFlagBits>(stage);
    createInfo.stage.module = FromJava<VkShaderModule>(module);
    createInfo.stage.pName = "main";
    createInfo.layout = FromJava<VkPipelineLayout>(layout);

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkResult result = vkCreateComputePipelines(FromJava<VkDevice>(device), FromJava<VkPipelineCache>(cache), 1, &createInfo, nullptr, &pipeline);
    if (result != VK_SUCCESS)
    {
        NativeVulkanLog("vkCreateComputePipelines failed: %s", NativeVulkanResultName(result));
        return 0;
    }
    return ToJava(pipeline);
}

extern "C" JNIEXPORT void JNICALL NATIVE_VULKAN(NativeVulkanPipeline, DestroyPipeline)(JNIEnv*, jclass, jlong device, jlong pipeline)
{
    vkDestroyPipeline(FromJava<VkDevice>(device), FromJava<VkPipeline>(pipeline), nullptr);
}
