#pragma once

#include <vke/fwd.hpp>
#include <vulkan/vulkan.h>

#include <vector>


namespace vke {

struct CompiledShader {
public:
    std::vector<u32> spv;
    VkShaderStageFlagBits stage;
};

class IGlslIncludeResolver;
class PipelineDescription;

class ShaderCompiler {
public:
    ShaderCompiler();
    ~ShaderCompiler();

    std::span<const std::shared_ptr<IGlslIncludeResolver>> get_includers() const { return m_include_resolver; }
    std::vector<CompiledShader> compile_shaders(PipelineDescription* description);
    void add_system_include_dir(std::string_view dir);

private:
    CompiledShader compile_glsl(const std::string& file_path, const std::unordered_map<std::string, std::string>& flags);

private:
    std::vector<std::shared_ptr<IGlslIncludeResolver>> m_include_resolver;
};

} // namespace vke