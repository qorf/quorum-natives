// Generated from VulkanPhysicalDeviceLimits.java: how the native strategy's
// arrays are filled, in the order of its Get actions. Limits that are
// arrays take one place per element.

#pragma once

#include <jni.h>
#include <vulkan/vulkan.h>

static const int NATIVE_VULKAN_LIMIT_INTEGERS = 83;
static const int NATIVE_VULKAN_LIMIT_NUMBERS = 13;

// Fills the integer (including true/false) and number limits, in order.
static void NativeVulkanFillLimits(const VkPhysicalDeviceLimits& limits, jlong* integers, jdouble* numbers)
{
    int i = 0;
    int n = 0;
    integers[i++] = (jlong)limits.maxImageDimension1D;
    integers[i++] = (jlong)limits.maxImageDimension2D;
    integers[i++] = (jlong)limits.maxImageDimension3D;
    integers[i++] = (jlong)limits.maxImageArrayLayers;
    integers[i++] = (jlong)limits.maxTexelBufferElements;
    integers[i++] = (jlong)limits.maxUniformBufferRange;
    integers[i++] = (jlong)limits.maxStorageBufferRange;
    integers[i++] = (jlong)limits.maxPushConstantsSize;
    integers[i++] = (jlong)limits.maxMemoryAllocationCount;
    integers[i++] = (jlong)limits.maxSamplerAllocationCount;
    integers[i++] = (jlong)limits.maxPerStageDescriptorSamplers;
    integers[i++] = (jlong)limits.maxPerStageDescriptorUniformBuffers;
    integers[i++] = (jlong)limits.maxPerStageDescriptorStorageBuffers;
    integers[i++] = (jlong)limits.maxPerStageDescriptorSampledImages;
    integers[i++] = (jlong)limits.maxPerStageDescriptorStorageImages;
    integers[i++] = (jlong)limits.maxPerStageDescriptorInputAttachments;
    integers[i++] = (jlong)limits.maxPerStageResources;
    integers[i++] = (jlong)limits.maxDescriptorSetSamplers;
    integers[i++] = (jlong)limits.maxDescriptorSetUniformBuffers;
    integers[i++] = (jlong)limits.maxDescriptorSetUniformBuffersDynamic;
    integers[i++] = (jlong)limits.maxDescriptorSetStorageBuffers;
    integers[i++] = (jlong)limits.maxDescriptorSetStorageBuffersDynamic;
    integers[i++] = (jlong)limits.maxDescriptorSetStorageImages;
    integers[i++] = (jlong)limits.maxDescriptorSetInputAttachments;
    integers[i++] = (jlong)limits.maxVertexInputAttributes;
    integers[i++] = (jlong)limits.maxVertexInputBindings;
    integers[i++] = (jlong)limits.maxVertexInputAttributeOffset;
    integers[i++] = (jlong)limits.maxVertexInputBindingStride;
    integers[i++] = (jlong)limits.maxVertexOutputComponents;
    integers[i++] = (jlong)limits.maxTessellationGenerationLevel;
    integers[i++] = (jlong)limits.maxTessellationPatchSize;
    integers[i++] = (jlong)limits.maxTessellationControlPerVertexInputComponents;
    integers[i++] = (jlong)limits.maxTessellationControlPerVertexOutputComponents;
    integers[i++] = (jlong)limits.maxTessellationControlPerPatchOutputComponents;
    integers[i++] = (jlong)limits.maxTessellationControlTotalOutputComponents;
    integers[i++] = (jlong)limits.maxTessellationEvaluationInputComponents;
    integers[i++] = (jlong)limits.maxTessellationEvaluationOutputComponents;
    integers[i++] = (jlong)limits.maxGeometryShaderInvocations;
    integers[i++] = (jlong)limits.maxGeometryInputComponents;
    integers[i++] = (jlong)limits.maxGeometryOutputComponents;
    integers[i++] = (jlong)limits.maxGeometryOutputVertices;
    integers[i++] = (jlong)limits.maxGeometryTotalOutputComponents;
    integers[i++] = (jlong)limits.maxFragmentInputComponents;
    integers[i++] = (jlong)limits.maxFragmentOutputAttachments;
    integers[i++] = (jlong)limits.maxFragmentDualSrcAttachments;
    integers[i++] = (jlong)limits.maxFragmentCombinedOutputResources;
    integers[i++] = (jlong)limits.maxComputeSharedMemorySize;
    integers[i++] = (jlong)limits.maxComputeWorkGroupCount[0];
    integers[i++] = (jlong)limits.maxComputeWorkGroupCount[1];
    integers[i++] = (jlong)limits.maxComputeWorkGroupCount[2];
    integers[i++] = (jlong)limits.maxComputeWorkGroupInvocations;
    integers[i++] = (jlong)limits.maxComputeWorkGroupSize[0];
    integers[i++] = (jlong)limits.maxComputeWorkGroupSize[1];
    integers[i++] = (jlong)limits.maxComputeWorkGroupSize[2];
    integers[i++] = (jlong)limits.subPixelPrecisionBits;
    integers[i++] = (jlong)limits.subTexelPrecisionBits;
    integers[i++] = (jlong)limits.mipmapPrecisionBits;
    integers[i++] = (jlong)limits.maxDrawIndexedIndexValue;
    integers[i++] = (jlong)limits.maxDrawIndirectCount;
    numbers[n++] = limits.maxSamplerLodBias;
    numbers[n++] = limits.maxSamplerAnisotropy;
    integers[i++] = (jlong)limits.maxViewports;
    integers[i++] = (jlong)limits.maxViewportDimensions[0];
    integers[i++] = (jlong)limits.maxViewportDimensions[1];
    numbers[n++] = limits.viewportBoundsRange[0];
    numbers[n++] = limits.viewportBoundsRange[1];
    integers[i++] = (jlong)limits.viewportSubPixelBits;
    integers[i++] = (jlong)limits.minTexelOffset;
    integers[i++] = (jlong)limits.maxTexelOffset;
    integers[i++] = (jlong)limits.minTexelGatherOffset;
    integers[i++] = (jlong)limits.maxTexelGatherOffset;
    numbers[n++] = limits.minInterpolationOffset;
    numbers[n++] = limits.maxInterpolationOffset;
    integers[i++] = (jlong)limits.subPixelInterpolationOffsetBits;
    integers[i++] = (jlong)limits.maxFramebufferWidth;
    integers[i++] = (jlong)limits.maxFramebufferHeight;
    integers[i++] = (jlong)limits.maxFramebufferLayers;
    integers[i++] = (jlong)limits.maxColorAttachments;
    integers[i++] = (jlong)limits.maxSampleMaskWords;
    numbers[n++] = limits.timestampPeriod;
    integers[i++] = (jlong)limits.maxClipDistances;
    integers[i++] = (jlong)limits.maxCullDistances;
    integers[i++] = (jlong)limits.maxCombinedClipAndCullDistances;
    integers[i++] = (jlong)limits.discreteQueuePriorities;
    numbers[n++] = limits.pointSizeRange[0];
    numbers[n++] = limits.pointSizeRange[1];
    numbers[n++] = limits.lineWidthRange[0];
    numbers[n++] = limits.lineWidthRange[1];
    numbers[n++] = limits.pointSizeGranularity;
    numbers[n++] = limits.lineWidthGranularity;
    integers[i++] = (jlong)limits.framebufferColorSampleCounts;
    integers[i++] = (jlong)limits.framebufferDepthSampleCounts;
    integers[i++] = (jlong)limits.framebufferStencilSampleCounts;
    integers[i++] = (jlong)limits.strictLines;
    integers[i++] = (jlong)limits.standardSampleLocations;
    integers[i++] = (jlong)limits.maxDescriptorSetSampledImages;
}
