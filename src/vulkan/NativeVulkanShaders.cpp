// Shaders on iOS and Android: glslang compiles GLSL to SPIR-V, as shaderc
// (which is built on glslang) does on the desktop, with shaderc's defaults:
// Vulkan 1.0 and SPIR-V 1.0, Vulkan's rules, and no optimization. Then the
// SPIR-V becomes a shader module.

#include "NativeVulkan.h"

#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/resource_limits_c.h>

#include <mutex>

namespace
{
    std::once_flag glslangStarted;

    // shaderc's shader kinds, as SPIRVShaderCompiler passes them, as glslang's stages.
    bool StageFor(jint kind, glslang_stage_t& stage)
    {
        switch (kind)
        {
            case 0: stage = GLSLANG_STAGE_VERTEX; return true;
            case 1: stage = GLSLANG_STAGE_FRAGMENT; return true;
            case 2: stage = GLSLANG_STAGE_COMPUTE; return true;
            case 3: stage = GLSLANG_STAGE_GEOMETRY; return true;
            case 4: stage = GLSLANG_STAGE_TESSCONTROL; return true;
            case 5: stage = GLSLANG_STAGE_TESSEVALUATION; return true;
            default: return false;
        }
    }

    std::string Text(const char* value)
    {
        return value == nullptr ? std::string() : std::string(value);
    }
}

// Returns the SPIR-V, or throws a RuntimeException with glslang's messages.
extern "C" JNIEXPORT jbyteArray JNICALL NATIVE_VULKAN(NativeVulkanShaders, CompileShader)(JNIEnv* env, jclass, jstring nameString,
        jstring codeString, jint kind)
{
    std::call_once(glslangStarted, []() { glslang_initialize_process(); });

    const char* nameCharacters = env->GetStringUTFChars(nameString, nullptr);
    std::string name = nameCharacters;
    env->ReleaseStringUTFChars(nameString, nameCharacters);

    glslang_stage_t stage;
    if (!StageFor(kind, stage))
    {
        NativeVulkanThrow(env, "Failed to compile shader \"" + name + "\": its kind (" + std::to_string(kind) + ") can't be compiled on this device.");
        return nullptr;
    }

    const char* codeCharacters = env->GetStringUTFChars(codeString, nullptr);
    std::string code = codeCharacters;
    env->ReleaseStringUTFChars(codeString, codeCharacters);

    glslang_input_t input = {};
    input.language = GLSLANG_SOURCE_GLSL;
    input.stage = stage;
    input.client = GLSLANG_CLIENT_VULKAN;
    input.client_version = GLSLANG_TARGET_VULKAN_1_0;
    input.target_language = GLSLANG_TARGET_SPV;
    input.target_language_version = GLSLANG_TARGET_SPV_1_0;
    input.code = code.c_str();
    input.default_version = 110;
    input.default_profile = GLSLANG_NO_PROFILE;
    input.force_default_version_and_profile = false;
    input.forward_compatible = false;
    input.messages = static_cast<glslang_messages_t>(GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT);
    input.resource = glslang_default_resource();

    glslang_shader_t* shader = glslang_shader_create(&input);
    if (!glslang_shader_preprocess(shader, &input) || !glslang_shader_parse(shader, &input))
    {
        std::string log = Text(glslang_shader_get_info_log(shader));
        glslang_shader_delete(shader);
        NativeVulkanThrow(env, "Failed to compile shader \"" + name + "\": " + log);
        return nullptr;
    }

    glslang_program_t* program = glslang_program_create();
    glslang_program_add_shader(program, shader);
    if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT))
    {
        std::string log = Text(glslang_program_get_info_log(program));
        glslang_program_delete(program);
        glslang_shader_delete(shader);
        NativeVulkanThrow(env, "Failed to link shader \"" + name + "\": " + log);
        return nullptr;
    }

    glslang_program_SPIRV_generate(program, stage);
    size_t words = glslang_program_SPIRV_get_size(program);
    const unsigned int* spirv = glslang_program_SPIRV_get_ptr(program);
    std::string messages = Text(glslang_program_SPIRV_get_messages(program));
    if (!messages.empty())
        NativeVulkanLog("Shader \"%s\": %s", name.c_str(), messages.c_str());

    jbyteArray result = env->NewByteArray(static_cast<jsize>(words * 4));
    if (result != nullptr && words > 0)
        env->SetByteArrayRegion(result, 0, static_cast<jsize>(words * 4), reinterpret_cast<const jbyte*>(spirv));

    glslang_program_delete(program);
    glslang_shader_delete(shader);
    return result;
}

extern "C" JNIEXPORT jlong JNICALL NATIVE_VULKAN(NativeVulkanShaders, CreateShaderModule)(JNIEnv* env, jclass, jlong device, jbyteArray codeArray)
{
    jsize size = env->GetArrayLength(codeArray);
    // SPIR-V is read as 32-bit words, so it's copied to memory aligned for them.
    std::vector<uint32_t> code((static_cast<size_t>(size) + 3) / 4);
    if (size > 0)
        env->GetByteArrayRegion(codeArray, 0, size, reinterpret_cast<jbyte*>(code.data()));

    VkShaderModuleCreateInfo createInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    createInfo.codeSize = static_cast<size_t>(size);
    createInfo.pCode = code.data();

    VkShaderModule module = VK_NULL_HANDLE;
    VkResult result = vkCreateShaderModule(FromJava<VkDevice>(device), &createInfo, nullptr, &module);
    if (result != VK_SUCCESS)
    {
        NativeVulkanLog("vkCreateShaderModule failed: %s", NativeVulkanResultName(result));
        return 0;
    }
    return ToJava(module);
}
